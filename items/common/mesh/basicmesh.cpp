#include "basicmesh.h"

#include "items/common/geometry/basicgeometry.h"
#include "items/common/material/basicmaterial.h"
#include "items/common/commonobject.h"
#include "rectmesh.h"
#include "graduationmesh.h"
#include "textmesh.h"
#include "fasttextmesh.h"

#include <QOpenGLFunctions_4_4_Compatibility>

#include "glm/ext.hpp"

namespace whd3d {

BasicMesh::BasicMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material):
    ptr_geometry_(ptr_geometry),
    ptr_material_(ptr_material)
{
}

BasicMesh::~BasicMesh()
{
    delete ptr_geometry_;
    delete ptr_material_;
}

MeshType BasicMesh::GetType() const
{
    return ptr_geometry_->GetType();
}

glm::mat4x4 BasicMesh::GetModelMat() const
{
    glm::mat4x4 model(1);
    model = glm::translate(model, position_);
    model = glm::rotate(model, glm::radians(rotation_.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation_.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation_.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale_);

    return model;
}

BasicMesh* BasicMesh::CreateMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material)
{
    BasicMesh* ptr_mesh = nullptr;
    switch (ptr_geometry->GetType()) {
    case MeshType::MeshLine:
        //ptr_mesh = new LineMesh(ptr_geometry, ptr_material);
        break;
    case MeshType::MeshRectangle:
        ptr_mesh = new RectMesh(ptr_geometry, ptr_material);
        break;
    case MeshType::MeshGraduation:
        ptr_mesh = new GraduationMesh(ptr_geometry, ptr_material);
        break;
    case MeshType::MeshText:
        ptr_mesh = new TextMesh(ptr_geometry, ptr_material);
        break;
    case MeshType::MeshFastText:
        ptr_mesh = new FastTextMesh(ptr_geometry, ptr_material);
        break;
    default:
        break;
    }

    return ptr_mesh;
}

}
