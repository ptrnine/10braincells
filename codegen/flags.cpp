#include <core/io/std.hpp>
#include <core/ranges/fold.hpp>
#include <core/ranges/split.hpp>
#include <core/ranges/subst.hpp>

#include <coroutine>

using namespace core;
using namespace core::views;

auto write_fold(auto&& out, auto&& args) {
    for (auto&& s : args)
        out.write(s);
}

int main(int argc, const char** argv) {
    if (argc < 4) {
        io::std_err.write("Invalid arguments count\n"sv);
        return 1;
    }
    subst replacer{
        subst_entry{"name", argv[1]},
        subst_entry{"name_combined", argv[2]},
        subst_entry{"type", argv[3]},
    };
    std::span flag_defs{argv + 4, argv + argc};

    auto& o = io::std_out;
    o.write("/*\n"
            " * [[[codegen start]]]\n"
            " * Generated with:\n"
            " *"sv);
    for (auto arg : std::span{argv, argv + argc})
        o.write(" "sv, std::string_view(arg));
    write_fold(o, "\n"
                  " */\n"
                  "\n"
                  "enum class ${name} : ${type} {\n" |
                      replacer);

    for (auto flag_def : flag_defs) {
        o.write("    "sv);
        write_fold(o, std::string_view(flag_def) | split{'='} | fold{" = "sv});
        o.write(",\n");
    }

    write_fold(o,
               "};\n"
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

    for (auto flag_def : flag_defs) {
        std::string_view flag = *(std::string_view(flag_def) | split{'='}).begin();
        write_fold(o,
                   "            core::tuple{${name}::${flag}, std::string_view(\"${flag}\")},\n" |
                       subst{subst_entry{"name", argv[1]}, subst_entry{"flag"sv, flag}});
    }
    write_fold(o,
               "        };\n"
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
               "}\n"
               "\n"
               "/* [[[codegen end]]] */\n" |
                   replacer);
}
