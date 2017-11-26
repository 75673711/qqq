#ifndef __CLIENTFRAME_WINDOWSHADOW_H__
#define __CLIENTFRAME_WINDOWSHADOW_H__

#include <QtWidgets/qgraphicseffect.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qwidget.h>

namespace clientframe
{
	class CWindowShadow : public QGraphicsEffect
	{
	public:
		enum Side {
			Left = 0x1,
			Right = 0x2,
			Bottom = 0x4,
			Top = 0x8,
			Around = Left | Top | Right | Bottom,
		};
		Q_DECLARE_FLAGS(Sides, Side);

		CWindowShadow(QObject *parent = 0);
		CWindowShadow(const QColor &c, qreal distance, qreal radius, Sides sides = Side::Around, QObject *parent = 0);

		Sides sides() const {
			return sides_;
		}
		void setSides(Sides s) {
			sides_ = s;
			updateBoundingRect();
		}

		QColor color() const {
			return color_;
		}
		void setColor(const QColor &c) {
			color_ = c;
			updateBoundingRect();
		}

		qreal blurRadius() const {
			return blur_radius_;
		}
		void setBlurRadius(qreal br) {
			blur_radius_ = br;
			updateBoundingRect();
		}

		qreal distance() const {
			return distance_;
		}
		void setDistance(qreal d) {
			distance_ = d;
			updateBoundingRect();
		}

		QRectF boundingRectFor(const QRectF &) const override;

		// Return a pixmap with target painted into it with margin = offset
		static QPixmap grab(QWidget *target, const QRect &rect, int offset); 

		// Return a background blurred QImage to Draw as the widget's shadow
		static QImage paint(QWidget *target, const QRect &box, qreal radius, qreal distance, const QColor &c, Sides sides = Side::Around);

	protected:
		void draw(QPainter *painter) override;

	private:
		Sides sides_;
		QColor color_;
		qreal distance_;
		qreal blur_radius_;
	};

	Q_DECLARE_OPERATORS_FOR_FLAGS(CWindowShadow::Sides)
}


#endif  // __CLIENTFRAME_WINDOWSHADOW_H__
