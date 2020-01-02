#ifndef BASICRENDERER_H
#define BASICRENDERER_H

#include "glm/glm.hpp"

namespace whd3d {

class BasicCamera;
class BasicScene;

class BasicRenderer
{
public:
    BasicRenderer() {}
    virtual ~BasicRenderer() {}

    void Render(BasicCamera* ptr_camera, BasicScene* ptr_scene);

    // void SetRenderTarget();  渲染对象   surface or imageview
    // void SetRenderCull();  渲染背面啥的 其他设置

    void SetClearColor();

    void SetViewport(const glm::vec2& view_size);

private:
    glm::vec2 view_size_ = {0, 0};
};
}

#endif // BASICRENDERER_H
