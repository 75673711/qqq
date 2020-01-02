#ifndef FASTTEXTMESH_H
#define FASTTEXTMESH_H

#include "basicmesh.h"

#include <vector>
#include <string>

namespace whd3d {

class BasicGeometry;
class BasicMaterial;

class FastTextMeshPrivate;

class FastTextMesh : public BasicMesh
{
public:
    FastTextMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material);
    virtual ~FastTextMesh() override;

    virtual bool Merge(BasicMesh* ptr_mesh) override;

    virtual void DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection) override;

    void SetFontSize(glm::u32 font_size);
    // 文字左下角为坐标原点   偏移以屏幕中心为原点（世界3D坐标系   y正方向上   x正方向右   z正方向指向屏幕外）
    void RenderText(const std::vector<std::pair<std::string, glm::fvec2>>& text_pair_vec);

protected:
    void EnsureInit();

private:
    FastTextMeshPrivate* ptr_d_ = nullptr;
};
}

#endif // FASTTEXTMESH_H
