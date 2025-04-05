#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <core/finalizer.hpp>
#include <net/net_error.hpp>
#include <net/full_addr_any.hpp>

namespace core::net
{
class socket_create_error : public net_error {
public:
    socket_create_error(int errc) : net_error("Cannot create socket: errc=" + std::to_string(errc)) {}
};

class socket_set_blocking_error : public net_error {
public:
    socket_set_blocking_error(int errc): net_error("Cannot setup blocking for socket: errc=" + std::to_string(errc)) {}
};

class address_already_in_use : public net_error {
public:
    address_already_in_use(const std::string& address): net_error("Socket " + address + " already in use") {}
};

class socket_bind_error : public net_error {
public:
    socket_bind_error(int errc): net_error("Cannot bind socket: errno=" + std::to_string(errc)) {}
};

inline std::string throw_socket_bind_exception(const full_addr_any& addr, int sys_error) {
    switch (sys_error) {
        case EADDRINUSE:
            throw address_already_in_use(addr.to_string());
        default:
            throw socket_bind_error(errno);
    }
}

class udp_socket {
public:
    udp_socket(const full_addr_any& address, bool blocking):
        fd(::socket(address.family(), SOCK_DGRAM, 0)), addr(address.native()) {
        if (fd == -1)
            throw socket_create_error(errno);

        finalizer scope_exit{[this] {
            ::close(fd);
        }};

        set_blocking(blocking);

        if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
            throw_socket_bind_exception(address, errno);

        scope_exit.dismiss();
    }

    ~udp_socket() {
        if (fd > 0)
            ::close(fd);
    }

    udp_socket(udp_socket&& sock) noexcept: fd(sock.fd), addr(sock.addr) {
        sock.fd = -1;
    }

    udp_socket& operator=(udp_socket&& sock) noexcept {
        if (this == &sock)
            return *this;

        if (fd > 0)
            ::close(fd);

        fd = sock.fd;
        addr = sock.addr;
        sock.fd = -1;

        return *this;
    }

    void set_blocking(bool block) {
        auto flags = fcntl(fd, F_GETFL);
        flags = block ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        if (fcntl(fd, F_SETFL, flags) == -1)
            throw socket_set_blocking_error(errno);
    }

    [[nodiscard]]
    full_addr_any address() const noexcept {
        return {addr};
    }

    ssize_t send(const full_addr_any& destination, const void* buf, size_t size) const noexcept {
        return ::sendto(fd, buf, size, 0, (const sockaddr*)&destination.native(), sizeof(destination.native()));
    }

    template <typename T>
        requires requires(const T& v) {
            { v.size() } -> std::convertible_to<size_t>;
            { v.data() } -> std::convertible_to<const void*>;
        }
    ssize_t send(const full_addr_any& destination, const T& data) const noexcept {
        return ::sendto(fd,
                        data.data(),
                        data.size() * sizeof(*data.data()),
                        0,
                        (const sockaddr*)&destination.native(),
                        sizeof(destination.native()));
    }

    //ssize_t send(const full_addr_any& destination, const transferable auto&& transferable_obj) const noexcept {
    //    return send(destination, transferable_obj.as_bytes());
    //}

    struct recv_result {
        full_addr_any src;
        void* buf;
        ssize_t size;
    };

    recv_result recv(void* buf, size_t max_size) const noexcept {
        sockaddr_storage addr;
        socklen_t socklen;
        auto size = recvfrom(fd, buf, max_size, 0, (struct sockaddr*)&addr, &socklen);
        return {.src = {addr}, .buf = buf, .size = size};
    }

    [[nodiscard]]
    int native() const noexcept {
        return fd;
    }

private:
    int fd;
    sockaddr_storage addr;
};
} // namespace core::net
