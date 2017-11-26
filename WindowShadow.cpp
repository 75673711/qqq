#include "WindowShadow.h"

Q_DECL_IMPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0); // src/widgets/effects/qpixmapfilter.cpp

namespace clientframe
{

	CWindowShadow::CWindowShadow(QObject *parent) :QGraphicsEffect(parent),
		blur_radius_(5),
		distance_(8)
	{

	}

	CWindowShadow::CWindowShadow(const QColor &c, qreal distance, qreal radius, Sides s, QObject *parent) : QGraphicsEffect(parent) 
	{
		setColor(c);
		setBlurRadius(radius);
		setDistance(distance);
		setSides(s);
	}

	QRectF CWindowShadow::boundingRectFor(const QRectF &r) const 
	{
		qreal delta = blurRadius() + distance();
		return r.marginsAdded(QMarginsF(
			(sides() & Side::Left) ? delta : 0,
			(sides() & Side::Top) ? delta : 0,
			(sides() & Side::Right) ? delta : 0,
			(sides() & Side::Bottom) ? delta : 0
		));
	}

	void CWindowShadow::draw(QPainter *painter) {
		if ((blurRadius() + distance()) <= 0) 
		{
			drawSource(painter);
			return;
		}

		QPoint offset;
		QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset, QGraphicsEffect::PadToEffectiveBoundingRect);
		if (pixmap.isNull()) 
			return;

		QTransform _transform = painter->worldTransform();
		painter->setWorldTransform(QTransform());

		QSize background_size = QSize(pixmap.size().width() + 2 * distance(), pixmap.size().height() + 2 * distance());
		QImage temp_image(background_size, QImage::Format_ARGB32_Premultiplied);

		QPixmap scaled = pixmap.scaled(background_size);
		temp_image.fill(0);

		QPainter temp_painter(&temp_image);
		temp_painter.setCompositionMode(QPainter::CompositionMode_Source);
		temp_painter.drawPixmap(QPointF(-distance(), -distance()), scaled);
		temp_painter.end();

		QImage blurred(temp_image.size(), QImage::Format_ARGB32_Premultiplied);
		blurred.fill(0);

		QPainter blurPainter(&blurred);
		qt_blurImage(&blurPainter, temp_image, blurRadius(), true, false);
		blurPainter.end();
		temp_image = blurred;

		temp_painter.begin(&temp_image);
		temp_painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		temp_painter.fillRect(temp_image.rect(), color());
		temp_painter.end();

		painter->drawImage(offset, temp_image);
		painter->drawPixmap(offset, pixmap, QRectF());
		painter->setWorldTransform(_transform);
	}

	QPixmap CWindowShadow::grab(QWidget *target, const QRect &rect, int offset) 
	{
		if (rect.size().isNull()) 
			return QPixmap();

		auto result = QPixmap(rect.size());
		auto r = rect.marginsRemoved(QMargins(offset, offset, offset, offset));
		result.fill(Qt::transparent);
		target->render(&result, QPoint(offset, offset), r);
		return result;
	}

	QImage CWindowShadow::paint(QWidget *target, const QRect &box, qreal radius, qreal distance, const QColor &c, Sides sides) 
	{
		const auto _source = grab(target, box, distance);
		if (_source.isNull() || distance <= 0) 
			return QImage();

		QImage background_image(box.size(), QImage::Format_ARGB32_Premultiplied);
		background_image.fill(0);

		QPainter background_painter(&background_image);
		background_painter.drawPixmap(QPointF(), _source);
		background_painter.end();

		QImage blurredImage(background_image.size(), QImage::Format_ARGB32_Premultiplied);
		blurredImage.fill(0);

		{
			QPainter blurPainter(&blurredImage);
			qt_blurImage(&blurPainter, background_image, radius, true, false);
			blurPainter.end();
		}
		background_image = blurredImage;

		background_painter.begin(&background_image);
		background_painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		auto margin = background_image.rect().marginsRemoved(QMargins(
			(sides & Left) ? 0 : distance,
			(sides & Top) ? 0 : distance,
			(sides & Right) ? 0 : distance,
			(sides & Bottom) ? 0 : distance
		));
		background_painter.fillRect(margin, c);
		background_painter.fillRect(margin.marginsRemoved(QMargins(distance, distance, distance, distance)), Qt::transparent);
		background_painter.end();

		return background_image;
	}

}