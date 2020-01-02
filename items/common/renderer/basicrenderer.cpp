#include "basicrenderer.h"

#include "items/common/camera/basiccamera.h"
#include "items/common/scene/basicscene.h"
#include "items/common/mesh/basicmesh.h"

namespace whd3d {
void BasicRenderer::SetViewport(const glm::vec2& view_size)
{
    view_size_ = view_size;
}

void BasicRenderer::Render(BasicCamera* ptr_camera, BasicScene* ptr_scene)
{
    for (const auto& it : ptr_scene->GetMeshes())
    {
        for (const auto& ptr_mesh : it)
        {
            ptr_mesh->DrawSelf(ptr_camera->GetViewMat4(), ptr_camera->GetProjectionMat4());
        }
    }
}
}
