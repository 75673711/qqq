#ifndef LINERENDER_H
#define LINERENDER_H

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

class LineRender
{
public:
    LineRender();

    void DrawLines(qint64 utc);
    void DrawLines_1(qint64 utc);

    void EnsureInit();

    void PrintFps();

private:
    void InitBuf();

private:
    bool is_inited_;

    QScopedPointer<QOpenGLShaderProgram> shader_program_;

    unsigned int VBO, VAO, line_data_VBO;

    int offset_x_loc_;
    double offset_x_;
};

#endif // LINERENDER_H
