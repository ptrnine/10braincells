#include <core/io/std.hpp>
#include <core/io/mmap.hpp>

#include <codegen/cg_generate.hpp>
#include <core/ranges/fold.hpp>
#include <core/ranges/split.hpp>
#include <core/ranges/subst.hpp>
#include <core/ranges/to.hpp>
#include <core/ranges/trim.hpp>
#include <util/arg_parse.hpp>

using namespace core;
using namespace core::views;
using namespace core::ct_str_literals;

tbc_cmd(main) {
    tbc_arg(name, std::string, "name of enum class"_ctstr);
    tbc_arg(name_combined, std::string, "name of struct that holds combination of flags"_ctstr);
    tbc_arg(type, std::string, "underlying type"_ctstr);
    tbc_arg(output, opt<std::string>, "output file"_ctstr);
    tbc_arg(includes, opt<std::string_view>, "additional includes"_ctstr);
    tbc_arg(namespace_name, opt<std::string_view>, "holding namespace"_ctstr);
    std::vector<std::string_view> flag_defs;

    util::cmd_description description{
        "generates flag helper types\n"
        "\n"
        "Usage:\n"
        "    cg_flags [OPTIONS]... [FLAGS_DEFINITIONS]\n"
        "\n"
        "Example:\n"
        "    cg_flags name=someflag name_combined=someflags type=int includes='<fcntl.h>,<stdio.h>' namespace_name=test output='some/path' 'one=0x1 two=0x2 three=0x4'",
    };
    util::cmdline cmdline;
};

void gen(auto&& out, const main_cmd<>& args) {
    out.write("/* Command: "sv, *args.cmdline | views::fold{" "}, " */\n"sv);

    out.write("#pragma once\n"sv);

    if (auto&& includes = args.includes.get()) {
        out.write("\n"sv);
        for (auto&& include : *includes | split{','})
            out.write("#include "sv, include.as_string(), "\n"sv);
    }

    out.write("\n#include <string>\n"
              "\n"
              "#include <core/array.hpp>\n"
              "#include <core/tuple.hpp>\n"sv);

    if (auto&& name = args.namespace_name.get())
        out.write("\nnamespace "sv, *name, " {"sv);

    subst replacer{
        subst_entry{"name", args.name.get()},
        subst_entry{"name_combined", args.name_combined.get()},
        subst_entry{"type", args.type.get()},
    };

    out.write("\n"
              "enum class ${name} : ${type} {\n" |
              replacer);

    for (auto&& flag_def : args.flag_defs) {
        out.write("    "sv, std::string_view(flag_def) | split{'='} | fold{" = "sv}, ",\n");
    }

    out.write("};\n"
              "\n"
              "struct ${name_combined} {\n"
              "    using enum ${name};\n"
              "\n"
              "    constexpr ${name_combined}() = default;\n"
              "    constexpr ${name_combined}(${name} ivalue): value(${type}(ivalue)) {}\n"
              "\n"
              "    constexpr ${name_combined} operator|(${name_combined} flag) const {\n"
              "        return ${name}(${type}(value | flag.value));\n"
              "    }\n"
              "\n"
              "    constexpr ${name_combined} operator&(${name_combined} flag) const {\n"
              "        return ${name}(${type}(value & flag.value));\n"
              "    }\n"
              "\n"
              "    constexpr ${name_combined}& operator|=(${name_combined} flag) {\n"
              "        value = ${type}(value | flag.value);\n"
              "        return *this;\n"
              "    }\n"
              "\n"
              "    constexpr bool test(${name_combined} flags) const {\n"
              "        return value & flags.value;\n"
              "    }\n"
              "\n"
              "    explicit constexpr operator bool() const {\n"
              "        return value;\n"
              "    }\n"
              "\n"
              "    constexpr void unset(${name_combined} flags) {\n"
              "        value &= ${type}(~flags.value);\n"
              "    }\n"
              "\n"
              "    constexpr std::string to_string() const {\n"
              "        constexpr core::array flags = {\n" |
              replacer);

    for (auto&& flag_def : args.flag_defs) {
        std::string_view flag = *(flag_def | split{'='}).begin();
        out.write(
            "            core::tuple{${name}::${flag}, std::string_view(\"${flag}\")},\n" |
            subst{subst_entry{"name", args.name.get()}, subst_entry{"flag"sv, flag | trim{' '} | to<std::string_view>{}}}
        );
    }
    out.write("        };\n"
              "        std::string res;\n"
              "        for (auto&& [f, s] : flags) {\n"
              "            if (${type}(value) & ${type}(f)) {\n"
              "                res.append(s);\n"
              "                res.append(\" | \");\n"
              "            }\n"
              "        }\n"
              "        if (res.size() > 2)\n"
              "            res.resize(res.size() - 2);\n"
              "        return res;\n"
              "    }\n"
              "\n"
              "    ${type} value;\n"
              "};\n"
              "\n"
              "inline constexpr ${name_combined} operator|(${name} lhs, ${name} rhs) {\n"
              "    return ${name}(${type}(lhs) | ${type}(rhs));\n"
              "}\n" |
              replacer);

    if (auto&& name = args.namespace_name.get())
        out.write("} // namespace "sv, *name, "\n"sv);
}

void tbc_main(const main_cmd<>& args) {
    cg_generate("cg_flags", args, args.output.get(), [](auto&&... args) {
        gen(args...);
    });
}

#include <util/tbc_main.hpp>
