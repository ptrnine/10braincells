#pragma once

#include <limits>

#include <net/ip_addr.hpp>

namespace core::net {
class invalid_address : public net_error {
public:
    invalid_address(const char* s): net_error("Address "s + s + " is invalid") {}
};

class invalid_port : public net_error {
public:
    invalid_port(const std::string& s): net_error("Port " + s + " is invalid") {}
};

using port_t = uint16_t;

template <ip_ver ver>
ip_addr<ver> parse_ip(const std::string& full_address) {
    if constexpr (ver == ip_ver::v6) {
        if (full_address.starts_with('[')) {
            auto e = full_address.find(']');
            if (e == full_address.npos)
                throw invalid_address(full_address.data());
            return {full_address.substr(1, e - 1)};
        }
        else {
            return {full_address};
        }
    }

    return {full_address.substr(0, full_address.find(':'))};
}

template <ip_ver ver>
class full_addr {
public:
    using ip_addr_t = ip_addr<ver>;
    using sockaddr_t = std::conditional_t<ver == ip_ver::v4, sockaddr_in, sockaddr_in6>;

    full_addr(const sockaddr_t& native_addr): addr(native_addr) {}

    full_addr(const std::string& address) {
        auto ip = parse_ip<ver>(address);

        auto delim_pos = address.find(ver == ip_ver::v4 ? ":" : "]:");
        if (delim_pos == std::string::npos)
            throw invalid_port("Missing port in address " + address);

        auto port_start = address.data() + delim_pos + (ver == ip_ver::v4 ? 1 : 2);
        auto port_end = address.data() + address.size();

        constexpr auto throw_invalid_port = [](auto port_start, auto port_end) {
            throw invalid_port(std::string(port_start, size_t(port_end - port_start)));
        };

        if (port_end - port_start > 5)
            throw_invalid_port(port_start, port_end);

        uint32_t port32 = 0;
        auto p = port_start;
        while (p != port_end) {
            if (*p < '0' || *p > '9')
                throw_invalid_port(port_start, port_end);
            port32 = port32 * 10 + uint32_t(*p++ - '0');
        }

        if (port32 > std::numeric_limits<port_t>::max())
            throw_invalid_port(port_start, port_end);

        auto port = port_t(port32);

        set(ip, port);
    }

    full_addr(ip_addr_t ip_addr, port_t iport) {
        set(ip_addr, iport);
    }

    auto operator<=>(const full_addr& ip) const noexcept = default;

    [[nodiscard]]
    const sockaddr_t& native() const noexcept {
        return addr;
    }

    [[nodiscard]]
    std::string to_string() const {
        if constexpr (ver == ip_ver::v6)
            return '[' + ip().to_string() + "]:" + std::to_string(uint32_t(port()));
        else
            return ip().to_string() + ':' + std::to_string(uint32_t(port()));
    }

    [[nodiscard]]
    ip_addr_t ip() const {
        if constexpr (ver == ip_ver::v4)
            return addr.sin_addr.s_addr;
        else
            return addr.sin6_addr;
    }

    [[nodiscard]]
    port_t port() const noexcept {
        if constexpr (ver == ip_ver::v4)
            return ntohs(addr.sin_port);
        else
            return ntohs(addr.sin6_port);
    }

private:
    void set(const ip_addr_t& ip, port_t port) {
        if constexpr (ver == ip_ver::v4)
            addr = sockaddr_in{
                .sin_family = AF_INET,
                .sin_port = htons(port),
                .sin_addr = {.s_addr = ip.native()},
                .sin_zero = {0},
            };
        else
            addr = sockaddr_in6{
                .sin6_family = AF_INET6,
                .sin6_port = htons(port),
                .sin6_flowinfo = 0,
                .sin6_addr = ip.native(),
                .sin6_scope_id = 0,
            };
    }

    sockaddr_t addr;
};

using full_addr_v4 = full_addr<ip_ver::v4>;
using full_addr_v6 = full_addr<ip_ver::v6>;

namespace literals
{
    inline full_addr_v4 operator"" _addr4(const char* str, size_t len) {
        return {std::string(str, len)};
    }

    inline full_addr_v6 operator"" _addr6(const char* str, size_t len) {
        return {std::string(str, len)};
    }
} // namespace literals
} // namespace core::net
