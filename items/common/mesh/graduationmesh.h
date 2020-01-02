#ifndef GRADUATIONMESH_H
#define GRADUATIONMESH_H

#include "basicmesh.h"

namespace whd3d {

class BasicGeometry;
class BasicMaterial;

class GraduationMeshPrivate;

class GraduationMesh : public BasicMesh
{
public:
    GraduationMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material);
    virtual ~GraduationMesh() override;

    virtual bool Merge(BasicMesh* ptr_mesh) override;

    virtual void DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection) override;

    inline glm::fvec2* GetUniformAddress() const;

protected:
    void EnsureInit();

private:
    GraduationMeshPrivate* ptr_d_ = nullptr;
};
}

#endif // GRADUATIONMESH_H
