#include "fasttextmesh.h"

#include "items/common/geometry/basicgeometry.h"
#include "items/common/material/basicmaterial.h"

#include "items/common/commonobject.h"

#include <QOpenGLFunctions_4_4_Compatibility>
#include <QFile>

#include "glm/ext.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>
#include <chrono>

namespace whd3d {

#define MAXWIDTH 1024    // 字体纹理最大宽度

static FT_Library ft = nullptr;
static FT_Face face = nullptr;

struct atlas {
    unsigned int w;			// width of texture in pixels
    unsigned int h;			// height of texture in pixels

    struct {
        float ax;	// advance.x
        float ay;	// advance.y

        float bw;	// bitmap.width;
        float bh;	// bitmap.height;

        float bl;	// bitmap_left;
        float bt;	// bitmap_top;

        float tx;	// x offset of glyph in texture coordinates
        float ty;	// y offset of glyph in texture coordinates
    } c[128];		// character information

    atlas(FT_Face face, FT_UInt font_pixel_size) {
        FT_Set_Pixel_Sizes(face, 0, font_pixel_size);
        FT_GlyphSlot g = face->glyph;

        unsigned int roww = 0;
        unsigned int rowh = 0;
        w = 0;
        h = 0;

        memset(c, 0, sizeof c);

        /* Find minimum size for a texture holding all visible ASCII characters */
        for (FT_ULong i = 32; i < 128; i++) {
            if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
                continue;
            }
            if (roww + g->bitmap.width + 1 >= MAXWIDTH) {
                w = std::max(w, roww);
                h += rowh;
                roww = 0;
                rowh = 0;
            }
            roww += g->bitmap.width + 1;
            rowh = std::max(rowh, g->bitmap.rows);
        }

        w = std::max(w, roww);
        h += rowh;

        QOpenGLFunctions_4_4_Compatibility* f = CommonObject::GetInstance().GetFunc();

        /* Create a texture that will be used to hold all ASCII glyphs */
        f->glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_ALPHA,
                        static_cast<unsigned int>(w),
                        static_cast<unsigned int>(h),
                        0,
                        GL_ALPHA,
                        GL_UNSIGNED_BYTE,
                        nullptr);

        /* We require 1 byte alignment when uploading texture data */
        f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        /* Clamping to edges is important to prevent artifacts when scaling */
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        /* Linear filtering usually looks best for text */
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        /* Paste all glyph bitmaps into the texture, remembering the offset */
        unsigned int ox = 0;
        unsigned int oy = 0;

        rowh = 0;

        for (FT_ULong i = 32; i < 128; i++) {
            FT_Error error = FT_Load_Char(face, i, FT_LOAD_RENDER);
            if (error) {
                qDebug() << "FT_Load_Char error: " << error;
                continue;
            }

            if (ox + g->bitmap.width + 1 >= MAXWIDTH) {
                oy += rowh;
                rowh = 0;
                ox = 0;
            }

            f->glTexSubImage2D(GL_TEXTURE_2D, 0, ox, oy, g->bitmap.width, g->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);
            c[i].ax = g->advance.x >> 6;
            c[i].ay = g->advance.y >> 6;

            c[i].bw = g->bitmap.width;
            c[i].bh = g->bitmap.rows;

            c[i].bl = g->bitmap_left;
            c[i].bt = g->bitmap_top;

            c[i].tx = ox / (float)w;
            c[i].ty = oy / (float)h;

            rowh = std::max(rowh, g->bitmap.rows);
            ox += g->bitmap.width + 1;
        }

        fprintf(stderr, "Generated a %d x %d (%d kb) texture atlas\n", w, h, w * h / 1024);
    }

    ~atlas() {

    }
};

static const char* k_VertexShader =
        "#version 330\n"
        "attribute vec4 coord;\n"
        "out vec2 texcoord;\n"
        "uniform mat4 mvp;\n"
        "void main()\n"
        "{\n"
            "gl_Position = mvp * vec4(coord.xy, 0, 1);\n"
            "texcoord = coord.zw;\n"
        "}\n";

static const char* k_FragmentShader =
        "#version 330\n"
        "in vec2 texcoord;\n"
        "uniform sampler2D tex;\n"
        "void main()\n"
        "{\n"
            "gl_FragColor = vec4(1, 1, 1, texture2D(tex, texcoord).a) * vec4(0,0,0,1);\n"
        "}\n";

const glm::i32 kMaxCharacterCount = 100;   // 实例绘制支持的最大线段数

class FastTextMeshPrivate {
public:
    FastTextMeshPrivate()
    {
        text_pair_vec_.reserve(kMaxCharacterCount);
        attribute_coord_ = new GLfloat[kMaxCharacterCount * 16];
    }

    ~FastTextMeshPrivate()
    {
        delete attribute_coord_;
        attribute_coord_ = nullptr;
    }

    bool is_inited_ = false;
    GLuint shader_program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint tex_ = 0;
    GLint loc_mvp_ = 0;
    GLint loc_xandlen_ = 0;

    GLfloat* attribute_coord_ = nullptr;
    glm::i32 render_count_ = 0;

    atlas* ptr_atlas_ = nullptr;
    FT_UInt font_size_ = 12;

    std::vector<std::pair<std::string, glm::fvec2>> text_pair_vec_;
    bool text_changed_ = false;
    int now_char_count_ = 0;
    // todo: haschange text_pair_vec_ font_size_
};

FastTextMesh::FastTextMesh(BasicGeometry* ptr_geometry, BasicMaterial* ptr_material):
    BasicMesh(ptr_geometry, ptr_material),
    ptr_d_(new FastTextMeshPrivate)
{
    FT_Error error;
    error = FT_Init_FreeType(&ft);
    if (error) {
        qDebug() << "Could not init freetype library\n";
    }

    std::string file_path = "E:/terriblePro/timeslider/TimeSliderGL/resource/arial.ttf";
    error = FT_New_Face(ft, file_path.c_str(), 0, &face);
    if (error) {
        qDebug() << "Could not open font exist:" << QFile(file_path.c_str()).exists() << " error: " << error;
    }

    // 必须先设置   否则FT_Load_Char失败
    FT_Set_Pixel_Sizes(face, 0, ptr_d_->font_size_);

    SetFontSize(12);
    RenderText({std::make_pair("lala", glm::fvec2(100.0f, 100.0f)),
               std::make_pair("haha", glm::fvec2(-100.0f, -100.0f))});
}

FastTextMesh::~FastTextMesh()
{
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    delete ptr_d_;
    ptr_d_ = nullptr;
}

bool FastTextMesh::Merge(BasicMesh* ptr_mesh)
{
    return true;
}

void FastTextMesh::SetFontSize(glm::u32 font_size)
{
    ptr_d_->font_size_ = font_size;

    FT_Set_Pixel_Sizes(face, 0, ptr_d_->font_size_);
    // todo:font size changed   recreate all
}

void FastTextMesh::RenderText(const std::vector<std::pair<std::string, glm::fvec2> > &text_pair_vec)
{
    ptr_d_->now_char_count_ = 0;
    for (const auto& it : text_pair_vec)
    {
        ptr_d_->now_char_count_ += it.first.length();
    }
    if (ptr_d_->now_char_count_ > kMaxCharacterCount)
    {
        Q_ASSERT(false);
        return;
    }

    ptr_d_->text_changed_ = true;
    ptr_d_->text_pair_vec_ = text_pair_vec;
}

// todo: 一些参数不改变的情况下   是否可以优化？
void FastTextMesh::DrawSelf(const glm::mat4x4& view, const glm::mat4x4& projection)
{
    if (face == nullptr || ft == nullptr || ptr_d_->text_pair_vec_.size() == 0)
    {
        return;
    }

    EnsureInit();


    QOpenGLFunctions_4_4_Compatibility* f = CommonObject::GetInstance().GetFunc();

    f->glUseProgram(ptr_d_->shader_program_);
    f->glBindVertexArray(ptr_d_->vao_);

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, ptr_d_->tex_);

    glm::fmat4x4 mvp = projection * view * GetModelMat();
    f->glUniformMatrix4fv(ptr_d_->loc_mvp_, 1, GL_FALSE,  glm::value_ptr(mvp));


    ////////////////////////////////////
    /////这个是以文字左下角为原点的绘制
    if (ptr_d_->text_changed_)
    {
        ptr_d_->text_changed_ = false;
        int index = 0;

        for (const auto& text_pair : ptr_d_->text_pair_vec_)
        {
            glm::f32 x = text_pair.second.x;
            glm::f32 y = text_pair.second.y;

            for (const char *p = text_pair.first.c_str(); *p; ++p)
            {
                short p_i = static_cast<short>(*p);
                float tx = ptr_d_->ptr_atlas_->c[p_i].tx;
                float ty = ptr_d_->ptr_atlas_->c[p_i].ty;
                float bl = ptr_d_->ptr_atlas_->c[p_i].bl;
                //float bt = ptr_d_->ptr_atlas_->c[p_i].bt;
                float bw = ptr_d_->ptr_atlas_->c[p_i].bw;
                float bh = ptr_d_->ptr_atlas_->c[p_i].bh;
                float ax = ptr_d_->ptr_atlas_->c[p_i].ax;
                float ay = ptr_d_->ptr_atlas_->c[p_i].ay;

                float x2 = x + bl;
                float y2 = y;
                float w = bw;
                float h = bh;

                ptr_d_->attribute_coord_[index++] = x2;
                ptr_d_->attribute_coord_[index++] = y2 + h;
                ptr_d_->attribute_coord_[index++] = tx;
                ptr_d_->attribute_coord_[index++] = ty;
                ptr_d_->attribute_coord_[index++] = x2;
                ptr_d_->attribute_coord_[index++] = y2;
                ptr_d_->attribute_coord_[index++] = tx;
                ptr_d_->attribute_coord_[index++] = ty + bh / ptr_d_->ptr_atlas_->h;
                ptr_d_->attribute_coord_[index++] = x2 + w;
                ptr_d_->attribute_coord_[index++] = y2 + h;
                ptr_d_->attribute_coord_[index++] = tx + bw / ptr_d_->ptr_atlas_->w;
                ptr_d_->attribute_coord_[index++] = ty;

                ptr_d_->attribute_coord_[index++] = x2 + w;
                ptr_d_->attribute_coord_[index++] = y2 + h;
                ptr_d_->attribute_coord_[index++] = tx + bw / ptr_d_->ptr_atlas_->w;
                ptr_d_->attribute_coord_[index++] = ty;
                ptr_d_->attribute_coord_[index++] = x2;
                ptr_d_->attribute_coord_[index++] = y2;
                ptr_d_->attribute_coord_[index++] = tx;
                ptr_d_->attribute_coord_[index++] = ty + bh / ptr_d_->ptr_atlas_->h;
                ptr_d_->attribute_coord_[index++] = x2 + w;
                ptr_d_->attribute_coord_[index++] = y2;
                ptr_d_->attribute_coord_[index++] = tx + bw / ptr_d_->ptr_atlas_->w;
                ptr_d_->attribute_coord_[index++] = ty + bh / ptr_d_->ptr_atlas_->h;

//                // 左上
//                ptr_d_->attribute_coord_[index++] = x2;
//                ptr_d_->attribute_coord_[index++] = y2 + h;
//                ptr_d_->attribute_coord_[index++] = tx;
//                ptr_d_->attribute_coord_[index++] = ty;
//                // 左下
//                ptr_d_->attribute_coord_[index++] = x2;
//                ptr_d_->attribute_coord_[index++] = y2;
//                ptr_d_->attribute_coord_[index++] = tx;
//                ptr_d_->attribute_coord_[index++] = ty + bh / ptr_d_->ptr_atlas_->h;
//                // 右上
//                ptr_d_->attribute_coord_[index++] = x2 + w;
//                ptr_d_->attribute_coord_[index++] = y2 + h;
//                ptr_d_->attribute_coord_[index++] = tx + bw / ptr_d_->ptr_atlas_->w;
//                ptr_d_->attribute_coord_[index++] = ty;
//                // 右下
//                ptr_d_->attribute_coord_[index++] = x2 + w;
//                ptr_d_->attribute_coord_[index++] = y2;
//                ptr_d_->attribute_coord_[index++] = tx + bw / ptr_d_->ptr_atlas_->w;
//                ptr_d_->attribute_coord_[index++] = ty + bh / ptr_d_->ptr_atlas_->h;

                x += ax;
                y += ay;
            }
        }

        f->glBindBuffer(GL_ARRAY_BUFFER, ptr_d_->vbo_);
        f->glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 6 * 4 * ptr_d_->now_char_count_, ptr_d_->attribute_coord_);  // todo: subdata
        f->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    f->glDrawArrays(GL_TRIANGLES, 0, ptr_d_->now_char_count_ * 6);

    /////////////////////////////////////

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    f->glBindVertexArray(0);
    f->glUseProgram(0);
}

void FastTextMesh::EnsureInit()
{
    if (!ptr_d_->is_inited_)
    {
        QOpenGLFunctions_4_4_Compatibility* f = CommonObject::GetInstance().GetFunc();

        // shader
        GLuint vertexShader = f->glCreateShader(GL_VERTEX_SHADER);
        f->glShaderSource(vertexShader, 1, &k_VertexShader, nullptr);
        f->glCompileShader(vertexShader);

        // fragment shader
        GLuint fragmentShader = f->glCreateShader(GL_FRAGMENT_SHADER);
        f->glShaderSource(fragmentShader, 1, &k_FragmentShader, nullptr);
        f->glCompileShader(fragmentShader);

        ptr_d_->shader_program_ = f->glCreateProgram();
        f->glAttachShader(ptr_d_->shader_program_, vertexShader);
        f->glAttachShader(ptr_d_->shader_program_, fragmentShader);
        f->glLinkProgram(ptr_d_->shader_program_);

        f->glDeleteShader(vertexShader);
        f->glDeleteShader(fragmentShader);

        f->glUseProgram(ptr_d_->shader_program_);

        // vao
        f->glGenVertexArrays(1, &ptr_d_->vao_);
        f->glBindVertexArray(ptr_d_->vao_);
        // vbo
        f->glGenBuffers(1, &ptr_d_->vbo_);

        f->glBindBuffer(GL_ARRAY_BUFFER, ptr_d_->vbo_);
        f->glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * kMaxCharacterCount * 24, nullptr, GL_DYNAMIC_DRAW);

        f->glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), static_cast<void*>(0));
        f->glEnableVertexAttribArray(0);

        // texture
        f->glGenTextures(1, &ptr_d_->tex_);
        f->glBindTexture(GL_TEXTURE_2D, ptr_d_->tex_);

        f->glActiveTexture(GL_TEXTURE0);
        ptr_d_->ptr_atlas_ = new atlas(face, ptr_d_->font_size_);

        f->glUniform1i(f->glGetUniformLocation(ptr_d_->shader_program_, "tex"), 0);

        // mvp
        glm::fmat4x4 mvp;
        ptr_d_->loc_mvp_ = f->glGetUniformLocation(ptr_d_->shader_program_, "mvp");
        f->glUniformMatrix4fv(ptr_d_->loc_mvp_, 1, GL_FALSE,  glm::value_ptr(mvp));


        f->glUseProgram(0);
        f->glBindVertexArray(0);

        // done
        ptr_d_->is_inited_ = true;
    }
}

}
