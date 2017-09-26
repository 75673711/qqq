#ifndef VIDEOITEM_H
#define VIDEOITEM_H

#include <QQuickFramebufferObject>

#include "yv12render.h"
#include "facerectrender.h"

class PreviewThread;
class PaintStruct;

class VideoItemRenderer : public QQuickFramebufferObject::Renderer
{
public:
    VideoItemRenderer();
    void synchronize(QQuickFramebufferObject* ptr_item) Q_DECL_OVERRIDE;
    void render() Q_DECL_OVERRIDE;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) Q_DECL_OVERRIDE;

    PaintStruct* ptr_paint_struct_;
    YV12Render yv12_render_;
    FaceRectRender face_rect_render_;
};

class VideoItem : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    explicit VideoItem(QQuickItem *parent = 0);

    QQuickFramebufferObject::Renderer *createRenderer() const Q_DECL_OVERRIDE;

    PreviewThread* GetPreviewThread() const
    {
        return ptr_preview_thread_;
    }

signals:

public slots:
    void Start();
    void Stop();

private:
    PreviewThread* ptr_preview_thread_;
};

#endif // VIDEOITEM_H
