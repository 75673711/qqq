#ifndef BASICMATERIAL_H
#define BASICMATERIAL_H

#include <string>

namespace whd3d {

class BasicMaterial
{
public:
    BasicMaterial() {}
    virtual ~BasicMaterial() {}

    //virtual std::string GetFragShader() const = 0;

    // todo:
    //virtual void BindParamters(GLuint vertex_shader, GLuint shader_program);
    //virtual void UpdateParamters(GLuint shader_program);
};
}

#endif // BASICMATERIAL_H
