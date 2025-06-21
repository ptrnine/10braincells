#pragma once

#include <regex>
#include <string>
#include <util/log.hpp>
#include <vector>
#include <pugixml.hpp>

#include <core/ranges/trim.hpp>

#include <codegen/enum_class.hpp>
#include <codegen/helpers.hpp>
#include <codegen/vulkan/helpers.hpp>
#include <codegen/vulkan/enum_generator.hpp>

namespace cg::vk
{
struct typedef_gen {
    std::string generate() const {
        if (struct_typedef)
            return "using " + key + " = struct " + key + "_s*;";
        else
            return "using " + key + " = " + value + ";";
    }

    std::string key;
    std::string value;
    bool        struct_typedef = false;
};

auto parse_function_types(const pugi::xml_node& registry) {
    std::vector<typedef_gen> res;

    for (auto t : registry.child("types").children()) {
        std::string_view category = t.attribute("category").value();
        if (category == "funcpointer") {
            std::string str;
            for (auto child : t.children()) {
                std::string lines = child.text().as_string();
                std::string_view node_type = child.name();
                for (auto text : lines | core::views::split('\n')) {
                    if (node_type == "type") {
                        str += ' ';
                        auto type = transform_type(std::string(text));
                        if (type == "debug_utils_messenger_callback_data_ext" || type == "fault_data" ||
                            type == "device_memory_report_callback_data_ext")
                            type = "struct " + type;
                        str += type;
                        str += ' ';
                    }
                    else
                        str += (text | core::trim(' ')).as_string();
                }
            }

            static std::regex func_rxp{R"(typedef (.*?) \(VKAPI_PTR \*PFN_(.*?)\)\((.*?)\);)"};
            std::smatch matches;
            if (std::regex_match(str, matches, func_rxp)) {
                typedef_gen t;
                t.struct_typedef = false;
                t.key = to_snake_case(matches[2].str());
                drop_prefix(t.key, "vk_");

                if (t.key == "debug_report_callback_ext")
                    drop_postfix(t.key, "_ext");

                auto return_type = transform_type(matches[1].str());
                t.value = return_type + "(*)(";

                auto params = matches[3].str();

                for (auto param_r : params | core::views::split(',')) {
                    auto param = param_r.as_string();
                    if (param == "void")
                        continue;
                    auto pos = param.rfind(' ');
                    if (pos != param.npos) {
                        auto type = (param.substr(0, pos) | core::trim{' '}).as_string();
                        auto name = (param.substr(pos) | core::trim{' '}).as_string();
                        t.value += type;
                        t.value += ' ';
                        t.value += to_snake_case(name);
                        t.value += ", ";
                    }
                }
                if (t.value.ends_with(", "))
                    t.value.resize(t.value.size() - 2);
                t.value += ')';

                res.push_back(t);
            }
        }
    }

    return res;
}

auto parse_typedefs(const pugi::xml_node& registry, const std::set<std::string>& flag_names) {
    std::vector<typedef_gen> res;

    for (auto t : registry.child("types").children()) {
        std::string_view category = t.attribute("category").value();
        if (category == "basetype") {
            std::string type = t.child("type").text().as_string();
            if (!type.empty()) {
                std::string alias = to_snake_case(t.child("name").text().as_string()) + "_t";
                drop_prefix(alias, "vk_");
                res.push_back(typedef_gen{alias, type});
            }
        }
        else if (category == "handle") {
            auto type = t.child("name");

            if (auto alias_attr = t.attribute("alias"))
                res.push_back(typedef_gen{
                    transform_type(t.attribute("name").value()),
                    transform_type(alias_attr.value()),
                });
            else {
                res.push_back(typedef_gen{
                    transform_type(type.text().as_string()),
                    "",
                    true,
                });
            }
        }
        else if (category == "bitmask" && !t.attribute("alias")) {
            auto type = transform_type(t.child("type").text().as_string());
            auto name = transform_type(t.child("name").text().as_string());

            auto flag_name = name;
            if (flag_name.ends_with("flags"))
                flag_name.resize(flag_name.size() - 1);
            if (flag_name.ends_with("_flags2")) {
                flag_name.resize(flag_name.size() - (sizeof("_flags2") - 1));
                flag_name += "2_flag";
            }

            if (flag_names.find(flag_name) == flag_names.end()) {
                res.push_back(typedef_gen{name, type});
            }
        }
    }

    return res;
}

void generate_types_header(const pugi::xml_node& registry, auto&& out) {
    out.write("#pragma once\n"
              "\n"
              "#include <core/basic_types.hpp>\n"
              "\n"
              "namespace vk\n"
              "{\n"
              "using u8  = core::u8;\n"
              "using u16 = core::u16;\n"
              "using u32 = core::u32;\n"
              "using u64 = core::u64;\n"
              "using i8  = core::i8;\n"
              "using i16 = core::i16;\n"
              "using i32 = core::i32;\n"
              "using i64 = core::i64;\n"
              "\n"
              "using size_t  = core::size_t;\n"
              "using ssize_t = core::ssize_t;\n"
              "\n");

    auto flags = cg::vk::parse_flags(registry);
    std::set<std::string> flag_names;
    for (auto&& [_, f] : flags)
        flag_names.emplace(f.name);

    for (auto&& t : cg::vk::parse_typedefs(registry, flag_names))
        out.write(t.generate(), "\n");

    out.write("} // namespace vk\n");
}

void generate_function_types_header(const pugi::xml_node& registry, auto&& out) {
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/types.cg.hpp>\n"
              "#include <grx/vk/enums.cg.hpp>\n"
              "\n"
              "namespace vk\n"
              "{\n");

    for (auto&& t : cg::vk::parse_function_types(registry))
         out.write(t.generate(), "\n\n");

    out.write("} // namespace vk\n");
}
} // namespace cg::vk
