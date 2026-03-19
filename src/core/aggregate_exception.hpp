#pragma once

#include <exception>
#include <vector>
#include <string>

#include <core/utility/move.hpp>

namespace core {
class aggregate_exception : public std::exception {
public:
    void add_exception(std::exception_ptr eptr) {
        exceptions.push_back(mov(eptr));
    }

    size_t size() const noexcept {
        return exceptions.size();
    }

    const char* what() const noexcept override {
        if (cached_what.empty()) {
            build_what();
        }
        return cached_what.c_str();
    }

    std::exception_ptr operator[](size_t index) const {
        return exceptions[index];
    }

private:
    void build_what() const {
        std::string result = "aggregate_exception with " + std::to_string(exceptions.size()) + " errors:\n";
        for (size_t i = 0; i < exceptions.size(); ++i) {
            try {
                std::rethrow_exception(exceptions[i]);
            } catch (const std::exception& e) {
                result += "  " + std::to_string(i + 1) + ": " + e.what() + "\n";
            } catch (...) {
                result += "  " + std::to_string(i + 1) + ": <unknown exception>\n";
            }
        }
        cached_what = mov(result);
    }

    mutable std::string             cached_what;
    std::vector<std::exception_ptr> exceptions;
};

void throw_aggregate_exception(const aggregate_exception& exception) {
    if (exception.size() == 0) {
        return;
    }

    if (exception.size() == 1) {
        throw exception[0];
    }

    throw exception;
}
} // namespace core
