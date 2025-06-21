#include <grx/vk/constants.cg.hpp>
#include <grx/vk/device.hpp>
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
#include <grx/vk/arg.hpp>

#include <util/fps_counter.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/ranges/zip.hpp>
#include <core/io/mmap.hpp>

using namespace core;

std::vector<std::string> required_extensions() {
    u32 count = 0;
    auto extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<std::string> result;

    for (u32 i = 0; i < count; ++i) {
        result.push_back(extensions[i]);
    }

    result.push_back("VK_EXT_debug_utils");
    result.push_back("VK_EXT_debug_report");

    return result;
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

    auto is_complete = [&] {
        return result.graphics != limits<u32>::max() && result.present != limits<u32>::max();
    };

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
            .width = std::clamp(u32(width), capabilities.min_image_extent.width, capabilities.max_image_extent.width),
            .height = std::clamp(u32(height), capabilities.min_image_extent.height, capabilities.max_image_extent.height),
        };
    }


    vk::surface_capabilities_khr capabilities;
    std::vector<vk::surface_format_khr> formats;
    std::vector<vk::present_mode_khr> present_modes;
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

    auto instance = lib.create_instance(vk::instance_create_info::make(
        vk::arg::appinfo =
            vk::application_info{
                .application_name    = "Test App",
                .application_version = {1, 0, 0},
                .engine_name         = "Test Engine",
                .engine_version      = {1, 0, 0},
                .api_version         = {1, 0, 0},
            },
        vk::arg::extensions = required_extensions(),
        vk::arg::layers     = {"VK_LAYER_KHRONOS_validation"}
    ));

    /*************** Init surface */
    vk::surface_khr surface_raw;
    glfwCreateWindowSurface((VkInstance)instance.handle(), wnd, nullptr, (VkSurfaceKHR*)&surface_raw);
    vk::surface_t surface{instance, surface_raw};

    auto phy_dev = find_suitable_physical_device(instance, surface);

    /*************** Find families */
    auto families = *find_queue_families(phy_dev, surface);

    /*************** Init logical device */
    auto dev = phy_dev.create_device(vk::device_create_info::make(
        vk::arg::extensions        = {"VK_KHR_swapchain"},
        vk::arg::layers            = {"VK_LAYER_KHRONOS_validation"},
        vk::arg::features          = {.shader_clip_distance = true, .shader_cull_distance = true},
        vk::arg::queue_create_info = {{.family_index = families.graphics}, {.family_index = families.present}}
    ));

    auto graphics_queue = dev.get_device_queue(families.graphics, 0);
    auto present_queue  = dev.get_device_queue(families.present, 0);

    /*************** Create swapchain */
    auto swapchain_support = query_swapchain_support(phy_dev, surface);
    auto surface_format = swapchain_support.select_format();
    auto present_mode = swapchain_support.select_present_mode();
    auto extent = swapchain_support.select_extent(wnd);

    auto image_count = swapchain_support.capabilities.min_image_count + 1;
    if (swapchain_support.capabilities.max_image_count > 0 && image_count > swapchain_support.capabilities.max_image_count) {
        image_count = swapchain_support.capabilities.max_image_count;
    }

    auto swapchain = dev.create_swapchain(vk::swapchain_create_info::make(
        vk::arg::surface         = surface,
        vk::arg::min_image_count = image_count,
        vk::arg::pre_transform   = swapchain_support.capabilities.current_transform,
        vk::arg::composite_alpha = vk::composite_alpha_khr_flag::opaque_khr,
        vk::arg::present_mode    = present_mode,
        vk::arg::clipped         = true,
        vk::arg::image_info =
            vk::arg::swapchain_image_info{
                .format       = surface_format,
                .extent       = extent,
                .array_layers = 1,
                .usage        = vk::image_usage_flag::color_attachment,
            },
        vk::arg::queue_family_indices = {families.graphics, families.present}
    ));

    auto swapchain_images = swapchain.images().value();
    auto swapchain_image_format = surface_format.format;
    auto swapchain_extent = extent;

    /*************** Create image views */
    std::vector<vk::image_view_t> image_views;
    image_views.reserve(swapchain_images.size());

    for (auto& image : swapchain_images) {
        image_views.emplace_back(dev.create_image_view(vk::image_view_create_info{
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
        }));
    }

    /*************** Create render pass */
    vk::attachment_description color_attachment{};
    color_attachment.format           = swapchain_image_format;
    color_attachment.samples          = vk::sample_count_flag::_1;
    color_attachment.load_op          = vk::attachment_load_op::clear;
    color_attachment.store_op         = vk::attachment_store_op::store;
    color_attachment.stencil_load_op  = vk::attachment_load_op::dont_care;
    color_attachment.stencil_store_op = vk::attachment_store_op::dont_care;
    color_attachment.initial_layout   = vk::image_layout::undefined;
    color_attachment.final_layout     = vk::image_layout::present_src_khr;

    vk::attachment_reference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = vk::image_layout::color_attachment_optimal;

    vk::subpass_description subpass{};
    subpass.pipeline_bind_point = vk::pipeline_bind_point::graphics;
    subpass.color_attachment_count = 1;
    subpass.color_attachments = &color_attachment_ref;

    vk::subpass_dependency dependency{};
    dependency.src_subpass = vk::constants::subpass_external;
    dependency.dst_subpass = 0;
    dependency.src_stage_mask = vk::pipeline_stage_flag::color_attachment_output;
    dependency.src_access_mask = {};
    dependency.dst_stage_mask = vk::pipeline_stage_flag::color_attachment_output;
    dependency.dst_access_mask = vk::access_flag::color_attachment_write;

    vk::render_pass_create_info render_pass_info{};
    render_pass_info.attachment_count = 1;
    render_pass_info.attachments = &color_attachment;
    render_pass_info.subpass_count = 1;
    render_pass_info.subpasses = &subpass;
    render_pass_info.dependency_count = 1;
    render_pass_info.dependencies = &dependency;

    auto render_pass = dev.create_render_pass(render_pass_info);

    /*************** Create shader modules */
    opt vert_shader = dev.create_shader_module(vk::arg::spirv_file{"vert.spv"});
    opt frag_shader = dev.create_shader_module(vk::arg::spirv_file{"frag.spv"});

    /*************** Create graphics pipeline */
    vk::pipeline_shader_stage_create_info vert_stage_info{};
    vert_stage_info.stage = vk::shader_stage_flag::vertex;
    vert_stage_info.module = *vert_shader;
    vert_stage_info.name = "main";

    vk::pipeline_shader_stage_create_info frag_stage_info{};
    frag_stage_info.stage = vk::shader_stage_flag::fragment;
    frag_stage_info.module = *frag_shader;
    frag_stage_info.name = "main";

    vk::pipeline_shader_stage_create_info stages[] = {vert_stage_info, frag_stage_info};

    vk::pipeline_vertex_input_state_create_info vertex_input_info{};
    vertex_input_info.vertex_binding_description_count = 0;
    vertex_input_info.vertex_attribute_description_count = 0;

    vk::pipeline_input_assembly_state_create_info input_assembly{};
    input_assembly.topology = vk::primitive_topology::triangle_list;
    input_assembly.primitive_restart_enable = false;

    vk::pipeline_viewport_state_create_info viewport_state{};
    viewport_state.viewport_count = 1;
    viewport_state.scissor_count = 1;

    vk::pipeline_rasterization_state_create_info rasterizer{};
    rasterizer.depth_clamp_enable = false;
    rasterizer.rasterizer_discard_enable = false;
    rasterizer.polygon_mode = vk::polygon_mode::fill;
    rasterizer.line_width = 1.f;
    rasterizer.cull_mode = vk::cull_mode_flag::back;
    rasterizer.front_face = vk::front_face::clockwise;
    rasterizer.depth_bias_enable = false;

    vk::pipeline_multisample_state_create_info multisampling{};
    multisampling.sample_shading_enable = false;
    multisampling.rasterization_samples = vk::sample_count_flag::_1;

    vk::pipeline_color_blend_attachment_state color_blend_attachment{};
    color_blend_attachment.color_write_mask = vk::color_component_flag::r | vk::color_component_flag::g |
                                              vk::color_component_flag::b | vk::color_component_flag::a;
    color_blend_attachment.blend_enable = false;

    vk::pipeline_color_blend_state_create_info color_blending{};
    color_blending.logic_op_enable = false;
    color_blending.logic_op = vk::logic_op::copy;
    color_blending.attachment_count = 1;
    color_blending.attachments = &color_blend_attachment;
    color_blending.blend_constants[0] = 0.f;
    color_blending.blend_constants[1] = 0.f;
    color_blending.blend_constants[2] = 0.f;
    color_blending.blend_constants[3] = 0.f;

    vk::dynamic_state states[] = {vk::dynamic_state::viewport, vk::dynamic_state::scissor};
    vk::pipeline_dynamic_state_create_info dynamic_state{};
    dynamic_state.dynamic_state_count = u32(std::size(states));
    dynamic_state.dynamic_states = states;

    vk::pipeline_layout_create_info pipeline_layout_info;
    pipeline_layout_info.set_layout_count = 0;
    pipeline_layout_info.push_constant_range_count = 0;

    auto pipeline_layout = dev.create_pipeline_layout(pipeline_layout_info);

    vk::graphics_pipeline_create_info pipeline_info{};
    pipeline_info.stage_count = u32(std::size(stages));
    pipeline_info.stages = stages;
    pipeline_info.vertex_input_state = &vertex_input_info;
    pipeline_info.input_assembly_state = &input_assembly;
    pipeline_info.viewport_state = &viewport_state;
    pipeline_info.rasterization_state = &rasterizer;
    pipeline_info.multisample_state = &multisampling;
    pipeline_info.color_blend_state = &color_blending;
    pipeline_info.dynamic_state = &dynamic_state;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.render_pass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.base_pipeline_handle = null;

    auto pipelines = dev.create_graphics_pipelines(nullptr, {&pipeline_info, 1});
    auto& graphics_pipeline = pipelines[0];

    vert_shader.reset();
    frag_shader.reset();

    /*************** Create framebuffers */
    std::vector<vk::framebuffer_t> framebuffers;
    for (auto&& image_view : image_views) {
        auto attachment = image_view.handle();
        vk::framebuffer_create_info framebuffer_info{};
        framebuffer_info.render_pass = render_pass.handle();
        framebuffer_info.attachment_count = 1;
        framebuffer_info.attachments = &attachment;
        framebuffer_info.width = swapchain_extent.width;
        framebuffer_info.height = swapchain_extent.height;
        framebuffer_info.layers = 1;

        framebuffers.push_back(dev.create_framebuffer(framebuffer_info));
    }

    /************** Create commad pool */
    vk::command_pool_create_info pool_info{};
    pool_info.flags = vk::command_pool_create_flag::reset_command_buffer;
    pool_info.queue_family_index = families.graphics;

    auto command_pool = dev.create_command_pool(pool_info);

    /************** Create command buffer */

    constexpr u32 frames_in_flight = 2;

    vk::command_buffer_allocate_info alloc_info{};
    alloc_info.command_pool = command_pool;
    alloc_info.level = vk::command_buffer_level::primary;
    alloc_info.command_buffer_count = frames_in_flight;

    auto command_buffers = dev.allocate_command_buffers(alloc_info);

    /*************** Create sync objects */
    core::array image_available_semaphores = {
        dev.create_semaphore({}),
        dev.create_semaphore({}),
    };
    std::vector<vk::semaphore_t> render_finished_semaphores;
    for (auto&& _ : swapchain_images)
        render_finished_semaphores.emplace_back(dev.create_semaphore({}));
    core::array in_flight_fences = {
        dev.create_fence(vk::fence_create_info{.flags = vk::fence_create_flag::signaled}),
        dev.create_fence(vk::fence_create_info{.flags = vk::fence_create_flag::signaled}),
    };
    static_assert(image_available_semaphores.size() == in_flight_fences.size());

    u32 frame = 0;
    util::fps_counter fps;

    while (!glfwWindowShouldClose(wnd)) {
        glfwPollEvents();

        dev.wait(in_flight_fences[frame]);
        dev.reset(in_flight_fences[frame]);

        auto image_index = swapchain.acquire_next_image(image_available_semaphores[frame]).value();

        /* Render pass */
        command_buffers[frame].reset({}).throws();
        command_buffers[frame].begin({}).throws();

        vk::render_pass_begin_info render_pass_info{};
        render_pass_info.render_pass = render_pass;
        render_pass_info.framebuffer = framebuffers[image_index];
        render_pass_info.render_area.offset = {0, 0};
        render_pass_info.render_area.extent = swapchain_extent;

        vk::clear_value clear_color = {.color = {.float32 = {0.f, 0.f, 0.f, 0.f}}};
        render_pass_info.clear_value_count = 1;
        render_pass_info.clear_values = &clear_color;

        command_buffers[frame].cmd_begin_render_pass(render_pass_info, vk::subpass_contents::_inline);
        command_buffers[frame].cmd_bind_pipeline(vk::pipeline_bind_point::graphics, graphics_pipeline);

        vk::viewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = float(swapchain_extent.width);
        viewport.height = float(swapchain_extent.height);
        viewport.min_depth = 0.f;
        viewport.max_depth = 1.f;
        command_buffers[frame].cmd_set_viewport(0, {&viewport, 1});

        vk::rect2d scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchain_extent;
        command_buffers[frame].cmd_set_scissor(0, {&scissor, 1});

        command_buffers[frame].cmd_draw(3, 1, 0, 0);

        command_buffers[frame].cmd_end_render_pass();
        command_buffers[frame].end().throws();


        /* Queue submit */
        vk::semaphore wait_semaphores[] = {image_available_semaphores[frame]};
        vk::pipeline_stage_flags wait_stages[] = {vk::pipeline_stage_flag::color_attachment_output};

        vk::submit_info submit_info{};
        submit_info.wait_semaphore_count = u32(std::size(wait_semaphores));
        submit_info.wait_semaphores = wait_semaphores;
        submit_info.wait_dst_stage_mask = wait_stages;

        vk::command_buffer buffers[] = {command_buffers[frame]};
        submit_info.command_buffer_count = u32(std::size(buffers));
        submit_info.command_buffers = buffers;

        vk::semaphore signal_semaphores[] = {render_finished_semaphores[image_index]};
        submit_info.signal_semaphore_count = u32(std::size(signal_semaphores));
        submit_info.signal_semaphores = signal_semaphores;

        graphics_queue.submit({&submit_info, 1}, in_flight_fences[frame]).throws();

        vk::present_info_khr present_info{};
        present_info.wait_semaphore_count = u32(std::size(signal_semaphores));
        present_info.wait_semaphores = signal_semaphores;

        vk::swapchain_khr swapchains[] = {swapchain};
        present_info.swapchain_count = u32(std::size(swapchains));
        present_info.swapchains = swapchains;
        present_info.image_indices = &image_index;

        present_queue.present(present_info).throws();

        frame = (frame + 1) % frames_in_flight;

        log.warn_update(0, "{}", fps.calculate());
    }

    dev.wait_idle().throws();
}
