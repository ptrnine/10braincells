#pragma once

#include <string>

#include <grx/vk/types.hpp>

namespace vk {
struct version {
    u16 major;
    u16 minor;
    u16 patch;
};

struct version_raw {
    version_raw() = default;
    version_raw(u16 maj, u16 min, u16 ipatch): patch(ipatch), minor(min), major(maj), variant(0) {}

    u32 patch : 12;
    u32 minor : 10;
    u32 major : 7;
    u32 variant : 3;

    constexpr version get() const {
        return {u16(major), u16(minor), u16(patch)};
    }
};


inline std::string to_string(const version& ver) {
    std::string res;
    res += std::to_string(ver.major);
    res += '.';
    res += std::to_string(ver.minor);
    res += '.';
    res += std::to_string(ver.patch);
    return res;
}

inline std::string to_string(const version_raw& ver) {
    return to_string(ver.get());
}
} // namespace vk
