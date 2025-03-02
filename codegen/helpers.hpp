#pragma once

#include <string>
#include <set>

namespace cg {
inline std::string to_snake_case(std::string_view str) {
    std::string res;
    bool        prev_upper = true;
    for (auto c : str) {
        bool upper = std::isupper(c);
        if (!prev_upper && upper)
            res.push_back('_');
        res.push_back(char(std::tolower(c)));
        prev_upper = upper;
    }
    return res;
}

inline std::string to_lower_case(const std::string& str) {
    std::string res;
    for (auto c : str)
        res.push_back(char(std::tolower(c)));
    return res;
}

static const auto cpp_keywords = [] {
    return std::set<std::string>{"alignas",
                                 "alignof",
                                 "and",
                                 "and_eq",
                                 "asm",
                                 "atomic_cancel",
                                 "atomic_commit",
                                 "atomic_noexcept",
                                 "auto",
                                 "bitand",
                                 "bitor",
                                 "bool",
                                 "break",
                                 "case",
                                 "catch",
                                 "char",
                                 "char8_t",
                                 "char16_t",
                                 "char32_t",
                                 "class",
                                 "compl",
                                 "concept",
                                 "const",
                                 "consteval",
                                 "constexpr",
                                 "constinit",
                                 "const_cast",
                                 "continue",
                                 "co_await",
                                 "co_return",
                                 "co_yield",
                                 "decltype",
                                 "default",
                                 "delete",
                                 "do",
                                 "double",
                                 "dynamic_cast",
                                 "else",
                                 "enum",
                                 "explicit",
                                 "export",
                                 "extern",
                                 "false",
                                 "float",
                                 "for",
                                 "friend",
                                 "goto",
                                 "if",
                                 "inline",
                                 "int",
                                 "long",
                                 "mutable",
                                 "namespace",
                                 "new",
                                 "noexcept",
                                 "not",
                                 "not_eq",
                                 "nullptr",
                                 "operator",
                                 "or",
                                 "or_eq",
                                 "private",
                                 "protected",
                                 "public",
                                 "reflexpr",
                                 "register",
                                 "reinterpret_cast",
                                 "requires",
                                 "return",
                                 "short",
                                 "signed",
                                 "sizeof",
                                 "static",
                                 "static_assert",
                                 "static_cast",
                                 "struct",
                                 "switch",
                                 "synchronized",
                                 "template",
                                 "this",
                                 "thread_local",
                                 "throw",
                                 "true",
                                 "try",
                                 "typedef",
                                 "typeid",
                                 "typename",
                                 "union",
                                 "unsigned",
                                 "using",
                                 "virtual",
                                 "void",
                                 "volatile",
                                 "wchar_t",
                                 "while",
                                 "xor",
                                 "xor_eq"};
}();

inline bool is_cpp_keyword(const std::string& str) {
    return cpp_keywords.contains(str);
}

inline bool is_digit_started(const std::string& str) {
    return !str.empty() && std::isdigit(str.front());
}

inline void add_underscore_if_invalid(std::string& str) {
    if (is_cpp_keyword(str) || is_digit_started(str))
        str = "_" + str;
}

bool drop_postfix(std::string& v, std::string_view postfix) {
    if (v.ends_with(postfix)) {
        v.resize(v.size() - postfix.size());
        return true;
    }
    return false;
}

bool drop_prefix(std::string& v, std::string_view prefix) {
    if (v.starts_with(prefix)) {
        v = v.substr(prefix.size());
        return true;
    }
    return false;
}

bool drop_postfixes(std::string& v, auto&&... postfixes) {
    return (drop_postfix(v, postfixes) || ...);
}

bool drop_prefixes(std::string& v, auto&&... prefixes) {
    return (drop_prefix(v, prefixes) || ...);
}

void drop_suffix(std::string& v, std::string_view suffix) {
    auto pos = v.rfind(suffix);
    if (pos != v.npos)
        v.erase(v.begin() + long(pos), v.begin() + long(pos + suffix.size()));
}
} // namespace cg
