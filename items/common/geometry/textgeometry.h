#ifndef TEXTGEOMETRY_H
#define TEXTGEOMETRY_H

#include "basicgeometry.h"

#include "glm/glm.hpp"

namespace whd3d {

class TextGeometry : public BasicGeometry
{
public:
    TextGeometry();
    virtual ~TextGeometry() override;

    virtual void GetVertexArray(std::vector<glm::f32>& vec) override;

private:

};

}

#endif // TEXTGEOMETRY_H
