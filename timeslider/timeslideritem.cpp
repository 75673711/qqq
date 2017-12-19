#include "timeslideritem.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QQuickWindow>
#include <WinBase.h>
#include <iostream>

#include "linerender.h"

static int i = 0;

struct MyStateBinder
{
    MyStateBinder(TimeSliderItemRenderer *r)
        : m_r(r) {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
//        f->glEnable(GL_CULL_FACE);
//        f->glFrontFace(GL_CCW);
//        f->glCullFace(GL_BACK);
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    ~MyStateBinder() {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
//        f->glDisable(GL_CULL_FACE);
//        f->glDisable(GL_BLEND);
    }
    TimeSliderItemRenderer *m_r;
};

TimeSliderItemRenderer::TimeSliderItemRenderer() :
    time_level_(500),
    current_time_msec_(0),
    ptr_line_render_(NULL)
{
    ptr_line_render_ = new LineRender;
}

TimeSliderItemRenderer::~TimeSliderItemRenderer()
{
    if (ptr_line_render_ != NULL)
    {
        delete ptr_line_render_;
        ptr_line_render_ = NULL;
    }
}

void TimeSliderItemRenderer::synchronize(QQuickFramebufferObject *qfbitem)
{
    TimeSliderItem *item = static_cast<TimeSliderItem *>(qfbitem);

    current_time_msec_ = QDateTime::currentMSecsSinceEpoch();

    item->update();

//    int syncState = item->swapSyncState();
//    if (syncState & TimeSliderItem::CurrentTimeNeedsSync)
//    {
//        current_time_msec_ = item->getcurrentTime();
//    }

//    if (syncState & TimeSliderItem::TimeLevelNeedsSync)
//    {
//        time_level_ = item->getTimeLevel();
//    }
}

void TimeSliderItemRenderer::render()
{
    MyStateBinder state(this);

    //ptr_line_render_->DrawLines(0);

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

TimeSliderItem::TimeSliderItem(QQuickItem *parent) : QQuickFramebufferObject(parent),
    current_time_msec_(-1)
{
}

QQuickFramebufferObject::Renderer *TimeSliderItem::createRenderer() const
{
    return new TimeSliderItemRenderer();
}

int TimeSliderItem::swapSyncState()
{
    int s = m_syncState;
    m_syncState = 0;
    return s;
}

void TimeSliderItem::setCurrentTime(const qint64 &current_time)
{
//    if (current_time_msec_ != current_time) {
//        current_time_msec_ = QDateTime::currentMSecsSinceEpoch();
//        m_syncState |= CurrentTimeNeedsSync;
//        update();
//    }
}

void TimeSliderItem::setTimeLevel(const TimeLevel& time_level)
{
//    if (time_level_ != time_level) {
//        time_level_ = time_level;
//        m_syncState |= TimeLevelNeedsSync;
//        update();
//    }
}
