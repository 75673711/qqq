#ifndef RECTRENDER_H
#define RECTRENDER_H

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include <QVector>
#include <QRect>
#include <QColor>

struct RectData
{
    QRect rect;
    QColor color;
};

class RectRender
{
public:
    RectRender();
    ~RectRender();

    bool SetMVP(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void RenderRect();
    void SetItemSize(const QSize& item_size);

private:
    void EnsureInit();
    void InitBuf();
    void UninitBuf();

private:
    bool is_inited_ = false;
    QSize item_size_;

    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint vbo = 0;

    QMatrix4x4 model_;
    QMatrix4x4 view_;
    QMatrix4x4 projection_;

    QMatrix4x4 mvp_array_[10];

    GLint loc_mvp_ = 0;
    bool mvp_changed_ = false;
};

#endif // RECTRENDER_H
