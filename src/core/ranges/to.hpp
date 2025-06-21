#pragma once
#include <vector>

#include <core/ranges/range.hpp>
#include <core/ranges/inward.hpp>

namespace core {
template <typename Container>
struct to {
    template <typename I>
    Container operator()(I begin, I end) const {
        using T  = std::decay_t<decltype(*begin)>;
        using CT = std::decay_t<decltype(*core::begin(std::declval<Container>()))>;
        if constexpr (range<CT> && !std::is_same_v<CT, T>) {
            auto inward_fix = range_holder{begin, end} | inward(to<CT>());
            return Container(core::begin(inward_fix), core::end(inward_fix));
        }
        else
            return Container(begin, end);
    }

    friend Container operator|(range auto&& rng, const to& to) {
        using std::begin, std::end;
        return to(begin(rng), end(rng));
    }
};

struct to_vector {
    template <typename I>
    auto operator()(I begin, I end) const {
        return std::vector(begin, end);
    }

    friend auto operator|(range auto&& rng, const to_vector& to) {
        using std::begin, std::end;
        return to(begin(rng), end(rng));
    }
};
} // namespace core
