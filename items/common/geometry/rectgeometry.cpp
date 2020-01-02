#include "rectgeometry.h"

namespace whd3d {

RectGeometry::RectGeometry(const glm::fvec2 &size) :BasicGeometry(MeshType::MeshRectangle),
    size_(size)
{

}

RectGeometry::~RectGeometry()
{

}

void RectGeometry::GetVertexArray(std::vector<glm::f32> &vec)
{
//    size_.x = 2.0f;
//    size_.y = 2.0f;

    glm::f32 x = size_.x / 2.0f;
    glm::f32 y = size_.y / 2.0f;

    vec.reserve(4 * 2);
    // left-top left-down right-top right-down
    vec.push_back(-x);
    vec.push_back(y);

    vec.push_back(-x);
    vec.push_back(-y);

    vec.push_back(x);
    vec.push_back(y);

    vec.push_back(x);
    vec.push_back(-y);
}
}
