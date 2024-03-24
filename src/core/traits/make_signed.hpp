#pragma once

namespace core {
namespace details {
    template <typename T>
    struct make_signed {
        using type = T;
    };

    template <>
    struct make_signed<char> {
        using type = signed char;
    };

    template <>
    struct make_signed<unsigned char> {
        using type = signed char;
    };

    template <>
    struct make_signed<unsigned short> {
        using type = signed short;
    };

    template <>
    struct make_signed<unsigned int> {
        using type = signed int;
    };

    template <>
    struct make_signed<unsigned long> {
        using type = signed long;
    };

    template <>
    struct make_signed<unsigned long long> {
        using type = signed long long;
    };
} // namespace details

template <typename T>
using make_signed = details::make_signed<T>::type;
} // namespace core
