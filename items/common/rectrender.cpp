#include "rectrender.h"

#include <QOpenGLFunctions_4_4_Compatibility>

#include <QDebug>

static const char* k_VertexShader =
        "#version 330\n"
        "layout (location = 0) in vec2 attribute_coord;\n"
        "uniform mat4 mvp[2];\n"
        "uniform float mmp[2];\n"
        "out vec3 f_color;\n"
        "void main()\n"
        "{\n"
            //"gl_Position = projection * view * model * vec4(attribute_coord, 0.0f, 1.0f);\n"
        "gl_Position = mvp[1] * vec4(attribute_coord, 0.0f, 1.0f);\n"
            "f_color = vec3(mmp[1], 1.0, 0.0);\n"
        "}\n";

//  - 1.0 + width / 2.0
// - 1.0 + hegiht / 2.0

static const char* k_FragmentShader =
        "#version 330\n"
        "out vec4 frag_color;\n"
        "in vec3 f_color;\n"
        "void main() {\n"
        "   frag_color = vec4(f_color, 1.0);\n"
        "}\n";

static float attribute_coord[] = {
//    -1.0, -1.0,
//    1.0, -1.0,
//    -1.0, 1.0,
//    1.0, 1.0
        -320.0, 120.0,
        -320.0, -120.0,
        320.0, 120.0,
        320.0, -120.0
};

static int max_rect_count = 100;

RectRender::RectRender()
{

}

RectRender::~RectRender()
{
    UninitBuf();
}

void RectRender::EnsureInit()
{
    if (!is_inited_)
        InitBuf();
}

void RectRender::UninitBuf()
{
    if (is_inited_)
    {
        is_inited_ = false;

        QOpenGLFunctions_4_4_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Compatibility>();

        f->glDeleteBuffers(1, &vbo);
        f->glDeleteVertexArrays(1, &vao);
        f->glDeleteShader(shaderProgram);
    }
}

void RectRender::InitBuf()
{
    is_inited_ = true;

    QOpenGLFunctions_4_4_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Compatibility>();

    // shader
    int vertexShader = f->glCreateShader(GL_VERTEX_SHADER);
    f->glShaderSource(vertexShader, 1, &k_VertexShader, NULL);
    f->glCompileShader(vertexShader);

    // fragment shader
    int fragmentShader = f->glCreateShader(GL_FRAGMENT_SHADER);
    f->glShaderSource(fragmentShader, 1, &k_FragmentShader, NULL);
    f->glCompileShader(fragmentShader);

    shaderProgram = f->glCreateProgram();
    f->glAttachShader(shaderProgram, vertexShader);
    f->glAttachShader(shaderProgram, fragmentShader);

    f->glLinkProgram(shaderProgram);

    f->glDeleteShader(vertexShader);
    f->glDeleteShader(fragmentShader);

    loc_mvp_ = f->glGetUniformLocation(shaderProgram, "mvp");

    // vao
    f->glGenVertexArrays(1, &vao);
    f->glBindVertexArray(vao);

    // vbo
    f->glGenBuffers(1, &vbo);
    f->glBindBuffer(GL_ARRAY_BUFFER, vbo);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(attribute_coord), attribute_coord, GL_STATIC_DRAW);

    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    f->glEnableVertexAttribArray(0);

    // 模型矩阵
    model_.rotate(0, QVector3D(1.0f, 0, 0));

    // 观察矩阵
    QVector3D pos(0.0, 0.0, 240.0);
    QVector3D center(0.0, 0.0, 0.0);
    QVector3D up(0.0, 1.0, 0.0);
    view_.lookAt(pos, center, up);

    f->glBindVertexArray(0);

    // 投影矩阵
    projection_.perspective(90, static_cast<float>(item_size_.width()) / static_cast<float>(item_size_.height()), 0.1f, 1000.0f);

    // 先行绑定   有变化时再更新
    f->glUseProgram(shaderProgram);

    float* temp = new float[4 * 4 * 2];
    QMatrix4x4 res = projection_*view_*model_;
    memcpy(temp, res.data(), 16 * sizeof(float));
    memcpy(temp + 16 * sizeof(float), res.data(), 16 * sizeof(float));


    f->glUniformMatrix4fv(loc_mvp_, 2, GL_TRUE, temp);

    float mmp[] = {0.0f, 1.0f};
    f->glUniform1fv(f->glGetUniformLocation(shaderProgram, "mmp"), 2, mmp);

    f->glUseProgram(0);
}

bool RectRender::SetMVP(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    model_ = model;
    view_ = view;
    projection_ = projection;

    mvp_changed_ = true;

    return true;
}

void RectRender::SetItemSize(const QSize& item_size)
{
    item_size_ = item_size;
}

void RectRender::RenderRect()
{
    EnsureInit();

    QOpenGLFunctions_4_4_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Compatibility>();

    f->glUseProgram(shaderProgram);
    f->glBindVertexArray(vao);

    // if matrix changed    update uniform params
    if (mvp_changed_)
    {
        f->glUseProgram(shaderProgram);
        f->glUniformMatrix4fv(loc_mvp_, 1, GL_FALSE, (projection_*view_*model_).data());
        f->glUseProgram(0);
    }

    f->glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

    f->glBindVertexArray(0);
    f->glUseProgram(0);
}

