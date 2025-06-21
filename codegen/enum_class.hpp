#pragma once

#include <string>
#include <list>

#include <core/generator.hpp>
#include <core/ranges/fold.hpp>
#include <core/ranges/subst.hpp>
#include <core/string_builder.hpp>
#include <core/utility/move.hpp>
#include <util/log.hpp>

namespace cg {
using core::views::subst;
using core::views::subst_entry;

using namespace std::string_view_literals;

struct enum_class_value {
    std::string key;
    std::string value;
    std::string comment;
    std::string ifdef;
    bool        is_alias = false;
};

struct enum_class {
    void generate(auto&& out) {
        subst replacer{
            subst_entry{"name", name},
            subst_entry{"type", underlying_type},
        };

        if (!comment.empty()) {
            out.write("/**\n"
                      " * ",
                      comment,
                      "\n",
                      " */\n");
        }
        out.write("enum class ${name} : ${type} {\n" | replacer);
        for (auto&& v : values) {
            if (!v.ifdef.empty())
                out.write("#if ", v.ifdef, "\n");
            auto comment = v.comment.empty() ? "" : (" /**< " + v.comment + " */");
            out.write("    ${key} = ${value},${comment}\n" | subst{
                                                                 subst_entry{"key", v.key},
                                                                 subst_entry{"value", v.value},
                                                                 subst_entry{"comment", comment},
                                                             });
            if (!v.ifdef.empty())
                out.write("#endif /* ", v.ifdef, " */\n");
        }
        out.write("};\n");
    }

    void generate_as_enum(auto&& out) {
        generate(out);

        auto  values_count = std::to_string(values.size());
        subst replacer{
            subst_entry{"name", name},
            subst_entry{"type", underlying_type},
            subst_entry{"values_count", values_count},
        };
        out.write("\n"
                  "inline constexpr std::string_view to_string(${name} value) {\n"
                  "    auto map = [] {\n"
                  "        core::static_int_map<${type}, std::string_view, ${values_count}> m;\n" |
                  replacer);
        for (auto&& v : values) {
            if (v.is_alias)
                continue;
            if (!v.ifdef.empty())
                out.write("#if ", v.ifdef, "\n");
            out.write("        m.emplace(${type}(${name}::${value}), \"${value}\");\n" |
                      subst{
                          subst_entry{"name", name},
                          subst_entry{"value", v.key},
                          subst_entry{"type", underlying_type},
                      });
            if (!v.ifdef.empty())
                out.write("#endif /* ", v.ifdef, " */\n");
        }
        out.write("        return m;\n"
                  "    }();\n"
                  "    return map.at(${type}(value));\n"
                  "}\n" |
                  replacer);
    }

    void generate_as_flags(auto&& out) {
        generate(out);

        auto  combined = combined_flag_name.empty() ? name + "s" : combined_flag_name;
        subst replacer{
            subst_entry{"name", name},
            subst_entry{"type", underlying_type},
            subst_entry{"combined", combined},
        };

        out.write("\n"
                  "template <>\n"
                  "struct details::_using_enum<${name}> {\n"
                  "    using enum ${name};\n"
                  "};\n"
                  "\n"
                  "using ${name}s = core::flags<${name}, details::_using_enum<${name}>>;\n"
                  "\n"
                  "inline constexpr ${combined} operator|(${name} lhs, ${name}s rhs) {\n"
                  "    return ${name}{${type}(${type}(lhs) | rhs.value)};\n"
                  "}\n"
                  "\n"
                  "inline constexpr ${combined} operator&(${name} lhs, ${combined} rhs) {\n"
                  "    return ${name}{${type}(${type}(lhs) & rhs.value)};\n"
                  "}\n" |
                  replacer);

        out.write("\n"
                  "inline constexpr std::string to_string(${name}s value) {\n"
                  "    constexpr core::array_maker flags = {\n"
                  "        core::type<core::tuple<${name}, std::string_view>>,\n" |
                  replacer);
        for (auto&& v : values) {
            if (v.is_alias)
                continue;
            if (!v.ifdef.empty())
                out.write("#if ", v.ifdef, "\n");
            out.write("        core::tuple{${name}::${flag}, std::string_view(\"${flag}\")},\n" |
                      subst{subst_entry{"name", name}, subst_entry{"flag"sv, v.key}});
            if (!v.ifdef.empty())
                out.write("#endif /* ", v.ifdef, " */\n");
        }
        out.write("    };\n"
                  "    std::string res;\n"
                  "    for (auto&& [f, s] : flags) {\n"
                  "        if (value.value & ${type}(f)) {\n"
                  "            res.append(s);\n"
                  "            res.append(\" | \");\n"
                  "        }\n"
                  "    }\n"
                  "    if (res.size() > 3)\n"
                  "        res.resize(res.size() - 3);\n"
                  "    return res;\n"
                  "}\n" |
                  replacer);
    }

    void append(enum_class_value value) {
        for (auto&& v : values)
            if (v.key == value.key)
                return;

        values.push_back(core::mov(value));
    }

    std::string                 comment;
    std::string                 name;
    std::string                 underlying_type = "int";
    std::list<enum_class_value> values;
    std::string                 combined_flag_name;
};
}
