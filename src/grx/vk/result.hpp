#pragma once

#include <core/traits/remove_cvref.hpp>
#include <core/traits/invoke.hpp>

#include <grx/vk/enums.cg.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace vk {
class result_error : public core::exception {
public:
    result_error(vk::result irc): rc(irc) {}

    const char* what() const noexcept override {
        return to_string(rc).data();
    }

private:
    vk::result rc;
};

template <typename T, auto success_codes, auto error_codes>
struct [[nodiscard("Result should be processed")]] result_t {
    constexpr bool ok() const {
        for (auto code : success_codes)
            if (code == rc)
                return true;
        return false;
    }

    constexpr auto&& operator*(this auto&& it) {
        return fwd(it).value_unsafe;
    }

    constexpr auto operator->(this auto&& it) {
        return &fwd(it).value_unsafe;
    }

    constexpr auto&& value(this auto&& it) {
        if (!it.ok())
            throw result_error(it.rc);
        return fwd(it).value_unsafe;
    }

    explicit constexpr operator bool() const {
        return ok();
    }

    constexpr auto map(this auto&& it, auto&& callback) {
        using t = core::invoke_result<core::remove_cvref<decltype(callback)>, decltype(fwd(it).value_unsafe)>;
        result_t<t, success_codes, error_codes> res;
        res.rc = it.rc;

        if (res.ok())
            res.value_unsafe = callback(fwd(it).value_unsafe);

        return res;
    }

    constexpr auto&& checked(this auto&& it) {
        if (!it.ok())
            throw result_error(it.rc);
        return fwd(it);
    }

    static constexpr auto ok_codes() {
        return success_codes;
    }

    static constexpr auto bad_codes() {
        return error_codes;
    }

    T          value_unsafe;
    vk::result rc;
};

template <auto success_codes, auto error_codes>
struct [[nodiscard("Result should be processed")]] result_t<void, success_codes, error_codes> {
    constexpr bool ok() const {
        for (auto code : success_codes)
            if (code == rc)
                return true;
        return false;
    }

    constexpr void throws() const {
        if (!ok())
            throw result_error(rc);
    }

    constexpr void value() const {
        if (!ok())
            throw result_error(rc);
    }

    constexpr auto&& checked(this auto&& it) {
        if (!it.ok())
            throw result_error(it.rc);
        return fwd(it);
    }

    explicit constexpr operator bool() const {
        return ok();
    }

    static constexpr auto ok_codes() {
        return success_codes;
    }

    static constexpr auto bad_codes() {
        return error_codes;
    }

    vk::result rc;
};
} // namespace vk

#undef fwd
