#pragma once

#include <atomic>
#include <core/utility/bind_member.hpp>
#include <core/var.hpp>
#include <event_groups.hpp>
#include <inp/generic_mouse.hpp>
#include <sys/epoll.hpp>
#include <util/log.hpp>

#include <core/epoller.hpp>

namespace tbc {

class input_mgr_no_mouse_slot : public core::exception {
public:
    const char* what() const noexcept override {
        return "No free mouse slot";
    }
};

/* TODO: build input device list */
template <typename Epoller>
class input_mgr {
public:
    static inline constexpr size_t max_mouses = 2;

    void attach_to_epoller(Epoller& epoller) requires(!core::is_same<Epoller, void>) {
        _e = &epoller;
        _e->add_handler(core::int_c<event_group::input_device>, core::bind_member(*this, &input_mgr::epoller_handler));

        for (sys::u32 slot = 0; slot < max_mouses; ++slot) {
            auto mouse = _mouses[slot].load(std::memory_order_relaxed);
            if (mouse && mouse->type() == inp::mouse_type::native)
                add_dev_fd_to_epoller(mouse->as_native().fd(), slot);
        }
    }

    void detach_epoller() {
        if (_e) {
            _e->remove_handler(core::int_c<event_group::input_device>);

            for (auto&& m : _mouses) {
                if (auto mouse = m.load(std::memory_order_relaxed)) {
                    if (mouse->type() == inp::mouse_type::native)
                        _e->remove(mouse->as_native().fd());
                }
            }

            _e = nullptr;
        }
    }

    void attach_mouse(inp::generic_mouse& mouse) {
        auto slot = find_free_mouse_slot();

        _mouses[slot].store(&mouse, std::memory_order_relaxed);

        if (_e && mouse.type() == inp::mouse_type::native) {
            util::glog().detail("[input_mgr]: attach native mouse fd={} slot={}", int(mouse.as_native().fd()), slot);
            add_dev_fd_to_epoller(mouse.as_native().fd(), sys::u32(slot));
        }

        ++_mouses_count;
    }

    void detach_mouse(inp::generic_mouse& mouse) {
        for (auto& m : _mouses) {
            if (m.load(std::memory_order_relaxed) == &mouse) {
                m.store(nullptr, std::memory_order_release);
                --_mouses_count;
                if (_e && mouse.type() == inp::mouse_type::native)
                    _e->remove(mouse.as_native().fd());
                return;
            }
        }
    }

private:
    void add_dev_fd_to_epoller(sys::fd_t fd, sys::u32 slot) {
        _e->add(fd, sys::epoll_event::in | sys::epoll_event::edge_triggered, event_group::input_device, slot);
    }

    size_t find_free_mouse_slot() {
        for (size_t i = 0; i < max_mouses; ++i)
            if (!_mouses[i].load(std::memory_order_relaxed))
                return i;
        throw input_mgr_no_mouse_slot();
    }

    /* Can be called from different thread */
    core::epoller_handler_result epoller_handler(sys::u32 dev_id, sys::fd_t, sys::epoll_events) {
        if (dev_id >= max_mouses) {
            util::glog().error("input_mgr::epoller_handler: invalid device id {}", dev_id);
            return core::epoller_handler_result::remove;
        }

        if (auto ptr = _mouses[dev_id].load(std::memory_order_acquire))
            ptr->as_native().handle();

        return core::epoller_handler_result::keep_on;
    }

private:
    Epoller*                         _e = nullptr;
    std::atomic<inp::generic_mouse*> _mouses[max_mouses];
    size_t                           _mouses_count;
};
} // namespace tbc
