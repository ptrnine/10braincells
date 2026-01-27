#pragma once

#include <core/tuple.hpp>
#include <grx/vk/extension_defines.hpp>

#include <core/opt.hpp>

#include <grx/basic_types.hpp>
#include <grx/vk/flags.cg.hpp>
#include <grx/vk/structs.cg.hpp>
#include <util/assert.hpp>

#include <grx/vk/uniq_string_buff.hpp>

namespace vk {
    auto data_or_null(const auto& vector) {
        return vector.size() ? vector.data() : nullptr;
    }
}

namespace vk::info {
struct device_queue {
    u32                       family_index;
    device_queue_create_flags flags = {};
    std::vector<float>        priorities = {1.f};

    operator device_queue_create_info() const {
        return {
            .flags              = flags,
            .queue_family_index = family_index,
            .queue_count        = u32(priorities.size()),
            .queue_priorities   = priorities.data(),
        };
    }

    bool operator<(const device_queue& q) const {
        return family_index < q.family_index;
    }
};

template <typename... Ts>
struct device {
    std::set<device_queue>     queues;
    details::uniq_string_buff  extensions = {};
    //physical_device_features   features   = {};
    mutable core::tuple<Ts...> chained    = {};

    mutable std::vector<device_queue_create_info> _info_queues = {};

    operator device_create_info() const {
        _info_queues = std::vector<device_queue_create_info>(queues.begin(), queues.end());

        return {
            .next                    = chain_setup(chained),
            .queue_create_info_count = u32(_info_queues.size()),
            .queue_create_infos      = _info_queues.data(),
            .enabled_layer_count     = 0,
            .enabled_layer_names     = nullptr,
            .enabled_extension_count = extensions.size(),
            .enabled_extension_names = extensions.data(),
            .enabled_features        = nullptr,
            //.enabled_features        = &features,
        };
    }
};

#ifdef VK_KHR_swapchain
struct swapchain_image {
    /** Format of the presentation images */
    surface_format_khr format;
    /** Dimensions of the presentation images */
    extent2d extent;
    /** Determines the number of views for multiview/stereo presentation */
    u32 array_layers;
    /** Bits indicating how the presentation images will be used */
    image_usage_flags usage;
    /** Sharing mode used for the presentation images */
    sharing_mode sharing_mode = sharing_mode::exclusive;
};

struct swapchain {
    /** The swapchain's target surface */
    surface_khr surface;
    /** Minimum number of presentation images the application needs */
    u32 min_image_count;

    swapchain_image image;
    /** Queue family indices having access to the images in case of concurrent sharing mode */
    std::set<u32> queue_family_indices;
    /**
     * The transform, relative to the device's natural orientation, applied to the image content prior to presentation
     */
    surface_transform_khr_flags pre_transform;
    /** The alpha blending mode used when compositing this surface with other surfaces in the window system */
    composite_alpha_khr_flags composite_alpha;
    /** Which presentation mode to use for presents on this swap chain */
    present_mode_khr present_mode;
    /** Specifies whether presentable images may be affected by window clip regions */
    bool clipped;
    /** Existing swap chain to replace, if any */
    swapchain_khr old_swapchain = {};

    mutable std::vector<u32> _info_q_family_indices = {};

    operator swapchain_create_info_khr() const {
        swapchain_create_info_khr result = {
            .surface                  = surface,
            .min_image_count          = min_image_count,
            .image_format             = image.format.format,
            .image_color_space        = image.format.color_space,
            .image_extent             = image.extent,
            .image_array_layers       = image.array_layers,
            .image_usage              = image.usage,
            .image_sharing_mode       = image.sharing_mode,
            .queue_family_index_count = 0,
            .queue_family_indices     = nullptr,
            .pre_transform            = pre_transform,
            .composite_alpha          = composite_alpha,
            .present_mode             = present_mode,
            .clipped                  = clipped,
            .old_swapchain            = old_swapchain,
        };

        if (queue_family_indices.size() > 1) {
            _info_q_family_indices.assign(queue_family_indices.begin(), queue_family_indices.end());
            result.image_sharing_mode       = sharing_mode::concurrent;
            result.queue_family_index_count = u32(_info_q_family_indices.size());
            result.queue_family_indices     = _info_q_family_indices.data();
        }

        return result;
    }
};
#endif // VK_KHR_swapchain
}

namespace vk::description {
struct subpass {
    vk::pipeline_bind_point           pipeline_bind_point = vk::pipeline_bind_point::graphics;
    std::vector<attachment_reference> input_attachments = {};
    std::vector<attachment_reference> color_attachments;
    std::vector<attachment_reference> resolve_attachments      = {};
    core::opt<attachment_reference>   depth_stencil_attachment = {};
    std::vector<u32>                  preserve_attachments     = {};

    operator subpass_description() const {
        R_ASSERT(resolve_attachments.empty() || resolve_attachments.size() == color_attachments.size());

        return {
            .pipeline_bind_point       = pipeline_bind_point,
            .input_attachment_count    = u32(input_attachments.size()),
            .input_attachments         = data_or_null(input_attachments),
            .color_attachment_count    = u32(color_attachments.size()),
            .color_attachments         = data_or_null(color_attachments),
            .resolve_attachments       = data_or_null(resolve_attachments),
            .depth_stencil_attachment  = depth_stencil_attachment ? &(*depth_stencil_attachment) : nullptr,
            .preserve_attachment_count = u32(preserve_attachments.size()),
            .preserve_attachments      = data_or_null(preserve_attachments),
        };
    }
};

using attachment             = vk::attachment_description;
using vertex_input_attribute = vk::vertex_input_attribute_description;
using vertex_input_binding   = vk::vertex_input_binding_description;
} // namespace vk::description

namespace vk::info {
struct render_pass {
    std::vector<description::attachment> attachments;
    std::vector<description::subpass>    subpasses;
    std::vector<subpass_dependency>      dependencies;

    mutable std::vector<subpass_description> _subpasses = {};

    operator render_pass_create_info() const {
        _subpasses.assign(subpasses.begin(), subpasses.end());

        return {
            .attachment_count = u32(attachments.size()),
            .attachments = data_or_null(attachments),
            .subpass_count = u32(subpasses.size()),
            .subpasses = data_or_null(_subpasses),
            .dependency_count = u32(dependencies.size()),
            .dependencies = data_or_null(dependencies),
        };
    }
};

using pipeline_shader_stage = pipeline_shader_stage_create_info;

struct pipeline_layout {
    /** Array of descriptor set layout objects defining the layout of the */
    std::vector<descriptor_set_layout> set_layouts = {};
    /** Array of ranges used by various shader stages */
    std::vector<push_constant_range> push_constant_ranges = {};

    operator pipeline_layout_create_info() const {
        return {
            .set_layout_count          = u32(set_layouts.size()),
            .set_layouts               = data_or_null(set_layouts),
            .push_constant_range_count = u32(push_constant_ranges.size()),
            .push_constant_ranges      = data_or_null(push_constant_ranges),
        };
    }
};

namespace pipeline {
    namespace state {
        using input_assembly = pipeline_input_assembly_state_create_info;
        using tessellation   = pipeline_tessellation_state_create_info;

        struct vertex_input {
            std::vector<description::vertex_input_binding>   vertex_binding_descriptions   = {};
            std::vector<description::vertex_input_attribute> vertex_attribute_descriptions = {};

            operator pipeline_vertex_input_state_create_info() const {
                return {
                    .vertex_binding_description_count   = u32(vertex_binding_descriptions.size()),
                    .vertex_binding_descriptions        = data_or_null(vertex_binding_descriptions),
                    .vertex_attribute_description_count = u32(vertex_attribute_descriptions.size()),
                    .vertex_attribute_descriptions      = data_or_null(vertex_attribute_descriptions),
                };
            }
        };

        struct viewport {
            std::vector<vk::viewport> viewports       = {};
            std::vector<vk::rect2d>   scissors        = {};
            u32                       viewport_count = 0;
            u32                       scissor_count  = 0;

            operator pipeline_viewport_state_create_info() const {
                auto v_count = std::max(u32(viewports.size()), viewport_count);
                auto s_count = std::max(u32(scissors.size()), scissor_count);

                return {
                    .viewport_count = v_count,
                    .viewports = data_or_null(viewports),
                    .scissor_count = s_count,
                    .scissors = data_or_null(scissors),
                };
            }
        };

        struct depth_bias {
            float constant_factor;
            float clamp;
            float slope_factor;
        };

        struct rasterization {
            bool                  depth_clamp_enable;
            bool                  rasterizer_discard_enable;
            polygon_mode          polygon_mode;
            cull_mode_flags       cull_mode;
            front_face            front_face;
            core::opt<depth_bias> depth_bias = {};
            float                 line_width;

            operator pipeline_rasterization_state_create_info() const {
                return {
                    .depth_clamp_enable         = depth_clamp_enable,
                    .rasterizer_discard_enable  = rasterizer_discard_enable,
                    .polygon_mode               = polygon_mode,
                    .cull_mode                  = cull_mode,
                    .front_face                 = front_face,
                    .depth_bias_enable          = !depth_bias.empty(),
                    .depth_bias_constant_factor = depth_bias ? depth_bias->constant_factor : 0.f,
                    .depth_bias_clamp           = depth_bias ? depth_bias->clamp : 0.f,
                    .depth_bias_slope_factor    = depth_bias ? depth_bias->slope_factor : 0.f,
                    .line_width                 = line_width,
                };
            }
        };

        struct multisample {
            sample_count_flags         rasterization_samples;
            float                      min_sample_shading       = 0.f;
            std::vector<sample_mask_t> sample_masks             = {};
            bool32_t                   alpha_to_coverage_enable = {};
            bool32_t                   alpha_to_one_enable      = {};

            operator pipeline_multisample_state_create_info() const {
                return {
                    .rasterization_samples    = rasterization_samples,
                    .sample_shading_enable    = !sample_masks.empty(),
                    .min_sample_shading       = min_sample_shading,
                    .sample_mask              = data_or_null(sample_masks),
                    .alpha_to_coverage_enable = alpha_to_coverage_enable,
                    .alpha_to_one_enable      = alpha_to_one_enable,
                };
            }
        };

        using depth_stencil  = pipeline_depth_stencil_state_create_info;

        struct color_blend {
            struct blend_info {
                blend_factor src_factor;
                blend_factor dst_factor;
                blend_op     op;
            };

            struct blend {
                blend_info color;
                blend_info alpha;
            };

            struct attachment {
                color_component_flags color_write_mask;
                core::opt<blend>      blend = {};

                operator pipeline_color_blend_attachment_state() const {
                    return {
                        .blend_enable           = !blend.empty(),
                        .src_color_blend_factor = blend ? blend->color.src_factor : blend_factor{},
                        .dst_color_blend_factor = blend ? blend->color.dst_factor : blend_factor{},
                        .color_blend_op         = blend ? blend->color.op : blend_op{},
                        .src_alpha_blend_factor = blend ? blend->alpha.src_factor : blend_factor{},
                        .dst_alpha_blend_factor = blend ? blend->alpha.dst_factor : blend_factor{},
                        .alpha_blend_op         = blend ? blend->alpha.op : blend_op{},
                        .color_write_mask       = color_write_mask
                    };
                }
            };

            bool                    logic_op_enable;
            logic_op                logic_op;
            std::vector<attachment> attachments        = {};
            float                   blend_constants[4] = {0.f, 0.f, 0.f, 0.f};

            mutable std::vector<pipeline_color_blend_attachment_state> _m_attachments = {};

            operator pipeline_color_blend_state_create_info() const {
                _m_attachments.assign(attachments.begin(), attachments.end());

                return {
                    .logic_op_enable  = logic_op_enable,
                    .logic_op         = logic_op,
                    .attachment_count = u32(attachments.size()),
                    .attachments      = data_or_null(_m_attachments),
                    .blend_constants = {blend_constants[0], blend_constants[1], blend_constants[2], blend_constants[3]},
                };
            }
        };

        struct dynamic {
            std::vector<dynamic_state> dynamic_states;

            operator pipeline_dynamic_state_create_info() const {
                return {
                    .dynamic_state_count = u32(dynamic_states.size()),
                    .dynamic_states      = data_or_null(dynamic_states),
                };
            }
        };
    } // namespace state

    struct states {
        std::vector<pipeline::state::vertex_input>   vertex_input   = {};
        std::vector<pipeline::state::input_assembly> input_assembly = {};
        std::vector<pipeline::state::tessellation>   tessellation   = {};
        std::vector<pipeline::state::viewport>       viewport       = {};
        std::vector<pipeline::state::rasterization>  rasterization  = {};
        std::vector<pipeline::state::multisample>    multisample    = {};
        std::vector<pipeline::state::depth_stencil>  depth_stencil  = {};
        std::vector<pipeline::state::color_blend>    color_blend    = {};
        std::vector<pipeline::state::dynamic>        dynamic        = {};
    };
} // namespace pipeline

template <typename... Ts>
struct graphics_pipeline {
    /** One entry for each active shader stage */
    std::vector<pipeline_shader_stage> stages = {};

    pipeline::states states;

    /** Interface layout of the pipeline */
    vk::pipeline_layout layout      = {};
    vk::render_pass     render_pass = {};
    u32                 subpass     = 0;
    /**
     * If VK_PIPELINE_CREATE_DERIVATIVE_BIT is set and this value is nonzero, it specifies the handle of the base
     * pipeline this is a derivative of
     */
    vk::pipeline base_pipeline_handle = {};
    /**
     * If VK_PIPELINE_CREATE_DERIVATIVE_BIT is set and this value is not -1, it specifies an index into pCreateInfos of
     * the base pipeline this is a derivative of
     */
    i32 base_pipeline_index = -1;

    mutable core::tuple<Ts...> chained = {};

    mutable std::vector<pipeline_vertex_input_state_create_info>  _m_vertex_input_state  = {};
    mutable std::vector<pipeline_viewport_state_create_info>      _m_viewport_state      = {};
    mutable std::vector<pipeline_rasterization_state_create_info> _m_rasterization_state = {};
    mutable std::vector<pipeline_multisample_state_create_info>   _m_multisample_state   = {};
    mutable std::vector<pipeline_color_blend_state_create_info>   _m_color_blend_state   = {};
    mutable std::vector<pipeline_dynamic_state_create_info>       _m_dynamic_state       = {};

    operator graphics_pipeline_create_info() const {
        _m_vertex_input_state.assign(states.vertex_input.begin(), states.vertex_input.end());
        _m_viewport_state.assign(states.viewport.begin(), states.viewport.end());
        _m_rasterization_state.assign(states.rasterization.begin(), states.rasterization.end());
        _m_multisample_state.assign(states.multisample.begin(), states.multisample.end());
        _m_color_blend_state.assign(states.color_blend.begin(), states.color_blend.end());
        _m_dynamic_state.assign(states.dynamic.begin(), states.dynamic.end());

        return {
            .next                 = chain_setup(chained),
            .stage_count          = u32(stages.size()),
            .stages               = data_or_null(stages),
            .vertex_input_state   = data_or_null(_m_vertex_input_state),
            .input_assembly_state = data_or_null(states.input_assembly),
            .tessellation_state   = data_or_null(states.tessellation),
            .viewport_state       = data_or_null(_m_viewport_state),
            .rasterization_state  = data_or_null(_m_rasterization_state),
            .multisample_state    = data_or_null(_m_multisample_state),
            .depth_stencil_state  = data_or_null(states.depth_stencil),
            .color_blend_state    = data_or_null(_m_color_blend_state),
            .dynamic_state        = data_or_null(_m_dynamic_state),
            .layout               = layout,
            .render_pass          = render_pass,
            .subpass              = subpass,
            .base_pipeline_handle = base_pipeline_handle,
            .base_pipeline_index  = base_pipeline_index,
        };
    }
};

struct framebuffer {
    framebuffer_create_flags flags = {};
    vk::render_pass          render_pass;
    std::vector<image_view>  attachments;
    extent2d                 size;
    u32                      layers;

    operator framebuffer_create_info() const {
        return {
            .flags            = flags,
            .render_pass      = render_pass,
            .attachment_count = u32(attachments.size()),
            .attachments      = data_or_null(attachments),
            .width            = size.width,
            .height           = size.height,
            .layers           = layers,
        };
    }
};

using command_pool   = command_pool_create_info;
using command_buffer = command_buffer_allocate_info;
} // namespace vk::info

namespace vk {
template <core::number T>
struct clear_color {
    T value[4];
};

using clear_depth_stencil = clear_depth_stencil_value;
using clear_color_values =
    core::var<std::vector<clear_color<i32>>, std::vector<clear_color<u32>>, std::vector<clear_color<float>>, std::vector<clear_depth_stencil>>;
} // namespace vk

namespace vk::info {
struct render_pass_begin {
    vk::render_pass    render_pass;
    vk::framebuffer    framebuffer;
    rect2d             render_area;
    clear_color_values clear_values;

    operator render_pass_begin_info() const {
        render_pass_begin_info res{
            .render_pass       = render_pass,
            .framebuffer       = framebuffer,
            .render_area       = render_area,
            .clear_value_count = u32(clear_values.size()),
            .clear_values      = nullptr,
        };

        if (clear_values.size()) {
            core::visit(
                clear_values,
                core::overloaded{
                    [&]<typename T>(const std::vector<T>& c) { res.clear_values = (const clear_value*)c.data(); },
                }
            );
        }

        return res;
    }
};

struct submit {
    struct wait_info {
        vk::semaphore        semaphore;
        pipeline_stage_flags flags;
        core::opt<u64>       timeline_value = {};
    };

    struct signal_info {
        vk::semaphore  semaphore;
        core::opt<u64> timeline_value = {};
    };

    std::vector<wait_info>          wait_semaphores;
    std::vector<vk::command_buffer> command_buffers;
    std::vector<signal_info>        signal_semaphores;

    mutable std::vector<semaphore>             _m_wait_semaphores      = {};
    mutable std::vector<pipeline_stage_flags>  _m_wait_flags           = {};
    mutable std::vector<semaphore>             _m_signal_semaphores    = {};
    mutable std::vector<u64>                   _m_timeline_wait_sems   = {};
    mutable std::vector<u64>                   _m_timeline_signal_sems = {};
    mutable vk::timeline_semaphore_submit_info _m_timeline_sem         = {};

    operator submit_info() const {
        _m_wait_semaphores.clear();
        _m_wait_flags.clear();
        _m_signal_semaphores.clear();
        _m_timeline_wait_sems.clear();
        _m_timeline_signal_sems.clear();

        for (auto&& w : wait_semaphores) {
            _m_wait_semaphores.push_back(w.semaphore);
            _m_wait_flags.push_back(w.flags);
            if (w.timeline_value) {
                _m_timeline_wait_sems.push_back(*w.timeline_value);
            }
        }

        for (auto&& s : signal_semaphores) {
            _m_signal_semaphores.push_back(s.semaphore);
            if (s.timeline_value) {
                _m_timeline_signal_sems.push_back(*s.timeline_value);
            }
        }

        vk::timeline_semaphore_submit_info* timeline_sem = nullptr;
        if (!_m_timeline_wait_sems.empty() || !_m_timeline_signal_sems.empty()) {
            _m_timeline_sem.wait_semaphore_value_count = u32(_m_wait_semaphores.size());
            _m_timeline_sem.wait_semaphore_values = _m_timeline_wait_sems.data();
            _m_timeline_sem.signal_semaphore_value_count = u32(_m_signal_semaphores.size());
            _m_timeline_sem.signal_semaphore_values = _m_timeline_signal_sems.data();
            timeline_sem = &_m_timeline_sem;
        }

        return {
            .next                   = timeline_sem,
            .wait_semaphore_count   = u32(_m_wait_semaphores.size()),
            .wait_semaphores        = data_or_null(_m_wait_semaphores),
            .wait_dst_stage_mask    = data_or_null(_m_wait_flags),
            .command_buffer_count   = u32(command_buffers.size()),
            .command_buffers        = data_or_null(command_buffers),
            .signal_semaphore_count = u32(_m_signal_semaphores.size()),
            .signal_semaphores      = data_or_null(_m_signal_semaphores),
        };
    }
};

#ifdef VK_KHR_swapchain
struct present {
    struct swapchain {
        /** Swapchain to present an image from */
        vk::swapchain_khr swapchain;
        /** Index of which presentable images to present */
        u32 image_index;
    };

    /** Semaphores to wait for before presenting */
    std::vector<vk::semaphore> wait_semaphores = {};
    std::vector<swapchain>     swapchains;

    mutable std::vector<vk::swapchain_khr> _m_swapchain     = {};
    mutable std::vector<u32>               _m_image_indices = {};

    operator present_info_khr() const {
        _m_swapchain.clear();
        _m_image_indices.clear();
        for (auto&& [sw, i] : swapchains) {
            _m_swapchain.push_back(sw);
            _m_image_indices.push_back(i);
        }

        return {
            .wait_semaphore_count = u32(wait_semaphores.size()),
            .wait_semaphores      = data_or_null(wait_semaphores),
            .swapchain_count      = u32(swapchains.size()),
            .swapchains           = data_or_null(_m_swapchain),
            .image_indices        = data_or_null(_m_image_indices),
            .results              = nullptr,
        };
    }
};

struct dependency {
    dependency_flags                    flags                  = {};
    std::vector<memory_barrier2>        memory_barriers        = {};
    std::vector<buffer_memory_barrier2> buffer_memory_barriers = {};
    std::vector<image_memory_barrier2>  image_memory_barriers  = {};

    operator dependency_info() const {
        return {
            .dependency_flags            = flags,
            .memory_barrier_count        = u32(memory_barriers.size()),
            .memory_barriers             = data_or_null(memory_barriers),
            .buffer_memory_barrier_count = u32(buffer_memory_barriers.size()),
            .buffer_memory_barriers      = data_or_null(buffer_memory_barriers),
            .image_memory_barrier_count  = u32(image_memory_barriers.size()),
            .image_memory_barriers       = data_or_null(image_memory_barriers),
        };
    }
};

struct rendering {
    rendering_flags                        flags = {};
    rect2d                                 render_area;
    u32                                    layer_count = 0;
    u32                                    view_mask   = 0;
    std::vector<rendering_attachment_info> color_attachments;
    core::opt<rendering_attachment_info>   depth_attachment   = {};
    core::opt<rendering_attachment_info>   stencil_attachment = {};

    operator rendering_info() const {
        return {
            .flags                  = flags,
            .render_area            = render_area,
            .layer_count            = layer_count,
            .view_mask              = view_mask,
            .color_attachment_count = u32(color_attachments.size()),
            .color_attachments      = data_or_null(color_attachments),
            .depth_attachment       = depth_attachment ? &*depth_attachment : nullptr,
            .stencil_attachment     = stencil_attachment ? &*stencil_attachment : nullptr,
        };
    }
};
#endif
} // namespace vk::info
