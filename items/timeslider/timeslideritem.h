#ifndef TIMESLIDERITEM_H
#define TIMESLIDERITEM_H

#include <QQuickFramebufferObject>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLPaintDevice>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

class TimeSliderRender;

class TimeSliderItemRenderer : public QQuickFramebufferObject::Renderer
{
    friend struct MyStateBinder;
public:
    enum TotalSecEnum{
        Six = 6,
        Twenteen = 12,
        TwentyFour = 24,
        Sixty = 60,
    };

public:
    TimeSliderItemRenderer();
    ~TimeSliderItemRenderer() override;
    void synchronize(QQuickFramebufferObject *item) Q_DECL_OVERRIDE;
    void render() Q_DECL_OVERRIDE;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) Q_DECL_OVERRIDE;

public:


private:
    QSize view_size_;

    TimeSliderRender* ptr_render_ = nullptr;
    uint32_t delta_time_ = 0;
    qint64 last_render_msec_ = 0;
};

class TimeSliderItem : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 current_time_msec_ READ getcurrentTime WRITE setCurrentTime)

public:
    enum SyncState {
        CurrentTimeNeedsSync = 0x01,
        LargerSync = 0x02,
        SmallerSync = 0x04,
        AllNeedsSync = 0xFF
    };

    enum TimeLevel {
        HalfSecond = 500,
        OneSecond = 1000,
        FiveSecond = 5000,
        TenSecond = 10000
    };

public:
    explicit TimeSliderItem(QQuickItem *parent = 0);

    QQuickFramebufferObject::Renderer *createRenderer() const Q_DECL_OVERRIDE;

    int swapSyncState();

    qint64 getcurrentTime() const { return current_time_msec_; }
    void setCurrentTime(const qint64& current_time);

public slots:
    void ScaleLarger();
    void ScaleSmaller();

    void Start();
    void Stop();
    bool IsRunning() const { return keey_sliding_; }

signals:

public slots:

private:
    qint64 current_time_msec_ = 0;

    int m_syncState = 0;
    bool keey_sliding_ = false;
};

#endif // TIMESLIDERITEM_H
