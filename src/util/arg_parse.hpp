#pragma once

#include <list>
#include <string>
#include <vector>

#include <core/as_tuple.cg.hpp>
#include <core/concepts/number.hpp>
#include <core/concepts/string.hpp>
#include <core/ct_str.hpp>
#include <core/enum_introspect.hpp>
#include <core/opt.hpp>
#include <core/var.hpp>
#include <core/ston.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

#define tbc_arg(name, ...) util::arg_def<#name##_ctstr, __VA_ARGS__> name
#define tbc_cmd(name) template <auto = #name##_ctstr> struct name##_cmd
#define tbc_sub(name, arg1, ...) util::var<arg1, __VA_ARGS__> name{arg1{}}

using namespace core::ct_str_literals;

namespace util
{
using core::as_tuple_ref, core::string_to_enum, core::build_str, core::ston, core::type_of, core::mov;
using core::is_ct_str, core::is_enum, core::is_same, core::null, core::type;
using core::opt, core::var, core::null_t, core::type_t, core::tuple, core::decl_type;
using core::same_as, core::any_of, core::number, core::floating_point, core::optional, core::signed_integral,
    core::unsigned_integral;

class arg_error : public core::exception {};

class missing_arg_error : public arg_error {
public:
    missing_arg_error(std::string imsg): msg(mov(imsg)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class generate_help_interrupt : core::exception {
public:
    generate_help_interrupt(std::string itext): text(mov(itext)) {}

    const char* what() const noexcept override {
        return "";
    }

    std::string text;
};

template <auto Name, typename T, auto Descr = core::ct_str<>{}>
struct arg_def {
    using type = T;

    constexpr arg_def() = default;

    template <core::convertible_to<T> U>
    constexpr arg_def(U&& value): _value(fwd(value)) {}

    constexpr std::string_view name() const {
        return Name;
    }

    constexpr std::string_view description() const {
        return Descr;
    }

    constexpr auto&& get(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto&& operator*(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto operator->(this auto&& it) {
        return &it._value;
    }

    T _value = {};
};

struct exec_path {
    constexpr auto&& get(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto&& operator*(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto operator->(this auto&& it) {
        return &it._value;
    }

    std::string_view _value;
};

struct cmdline {
    constexpr auto&& get(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto&& operator*(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto operator->(this auto&& it) {
        return &it._value;
    }

    std::vector<std::string_view> _value;
};

struct cmd_description {
    constexpr cmd_description() = default;
    constexpr cmd_description(std::string_view idata): data(idata) {}
    std::string_view data;
};

namespace details
{
    template <typename>
    struct command_def_traits {
        static inline constexpr bool value = false;
    };

    template <template <auto> class T, auto S>
        requires is_ct_str<decltype(S)>
    struct command_def_traits<T<S>> {
        static inline constexpr bool value = true;
        static inline constexpr auto name  = S;
    };

    template <typename T>
    concept command_def_ct = command_def_traits<T>::value;

    template <typename T>
    static inline constexpr bool is_arg_def = false;

    template <auto Name, typename T, auto Descr>
    static inline constexpr bool is_arg_def<arg_def<Name, T, Descr>> = true;

    template <typename T>
    static inline constexpr bool is_command_var = false;

    template <typename T, command_def_ct... Ts> requires (command_def_ct<T> || same_as<T, null_t>)
    static inline constexpr bool is_command_var<var<T, Ts...>> = true;

    enum class arg_type {
        exec_path = 0,
        cmd_description,
        cmdline,
        positional,
        key_value,
        boolean,
        command,
        command_var,
    };

    //template <auto>
    //void pf() {
    //    std::cout << "PF: " << __PRETTY_FUNCTION__ << std::endl;
    //}

    struct help_arg_info {
        template <typename T>
        help_arg_info(T& f): descr(f.description()) {
            std::string_view opt  = optional<typename T::type> ? "?" : "";
            auto             init = core::overloaded{
                [&](type_t<bool>) {
                    name = build_str("--", f.name(), "?");
                },
                [&]<floating_point U>(type_t<U>) {
                    name = build_str(f.name(), "=<float", opt, ">");
                },
                [&]<signed_integral U>(type_t<U>) {
                    name = build_str(f.name(), "=<int", opt, ">");
                },
                [&]<unsigned_integral U>(type_t<U>) {
                    name = build_str(f.name(), "=<uint", opt, ">");
                },
                [&]<any_of<std::string, std::string_view> U>(type_t<U>) {
                    name = build_str(f.name(), "=<int", opt, ">");
                },
                [](auto) {},
                };
            if constexpr (optional<typename T::type>)
                init(type<core::remove_cvref<decltype(*core::declval<typename T::type>())>>);
            else
                init(type<typename T::type>);
        }

        std::string name;
        std::string descr;
    };

    void generate_help(auto cmd_name, auto& v, std::string& text, std::string_view exec_path) {
        std::string                descr;
        std::vector<help_arg_info> arg_infos;
        size_t                     max_len = 0;

        v.foreach ([&]<typename T>(T& field) {
            if constexpr (is_same<T, cmd_description>) {
                descr = field.data;
            }
            else if constexpr (is_arg_def<T>) {
                arg_infos.emplace_back(field);
                auto len = arg_infos.back().name.size();
                if (max_len < len)
                    max_len = len;
            }
        });

        text += cmd_name;
        text += " - ";
        text += descr;
        text += "\n\nOptions:\n";
        for (auto&& arg : arg_infos) {
            text += "    ";
            text += arg.name;
            if (!arg.descr.empty()) {
                auto nspaces = arg.name.size() < max_len ? (max_len - arg.name.size()) : 0;
                text += std::string(nspaces, ' ');
                text += " - ";
                text += arg.descr;
            }
            text += '\n';
        }
    }

    template <typename T>
    constexpr arg_type deduce_arg_type(type_t<T>) {
        if constexpr (is_arg_def<T>) {
            using element_t = typename T::type;
            if constexpr (is_same<element_t, bool>)
                return arg_type::boolean;
            else if constexpr (command_def_ct<element_t>)
                return arg_type::command;
            else
                return arg_type::key_value;
        }
        else if constexpr (is_command_var<T>)
            return arg_type::command_var;
        else if constexpr (is_same<T, exec_path>)
            return arg_type::exec_path;
        else if constexpr (is_same<T, cmd_description>)
            return arg_type::cmd_description;
        else if constexpr (is_same<T, cmdline>)
            return arg_type::cmdline;
        else
            return arg_type::positional;
    }

    opt<std::string_view> arg_by_key(std::list<std::string_view>& args, std::string_view key, std::string& log) {
        auto key2 = "--" + std::string(key);
        auto key1 = std::string(key) + "=";

        for (auto it = args.begin(); it != args.end();) {
            if (it->starts_with(key1)) {
                //std::cout << "parse " << *it << " as key value" << std::endl;
                auto value = it->substr(key1.size());
                args.erase(it/*++*/);
                log += build_str(" ", key1, value);
                return value;
            }
            else if (*it == key2) {
                auto next = std::next(it);
                if (next == args.end())
                    throw missing_arg_error(build_str("Missing argument: ", log, " ", key1, "HERE"));
                auto value = *next;
                //std::cout << "parse " << value << " as keyvalue" << std::endl;
                ++next;
                args.erase(it, next);
                //it = next;
                log += build_str(" ", value);
                return value;
            }
            else
                ++it;
        }

        return {};
    }

    opt<std::string_view> arg_next(std::list<std::string_view>& args, std::string& log) {
        if (args.empty())
            return {};

        auto value = *args.begin();
        log += build_str(" ", value);
        args.pop_front();
        return value;
    }

    bool arg_bool(std::list<std::string_view>& args, std::string_view key, std::string& log) {
        auto k = "--" + std::string(key);
        for (auto it = args.begin(); it != args.end();) {
            if (*it == k) {
                //std::cout << "parse " << *it << " as bool" << std::endl;
                log += build_str(" ", k);
                args.erase(it/*++*/);
                return true;
            }
            else
                ++it;
        }
        return false;
    }

    template <typename T>
    struct arg_cast;

    template <any_of<std::string, std::string_view> T>
    struct arg_cast<T> {
        auto operator()(const opt<std::string_view>& value, std::string& log) {
            if (value) {
                //std::cout << "Cast '" << *value << "' to string" << std::endl;
                return T{*value};
            }

            throw missing_arg_error(build_str("Missing string value: ", log, " <-- HERE"));
        }
    };

    template <typename T> requires is_enum<T>
    struct arg_cast<T> {
        T operator()(const opt<std::string_view>& value, std::string& log) {
            if (value) {
                // std::cout << "Cast '" << *value << "' to enum" << std::endl;
                try {
                    return string_to_enum<T>(*value);
                }
                catch (...) {
                    throw missing_arg_error(build_str("Invalid enum: ", log, " <-- HERE"));
                }
            }
            throw missing_arg_error(build_str("Missing enum value: ", log, " <-- HERE"));
        }
    };

    template <typename T>
    struct arg_cast<opt<T>> {
        opt<T> operator()(const opt<std::string_view>& value, std::string& log) {
            if (value)
                return arg_cast<T>{}(value, log);
            return {};
        }
    };

    template <number T>
    struct arg_cast<T> {
        T operator()(const opt<std::string_view>& value, std::string& log) {
            if (value) {
                try {
                    if constexpr (floating_point<T>) {
                        return ston<T>(*value);
                    }
                    else {
                        if (value->starts_with("0x"))
                            return ston<T>(value->substr(2), 16);
                        else if (value->starts_with("0b"))
                            return ston<T>(value->substr(2), 2);
                        else if (value->starts_with('0'))
                            return ston<T>(value->substr(1), 8);
                        else
                            return ston<T>(*value, 10);
                    }
                }
                catch (...) {
                    throw missing_arg_error(build_str("Invalid numer: ", log, " <-- HERE"));
                }
            }
            throw missing_arg_error(build_str("Missing numer value: ", log, " <-- HERE"));
        }
    };

    template <typename... Ts>
    void parse_commands_var(std::list<std::string_view>& args,
                            var<Ts...>&                  result,
                            std::string&                 log,
                            std::string_view             exec_path,
                            auto&                        cmdline) {
        tuple<Ts...> vs;

        auto arg = arg_next(args, log);
        if (!arg) {
            if constexpr (any_of<null_t, Ts...>) {
                result = null;
                return;
            }
        }

        //std::cout << "CMD NAME: " << *arg << std::endl;
        bool init = false;
        vs.foreach ([&](auto&& value) {
            if constexpr (type_of(value) != type_of(null)) {
                if (command_def_traits<decl_type<type_of(value)>>::name == *arg) {
                    constexpr auto t = type_of(value);
                    result.emplace(t);
                    log += build_str(" ", *arg);
                    parse_commands(args, result.unsafe_get(t), log, exec_path, cmdline);
                    init = true;
                }
            }
        });

        if (!init) {
            if constexpr (any_of<null_t, Ts...>) {
                args.push_front(*arg);
                result = null;
                return;
            }
            else {
                std::string names;
                vs.foreach ([&](auto&& v) {
                    if (!names.empty())
                        names += "|";
                    names += command_def_traits<decl_type<type_of(v)>>::name;
                });
                throw missing_arg_error(build_str("Missing subcommand: ", log, " HERE(", names, ")"));
            }
        }

        return;
    }

    void parse_commands(
        std::list<std::string_view>& args, auto& result, std::string& log, std::string_view exec_path, auto& cmdline) {
        auto struct_ref = as_tuple_ref(result);

        if (!args.empty() && args.front() == "--help") {
            std::string text; // = "Args: " + log + "\n\n";
            generate_help(command_def_traits<core::remove_ref<decltype(result)>>::name, struct_ref, text, exec_path);
            throw generate_help_interrupt(text);
        }

        struct_ref.foreach ([&](auto&& v) {
            constexpr auto t     = type_of(v);
            constexpr auto arg_t = deduce_arg_type(t);
            // pf<t>();
            // pf<arg_t>();
            if constexpr (arg_t == arg_type::boolean)
                v._value = arg_bool(args, v.name(), log);
            else if constexpr (arg_t == arg_type::key_value) {
                auto arg = arg_by_key(args, v.name(), log);
                constexpr auto is_opt_type = optional<typename decl_type<t>::type>;
                if constexpr (!is_opt_type) {
                    if (arg.empty())
                        throw missing_arg_error(build_str("Missing key argument: ", log, " ", v.name(), "=HERE"));
                }
                if (!is_opt_type || (is_opt_type && !arg.empty()))
                    v._value = arg_cast<typename decl_type<t>::type>{}(arg, log);
            }
            else if constexpr (arg_t == arg_type::positional) {
                auto arg = arg_next(args, log);
                v        = arg_cast<decl_type<t>>{}(arg, log);
            }
            else if constexpr (arg_t == arg_type::command) {
                parse_commands(args, v._value, log, exec_path, cmdline);
            }
            else if constexpr (arg_t == arg_type::command_var) {
                parse_commands_var(args, v, log, exec_path, cmdline);
            }
            else if constexpr (arg_t == arg_type::exec_path) {
                v._value = exec_path;
            }
            else if constexpr (arg_t == arg_type::cmdline) {
                v._value = cmdline;
            }
        });

        if (!args.empty() && args.front() == "--help") {
            std::string text; // = "Args: " + log + "\n\n";
            generate_help(command_def_traits<core::remove_ref<decltype(result)>>::name, struct_ref, text, exec_path);
            throw generate_help_interrupt(text);
        }
    }
} // namespace details

template <details::command_def_ct T>
T parse_args(int argc, const char** argv, bool have_exec_path = true) {
    T   result;
    int argn = 0;

    std::string                   parse_log;
    std::string_view              exec_path;
    std::vector<std::string_view> cmdline;

    for (auto&& arg : std::span{argv, argv + argc})
        cmdline.push_back(arg);

    if (have_exec_path) {
        if (argc < 1)
            throw missing_arg_error("Missing exec path argument");
        exec_path = argv[argn];
        parse_log += exec_path;
        ++argn;
    }

    std::list<std::string_view> args(argv + argn, argv + argc);
    details::parse_commands(args, result, parse_log, exec_path, cmdline);
    return result;
}

namespace arg
{
    enum class opt { off = 0, on };
    using triopt = core::opt<opt>;
} // namespace arg
} // namespace util

#undef fwd
