#pragma once

#include <span>
#include <glm/matrix.hpp>

#include <core/concepts/any_of.hpp>
#include <core/moveonly_trivial.hpp>
#include <core/utility/move.hpp>
#include <grx/basic_types.hpp>
#include <grx/gl/gl_shader_basic.hpp>
#include <grx/gl/gl_uniform_functions.hpp>
#include <util/vec.hpp>

namespace grx::gl {

template <typename T>
concept gl_uniformable = core::any_of<T,
                                      float,
                                      int,
                                      uint,
                                      double,
                                      util::vec2f,
                                      util::vec3f,
                                      util::vec4f,
                                      util::vec2d,
                                      util::vec3d,
                                      util::vec4d,
                                      util::vec2i,
                                      util::vec3i,
                                      util::vec4i,
                                      util::vec2u,
                                      util::vec3u,
                                      util::vec4u,
                                      glm::mat2x2,
                                      glm::mat2x3,
                                      glm::mat2x4,
                                      glm::mat3x2,
                                      glm::mat3x3,
                                      glm::mat3x4,
                                      glm::mat4x2,
                                      glm::mat4x3,
                                      glm::mat4x4,
                                      glm::mat<2, 2, double>,
                                      glm::mat<2, 3, double>,
                                      glm::mat<2, 4, double>,
                                      glm::mat<3, 2, double>,
                                      glm::mat<3, 3, double>,
                                      glm::mat<3, 4, double>,
                                      glm::mat<4, 2, double>,
                                      glm::mat<4, 3, double>,
                                      glm::mat<4, 4, double>,
                                      std::span<float>,
                                      std::span<double>,
                                      std::span<int>,
                                      std::span<uint>,
                                      std::span<util::vec2f>,
                                      std::span<util::vec3f>,
                                      std::span<util::vec4f>,
                                      std::span<util::vec2d>,
                                      std::span<util::vec3d>,
                                      std::span<util::vec4d>,
                                      std::span<util::vec2i>,
                                      std::span<util::vec3i>,
                                      std::span<util::vec4i>,
                                      std::span<util::vec2u>,
                                      std::span<util::vec3u>,
                                      std::span<util::vec4u>,
                                      std::span<glm::mat2x2>,
                                      std::span<glm::mat2x3>,
                                      std::span<glm::mat2x4>,
                                      std::span<glm::mat3x2>,
                                      std::span<glm::mat3x3>,
                                      std::span<glm::mat3x4>,
                                      std::span<glm::mat4x2>,
                                      std::span<glm::mat4x3>,
                                      std::span<glm::mat4x4>,
                                      std::span<glm::mat<2, 2, double>>,
                                      std::span<glm::mat<2, 3, double>>,
                                      std::span<glm::mat<2, 4, double>>,
                                      std::span<glm::mat<3, 2, double>>,
                                      std::span<glm::mat<3, 3, double>>,
                                      std::span<glm::mat<3, 4, double>>,
                                      std::span<glm::mat<4, 2, double>>,
                                      std::span<glm::mat<4, 3, double>>,
                                      std::span<glm::mat<4, 4, double>>>;

class gl_uniform_error : public gl_shader_exception {
public:
    gl_uniform_error(std::string message): msg(core::mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

namespace details {
template <typename T>
struct add_const_to_span_element {
    using type = T;
};

template <typename T>
struct add_const_to_span_element<std::span<T>> {
    using type = std::span<const T>;
};
}

template <gl_uniformable T>
class gl_uniform {
public:
    using element_t = details::add_const_to_span_element<T>::type;

    gl_uniform(uint program, const std::string& name):
        _program(program), _location(glGetUniformLocation(_program, name.data())) {
        if (_location < 0)
            throw gl_uniform_error("Invalid location of uniform \"" + name + '"');
    }

    gl_uniform(uint program, int location): _program(program), _location(location) {}

    bool set(const element_t& value) const {
        return details::uniform(_program, _location, value);
    }

    gl_uniform& operator=(const element_t& value) {
        if (!set(value))
            throw gl_uniform_error("Cannot push value to uniform");
        return *this;
    }

    uint program() const {
        return _program;
    }

    int location() const {
        return _location;
    }

    void set_location(int location) {
        _location = location;
    }

    void set_location(const std::string& name) {
        int location = glGetUniformLocation(_program, name.data());
        if (location < 0)
            throw gl_uniform_error("Invalid location of uniform \"" + name + '"');
        _location = location;
    }

    bool try_set_location(const std::string& name) {
        int location = glGetUniformLocation(_program, name.data());
        if (location < 0)
            return false;
        _location = location;
        return true;
    }

private:
    uint _program;
    int  _location;
};

template <gl_uniformable T>
class gl_lazy_uniform {
public:
    using element_t = details::add_const_to_span_element<T>;

    gl_lazy_uniform(std::string name, const element_t& value = {}): _name(core::mov(name)), _value(value) {}

    void set_program(uint program) {
        _uniform = gl_uniform<T>{program, _name};
    }

    void set(const element_t& value) {
        _value = value;
        if (_uniform.program() != uint(-1)) {
            _uniform.set_location(_name);
        }
    }

private:
    std::string   _name;
    gl_uniform<T> _uniform{uint(-1), -1};
    T             _value;
};
} // namespace grx::gl
