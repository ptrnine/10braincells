#include <codegen/vulkan/enum_generator.hpp>
#include <codegen/vulkan/structs.hpp>
#include <codegen/vulkan/constants.hpp>
#include <codegen/vulkan/types.hpp>
#include <codegen/vulkan/commands.hpp>
#include <codegen/vulkan/class.hpp>

#include <core/io/std.hpp>

#include <pugixml.hpp>
#include <util/arg_parse.hpp>

using namespace core;
using namespace util;

tbc_cmd(vulkan) {
    tbc_arg(api, std::string, "path to vk.xml API"_ctstr);
    tbc_arg(dir, std::string, "output directory"_ctstr);

    cmd_description description{"generate vulkan bindings"};
};

void generate_physical_device_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name = "inst";
    c.instance.type = "instance_t";
    c.handle.name   = "dev";
    c.handle.type   = "vk::physical_device";
    c.name          = "physical_device_t";
    c.functions     = {
        {.name = "get_physical_device_properties"},
        {.name = "get_physical_device_properties2", .type = cg::vk::gen_type::only_cache_this},
        {.name = "get_physical_device_features"},
        {.name = "get_physical_device_features2"},
        {.name = "get_physical_device_memory_properties"},
        {.name = "get_physical_device_memory_properties2"},
        {.name = "get_physical_device_queue_family_properties"},
        {.name = "get_physical_device_queue_family_properties2"},
        {.name = "get_physical_device_format_properties"},
        {.name = "get_physical_device_format_properties2"},
        {.name = "get_physical_device_image_format_properties"},
        {.name = "get_physical_device_image_format_properties2"},
        {.name = "get_physical_device_sparse_image_format_properties"},
        {.name = "get_physical_device_sparse_image_format_properties2"},
        {.name = "get_physical_device_external_buffer_properties"},
        {.name = "get_physical_device_external_semaphore_properties"},
        {.name = "get_physical_device_external_fence_properties"},
        {.name = "get_physical_device_surface_support_khr"},
        {.name = "get_physical_device_surface_capabilities_khr"},
        {.name = "get_physical_device_surface_formats_khr"},
        {.name = "get_physical_device_surface_present_modes_khr"},
        {.name = "create_device", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "device_t"},
        {.name = "enumerate_device_layer_properties"},
        {.name = "enumerate_device_extension_properties"},
        {.name = "get_physical_device_display_properties_khr"},
        {.name = "get_physical_device_display_plane_properties_khr"},
        {.name = "get_display_plane_supported_displays_khr"},
        {.name = "get_physical_device_display_properties2_khr"},
        {.name = "get_physical_device_multisample_properties_ext"},
        {.name = "get_drm_display_ext"},
    };
    c.func_prefixes = {
        "get_physical_device_",
        "enumerate_device_",
        "get_",
    };
    c.func_postfixes = {"_khr", "_ext"};
    c.definitions.push_back(
        "template <typename... Ts>\n"
        "    auto properties2() const {\n"
        "        auto func = f[cmd::get_physical_device_properties2];\n"
        "        core::tuple<vk::physical_device_properties2, Ts...> result;\n"
        "        func.call(dev, chain_setup(result));\n"
        "        return result;\n"
        "    }\n"
    );
    c.definitions.push_back(
        "template <typename T>\n"
        "    auto get_prop() const {\n"
        "        auto func = f[cmd::get_physical_device_properties2];\n"
        "        core::tuple<vk::physical_device_properties2, T> result;\n"
        "        func.call(dev, chain_setup(result));\n"
        "        return result[core::type<T>];\n"
        "    }\n"
    );
    c.after_class    = "\n"
                       "auto instance_t::physical_devices() const {\n"
                       "    return physical_devices_raw().map([this](std::span<const physical_device> devs) {\n"
                       "        std::vector<physical_device_t> res;\n"
                       "        for (auto dev : devs)\n"
                       "            res.emplace_back(*this, dev);\n"
                       "        return res;\n"
                       "    });\n"
                       "}\n";

    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/instance.hpp>\n"
        "#include <grx/vk/commands.cg.hpp>\n"
        "#include <grx/vk/structs.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_logical_device_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name = "physical_dev";
    c.instance.type = "physical_device_t";
    c.handle.name   = "dev";
    c.handle.type   = "vk::device";
    c.name          = "device_t";
    c.functions     = {
        {.name = "create_swapchain_khr", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "swapchain_t"},
        {.name = "create_image_view", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "image_view_t"},
        {.name = "create_shader_module", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "shader_module_t"},
        {.name = "create_pipeline_layout", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "pipeline_layout_t"},
        {.name = "create_graphics_pipelines", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "pipeline_t"},
        {.name = "create_render_pass", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "render_pass_t"},
        {.name = "create_framebuffer", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "framebuffer_t"},
        {.name = "create_command_pool", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "command_pool_t"},
        {.name = "allocate_command_buffers", .type = cg::vk::gen_type::full_plus_declare_interface},
        {.name = "create_semaphore", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "semaphore_t"},
        {.name = "create_fence", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "fence_t"},
        {.name = "get_device_queue", .type = cg::vk::gen_type::full_plus_declare_interface, .external_interface = "queue_t"},
        {.name = "destroy_device", .type = cg::vk::gen_type::destructor},
        {.name = "device_wait_idle"},
        {.name = "wait_for_fences", .type = cg::vk::gen_type::only_cache_this},
        {.name = "reset_fences", .type = cg::vk::gen_type::only_cache_this},
        {.name = "wait_semaphores", .type = cg::vk::gen_type::only_cache_this},
    };
    c.func_prefixes  = {"device_"};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    c.definitions.push_back(
        "void wait(fence fence, std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max()) {\n"
        "        f[cmd::wait_for_fences].call(dev, 1, &fence, true, u64(timeout.count()));\n"
        "    }\n"
    );
    c.definitions.push_back(
        "void wait(semaphore sem, core::opt<u64> timeline_signal_value = {}, std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max()) {\n"
        "        semaphore_wait_info info{};\n"
        "        info.semaphore_count = 1;\n"
        "        info.semaphores = &sem;\n"
        "        info.values = timeline_signal_value ? &*timeline_signal_value : nullptr;\n"
        "        f[cmd::wait_semaphores].call(dev, &info, u64(timeout.count()));\n"
        "    }\n"
    );
    c.definitions.push_back(
        "void reset(fence fence) {\n"
        "        f[cmd::reset_fences].call(dev, 1, &fence);\n"
        "    }\n"
    );
    c.definitions.push_back(
        "auto create_graphics_pipeline(vk::pipeline_cache pipeline_cache, const vk::graphics_pipeline_create_info& create_info, "
        "core::opt<vk::allocation_callbacks> allocator = core::null) const;\n"
    );
    c.definitions.push_back("auto create_typed_semaphore(vk::semaphore_type type, u64 intiial_value = 0, core::opt<vk::allocation_callbacks> allocator = core::null) const;");

    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/physical_device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_swapchain_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "sc";
    c.handle.type        = "vk::swapchain_khr";
    c.name               = "swapchain_t";
    c.functions          = {
        {.name = "destroy_swapchain_khr", .type = cg::vk::gen_type::destructor},
        {.name = "get_swapchain_images_khr"},
        {.name = "acquire_next_image_khr", .type = cg::vk::gen_type::only_cache_this},
    };
    c.func_prefixes  = {"get_swapchain_"};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    c.definitions.push_back(
        "#if defined(VK_KHR_swapchain)\n"
        "    auto acquire_next_image(vk::semaphore semaphore, vk::fence fence = core::null, std::chrono::nanoseconds timeout = "
        "std::chrono::nanoseconds::max()) const {\n"
        "        auto func = f[cmd::acquire_next_image_khr];\n"
        "        auto res = func.result_type(core::type<u32>).make();\n"
        "        res.rc = func.call(dev->handle(), sc, core::u64(timeout.count()), semaphore, fence, &res.value_unsafe);\n"
        "        return res;\n"
        "    }\n"
        "#endif /* defined(VK_KHR_swapchain) */\n"
        "\n"
    );

    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_image_view_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "iv";
    c.handle.type        = "vk::image_view";
    c.name               = "image_view_t";
    c.functions          = {
        {.name = "destroy_image_view", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_shader_module_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::shader_module";
    c.name               = "shader_module_t";
    c.functions          = {
        {.name = "destroy_shader_module", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_pipeline_layout_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::pipeline_layout";
    c.name               = "pipeline_layout_t";
    c.functions          = {
        {.name = "destroy_pipeline_layout", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_pipeline_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::pipeline";
    c.name               = "pipeline_t";
    c.functions          = {
        {.name = "destroy_pipeline", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;

    c.after_class += "\n"
                     "auto device_t::create_graphics_pipeline(vk::pipeline_cache pipeline_cache, const vk::graphics_pipeline_create_info& create_info, "
                     "core::opt<vk::allocation_callbacks> allocator) const {\n"
                     "    return core::mov(create_graphics_pipelines(pipeline_cache, std::span{&create_info, 1}, allocator)[0]);\n"
                     "}\n";

    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_render_pass_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::render_pass";
    c.name               = "render_pass_t";
    c.functions          = {
        {.name = "destroy_render_pass", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_framebuffer_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::framebuffer";
    c.name               = "framebuffer_t";
    c.functions          = {
        {.name = "destroy_framebuffer", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_command_pool_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::command_pool";
    c.name               = "command_pool_t";
    c.functions          = {
        {.name = "destroy_command_pool", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_command_buffer_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::command_buffer";
    c.name               = "command_buffer_t";
    c.functions          = {
        {.name = "reset_command_buffer"},
        {.name = "begin_command_buffer"},
        {.name = "end_command_buffer"},
        {.name = "cmd_begin_render_pass"},
        {.name = "cmd_bind_pipeline"},
        {.name = "cmd_set_viewport"},
        {.name = "cmd_set_scissor"},
        {.name = "cmd_draw"},
        {.name = "cmd_end_render_pass"},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext", "_command_buffer"};
    c.copyable       = true;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_semaphore_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::semaphore";
    c.name               = "semaphore_t";
    c.functions          = {
        {.name = "destroy_semaphore", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;

    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);

    out.write(
        "namespace vk {\n"
        "auto device_t::create_typed_semaphore(vk::semaphore_type type, u64 initial_value, core::opt<vk::allocation_callbacks> allocator) const {\n"
        "    vk::semaphore_create_info info{};\n"
        "    vk::semaphore_type_create_info type_info{};\n"
        "    type_info.semaphore_type = type;\n"
        "    type_info.initial_value = initial_value;\n"
        "    info.next = &type_info;\n"
        "    return create_semaphore(info, core::mov(allocator));\n"
        "}\n"
        "} // namespace vk\n"
    );
}

void generate_fence_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::fence";
    c.name               = "fence_t";
    c.functions          = {
        {.name = "destroy_fence", .type = cg::vk::gen_type::destructor},
    };
    c.func_prefixes  = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;
    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void generate_queue_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto                             cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::queue";
    c.name               = "queue_t";
    c.functions          = {
        {.name = "queue_submit"},
        {.name = "queue_present_khr"},
    };
    c.func_prefixes  = {"queue_"};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable       = false;

    c.definitions.push_back(
        "auto submit(const vk::submit_info& submits, vk::fence fence) const {\n"
        "        return submit(std::span{&submits, 1}, fence);\n"
        "   }\n"
    );

    out.write(
        "#pragma once\n"
        "\n"
        "#include <grx/vk/device.cg.hpp>\n\n"
    );
    c.generate(out, cmds, eg);
}

void tbc_main(vulkan_cmd<> args) {
    pugi::xml_document doc;
    doc.load_file(args.api->data());
    auto registry = doc.child("registry");

    auto openfile = [&](const std::string& name) {
        return io::out{
            io::file::open(*args.dir + "/" + name, sys::openflags::write_only | sys::openflags::create | sys::openflags::trunc, sys::file_perms::o644),
        };
    };

    cg::vk::generate_commands_header(registry, openfile("commands.cg.hpp"));
    cg::vk::generate_structs_header(
        registry,
        {
            "instance_create_info",
            "device_create_info",
            "swapchain_create_info_khr",
        },
        {
            "physical_device_features",
            "pipeline_vertex_input_state_create_info",
        },
        openfile("structs.cg.hpp")
    );
    cg::vk::generate_flags_header(registry, openfile("flags.cg.hpp"));
    cg::vk::generate_enums_header(registry, openfile("enums.cg.hpp"));
    cg::vk::generate_constants_header(registry, openfile("constants.cg.hpp"));
    cg::vk::generate_types_header(registry, openfile("types.cg.hpp"));
    cg::vk::generate_function_types_header(registry, openfile("function_types.cg.hpp"));

    cg::vk::external_generated eg;

    generate_physical_device_header(registry, eg, openfile("physical_device.cg.hpp"));
    generate_logical_device_header(registry, eg, openfile("device.cg.hpp"));
    generate_swapchain_header(registry, eg, openfile("swapchain.cg.hpp"));
    generate_image_view_header(registry, eg, openfile("image_view.cg.hpp"));
    generate_shader_module_header(registry, eg, openfile("shader_module.cg.hpp"));
    generate_pipeline_layout_header(registry, eg, openfile("pipeline_layout.cg.hpp"));
    generate_pipeline_header(registry, eg, openfile("pipeline.cg.hpp"));
    generate_render_pass_header(registry, eg, openfile("render_pass.cg.hpp"));
    generate_framebuffer_header(registry, eg, openfile("framebuffer.cg.hpp"));
    generate_command_pool_header(registry, eg, openfile("command_pool.cg.hpp"));
    generate_command_buffer_header(registry, eg, openfile("command_buffer.cg.hpp"));
    generate_semaphore_header(registry, eg, openfile("semaphore.cg.hpp"));
    generate_fence_header(registry, eg, openfile("fence.cg.hpp"));
    generate_queue_header(registry, eg, openfile("queue.cg.hpp"));
}

#include <util/tbc_main.hpp>
