#pragma once

#include <grx/vk/extension_defines.hpp>

#include <grx/vk/commands.cg.hpp>
#include <grx/vk/flags.cg.hpp>
#include <grx/vk/function_types.cg.hpp>
#include <grx/vk/structs.cg.hpp>
#include <grx/vk/types.cg.hpp>

#include <config.hpp>
#include <core/ranges/zip.hpp>
#include <core/assert.hpp>
#include <core/function.hpp>

#include <grx/vk/lib.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace vk {

class instance_t;

namespace details {
class instances_limit_reached : public std::exception {
public:
    instances_limit_reached() = default;
    const char* what() const noexcept override {
        return "Cannot create new vulkan instance because instances limit reached. You can modify max_instances in src/cfg.hpp";
    }
};

struct instance_manager {
    static inline constexpr u32 no_id = ~u32(0);

    static instance_manager& manager() {
        static instance_manager inst;
        return inst;
    }

    instance_manager() {
        for (auto&& [idx, id] : core::with_index(ids)) {
            id = u32(idx);
        }
    }

    u32 alloc() {
        std::lock_guard lock{mtx};

        for (auto& id : ids) {
            if (id != no_id) {
                auto result = id;
                id = no_id;
                util::glog().debug("Create new vulkan instance with id={}", result);
                return result;
            }
        }
        throw instances_limit_reached{};
    }

    void free(u32 free_id) {
        std::lock_guard lock{mtx};

        for (auto& id : ids) {
            if (id == no_id) {
                util::glog().debug("Free vulkan instance with id={}", free_id);
                for (auto& f : destroy_handlers[free_id])
                    f();
                destroy_handlers[free_id].clear();
                id = free_id;
                return;
            }
        }
        tbc_assert(false && !"Invalid instance_manager::free() call");
    }

    void add_destroy_handler(u32 id, auto&& handler) {
        destroy_handlers[id].emplace_back(fwd(handler));
    }

    core::array<u32, cfg::vk::max_instances>                                    ids;
    core::array<std::vector<core::function<void(), 8>>, cfg::vk::max_instances> destroy_handlers;
    std::mutex                                                                  mtx;
};

template <typename... Fs>
struct function_loader {
    static function_loader& loader() {
        static function_loader inst;
        return inst;
    }

    struct func_block {
        void               load(const instance_t& inst);
        core::tuple<Fs...> f;
        bool               loaded = false;
    };

    core::tuple<Fs...>* functions(const instance_t& inst);

    core::array<func_block, cfg::vk::max_instances> fs;
};
}

class instance_logger {
public:
    instance_logger(const instance_t& inst): instance(&inst) {}

    void log(util::log_level level, std::string_view format, auto&&... args);
    void log_update(util::log_level level, u16 update_id, std::string_view format, auto&&... args);

    bool will_be_logged(util::log_level level) const;

#define def_log_func(level)                                                                   \
    template <typename... Ts>                                                                 \
    void level(std::string_view format_str, Ts&&... args) {                                   \
        log(util::log_level::level, format_str, std::forward<Ts>(args)...);                   \
    }                                                                                         \
    template <typename... Ts>                                                                 \
    void level##_update(u16 update_id, std::string_view format_str, Ts&&... args) {           \
        log_update(util::log_level::level, update_id, format_str, std::forward<Ts>(args)...); \
    }

    def_log_func(debug)
    def_log_func(detail)
    def_log_func(info)
    def_log_func(warn)
    def_log_func(error)
#undef def_log_func

private:
    const instance_t* instance = nullptr;
};

class instance_t {
public:
    using inst_t = core::moveonly_trivial<vk::instance, nullptr>;

    instance_t() = default;

    instance_t(const vk_lib& library, info::instance create_info, core::opt<allocation_callbacks> allocator = core::null):
        _id(details::instance_manager::manager().alloc()),
        lib(&library),
        inst(init(&library, core::mov(create_info), allocator)),
        log_sev(create_info.log_severity) {
        f.pass_to([&](auto&... functions) { load_functions(functions...); });

        if (create_info.log_severity != log_severity::off) {
            create_debug_callback(create_info.log_severity);
        }
    }

    ~instance_t() {
        destroy();
    }

    instance_t(instance_t&& instance) noexcept: _id(instance._id), lib(instance.lib), inst(core::mov(instance.inst)), f(core::mov(instance.f)), log_sev(instance.log_sev) {
        std::lock_guard lock{mtx};

        instance._id = details::instance_manager::no_id;

        // NOLINTNEXTLINE
        func_cache = core::mov(instance.func_cache);
        // NOLINTNEXTLINE
        deb_callback = instance.deb_callback;
    }

    instance_t& operator=(instance_t&& instance) noexcept {
        if (this == &instance) {
            return *this;
        }

        std::scoped_lock lock{mtx, instance.mtx};

        destroy();
        _id          = instance._id;
        instance._id = details::instance_manager::no_id;
        lib          = instance.lib;
        inst         = core::mov(instance.inst);
        f            = core::mov(instance.f);
        func_cache   = core::mov(instance.func_cache);
        deb_callback = instance.deb_callback;
        log_sev      = instance.log_sev;

        return *this;
    }

    void load_functions(auto&... func_holders) const {
        lib->load_functions(inst, func_holders...);
    }

    /* TODO: lazy load */
    void load_functions_cached(auto&... func_holders) const {
        auto load = [&](auto& func_holder) {
            if constexpr (!core::is_same<core::remove_cvref<decltype(func_holder)>, core::null_t>) {
                auto [pos, insert] = func_cache.emplace(func_holder.name, nullptr);
                if (insert) {
                    load_functions(func_holder);
                    pos->second = (void*)func_holder.call;
                }
                else {
                    func_holder.call = (typename core::remove_cvref<decltype(func_holder)>::type)pos->second;
                }
            }
        };

        std::lock_guard lock{mtx};
        (load(func_holders), ...);
    }

    template <typename... Fs>
    auto get_functions() const {
        return details::function_loader<Fs...>::loader().functions(*this);
    }

    auto physical_devices_raw() const {
        auto func = f[cmd::enumerate_physical_devices];

        u32 count;
        func.call(inst, &count, nullptr);

        auto res = func.result_type(core::type<std::vector<physical_device>>).make();
        res.value_unsafe.resize(count);

        res.rc = func.call(inst, &count, res.value_unsafe.data());
        return res;
    }

    void destroy_device_raw(vk::device device, core::opt<allocation_callbacks> allocator = core::null) const {
        f[cmd::destroy_device].call(device, allocator ? &*allocator : nullptr);
    }

    void set_debug_report_callback(const debug_report_callback_create_info_ext& info,
                                   core::opt<allocation_callbacks>              allocator = core::null) {
        f[cmd::create_debug_report_callback_ext].call(inst, &info, allocator ? &*allocator : nullptr, &deb_callback);
    }

    instance handle() const {
        return inst;
    }

    auto physical_devices() const;

    explicit operator bool() const noexcept {
        return inst.not_default();
    }

    u32 id() const {
        return _id;
    }

    auto log_severity() const {
        return log_sev;
    }

    instance_logger logger() const {
        return {*this};
    }

private:
    void destroy() {
        if (inst.not_default()) {
            destroy_debug_callback();
            lib->destroy_instance_raw(inst);
            details::instance_manager::manager().free(_id);
        }
    }

    inst_t init(const vk_lib* lib, info::instance create_info, core::opt<allocation_callbacks> allocator) {
        if (create_info.log_severity != log_severity::off) {
            create_info.extensions.push_back("VK_EXT_debug_utils");
            create_info.extensions.push_back("VK_EXT_debug_report");
        }

        instance_create_info info = create_info;

        auto debug_report_info = create_debug_utils_info(create_info.log_severity);
        if (create_info.log_severity != log_severity::off) {
            info.next = &debug_report_info;
        }

        return lib->create_instance_raw(info, allocator ? &(*allocator) : nullptr).value();
    }

    debug_utils_messenger_create_info_ext create_debug_utils_info(enum log_severity log_level) {
        debug_utils_messenger_create_info_ext result{};

        result.message_severity = {};
        switch (log_level) {
        case log_severity::debug: result.message_severity |= debug_utils_message_severity_ext_flag::verbose_ext;
        case log_severity::info: result.message_severity |= debug_utils_message_severity_ext_flag::info_ext;
        case log_severity::performance_warning:
        case log_severity::warning: result.message_severity |= debug_utils_message_severity_ext_flag::warning_ext;
        case log_severity::error: result.message_severity |= debug_utils_message_severity_ext_flag::error_ext;
        default: break;
        }

        result.message_type = {};
        switch (log_level) {
        case log_severity::debug:;
        case log_severity::info: result.message_type |= debug_utils_message_type_ext_flag::validation_ext;
        case log_severity::performance_warning: result.message_type |= debug_utils_message_type_ext_flag::performance_ext;
        case log_severity::warning:
        case log_severity::error: result.message_type |= debug_utils_message_type_ext_flag::general_ext;
        default: break;
        }

        result.user_callback = debug_utils_callback;

        return result;
    }

    void create_debug_callback(enum log_severity level) {
        if (level == log_severity::off) {
            return;
        }

        debug_report_ext_flags flags = {};
        switch (level) {
        case log_severity::debug: flags |= debug_report_ext_flag::debug_ext;
        case log_severity::info: flags |= debug_report_ext_flag::information_ext;
        case log_severity::performance_warning: flags |= debug_report_ext_flag::performance_warning_ext;
        case log_severity::warning: flags |= debug_report_ext_flag::warning_ext;
        case log_severity::error: flags |= debug_report_ext_flag::error_ext;
        default: break;
        }

        debug_report_callback_create_info_ext info{
            .flags     = flags,
            .callback  = (debug_report_callback_ext)debug_callback,
            .user_data = this,
        };

        f[cmd::create_debug_report_callback_ext].call(inst, &info, nullptr, &deb_callback);
    }

    void destroy_debug_callback() {
        if (deb_callback) {
            f[cmd::destroy_debug_report_callback_ext].call(inst, deb_callback, nullptr);
        }
    }

    static bool32_t debug_callback(vk::debug_report_ext_flags       flags,
                                   vk::debug_report_object_type_ext object_type,
                                   core::u64                        object,
                                   size_t /*location*/,
                                   core::i32 /*message_code*/,
                                   const char* p_layer_prefix,
                                   const char* p_message,
                                   void* /*user_data*/) {
        details::logger_callback(util::glog(), flags, object_type, object, p_layer_prefix, p_message);
        return true;
    }

    static bool32_t debug_utils_callback(debug_utils_message_severity_ext_flags         severity,
                                         debug_utils_message_type_ext_flags             type,
                                         const debug_utils_messenger_callback_data_ext* data,
                                         void* /*user_data*/) {
        details::debug_utils_callback(util::glog(), severity, type, data);
        return true;
    }

private:
    u32           _id;
    const vk_lib* lib;
    inst_t        inst;

    core::tuple</* start */
                cmd::enumerate_physical_devices_t,
                cmd::destroy_device_t,
                cmd::create_debug_report_callback_ext_t,
                cmd::destroy_debug_report_callback_ext_t
                /* end */>
        f;

    mutable std::mutex mtx;
    /* TODO: replace with robin map */
    mutable std::map<const char*, void*> func_cache;
    debug_report_callback_ext            deb_callback = nullptr;
    enum log_severity                    log_sev;
};

void instance_logger::log(util::log_level level, std::string_view format, auto&&... args) {
    auto cur_level = details::severity_to_level(instance->log_severity());
    if (cur_level <= level) {
        util::glog().log(level, format, fwd(args)...);
    }
}

void instance_logger::log_update(util::log_level level, u16 update_id, std::string_view format, auto&&... args) {
    auto cur_level = details::severity_to_level(instance->log_severity());
    if (cur_level <= level) {
        util::glog().log_update(level, update_id, format, fwd(args)...);
    }
}

bool instance_logger::will_be_logged(util::log_level level) const {
    auto cur_level = details::severity_to_level(instance->log_severity());
    return cur_level <= level;
}

namespace details {
    template <typename... Fs>
    void function_loader<Fs...>::func_block::load(const instance_t& inst) {
        if (loaded) {
            return;
        }

        util::glog().debug("Vulkan functions loaded in {}", __PRETTY_FUNCTION__);
        f.foreach ([&](auto& f) {
            if constexpr (!core::is_same<core::decay<decltype(f)>, core::null_t>)
                inst.load_functions_cached(f);
        });

        details::instance_manager::manager().add_destroy_handler(inst.id(), [&] {
            util::glog().debug("Vulkan functions cleared in {}", __PRETTY_FUNCTION__);
            f.foreach ([](auto&& func) {
                if constexpr (!core::is_same<core::decay<decltype(func)>, core::null_t>)
                    func.call = nullptr;
            });
            loaded = false;
        });

        loaded = true;
    }

    template <typename... Fs>
    core::tuple<Fs...>* function_loader<Fs...>::functions(const instance_t& inst) {
        auto& block = fs[inst.id()];
        block.load(inst);
        return &block.f;
    }
} // namespace details

template <typename... Fs>
class function_provider {
public:
    function_provider() = default;
    function_provider(const instance_t& inst): f(inst.get_functions<Fs...>()) {}

    auto&& operator[](this auto&& it, auto func_type) {
        return (*fwd(it).f)[func_type];
    }

private:
    core::tuple<Fs...>* f = nullptr;
};


inline constexpr std::string to_string(const extension_properties& extension) {
    return std::string(extension.extension_name) + " = " + to_string(extension.spec_version);
}

namespace arg {
    constexpr struct _layers {
        constexpr auto operator=(const std::vector<std::string>& values) const {
            struct _result {
                constexpr void visit(instance_create_info& info) {
                    info.enabled_layer_count = u32(values.size());
                    info.enabled_layer_names = values.data();
                }

                constexpr void visit(device_create_info& info) {
                    info.enabled_layer_count = u32(values.size());
                    info.enabled_layer_names = values.data();
                }

                std::vector<const char*> values;
            } result;

            for (auto& v : values) {
                result.values.push_back(v.data());
            }
            return result;
        }
    } layers;

    constexpr struct _extensions {
        struct _result {
            constexpr void visit(instance_create_info& info) {
                info.enabled_extension_count = u32(values.size());
                info.enabled_extension_names = values.data();
            }

            constexpr void visit(device_create_info& info) {
                info.enabled_extension_count = u32(values.size());
                info.enabled_extension_names = values.data();
            }

            std::vector<const char*> values;
        };

        constexpr auto operator=(const std::vector<std::string>& values) const {
            _result result;
            for (auto& v : values) {
                result.values.push_back(v.data());
            }
            return result;
        }

        constexpr auto operator=(const std::vector<extension_properties>& values) const {
            _result result;
            for (auto& v : values) {
                result.values.push_back(v.extension_name);
            }
            return result;
        }
    } extensions;

    constexpr struct _features {
        struct _result {
            constexpr void visit(device_create_info& info) {
                info.enabled_features = &value;
            }

            physical_device_features value;
        };

        constexpr auto operator=(physical_device_features value) const {
            return _result{value};
        }
    } features;

    constexpr struct _appinfo {
        constexpr auto operator=(const application_info& app_info) const {
            struct _result {
                constexpr void visit(instance_create_info& info) {
                    info.application_info = &app_info;
                }

                const application_info& app_info;
            } result{app_info};
            return result;
        }
    } appinfo;

    constexpr struct _instance_flags {
        constexpr auto operator=(instance_create_flags flags) const {
            struct _result {
                constexpr void visit(instance_create_info& info) {
                    info.flags = flags;
                }
                instance_create_flags flags;
            } result{flags};
        }
    } instance_flags;
} // namespace arg

auto vk_lib::create_instance(info::instance create_info, core::opt<allocation_callbacks> allocator) const {
    return instance_t{*this, core::mov(create_info), allocator};
}

template <typename... Ts>
auto* chain_setup(core::tuple<Ts...>& chain) {
    if constexpr (chain.size() > 0) {
        core::make_idx_seq<chain.size()>().foreach ([&](auto idx) {
            if constexpr (idx + 1 < chain.size()) {
                chain[idx].next = &chain[idx + core::size_c<1>];
            }
        });
        return &chain[core::size_c<0>];
    } else {
        return (void*)nullptr;
    }
}
} // namespace vk

#undef fwd
