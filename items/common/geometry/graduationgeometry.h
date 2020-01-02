#ifndef GRADUATIONGEOMETRY_H
#define GRADUATIONGEOMETRY_H

#include "basicgeometry.h"

#include "glm/glm.hpp"

namespace whd3d {

class GraduationGeometry : public BasicGeometry
{
public:
    GraduationGeometry(const glm::f32& length);
    virtual ~GraduationGeometry() override;

    virtual void GetVertexArray(std::vector<glm::f32>& vec) override;

private:
    glm::f32 length_ = 0.0f;
};

}

#endif // GRADUATIONGEOMETRY_H
