#pragma once

#include <exception>
#include <string>

namespace core::net {
class net_error : public std::exception {
public:
    net_error(std::string message): msg(std::move(message)) {}

    [[nodiscard]]
    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};
} // namespace core::net
