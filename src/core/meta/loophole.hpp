#include <core/meta/type.hpp>

namespace core::lh {
namespace details {
    template <typename Tag, size_t k>
    struct decl {
        friend auto get(decl<Tag, k>);
    };

    template <typename Tag, size_t k, auto r = 0>
    struct def {
        friend auto get(decl<Tag, k>) {
            return r;
        }
    };

    template <typename Tag, size_t key, size_t N, typename = decltype([] {})>
    constexpr auto find() {
        if constexpr (requires { get(decl<Tag, key>{}); })
            return find<Tag, key + N, N>();
        else
            return key;
    }
} // namespace details

#define fnd(MAX, ...) details::find<Tag, __VA_ARGS__, MAX, u>() - MAX
template <typename Tag, typename u = decltype([] {})>
constexpr auto count() {
    constexpr auto c = details::find<Tag, fnd(4, fnd(8, fnd(16, fnd(32, fnd(64, fnd(128, 252)))))), 1, u>();
    return c;
}
#undef fnd

template <typename Tag, typename u = decltype([] {})>
constexpr auto count_and_next() {
    constexpr auto c = count<Tag, u>();
    static_assert(!requires { details::def<Tag, c>::get; });
    return c;
}

template <typename Tag, size_t Idx>
struct decl_type_idx {
    constexpr friend auto get_type(decl_type_idx<Tag, Idx>);
};

template <typename Tag, size_t Idx, typename T = void>
struct def_type_idx {
    constexpr friend auto get_type(decl_type_idx<Tag, Idx>) {
        return type<T>;
    }
};

enum class type_not_found {};

template <typename Tag, size_t Idx>
constexpr auto get_type_from_idx() {
    using namespace details;
    if constexpr (requires { get_type(decl_type_idx<Tag, Idx>{}); })
        return get_type(decl_type_idx<Tag, Idx>{});
    else
        return type<type_not_found>;
}
} // namespace core::lh

namespace core {
template <typename Tag, size_t idx, typename Derived>
struct closed_set_type {
    static_assert(!requires { !lh::def_type_idx<Tag, idx, Derived>::get_type; });

    using cst_tag = Tag;
    using cst     = closed_set_type;

    static inline constexpr size_t cst_idx = idx;
};
} // namespace core
