#include "graduationgeometry.h"

namespace whd3d {

GraduationGeometry::GraduationGeometry(const glm::f32 &length) :BasicGeometry(MeshType::MeshGraduation),
    length_(length)
{

}

GraduationGeometry::~GraduationGeometry()
{

}

void GraduationGeometry::GetVertexArray(std::vector<glm::f32> &vec)
{
    glm::f32 x = 0.0f;

    vec.reserve(4 * 2);
    // left-top right-top right-down left-down
    vec.push_back(x);
    vec.push_back(0.0f);

    vec.push_back(x);
    vec.push_back(-length_);
}
}
