#pragma once

#include <GLFW/glfw3.h>

#include <core/robin_map.hpp>
#include <util/log.hpp>

namespace grx::gl
{

template <typename IdT, size_t MaxWindowCount = 16>
class glfw_ctx_mapper {
public:
    IdT get_current_ctx_gl_id(this auto&& it) {
        auto ctx = glfwGetCurrentContext();

        if (!ctx)
            util::glog().warn("{}: binding GL id without active context");

        auto [pos, inserted] = it.ctx_map.emplace(ctx, -1);
        if (inserted)
            pos.value() = it.new_glfw_ctx_handler(ctx);

        return pos.value();
    }

private:
    core::static_ptr_map<GLFWwindow*, IdT, MaxWindowCount> ctx_map;
};

} // namespace grx::gl
