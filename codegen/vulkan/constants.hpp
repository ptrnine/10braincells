#pragma once

#include <pugixml.hpp>

#include <codegen/helpers.hpp>

namespace cg::vk
{
void generate_constants_header(const pugi::xml_node& registry, auto&& out) {
    out.write("#pragma once\n"
              "#include <cstdint>\n"
              "\n"
              "namespace vk::constants {\n");
    for (auto constants : registry.children("enums")) {
        if (constants.attribute("name").value() != std::string_view("API Constants"))
            break;

        for (auto xml_enum : constants.children("enum")) {
            if (auto type = xml_enum.attribute("type")) {
                std::string name = to_lower_case(xml_enum.attribute("name").value());
                if (name != "vk_true" && name != "vk_false")
                    drop_prefix(name, "vk_");

                std::string value        = xml_enum.attribute("value").value();
                auto        comment_attr = xml_enum.attribute("comment");
                if (comment_attr)
                    out.write("/** ", std::string_view(comment_attr.value()), " */\n");
                out.write("static inline constexpr ", std::string(type.value()), " ", name, " = ", value, ";\n");
            }
            else if (auto xml_alias = xml_enum.attribute("alias")) {
                std::string name         = to_lower_case(xml_enum.attribute("name").value());
                std::string alias        = to_lower_case(xml_alias.value());
                drop_prefix(name, "vk_");
                drop_prefix(alias, "vk_");
                auto        comment_attr = xml_enum.attribute("comment");
                if (comment_attr)
                    out.write("/** ", std::string_view(comment_attr.value()), " */\n");
                out.write("static inline constexpr auto ", name, " = ", alias, ";\n");
            }
        }
    }
    out.write("} // namespace grx::vk::constants\n");
}
} // namespace c
