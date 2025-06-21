#include <core/ranges/index.hpp>
#include <grx/vk/constants.cg.hpp>
#include <grx/vk/device.cg.hpp>
#include <grx/vk/enums.cg.hpp>
#include <grx/vk/instance.hpp>
#include <grx/vk/structs.cg.hpp>
#include <grx/vk/surface.hpp>
#include <grx/vk/swapchain.cg.hpp>
#include <grx/vk/image_view.cg.hpp>
#include <grx/vk/shader_module.cg.hpp>
#include <grx/vk/pipeline_layout.cg.hpp>
#include <grx/vk/pipeline.cg.hpp>
#include <grx/vk/render_pass.cg.hpp>
#include <grx/vk/framebuffer.cg.hpp>
#include <grx/vk/command_pool.cg.hpp>
#include <grx/vk/command_buffer.hpp>
#include <grx/vk/semaphore.cg.hpp>
#include <grx/vk/fence.cg.hpp>
#include <grx/vk/queue.cg.hpp>
#include <grx/vk/info.hpp>
#include <grx/vk/arg.hpp>

#include <core/ranges/to.hpp>
#include <core/ranges/transform.hpp>

#include <util/arg_parse.hpp>
#include <util/fps_counter.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/ranges/zip.hpp>
#include <core/io/mmap.hpp>

using namespace core;

std::vector<std::string> required_extensions() {
    u32  count      = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&count);
    return std::span{extensions, count} | transform{[](auto v) { return std::string(v); }} | to_vector{};
}

struct queue_family_indices {
    u32 graphics;
    u32 present;
};

opt<queue_family_indices> find_queue_families(const vk::physical_device_t& dev, const vk::surface_khr& surface) {
    queue_family_indices result{
        .graphics = limits<u32>::max(),
        .present  = limits<u32>::max(),
    };

    auto is_complete = [&] { return result.graphics != limits<u32>::max() && result.present != limits<u32>::max(); };

    for (auto&& [idx, queue_family] : with_index(dev.queue_family_properties())) {
        auto i = u32(idx);

        if (queue_family.queue_flags.test(vk::queue_flag::graphics))
            result.graphics = i;

        if (dev.surface_support(i, surface))
            result.present = i;

        if (is_complete())
            break;
    }

    if (is_complete())
        return result;
    return {};
}

bool support_extensions(const vk::physical_device_t& dev, const std::vector<std::string>& required_extensions) {
    auto extensions = dev.extension_properties().value();

    for (auto&& required : required_extensions) {
        auto found = std::ranges::find_if(extensions, [&](const vk::extension_properties& ext) { return ext.extension_name == required; });
        if (found == extensions.end())
            return false;
    }
    return true;
}

struct swapchain_details {
    auto select_format() {
        for (auto&& format : formats) {
            if (format.format == vk::format::b8g8r8a8_srgb && format.color_space == vk::color_space_khr::srgb_nonlinear_khr) {
                return format;
            }
        }
        return formats[0];
    }

    auto select_present_mode() {
        for (auto&& mode : present_modes) {
            if (mode == vk::present_mode_khr::mailbox_khr) {
                return mode;
            }
        }
        return vk::present_mode_khr::fifo_khr;
    }

    auto select_extent(GLFWwindow* window) {
        if (capabilities.current_extent.width != limits<u32>::max()) {
            return capabilities.current_extent;
        }

        int width;
        int height;
        glfwGetFramebufferSize(window, &width, &height);

        return vk::extent2d{
            .width  = std::clamp(u32(width), capabilities.min_image_extent.width, capabilities.max_image_extent.width),
            .height = std::clamp(u32(height), capabilities.min_image_extent.height, capabilities.max_image_extent.height),
        };
    }

    vk::surface_capabilities_khr        capabilities;
    std::vector<vk::surface_format_khr> formats;
    std::vector<vk::present_mode_khr>   present_modes;
};

swapchain_details query_swapchain_support(const vk::physical_device_t& dev, const vk::surface_khr& surface) {
    return {
        .capabilities  = dev.surface_capabilities(surface).value(),
        .formats       = dev.surface_formats(surface).value(),
        .present_modes = dev.surface_present_modes(surface).value(),
    };
}

vk::physical_device_t find_suitable_physical_device(const vk::instance_t& instance, const vk::surface_khr& surface) {
    auto devices = instance.physical_devices().value();

    for (auto&& dev : devices) {
        if (find_queue_families(dev, surface) && support_extensions(dev, {"VK_KHR_swapchain"})) {
            auto [_, formats, present_modes] = query_swapchain_support(dev, surface);
            if (!formats.empty() && !present_modes.empty()) {
                return dev;
            }
        }
    }

    throw std::runtime_error("Suitable physical device not found");
}

int main() {
    auto& log = util::glog();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto wnd = glfwCreateWindow(800, 600, "vulkan test", nullptr, nullptr);

    vk::vk_lib lib{"/usr/lib64/libvulkan.so"};

    auto instance = lib.create_instance({
        .application =
            vk::info::application{
                .name           = "Test App",
                .version        = {.major = 1, .minor = 0, .patch = 0},
                .engine_name    = "Test Engine",
                .engine_version = {.major = 1, .minor = 0, .patch = 0},
                .api_version    = {.major = 1, .minor = 0, .patch = 0},
            },
        .extensions = required_extensions(),
        .layers     = {"VK_LAYER_KHRONOS_validation"},
    });

    /*************** Init surface */
    vk::surface_khr surface_raw;
    glfwCreateWindowSurface((VkInstance)instance.handle(), wnd, nullptr, (VkSurfaceKHR*)&surface_raw);
    vk::surface_t surface{instance, surface_raw};

    auto physical_device = find_suitable_physical_device(instance, surface);

    /*************** Find families */
    auto families = *find_queue_families(physical_device, surface);

    /*************** Init logical device */
    auto dev = physical_device.create_device(
        vk::info::device{
            .queues     = {{.family_index = families.graphics}, {.family_index = families.present}},
            .extensions = {"VK_KHR_swapchain"},
            .features   = {.shader_clip_distance = true, .shader_cull_distance = true},
        }
    );

    auto graphics_queue = dev.get_device_queue(families.graphics, 0);
    auto present_queue  = dev.get_device_queue(families.present, 0);

    /*************** Create swapchain */
    auto swapchain_support = query_swapchain_support(physical_device, surface);
    auto surface_format    = swapchain_support.select_format();
    auto present_mode      = swapchain_support.select_present_mode();
    auto extent            = swapchain_support.select_extent(wnd);

    auto image_count = swapchain_support.capabilities.min_image_count + 1;
    if (swapchain_support.capabilities.max_image_count > 0 && image_count > swapchain_support.capabilities.max_image_count) {
        image_count = swapchain_support.capabilities.max_image_count;
    }

    auto swapchain = dev.create_swapchain(
        vk::info::swapchain{
            .surface         = surface,
            .min_image_count = image_count,
            .image =
                {
                    .format       = surface_format,
                    .extent       = extent,
                    .array_layers = 1,
                    .usage        = vk::image_usage_flag::color_attachment,
                },
            .queue_family_indices = {families.graphics, families.present},
            .pre_transform        = swapchain_support.capabilities.current_transform,
            .composite_alpha      = vk::composite_alpha_khr_flag::opaque_khr,
            .present_mode         = present_mode,
            .clipped              = true
        }
    );

    auto swapchain_images       = swapchain.images().value();
    auto swapchain_image_format = surface_format.format;
    auto swapchain_extent       = extent;

    /*************** Create image views */
    auto image_views = swapchain_images | transform{[&](const vk::image& image) {
                           return dev.create_image_view(
                               vk::image_view_create_info{
                                   .image      = image,
                                   .view_type  = vk::image_view_type::_2d,
                                   .format     = swapchain_image_format,
                                   .components = vk::arg::identity,
                                   .subresource_range =
                                       {
                                           .aspect_mask      = vk::image_aspect_flag::color,
                                           .base_mip_level   = 0,
                                           .level_count      = 1,
                                           .base_array_layer = 0,
                                           .layer_count      = 1,
                                       },
                               }
                           );
                       }} |
                       to_vector{};

    /*************** Create render pass */
    auto render_pass = dev.create_render_pass(
        vk::info::render_pass{
            .attachments  = {{
                 .format           = swapchain_image_format,
                 .samples          = vk::sample_count_flag::_1,
                 .load_op          = vk::attachment_load_op::clear,
                 .store_op         = vk::attachment_store_op::store,
                 .stencil_load_op  = vk::attachment_load_op::dont_care,
                 .stencil_store_op = vk::attachment_store_op::dont_care,
                 .initial_layout   = vk::image_layout::undefined,
                 .final_layout     = vk::image_layout::present_src_khr,
            }},
            .subpasses    = {{
                   .color_attachments = {{
                       .attachment = 0,
                       .layout     = vk::image_layout::color_attachment_optimal,
                }},
            }},
            .dependencies = {{
                .src_subpass     = vk::constants::subpass_external,
                .dst_subpass     = 0,
                .src_stage_mask  = vk::pipeline_stage_flag::color_attachment_output,
                .dst_stage_mask  = vk::pipeline_stage_flag::color_attachment_output,
                .src_access_mask = {},
                .dst_access_mask = vk::access_flag::color_attachment_write,
            }},
        }
    );

    auto pipeline_layout = dev.create_pipeline_layout(vk::info::pipeline_layout{});

    namespace state = vk::info::pipeline::state;

    auto graphics_pipeline = dev.create_graphics_pipeline(
        null,
        vk::info::graphics_pipeline{
            .stages =
                {vk::info::pipeline_shader_stage{
                     .stage  = vk::shader_stage_flag::vertex,
                     .module = dev.create_shader_module(vk::arg::spirv_file{"vert.spv"}),
                     .name   = "main",
                 },
                 vk::info::pipeline_shader_stage{
                     .stage  = vk::shader_stage_flag::fragment,
                     .module = dev.create_shader_module(vk::arg::spirv_file{"frag.spv"}),
                     .name   = "main",
                 }},
            .states =
                {.vertex_input   = {state::vertex_input{}},
                 .input_assembly = {state::input_assembly{.topology = vk::primitive_topology::triangle_list, .primitive_restart_enable = false}},
                 .viewport       = {state::viewport{.viewport_count = 1, .scissor_count = 1}},
                 .rasterization  = {state::rasterization{
                      .depth_clamp_enable        = false,
                      .rasterizer_discard_enable = false,
                      .polygon_mode              = vk::polygon_mode::fill,
                      .cull_mode                 = vk::cull_mode_flag::back,
                      .front_face                = vk::front_face::clockwise,
                      .line_width                = 1.f,
                 }},
                 .multisample    = {state::multisample{.rasterization_samples = vk::sample_count_flag::_1}},
                 .color_blend    = {state::color_blend{
                        .logic_op_enable = false,
                        .logic_op        = vk::logic_op::copy,
                        .attachments     = {state::color_blend::attachment{.color_write_mask = vk::arg::color_component_rgba}},
                 }},
                 .dynamic        = {state::dynamic{.dynamic_states = {vk::dynamic_state::viewport, vk::dynamic_state::scissor}}}},
            .layout      = pipeline_layout,
            .render_pass = render_pass,
            .subpass     = 0,
        }
    );

    /*************** Create framebuffers */
    auto framebuffers = image_views | transform{[&](const vk::image_view_t& image_view) {
                            return dev.create_framebuffer(
                                vk::info::framebuffer{
                                    .render_pass = render_pass,
                                    .attachments = {image_view},
                                    .size        = swapchain_extent,
                                    .layers      = 1,
                                }
                            );
                        }} |
                        to_vector{};

    /************** Create commad pool */
    auto command_pool = dev.create_command_pool(
        vk::info::command_pool{
            .flags              = vk::command_pool_create_flag::reset_command_buffer,
            .queue_family_index = families.graphics,
        }
    );

    /************** Create command buffer */

    constexpr u32 frames_in_flight = 2;

    auto command_buffers = dev.allocate_command_buffers(
        vk::info::command_buffer{
            .command_pool         = command_pool,
            .level                = vk::command_buffer_level::primary,
            .command_buffer_count = frames_in_flight,
        }
    );

    /*************** Create sync objects */
    auto image_available_semaphores = filled_with(frames_in_flight, [&] { return dev.create_semaphore({}); }) | to_vector{};
    auto render_finished_semaphores = filled_with(swapchain_images.size(), [&] { return dev.create_semaphore({}); }) | to_vector{};
    auto in_flight_fences           = filled_with(frames_in_flight, [&] { return dev.create_fence({.flags = vk::fence_create_flag::signaled}); }) | to_vector{};

    u32               frame = 0;
    util::fps_counter fps;

    while (!glfwWindowShouldClose(wnd)) {
        glfwPollEvents();

        dev.wait(in_flight_fences[frame]);
        dev.reset(in_flight_fences[frame]);

        auto image_index = swapchain.acquire_next_image(image_available_semaphores[frame]).value();

        /* Render pass */
        command_buffers[frame].reset({}).throws();
        command_buffers[frame].begin({}).throws();
        command_buffers[frame].cmd_begin_render_pass(
            vk::info::render_pass_begin{
                .render_pass  = render_pass,
                .framebuffer  = framebuffers[image_index],
                .render_area  = {.offset = {.x = 0, .y = 0}, .extent = swapchain_extent},
                .clear_values = std::vector{vk::clear_color<float>{0.f, 0.f, 0.f, 0.f}},
            },
            vk::subpass_contents::_inline
        );
        command_buffers[frame].cmd_bind_pipeline(vk::pipeline_bind_point::graphics, graphics_pipeline);
        command_buffers[frame].cmd_set_viewport(
            0,
            array{vk::viewport{
                .x         = 0.f,
                .y         = 0.f,
                .width     = float(swapchain_extent.width),
                .height    = float(swapchain_extent.height),
                .min_depth = 0.f,
                .max_depth = 1.f,
            }}
        );
        command_buffers[frame].cmd_set_scissor(0, array{vk::rect2d{.offset = {.x = 0, .y = 0}, .extent = swapchain_extent}});
        command_buffers[frame].cmd_draw(3, 1, 0, 0);
        command_buffers[frame].cmd_end_render_pass();
        command_buffers[frame].end().throws();

        /* Queue submit */
        graphics_queue
            .submit(
                vk::info::submit{
                    .wait_semaphores   = {vk::info::submit::wait_info{
                          .semaphore = image_available_semaphores[frame], .flags = vk::pipeline_stage_flag::color_attachment_output
                    }},
                    .command_buffers   = {command_buffers[frame]},
                    .signal_semaphores = {render_finished_semaphores[image_index]}
                },
                in_flight_fences[frame]
            )
            .throws();

        present_queue
            .present(
                vk::info::present{
                    .wait_semaphores = {render_finished_semaphores[image_index]},
                    .swapchains      = {vk::info::present::swapchain{.swapchain = swapchain, .image_index = image_index}}
                }
            )
            .throws();

        frame = (frame + 1) % frames_in_flight;

        log.warn_update(0, "{}", fps.calculate());
    }

    dev.wait_idle().throws();
}
