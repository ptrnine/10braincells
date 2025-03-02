#pragma once

#include <string>

#include <core/concepts/string.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {
namespace details {
    template <char_type C, typename Traits, typename Alloc>
    void string_append(std::basic_string<C, Traits, Alloc> & dst, const sized_str_typed<C> auto& str) {
        dst.append(str.data(), str.size());
    }

    template <char_type C, typename Traits, typename Alloc, size_t N>
    void string_append(std::basic_string<C, Traits, Alloc>& dst, const C(&str)[N]) {
        dst.append(str, N - 1);
    }

    template <char_type C, typename Traits, typename Alloc>
    void string_append(std::basic_string<C, Traits, Alloc>& dst, C c) {
        dst.push_back(c);
    }
}

std::string build_str(auto&&... strs) {
    std::string res;
    (details::string_append(res, fwd(strs)), ...);
    return res;
}

template <char_type C, typename Traits, typename Alloc>
void append_str(std::basic_string<C, Traits, Alloc>& res, auto&&... strs) {
    (details::string_append(res, fwd(strs)), ...);
}
} // namespace core

#undef fwd
