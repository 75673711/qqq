/****************************************************************************
**
** 2019/12/21
** item主线程中记录当前时间 刻度 等主要信息 渲染线程中计算出各元素坐标提交至OpenGL实例化渲染
** 渲染线程每次同步数据时，更新主线程的当前时间，若主线程因用户输入等原因强制更新时间，则使用
** 标志位表明当前时间不可吸入，只能读取
**
****************************************************************************/
#include "timeslideritem.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QQuickWindow>
#include <WinBase.h>
#include <iostream>

#include "timesliderrender.h"

static QVector<int> kTimeEnum;

struct MyStateBinder
{
    MyStateBinder(TimeSliderItemRenderer *r)
        : m_r(r) {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glEnable(GL_CULL_FACE);
        f->glFrontFace(GL_CCW);   //有两个基本作用，一是可以用来用在某些特殊场合（比如剔除面片），二是可以提高渲染效率  GL_CCW表示窗口坐标上投影多边形的顶点顺序为逆时针方向的表面为正面
        f->glCullFace(GL_BACK);   //表示禁用多边形正面或者背面上的光照、阴影和颜色计算及操作
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        f->glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    ~MyStateBinder() {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glDisable(GL_CULL_FACE);
        f->glDisable(GL_BLEND);
    }
    TimeSliderItemRenderer *m_r;
};

TimeSliderItemRenderer::TimeSliderItemRenderer()
{
    ptr_render_ = new TimeSliderRender;
    ptr_render_->SetViewSize(this->view_size_);  // todo: 运行时改变

    kTimeEnum << 6 << 12 << 24 << 60;
}

TimeSliderItemRenderer::~TimeSliderItemRenderer()
{
    if (ptr_render_ != nullptr)
    {
        delete ptr_render_;
        ptr_render_ = nullptr;
    }
}

void TimeSliderItemRenderer::synchronize(QQuickFramebufferObject *qfbitem)
{
    TimeSliderItem *item = static_cast<TimeSliderItem *>(qfbitem);

    int syncState = item->swapSyncState();
    if (syncState & TimeSliderItem::CurrentTimeNeedsSync)
    {

    }

    if (syncState & TimeSliderItem::LargerSync)
    {

    }

    if (syncState & TimeSliderItem::SmallerSync)
    {

    }

    item->update();
}

void TimeSliderItemRenderer::render()
{    
    MyStateBinder state(this);

    qint64 now_render_msec_ = QDateTime::currentMSecsSinceEpoch();
    delta_time_ = static_cast<uint32_t>(now_render_msec_ - last_render_msec_);
    last_render_msec_ = now_render_msec_;

    ptr_render_->Render(delta_time_);
}

QOpenGLFramebufferObject *TimeSliderItemRenderer::createFramebufferObject(const QSize &size)
{
    view_size_ = size;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

//////////////////////////////////////////////////////////////

TimeSliderItem::TimeSliderItem(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
    this->setMirrorVertically(true);        // 上下颠倒
}

QQuickFramebufferObject::Renderer *TimeSliderItem::createRenderer() const
{
    return new TimeSliderItemRenderer();
}

void TimeSliderItem::Start()
{
    keey_sliding_ = true;
    update();
}

void TimeSliderItem::Stop()
{
    keey_sliding_ = false;
}

int TimeSliderItem::swapSyncState()
{
    if (!this->isVisible())
    {
        return 0;
    }

    if (keey_sliding_)
    {
        setCurrentTime(QDateTime::currentMSecsSinceEpoch());
    }

    int s = m_syncState;
    m_syncState = 0;
    return s;
}

void TimeSliderItem::setCurrentTime(const qint64 &current_time)
{
    if (current_time_msec_ != current_time) {
        current_time_msec_ = QDateTime::currentMSecsSinceEpoch();
        m_syncState |= CurrentTimeNeedsSync;
        update();
    }
}

void TimeSliderItem::ScaleLarger()
{
    m_syncState |= LargerSync;
    update();
}

void TimeSliderItem::ScaleSmaller()
{
    m_syncState |= SmallerSync;
    update();
}
