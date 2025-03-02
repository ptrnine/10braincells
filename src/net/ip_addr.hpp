#pragma once

#include <cstring>

#include <arpa/inet.h>
#include <netdb.h>

#include <net/ip_ver.hpp>
#include <net/net_error.hpp>

namespace core::net
{
using namespace std::string_literals;
using namespace std::string_view_literals;

class invalid_ip_address : public net_error {
public:
    invalid_ip_address(const char* s): net_error("Ip address "s + s + " is invalid") {}
};

template <ip_ver>
class ip_addr;

template <>
class ip_addr<ip_ver::v4> {
public:
    ip_addr(in_addr_t address): addr(address) {}

    ip_addr(const std::string& address) {
        setup(address);
    }

    static auto localhost() {
        return ip_addr{"localhost"};
    }

    static auto broadcast() {
        return ip_addr{"255.255.255.255"};
    }

    static auto any() {
        return ip_addr{"0.0.0.0"};
    }

    [[nodiscard]]
    std::string to_string() const {
        std::string res(INET_ADDRSTRLEN, '\0');
        inet_ntop(AF_INET, &addr, res.data(), INET_ADDRSTRLEN);
        res.resize(size_t(::strchr(res.data(), '\0') - res.data()));
        return res;
    }

    [[nodiscard]]
    const in_addr_t& native() const {
        return addr;
    }

    auto operator<=>(const ip_addr& ip) const noexcept = default;

private:
    void setup(const std::string& address) {
        if (address == "0.0.0.0"sv)
            addr = INADDR_ANY;
        else if (address == "255.255.255.255"sv)
            addr = INADDR_BROADCAST;
        else if (inet_pton(AF_INET, address.data(), &addr) != 1) {
            addrinfo hints{};
            hints.ai_family = AF_INET;
            addrinfo* result = nullptr;

            if (getaddrinfo(address.data(), nullptr, &hints, &result) != 0 || !result)
                throw invalid_ip_address(address.data());

            addr = reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr.s_addr; // NOLINT
            freeaddrinfo(result);
        }
    }

private:
    in_addr_t addr = INADDR_ANY;
};

template <>
class ip_addr<ip_ver::v6> {
public:
    ip_addr(const in6_addr& address): addr(address) {}

    ip_addr(const std::string& address) {
        setup(address);
    }

    static auto localhost() {
        return ip_addr{"localhost"};
    }

    static auto any() {
        return ip_addr{"::"};
    }

    [[nodiscard]]
    std::string to_string() const {
        std::string res(INET6_ADDRSTRLEN, ' ');
        inet_ntop(AF_INET6, &addr, res.data(), INET6_ADDRSTRLEN);
        res.resize(size_t(::strchr(res.data(), '\0') - res.data()));
        return res;
    }

    [[nodiscard]]
    const in6_addr& native() const {
        return addr;
    }

    auto operator<=>(const ip_addr& ip) const noexcept {
        return ::memcmp(&addr, &ip.addr, sizeof(addr));
    }

private:
    void setup(const std::string& address) {
        if (address == "::"sv)
            return;
        else if (inet_pton(AF_INET6, address.data(), &addr) != 1) {
            addrinfo hints{};
            hints.ai_family = AF_INET6;
            addrinfo* result = nullptr;

            if (getaddrinfo(address.data(), nullptr, &hints, &result) != 0 || !result)
                throw invalid_ip_address(address.data());

            addr = reinterpret_cast<sockaddr_in6*>(result->ai_addr)->sin6_addr; // NOLINT
            freeaddrinfo(result);
        }
    }

private:
    in6_addr addr = IN6ADDR_ANY_INIT;
};

using ip_addr_v4 = ip_addr<ip_ver::v4>;
using ip_addr_v6 = ip_addr<ip_ver::v6>;

namespace literals
{
    inline ip_addr_v4 operator"" _ipv4(const char* str, size_t len) {
        return {std::string(str, len)};
    }

    inline ip_addr_v6 operator"" _ipv6(const char* str, size_t len) {
        return {std::string(str, len)};
    }
} // namespace literals
} // namespace core::net
