#ifndef BASICMESH_H
#define BASICMESH_H

#include "items/common/commonobject.h"

#include "glm/glm.hpp"

namespace whd3d {

class BasicGeometry;
class BasicMaterial;

class BasicMeshPrivate;

class BasicMesh
{
public:
    BasicMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material);
    virtual ~BasicMesh();

    // 将其他mesh合并到当前mesh中，绘制时就跳过被合并的mesh
    virtual bool Merge(BasicMesh* ptr_mesh) = 0;

    MeshType GetType() const;

    void translate(const glm::fvec3 t);
    void Rotate(const glm::fvec3 r);
    void Scale(const glm::fvec3 s);

    void SetPosition(const glm::fvec3 p) {
        position_ = p;
    }
    void SetRotation(const glm::fvec3 r);
    void SetScale(const glm::fvec3 s);

    glm::mat4x4 GetModelMat() const;

    virtual void DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection) = 0;

    static BasicMesh* CreateMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material);

protected:
    BasicGeometry* GetGeoMetry() const {
        return ptr_geometry_;
    }
    BasicMaterial* GetMaterial() const {
        return ptr_material_;
    }

private:
    BasicGeometry* ptr_geometry_ = nullptr;
    BasicMaterial* ptr_material_ = nullptr;

    glm::fvec3 position_ = {0.0f, 0.0f, 0.0f};
    glm::fvec3 rotation_ = {0.0f, 0.0f, 0.0f}; // 每个分量代表所属方向的旋转角度
    glm::fvec3 scale_ = {1.0f, 1.0f, 1.0f};
};
}

#endif // BASICMESH_H
