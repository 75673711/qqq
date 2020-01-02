#ifndef TEXTMESH_H
#define TEXTMESH_H

#include "basicmesh.h"

#include <vector>
#include <string>

namespace whd3d {

class BasicGeometry;
class BasicMaterial;

class TextMeshPrivate;

class TextMesh : public BasicMesh
{
public:
    TextMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material);
    virtual ~TextMesh() override;

    virtual bool Merge(BasicMesh* ptr_mesh) override;

    virtual void DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection) override;

    void SetFontSize(glm::u32 font_size);
    void RenderText(const std::vector<std::pair<std::string, glm::fvec2>>& text_pair_vec);

protected:
    void EnsureInit();

private:
    TextMeshPrivate* ptr_d_ = nullptr;
};
}

#endif // TEXTMESH_H
