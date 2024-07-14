#pragma once

#include <csignal>

#include <core/bit.hpp>
#include <core/concepts/integral.hpp>

namespace sys
{
using sig_word_t                    = unsigned long int;
static constexpr size_t word_width  = sizeof(sig_word_t) * 8;
static constexpr size_t words_count = core::round_up(_NSIG - 1, word_width) / word_width;
static_assert(words_count == 1);

struct sigset {
    static constexpr sigset empty() {
        return {0};
    }

    static constexpr sigset filled() {
        return {~sig_word_t{0}};
    }

    static constexpr sigset with(core::integral auto... signals) {
        sigset res{0};
        res.add(signals...);
        return res;
    }

    constexpr void add(core::integral auto... signals) {
        ((_val |= (sig_word_t(1) << (signals - 1))), ...);
    }

    constexpr void del(core::integral auto... signals) {
        ((_val &= ~(sig_word_t(1) << (signals - 1))), ...);
    }

    constexpr bool test(int signal) const {
        return _val & (sig_word_t(1) << (signal - 1));
    }

    sig_word_t _val;
};
} // namespace sys
