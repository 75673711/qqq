#ifndef FASTTEXTGEOMETRY_H
#define FASTTEXTGEOMETRY_H

#include "basicgeometry.h"

#include "glm/glm.hpp"

namespace whd3d {

class FastTextGeometry : public BasicGeometry
{
public:
    FastTextGeometry();
    virtual ~FastTextGeometry() override;

    virtual void GetVertexArray(std::vector<glm::f32>& vec) override;

private:

};

}

#endif // FASTTEXTGEOMETRY_H
