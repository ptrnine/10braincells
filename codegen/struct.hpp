#pragma once

#include <set>
#include <vector>
#include <string>

#include <core/ranges/subst.hpp>
#include <core/ranges/split.hpp>

namespace cg {
using core::views::subst, core::views::subst_entry;
using core::views::split;

struct field_gen {
    bool operator<(const field_gen& f) const {
        return name < f.name;
    }

    std::string generate() const {
        std::string res = type + " " + name;
        if (is_array) {
            res += '[';
            res += size;
            res += ']';
        }

        if (!bitfield.empty()) {
            res += " : ";
            res += bitfield;
        }

        if (!value.empty()) {
            res += " = ";
            res += value;
        }
        else if (optional) {
            if (type.ends_with('*'))
                res += " = nullptr";
            else
                res += " = {}";
        }

        res += ';';
        return res;
    }

    std::string type;
    std::string name;
    std::string size;
    std::string comment;
    std::string bitfield;
    std::string value;
    bool        optional    = false;
    bool        is_array    = false;
};

struct struct_gen {
    void generate(auto&& out) const {
        if (!ifdef.empty())
            out.write("#if ", ifdef, "\n");

        subst replacer{
            subst_entry{"classdef", is_union ? "union" : "struct"},
            subst_entry{"type", name},
        };

        if (!alias.empty()) {
            out.write("using ", name, " = ", alias, ";\n");
            goto end;
        }

        out.write("${classdef} ${type} {\n" | replacer);
        for (auto&& method : methods) {
            for (auto&& line : method | split('\n'))
                out.write("    ", line, "\n");
            out.write("\n");
        }

        for (auto&& field : fields) {
            if (!field.comment.empty()) {
                if (field.comment.size() < 108)
                    out.write("    /** ", field.comment, " */\n");
                else
                    out.write("    /**\n     * ", field.comment, "\n     */\n");
            }
            out.write("    ", field.generate(), "\n");
        }
        out.write("};\n");

    end:
        if (!ifdef.empty())
            out.write("#endif /* ", ifdef, " */\n");
    }

    std::string              name;
    std::string              alias;
    std::vector<std::string> methods;
    std::vector<field_gen>   fields;
    std::string              ifdef;
    bool                     is_union = false;
};

} // namespace cg
