#ifndef BASICGEOMETRY_H
#define BASICGEOMETRY_H

#include "items/common/commonobject.h"

#include "glm/glm.hpp"

#include <vector>

namespace whd3d {

class BasicGeometry
{
public:
    BasicGeometry(const MeshType& mesh_type);
    virtual ~BasicGeometry() {}

    MeshType GetType() const {
        return mesh_type_;
    }

    virtual void GetVertexArray(std::vector<glm::f32>& vec) = 0;

private:
    MeshType mesh_type_;
};

}

#endif // BASICGEOMETRY_H
