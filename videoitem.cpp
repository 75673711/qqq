#include "videoitem.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>

#include "previewthread.h"

#include <QDebug>

struct MyStateBinder
{
    MyStateBinder(VideoItemRenderer *r)
        : m_r(r) {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        //f->glEnable(GL_CULL_FACE);
        f->glFrontFace(GL_CCW);
        f->glCullFace(GL_BACK);
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    ~MyStateBinder() {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        f->glDisable(GL_CULL_FACE);
        f->glDisable(GL_BLEND);
    }
    VideoItemRenderer *m_r;
};

VideoItemRenderer::VideoItemRenderer():
    ptr_paint_struct_(NULL)
{

}

void VideoItemRenderer::synchronize(QQuickFramebufferObject* ptr_item)
{
    VideoItem* ptr_view_item = dynamic_cast<VideoItem*>(ptr_item);
    if (ptr_view_item != NULL)
    {
        if (ptr_paint_struct_ != NULL)
        {
            delete [] ptr_paint_struct_->data_buffer;
            delete ptr_paint_struct_;
            ptr_paint_struct_ = NULL;
        }

        ptr_paint_struct_ = ptr_view_item->GetPreviewThread()->GetPaintStruct();
    }
}

void VideoItemRenderer::render()
{
    if (ptr_paint_struct_ != NULL)
    {
        MyStateBinder state(this);

        QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
        //face_rect_render_.Render(ptr_paint_struct_, f);
        yv12_render_.Render(ptr_paint_struct_, f);
    }
}

QOpenGLFramebufferObject *VideoItemRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}

/////////////////////////////////////////////////////////////////////////

VideoItem::VideoItem(QQuickItem *parent) : QQuickFramebufferObject(parent),
    ptr_preview_thread_(NULL)
{
    ptr_preview_thread_ = new PreviewThread(this);
    connect(ptr_preview_thread_, &PreviewThread::SignalUpdate, this, &VideoItem::update);
}

QQuickFramebufferObject::Renderer *VideoItem::createRenderer() const
{
    return new VideoItemRenderer();
}

void VideoItem::Start()
{
    if (!ptr_preview_thread_->isRunning())
    {
        ptr_preview_thread_->start();
    }
}

void VideoItem::Stop()
{
    if (ptr_preview_thread_->isRunning())
    {
        ptr_preview_thread_->Stop();
    }
}
