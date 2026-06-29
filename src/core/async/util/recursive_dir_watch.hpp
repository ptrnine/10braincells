#pragma once

#include <core/async/sys/cancel.hpp>
#include <core/limits.hpp>
#include <deque>
#include <filesystem>

#include <core/async/concurrent.hpp>
#include <core/async/util/dir_watch.hpp>
#include <core/ranges/zip.hpp>
#include <sys/inotify_watch_flags.hpp>
#include <sys/readdir.hpp>
#include <util/log.hpp>

namespace core::async::util {
namespace fs = std::filesystem;

class recursive_dir_watch {
public:
    enum class comp_rc {
        canceled = 0,
        deleted,
        child,
        events,
    };

    struct dir_component {
        dir_component() = default;
        explicit dir_component(fs::path ipath): path(core::mov(ipath)) {}

        dir_component(const dir_component&) = delete;
        dir_component& operator=(const dir_component&) = delete;
        dir_component(dir_component&&) = default;
        dir_component& operator=(dir_component&&) = default;

        fs::path                                  path;
        dir_watcher                               watcher;
        inotify_watch::event_poller               poller;
        function<task<tuple<size_t, bool>>(), 16> handler;
        cancelation_point_t                       cp;
    };

    recursive_dir_watch(const fs::path& root, fs::path watch_dir, sys::inotify_watch_flags flags = {}, u32 ring_size = 32):
        _root(fs::weakly_canonical(root)), _watch_dir(mov(watch_dir)), _flags(flags), _ring_size(ring_size) {
        if (_root.empty()) {
            _root = fs::current_path();
        }
        if (_root.filename().empty()) {
            _root = _root.parent_path();
        }

        fs::path component = _root;
        _components.emplace_back(component);

        for (auto&& p : _watch_dir) {
            if (p.filename().empty()) {
                break;
            }
            component /= p;
            _components.emplace_back(component);
        }

        //glog().debug("recursive_dir_watch: root={}", _root);
        //glog().debug("recursive_dir_watch: components:");
        //for (auto&& c : _components) {
        //    glog().debug("- {}", c.path);
        //}

        init();
    }

    task<opt<std::vector<wd_event>>> poll() {
        while (auto task = co_await _conc.select()) {
            auto [idx, cancel] = co_await *task;
            if (cancel) {
                auto& comp   = _components[idx];
                comp.poller  = {};
                comp.watcher = {};
                comp.cp      = {};
                comp.handler.reset();
            } else {
                auto events = co_await handle_component(idx);
                if (!events.empty()) {
                    co_return events;
                }
            }
        }

        co_return {};
    }

private:
    static inline constexpr size_t no_component = limits<size_t>::max();

    void init(size_t index = 0) {
        for (size_t i = index; i < _components.size(); ++i) {
            try {
                auto& component = _components[i];

                sys::inotify_watch_flags flags;
                if (i != _components.size() - 1) {
                    flags = sys::inotify_watch_flag::moved_from | sys::inotify_watch_flag::moved_to | sys::inotify_watch_flag::delete_ |
                            sys::inotify_watch_flag::create | sys::inotify_watch_flag::isdir;
                } else {
                    flags = _flags | sys::inotify_watch_flag::delete_self | sys::inotify_watch_flag::move_self;
                }

                component.watcher = dir_watcher{component.path, flags, _ring_size};
                component.poller  = component.watcher.poller();
                component.handler = [this, i] -> task<tuple<size_t, bool>> {
                    auto& comp = _components[i];
                    auto res = co_await comp.poller.wait();
                    if (!res.ok()) {
                        if (res.error() == errc::ecanceled) {
                            glog().warn("Canceled watch for {}", comp.path.string());
                            co_return tuple{i, true};
                        }
                        res.throw_if_error();
                    }
                    co_return tuple{i, false};
                };
                //glog().info("recursive_dir_watch::init(): {}", component.path.string());
            } catch (const errc_exception& e) {
                if (e.error() != errc::enoent) {
                    glog().warn("recursive_dir_watch::init(): unexpected error '{}'", e.error().info());
                }
                break;
            }

            _last_valid_component = i;
            schedule_component(i);
        }
    }

    void schedule_component(size_t idx) {
        auto& comp = _components[idx];
        //glog().info("schedule_component(): {}", comp.path.string());
        auto task = comp.handler();
        comp.cp = task.cancelation_point();
        _conc.push(core::mov(task));
    }

    task<std::vector<wd_event>> handle_component(size_t idx) {
        if (idx == _components.size() - 1) {
            co_return handle_final_component();
        }

        auto& comp = _components[idx];
        auto events = comp.poller.poll();

        enum class event_verdict {
            reschedule,
            destroy_me,
            destroy_child,
            create_child,
        };

        auto verdict = event_verdict::reschedule;
        if (idx == 0) {
            for (auto& e : events) {
                if (e.flags.test(sys::inotify_watch_flag::delete_self) || e.flags.test(sys::inotify_watch_flag::move_self)) {
                    verdict = event_verdict::destroy_me;
                }
            }
        }

        //glog().detail("events({}):", comp.path.string());
        if (verdict != event_verdict::destroy_me) {
            for (auto& e : events) {
                //glog().detail("    {} {}", e.name, e.flags.to_string());
                auto child_name = _components[idx + 1].path.filename().string();
                if (e.flags.test(sys::inotify_watch_flag::moved_from) && e.flags.test(sys::inotify_watch_flag::isdir)) {
                    if (child_name == e.name) {
                        if (verdict == event_verdict::create_child) {
                            verdict = event_verdict::reschedule;
                        } else {
                            verdict = event_verdict::destroy_child;
                        }
                    }
                } else if (e.flags.test(sys::inotify_watch_flags::moved_to) && e.flags.test(sys::inotify_watch_flag::isdir)) {
                    if (child_name == e.name) {
                        if (verdict == event_verdict::destroy_child) {
                            verdict = event_verdict::reschedule;
                        } else {
                            verdict = event_verdict::create_child;
                        }
                    }
                } else if (e.flags.test(sys::inotify_watch_flag::create) && e.flags.test(sys::inotify_watch_flag::isdir)) {
                    if (child_name == e.name) {
                        verdict = event_verdict::create_child;
                    }
                } else if (e.flags.test(sys::inotify_watch_flags::delete_) && e.flags.test(sys::inotify_watch_flag::isdir)) {
                    if (child_name == e.name) {
                        verdict = event_verdict::destroy_child;
                    }
                }
            }
        }

        std::vector<wd_event> result_events;

        switch (verdict) {
        case event_verdict::reschedule:
            glog().debug("recursive_dir_watch::handle_component({}): reschedule", comp.path.string());
            schedule_component(idx);
            break;
        case event_verdict::destroy_child:
            glog().debug("recursive_dir_watch::handle_component({}): destroy_child", comp.path.string());
            schedule_component(idx);
        case event_verdict::destroy_me:
            glog().debug("recursive_dir_watch::handle_component({}): destroy_me", comp.path.string());
            for (size_t i = idx + 1; i < _components.size(); ++i) {
                co_await async::cancel(_components[i].cp);
            }
            result_events.push_back(wd_event{.flags = sys::inotify_watch_flag::delete_self, .name = ""});
            break;
        case event_verdict::create_child:
            glog().debug("recursive_dir_watch::handle_component({}): create_child", comp.path.string());
            schedule_component(idx);
            init(idx + 1);

            if (_last_valid_component == _components.size() - 1) {
                try {
                    auto dir   = io::file::open(_components.back().path.string(), sys::openflag::read_only | sys::openflag::directory);
                    auto files = sys::readdir(dir.fd());
                    for (auto file : files) {
                        if (file.name == "." || file.name == "..") {
                            continue;
                        }
                        sys::inotify_watch_flags flags = sys::inotify_watch_flag::create;
                        if (file.type == sys::file_type_dt::dir) {
                            flags |= sys::inotify_watch_flag::isdir;
                        }
                        result_events.push_back(wd_event{.flags = flags, .name = std::string(file.name)});
                    }
                } catch (const std::exception& e) {
                    glog().warn(
                        "recursive_dir_watch::handle_component(): cannot read files in final directory {}: {}", _components.back().path.string(), e.what()
                    );
                }
            }

            break;
        }

        co_return result_events;
    }

    std::vector<wd_event> handle_final_component() {
        auto& comp = _components.back();
        auto events = comp.poller.poll();

        size_t size = events.size();
        for (auto [i, e] : with_index(events)) {
            if (e.flags.test(sys::inotify_watch_flag::delete_self) || e.flags.test(sys::inotify_watch_flag::move_self)) {
                size = i + 1;
                break;
            }
        }
        events.resize(size);
        schedule_component(_components.size() - 1);

        return events;
    }

private:
    fs::path                  _root;
    fs::path                  _watch_dir;
    sys::inotify_watch_flags  _flags;
    u32                       _ring_size = 32;
    std::deque<dir_component> _components;
    size_t                    _last_valid_component = 0;

    static inline constexpr size_t                  max_components = 32;
    concurrent<tuple<size_t, bool>, max_components> _conc;
};
} // namespace core::async::util
