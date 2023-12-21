#pragma once

#include "remove_cv.hpp"

namespace core {
namespace dtls {
    template <typename>
    inline constexpr bool is_pointer = false;
    template <typename T>
    inline constexpr bool is_pointer<T*> = true;
} // namespace details_traits

template <typename T>
inline constexpr auto is_ptr = dtls::is_pointer<remove_cv<T>>;
} // namespace core
