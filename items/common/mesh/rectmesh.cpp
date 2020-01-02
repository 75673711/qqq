#include "rectmesh.h"

#include "items/common/geometry/basicgeometry.h"
#include "items/common/material/basicmaterial.h"

#include "items/common/commonobject.h"

#include <QOpenGLFunctions_4_4_Compatibility>

#include "glm/ext.hpp"

#include <vector>

namespace whd3d {

static const char* k_VertexShader =
        "#version 330\n"
        "layout (location = 0) in vec2 attribute_coord;\n"
        "uniform mat4 mvp;\n"
        "out vec4 f_color;\n"
        "void main()\n"
        "{\n"
        //"gl_Position = vec4(attribute_coord, 0.0f, 1.0f);\n"
        "gl_Position = mvp * vec4(attribute_coord, 0.0f, 1.0f);\n"
        "f_color = vec4(1.0, 1.0, 0.0, 1.0);\n"
        "}\n";

static const char* k_FragmentShader =
        "#version 330\n"
        "out vec4 frag_color;\n"
        "in vec4 f_color;\n"
        "void main() {\n"
        "   frag_color = f_color;\n"
        "}\n";

class RectMeshPrivate {
public:
    bool is_inited_ = false;
    GLuint shader_program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLint loc_mvp_ = 0;
};

RectMesh::RectMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material):
    BasicMesh(ptr_geometry, ptr_material),
    ptr_d_(new RectMeshPrivate)
{

}

RectMesh::~RectMesh()
{
    delete ptr_d_;
    ptr_d_ = nullptr;
}

bool RectMesh::Merge(BasicMesh* ptr_mesh)
{
    return true;
}

// todo: 一些参数不改变的情况下   是否可以优化？
void RectMesh::DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection)
{
    EnsureInit();

    QOpenGLFunctions_4_4_Compatibility* f = CommonObject::GetInstance().GetFunc();

    f->glUseProgram(ptr_d_->shader_program_);
    f->glBindVertexArray(ptr_d_->vao_);

    glm::fmat4x4 mvp = projection * view * GetModelMat();
    f->glUniformMatrix4fv(ptr_d_->loc_mvp_, 1, GL_FALSE,  glm::value_ptr(mvp));

    f->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

    f->glBindVertexArray(0);
    f->glUseProgram(0);
}

void RectMesh::EnsureInit()
{
    if (!ptr_d_->is_inited_)
    {
        QOpenGLFunctions_4_4_Compatibility* f = CommonObject::GetInstance().GetFunc();

        // shader
        GLuint vertexShader = f->glCreateShader(GL_VERTEX_SHADER);
        f->glShaderSource(vertexShader, 1, &k_VertexShader, nullptr);
        f->glCompileShader(vertexShader);

        GLuint fragmentShader = f->glCreateShader(GL_FRAGMENT_SHADER);
        f->glShaderSource(fragmentShader, 1, &k_FragmentShader, nullptr);
        f->glCompileShader(fragmentShader);

        ptr_d_->shader_program_ = f->glCreateProgram();
        f->glAttachShader(ptr_d_->shader_program_, vertexShader);
        f->glAttachShader(ptr_d_->shader_program_, fragmentShader);

        f->glLinkProgram(ptr_d_->shader_program_);

        f->glDeleteShader(vertexShader);
        f->glDeleteShader(fragmentShader);

        std::vector<glm::f32> vertex_array;
        GetGeoMetry()->GetVertexArray(vertex_array);

        // vao
        f->glGenVertexArrays(1, &ptr_d_->vao_);
        f->glBindVertexArray(ptr_d_->vao_);

        // vbo
        f->glGenBuffers(1, &ptr_d_->vbo_);
        f->glBindBuffer(GL_ARRAY_BUFFER, ptr_d_->vbo_);
        f->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertex_array.size() * sizeof(glm::f32)), vertex_array.data(), GL_STATIC_DRAW);

        f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::f32), nullptr);
        f->glEnableVertexAttribArray(0);

        f->glBindVertexArray(0);

        // uniform
        glm::fmat4x4 mvp;

        f->glUseProgram(ptr_d_->shader_program_);

        ptr_d_->loc_mvp_ = f->glGetUniformLocation(ptr_d_->shader_program_, "mvp");
        f->glUniformMatrix4fv(ptr_d_->loc_mvp_, 1, GL_FALSE,  glm::value_ptr(mvp));

        f->glUseProgram(0);
        f->glBindVertexArray(0);

        // done
        ptr_d_->is_inited_ = true;
    }
}

}
