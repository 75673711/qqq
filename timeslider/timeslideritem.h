#ifndef TIMESLIDERITEM_H
#define TIMESLIDERITEM_H

#include <QQuickFramebufferObject>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLPaintDevice>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>

class LineRender;

class TimeSliderItemRenderer : public QQuickFramebufferObject::Renderer
{
    friend struct MyStateBinder;
public:
    TimeSliderItemRenderer();
    ~TimeSliderItemRenderer();
    void synchronize(QQuickFramebufferObject *item) Q_DECL_OVERRIDE;
    void render() Q_DECL_OVERRIDE;
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) Q_DECL_OVERRIDE;

private:
    QSize view_size_;
    int time_level_;
    qint64 current_time_msec_;

    LineRender* ptr_line_render_;
};

class TimeSliderItem : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 current_time_msec_ READ getcurrentTime WRITE setCurrentTime)

public:
    enum SyncState {
        CurrentTimeNeedsSync = 0x01,
        TimeLevelNeedsSync = 0x02,
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

    TimeLevel getTimeLevel() const { return time_level_; }
    void setTimeLevel(const TimeLevel& time_level);

signals:

public slots:

private:
    TimeLevel time_level_;
    qint64 current_time_msec_;

    int m_syncState;
};

#endif // TIMESLIDERITEM_H
