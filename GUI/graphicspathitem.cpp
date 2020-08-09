#include "graphicspathitem.h"
#include <QPainterPathStroker>

GraphicsPathItem::GraphicsPathItem()
	:
	contour_id(-1),
	roi_id(-1),
	tmp_id(-1),
	axis(-1),
	slice(-1),
	contour_type(2)
{
}

GraphicsPathItem::~GraphicsPathItem()
{
}

int GraphicsPathItem::get_contour_id() const
{
	return contour_id;
}

void GraphicsPathItem::set_contour_id(int a)
{
	contour_id = a;
}

int  GraphicsPathItem::get_roi_id() const
{
	return roi_id;
}

void GraphicsPathItem::set_roi_id(int a)
{
	roi_id = a;
}

long GraphicsPathItem::get_tmp_id() const
{
	return tmp_id;
}

void GraphicsPathItem::set_tmp_id(long a)
{
	tmp_id = a;
}

int GraphicsPathItem::get_axis() const
{
	return axis;
}

void GraphicsPathItem::set_axis(int a)
{
	axis = a;
}

int GraphicsPathItem::get_slice() const
{
	return slice;
}

void GraphicsPathItem::set_slice(int s)
{
	slice = s;
}

int GraphicsPathItem::get_type() const
{
	return contour_type;
}

void GraphicsPathItem::set_type(int x)
{
	contour_type = x;
}

QPainterPath GraphicsPathItem::shape() const
{
	QPainterPathStroker s;
	s.setWidth(2);
	return s.createStroke(this->path());
}

void GraphicsPathItem::paint(
	QPainter * painter,
	const QStyleOptionGraphicsItem * option,
	QWidget * widget)
{
	if (option->state & QStyle::State_Selected)
	{
		QPen pen = this->pen();
		pen.setWidth(0);
		pen.setStyle(Qt::DashLine);
		painter->setPen(pen);
		painter->drawPath(this->path());
	}
	else
	{
		QGraphicsPathItem::paint(painter,option,widget);
	}
}

