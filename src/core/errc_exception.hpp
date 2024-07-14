#pragma once

#include <core/errc.hpp>
#include <core/exception.hpp>

namespace core
{
class errc_exception : public exception {
public:
    errc_exception(errc error): _error(error), _message(error.info()) {}

    const char* what() const noexcept override {
        return _message.data();
    }

    const errc& error() const {
        return _error;
    }

private:
    errc        _error;
    std::string _message;
};
} // namespace core
