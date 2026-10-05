// Renders a gun model from the Tacz gun pack (../tacz-default-gunpack) held
// in first-person view. The gun's geometry uses the Minecraft Bedrock format
// ("minecraft:geometry"); see tacz_geo.hpp for the parser. Camera and input
// handling are based on vk_perspective_camera.cpp.
//
// Animations: the gun's animation file is parsed into keyframe animations
// (see tacz_geo.hpp); the static_idle one gives the resting pose. Space
// cycles the selected animation, left mouse click plays it from the start;
// the mesh is rebuilt on the CPU every frame while playing. Non-looping
// animations stop at their end (looping ones, e.g. shoot, keep going).

#include <chrono>
#include <grx/vk.hpp>
#include <grx/vk/constants.cg.hpp>
#include <grx/vk/device_memory.cg.hpp>
#include <grx/vk/enums.cg.hpp>
#include <grx/vk/flags.cg.hpp>
#include <grx/vk/info.hpp>
#include <grx/vk/structs.cg.hpp>
#include <util/fps_counter.hpp>
#include <grx/basic_types.hpp>

#include <grx/image.hpp>

#include <algorithm>
#include <cmath>

#include <core/io/file.hpp>
#include <core/poller.hpp>
#include <sys/event.hpp>
#include <sys/open_flags.hpp>
#include <sys/read.hpp>
#include <util/arg_parse.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "tacz_geo.hpp"

#include <core/ranges/transform.hpp>
#include <core/ranges/to.hpp>
#include <core/ranges/zip.hpp>
#include <util/log.hpp>

using namespace core;

// The Tacz gun pack lives next to this repository.
static constexpr auto kGeoDir = "../tacz-default-gunpack/assets/tacz/geo_models/gun/";
static constexpr auto kTexDir = "../tacz-default-gunpack/assets/tacz/textures/gun/uv/";
static constexpr auto kAnimDir = "../tacz-default-gunpack/assets/tacz/animations/";

struct ubo_t {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    f32       depth_bias; /* per depth level, in eye-space units, see tacz_gun.slang */
};

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
        if (dev.find_queue_family_indices(surface) && dev.support_extensions("VK_KHR_swapchain")) {
            auto [_, formats, present_modes] = query_swapchain_support(dev, surface);
            if (!formats.empty() && !present_modes.empty()) {
                return dev;
            }
        }
    }

    throw std::runtime_error("Suitable physical device not found");
}

struct vertex {
    grx::vec3f    pos;
    grx::vec3f    color;
    grx::vec2f    coord;
    f32           level; /* per-face depth level, see tacz_gun.slang */
    grx::vec3f    nrm;   /* face normal, for the shader-side depth shift */

    static constexpr vk::vertex_input_binding_description binding_description() {
        return {.binding = 0, .stride = sizeof(vertex), .input_rate = vk::vertex_input_rate::vertex};
    }

    static constexpr auto attribute_description() {
        return std::vector{
            vk::vertex_input_attribute_description{.location = 0, .binding = 0, .format = vk::format::r32g32b32_sfloat, .offset = offsetof(vertex, pos)},
            vk::vertex_input_attribute_description{.location = 1, .binding = 0, .format = vk::format::r32g32b32_sfloat, .offset = offsetof(vertex, pos)},
            vk::vertex_input_attribute_description{.location = 2, .binding = 0, .format = vk::format::r32g32_sfloat, .offset = offsetof(vertex, coord)},
            vk::vertex_input_attribute_description{.location = 3, .binding = 0, .format = vk::format::r32_sfloat, .offset = offsetof(vertex, level)},
            vk::vertex_input_attribute_description{.location = 4, .binding = 0, .format = vk::format::r32g32b32_sfloat, .offset = offsetof(vertex, nrm)},
        };
    }

    bool operator==(const vertex& rhs) const {
        return pos.binary_equal(rhs.pos) && color.binary_equal(rhs.color) && coord.binary_equal(rhs.coord) && level == rhs.level && nrm.binary_equal(rhs.nrm);
    }
};

template <>
struct std::hash<vertex> {
    size_t operator()(const vertex& vert) const noexcept {
        return hash<grx::vec3f>()(vert.pos) ^ hash<grx::vec3f>()(vert.color) ^ hash<grx::vec2f>()(vert.coord) ^ hash<f32>()(vert.level) ^ hash<grx::vec3f>()(vert.nrm);
    }
};

/* Per-frame state filled by the input handlers below */
struct camera_input {
    i32  forward     = 0; /* W */
    i32  backward    = 0; /* S */
    i32  left        = 0; /* A */
    i32  right       = 0; /* D */
    i32  dx          = 0; /* mouse horizontal delta (counts) */
    i32  dy          = 0; /* mouse vertical delta (counts) */
    bool quit        = false;
    bool switch_anim = false; /* space: cycle to the next animation */
    bool play_anim   = false; /* left mouse click: play the selected animation */

    void reset_relative() {
        dx = dy = 0;
    }
};

/* Read keyboard events from a linux input device, same pattern as examples/mouse_streamer.cpp */
auto handle_keyboard(sys::fd_t fd, camera_input& input) {
    return poll_handle{
        fd,
        sys::poll_event::in,
        [fd, &input](auto&&) {
            array<sys::event, 4> buff;

            if (auto count = sys::read(fd, buff)) {
                for (auto&& event : std::span{buff}.subspan(0, *count)) {
                    event.dispatch([&](sys::event_key_code code, i32 value) {
                        if (value < 0)
                            return; /* ignore key autorepeat */
                        switch (code) {
                        case sys::event_key_code::w:
                            input.forward = value;
                            break;
                        case sys::event_key_code::s:
                            input.backward = value;
                            break;
                        case sys::event_key_code::a:
                            input.left = value;
                            break;
                        case sys::event_key_code::d:
                            input.right = value;
                            break;
                        case sys::event_key_code::space:
                            if (value)
                                input.switch_anim = true;
                            break;
                        case sys::event_key_code::esc:
                            if (value)
                                input.quit = true;
                            break;
                        default:
                            break;
                        }
                    });
                }
            }
        },
    };
}

/* Read mouse relative motion (REL_X / REL_Y) from a linux input device */
auto handle_mouse(sys::fd_t fd, camera_input& input) {
    return poll_handle{
        fd,
        sys::poll_event::in,
        [fd, &input](auto&&) {
            array<sys::event, 8> buff;

            if (auto count = sys::read(fd, buff)) {
                for (auto&& event : std::span{buff}.subspan(0, *count)) {
                    event.dispatch(
                        [&](sys::event_relative_code code, i32 value) {
                            switch (code) {
                            case sys::event_relative_code::x:
                                input.dx += value;
                                break;
                            case sys::event_relative_code::y:
                                input.dy += value;
                                break;
                            default:
                                break;
                            }
                        },
                        [&](sys::event_button_code code, i32 value) {
                            if (code == sys::event_button_code::left && value)
                                input.play_anim = true;
                        });
                }
            }
        },
    };
}

class tacx_gun {
public:
    util::logger& log = glog();

    tacx_gun(std::string ikbd_path, std::string imouse_path, core::opt<std::string> igun_name, core::opt<f32> ifov, core::opt<f32> iscale):
        kbd_path(mov(ikbd_path)),
        mouse_path(mov(imouse_path)),
        gun_name(igun_name.value_or(std::string{"ak47"})),
        fov(ifov.value_or(70.f)),
        scale(iscale.value_or(3.f)) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // NOLINTNEXTLINE
        wnd = glfwCreateWindow(800, 600, "tacz gun", nullptr, nullptr);
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
        families        = *physical_device.find_queue_family_indices(surface);

        dev = physical_device.create_device(
            vk::info::device{
                .queues     = {{.family_index = families.graphics}, {.family_index = families.present}},
                .extensions = {"VK_KHR_swapchain"},
                .chained    = core::tuple{
                    vk::physical_device_features2{.features = {.sampler_anisotropy = true, .shader_clip_distance = true, .shader_cull_distance = true}},
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

        descriptor_set_layout = dev.create_descriptor_set_layout(
            vk::info::descriptor_set_layout{
                .bindings = {
                    vk::descriptor_set_layout_binding{
                        .binding          = 0,
                        .descriptor_type  = vk::descriptor_type::uniform_buffer,
                        .descriptor_count = 1,
                        .stage_flags      = vk::shader_stage_flag::vertex,
                    },
                    vk::descriptor_set_layout_binding{
                        .binding          = 1,
                        .descriptor_type  = vk::descriptor_type::combined_image_sampler,
                        .descriptor_count = 1,
                        .stage_flags      = vk::shader_stage_flag::fragment,
                    },
                }
            }
        );

        namespace state   = vk::info::pipeline::state;
        pipeline_layout   = dev.create_pipeline_layout(vk::info::pipeline_layout{.set_layouts = {descriptor_set_layout}});
        graphics_pipeline = dev.create_graphics_pipeline(
            null,
            vk::info::graphics_pipeline{
                .stages =
                    {vk::info::pipeline_shader_stage{
                         .stage  = vk::shader_stage_flag::vertex,
                         .module = dev.create_shader_module(vk::arg::spirv_file{"examples/tacz_gun.spirv"}),
                         .name   = "vertMain",
                     },
                     vk::info::pipeline_shader_stage{
                         .stage  = vk::shader_stage_flag::fragment,
                         .module = dev.create_shader_module(vk::arg::spirv_file{"examples/tacz_gun.spirv"}),
                         .name   = "fragMain",
                     }},
                .states =
                    {.vertex_input   = {state::vertex_input{
                           .vertex_binding_descriptions   = std::vector{vertex::binding_description()},
                           .vertex_attribute_descriptions = vertex::attribute_description()
                     }},
                     .input_assembly = {state::input_assembly{.topology = vk::primitive_topology::triangle_list}},
                     .viewport       = {state::viewport{.viewport_count = 1, .scissor_count = 1}},
                     .rasterization  = {state::rasterization{
                          .depth_clamp_enable        = false,
                          .rasterizer_discard_enable = false,
                          .polygon_mode              = vk::polygon_mode::fill,
                          .cull_mode                 = vk::cull_mode_flag::back, // quads are wound CCW from the outside; MC draws item model faces with culling enabled (it only disables culling for arm skins and maps)
                          .front_face                = vk::front_face::counter_clockwise,
                          .line_width                = 1.f,
                     }},
                     .multisample    = {state::multisample{.rasterization_samples = vk::sample_count_flag::_1}},
                     .depth_stencil  = {vk::info::pipeline::state::depth_stencil{
                          .depth_test_enable        = vk::constants::vk_true,
                          .depth_write_enable       = vk::constants::vk_true,
                          .depth_compare_op         = vk::compare_op::less,
                          .depth_bounds_test_enable = vk::constants::vk_false,
                          .stencil_test_enable      = vk::constants::vk_false,
                     }},
                     .color_blend    = {state::color_blend{
                            .logic_op_enable = false,
                            .logic_op        = vk::logic_op::copy,
                            .attachments     = {state::color_blend::attachment{.color_write_mask = vk::arg::color_component_rgba}},
                     }},
                     .dynamic        = {state::dynamic{.dynamic_states = {vk::dynamic_state::viewport, vk::dynamic_state::scissor}}}},
                .layout  = pipeline_layout,
                .chained = core::tuple{vk::pipeline_rendering_create_info{
                    .color_attachment_count   = 1,
                    .color_attachment_formats = &swapchain_surface_format.format,
                    .depth_attachment_format  = find_depth_format(),
                }},
            }
        );

        command_pool = dev.create_command_pool(
            vk::info::command_pool{
                .flags              = vk::command_pool_create_flag::reset_command_buffer,
                .queue_family_index = families.graphics,
            }
        );

        create_depth_buffer();
        create_texture();

        load_model();
        create_vertex_buffer(size_t(max_faces) * 4);
        create_index_buffer(size_t(max_faces) * 6);
        create_uniform_buffers();

        descriptor_pool = dev.create_descriptor_pool(
            vk::info::descriptor_pool{
                .flags      = vk::descriptor_pool_create_flag::free_descriptor_set,
                .max_sets   = frames_in_flight,
                .pool_sizes = {
                    vk::descriptor_pool_size{.type = vk::descriptor_type::uniform_buffer, .descriptor_count = frames_in_flight},
                    vk::descriptor_pool_size{.type = vk::descriptor_type::combined_image_sampler, .descriptor_count = frames_in_flight},
                }
            }
        );

        descriptor_sets = dev.allocate_descriptor_sets(
            vk::info::descriptor_set{
                .descriptor_pool = descriptor_pool,
                .set_layouts     = std::vector<vk::descriptor_set_layout>(frames_in_flight, descriptor_set_layout),
            }
        );

        for (size_t i = 0; i < frames_in_flight; ++i) {
            vk::descriptor_buffer_info buffer_info{.buffer = uniform_buffers[i].buffer, .offset = 0, .range = sizeof(ubo_t)};
            vk::descriptor_image_info  image_info{
                 .sampler      = texture_sampler,
                 .image_view   = texture_image_view,
                 .image_layout = vk::image_layout::shader_read_only_optimal,
            };
            array descriptor_writes = array{
                vk::write_descriptor_set{
                    .dst_set           = descriptor_sets[i],
                    .dst_binding       = 0,
                    .dst_array_element = 0,
                    .descriptor_count  = 1,
                    .descriptor_type   = vk::descriptor_type::uniform_buffer,
                    .image_info        = nullptr,
                    .buffer_info       = &buffer_info,
                    .texel_buffer_view = nullptr,
                },
                vk::write_descriptor_set{
                    .dst_set           = descriptor_sets[i],
                    .dst_binding       = 1,
                    .dst_array_element = 0,
                    .descriptor_count  = 1,
                    .descriptor_type   = vk::descriptor_type::combined_image_sampler,
                    .image_info        = &image_info,
                    .buffer_info       = nullptr,
                    .texel_buffer_view = nullptr,
                },
            };

            dev.update_descriptor_sets(descriptor_writes, {});
        }

        command_buffers = dev.allocate_command_buffers(
            vk::info::command_buffer{
                .command_pool         = command_pool,
                .level                = vk::command_buffer_level::primary,
                .command_buffer_count = frames_in_flight,
            }
        );
    }

    ~tacx_gun() {
        glfwDestroyWindow(wnd);
        glfwTerminate();
    }

    void run() {
        auto timeline_sem               = dev.create_typed_semaphore(vk::semaphore_type::timeline);
        u64  timeline_value             = 0;
        auto image_available_semaphores = filled_with(frames_in_flight, [&] { return dev.create_typed_semaphore(vk::semaphore_type::binary); }) | to_vector{};
        auto in_flight_fences = filled_with(frames_in_flight, [&] { return dev.create_fence({.flags = vk::fence_create_flag::signaled}); }) | to_vector{};

        auto mouse = io::file::open(mouse_path, sys::openflags::read_only | sys::openflags::nonblock);
        auto kbd   = io::file::open(kbd_path, sys::openflags::read_only | sys::openflags::nonblock);

        camera_input          input;
        auto                  mouse_h = handle_mouse(mouse, input);
        auto                  kbd_h   = handle_keyboard(kbd, input);
        array<sys::pollfd, 2> fds;
        fds[0] = {.fd = mouse_h.fd, .events = mouse_h.events, .revents = sys::poll_event::none};
        fds[1] = {.fd = kbd_h.fd, .events = kbd_h.events, .revents = sys::poll_event::none};

        u32               frame = 0;
        util::fps_counter fps;
        auto              last_frame = std::chrono::high_resolution_clock::now();

        while (!glfwWindowShouldClose(wnd) && !input.quit) {
            glfwPollEvents();

            /* Drain pending input events (non-blocking) */
            if (auto count = sys::poll(fds, sys::nanoseconds{0})) {
                for (size_t i = 0; *count > 0 && i < fds.size(); ++i) {
                    if (fds[i].revents & fds[i].events) {
                        --*count;
                        if (i == 0)
                            mouse_h.handler(fds[i]);
                        else
                            kbd_h.handler(fds[i]);
                    }
                }
            }

            auto now = std::chrono::high_resolution_clock::now();
            const f32 dt = std::chrono::duration<f32>(now - last_frame).count();
            last_frame = now;

            /* Animation controls: space cycles the selection (and returns the
               gun to the idle pose), left click plays the selected animation
               from the start. */
            if (input.switch_anim && !anims_list.empty()) {
                selected_anim = (selected_anim + 1) % anims_list.size();
                playing = false;
                anim_time = 0.f;
                rebuild_mesh();
                std::memcpy(vertex_data, vertices.data(), sizeof(vertex) * vertices.size());
                std::memcpy(index_data, indices.data(), sizeof(u32) * indices.size());
                log.info("selected animation: '{}'", anims_list[selected_anim].name);
            }
            if (input.play_anim && !anims_list.empty()) {
                playing = true;
                anim_time = 0.f;
            }
            input.switch_anim = false;
            input.play_anim   = false;

            update_camera(input, dt);
            update_animation(dt);

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
                    swapchain_images[image_index],
                    vk::image_layout::undefined,
                    vk::image_layout::color_attachment_optimal,
                    {.access = vk::access2_flags::none, .stage = vk::pipeline_stage2_flags::color_attachment_output},
                    {.access = vk::access2_flags::color_attachment_write, .stage = vk::pipeline_stage2_flags::color_attachment_output},
                    vk::image_aspect_flag::color
                );

                transition_image_layout(
                    *buff,
                    depth_image,
                    vk::image_layout::undefined,
                    vk::image_layout::depth_attachment_optimal,
                    {.access = vk::access2_flag::depth_stencil_attachment_write,
                     .stage  = vk::pipeline_stage2_flag::early_fragment_tests | vk::pipeline_stage2_flag::late_fragment_tests},
                    {.access = vk::access2_flag::depth_stencil_attachment_write,
                     .stage  = vk::pipeline_stage2_flag::early_fragment_tests | vk::pipeline_stage2_flag::late_fragment_tests},
                    vk::image_aspect_flag::depth
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
                            }},
                            .depth_attachment  = vk::rendering_attachment_info{
                                 .image_view   = depth_image_view,
                                 .image_layout = vk::image_layout::depth_attachment_optimal,
                                 .load_op      = vk::attachment_load_op::clear,
                                 .store_op     = vk::attachment_store_op::dont_care,
                                 .clear_value  = {.depth_stencil = vk::clear_depth_stencil_value{.depth = 1.f, .stencil = 0}}
                            },
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
                    buff->bind_index_buffer(index_b.buffer, 0, vk::to_index_type(core::type_of(indices[0])));
                    buff->bind_descriptor_sets(vk::pipeline_bind_point::graphics, pipeline_layout, 0, {&descriptor_sets.handles()[frame], 1}, {});
                    buff->draw_indexed(u32(indices.size()), 1, 0, 0, 0);
                }

                transition_image_layout(
                    *buff,
                    swapchain_images[image_index],
                    vk::image_layout::color_attachment_optimal,
                    vk::image_layout::present_src_khr,
                    {.access = vk::access2_flags::color_attachment_write, .stage = vk::pipeline_stage2_flags::color_attachment_output},
                    {.access = vk::access2_flags::none, .stage = vk::pipeline_stage2_flags::bottom_of_pipe},
                    vk::image_aspect_flag::color
                );
            }

            update_uniform_buffer(frame);

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
        auto app          = reinterpret_cast<tacx_gun*>(glfwGetWindowUserPointer(window));
        app->framebuffer_resized = true;
    }

    struct transition_params {
        vk::access2_flags         access;
        vk::pipeline_stage2_flags stage;
    };

    void transition_image_layout(
        vk::command_buffer_t&  buff,
        vk::image              image,
        vk::image_layout       old,
        vk::image_layout       lnew,
        transition_params      src,
        transition_params      dst,
        vk::image_aspect_flags aspect_flags
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
                    .image                  = image,
                    .subresource_range      = {
                             .aspect_mask      = aspect_flags,
                             .base_mip_level   = 0,
                             .level_count      = 1,
                             .base_array_layer = 0,
                             .layer_count      = 1,
                    },
                }}
            }
        );
    }

    void transition_texture_image_layout(const vk::command_buffer_t& buff, const vk::image_t& image, vk::image_layout old, vk::image_layout new_) {
        vk::image_memory_barrier barrier{
            .old_layout             = old,
            .new_layout             = new_,
            .src_queue_family_index = vk::constants::queue_family_ignored,
            .dst_queue_family_index = vk::constants::queue_family_ignored,
            .image                  = image,
            .subresource_range      = {
                     .aspect_mask = vk::image_aspect_flag::color,
                     .level_count = 1,
                     .layer_count = 1,
            },
        };

        vk::pipeline_stage_flags src_stage;
        vk::pipeline_stage_flags dst_stage;

        if (old == vk::image_layout::undefined && new_ == vk::image_layout::transfer_dst_optimal) {
            barrier.src_access_mask = {};
            barrier.dst_access_mask = vk::access_flag::transfer_write;
            src_stage = vk::pipeline_stage_flag::top_of_pipe;
            dst_stage = vk::pipeline_stage_flag::transfer;
        } else if (old == vk::image_layout::transfer_dst_optimal && new_ == vk::image_layout::shader_read_only_optimal) {
            barrier.src_access_mask = vk::access_flag::transfer_write;
            barrier.dst_access_mask = vk::access_flag::shader_read;
            src_stage = vk::pipeline_stage_flag::transfer;
            dst_stage = vk::pipeline_stage_flag::fragment_shader;
        } else {
            throw std::invalid_argument("unsupported layout transition");
        }

        buff.pipeline_barrier(src_stage, dst_stage, {}, {}, {}, {&barrier, 1});
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
        create_depth_buffer();
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

    /* The mesh is rebuilt on the CPU while an animation plays, so the
       vertex/index buffers live in host-visible memory and are updated in
       place (safe: the frame loop blocks on the timeline semaphore after
       every submit, so no GPU work can reference these buffers). The
       initial capacity is max_face_count * 4 / * 6 vertices/indices - the
       unclipped worst case. Clipped faces can emit MORE than 4 vertices
       (a piece per clipped-away region), so when a rebuilt mesh exceeds the
       capacity, ensure_mesh_capacity() recreates the buffers larger (the
       per-face count stays bounded by tacz::kMaxClipTriangles, and animated
       frames are usually SMALLER than the idle pose - a scale-0 channel
       hides its cubes). */
    void create_vertex_buffer(size_t cap) {
        vertex_cap = std::max(cap, vertices.size());
        auto size  = sizeof(vertex) * vertex_cap;
        vertex_b   = create_buffer(size, vk::buffer_usage_flags::vertex_buffer, vk::memory_property_flags::host_visible | vk::memory_property_flags::host_coherent);
        vertex_data = vertex_b.memory.map_memory(0, size).value();
        std::memcpy(vertex_data, vertices.data(), sizeof(vertex) * vertices.size());
    }

    void create_index_buffer(size_t cap) {
        index_cap = std::max(cap, indices.size());
        auto size  = sizeof(u32) * index_cap;
        index_b   = create_buffer(size, vk::buffer_usage_flags::index_buffer, vk::memory_property_flags::host_visible | vk::memory_property_flags::host_coherent);
        index_data = index_b.memory.map_memory(0, size).value();
        std::memcpy(index_data, indices.data(), sizeof(u32) * indices.size());
    }

    /* Recreate the mapped mesh buffers when a rebuilt mesh no longer fits
       them. Only safe to do where no GPU work is in flight: the frame loop
       blocks on the timeline semaphore after every submit, and the initial
       build precedes any submit. */
    void ensure_mesh_capacity(size_t nv, size_t ni) {
        if (nv <= vertex_cap && ni <= index_cap)
            return;
        if (!vertex_data)
            return; /* initial sizing happens in create_*_buffer */
        auto nv2 = std::max(nv, vertex_cap * 2 + 16);
        auto ni2 = std::max(ni, index_cap * 2 + 16);
        vertex_b.memory.unmap_memory();
        index_b.memory.unmap_memory();
        vertex_b = {};
        index_b  = {};
        create_vertex_buffer(nv2);
        create_index_buffer(ni2);
        log.info("mesh buffers grew to {} vertices / {} indices", nv2, ni2);
    }

    void create_depth_buffer() {
        auto depth_format = find_depth_format();

        depth_image = dev.create_image({
            .image_type   = vk::image_type::_2d,
            .format       = depth_format,
            .extent       = {.width = swapchain_extent.width, .height = swapchain_extent.height, .depth = 1},
            .mip_levels   = 1,
            .array_layers = 1,
            .samples      = vk::sample_count_flag::_1,
            .tiling       = vk::image_tiling::optimal,
            .usage        = vk::image_usage_flag::depth_stencil_attachment,
            .sharing_mode = vk::sharing_mode::exclusive,
        });

        auto requirements  = depth_image.memory_requirements();
        depth_image_memory = dev.allocate_memory(
            {.allocation_size = requirements.size, .memory_type_index = find_memory_type(requirements.memory_type_bits, vk::memory_property_flag::device_local)}
        );
        depth_image.bind_memory(depth_image_memory, 0).throws();

        depth_image_view = dev.create_image_view({
            .image             = depth_image,
            .view_type         = vk::image_view_type::_2d,
            .format            = depth_format,
            .subresource_range = {
                .aspect_mask      = vk::image_aspect_flag::depth,
                .base_mip_level   = 0,
                .level_count      = 1,
                .base_array_layer = 0,
                .layer_count      = 1,
            },
        });
    }

    void create_texture() {
        auto image = grx::image_rgba::from_image(
            io::mmap{io::file::open(std::string{kTexDir} + gun_name + ".png", sys::openflag::read_only), sys::map_prot::read, sys::map_flag::priv}.span()
        );
        auto staging = create_buffer(
            image.size_in_bytes(), vk::buffer_usage_flag::transfer_src, vk::memory_property_flag::host_visible | vk::memory_property_flag::host_coherent
        );
        auto data = staging.memory.map_memory(0, image.size_in_bytes()).value();
        std::memcpy(data, &image.data()->x(), image.size_in_bytes());
        staging.memory.unmap_memory();

        texture_image     = dev.create_image({
                .image_type   = vk::image_type::_2d,
                .format       = vk::format::r8g8b8a8_srgb,
                .extent       = {.width = image.size().x(), .height = image.size().y(), .depth = 1},
                .mip_levels   = 1,
                .array_layers = 1,
                .samples      = vk::sample_count_flag::_1,
                .tiling       = vk::image_tiling::optimal,
                .usage        = vk::image_usage_flag::transfer_dst | vk::image_usage_flag::sampled,
                .sharing_mode = vk::sharing_mode::exclusive,
        });
        auto requirements = texture_image.memory_requirements();

        texture_image_memory = dev.allocate_memory(
            {.allocation_size = requirements.size, .memory_type_index = find_memory_type(requirements.memory_type_bits, vk::memory_property_flag::device_local)}
        );
        texture_image.bind_memory(texture_image_memory, 0).throws();

        auto command_buffers = dev.allocate_command_buffers({
            .command_pool         = command_pool,
            .level                = vk::command_buffer_level::primary,
            .command_buffer_count = 1,
        });
        auto command_buffer = command_buffers[0];
        if (auto buff = vk::with_buffer(command_buffer, {.flags = vk::command_buffer_usage_flag::one_time_submit})) {
            transition_texture_image_layout(*buff, texture_image, vk::image_layout::undefined, vk::image_layout::transfer_dst_optimal);
            vk::buffer_image_copy region{
                .buffer_offset       = 0,
                .buffer_row_length   = 0,
                .buffer_image_height = 0,
                .image_subresource =
                    {
                        .aspect_mask      = vk::image_aspect_flag::color,
                        .mip_level        = 0,
                        .base_array_layer = 0,
                        .layer_count      = 1,
                    },
                .image_offset = {.x = 0, .y = 0, .z = 0},
                .image_extent = {.width = image.size().x(), .height = image.size().y(), .depth = 1},
            };
            buff->copy_buffer_to_image(staging.buffer, texture_image, vk::image_layout::transfer_dst_optimal, {&region, 1});
            transition_texture_image_layout(*buff, texture_image, vk::image_layout::transfer_dst_optimal, vk::image_layout::shader_read_only_optimal);
        }

        graphics_queue.submit(vk::info::submit{.command_buffers = {command_buffer}}).throws();
        graphics_queue.wait_idle().throws();

        // Create texture image view
        texture_image_view = dev.create_image_view({
            .image             = texture_image,
            .view_type         = vk::image_view_type::_2d,
            .format            = vk::format::r8g8b8a8_srgb,
            .subresource_range = {.aspect_mask = vk::image_aspect_flag::color, .base_mip_level = 0, .level_count = 1, .base_array_layer = 0, .layer_count = 1},
        });

        // Create texture sampler
        auto phys_props = physical_device.properties();
        texture_sampler = dev.create_sampler({
            .mag_filter        = vk::filter::linear,
            .min_filter        = vk::filter::linear,
            .mipmap_mode       = vk::sampler_mipmap_mode::linear,
            .address_mode_u    = vk::sampler_address_mode::repeat,
            .address_mode_v    = vk::sampler_address_mode::repeat,
            .address_mode_w    = vk::sampler_address_mode::repeat,
            .mip_lod_bias      = 0.0f,
            .anisotropy_enable = vk::constants::vk_true,
            .max_anisotropy    = phys_props.limits.max_sampler_anisotropy,
            .compare_enable    = vk::constants::vk_false,
            .compare_op        = vk::compare_op::always,
        });
    }

    struct uniform_buffer_t {
        vk::buffer_t        buffer;
        vk::device_memory_t memory;
        void*               data;
    };

    void create_uniform_buffers() {
        uniform_buffers.clear();

        for (size_t i = 0; i < frames_in_flight; ++i) {
            auto result = create_buffer(
                sizeof(ubo_t), vk::buffer_usage_flag::uniform_buffer, vk::memory_property_flag::host_visible | vk::memory_property_flag::host_coherent
            );

            auto data = result.memory.map_memory(0, sizeof(ubo_t)).value();
            uniform_buffers.push_back(
                uniform_buffer_t{
                    .buffer = mov(result.buffer),
                    .memory = mov(result.memory),
                    .data   = data,
                }
            );
        }
    }

    /* First-person camera: the mesh is built in eye space (eye at the
       origin), so the camera starts 4 units in front of the gun, looking
       down -Z (the muzzle points -Z). Controlled by WASD and mouse. */
    struct camera {
        camera():
            position(0.f, 0.f, 4.f),
            yaw(0.f),
            pitch(0.f) {}

        glm::vec3 forward() const {
            return {-std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
        }

        glm::vec3 right() const {
            return {std::cos(yaw) * std::cos(pitch), 0.f, -std::sin(yaw)};
        }

        glm::vec3 position;
        f32       yaw;   /* rotation around Y */
        f32       pitch; /* rotation around X */
    } cam;

    void update_camera(camera_input& input, f32 dt) {
        constexpr f32 move_speed  = 2.f; /* world units per second */
        constexpr f32 rotate_sens = 0.0025f; /* radians per mouse count */
        constexpr f32 pitch_limit = glm::pi<f32>() / 2 - 0.01f;

        cam.yaw   -= f32(input.dx) * rotate_sens;
        cam.pitch  = std::clamp(cam.pitch - f32(input.dy) * rotate_sens, -pitch_limit, pitch_limit);
        input.reset_relative();

        glm::vec3 move = cam.forward() * f32(input.forward - input.backward) + cam.right() * f32(input.right - input.left);
        cam.position  += move * move_speed * dt;
    }

    void update_uniform_buffer(u32 frame) {
        glm::vec3 up(0.f, 1.f, 0.f);
        ubo_t ubo{
            .model = model_matrix,
            .view  = lookAt(cam.position, cam.position + cam.forward(), up),
            .projection =
                glm::perspective(glm::radians(fov), static_cast<float>(swapchain_extent.width) / static_cast<float>(swapchain_extent.height), 0.1f, 100.0f),
            .depth_bias = 5e-5f * scale,
        };
        ubo.projection[1][1] *= -1;

        std::memcpy(uniform_buffers[frame].data, &ubo, sizeof(ubo));
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

    vk::format find_supported_format(std::span<const vk::format> candidates, vk::image_tiling tiling, vk::format_feature_flags features) {
        for (auto candidate : candidates) {
            auto props = physical_device.format_properties(candidate);
            if ((tiling == vk::image_tiling::linear && props.linear_tiling_features.have_all(features)) ||
                (tiling == vk::image_tiling::optimal && props.optimal_tiling_features.have_all(features))) {
                return candidate;
            }
        }
        throw std::runtime_error("failed to find supported format");
    }

    vk::format find_depth_format() {
        return find_supported_format(
            array{vk::format::d32_sfloat, vk::format::d32_sfloat_s8_uint, vk::format::d24_unorm_s8_uint},
            vk::image_tiling::optimal,
            vk::format_feature_flag::depth_stencil_attachment
        );
    }

    /* (Re)build the eye-space mesh for the current pose: the static idle
       pose, or the selected animation sampled at anim_time. */
    void rebuild_mesh() {
        std::map<std::string, tacz::bone_anim> anims = idle_anims;
        if (selected_anim < anims_list.size() && (playing || anim_time > 0.f)) {
            // Sampled channels override the static_idle ones; channels the
            // animation does not sample keep their static_idle values (the
            // gun pack authors keyframes as absolute poses that start from
            // the idle values, so the t = 0 frame is exactly the idle pose).
            for (auto& [name, ba] : tacz::sample_animation(anims_list[selected_anim], anim_time)) {
                auto& cur = anims[name];
                if (ba.position)
                    cur.position = ba.position;
                if (ba.rotation)
                    cur.rotation = ba.rotation;
                if (ba.scale)
                    cur.scale = ba.scale;
            }
        }
        auto mesh = tacz::build_mesh(geo, anims);

        vertices.clear();
        indices.clear();
        vertices.reserve(mesh.vertices.size());
        for (auto&& mv : mesh.vertices) {
            vertex v{};
            v.pos.set(mv.pos.x, mv.pos.y, mv.pos.z);
            v.color.set(1.f, 1.f, 1.f);
            v.coord.set(mv.uv.x, mv.uv.y);
            v.level = mv.level;
            v.nrm.set(mv.nrm.x, mv.nrm.y, mv.nrm.z);
            vertices.push_back(v);
        }
        indices = mov(mesh.indices);
        ensure_mesh_capacity(vertices.size(), indices.size());
    }

    /* Advance the playing animation by dt and push the rebuilt mesh into the
       vertex/index buffers. Non-looping animations stop at their end (the
       last frame then stays in the buffers until it is re-played or
       switched). */
    void update_animation(f32 dt) {
        if (!playing || selected_anim >= anims_list.size())
            return;
        const auto& a = anims_list[selected_anim];
        anim_time += dt;
        if (!a.loop && a.end_time > 0.f && anim_time >= a.end_time) {
            anim_time = a.end_time;
            playing = false; // stop at the end
        }
        rebuild_mesh();
        std::memcpy(vertex_data, vertices.data(), sizeof(vertex) * vertices.size());
        std::memcpy(index_data, indices.data(), sizeof(u32) * indices.size());
    }

    void load_model() {
        // Parse the Bedrock geometry and flatten it into a triangle mesh
        // already in first-person eye space (the TACZ camera chain is baked
        // in by tacz::build_mesh), so the model matrix is just a scale.
        geo = tacz::load_geometry(std::string{kGeoDir} + gun_name + "_geo.json");

        // Bake the static idle pose (places the hand/arm proxy cubes at the
        // grip, as in game); missing animation files are fine.
        const auto anim_path = std::string{kAnimDir} + gun_name + ".animation.json";
        try {
            idle_anims = tacz::load_idle_animation(anim_path);

            // Playable keyframe animations (the static_* entries are just
            // constant poses, not animations).
            for (auto a : tacz::load_animations(anim_path))
                if (!a.name.starts_with("static_"))
                    anims_list.push_back(mov(a));
            if (anims_list.empty())
                log.info("no playable animations for '{}'", gun_name);
            else {
                for (size_t i = 0; i < anims_list.size(); ++i)
                    log.info("animation {}: '{}'{}", i, anims_list[i].name, anims_list[i].loop ? " [loop]" : "");
            }
        } catch (...) {
            log.info("no animation file for '{}', using rest pose", gun_name);
        }

        rebuild_mesh();

        // Upper bound on the face count of any pose (see the buffer
        // creation comment); used for the initial buffer capacity.
        max_faces = tacz::max_face_count(geo);

        // Vanilla MC 1.12.2 idle main-hand transform (ItemRenderer.renderItemInFirstPerson,
        // right-handed player, at rest): the 45/-45 Y rotations cancel and the swing
        // offsets are zero, leaving transformSideFirstPerson = translate(0.56, -0.52, -0.72).
        // It is applied on top of the TACZ camera chain (which puts the model in eye space).
        const auto idle = glm::translate(glm::mat4{1.f}, glm::vec3{0.56f, -0.52f, -0.72f});
        model_matrix = glm::mat4{scale, 0.f, 0.f, 0.f, 0.f, scale, 0.f, 0.f, 0.f, 0.f, scale, 0.f, 0.f, 0.f, 0.f, 1.f} * idle;

        glm::vec3 mn{1e30f, 1e30f, 1e30f}, mx{-1e30f, -1e30f, -1e30f};
        for (auto&& v : vertices) {
            const auto p = glm::vec3(v.pos.x(), v.pos.y(), v.pos.z());
            mn = glm::min(mn, p);
            mx = glm::max(mx, p);
        }
        log.info("loaded gun '{}': {} verts, {} indices, eye-space bbox ({}, {}, {})..({}, {}, {})", gun_name, vertices.size(), indices.size(), mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]);
    }

private:
    GLFWwindow*                                 wnd;
    vk::vk_lib                                  lib;
    vk::instance_t                              instance;
    vk::surface_t                               surface;
    vk::physical_device_t                       physical_device;
    vk::physical_device_t::queue_family_indices families;
    vk::device_t                                dev;
    vk::queue_t                                 graphics_queue;
    vk::queue_t                                 present_queue;
    vk::swapchain_t                             swapchain;
    vk::surface_format_khr                      swapchain_surface_format;
    vk::extent2d                                swapchain_extent;
    std::vector<vk::image>                      swapchain_images;
    std::vector<vk::image_view_t>               image_views;
    glm::mat4                                   model_matrix{};
    vk::pipeline_layout_t                       pipeline_layout;
    vk::pipeline_t                              graphics_pipeline;

    std::vector<vertex> vertices;
    std::vector<u32> indices;
    u32 max_faces{};    // tacz::max_face_count(geo): initial buffer sizing bound
    size_t vertex_cap{}; /* mapped vertex buffer capacity, in vertices */
    size_t index_cap{};  /* mapped index buffer capacity, in indices */
    buffer_result vertex_b;
    buffer_result index_b;
    void*               vertex_data{}; /* mapped vertex buffer (host visible) */
    void*               index_data{};  /* mapped index buffer (host visible) */

    tacz::geometry                    geo{};
    std::map<std::string, tacz::bone_anim> idle_anims; /* "static_idle" pose */
    std::vector<tacz::keyframe_anim>  anims_list; /* playable (non-static) animations */
    size_t                            selected_anim = 0;
    bool                              playing = false;
    f32                               anim_time = 0.f; /* seconds into the selected animation */

    vk::command_pool_t            command_pool;
    vk::command_buffer_store_t    command_buffers;

    vk::descriptor_set_layout_t   descriptor_set_layout;
    vk::descriptor_pool_t         descriptor_pool;
    vk::descriptor_set_store_t    descriptor_sets;
    std::vector<uniform_buffer_t> uniform_buffers;

    vk::image_t                   texture_image;
    vk::device_memory_t           texture_image_memory;
    vk::image_view_t              texture_image_view;
    vk::image_t                   depth_image;
    vk::device_memory_t           depth_image_memory;
    vk::image_view_t              depth_image_view;
    vk::sampler_t                 texture_sampler;

    bool framebuffer_resized = false;

    std::string                   kbd_path;
    std::string                   mouse_path;
    std::string                   gun_name;
    f32                           fov;
    f32                           scale;
};

tbc_cmd(main) {
    tbc_arg(kbd, std::string, "path to keyboard input device"_ctstr);
    tbc_arg(mouse, std::string, "path to mouse input device"_ctstr);
    tbc_arg(gun, core::opt<std::string>, "gun model name in the Tacz gun pack (default: ak47)"_ctstr);
    tbc_arg(fov, core::opt<f32>, "field of view in degrees (default: 70)"_ctstr) = 70.f;
    tbc_arg(scale, core::opt<f32>, "gun scale (default: 3)"_ctstr) = 3.f;
};

void tbc_main(main_cmd<> args) {
    tacx_gun(*args.kbd, *args.mouse, *args.gun, *args.fov, *args.scale).run();
}

#include <util/tbc_main.hpp>
