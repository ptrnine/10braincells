#pragma once

#include <string>

#include <core/meta/type.hpp>
#include <core/opt.hpp>
#include <core/string_builder.hpp>
#include <core/traits/is_enum.hpp>
#include <core/utility/idx_dispatch.hpp>
#include <core/utility/int_seq.hpp>

namespace core
{
namespace details
{
    template <typename T>
    concept enum_ct = is_enum<T>;
}

template <details::enum_ct auto E>
constexpr bool is_valid_enum() {
    std::string_view pf = __PRETTY_FUNCTION__;
    auto p1 = pf.find("E = ");
    return pf[p1 + 4] != '(';
}

template <details::enum_ct E>
constexpr std::string_view enum_type_name() {
    std::string_view pf = __PRETTY_FUNCTION__;
    auto p = pf.find("E = ");
    auto s = p + 4;
    auto e = pf.find(';', s);
    return pf.substr(s, e - s);
}

template <details::enum_ct auto E> requires (is_valid_enum<E>())
constexpr std::string_view enum_to_string() {
    std::string_view pf = __PRETTY_FUNCTION__;
    auto p = pf.find("E = ");
    auto s = p + 4;
    auto e = pf.find(';', s);
    s = pf.rfind(':', e);
    ++s;
    return pf.substr(s, e - s);
}

class invalid_enum : public exception {
public:
    template <details::enum_ct E>
    invalid_enum(type_t<E>, std::string_view enum_value):
        msg(build_str("Invalid enum string: ", enum_type_name<E>(), "::", enum_value)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

template <details::enum_ct E, ssize_t Min = 0, ssize_t Max = 16>
constexpr E string_to_enum(std::string_view str) {
    std::string_view pf = __PRETTY_FUNCTION__;

    for (size_t i = 0; i < size_t(Max - Min); ++i) {
        auto res = idx_dispatch<size_t(Max - Min)>(i, [&](auto num) -> opt<E> {
            constexpr size_t i = num + int_c<Min>;
            if constexpr (is_valid_enum<E{i}>()) {
                if (enum_to_string<E{i}>() == str) {
                    return E{i};
                }
            }
            return null;
        });
        if (res)
            return *res;
    }

    throw invalid_enum(type<E>, str);
}
} // namespace core
