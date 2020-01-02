#include "timesliderrender.h"

#include <QOpenGLFunctions_4_4_Compatibility>
#include <QDateTime>
#include <QDebug>
#include <QColor>
#include <QPair>

#include <windows.h>
#include <stdio.h>

#include "items/common/commonobject.h"
#include "items/common/scene/basicscene.h"
#include "items/common/camera/basiccamera.h"
#include "items/common/geometry/rectgeometry.h"
#include "items/common/geometry/graduationgeometry.h"
#include "items/common/geometry/textgeometry.h"
#include "items/common/geometry/fasttextgeometry.h"
#include "items/common/material/colormaterial.h"
#include "items/common/mesh/basicmesh.h"
#include "items/common/mesh/fasttextmesh.h"
#include "items/common/mesh/graduationmesh.h"
#include "items/common/renderer/basicrenderer.h"

#include "glm/glm.hpp"

using namespace whd3d;

TimeSliderRender::TimeSliderRender() :
    is_inited_(false)
{
}

void TimeSliderRender::EnsureInit()
{
    if (!is_inited_)
        InitBuf();
}

void TimeSliderRender::UninitBuf()
{
    if (is_inited_)
    {
    }
}

void TimeSliderRender::InitBuf()
{
    glm::f32 width = 600.0f;
    glm::f32 height = 300.0f;

    if (!is_inited_)
    {
        is_inited_ = true;

        ptr_scene_ = new BasicScene;

        ptr_camera_ = new BasicCamera;
        ptr_camera_->SetPosition(glm::fvec3(0.0, 0.0, height / 2.0f));
        ptr_camera_->SetProjection(90.0f, width, height, 0.1f, 10000.0f);

        ptr_renderer_ = new BasicRenderer;
        ptr_renderer_->SetViewport(glm::vec2(width, height));

        RectGeometry* ptr_rect = new RectGeometry(glm::vec2(width, height));

        ColorMaterial* ptr_material = new ColorMaterial;

        BasicMesh* ptr_mesh = BasicMesh::CreateMesh(ptr_rect, ptr_material);
        ptr_scene_->AddMesh(ptr_mesh);

        GraduationGeometry* ptr_gra = new GraduationGeometry(10.0f);
        ColorMaterial* ptr_gra_m = new ColorMaterial;
        GraduationMesh* ptr_mesh3 = dynamic_cast<GraduationMesh*>(BasicMesh::CreateMesh(ptr_gra, ptr_gra_m));
        ptr_scene_->AddMesh(ptr_mesh3);
        ptr_mesh3->SetPosition(glm::fvec3(0.0f, 0, 1.0f));

        FastTextGeometry* ptr_fast_text = new FastTextGeometry();
        ColorMaterial* ptr_fast_text_m = new ColorMaterial;
        FastTextMesh* ptr_mesh5 = dynamic_cast<FastTextMesh*>(BasicMesh::CreateMesh(ptr_fast_text, ptr_fast_text_m));
        ptr_scene_->AddMesh(ptr_mesh5);
        ptr_mesh5->SetPosition(glm::fvec3(0.0f, 0.0f, 1.0f));
    }
}

void TimeSliderRender::SetViewSize(const QSize& view_size)
{
    view_size_ = view_size;

    if (is_inited_)
    {

    }
}

void TimeSliderRender::Render(uint32_t delta)
{
    QOpenGLFunctions_4_4_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_4_Compatibility>();
    if (!f)
    {
        Q_ASSERT(false);
    }
    CommonObject::GetInstance().SetFunc(f);

    EnsureInit();

    ptr_scene_->UpdateWorld(delta);

    //ptr_camera_->SetPosition(glm::fvec3(200.0, 200.0, 800.0));

    ptr_renderer_->Render(ptr_camera_, ptr_scene_);
}

void TimeSliderRender::PrintFps()
{
    static int draw_c = 0;

    static int sec = 0;
    int now_sec = QDateTime::currentDateTime().time().second();

    if (sec != now_sec)
    {
        qDebug() << draw_c;
        sec = now_sec;
        draw_c = 0;
    }
    else
    {
        ++draw_c;
    }
}
