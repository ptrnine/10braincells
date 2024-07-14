#pragma once

#include <core/begin_end.hpp>
#include <core/int_const.hpp>
#include <core/meta/type_list.hpp>
#include <core/tuple.hpp>
#include <core/utility/int_seq.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {

template <typename... Ts>
struct zip_iterator_sentinel {
    zip_iterator_sentinel() = default;
    zip_iterator_sentinel(Ts&&... iters): iterators{fwd(iters)...} {}
    zip_iterator_sentinel(const tuple<Ts...>& t): iterators(t) {}

    tuple<Ts...> iterators;
};

template <typename... Ts>
struct zip_iterator_fwd {
    zip_iterator_fwd() = default;
    zip_iterator_fwd(Ts&&... iters): iterators{fwd(iters)...} {}
    zip_iterator_fwd(const tuple<Ts...>& t): iterators(t) {}

    auto& operator++(this auto&& it) {
        it.iterators.foreach([](auto& i) { ++i; });
        return it;
    }

    auto operator++(this auto&& it, int) {
        auto i = it;
        ++it;
        return i;
    }

    template <typename... Ts2> requires(sizeof...(Ts) == sizeof...(Ts2))
    bool operator==(const zip_iterator_sentinel<Ts2...>& sentinel) const {
        return make_idx_seq<sizeof...(Ts)>().reduce(
            false, [&](auto result, auto idx) { return result || (iterators[idx] == sentinel.iterators[idx]); });
    }

    template <typename... Ts2> requires (sizeof...(Ts) == sizeof...(Ts2))
    bool operator!=(const zip_iterator_sentinel<Ts2...>& zi) const {
        return !(*this == zi);
    }

    auto operator*(this auto&& it) {
        return it.iterators.map([](auto&& i) -> decltype(*i) { return *i; });
    }

    tuple<Ts...> iterators;
};

template <typename T>
concept zip_bidir_iterator = requires(T& t) {
    t.foreach([](auto& i) {
        --i;
        i--;
    });
};

template <typename... Ts>
struct zip_iterator_bidir : zip_iterator_fwd<Ts...> {
    using zip_iterator_fwd<Ts...>::zip_iterator_fwd;

    auto& operator--(this auto&& it) {
        it.iterators.foreach([](auto& i) { --i; });
        return it;
    }

    auto operator--(this auto&& it, int) {
        auto i = it;
        --it;
        return i;
    }
};

template <typename... Ts1, typename... Ts2> requires (sizeof...(Ts1) == sizeof...(Ts2))
diff_t zip_difference(const tuple<Ts1...>& lhs, const tuple<Ts2...>& rhs) {
    return make_idx_seq<sizeof...(Ts1)>().reduce(limits<diff_t>::max(), [&](auto init, auto idx) {
        auto diff = lhs[idx] - rhs[idx];
        return diff < init ? diff : init;
    });
}

template <typename T>
concept zip_ra_iterator = requires(T& t) {
    t.foreach([](auto& i) {
        i += 1;
        i + 1;
        i -= 1;
        i - 1;
        i - i;
    });
} && zip_bidir_iterator<T>;

template <typename... Ts>
struct zip_iterator_ra : zip_iterator_bidir<Ts...> {
    using zip_iterator_bidir<Ts...>::zip_iterator_bidir;

    auto& operator+=(this auto& it, diff_t num) {
        it.iterators.foreach([num](auto& i) { i+= num; });
        return it;;
    }

    auto& operator-=(this auto& it, diff_t num) {
        it.iterators.foreach([num](auto& i) { i-= num; });
        return it;;
    }

    auto operator+(this const auto& it, diff_t num) {
        auto res = it;
        res += num;
        return res;
    }

    auto operator-(this const auto& it, diff_t num) {
        auto res = it;
        res -= num;
        return res;
    }

    diff_t operator-(const zip_iterator_ra& iterator) const {
        return zip_difference(this->iterators, iterator.iterators);
    }
};

template <typename... Ts>
struct zip_iterator : zip_iterator_fwd<Ts...> {
    using zip_iterator_fwd<Ts...>::zip_iterator_fwd;
};

template <typename... Ts> requires zip_bidir_iterator<tuple<Ts...>>
struct zip_iterator<Ts...> : zip_iterator_bidir<Ts...> {
    using zip_iterator_bidir<Ts...>::zip_iterator_bidir;
};

template <typename... Ts> requires zip_ra_iterator<tuple<Ts...>>
struct zip_iterator<Ts...> : zip_iterator_ra<Ts...> {
    using zip_iterator_ra<Ts...>::zip_iterator_ra;
};

template <typename T>
struct zip_container_type {
    using type = T&;
};

template <typename T>
struct zip_container_type<T&&> {
    using type = T;
};

template <typename... Ts>
struct zip {
    zip(Ts... icontainers):
        containers{icontainers...},
        begins(containers.map([](auto& v) { return core::begin(v); })),
        ends(containers.map([](auto& v) { return core::end(v); })) {}

    auto& begin() const {
        return begins;
    }

    auto& end() const {
        return ends;
    }

    tuple<Ts...>                                                  containers;
    zip_iterator<decltype(core::begin(declval<Ts&>()))...>        begins;
    zip_iterator_sentinel<decltype(core::end(declval<Ts&>()))...> ends;
};

template <typename... Ts>
zip(Ts&&...) -> zip<typename zip_container_type<Ts&&>::type...>;

template <typename... Ts>
struct zip_view {
    zip_view(const Ts&... containers):
        containers{containers...}, begins{core::begin(containers)...}, ends{core::end(containers)...} {}

    auto& begin() const {
        return begins;
    }

    auto& end() const {
        return ends;
    }

    tuple<const Ts&...>                                                 containers;
    zip_iterator<decltype(core::begin(declval<const Ts&>()))...>        begins;
    zip_iterator_sentinel<decltype(core::end(declval<const Ts&>()))...> ends;
};
} // namespace core

#undef fwd
