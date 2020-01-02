#include "graduationmesh.h"

#include "items/common/geometry/basicgeometry.h"
#include "items/common/material/basicmaterial.h"

#include "items/common/commonobject.h"

#include <QOpenGLFunctions_4_4_Compatibility>

#include "glm/ext.hpp"

#include <QDateTime>

#include <vector>
#include <chrono>

namespace whd3d {

static const char* k_VertexShader =
        "#version 330\n"
        "layout (location = 0) in vec2 attribute_coord;\n"
        "uniform mat4 mvp;\n"
        "uniform vec2 xandlen[100];\n"   // x轴偏移和y的倍数  （因为是y中一个点是0.0，所有直接相乘）
        "out vec4 f_color;\n"
        "void main()\n"
        "{\n"
        //"gl_Position = vec4(attribute_coord, 0.0f, 1.0f);\n"
        "float new_x = attribute_coord.x + xandlen[gl_InstanceID].x;\n"
        "float new_y = attribute_coord.y * xandlen[gl_InstanceID].y;\n"
        "gl_Position = mvp * vec4(new_x, new_y, 0.0f, 1.0f);\n"
        "f_color = vec4(0.0, 0.0, 0.0, 1.0);\n"
        "}\n";

static const char* k_FragmentShader =
        "#version 330\n"
        "out vec4 frag_color;\n"
        "in vec4 f_color;\n"
        "void main() {\n"
        "   frag_color = f_color;\n"
        "}\n";

static const glm::i32 kMaxLineCount = 100;   // 实例绘制支持的最大线段数

class GraduationMeshPrivate {
public:
    bool is_inited_ = false;
    GLuint shader_program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLint loc_mvp_ = 0;
    GLint loc_xandlen_ = 0;

    glm::fvec2 xandlen_array_[kMaxLineCount];
    glm::i32 line_count_ = kMaxLineCount;       // 实例绘制个数

    glm::i64 current_msec_ = 0;                      // 当前utc毫秒数
    glm::i32 big_scale_count_ = 12;                  // 总共大格数
    const static glm::i32 kOneBigScaleDivision = 5;  // 每大格分5小格
    glm::i32 small_scale_duration_msec = 1000;       // 每小格对应时间跨度  毫秒
    glm::f32 small_scale_length = 0.0f;              // 每小格对应长度

    void CalculateGraduation()
    {
        static glm::f32 width = 600.0f;

        glm::i32 total_msec = kOneBigScaleDivision * big_scale_count_ * small_scale_duration_msec;
        small_scale_length = width / static_cast<glm::f32>(kOneBigScaleDivision * big_scale_count_);

        current_msec_ = QDateTime::currentMSecsSinceEpoch();

        QDateTime date_time = QDateTime::fromMSecsSinceEpoch(current_msec_);
        glm::i32 current_sec = date_time.time().second();
        glm::i32 current_msec = date_time.time().msec();

        // 时间轴从左向右  这里多出来的时间对应中线往左侧偏移的位置
        glm::i32 offset_msec = (current_sec * 1000 + current_msec) % (kOneBigScaleDivision * small_scale_duration_msec)  ;
        glm::f32 middle_offset = static_cast<glm::f32>(offset_msec) * small_scale_length / static_cast<glm::f32>(small_scale_duration_msec);

        // right
        int index = 0;
        xandlen_array_[0].x = -middle_offset;
        xandlen_array_[0].y = 2.0f;
        for (index = 1; index < kMaxLineCount; ++index)
        {
            xandlen_array_[index].x = xandlen_array_[index - 1].x + small_scale_length;
            xandlen_array_[index].y = (index % kOneBigScaleDivision == 0) ? 2.0f : 1.0f;

            if (xandlen_array_[index].x > width / 2.0f)
            {
                break;
            }
        }

        glm::i32  l_count = 1;
        ++index;
        xandlen_array_[index].x = -middle_offset - small_scale_length;
        xandlen_array_[index].y = 1.0f;

        while (++index < kMaxLineCount)
        {
            ++l_count;
            xandlen_array_[index].x = xandlen_array_[index - 1].x - small_scale_length;
            if (l_count == 5)
            {
                xandlen_array_[index].y = 2.0f;
                l_count = 0;
            }
            else
            {
                xandlen_array_[index].y = 1.0f;
            }

            if (xandlen_array_[index].x < -width / 2.0f)
            {
                break;
            }
        }

        line_count_ = index;
    }
};

GraduationMesh::GraduationMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material):
    BasicMesh(ptr_geometry, ptr_material),
    ptr_d_(new GraduationMeshPrivate)
{

}

GraduationMesh::~GraduationMesh()
{
    delete ptr_d_;
    ptr_d_ = nullptr;
}

bool GraduationMesh::Merge(BasicMesh* )
{
    return true;
}

glm::fvec2* GraduationMesh::GetUniformAddress() const
{
    return ptr_d_->xandlen_array_;
}

// todo: 一些参数不改变的情况下   是否可以优化？
void GraduationMesh::DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection)
{
    EnsureInit();
    ptr_d_->CalculateGraduation();

    QOpenGLFunctions_4_4_Compatibility* f = CommonObject::GetInstance().GetFunc();

    f->glUseProgram(ptr_d_->shader_program_);
    f->glBindVertexArray(ptr_d_->vao_);

    glm::fmat4x4 mvp = projection * view * GetModelMat();
    f->glUniformMatrix4fv(ptr_d_->loc_mvp_, 1, GL_FALSE,  glm::value_ptr(mvp));

    f->glUniform2fv(ptr_d_->loc_xandlen_, ptr_d_->line_count_, glm::value_ptr(ptr_d_->xandlen_array_[0]));

    f->glDrawArraysInstanced(GL_LINES, 0, 2, ptr_d_->line_count_);

    f->glBindVertexArray(0);
    f->glUseProgram(0);
}

void GraduationMesh::EnsureInit()
{
    if (!ptr_d_->is_inited_)
    {
        for (glm::i32 i = 0; i < kMaxLineCount; ++i)
        {
            ptr_d_->xandlen_array_[i] = glm::fvec2(0.0f, 0.0f);
        }

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

        ptr_d_->loc_xandlen_ = f->glGetUniformLocation(ptr_d_->shader_program_, "xandlen");
        f->glUniform2fv(ptr_d_->loc_xandlen_, kMaxLineCount, glm::value_ptr(ptr_d_->xandlen_array_[0]));

        f->glUseProgram(ptr_d_->shader_program_);

        // done
        ptr_d_->is_inited_ = true;
    }
}

}
