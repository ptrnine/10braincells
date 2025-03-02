#pragma once

#include <pugixml.hpp>

#include <codegen/struct.hpp>
#include <codegen/helpers.hpp>
#include <codegen/vulkan/types.hpp>
#include <util/log.hpp>

namespace cg::vk
{
inline auto parse_structs(const pugi::xml_node& registry) {
    std::multimap<size_t, struct_gen> structs;
    std::map<std::string, size_t>     name_to_idx;

    size_t idx = 0;
    for (auto t : registry.child("types").children()) {
        std::string_view category = t.attribute("category").value();
        if (category != "struct" && category != "union")
            continue;

        struct_gen struct_res;
        struct_res.is_union = category == "union";
        struct_res.name     = transform_type(t.attribute("name").value());
        if (auto alias_attr = t.attribute("alias")) {
            struct_res.alias = transform_type(alias_attr.value());
        }
        else {
            std::set<std::string> same_type_and_name;
            for (auto member : t.children("member")) {
                /* Skip api="vulkan" */
                if (auto api = member.attribute("api"))
                    if (std::string_view(api.value()) != "vulkan")
                        continue;

                auto field = parse_field(member);

                /* Fix when member name same as type */
                auto non_ptr_type = field.type;
                drop_postfix(non_ptr_type, "*");
                if (same_type_and_name.contains(non_ptr_type))
                    field.type = "vk::" + field.type;
                if (non_ptr_type == field.name)
                    same_type_and_name.emplace(non_ptr_type);

                if (field.name.ends_with("_d")) {
                    field.name.resize(field.name.size() - 2);
                    field.name += 'd';
                }

                /* Replace u32 version */
                if (field.type == "u32" && field.name.ends_with("_version"))
                    field.type = "version_raw";

                struct_res.fields.push_back(core::mov(field));
            }
        }

        if (struct_res.name == "instance_create_info") {
            struct_res.methods.push_back("static constexpr auto pass(auto&&... args);");
        }

        auto name = struct_res.name;
        structs.emplace(idx, core::mov(struct_res));
        name_to_idx.emplace(name, idx++);
    }

    /* Parse extension ifdefs */
    for (auto extension : registry.child("extensions").children("extension")) {
        auto ifdef = std::string("defined(") + extension.attribute("name").value() + ")";
        for (auto require : extension.children("require")) {
            for (auto type : require.children()) {
                auto name = transform_type(type.attribute("name").value());
                auto idx = name_to_idx.find(name);
                if (idx != name_to_idx.end()) {
                    auto s = structs.find(idx->second);
                    if (s != structs.end())
                        s->second.ifdef = ifdef;
                }
            }
        }
    }

    /* Sort by usage
     * TODO: refactor
     */
    for (size_t i = 0; i < 2; ++i) {
        for (auto it = structs.begin(); it != structs.end(); ++it) {
            for (auto&& field : it->second.fields) {
                auto type = field.type;
                if (type.ends_with('*'))
                    type.pop_back();
                if (type.starts_with("const "))
                    type = type.substr(6);

                auto name_idx = name_to_idx.find(type);
                if (name_idx == name_to_idx.end())
                    continue;

                if (name_idx->second > it->first) {
                    auto prev_i      = name_idx->second;
                    name_idx->second = it->first;
                    if (name_idx->second != 0)
                        --(name_idx->second);

                    auto prev_pos = structs.find(prev_i);
                    if (prev_pos == structs.end())
                        continue;

                    while (prev_pos->second.name != type)
                        ++prev_pos;

                    structs.emplace(name_idx->second, core::mov(prev_pos->second));
                    structs.erase(prev_pos);
                }
            }
        }
    }

    std::vector<struct_gen> result;
    for (auto&& s : structs)
        result.push_back(core::mov(s.second));

    return result;
}

inline void generate_structs_header(const pugi::xml_node& registry, auto&& out) {
    out.write("#pragma once\n"
              "\n"
              "#include <grx/vk/constants.hpp>\n"
              "#include <grx/vk/enums.hpp>\n"
              "#include <grx/vk/flags.hpp>\n"
              "#include <grx/vk/types.hpp>\n"
              "#include <grx/vk/function_types.hpp>\n"
              "#include <grx/vk/version.hpp>\n"
              "\n"
              "namespace vk {\n");
    for (auto&& s : cg::vk::parse_structs(registry)) {
        s.generate(out);
        out.write("\n");
    }
    out.write("} /* namespace vk */\n");
}
} // namespace cg::vk
