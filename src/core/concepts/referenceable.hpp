#pragma once

namespace core {
namespace details_traits {
    template <typename T>
    using add_ref = T&;
} // namespace details_traits

template <typename T>
concept referenceable = requires { typename details_traits::add_ref<T>; };
} // namespace core
