#pragma once

namespace core {
namespace details {
    template <bool, typename T1, typename>
    struct conditional {
        using type = T1;
    };
    template <typename T1, typename T2>
    struct conditional<false, T1, T2> {
        using type = T2;
    };
} // namespace details

template <bool Cond, typename IfTrue, typename IfFalse>
using conditional = typename details::conditional<Cond, IfTrue, IfFalse>::type;
} // namespace core
