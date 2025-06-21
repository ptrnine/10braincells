#pragma once

#include <grx/vk/extension_defines.hpp>

#include <grx/vk/commands.cg.hpp>
#include <grx/vk/flags.cg.hpp>
#include <grx/vk/function_types.cg.hpp>
#include <grx/vk/structs.cg.hpp>
#include <grx/vk/types.cg.hpp>

#include <map>
#include <util/assert.hpp>

#include <grx/vk/lib.hpp>

namespace vk {
class instance_t {
public:
    using inst_t = core::moveonly_trivial<vk::instance, nullptr>;

    instance_t(const vk_lib&                   library,
               const instance_create_info&     create_info,
               log_level                       log_level = log_level::debug,
               core::opt<allocation_callbacks> allocator = core::null):
        lib(&library), inst(init(&library, create_info, log_level, allocator)) {

        f.pass_to([&](auto&... functions) {
            load_functions(functions...);
        });

        create_debug_callback(log_level);
    }

    ~instance_t() {
        destroy_debug_callback();

        if (inst.not_default())
            lib->destroy_instance_raw(inst);
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

private:
    inst_t init(const vk_lib*                   lib,
                const instance_create_info&     create_info,
                log_level                       log_level,
                core::opt<allocation_callbacks> allocator) {
        instance_create_info info = create_info;

        auto debug_report_info = create_debug_utils_info(log_level);
        if (log_level != log_level::off) {
            R_ASSERT(info.next == nullptr);
            info.next = &debug_report_info;
        }

        return lib->create_instance_raw(info, allocator ? &(*allocator) : nullptr).value();
    }

    debug_utils_messenger_create_info_ext create_debug_utils_info(log_level log_level) {
        debug_utils_messenger_create_info_ext result{};

        result.message_severity = {};
        switch (log_level) {
        case log_level::debug: result.message_severity |= debug_utils_message_severity_ext_flag::verbose_ext;
        case log_level::info: result.message_severity |= debug_utils_message_severity_ext_flag::info_ext;
        case log_level::performance_warning:
        case log_level::warning: result.message_severity |= debug_utils_message_severity_ext_flag::warning_ext;
        case log_level::error: result.message_severity |= debug_utils_message_severity_ext_flag::error_ext;
        default: break;
        }

        result.message_type = {};
        switch (log_level) {
        case log_level::debug:;
        case log_level::info: result.message_type |= debug_utils_message_type_ext_flag::validation_ext;
        case log_level::performance_warning: result.message_type |= debug_utils_message_type_ext_flag::performance_ext;
        case log_level::warning:
        case log_level::error: result.message_type |= debug_utils_message_type_ext_flag::general_ext;
        default: break;
        }

        util::glog().info("create debug utils callback: {} {}", result.message_severity, result.message_type);
        result.user_callback = debug_utils_callback;

        return result;
    }

    void create_debug_callback(log_level level) {
        if (level == log_level::off) {
            return;
        }

        debug_report_ext_flags flags = {};
        switch (level) {
        case log_level::debug: flags |= debug_report_ext_flag::debug_ext;
        case log_level::info: flags |= debug_report_ext_flag::information_ext;
        case log_level::performance_warning: flags |= debug_report_ext_flag::performance_warning_ext;
        case log_level::warning: flags |= debug_report_ext_flag::warning_ext;
        case log_level::error: flags |= debug_report_ext_flag::error_ext;
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

auto vk_lib::create_instance(const instance_create_info& create_info, log_level log_level, core::opt<allocation_callbacks> allocator) const {
    return instance_t{*this, create_info, log_level, allocator};
}
} // namespace vk
