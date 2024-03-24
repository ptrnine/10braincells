#pragma once

namespace core
{
struct null_t {
    constexpr operator decltype(nullptr)() const noexcept {
        return nullptr;
    }
};

static inline constexpr null_t null{};
} // namespace core
