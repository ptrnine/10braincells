#pragma once

#include <map>
#include <mutex>
#include <shared_mutex>

#include <core/box.hpp>
#include <core/compact_hashes.hpp>

#include <util/log/log_handler_fd.hpp>
#include <util/print.hpp>
#include <util/time.hpp>

namespace util {

class logger {
public:
    static inline constexpr std::string_view time_format = "[hh:mm:ss.xxx]";

    logger(bool init_stdout = true) {
        if (init_stdout)
            add_handler("stdout", sys::stdout_fd);
    }

    ~logger() {
        info("******* log close {} *******\n", handler_names());
    }

    void add_handler(const std::string& name, core::box<log_handler_base> log_handler) {
        {
            std::unique_lock lock{mtx};
            handlers.insert_or_assign(name, std::move(log_handler));
        }
        log_to_handler(name, log_level::info, "******* handler [{}] attached *******", name);
    }

    void add_handler(const std::string& name, core::convertible_to<sys::fd_t> auto ofd) {
        {
            std::unique_lock lock{mtx};
            handlers.insert_or_assign(name, create_log_handler(core::mov(ofd)));
        }
        log_to_handler(name, log_level::info, "******* handler [{}] attached *******", name);
    }

    void remove_handler(const std::string& name) {
        if (auto handler = take_handler(name)) {
            log_to_handler(*handler, log_level::info, "******* handler [{}] detached *******", name);
        }
    }

    core::box<log_handler_base> take_handler(const std::string& name) {
        std::unique_lock lock{mtx};
        auto             found = handlers.find(name);
        if (found != handlers.end()) {
            auto res = std::move(found->second);
            handlers.erase(found);
            return res;
        }
        return {};
    }

    std::vector<std::string> handler_names() const {
        std::unique_lock lock{mtx};

        std::vector<std::string> result;
        result.reserve(handlers.size());

        for (auto&& [name, _] : handlers)
            result.push_back(name);

        return result;
    }

    template <typename... Ts>
    static void log_to_handler(log_handler_base& handler, log_level level, std::string_view format_str, Ts&&... args) {
        auto msg  = util::format(format_str, std::forward<Ts>(args)...);
        auto hash = core::fnv1a64(msg.data(), msg.size());
        auto time = current_datetime(time_format);
        handler.write(level, time, msg, hash);
    }

    template <typename... Ts>
    void log_to_handler(const std::string& handler_name, log_level level, std::string_view format_str, Ts&&... args) {
        if (!check_level(level))
            return;

        auto msg  = util::format(format_str, std::forward<Ts>(args)...);
        auto hash = core::fnv1a64(msg.data(), msg.size());
        auto time = current_datetime(time_format);

        std::shared_lock lock{mtx};
        auto             found = handlers.find(handler_name);
        if (found != handlers.end())
            found->second->write(level, time, msg, hash);
    }

    template <typename... Ts>
    void log(log_level level, std::string_view format_str, Ts&&... args) {
        if (!check_level(level))
            return;

        auto msg  = util::format(format_str, std::forward<Ts>(args)...);
        auto hash = core::fnv1a64(msg.data(), msg.size());
        auto time = current_datetime(time_format);

        std::shared_lock lock{mtx};
        for (auto& [_, handler] : handlers)
            handler->write(level, time, msg, hash);
    }

    template <typename... Ts>
    void log_update(log_level level, u16 update_id, std::string_view format_str, Ts&&... args) {
        if (!check_level(level))
            return;

        auto msg  = util::format(format_str, std::forward<Ts>(args)...);
        auto time = current_datetime(time_format);

        std::shared_lock lock{mtx};
        for (auto& [_, handler] : handlers)
            handler->write_update(update_id, level, time, msg);
    }

    void set_level(log_level value) {
        level = value;
    }

    log_level get_level() const {
        return level;
    }

#define def_log_func(level)                                                             \
    template <typename... Ts>                                                           \
    void level(std::string_view format_str, Ts&&... args) {                             \
        log(log_level::level, format_str, std::forward<Ts>(args)...);                   \
    }                                                                                   \
    template <typename... Ts>                                                           \
    void level##_update(u16 update_id, std::string_view format_str, Ts&&... args) {     \
        log_update(log_level::level, update_id, format_str, std::forward<Ts>(args)...); \
    }

    def_log_func(debug)
    def_log_func(detail)
    def_log_func(info)
    def_log_func(warn)
    def_log_func(error)
#undef def_log_func

    private : bool check_level(log_level target_level) {
        return target_level >= level;
    }

    std::map<std::string, core::box<log_handler_base>> handlers;
    mutable std::shared_mutex                          mtx;
    log_level                                          level = log_level::debug;
};
} // namespace util
