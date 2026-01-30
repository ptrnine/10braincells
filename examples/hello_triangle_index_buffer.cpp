#include <grx/vk.hpp>
#include <grx/vk/constants.cg.hpp>
#include <grx/vk/device_memory.cg.hpp>
#include <grx/vk/enums.cg.hpp>
#include <grx/vk/flags.cg.hpp>
#include <grx/vk/info.hpp>
#include <grx/vk/structs.cg.hpp>
#include <util/fps_counter.hpp>
#include <grx/basic_types.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/ranges/transform.hpp>
#include <core/ranges/to.hpp>
#include <core/ranges/zip.hpp>
#include <util/log.hpp>

using namespace core;

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

    u32 select_min_image_count() const {
        auto image_count = capabilities.min_image_count + 1;
        if (capabilities.max_image_count > 0 && image_count > capabilities.max_image_count) {
            image_count = capabilities.max_image_count;
        }
        return image_count;
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

struct vertex {
    grx::vec2f    pos;
    grx::clrf_rgb color;

    static constexpr vk::vertex_input_binding_description binding_description() {
        return {.binding = 0, .stride = sizeof(vertex), .input_rate = vk::vertex_input_rate::vertex};
    }

    static constexpr auto attribute_description() {
        return core::array{
            vk::vertex_input_attribute_description{0, 0, vk::format::r32g32_sfloat, offsetof(vertex, pos)},
            vk::vertex_input_attribute_description{1, 0, vk::format::r32g32b32_sfloat, offsetof(vertex, color)},
        };
    }
};

class hello_triangle {
public:
    util::logger& log = util::glog();

    std::vector<vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
    };

    std::vector<u32> indices = {0, 1, 2, 2, 3, 0};

    hello_triangle() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // NOLINTNEXTLINE
        wnd = glfwCreateWindow(800, 600, "hello triangle", nullptr, nullptr);
        glfwSetWindowUserPointer(wnd, this);
        glfwSetFramebufferSizeCallback(wnd, framebuffer_resize_callback);

        lib = vk::vk_lib{"/usr/lib64/libvulkan.so"};

        instance = lib.create_instance({
            .application =
                vk::info::application{
                    .name           = "Test App",
                    .version        = {.major = 1, .minor = 0, .patch = 0},
                    .engine_name    = "Test Engine",
                    .engine_version = {.major = 1, .minor = 0, .patch = 0},
                    .api_version    = {.major = 1, .minor = 4, .patch = 0},
                },
            .extensions = required_extensions(),
            .layers     = {"VK_LAYER_KHRONOS_validation"},
        });

        vk::surface_khr surface_raw;
        glfwCreateWindowSurface((VkInstance)instance.handle(), wnd, nullptr, (VkSurfaceKHR*)&surface_raw);
        surface         = vk::surface_t{instance, surface_raw};
        physical_device = find_suitable_physical_device(instance, surface);
        families        = *find_queue_families(physical_device, surface);

        dev = physical_device.create_device(
            vk::info::device{
                .queues     = {{.family_index = families.graphics}, {.family_index = families.present}},
                .extensions = {"VK_KHR_swapchain"},
                .chained    = core::tuple{
                    vk::physical_device_features2{.features = {.shader_clip_distance = true, .shader_cull_distance = true}},
                    vk::physical_device_timeline_semaphore_features{.timeline_semaphore = true},
                    vk::physical_device_vulkan11_features{.shader_draw_parameters = true},
                    vk::physical_device_vulkan13_features{.synchronization2 = true, .dynamic_rendering = true},
                    vk::physical_device_extended_dynamic_state_features_ext{.extended_dynamic_state = true},
                },
            }
        );

        graphics_queue = dev.get_device_queue(families.graphics, 0);
        present_queue  = dev.get_device_queue(families.present, 0);

        create_swapchain();

        namespace state   = vk::info::pipeline::state;
        pipeline_layout   = dev.create_pipeline_layout(vk::info::pipeline_layout{});
        graphics_pipeline = dev.create_graphics_pipeline(
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
                     .input_assembly = {state::input_assembly{.topology = vk::primitive_topology::triangle_list}},
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
                .layout = pipeline_layout,
                .chained = core::tuple{vk::pipeline_rendering_create_info{
                    .color_attachment_count   = 1,
                    .color_attachment_formats = &swapchain_surface_format.format,
                }},
            }
        );

        command_pool = dev.create_command_pool(
            vk::info::command_pool{
                .flags              = vk::command_pool_create_flag::reset_command_buffer,
                .queue_family_index = families.graphics,
            }
        );

        create_vertex_buffer();
        create_index_buffer();

        command_buffers = dev.allocate_command_buffers(
            vk::info::command_buffer{
                .command_pool         = command_pool,
                .level                = vk::command_buffer_level::primary,
                .command_buffer_count = frames_in_flight,
            }
        );
    }

    ~hello_triangle() {
        glfwDestroyWindow(wnd);
        glfwTerminate();
    }

    void run() {
        auto timeline_sem               = dev.create_typed_semaphore(vk::semaphore_type::timeline);
        u64  timeline_value             = 0;
        auto image_available_semaphores = filled_with(frames_in_flight, [&] { return dev.create_typed_semaphore(vk::semaphore_type::binary); }) | to_vector{};
        auto in_flight_fences = filled_with(frames_in_flight, [&] { return dev.create_fence({.flags = vk::fence_create_flag::signaled}); }) | to_vector{};

        u32               frame = 0;
        util::fps_counter fps;

        while (!glfwWindowShouldClose(wnd)) {
            glfwPollEvents();

            dev.wait(in_flight_fences[frame]);

            auto acquire_result = swapchain.acquire_next_image(image_available_semaphores[frame]);
            if (acquire_result.rc == vk::result::error_out_of_date_khr) {
                recreate_swapchain();
                continue;
            }
            auto image_index = acquire_result.value();

            dev.reset(in_flight_fences[frame]);

            /* Render pass */
            if (auto buff = vk::with_buffer(command_buffers[frame], vk::command_buffer_reset_flag{})) {
                transition_image_layout(
                    *buff,
                    image_index,
                    vk::image_layout::undefined,
                    vk::image_layout::color_attachment_optimal,
                    {.access = vk::access2_flags::none, .stage = vk::pipeline_stage2_flags::color_attachment_output},
                    {.access = vk::access2_flags::color_attachment_write, .stage = vk::pipeline_stage2_flags::color_attachment_output}
                );

                if (auto rendering_guard = vk::with_rendering(
                        buff,
                        vk::info::rendering{
                            .render_area       = {.offset = {.x = 0, .y = 0}, .extent = swapchain_extent},
                            .layer_count       = 1,
                            .color_attachments = {{
                                .image_view   = image_views[image_index],
                                .image_layout = vk::image_layout::color_attachment_optimal,
                                .load_op      = vk::attachment_load_op::clear,
                                .store_op     = vk::attachment_store_op::store,
                                .clear_value  = {vk::clear_color_value{0.f, 0.f, 0.f, 0.f}},
                            }}
                        }
                    )) {
                    buff->bind_pipeline(vk::pipeline_bind_point::graphics, graphics_pipeline);
                    buff->set_viewport(
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
                    buff->set_scissor(0, array{vk::rect2d{.offset = {.x = 0, .y = 0}, .extent = swapchain_extent}});
                    buff->bind_vertex_buffer(0, vertex_b.buffer, 0);
                    buff->bind_index_buffer(index_b.buffer, 0, vk::index_type::uint32);
                    buff->draw_indexed(u32(indices.size()), 1, 0, 0, 0);
                }

                transition_image_layout(
                    *buff,
                    image_index,
                    vk::image_layout::color_attachment_optimal,
                    vk::image_layout::present_src_khr,
                    {.access = vk::access2_flags::color_attachment_write, .stage = vk::pipeline_stage2_flags::color_attachment_output},
                    {.access = vk::access2_flags::none, .stage = vk::pipeline_stage2_flags::bottom_of_pipe}
                );
            }

            /* Queue submit */
            auto timeline_signal = ++timeline_value;
            graphics_queue
                .submit(
                    vk::info::submit{
                        .wait_semaphores   = {vk::info::submit::wait_info{
                              .semaphore = image_available_semaphores[frame],
                              .flags     = vk::pipeline_stage_flag::color_attachment_output,
                        }},
                        .command_buffers   = {command_buffers[frame]},
                        .signal_semaphores = {vk::info::submit::signal_info{
                            .semaphore      = timeline_sem,
                            .timeline_value = timeline_signal,
                        }}
                    },
                    in_flight_fences[frame]
                )
                .throws();

            dev.wait(timeline_sem, timeline_signal);

            auto present_result = present_queue.present(
                vk::info::present{
                    .swapchains = {vk::info::present::swapchain{.swapchain = swapchain, .image_index = image_index}},
                }
            );
            if (present_result.rc == vk::result::suboptimal_khr || present_result.rc == vk::result::error_out_of_date_khr || framebuffer_resized) {
                framebuffer_resized = false;
                recreate_swapchain();
            }

            frame = (frame + 1) % frames_in_flight;

            log.warn_update(1, "{} {}", fps.calculate(), timeline_value);
        }
    }

private:
    static inline constexpr u32 frames_in_flight = 2;

private:
    static std::vector<std::string> required_extensions() {
        u32  count      = 0;
        auto extensions = glfwGetRequiredInstanceExtensions(&count);
        return std::span{extensions, count} | transform{[](auto v) { return std::string(v); }} | to_vector{};
    }

    static void framebuffer_resize_callback(GLFWwindow* window, int, int) {
        auto app                 = reinterpret_cast<hello_triangle*>(glfwGetWindowUserPointer(window));
        app->framebuffer_resized = true;
    }

    struct transition_params {
        vk::access2_flags         access;
        vk::pipeline_stage2_flags stage;
    };

    void transition_image_layout(
        vk::command_buffer_t& buff, u32 image_index, vk::image_layout old, vk::image_layout lnew, transition_params src, transition_params dst
    ) {
        buff.pipeline_barrier2(
            vk::info::dependency{
                .image_memory_barriers = {{
                    .src_stage_mask         = src.stage,
                    .src_access_mask        = src.access,
                    .dst_stage_mask         = dst.stage,
                    .dst_access_mask        = dst.access,
                    .old_layout             = old,
                    .new_layout             = lnew,
                    .src_queue_family_index = vk::constants::queue_family_ignored,
                    .dst_queue_family_index = vk::constants::queue_family_ignored,
                    .image                  = swapchain_images[image_index],
                    .subresource_range      = {
                             .aspect_mask      = vk::image_aspect_flags::color,
                             .base_mip_level   = 0,
                             .level_count      = 1,
                             .base_array_layer = 0,
                             .layer_count      = 1,
                    },
                }}
            }
        );
    }

    void create_swapchain() {
        auto swapchain_support   = query_swapchain_support(physical_device, surface);
        swapchain_surface_format = swapchain_support.select_format();
        swapchain_extent         = swapchain_support.select_extent(wnd);

        swapchain = dev.create_swapchain(
            vk::info::swapchain{
                .surface         = surface,
                .min_image_count = swapchain_support.select_min_image_count(),
                .image =
                    {
                        .format       = swapchain_surface_format,
                        .extent       = swapchain_extent,
                        .array_layers = 1,
                        .usage        = vk::image_usage_flag::color_attachment,
                        .sharing_mode = vk::sharing_mode::exclusive,
                    },
                .queue_family_indices = {families.graphics, families.present},
                .pre_transform        = swapchain_support.capabilities.current_transform,
                .composite_alpha      = vk::composite_alpha_khr_flag::opaque_khr,
                .present_mode         = swapchain_support.select_present_mode(),
                .clipped              = true
            }
        );

        swapchain_images = swapchain.images().value();

        image_views = swapchain_images | transform{[&](const vk::image& image) {
                          return dev.create_image_view(
                              vk::image_view_create_info{
                                  .image             = image,
                                  .view_type         = vk::image_view_type::_2d,
                                  .format            = swapchain_surface_format.format,
                                  .components        = vk::arg::identity,
                                  .subresource_range = {
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
    }

    void recreate_swapchain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(wnd, &width, &height);
        while (width == 0 || height == 0) {
            log.info("wait");
            glfwGetFramebufferSize(wnd, &width, &height);
            glfwWaitEvents();
        }
        log.info("new size: {} {}", width, height);

        dev.wait_idle().checked();
        create_swapchain();
    }

    struct buffer_result {
        vk::buffer_t        buffer;
        vk::device_memory_t memory;
    };

    buffer_result create_buffer(vk::device_size_t size, vk::buffer_usage_flags usage, vk::memory_property_flags properties) {
        auto buff         = dev.create_buffer({.size = size, .usage = usage, .sharing_mode = vk::sharing_mode::exclusive});
        auto requirements = buff.memory_requirements();
        auto memory =
            dev.allocate_memory({.allocation_size = requirements.size, .memory_type_index = find_memory_type(requirements.memory_type_bits, properties)});
        buff.bind_memory(memory, 0).throws();
        return {.buffer = mov(buff), .memory = mov(memory)};
    }

    void copy_buffer(vk::buffer_t& src_buffer, vk::buffer_t& dst_buffer, vk::device_size_t size) {
        auto cmd_buffs = dev.allocate_command_buffers({.command_pool = command_pool, .level = vk::command_buffer_level::primary, .command_buffer_count = 1});
        if (auto cmd_buff = vk::with_buffer(cmd_buffs[0], {.flags = vk::command_buffer_usage_flags::one_time_submit})) {
            cmd_buff->copy_buffer(src_buffer, dst_buffer, std::vector{vk::buffer_copy{.src_offset = 0, .dst_offset = 0, .size = size}});
        }
        graphics_queue.submit(vk::info::submit{.command_buffers = cmd_buffs.handles()}).throws();
        graphics_queue.wait_idle().throws();
    }

    void create_vertex_buffer() {
        auto size = sizeof(vertices[0]) * vertices.size();
        auto staging =
            create_buffer(size, vk::buffer_usage_flags::transfer_src, vk::memory_property_flags::host_visible | vk::memory_property_flags::host_coherent);
        auto data_staging = staging.memory.map_memory(0, size).value();
        std::memcpy(data_staging, vertices.data(), size);
        staging.memory.unmap_memory();
        vertex_b = create_buffer(size, vk::buffer_usage_flags::transfer_dst | vk::buffer_usage_flags::vertex_buffer, vk::memory_property_flags::device_local);
        copy_buffer(staging.buffer, vertex_b.buffer, size);
    }

    void create_index_buffer() {
        auto size = sizeof(indices[0]) * indices.size();
        auto staging =
            create_buffer(size, vk::buffer_usage_flags::transfer_src, vk::memory_property_flags::host_visible | vk::memory_property_flags::host_coherent);
        auto data_staging = staging.memory.map_memory(0, size).value();
        std::memcpy(data_staging, indices.data(), size);
        staging.memory.unmap_memory();
        index_b = create_buffer(size, vk::buffer_usage_flags::transfer_dst | vk::buffer_usage_flags::index_buffer, vk::memory_property_flags::device_local);
        copy_buffer(staging.buffer, index_b.buffer, size);
    }

    u32 find_memory_type(u32 type_filter, vk::memory_property_flags properties) {
        auto memory_properties = physical_device.memory_properties();
        auto mem_types = std::span{memory_properties.memory_types, memory_properties.memory_type_count};

        for (auto&& [idx, mem_type] : with_index(mem_types)) {
            if (type_filter & (1 << idx) && mem_type.property_flags.test(properties)) {
                return u32(idx);
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

private:
    GLFWwindow*                   wnd;
    vk::vk_lib                    lib;
    vk::instance_t                instance;
    vk::surface_t                 surface;
    vk::physical_device_t         physical_device;
    queue_family_indices          families;
    vk::device_t                  dev;
    vk::queue_t                   graphics_queue;
    vk::queue_t                   present_queue;
    vk::swapchain_t               swapchain;
    vk::surface_format_khr        swapchain_surface_format;
    vk::extent2d                  swapchain_extent;
    std::vector<vk::image>        swapchain_images;
    std::vector<vk::image_view_t> image_views;
    vk::pipeline_layout_t         pipeline_layout;
    vk::pipeline_t                graphics_pipeline;

    buffer_result vertex_b;
    buffer_result index_b;

    vk::command_pool_t            command_pool;
    vk::command_buffer_store_t    command_buffers;

    bool framebuffer_resized = false;
};

int main() {
    hello_triangle{}.run();
}
