#ifndef RECTGEOMETRY_H
#define RECTGEOMETRY_H

#include "basicgeometry.h"

#include "glm/glm.hpp"

namespace whd3d {

class RectGeometry : public BasicGeometry
{
public:
    RectGeometry(const glm::fvec2& size);
    virtual ~RectGeometry() override;

    virtual void GetVertexArray(std::vector<glm::f32>& vec) override;

private:
    glm::fvec2 size_;
};

}

#endif // RECTGEOMETRY_H
