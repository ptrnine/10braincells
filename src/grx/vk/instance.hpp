#pragma once

#include <grx/vk/commands.hpp>
#include <map>

#include <grx/vk/lib.hpp>

namespace vk
{
class instance_t {
public:
    using inst_t = core::moveonly_trivial<vk::instance, nullptr>;

    instance_t(const vk_lib&                   library,
               const instance_create_info&     create_info,
               core::opt<allocation_callbacks> allocator = core::null):
        lib(&library), inst(lib->create_instance_raw(create_info, allocator ? &(*allocator) : nullptr).value()) {

        f.pass_to([&](auto&... functions) {
            load_functions(functions...);
        });
    }

    ~instance_t() {
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

    auto physical_devices() const;

private:
    const vk_lib* lib;
    inst_t        inst;

    core::tuple</* start */
                cmd::enumerate_physical_devices_t,
                cmd::destroy_device_t
                /* end */>
        f;

    mutable std::mutex mtx;
    /* TODO: replace with robin map */
    mutable std::map<const char*, void*> func_cache;
};

auto vk_lib::create_instance(const instance_create_info& create_info, core::opt<allocation_callbacks> allocator) const {
    return instance_t{*this, create_info, allocator};
}
} // namespace vk
