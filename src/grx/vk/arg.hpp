#pragma once

#include <core/io/mmap.hpp>
#include <set>

#include <grx/vk/swapchain.cg.hpp>

#include <grx/vk/macro.hpp>

namespace vk {
using swapchain_create_info = swapchain_create_info_khr;

namespace arg {
    template <typename T>
    struct fill_with_t;

    template <>
    struct fill_with_t<component_mapping> {
        constexpr component_mapping operator()(component_swizzle value) const {
            return {
                .r = value,
                .g = value,
                .b = value,
                .a = value,
            };
        }
    };

    template <typename T>
    static inline constexpr fill_with_t<T> fill_with;

    constexpr struct _identity {
        constexpr operator component_mapping() const {
            return fill_with<component_mapping>(component_swizzle::identity);
        }
    } identity;

    TBC_VK_DEF_INFO_ARG(swapchain_create_info, surface, surface);
    TBC_VK_DEF_INFO_ARG(swapchain_create_info, min_image_count, min_image_count);
    TBC_VK_DEF_INFO_ARG(swapchain_create_info, pre_transform, pre_transform);
    TBC_VK_DEF_INFO_ARG(swapchain_create_info, composite_alpha, composite_alpha);
    TBC_VK_DEF_INFO_ARG(swapchain_create_info, present_mode, present_mode);
    TBC_VK_DEF_INFO_ARG(swapchain_create_info, clipped, clipped);
    TBC_VK_DEF_INFO_ARG(swapchain_create_info, old_swapchain, old_swapchain);

    struct swapchain_image_info {
        vk::surface_format_khr format;
        vk::extent2d           extent;
        u32                    array_layers;
        vk::image_usage_flags  usage;
    };

    constexpr struct _image_info {
        constexpr auto operator=(swapchain_image_info&& value) const {
            struct _result {
                constexpr void visit(swapchain_create_info& info) {
                    info.image_format = value.format.format;
                    info.image_color_space = value.format.color_space;
                    info.image_extent = value.extent;
                    info.image_array_layers = value.array_layers;
                    info.image_usage = value.usage;
                }
                swapchain_image_info& value;
            };
            return _result{value};
        }
    } image_info;

    constexpr struct _queue_family_indices {
        constexpr auto operator=(std::vector<u32> values) const {
            struct _result {
                void visit(swapchain_create_info& info) {
                    std::set<u32> uniq{values.begin(), values.end()};

                    if (uniq.size() != 1) {
                        info.image_sharing_mode       = vk::sharing_mode::concurrent;
                        info.queue_family_index_count = u32(values.size());
                        info.queue_family_indices     = values.data();
                    }
                    else {
                        info.image_sharing_mode = vk::sharing_mode::exclusive;
                    }
                }
                std::vector<u32> values;
            };
            return _result{core::mov(values)};
        }
    } queue_family_indices;

    struct spirv_file {
        spirv_file(const std::string& path):
            file{core::io::mmap{
                core::io::file::open(path, core::io::openflags::read_only),
                core::io::map_flags::priv,
                core::io::map_prots::read,
            }} {}

        operator shader_module_create_info() const {
            return {
                .code_size = file.size(),
                .code      = (const u32*)file.data(), // NOLINT
            };
        }

        core::io::mmap<core::io::file> file;
    };
} // namespace arg
} // namespace vk
