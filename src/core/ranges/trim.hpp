#pragma once

#include <core/ranges/range.hpp>
#include <core/utility/move.hpp>
#include <core/array.hpp>

namespace core
{
namespace details_trim
{
    auto prev(auto i) {
        auto res = i;
        --res;
        return res;
    }
} // namespace details_trim

template <typename F>
struct trim_if {
    trim_if(F ibinary_op): binary_op(mov(ibinary_op)) {}

    void trim_op(auto& b, auto& e) {
        while (b != e && binary_op(*b))
            ++b;
        while (e != b && binary_op(*details_trim::prev(e)))
            --e;
    }

    auto operator()(bidirect_range auto&& rng) {
        auto b = begin(rng);
        auto e = end(rng);
        trim_op(b, e);
        return range_holder{b, e};
    }

    friend auto operator|(bidirect_range auto&& rng, trim_if tr) {
        return tr(rng);
    }

    F binary_op;
};

template <typename C, size_t N>
struct trim {
    template <typename ArgT, typename... ArgsT>
    trim(ArgT trimmed_symbol, ArgsT... trimmed_symbols): cs{mov(trimmed_symbol), mov(trimmed_symbols)...} {}

    void trim_op(auto& b, auto& e) const {
        constexpr auto has = [](const array<C, N>& cs, auto cur) {
            bool result = false;
            for (auto c : cs)
                result = result || c == cur;
            return result;
        };
        while (b != e && has(cs, *b))
            ++b;
        while (b != e && has(cs, *details_trim::prev(e)))
            --e;
    }

    auto operator()(bidirect_range auto&& rng) const {
        auto b = begin(rng);
        auto e = end(rng);
        trim_op(b, e);
        return range_holder{b, e};
    }

    friend auto operator|(bidirect_range auto&& rng, trim tr) {
        return tr(rng);
    }

    array<C, N> cs;
};

template <typename C, typename... Cs>
trim(C, Cs...) -> trim<C, sizeof...(Cs) + 1>;
} // namespace core
