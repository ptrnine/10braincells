#pragma once

#include <vector>

#include <core/utility/overloaded.hpp>

#include <grx/vk/structs.hpp>

namespace vk
{
inline constexpr std::string to_string(const extension_properties& extension) {
    return std::string(extension.extension_name) + " = " + to_string(extension.spec_version);
}

struct enabled_layers {
    template <typename... Ts>
        requires((core::same_as<core::decay<Ts>, const char*> ||
                  core::same_as<core::remove_ref<Ts>, std::string>) &&
                 ...)
    enabled_layers(Ts&&... args) {
        core::fwd_as_tuple(core::fwd<Ts>(args)...)
            .foreach (core::overloaded{
                [&](const char* s) {
                    values.push_back(s);
                },
                [&](const std::string& s) {
                    values.push_back(s.data());
                },
            });
    }

    enabled_layers(std::span<const char*> v): values(v.begin(), v.end()) {}
    enabled_layers(std::span<const std::string> v) {
        for (auto&& s : v)
            values.push_back(s.data());
    }

    std::vector<const char*> values;
};

struct enabled_extensions {
    template <typename... Ts>
        requires((core::same_as<core::decay<Ts>, const char*> ||
                  core::same_as<core::remove_ref<Ts>, std::string>) &&
                 ...)
    enabled_extensions(Ts&&... args) {
        core::fwd_as_tuple(core::fwd<Ts>(args)...)
            .foreach (core::overloaded{
                [&](const char* s) {
                    values.push_back(s);
                },
                [&](const std::string& s) {
                    values.push_back(s.data());
                },
            });
    }

    enabled_extensions(std::span<const char*> v): values(v.begin(), v.end()) {}
    enabled_extensions(std::span<const std::string> v) {
        for (auto&& s : v)
            values.push_back(s.data());
    }

    std::vector<const char*> values;
};

struct instance_create_info_store {
    constexpr operator instance_create_info() const {
        instance_create_info res;
        res.flags                   = flags;
        res.enabled_layer_count     = u32(layers.size());
        res.enabled_layer_names     = layers.data();
        res.enabled_extension_count = u32(extensions.size());
        res.enabled_extension_names = extensions.data();
        res.application_info        = &appinfo;
        return res;
    }

    instance_create_flags    flags = {};
    std::vector<const char*> layers;
    std::vector<const char*> extensions;
    application_info         appinfo;
};

constexpr auto instance_create_info::pass(auto&&... args) {
    instance_create_info_store result;

    auto parser = core::overloaded{
        [&](instance_create_flags flags) {
            result.flags |= flags;
        },
        [&](const enabled_layers& layers) {
            result.layers = layers.values;
        },
        [&](const enabled_extensions& extensions) {
            result.extensions = extensions.values;
        },
        [&](const vk::application_info& appinfo) {
            result.appinfo = appinfo;
        },
    };

    (parser(static_cast<decltype(args)>(args)), ...);

    return result;
}
} // namespace vk
