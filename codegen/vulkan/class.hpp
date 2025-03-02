#pragma once

#include <ranges>
#include <string>
#include <set>

#include <core/generator.hpp>
#include <core/ranges/subst.hpp>
#include <core/ranges/fold.hpp>
#include <core/string_builder.hpp>

#include <codegen/vulkan/function_type.hpp>
#include <util/log.hpp>

namespace cg::vk {
struct class_param {
    std::string type;
    std::string name;
};

struct class_instance_dependent {
    void generate(auto&& out, std::span<const function_type> funcs) {
        using core::views::subst_entry, core::views::subst, core::views::fold;
        subst cfg = {
            subst_entry{"name", name},
            subst_entry{"iname", instance.name},
            subst_entry{"itype", instance.type},
            subst_entry{"hname", handle.name},
            subst_entry{"htype", handle.type},
        };

        //auto func_names_generator = [&] -> core::generator<std::string> {
        //    for (auto&& f : funcs) {
        //        if (!functions.contains(f.name))
        //            continue;

        //        if (f.macro_condition.empty()) {}
        //        else {
        //            co_yield "                cmd::" + f.name + "_t";
        //        }
        //    }
        //}();

        out.write(
            "namespace vk {\n"
            "using core::type;\n"
            "class ${name} {\n"
            "public:\n"
            "    ${name}(const ${itype}& i${iname}, ${htype} i${hname}): ${iname}(&i${iname}), ${hname}(i${hname}) {\n"
            "        f.pass_to([&](auto&... functions) {\n"
            "            ${iname}->load_functions_cached(functions...);\n"
            "        });\n"
            "     }\n"
            "\n"
            "    ${htype} handle() const {\n"
            "        return ${hname};\n"
            "    }\n" |
            cfg);

        std::string func_types;
        std::string opened_macro;

        /* Gen functions */
        for (auto&& f : funcs) {
            if (!functions.contains(f.name))
                continue;

            out.write("\n");

            /* Handle ifdefs */
            if (f.macro_condition != opened_macro) {
                if (!opened_macro.empty()) {
                    auto close_macro = "#endif /* " + opened_macro + " */\n";
                    func_types += close_macro;
                    out.write(close_macro, "\n");
                }
                if (!f.macro_condition.empty()) {
                    auto open_macro = "#if " + f.macro_condition + "\n";
                    func_types += open_macro;
                    out.write(open_macro);
                }

                opened_macro = f.macro_condition;
            }

            func_types += "                cmd::" + f.name + "_t,\n";

            /* Gen member function name */
            auto funcname = f.name;
            for (auto& pref : func_prefixes)
                if (drop_prefix(funcname, pref))
                    break;
            for (auto& post : func_postfixes)
                if (drop_postfixes(funcname, post))
                    break;

            auto args = cppify_args(f.args);

            /* Take handle */
            for (auto& arg : args.args) {
                if (arg.type == handle.type) {
                    arg.name = handle.name;
                    break;
                }
            }

            out.write("    auto ${name}(" | subst{subst_entry{"name", funcname}},
                      args.arg_defs({handle.name}) | fold(", "),
                      ") const {\n"
                      "        auto func = f[cmd::${fname}];\n" |
                          subst{subst_entry{"fname", f.name}});

            /* Wrap optionals */
            for (auto& arg : args.args) {
                if (arg.type.starts_with("core::opt<")) {
                    out.write("        auto tmp${argname} = ${argname} ? ${access} : nullptr;\n" |
                              subst{subst_entry{"argname", arg.name},
                                    subst_entry{"access",
                                                arg.type == "core::opt<std::string>" ? "${argname}->data()"
                                                                                     : "&(*${argname})"}});
                    arg.name = "tmp" + arg.name;
                }
            }

            /* Define result */
            if (f.can_fail())
                out.write("        auto res = func.result_type(core::type<${type}>).make();\n" |
                          subst{subst_entry{"type", args.result.type}});
            else
                out.write("        ${type} res;\n" | subst{subst_entry{"type", args.result.type}});

            /* Calculate len */
            if (!args.result.bind_size.empty()) {
                auto count_type = f.get_arg(args.result.bind_size).type;
                drop_postfix(count_type, "*");
                out.write("\n"
                          "        ${type} count;\n" |
                          subst{subst_entry{"type", count_type}});
                out.write("        ",
                          std::string_view(f.can_fail() ? "res.rc = " : ""),
                          "func.call(",
                          pass_cpp_args_to_c(f, args, true) | fold(", "),
                          ");\n");
                if (f.can_fail())
                    out.write("        if (!res.ok())\n"
                              "            return res;\n");
                out.write("\n"
                          "        res",
                          std::string_view(f.can_fail() ? ".value_unsafe" : ""),
                          ".resize(count);\n");
            }

            out.write("        ",
                      std::string_view(f.can_fail() ? "res.rc = " : ""),
                      "func.call(",
                      pass_cpp_args_to_c(f, args, false) | fold(", "),
                      ");\n"
                      "        return res;\n"
                      "    }\n");
        }

        if (!opened_macro.empty()) {
            auto close_macro = "#endif /* " + opened_macro + " */\n";
            func_types += close_macro;
            out.write(close_macro, "\n");
        }

        for (auto& def : definitions)
            out.write("    ", def, "\n");

        out.write("\n"
                  "private:\n"
                  "    const ${itype}* ${iname};\n"
                  "    ${htype} ${hname};\n"
                  "\n"
                  "    core::tuple</* start */\n" |
                      cfg,
                  func_types,
                  "                /* end */ core::null_t>\n"
                  "    f;\n"
                  "};\n",
                  after_class,
                  "} /* namespace vk */\n");
    }

    std::string              name;
    class_param              instance;
    class_param              handle;
    std::vector<std::string> func_prefixes;
    std::vector<std::string> func_postfixes;
    std::set<std::string>    functions;
    std::vector<std::string> definitions;
    std::string              after_class;
};
} // namespace cg::vk
