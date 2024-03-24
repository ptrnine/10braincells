#pragma once

#include "basic_types.hpp"

namespace core {
constexpr auto to_int(char c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    else
        return c - '0';
}

struct parse_int_result {
    unsigned long long value;
    bool overflow;
};

template <size_t o = 0, size_t max, size_t s>
constexpr parse_int_result parse_int_n(const char (&str)[s]) {
    unsigned base     = 10;
    size_t   offset   = o;
    bool     overflow = false;

    if (str[0] == '0' && o + max != 1) {
        switch (str[1]) {
        case 'b':
            base = 2;
            offset += 2;
            break;
        case 'x':
            base = 16;
            offset += 2;
            break;
        default: base = 8; offset += 1;
        }
    }

    unsigned long long result = 0;
    for (size_t i = offset; i < max; ++i) {
        if (str[i] == '\'')
            continue;

        auto prev = result;
        result *= base;
        result += to_int(str[i]);

        if (prev > result || (prev == result && str[i] != '0'))
            overflow = true;
    }

    return {result, overflow};
}

template <size_t o = 0, size_t s>
constexpr auto parse_int(const char (&str)[s]) {
    return parse_int_n<o, s - o, s>(str);
}

template <char... cs>
constexpr auto parse_int_checked() {
    constexpr auto res = parse_int<0, sizeof...(cs)>({cs...});
    static_assert(!res.overflow, "Integer literal is too large to be represented in any integer type");
    return res.value;
}
} // namespace core
