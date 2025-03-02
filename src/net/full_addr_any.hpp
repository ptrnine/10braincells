#pragma once

#include <net/full_addr.hpp>
#include <net/ip_addr_any.hpp>

namespace core::net {
class full_addr_any {
public:
    full_addr_any(const sockaddr_storage& native_addr): addr(native_addr) {}

    full_addr_any(const sockaddr_in& native_addr) {
        std::memcpy(&addr, &native_addr, sizeof(native_addr));
    }

    full_addr_any(const sockaddr_in6& native_addr) {
        std::memcpy(&addr, &native_addr, sizeof(native_addr));
    }

    template <ip_ver ver>
    full_addr_any(const full_addr<ver>& address) {
        std::memcpy(&addr, &address.native(), sizeof(address.native()));
    }

    full_addr_any(ip_addr_any ip_addr, port_t iport) {
        set(ip_addr, iport);
    }

    auto operator<=>(const full_addr_any& ip) const noexcept {
        return ::memcmp(&addr, &ip.addr, sizeof(addr));
    }

    [[nodiscard]]
    const sockaddr_storage& native() const noexcept {
        return addr;
    }

    [[nodiscard]]
    std::string to_string() const {
        if (is_v6())
            return '[' + ip().to_string() + "]:" + std::to_string(uint32_t(port()));
        else
            return ip().to_string() + ':' + std::to_string(uint32_t(port()));
    }

    [[nodiscard]]
    ip_addr_any ip() const noexcept {
        if (is_v4())
            return {((const sockaddr_in*)&addr)->sin_addr};
        else
            return {((const sockaddr_in6*)&addr)->sin6_addr};
    }

    [[nodiscard]]
    port_t port() const noexcept {
        if (is_v4())
            return ntohs(((const sockaddr_in*)&addr)->sin_port);
        else
            return ntohs(((const sockaddr_in6*)&addr)->sin6_port);
    }

    [[nodiscard]]
    sa_family_t family() const noexcept {
        return addr.ss_family;
    }

    [[nodiscard]]
    bool is_v4() const noexcept {
        return addr.ss_family == AF_INET;
    }

    [[nodiscard]]
    bool is_v6() const noexcept {
        return addr.ss_family == AF_INET6;
    }

    [[nodiscard]]
    full_addr<ip_ver::v4> as_v4() const noexcept {
        return {*(const sockaddr_in*)&addr};
    }

    [[nodiscard]]
    full_addr<ip_ver::v6> as_v6() const noexcept {
        return {*(const sockaddr_in6*)&addr};
    }

private:
    void set(const ip_addr_any& ip, port_t port) noexcept {
        if (ip.is_v4()) {
            sockaddr_in a{
                .sin_family = AF_INET,
                .sin_port = htons(port),
                .sin_addr = {.s_addr = ip.as_v4().native()},
                .sin_zero = {0},
            };
            std::memcpy(&addr, &a, sizeof(a));
        } else {
            sockaddr_in6 a{
                .sin6_family = AF_INET6,
                .sin6_port = htons(port),
                .sin6_flowinfo = 0,
                .sin6_addr = ip.as_v6().native(),
                .sin6_scope_id = 0,
            };
            std::memcpy(&addr, &a, sizeof(a));
        }
    }

    sockaddr_storage addr;
};
} // namespace core::net
