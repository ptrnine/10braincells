#pragma once

#include "../basic_types.hpp"
#include "../ebo.hpp"
#include "../meta/type.hpp"
#include "../utility/int_seq.hpp"

namespace core {
namespace dtls {
    template <typename... Ts>
    struct type_indexer : ebo<type<Ts>, size_t>... {};

    template <typename... Ts, size_t... Sz>
    constexpr auto make_type_indexer(idx_seq<Sz...>) {
        return type_indexer<Ts...>{{Sz}...};
    }

    template <typename Pack, typename T>
    struct _index_of_type;

    template <template <typename...> typename Pack, typename T, typename... Ts>
    struct _index_of_type<Pack<Ts...>, T> {
        static inline constexpr auto value = get<type<T>>(make_type_indexer<Ts...>(make_idx_seq<sizeof...(Ts)>()));
    };
} // namespace dtls

template <typename Pack, typename T>
static inline constexpr auto idx_of_type = dtls::_index_of_type<Pack, T>::value;
} // namespace core
