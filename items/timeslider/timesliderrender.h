#ifndef TIMESLIDERRENDER_H
#define TIMESLIDERRENDER_H

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include <QVector>

namespace whd3d {
    class BasicScene;
    class BasicCamera;
    class BasicRenderer;
    class BasicMesh;
}


class TimeSliderRender
{
public:
    TimeSliderRender();

    void SetViewSize(const QSize& view_size);

    void Render(uint32_t delta);

    void EnsureInit();

    void PrintFps();

private:
    void InitBuf();
    void UninitBuf();

private:
    bool is_inited_ = false;

    QSize view_size_;

    whd3d::BasicScene* ptr_scene_ = nullptr;
    whd3d::BasicCamera* ptr_camera_ = nullptr;
    whd3d::BasicRenderer* ptr_renderer_ = nullptr;
};

#endif // TIMESLIDERRENDER_H
