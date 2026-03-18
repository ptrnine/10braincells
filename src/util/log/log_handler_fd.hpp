#pragma once

#include <core/box.hpp>
#include <core/io/helpers.hpp>

#include <util/log/log_handler_base.hpp>
#include <util/using_std.hpp>

#include <sys/write.hpp>

namespace util {
template <core::convertible_to<sys::fd_t> Fd>
class log_handler_fd : public log_handler_base {
public:
    /* TODO: replace isatty */
    log_handler_fd(Fd out): ofd(core::mov(out)), is_fifo(core::io::is_pipe_like(ofd)), is_tty(isatty((int)sys::fd_t(out))) {}

    void write_handler(log_level level, write_type wt, std::string_view time, std::string_view msg, u64 times) final {
        static constexpr std::string_view level_str[] = {
            ": [debug] "sv,
            ": "sv,
            ": [info] "sv,
            ": [warn] "sv,
            ": [error] "sv,
        };
        static constexpr std::string_view level_color[] = {
            "\033[0;38;5;12m",
            "\033[0;38;5;7m",
            "\033[0;38;5;10m",
            "\033[0;38;5;11m",
            "\033[0;38;5;1m",
        };

        std::string record;

        if (wt != write_type::new_record && is_fifo)
            record += '\r';
        else
            record += '\n';

        /* Setup color for log level */
        if (is_tty)
            record += level_color[size_t(level)];

        record += time;

        /* Enable bold */
        if (is_tty)
            record += "\033[1m";

        if (times > 1 && wt == write_type::write_same) {
            if (times != std::numeric_limits<u16>::max()) {
                record += " (";
                record += std::to_string(times);
                record += " times)";
            } else {
                record += " (repeats infinitely)";
            }
        }
        record += level_str[size_t(level)];

        /* Disable bold */
        if (is_tty)
            record += "\033[22m";

        record += msg;

        /* Reset colors */
        if (is_tty)
            record += "\033[0m";

        /* XXX: racy */
        size_t prev_len = prev_record_len.exchange(record.size());
        if (wt != write_type::new_record && !is_fifo)
            sys::lseek(ofd, -off_t(prev_len), sys::seek_whence::cur);
        sys::write(ofd, record.data(), record.size());
    }

private:
    Fd                  ofd;
    const bool          is_fifo;
    const bool          is_tty;
    std::atomic<size_t> prev_record_len = 0;
};

template <core::convertible_to<sys::fd_t> Fd>
inline auto create_log_handler(Fd ofd) {
    return core::boxed<log_handler_fd<Fd>>(core::mov(ofd));
}
} // namespace util
