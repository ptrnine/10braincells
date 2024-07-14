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

template <typename Epoller>
class input_mgr {
public:
    static inline constexpr size_t max_mouses = 2;

    void attach_to_epoller(Epoller& epoller) requires(!core::is_same<Epoller, void>) {
        _e = &epoller;
        _e->add_handler(core::int_c<event_group::input_device>, core::bind_member(*this, &input_mgr::epoller_handler));
    }

    void detach_epoller() {
        _e->remove_handler(core::int_c<event_group::input_device>);
    }

    void attach_mouse(inp::generic_mouse& mouse) {
        auto slot = find_free_mouse_slot();

        _mouses[slot].store(&mouse, std::memory_order_relaxed);

        if (mouse.type() == inp::mouse_type::native)
            _e->add(mouse.as_native().fd(),
                    sys::epoll_event::in | sys::epoll_event::edge_triggered,
                    event_group::input_device,
                    sys::u32(slot));

        ++_mouses_count;
    }

    void detach_mouse(inp::generic_mouse& mouse) {
        for (auto& m : _mouses) {
            if (m.load(std::memory_order_relaxed) == &mouse) {
                m.store(nullptr, std::memory_order_release);
                --_mouses_count;
                if (mouse.type() == inp::mouse_type::native)
                    _e->remove(mouse.as_native().fd());
                return;
            }
        }
    }

private:
    size_t find_free_mouse_slot() {
        for (size_t i = 0; i < max_mouses; ++i)
            if (!_mouses[i].load(std::memory_order_relaxed))
                return i;
        throw input_mgr_no_mouse_slot();
    }

public:
    /* Can be called from different thread */
    bool epoller_handler(sys::u32 dev_id, sys::fd_t, sys::epoll_events) {
        if (dev_id >= max_mouses) {
            util::glog().error("input_mgr::epoller_handler: invalid device id {}", dev_id);
            return false;
        }

        if (auto ptr = _mouses[dev_id].load(std::memory_order_acquire))
            ptr->as_native().handle();

        return true;
    }

private:
    Epoller*                         _e = nullptr;
    std::atomic<inp::generic_mouse*> _mouses[max_mouses];
    size_t                           _mouses_count;
};
} // namespace tbc
