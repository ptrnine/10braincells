#pragma once

#include <string_view>

#include <core/concepts/char_type.hpp>
#include <core/traits/is_same.hpp>
#include <core/utility/int_seq.hpp>

namespace core {
template <char... Cs>
struct ct_str {
    static constexpr const char _storage[] = {Cs..., '\0'};

    constexpr const char* data() const {
        return _storage;
    }

    constexpr size_t size() const {
        return sizeof...(Cs);
    }

    template <typename StartT, typename SizeT = int_const<sizeof...(Cs) - StartT{}>>
    constexpr auto substr(StartT, SizeT) const {
        return []<size_t... Idxs>(idx_seq<Idxs...>) {
            return ct_str<_storage[StartT{} + Idxs]...>{};
        }(make_idx_seq<SizeT{}>());
    }

    template <char... Cs2>
    constexpr auto operator+(ct_str<Cs2...>) const {
        return ct_str<Cs..., Cs2...>{};
    }
    constexpr operator std::string_view() const {
        return {data(), size()};
    }

    template <char... Cs2>
    constexpr bool operator==(const ct_str<Cs2...>&&) const {
        return is_same<ct_str, ct_str<Cs2...>>;
    }
    template <char... Cs2>
    constexpr bool operator!=(const ct_str<Cs2...>&&) const {
        return !std::is_same_v<ct_str, ct_str<Cs2...>>;
    }
};

template <typename T>
static inline constexpr bool is_ct_str = false;

template <char... Cs>
static inline constexpr bool is_ct_str<ct_str<Cs...>> = true;
} // namespace core

#define CT_STR(STR)                                        \
    []<size_t... Idxs>(core::idx_seq<Idxs...>) constexpr { \
        return core::ct_str<STR[Idxs]...>();               \
    }(core::make_idx_seq<sizeof(STR) - 1>())

namespace core::ct_str_literals
{
template <typename T, T... Cs>
constexpr core::ct_str<Cs...> operator""_ctstr() {
    return {};
}
} // namespace core::ct_str_literals
