#pragma once

#include <pugixml.hpp>

#include <core/ranges/trim.hpp>

#include <codegen/vulkan/function_type.hpp>
#include <codegen/vulkan/helpers.hpp>
#include <util/log.hpp>

namespace cg::vk
{
inline function_arg parse_arg(const pugi::xml_node& param) {
    function_arg arg;

    auto append = [](auto&& s, auto&& postfix) {
        if (!s.empty() && !postfix.starts_with('*'))
            s += ' ';
        s += postfix;
    };

    if (auto len = param.attribute("len"))
        arg.length_arg = to_snake_case(len.value());

    for (auto child : param.children()) {
        std::string_view node = child.name();
        auto trimmed = std::string{std::string_view(child.text().as_string()) | core::trim{' ', '\n'}};
        if (node == "type") {
            append(arg.type, transform_type(trimmed, true));
        }
        else if (node == "name") {
            arg.name = to_snake_case(trimmed);
        }
        else if (trimmed.starts_with('[') || trimmed.ends_with(']')) {
            arg.name += trimmed;
        }
        else {
            append(arg.type, trimmed);
        }

        drop_prefixes(arg.name, "p_", "pp_");
        drop_prefixes(arg.length_arg, "p_", "pp_");
    }

    /* Fix u32 versions */
    if (arg.type == "u32 *" && arg.name.ends_with("_version"))
        arg.type = "version_raw*";

    if (auto opt = param.attribute("optional"))
        arg.optional = opt.value();

    return arg;
}

inline std::vector<function_type> parse_commands(const pugi::xml_node& registry) {
    std::vector<function_type> res;
    std::map<std::string, size_t> name_to_idx;

    size_t idx = 0;
    for (auto command_xml : registry.child("commands").children("command")) {
        if (command_xml.attribute("alias"))
            continue;

        if (auto api = command_xml.attribute("api"); api && api.value() != std::string("vulkan"))
            continue;

        function_type func;

        for (auto code : std::string_view(command_xml.attribute("successcodes").value()) | core::views::split(',')) {
            auto c = to_lower_case(std::string(code));
            drop_prefix(c, "vk_");
            func.success_codes.push_back(c);
        }

        for (auto code : std::string_view(command_xml.attribute("errorcodes").value()) | core::views::split(',')) {
            auto c = to_lower_case(std::string(code));
            drop_prefix(c, "vk_");
            //drop_postfixes(c, "_khr", "_ext", "_nv");
            func.error_codes.push_back(c);
        }

        auto proto_xml      = command_xml.child("proto");
        auto proto_name_xml = proto_xml.child("name");

        auto def  = parse_arg(proto_xml);
        func.name = def.name;
        drop_prefix(func.name, "vk_");
        func.return_type = def.type;
        func.native      = proto_name_xml.text().as_string();

        for (auto param : command_xml.children("param"))
            func.args.push_back(parse_arg(param));

        name_to_idx.emplace(func.native, idx++);
        res.push_back(core::mov(func));
    }

    /* Parse extension ifdefs */
    for (auto extension : registry.child("extensions").children("extension")) {
        auto ifdef = std::string("defined(") + extension.attribute("name").value() + ")";
        for (auto require : extension.children("require")) {
            for (auto command : require.children("command")) {
                std::string name = command.attribute("name").value();
                auto idx = name_to_idx.find(name);
                if (idx != name_to_idx.end())
                    res[idx->second].macro_condition = ifdef;
            }
        }
    }

    return res;
}

inline void generate_commands_header(const pugi::xml_node& registry, auto&& out) {
    auto commands = cg::vk::parse_commands(registry);

    out.write("#pragma once\n"
              "\n"
              "#include <dlfcn.h>\n"
              "\n"
              "#include <core/meta/type.hpp>\n"
              "#include <core/moveonly_trivial.hpp>\n"
              "#include <grx/vk/result.hpp>\n"
              "#include <grx/vk/structs.hpp>\n"
              "#include <grx/vk/version.hpp>\n"
              "\n"
              "namespace vk::cmd {\n");

    for (auto&& command : commands) {
        command.generate_traits(out);
        out.write("\n");
    }

    out.write("void load_from_lib(void* handle, auto&&... func_holders) {\n"
              "    ((func_holders.call = (typename core::remove_ref<decltype(func_holders)>::type)dlsym(handle, "
              "func_holders.name)), ...);\n"
              "}\n"
              "\n"
              "} /* namespace vk::cmd */\n");
}
} // namespace cg::vk
