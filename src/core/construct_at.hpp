#pragma once

#include <memory>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core
{
template <typename T>
constexpr auto construct_at(T* location, auto&&... args) {
    /* ::new((void*)) T(...) failed in constexpr context */
    return std::construct_at(location, fwd(args)...);
}
} // namespace core

#undef fwd
