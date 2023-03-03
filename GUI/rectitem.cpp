#include "rectitem.h"

RectItem::RectItem(
	qreal x,
	qreal y,
	qreal w,
	qreal h,
	QGraphicsItem * p) : QGraphicsRectItem(x, y, w, h, p)
{
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, false);
    setAcceptHoverEvents(false);
	pwidth = 3.0;
	set_pen1(pwidth);
}

RectItem::~RectItem()
{
}

void RectItem::set_pen1(
	qreal w,
	unsigned int r,
	unsigned int g,
	unsigned int b)
{
	current_pen = 1;
	pwidth = w;
	QBrush brush(QColor(r,g,b));
	QPen pen;
	pen.setBrush(brush);
	pen.setWidthF(w);
	pen.setStyle(Qt::SolidLine);
	setPen(pen);
}

void RectItem::set_pen2(
	qreal w,
	unsigned int r,
	unsigned int g,
	unsigned int b)
{
	current_pen = 2;
	pwidth = w;
	QBrush brush(QColor(r, g, b));
	QPen pen;
	pen.setBrush(brush);
	pen.setWidthF(w);
	pen.setStyle(Qt::DashLine);
	setPen(pen);
}

qreal RectItem::get_width() const
{
	return pwidth;
}

void RectItem::set_width(double x)
{
	pwidth = x;
}

short RectItem::get_current_pen() const
{
	return current_pen;
}

QRectF RectItem::boundingRect() const
{
	return QGraphicsRectItem::boundingRect();
}

int RectItem::type() const
{
    return Type;
}

void RectItem::mouseMoveEvent(
	QGraphicsSceneMouseEvent * e)
{
	QGraphicsItem::mouseMoveEvent(e);
}

void RectItem::mousePressEvent(
	QGraphicsSceneMouseEvent * e)
{
	QGraphicsItem::mousePressEvent(e);
}

void RectItem::mouseReleaseEvent(
	QGraphicsSceneMouseEvent * e)
{
	QGraphicsItem::mouseReleaseEvent(e);
}

void RectItem::keyPressEvent(QKeyEvent * e)
{
	QGraphicsItem::keyPressEvent(e);
}

void RectItem::paint(
	QPainter * painter,
	const QStyleOptionGraphicsItem * option,
	QWidget * widget)
{
	QGraphicsRectItem::paint(painter, option, widget);
}

