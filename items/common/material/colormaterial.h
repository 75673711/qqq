#ifndef COLORMATERIAL_H
#define COLORMATERIAL_H

#include "basicmaterial.h"

#include "glm/glm.hpp"

namespace whd3d {

class ColorMaterial : public BasicMaterial
{
public:
    ColorMaterial();
    virtual ~ColorMaterial() override;

    void SetColor(const glm::fvec3& color) {
        color_ = color;
    }

    glm::fvec3 GetColor() const {
        return color_;
    }

private:
    glm::fvec3 color_ = {0.0f, 0.0f, 0.0f};
};
}

#endif // BASICMATERIAL_H
