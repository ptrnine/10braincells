#pragma once

#include <core/var.hpp>
#include <inp/native_mouse.hpp>
#include <inp/glfw_mouse.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace inp
{
enum class mouse_type {
    native = 0,
    glfw,
};

class generic_mouse {
public:
    template <typename T>
    constexpr generic_mouse(core::type_t<T> type, auto&&... args): _mouse(type, fwd(args)...) {}

    void introduce() {
        core::visit(_mouse, [](auto&& m) { m.introduce(); });
    }

    mouse_type type() const {
        return mouse_type{int(_mouse.index())};
    }

    auto&& as_native(this auto&& it) {
        return it._mouse.unsafe_get(core::type<native_mouse<>>);
    }

    auto&& as_glfw(this auto&& it) {
        return it._mouse.unsafe_get(core::type<glfw_mouse>);
    }

    const mouse& state() const {
        return core::visit(_mouse, [](auto&& m) -> const mouse& { return m.state(); });
    }

private:
    core::var<native_mouse<>, glfw_mouse> _mouse;
};
} // namespace inp

#undef fwd
