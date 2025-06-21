#pragma once

#include <string>
#include <vector>

#include <codegen/helpers.hpp>
#include <core/exception.hpp>
#include <core/generator.hpp>
#include <core/ranges/subst.hpp>

namespace cg
{
struct function_cpp_arg {
    std::string type;
    std::string name;
    std::string bind_arg;
    std::string bind_size;
    std::string default_val;
    bool need_tmp = false;
    bool calculate_size = false;
    std::string length_calc;

    std::string get_name() const {
        return need_tmp ? "tmp" + name : name;
    }
};

struct function_arg {
    std::string generate() const {
        return type + ' ' + name;
    }

    std::string type;
    std::string name;
    std::string length_arg;
    std::string optional;
};

class function_arg_not_found : public core::exception {
public:
    function_arg_not_found(const std::string& argname): msg("arg not found: " + argname) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

struct function_cpp_args {
    struct get_arg_result {
        const function_cpp_arg* arg;
        bool                    size = false;
        bool                    is_result = false;
    };

    get_arg_result get_arg_by_c_name(const std::string& cname) const {
        for (auto&& arg : args) {
            if (arg.bind_arg == cname)
                return {&arg};
            else if (arg.bind_size == cname)
                return {&arg, true};
        }
        if (result.bind_arg == cname)
            return {&result, false, true};
        else if (result.bind_size == cname)
            return {&result, true, true};

        throw function_arg_not_found{"c-arg " + cname};
    }

    std::vector<std::string> arg_defs(const std::set<std::string>& skip_args = {}, bool print_defaults = true) const {
        std::vector<std::string> res;
        for (auto&& arg : args) {
            if (!skip_args.contains(arg.name)) {
                res.push_back(arg.type + " " + arg.name);
                if (print_defaults && !arg.default_val.empty()) {
                    res.back() += " = " + arg.default_val;
                }
            }
        }
        return res;
    }

    std::vector<std::string> arg_names(const std::set<std::string>& skip_args = {}) const {
        std::vector<std::string> res;
        for (auto&& arg : args) {
            if (!skip_args.contains(arg.name)) {
                res.push_back(arg.name);
            }
        }
        return res;
    }

    std::vector<function_cpp_arg> args;
    function_cpp_arg              result;
};

inline function_cpp_args cppify_args(std::span<const function_arg> args) {
    function_cpp_args res;

    std::set<std::string> remove_ints;

    for (auto&& arg : args) {
        if (!arg.type.starts_with("const ") && arg.type.ends_with("*") && res.result.bind_size != arg.name) {
            res.result.name     = arg.name;
            res.result.type     = arg.type;
            res.result.bind_arg = arg.name;
            drop_postfix(res.result.type, "*");
            if (!arg.length_arg.empty()) {
                res.result.bind_size = arg.length_arg;
                res.result.type = "std::vector<" + res.result.type + ">";
                if (res.result.bind_size.contains("->")) {
                    auto length = res.result.bind_size;
                    length.replace(length.find("->"), 2, ".");
                    res.result.length_calc = length;
                }
            } else {
                res.result.calculate_size = true;
            }
        }
        else if (arg.type.starts_with("const ") && arg.type.ends_with("*")) {
            res.args.push_back({});
            auto& a = res.args.back();
            if (arg.optional.starts_with("true")) {
                a.type = arg.type;
                drop_prefix(a.type, "const ");
                drop_postfix(a.type, "*");
                if (arg.length_arg == "null-terminated" && a.type.starts_with("char"))
                    a.type = "std::string";

                a.type        = "core::opt<" + a.type + ">";
                a.name        = arg.name;
                a.bind_arg    = arg.name;
                a.default_val = "core::null";
            }
            else {
                a.type = arg.type;
                drop_postfix(a.type, "*");
                a.name     = arg.name;
                a.bind_arg = arg.name;

                if (!arg.length_arg.empty()) {
                    remove_ints.emplace(arg.length_arg);
                    a.type = "std::span<" + a.type + ">";
                    a.bind_size = arg.length_arg;
                    //res.result.calculate_size = false;
                    res.result.length_calc = a.name + ".size()";
                } else {
                    a.type += '&';
                }
            }
        }
        else {
            res.args.push_back({});
            auto& a    = res.args.back();
            a.type     = arg.type;
            a.name     = arg.name;
            a.bind_arg = arg.name;
        }
    }

    for (auto&& arg : remove_ints) {
        std::erase_if(res.args, [&](const function_cpp_arg& a) { return a.name == arg; });
    }

    return res;
}

struct function_type {
    std::vector<function_arg> deduce_return() const {
        std::vector<function_arg> res;

        for (auto&& arg : args)
            if (!arg.type.starts_with("const ") && arg.type.ends_with("*"))
                res.push_back(arg);

        return res;
    }

    bool can_fail() const {
        return !(success_codes.empty() && error_codes.empty());
    }

    std::string gen_signature(std::string iname = {}) const {
        if (iname.empty())
            iname = name;

        std::string res = "using ";
        res += iname;
        res += " = ";
        res += return_type;
        res += "(*)(";
        for (auto&& arg : args) {
            res += arg.generate();
            res += ", ";
        }
        if (res.ends_with(", "))
            res.resize(res.size() - 2);
        res += ");";
        return res;
    }

    void generate(auto&& out) const {
        if (!macro_condition.empty())
            out.write("#if ", macro_condition, "\n");
        out.write(gen_signature(), "\n");
        if (!macro_condition.empty())
            out.write("#endif /* ", macro_condition, " */\n");
    }

    void generate_traits(auto&& out) const {
        using core::views::subst, core::views::subst_entry;

        if (!macro_condition.empty())
            out.write("#if ", macro_condition, "\n");

        auto fold_codes = [](auto&& codes) {
            std::string res;
            for (auto&& code : codes) {
                res += "vk::result::";
                res += code;
                res += ", ";
            }
            return res;
        };

        auto scodes = fold_codes(success_codes);
        auto ecodes = fold_codes(error_codes);
        auto scount = std::to_string(success_codes.size());
        auto ecount = std::to_string(error_codes.size());

        out.write("struct ${name}_t {\n"
                  "    ${func}\n"
                  "\n"
                  "    template <typename T>\n"
                  "    static constexpr auto result_type(core::type_t<T> = {}) {\n"
                  "        return core::type<result_t<T, success_codes, error_codes>>;\n"
                  "    }\n"
                  "\n"
                  "    static inline constexpr const char* name = \"${native}\";\n"
                  "\n"
                  "    static inline constexpr core::array<vk::result, ${scount}> success_codes = {${scodes}};\n"
                  "\n"
                  "    static inline constexpr core::array<vk::result, ${ecount}> error_codes = {${ecodes}};\n"
                  "\n"
                  "    type call = nullptr;\n"
                  "};\n"
                  "static inline constexpr core::type_t<${name}_t> ${name} = {};\n" |
                  subst{
                      subst_entry{"name", name},
                      subst_entry{"func", gen_signature("type")},
                      subst_entry{"native", native},
                      subst_entry{"scodes", scodes},
                      subst_entry{"ecodes", ecodes},
                      subst_entry{"scount", scount},
                      subst_entry{"ecount", ecount},
                  });

        if (!macro_condition.empty())
            out.write("#endif /* ", macro_condition, " */\n");
    }

    void generate_command_name_cast(auto&& out) const {
        using core::views::subst, core::views::subst_entry;
        if (!macro_condition.empty())
            out.write("#if ", macro_condition, "\n");
        out.write("template <>\n"
                       "struct details::_command_name<${name}> {\n"
                       "    static inline constexpr const char* value = \"${native}\";\n"
                       "};\n" |
                       subst{
                           subst_entry{"name", name},
                           subst_entry{"native", native},
                       });
        if (!macro_condition.empty())
            out.write("#endif /* ", macro_condition, " */\n");
    }

    const function_arg& get_arg(const std::string& name) const {
        for (auto&& arg : args)
            if (arg.name == name)
                return arg;
        throw function_arg_not_found{name};
    }

    std::string               native;
    std::string               name;
    std::string               return_type;
    std::string               macro_condition;
    std::vector<function_arg> args;
    std::vector<std::string>  success_codes;
    std::vector<std::string>  error_codes;
};



inline std::vector<std::string>
pass_cpp_args_to_c(const function_type& cfunc, const function_cpp_args& args, bool pass_result_size_only) {
    std::vector<std::string> result;

    for (auto&& carg : cfunc.args) {
        auto arg = args.get_arg_by_c_name(carg.name);
        if (arg.size) {
            if (arg.is_result)
                result.push_back("&count");
            else
                result.push_back("core::u32(" + arg.arg->get_name() + ".size())");
        }
        else {
            if (arg.is_result) {
                if (pass_result_size_only)
                    result.push_back("nullptr");
                else {
                    std::string val = cfunc.can_fail() ? "res.value_unsafe" : "res";
                    if (arg.arg->type.starts_with("std::vector"))
                        result.push_back(val + ".data()");
                    else
                        result.push_back("&" + val);
                }
            }
            else {
                if (arg.arg->type.ends_with('&') && carg.type.ends_with('*'))
                    result.push_back("&" + arg.arg->get_name());
                else if (arg.arg->type.starts_with("std::span"))
                    result.push_back(arg.arg->get_name() + ".data()");
                else
                    result.push_back(arg.arg->get_name());
            }
        }
    }

    return result;
}
} // namespace cg
