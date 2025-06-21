#pragma once

#include <ranges>
#include <string>
#include <set>
#include <map>

#include <core/generator.hpp>
#include <core/ranges/subst.hpp>
#include <core/ranges/fold.hpp>
#include <core/string_builder.hpp>
#include <core/opt.hpp>
#include <core/io/out.hpp>

#include <codegen/vulkan/function_type.hpp>

namespace cg::vk {
struct class_param {
    std::string type;
    std::string name;
    std::string real_type;
};

enum gen_type {
    full = 0,
    full_plus_declare_interface,
    only_cache_this,
    destructor,
};

struct member_function {
    bool operator<(const member_function& mf) const {
        return name < mf.name;
    }

    std::string name;
    gen_type type = gen_type::full;
    core::opt<std::string> external_interface = {};
};

struct external_generated {
    std::string code_for_instance(const std::string& external_instance_type) const {
        auto found = instances.find(external_instance_type);
        if (found != instances.end()) {
            return found->second;
        }
        return {};
    }

    void generate_for_instance(const std::string& instance, const std::string& code) {
        instances[instance] += code;
    }

    std::map<std::string, std::string> instances;
};

struct class_instance_dependent {
    void generate(auto&& out, std::span<const function_type> funcs, external_generated& external) {
        using core::views::subst_entry, core::views::subst, core::views::fold;
        auto  handle_store = copyable ? handle.type : "core::moveonly_trivial<" + handle.type + ", nullptr>";
        subst cfg          = {
            subst_entry{"name", name},
            subst_entry{"iname", instance.name},
            subst_entry{"itype", instance.type},
            subst_entry{"hname", handle.name},
            subst_entry{"htype", handle.type},
            subst_entry{"hstore", handle_store},
        };

        // auto func_names_generator = [&] -> core::generator<std::string> {
        //     for (auto&& f : funcs) {
        //         if (!functions.contains(f.name))
        //             continue;

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
            "     }\n" | cfg);
        if (!copyable) {
            out.write("\n"
                      "    ${name}(${name}&&) noexcept = default;\n"
                      "    ${name}& operator=(${name}&&) noexcept = default;\n" |
                      cfg);
        }
        out.write(("\n"
                   "    void load_functions_cached(auto&... functions) const {\n"
                   "        ${iname}->load_functions_cached(functions...);\n"
                   "    }\n"
                   "\n"
                   "    ${htype} handle() const {\n"
                   "        return ${hname};\n"
                   "    }\n"
                   "\n"
                   "    auto& handle_ref(this auto& it) {\n"
                   "        return it.${hname}" +
                   (copyable ? std::string{} : ".get()") +
                   ";\n"
                   "    }\n") |
                  cfg);

        if (user_defined_dtor) {
            out.write("\n    ~${name}();\n" | cfg);
        }

        std::string func_types;
        std::string opened_macro;

        /* Gen functions */
        for (auto&& f : funcs) {
            auto function_info = functions.find(member_function{f.name});
            if (function_info == functions.end()) {
                continue;
            }

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

            if (function_info->type == gen_type::only_cache_this) {
                continue;
            }

            auto args = cppify_args(f.args);

            /* Take handle */
            for (auto& arg : args.args) {
                if (arg.type == handle.type) {
                    arg.name = handle.name;
                }
                if (arg.type == instance.real_type) {
                    arg.name = instance.name + "->handle()";
                }
            }

            if (function_info->type == gen_type::destructor) {
                auto dtor_args = args;
                for (auto& arg : dtor_args.args) {
                    /* TODO: pass allocation_callbacks to destructor */
                    if (arg.type == "core::opt<vk::allocation_callbacks>") {
                        arg.name = "nullptr";
                    }
                }

                out.write("    ~${name}() {\n" | cfg);
                if (!copyable) {
                    out.write("        if (${hname}.not_default()) {\n" | cfg,
                              "            f[cmd::",
                              function_info->name,
                              "].call(",
                              pass_cpp_args_to_c(f, dtor_args, true) | fold(", "),
                              ");\n"
                              "        }\n");
                }
                else {
                    out.write("        f[cmd::",
                              function_info->name,
                              "].call(",
                              pass_cpp_args_to_c(f, dtor_args, true) | fold(", "),
                              ");\n");
                }
                out.write("    }\n");
                continue;
            }

            /* Gen member function name */
            auto funcname = f.name;
            for (auto& pref : func_prefixes)
                if (drop_prefix(funcname, pref))
                    break;
            for (auto& post : func_postfixes)
                if (drop_postfixes(funcname, post))
                    break;

            auto generated_function_name = funcname;
            if (function_info->type == gen_type::full_plus_declare_interface) {
                generated_function_name += "_raw";
            }

            out.write("    auto ${name}(" | subst{subst_entry{"name", generated_function_name}},
                      args.arg_defs({handle.name, instance.name + "->handle()"}) | fold(", "),
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
                    arg.need_tmp = true;
                    //arg.name = "tmp" + arg.name;
                }
            }

            /* Define result */
            if (f.can_fail()) {
                auto result_type = args.result.type.empty() ? "void" : args.result.type;
                out.write("        auto res = func.result_type(core::type<${type}>).make();\n" |
                          subst{subst_entry{"type", result_type}});
            }
            else {
                if (!args.result.type.empty()) {
                    out.write("        ${type} res;\n" | subst{subst_entry{"type", args.result.type}});
                }
            }

            /* Calculate len */
            if (!args.result.bind_size.empty()) {
                if (args.result.calculate_size) {
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
                }
                out.write("\n"
                          "        res",
                          std::string_view(f.can_fail() ? ".value_unsafe" : ""),
                          ".resize(",
                          args.result.length_calc.empty() ? "count" : args.result.length_calc,
                          ");\n");
            }

            out.write("        ",
                      std::string_view(f.can_fail() ? "res.rc = " : ""),
                      "func.call(",
                      pass_cpp_args_to_c(f, args, false) | fold(", "),
                      ");\n");
            if (f.can_fail() || !args.result.type.empty()) {
                out.write("        return res;\n");
            }
            out.write("    }\n");

            if (function_info->type == gen_type::full_plus_declare_interface) {
                out.write("\n    auto ${name}(" | subst{subst_entry{"name", funcname}},
                          args.arg_defs({handle.name}) | fold(", "),
                          ") const;\n");

                if (function_info->external_interface) {
                    core::io::out o{std::string{}};
                    o.write("\nauto ${itype}::${name}(" |
                                subst{subst_entry{"name", funcname}, subst_entry{"itype", name}},
                            args.arg_defs({handle.name}, false) | fold(", "),
                            ") const {\n");
                    if (args.result.type.starts_with("std::vector")) {
                        subst sbst{subst_entry{"iface", *function_info->external_interface},
                                   subst_entry{"raw_func", generated_function_name}};

                        o.write("    std::vector<${iface}> res;\n"
                                "    auto elements = ${raw_func}(" | sbst,
                                args.arg_names({handle.name}) | fold(", "),
                                ");\n",
                                "    res.reserve(elements.value().size());\n"
                                "    for (auto&& e : elements.value()) {\n"
                                "        res.emplace_back(*this, core::mov(e));\n"
                                "    }\n"
                                "    return res;\n");
                    }
                    else {
                        o.write("    return ${iface}{*this, ${raw_func}(" |
                                    subst{subst_entry{"iface", *function_info->external_interface},
                                          subst_entry{"raw_func", generated_function_name}},
                                args.arg_names({handle.name}) | fold(", "),
                                ")",
                                f.can_fail() ? ".value()" : std::string(""),
                                "};\n");
                    }
                    o.write("}\n");
                    o.flush();

                    external.generate_for_instance(*function_info->external_interface, o.base_buff());
                }
            }
        }

        if (!opened_macro.empty()) {
            auto close_macro = "#endif /* " + opened_macro + " */\n";
            func_types += close_macro;
            out.write(close_macro, "\n");
        }

        for (auto& def : definitions)
            out.write("    ", def, "\n");

        out.write("\n"
                  "    operator ${htype}() const {\n"
                  "        return ${hname};\n"
                  "    }\n"
                  "\n"
                  "private:\n"
                  "    const ${itype}* ${iname};\n"
                  "    ${hstore} ${hname};\n"
                  "\n"
                  "    core::tuple</* start */\n" |
                      cfg,
                  func_types,
                  "                /* end */ core::null_t>\n"
                  "    f;\n"
                  "};\n",
                  external.code_for_instance(name),
                  after_class,
                  "} /* namespace vk */\n");
    }

    std::string               name;
    class_param               instance;
    class_param               handle;
    std::vector<std::string>  func_prefixes;
    std::vector<std::string>  func_postfixes;
    std::set<member_function> functions;
    std::vector<std::string>  definitions;
    std::string               after_class;
    bool                      user_defined_dtor = false;
    bool                      copyable = true;
};
} // namespace cg::vk
