#ifndef LINERENDER_H
#define LINERENDER_H

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

class FastTextRender;

class LineRender
{
public:
    LineRender();

    void DrawLines(qint64 utc,
                   const QSize& view_size,
                   int total_width,
                   int total_sec,
                   int total_line,
                   int one_unit_line,
                   int line_height);

    void EnsureInit();

    void PrintFps();

private:
    void InitBuf();
    void UninitBuf();

private:
    bool is_inited_;

    QScopedPointer<QOpenGLShaderProgram> shader_program_;

    unsigned int VBO, VAO, line_data_VBO;

    int offset_x_loc_;
    double offset_x_;

    float* line_data_buffer;

    FastTextRender* ptr_text_render_;
};

#endif // LINERENDER_H
