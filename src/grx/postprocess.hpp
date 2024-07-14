#pragma once

#include <core/function.hpp>
#include <core/traits/declval.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace grx
{
template <typename ProgramT, size_t MaxUpdateCallbackSize = 32>
class postprocess {
public:
    template <typename T>
    using uniform_t = decltype(core::declval<ProgramT>().template get_uniform<T>(""));

    postprocess(ProgramT& program, auto&& update_callback):
        _update_callback(fwd(update_callback)),
        _program(&program),
        _input_texture_id(_program->get_uniform("screen")) {}

    void use() const {
        _program->use();
    }

    void set_input_texture_id(int id) const {
        use();
        _input_texture_id = id;
    }

    void update() const {
        use();
        if (_update_callback)
            _update_callback(*_program);
    }

private:
    core::function<void(const ProgramT& program), MaxUpdateCallbackSize> _update_callback;
    ProgramT*                                                            _program;
    uniform_t<int>                                                       _input_texture_id;
};
} // namespace grx

#undef fwd
