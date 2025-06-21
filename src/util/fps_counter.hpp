#pragma once

#include <chrono>

#include <util/basic_types.hpp>

namespace util {
class fps_counter {
public:
    using clock = std::chrono::steady_clock;

    fps_counter(size_t frames_count = 20): accumulator_max(frames_count) {}

    double calculate() {
        using std::chrono::duration;
        using std::chrono::duration_cast;

        if (accumulator++ == accumulator_max - 1) {
            auto now    = clock::now();
            auto dur    = duration_cast<duration<double, std::chrono::seconds::period>>(now - ns);
            ns          = now;
            fps         = static_cast<double>(accumulator_max) / dur.count();
            accumulator = 0;
        }

        return fps;
    }

    double get() const {
        return fps;
    }

private:
    double            fps             = 0;
    size_t            accumulator     = 0;
    size_t            accumulator_max = 10;
    clock::time_point ns              = clock::now();
};
} // namespace util
