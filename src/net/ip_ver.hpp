#pragma once

#include <sys/socket.h>

namespace core::net {
enum class ip_ver {
    v4 = 4,
    v6 = 6,
};

inline constexpr int af_from_ipver(ip_ver ver) {
    switch (ver) {
    case ip_ver::v4: return AF_INET;
    case ip_ver::v6: return AF_INET6;
    }
    __builtin_unreachable();
}
} // namespace core::net
