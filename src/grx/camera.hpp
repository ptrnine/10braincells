#pragma once

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <grx/basic_types.hpp>
#include <util/math.hpp>

namespace grx {

class camera {
public:
    static constexpr float FOV_MIN    = 10.f;
    static constexpr float FOV_MAX    = 180.f;
    static constexpr float PITCH_LOCK = glm::half_pi<float>() - 0.05f;
    static constexpr float ROLL_LOCK  = glm::half_pi<float>() - 0.1f;

    void look_at(const vec3f& position) {
        if (!position.essentially_equal(pos, 0.001f)) {
            dir     = (position - pos).normalize();
            ypr.y() = glm::clamp(asinf(-dir.y()), -PITCH_LOCK, PITCH_LOCK);
            ypr.x() = util::constraint_pi(-atan2f(-dir.x(), -dir.z()));
            ypr.z() = 0.0f;

            orientation = glm::rotate(glm::mat4(1.f), ypr.y(), glm::vec3(1.f, 0.f, 0.f));
            orientation = glm::rotate(orientation, ypr.x(), glm::vec3(0.f, 1.f, 0.f));

            glm::mat4 inv_orient = glm::inverse(orientation);

            right       = vec4f(inv_orient * glm::vec4(1.f, 0.f, 0.f, 1.f)).xyz();
            up          = right.cross(dir);
            orientation = glm::rotate(orientation, ypr.z(), dir.to_glm());
        }
    }

    void calc_orientation() {
        auto [yaw, pitch, roll] = ypr;

        orientation = glm::rotate(glm::mat4(1.f), pitch, glm::vec3(1.f, 0.f, 0.f));
        orientation = glm::rotate(orientation, yaw, glm::vec3(0.f, 1.f, 0.f));

        glm::mat4 inv_orient = glm::inverse(orientation);

        dir   = vec4f(inv_orient * glm::vec4(0.f, 0.f, -1.f, 1.f)).xyz();
        right = vec4f(inv_orient * glm::vec4(1.f, 0.f, 0.f, 1.f)).xyz();

        up          = right.cross(dir);
        orientation = glm::rotate(orientation, roll, dir.to_glm());
    }

    void calc_view_projection() {
        view       = orientation * glm::translate(glm::mat4(1.f), (-pos).to_glm());
        projection = glm::perspective(glm::radians(fov), aspect_ratio, z_near, z_far);
    }

private:
    vec3f pos;
    vec3f dir   = {0.f, 0.f, 1.f};
    vec3f right = {1.f, 0.f, 0.f};
    vec3f up    = {0.f, 1.f, 0.f};

    glm::mat4 orientation = glm::mat4(1.f);
    glm::mat4 view        = glm::mat4(1.f);
    glm::mat4 projection  = glm::mat4(1.f);

    float aspect_ratio;
    float fov;
    float z_near;
    float z_far;

    vec3f ypr       = {0.f, 0.f, 0.f};
    vec3f ypr_speed = {0.f, 0.f, 0.f};
};
} // namespace grx
