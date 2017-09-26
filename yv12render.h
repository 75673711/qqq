#ifndef YV12RENDER_H
#define YV12RENDER_H

#include <QScopedPointer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

class QOpenGLFunctions;
struct PaintStruct;

class YV12Render
{
public:
    YV12Render();

    void EnsureInit(PaintStruct* ptr_paint_struct);

    void Render(PaintStruct* ptr_paint_struct, QOpenGLFunctions* f);

    bool is_inited_;

    int shaderProgram;

    unsigned int VBO, VAO, EBO;

    unsigned int texture1_id_;
    unsigned int texture2_id_;
    unsigned int texture3_id_;
};

#endif // YV12RENDER_H
