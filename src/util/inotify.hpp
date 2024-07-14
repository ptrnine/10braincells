#pragma once

#include <filesystem>
#include <list>
#include <poll.h>
#include <sys/inotify.h>

#include <functional>
#include <sys/poll.h>
#include <thread>

#include <core/exception.hpp>
#include <core/errc.hpp>
#include <util/log.hpp>

namespace util {

class inotify_exception : public core::exception {};

class inotify_init_failed : inotify_exception {
public:
    inotify_init_failed(const std::string message): err(core::errc::from_errno()), msg(message + ": " + err.info()) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    core::errc err;
    std::string msg;
};

class inotify_not_a_directory : public inotify_exception {
public:
    inotify_not_a_directory(const std::string& path): msg("Not a directory: " + path) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

/* TODO: decompose into poll and inotify */
class inotify_watcher {
public:
    inotify_watcher(bool start_worker = true): ino_fd(inotify_init()) {
        if (ino_fd == -1)
            throw inotify_init_failed("inotify_init() failed");

        if (pipe(pipefd) == -1)
            throw inotify_init_failed("pipe() failed");

        if (start_worker)
            start();
    }

    ~inotify_watcher() {
        stop();

        close(pipefd[1]);
        close(pipefd[0]);
        close(ino_fd);
    }

    void start() {
        if (running)
            return;

        t = std::jthread(&inotify_watcher::worker, this);
        running = true;
    }

    void stop() {
        if (!running)
            return;

        write(pipefd[1], "\n", 2);
        t.join();
        running = false;
    }

    void add_handler(const std::string& path, auto&& handler) {
        auto canonical = std::filesystem::weakly_canonical(path).string();
        if (!std::filesystem::is_directory(canonical))
            throw inotify_not_a_directory(canonical);

        std::lock_guard lock{mtx};
        auto [pos, inserted] = dir_wd_map.emplace(canonical, -1);

        int& wd = pos->second;
        if (inserted) {
            wd = inotify_add_watch(ino_fd, canonical.data(), IN_MODIFY);
            glog().debug("[inotify_watcher] add directory \"{}\"", canonical);
        }

        auto [handler_pos, _] = wd_map.emplace(wd, dir_handler{});
        handler_pos->second.dir_path = canonical;
        handler_pos->second.handlers.push_back(static_cast<decltype(handler)>(handler));
    }

private:
    void worker() {
        glog().debug("[inotify_watcher] started");

        struct pollfd pevt[2] = {
            {.fd = ino_fd, .events = POLLIN, .revents = {}},
            {.fd = pipefd[0], .events = POLLIN, .revents = {}},
        };
        char inotify_buff[8192];

        while (true) {
            if (poll(pevt, std::size(pevt), -1) < 0) {
                glog().error("inotify_watcher: poll() failed with: {}", core::errc::from_errno().info());
                break;
            }

            if (pevt[0].revents & POLLIN) {
                auto len = read(ino_fd, inotify_buff, sizeof(inotify_buff));
                if (len > 0) {
                    for (auto p = inotify_buff, e = inotify_buff + len; p < e;) {
                        struct inotify_event evt;
                        memcpy(&evt, p, sizeof(evt));
                        if (evt.len == 0)
                            break;

                        std::lock_guard lock{mtx};
                        auto pos = wd_map.find(evt.wd);
                        if (pos != wd_map.end()) {
                            auto dir = pos->second.dir_path;
                            auto filename = p + sizeof(evt);
                            auto path = dir + "/" + filename;
                            auto& handlers = pos->second.handlers;

                            for (auto i = handlers.begin(); i != handlers.end();) {
                                auto continue_watch = (*i)(path);
                                if (continue_watch)
                                    ++i;
                                else
                                    handlers.erase(i++);
                            }

                            if (handlers.empty()) {
                                inotify_rm_watch(ino_fd, evt.wd);
                                wd_map.erase(pos);
                                dir_wd_map.erase(dir);
                                glog().debug("[inotify_watcher] remove directory \"{}\"", dir);
                            }
                        }

                        p += evt.len;
                    }
                }
            }

            if (pevt[1].revents & POLLIN) {
                /* Stop thread */
                break;
            }
        }

        glog().debug("[inotify_watcher] stopped");
    }

private:
    int  ino_fd = -1;
    int  pipefd[2];
    bool running = false;

    struct dir_handler {
        std::string dir_path;
        std::list<std::function<bool(const std::string&)>> handlers;
    };

    std::map<int, dir_handler> wd_map;
    std::map<std::string, int> dir_wd_map;
    mutable std::mutex         mtx;
    std::jthread               t;
};
} // namespace util
