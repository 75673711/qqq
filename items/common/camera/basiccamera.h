#ifndef BASICCAMERA_H
#define BASICCAMERA_H

#include "glm/glm.hpp"

namespace whd3d {
class BasicCamera
{
public:
    BasicCamera();
    virtual ~BasicCamera() {}

    const glm::mat4& GetViewMat4() const {
        return view_mat4_;
    }

    const glm::mat4& GetProjectionMat4() const {
        return projection_mat4_;
    }

    void SetPosition(const glm::vec3& pos);

    void SetProjection(const glm::f32& angle,
                       const glm::f32& width,
                       const glm::f32& height,
                       const glm::f32 near,
                       const glm::f32 far);

private:
    glm::mat4 view_mat4_;
    glm::mat4 projection_mat4_;

    glm::fvec3 position_ = glm::fvec3(0.0f, 0.0f, 0.0f);
    glm::fvec3 center_ = glm::fvec3(0.0f, 0.0f, 0.0f);
    glm::fvec3 up_ = glm::fvec3(0.0f, 1.0f, 0.0f);
};
}

#endif // BASICCAMERA_H
