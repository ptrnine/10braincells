#pragma once

#include <net/ip_addr.hpp>

namespace core::net
{
class ip_addr_any {
public:
    ip_addr_any(const in6_addr& address): addr(address) {}
    ip_addr_any(const in_addr& address): ipv4(true) {
        std::memcpy(&addr, &address, sizeof(address));
    }

    [[nodiscard]]
    std::string to_string() const {
        socklen_t len = INET6_ADDRSTRLEN;
        sa_family_t fam = AF_INET6;
        if (ipv4) {
            len = INET_ADDRSTRLEN;
            fam = AF_INET;
        }

        std::string res(len, ' ');
        inet_ntop(fam, &addr, res.data(), len);
        res.resize(size_t(::strchr(res.data(), '\0') - res.data()));
        return res;
    }

    auto operator<=>(const ip_addr_any& ip) const noexcept {
        return ::memcmp(&addr, &ip.addr, sizeof(addr));
    }

    [[nodiscard]]
    sa_family_t family() const noexcept {
        return ipv4 ? AF_INET : AF_INET6;
    }

    [[nodiscard]]
    bool is_v4() const noexcept {
        return ipv4;
    }

    [[nodiscard]]
    bool is_v6() const noexcept {
        return !ipv4;
    }

    [[nodiscard]]
    ip_addr<ip_ver::v4> as_v4() const noexcept {
        return {*(const in_addr_t*)&addr};
    }

    [[nodiscard]]
    ip_addr<ip_ver::v6> as_v6() const noexcept {
        return {addr};
    }

private:
    in6_addr addr = IN6ADDR_ANY_INIT;
    bool ipv4 = false;
};
} // namespace core::net
