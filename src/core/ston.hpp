#pragma once

#include <charconv>

#include <core/concepts/pointer.hpp>
#include <core/concepts/string.hpp>
#include <core/errc.hpp>
#include <core/exception.hpp>

namespace core
{
class string_to_number_error : public exception {
public:
    string_to_number_error(errc ec): msg("string/number conversion failed: " + ec.info()) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

template <typename T, sized_const_str S, typename... BaseOrFmt>
    requires requires(T& res, const S& str, BaseOrFmt... fmt) {
        std::from_chars(str.data(), str.data() + str.size(), res, fmt...);
    }
T ston(const S& string, BaseOrFmt... base_or_format) {
    T    result;
    auto rc = std::from_chars(string.data(), string.data() + string.size(), result, base_or_format...);
    if (rc.ec != std::errc())
        throw string_to_number_error(rc.ec);
    return result;
}

template <typename T, typename S, typename... BaseOrFmt>
    requires(!sized_const_str<S>) && requires(T& res, const S& str, BaseOrFmt... fmt) {
        { str.begin() } -> pointer;
        { str.end() } -> pointer;
        { std::from_chars(str.begin(), str.end(), res, fmt...) };
    }
T ston(const S& string, BaseOrFmt... base_or_format) {
    T    result;
    auto rc = std::from_chars(string.begin(), string.end(), result, base_or_format...);
    if (rc.ec != std::errc())
        throw string_to_number_error(rc.ec);
    return result;
}

template <typename T, typename S, typename... BaseOrFmt>
    requires(!sized_const_str<S>) && requires(T& res, const S& str, BaseOrFmt... fmt) {
        { auto(str.begin().base()) } -> pointer;
        { auto(str.end().base()) } -> pointer;
        { std::from_chars(str.begin().base(), str.end().base(), res, fmt...) };
    }
T ston(const S& string, BaseOrFmt... base_or_format) {
    return ston<T>(std::span{string.begin().base(), string.end().base()}, base_or_format...);
}

template <typename T, typename... BaseOrFmt>
    requires requires(const char* str, BaseOrFmt... fmt) { ston<T>(std::string_view(str), fmt...); }
T ston(const char* string, BaseOrFmt... base_or_format) {
    return ston<T>(std::string_view(string), base_or_format...);
}
} // namespace core
