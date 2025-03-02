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

void generate_physical_device_header(const pugi::xml_node& registry, auto&& out) {
    auto cmds = cg::vk::parse_commands(registry);
    cg::vk::class_instance_dependent c;
    c.instance.name = "inst";
    c.instance.type = "instance_t";
    c.handle.name = "dev";
    c.handle.type = "vk::physical_device";
    c.name = "physical_device_t";
    c.functions     = {
        "get_physical_device_properties",
        "get_physical_device_properties2",
        "get_physical_device_features",
        "get_physical_device_features2",
        "get_physical_device_memory_properties",
        "get_physical_device_memory_properties2",
        "get_physical_device_queue_family_properties",
        "get_physical_device_queue_family_properties2",
        "get_physical_device_format_properties",
        "get_physical_device_format_properties2",
        "get_physical_device_image_format_properties",
        "get_physical_device_image_format_properties2",
        "get_physical_device_sparse_image_format_properties",
        "get_physical_device_sparse_image_format_properties2",
        "get_physical_device_external_buffer_properties",
        "get_physical_device_external_semaphore_properties",
        "get_physical_device_external_fence_properties",
        "create_device",
        "enumerate_device_layer_properties",
        "enumerate_device_extension_properties",
        "get_physical_device_display_properties_khr",
        "get_physical_device_display_plane_properties_khr",
        "get_display_plane_supported_displays_khr",
        "get_physical_device_display_properties2_khr",
        "get_physical_device_multisample_properties_ext",
        "get_drm_display_ext",
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
              "#include <grx/vk/commands.hpp>\n"
              "#include <grx/vk/instance.hpp>\n"
              "#include <grx/vk/structs.hpp>\n");
    c.generate(out, cmds);
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

    cg::vk::generate_commands_header(registry, openfile("commands.hpp"));
    cg::vk::generate_structs_header(registry, openfile("structs.hpp"));
    cg::vk::generate_flags_header(registry, openfile("flags.hpp"));
    cg::vk::generate_enums_header(registry, openfile("enums.hpp"));
    cg::vk::generate_constants_header(registry, openfile("constants.hpp"));
    cg::vk::generate_types_header(registry, openfile("types.hpp"));
    cg::vk::generate_function_types_header(registry, openfile("function_types.hpp"));

    generate_physical_device_header(registry, openfile("physical_device.hpp"));
}

#include <util/tbc_main.hpp>
