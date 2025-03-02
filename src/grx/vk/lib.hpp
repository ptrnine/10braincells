#pragma once

#include <dlfcn.h>

#include <vector>

#include <core/opt.hpp>
#include <core/string_builder.hpp>
#include <grx/vk/commands.hpp>
#include <grx/vk/result.hpp>

namespace vk
{
class vk_lib {
public:
    using lib_t = core::moveonly_trivial<void*, nullptr>;

    vk_lib(const std::string& lib_path): _lib(dlopen(lib_path.data(), RTLD_LAZY | RTLD_LOCAL)) {
        cmd::load_from_lib(_lib,
                           _get_instance_proc_addr,
                           _enumerate_instance_extension_properties,
                           _enumerate_instance_version,
                           _enumerate_instance_layer_properties,
                           _create_instance,
                           _destroy_instance);
    }

    ~vk_lib() {
        if (_lib.not_default())
            dlclose(_lib.get());
    }

    std::vector<extension_properties> extensions(const core::opt<std::string>& layer_name = core::null) const {
        auto layer = layer_name ? layer_name->data() : nullptr;

        std::vector<extension_properties> res;
        u32                               count;
        _enumerate_instance_extension_properties.call(layer, &count, nullptr);
        res.resize(count);
        _enumerate_instance_extension_properties.call(layer, &count, res.data());
        return res;
    }

    vk::version version() const {
        version_raw ver;
        _enumerate_instance_version.call((u32*)&ver);
        return ver.get();
    }

    std::vector<layer_properties> layers() const {
        u32 count;
        _enumerate_instance_layer_properties.call(&count, nullptr);
        std::vector<layer_properties> res;
        _enumerate_instance_layer_properties.call(&count, res.data());
        return res;
    }

    auto create_instance(const instance_create_info&     create_info,
                         core::opt<allocation_callbacks> allocator = core::null) const;

    auto create_instance_raw(const instance_create_info& create_info,
                             const allocation_callbacks* allocator = nullptr) const {
        auto res = _create_instance.result_type(core::type<instance>).make();
        res.rc   = _create_instance.call(&create_info, allocator, &res.value_unsafe);
        return res;
    }

    void destroy_instance_raw(instance instance, const allocation_callbacks* allocator = nullptr) const {
        _destroy_instance.call(instance, allocator);
    }

    void load_functions(vk::instance instance, auto&... func_holders) const {
        auto load = [&](auto& func_holder) {
            func_holder.call = (typename core::remove_ref<decltype(func_holder)>::type)_get_instance_proc_addr.call(
                instance, func_holder.name);
            if (!func_holder.call)
                cmd::load_from_lib(_lib, func_holder);
        };
        (load(func_holders), ...);
    }

private:
    lib_t                                          _lib;
    cmd::get_instance_proc_addr_t                  _get_instance_proc_addr;
    cmd::enumerate_instance_extension_properties_t _enumerate_instance_extension_properties;
    cmd::enumerate_instance_version_t              _enumerate_instance_version;
    cmd::enumerate_instance_layer_properties_t     _enumerate_instance_layer_properties;
    cmd::create_instance_t                         _create_instance;
    cmd::destroy_instance_t                        _destroy_instance;
};
} // namespace vk
