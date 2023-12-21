#pragma once

#include "../basic_types.hpp"

#if !__has_builtin(__type_pack_element)
#include "../ebo.hpp"
#include "../meta/type.hpp"
#include "../utility/int_seq.hpp"
#endif

namespace core {
namespace dtls {
#if __has_builtin(__type_pack_element)
    template <typename Pack, size_t I>
    struct _type_at_index_pack;
    template <template <typename...> typename Pack, typename... Ts, size_t I>
    struct _type_at_index_pack<Pack<Ts...>, I> {
        using type = __type_pack_element<I, Ts...>;
    };
    template <size_t I, typename... Ts>
    struct _type_at_index {
        using type = __type_pack_element<I, Ts...>;
    };
#else
    template <typename... Ts, size_t... Sz>
    constexpr auto make_index_type_indexer(idx_seq<Sz...>) {
        struct indexer : ebo<Sz, type_t<Ts>>... {};
        return indexer{};
    }

    template <typename Pack, size_t I>
    struct _type_at_index_pack;

    template <template <typename...> typename Pack, size_t I, typename... Ts>
    struct _type_at_index_pack<Pack<Ts...>, I> {
        using type = decltype(+get<I>(make_index_type_indexer<Ts...>(make_idx_seq<sizeof...(Ts)>())));
    };

    template <size_t I, typename... Ts>
    struct _type_at_index {
        using type = decltype(+get<I>(make_index_type_indexer<Ts...>(make_idx_seq<sizeof...(Ts)>())));
    };
#endif
} // namespace dtls

template <typename Pack, size_t I>
using type_at_index_pack = typename dtls::_type_at_index_pack<Pack, I>::type;

template <size_t I, typename... Ts>
using type_at_index = typename dtls::_type_at_index<I, Ts...>::type;
} // namespace core
