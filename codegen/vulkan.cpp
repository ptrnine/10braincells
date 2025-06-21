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
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name = "inst";
    c.instance.type = "instance_t";
    c.handle.name = "dev";
    c.handle.type = "vk::physical_device";
    c.name = "physical_device_t";
    c.functions     = {
        {"get_physical_device_properties"},
        {"get_physical_device_properties2"},
        {"get_physical_device_features"},
        {"get_physical_device_features2"},
        {"get_physical_device_memory_properties"},
        {"get_physical_device_memory_properties2"},
        {"get_physical_device_queue_family_properties"},
        {"get_physical_device_queue_family_properties2"},
        {"get_physical_device_format_properties"},
        {"get_physical_device_format_properties2"},
        {"get_physical_device_image_format_properties"},
        {"get_physical_device_image_format_properties2"},
        {"get_physical_device_sparse_image_format_properties"},
        {"get_physical_device_sparse_image_format_properties2"},
        {"get_physical_device_external_buffer_properties"},
        {"get_physical_device_external_semaphore_properties"},
        {"get_physical_device_external_fence_properties"},
        {"get_physical_device_surface_support_khr"},
        {"get_physical_device_surface_capabilities_khr"},
        {"get_physical_device_surface_formats_khr"},
        {"get_physical_device_surface_present_modes_khr"},
        {"create_device", cg::vk::gen_type::full_plus_declare_interface, "device_t"},
        {"enumerate_device_layer_properties"},
        {"enumerate_device_extension_properties"},
        {"get_physical_device_display_properties_khr"},
        {"get_physical_device_display_plane_properties_khr"},
        {"get_display_plane_supported_displays_khr"},
        {"get_physical_device_display_properties2_khr"},
        {"get_physical_device_multisample_properties_ext"},
        {"get_drm_display_ext"},
    };
    c.func_prefixes = {
        "get_physical_device_",
        "enumerate_device_",
        "get_",
    };
    c.func_postfixes = {"_khr", "_ext"};
    c.after_class    = "\n"
                       "auto instance_t::physical_devices() const {\n"
                       "    return physical_devices_raw().map([this](std::span<const physical_device> devs) {\n"
                       "        std::vector<physical_device_t> res;\n"
                       "        for (auto dev : devs)\n"
                       "            res.emplace_back(*this, dev);\n"
                       "        return res;\n"
                       "    });\n"
                       "}\n";

    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/instance.hpp>\n"
              "#include <grx/vk/commands.cg.hpp>\n"
              "#include <grx/vk/structs.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_logical_device_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name = "physical_dev";
    c.instance.type = "physical_device_t";
    c.handle.name = "dev";
    c.handle.type = "vk::device";
    c.name          = "device_t";
    c.functions     = {
        {"create_swapchain_khr", cg::vk::gen_type::full_plus_declare_interface, "swapchain_t"},
        {"create_image_view", cg::vk::gen_type::full_plus_declare_interface, "image_view_t"},
        {"create_shader_module", cg::vk::gen_type::full_plus_declare_interface, "shader_module_t"},
        {"create_pipeline_layout", cg::vk::gen_type::full_plus_declare_interface, "pipeline_layout_t"},
        {"create_graphics_pipelines", cg::vk::gen_type::full_plus_declare_interface, "pipeline_t"},
        {"create_render_pass", cg::vk::gen_type::full_plus_declare_interface, "render_pass_t"},
        {"create_framebuffer", cg::vk::gen_type::full_plus_declare_interface, "framebuffer_t"},
        {"create_command_pool", cg::vk::gen_type::full_plus_declare_interface, "command_pool_t"},
        {"allocate_command_buffers", cg::vk::gen_type::full_plus_declare_interface},
        {"create_semaphore", cg::vk::gen_type::full_plus_declare_interface, "semaphore_t"},
        {"create_fence", cg::vk::gen_type::full_plus_declare_interface, "fence_t"},
        {"get_device_queue", cg::vk::gen_type::full_plus_declare_interface, "queue_t"},
        {"destroy_device", cg::vk::gen_type::destructor},
        {"device_wait_idle"},
        {"wait_for_fences", cg::vk::gen_type::only_cache_this},
        {"reset_fences", cg::vk::gen_type::only_cache_this},
    };
    c.func_prefixes     = {"device_"};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable          = false;
    c.definitions.push_back(
        "void wait(fence fence, std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max()) {\n"
        "        f[cmd::wait_for_fences].call(dev, 1, &fence, true, timeout.count());\n"
        "    }\n");
    c.definitions.push_back(
        "void reset(fence fence) {\n"
        "        f[cmd::reset_fences].call(dev, 1, &fence);\n"
        "    }\n");

    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/physical_device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_swapchain_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name = "dev";
    c.instance.type = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name = "sc";
    c.handle.type = "vk::swapchain_khr";
    c.name          = "swapchain_t";
    c.functions     = {
        {"destroy_swapchain_khr", cg::vk::gen_type::destructor},
        {"get_swapchain_images_khr"},
        {"acquire_next_image_khr", cg::vk::gen_type::only_cache_this},
    };
    c.func_prefixes     = {"get_swapchain_"};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable          = false;
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
        "\n");

    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_image_view_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name = "dev";
    c.instance.type = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name = "iv";
    c.handle.type = "vk::image_view";
    c.name          = "image_view_t";
    c.functions     = {
        {"destroy_image_view", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes = {"_khr", "_ext"};
    c.copyable = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_shader_module_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::shader_module";
    c.name               = "shader_module_t";
    c.functions          = {
        {"destroy_shader_module", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_pipeline_layout_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::pipeline_layout";
    c.name               = "pipeline_layout_t";
    c.functions          = {
        {"destroy_pipeline_layout", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_pipeline_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::pipeline";
    c.name               = "pipeline_t";
    c.functions          = {
        {"destroy_pipeline", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_render_pass_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::render_pass";
    c.name               = "render_pass_t";
    c.functions          = {
        {"destroy_render_pass", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_framebuffer_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::framebuffer";
    c.name               = "framebuffer_t";
    c.functions          = {
        {"destroy_framebuffer", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_command_pool_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::command_pool";
    c.name               = "command_pool_t";
    c.functions          = {
        {"destroy_command_pool", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_command_buffer_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::command_buffer";
    c.name               = "command_buffer_t";
    c.functions          = {
        {"reset_command_buffer"},
        {"begin_command_buffer"},
        {"end_command_buffer"},
        {"cmd_begin_render_pass"},
        {"cmd_bind_pipeline"},
        {"cmd_set_viewport"},
        {"cmd_set_scissor"},
        {"cmd_draw"},
        {"cmd_end_render_pass"},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext", "_command_buffer"};
    c.copyable          = true;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_semaphore_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::semaphore";
    c.name               = "semaphore_t";
    c.functions          = {
        {"destroy_semaphore", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_fence_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::fence";
    c.name               = "fence_t";
    c.functions          = {
        {"destroy_fence", cg::vk::gen_type::destructor},
    };
    c.func_prefixes     = {};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}

void generate_queue_header(const pugi::xml_node& registry, cg::vk::external_generated& eg, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name      = "dev";
    c.instance.type      = "device_t";
    c.instance.real_type = "vk::device";
    c.handle.name        = "_handle";
    c.handle.type        = "vk::queue";
    c.name               = "queue_t";
    c.functions          = {
        {"queue_submit"},
        {"queue_present_khr"},
    };
    c.func_prefixes     = {"queue_"};
    c.func_postfixes    = {"_khr", "_ext"};
    c.copyable          = false;
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/device.cg.hpp>\n\n");
    c.generate(out, cmds, eg);
}



void tbc_main(vulkan_cmd<> args) {
    pugi::xml_document doc;
    doc.load_file(args.api->data());
    auto registry = doc.child("registry");

    auto openfile = [&](const std::string& name) {
        return io::out{
            io::file::open(*args.dir + "/" + name,
                           sys::openflags::write_only | sys::openflags::create | sys::openflags::trunc,
                           sys::file_perms::o644),
        };
    };

    cg::vk::generate_commands_header(registry, openfile("commands.cg.hpp"));
    cg::vk::generate_structs_header(registry,
                                    {
                                        "instance_create_info",
                                        "device_create_info",
                                        "swapchain_create_info_khr",
                                    },
                                    {"physical_device_features"},
                                    openfile("structs.cg.hpp"));
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
