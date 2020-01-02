#ifndef RECTMESH_H
#define RECTMESH_H

#include "basicmesh.h"

namespace whd3d {

class BasicGeometry;
class BasicMaterial;

class RectMeshPrivate;

class RectMesh : public BasicMesh
{
public:
    RectMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material);
    virtual ~RectMesh() override;

    virtual bool Merge(BasicMesh* ptr_mesh) override;

    virtual void DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection) override;

protected:
    void EnsureInit();

private:
    RectMeshPrivate* ptr_d_ = nullptr;
};
}

#endif // RECTMESH_H
