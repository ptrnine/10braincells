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

    template <typename T, typename... Ts>
    struct _index_of_type {
        static inline constexpr auto value = get<type<T>>(make_type_indexer<Ts...>(make_idx_seq<sizeof...(Ts)>()));
    };

    template <typename T, typename Pack>
    struct _index_of_type_pack;

    template <typename T, template <typename...> typename Pack, typename... Ts>
    struct _index_of_type_pack<T, Pack<Ts...>> : _index_of_type<T, Ts...> {};
} // namespace dtls

template <typename T, typename... Ts>
static inline constexpr auto idx_of_type = dtls::_index_of_type<T, Ts...>::value;

template <typename T, typename Pack>
static inline constexpr auto idx_of_type_pack = dtls::_index_of_type_pack<T, Pack>::value;
} // namespace core
