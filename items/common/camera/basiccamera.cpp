#include "basiccamera.h"

#include "glm/common.hpp"
#include "glm/ext.hpp"

namespace whd3d {
    BasicCamera::BasicCamera()
    {
//        glm::fvec3 pos(0.0, 0.0, 120.0);
//        glm::fvec3 center(0.0, 0.0, 0.0);
//        glm::fvec3 up(0.0, 1.0, 0.0);
//        view_mat4_ = glm::lookAt(pos, center, up);

//        projection_mat4_ = glm::perspective(glm::radians(45.0f), 240.0f / 640.0f, 0.1f, 1000.0f);
    }

    void BasicCamera::SetPosition(const glm::vec3& pos)
    {
        position_ = pos;
        view_mat4_ = glm::lookAt(position_, center_, up_);
    }

    void BasicCamera::SetProjection(const glm::f32& angle,
                       const glm::f32& width,
                       const glm::f32& height,
                       const glm::f32 near,
                       const glm::f32 far)
    {
        projection_mat4_ = glm::perspective(glm::radians(angle), width / height, near, far);
    }
}
