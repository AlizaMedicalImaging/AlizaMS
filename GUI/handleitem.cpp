#include "handleitem.h"
#include <QPainter>
#include <QPointF>
#include <cmath>
#include <iostream>
#include "graphicsview.h"

HandleItem::HandleItem(
	GraphicsView * v,
	QGraphicsRectItem * item,
	HandleItem::HandleRole role)
	:
	QGraphicsItem(),
	view(v),
	m_item(item),
	m_role(role)
{
	setZValue(1);
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	switch (role)
	{
	case LeftHandle:
		setCursor(Qt::SizeHorCursor);
		break;
	case RightHandle:
		setCursor(Qt::SizeHorCursor);
		break;
	case TopHandle:
		setCursor(Qt::SizeVerCursor);
		break;
	case BottomHandle:
		setCursor(Qt::SizeVerCursor);
		break;
	default:
		break;
	}
}

void HandleItem::paint(
	QPainter * paint,
	const QStyleOptionGraphicsItem *,
	QWidget *)
{
#if 0
	paint->drawRect(boundingRect());
#endif
}

QRectF HandleItem::boundingRect() const
{
	QRectF r = QRectF();
	switch (m_role)
	{
	case LeftHandle:
		{
			QPointF point(
				m_item->boundingRect().left() -
					pos().x(),
				m_item->boundingRect().top() +
					m_item->boundingRect().height() / 2.0);
			r = QRectF(
				point-QPointF(
					18.0,
					m_item->boundingRect().height() / 2.0),
				QSize(36, m_item->boundingRect().height()));
		}
		break;
	case RightHandle:
		{
			QPointF point(
				m_item->boundingRect().right() -
					pos().x(),
				m_item->boundingRect().top() +
					m_item->boundingRect().height() / 2.0);
			r = QRectF(
				point-QPointF(
					18.0,
					m_item->boundingRect().height() / 2.0),
				QSize(36, m_item->boundingRect().height()));
		}
		break;
	case TopHandle:
		{
			QPointF point(
				m_item->boundingRect().left() +
					m_item->boundingRect().width() / 2.0,
				m_item->boundingRect().top() -
					pos().y());
			r = QRectF(
				point-QPointF(
					m_item->boundingRect().width() / 2.0,
					18.0),
				QSize(m_item->boundingRect().width(), 36));
		}
		break;
	case BottomHandle:
		{
			QPointF point(
				m_item->boundingRect().left() +
					m_item->boundingRect().width() / 2.0,
				m_item->boundingRect().bottom() -
					pos().y());
			r = QRectF(
				point-QPointF(
					m_item->boundingRect().width() / 2.0,
					18.0),
				QSize(m_item->boundingRect().width(), 36));
		}
		break;
	default: break;
	}
	return r;
}

QVariant HandleItem::itemChange(
	GraphicsItemChange change,
	const QVariant & data)
{
	if (m_pressed && change == ItemPositionChange)
	{
		QPointF movement;
		QPointF newData = data.toPointF();
		QRectF  newRect = m_item->rect();
		switch (m_role)
		{
		case LeftHandle:
			{
				newData.setY(0);
				movement = newData - pos();
				newRect.setLeft(
					m_item->rect().left()+movement.x()
						< m_item->rect().right()
					?
					m_item->rect().left()+movement.x()
					:
					m_item->rect().right());
			}
			break;
		case RightHandle:
			{
				newData.setY(0);
				movement = newData - pos();
				newRect.setRight(
					m_item->rect().right()+movement.x()
						> m_item->rect().left()
					?
					m_item->rect().right()+movement.x()
					:
					m_item->rect().left());
			}
			break;
		case TopHandle:
			{
				newData.setX(0);
				movement = newData - pos();
				newRect.setTop(
					m_item->rect().top()+movement.y()
						< m_item->rect().bottom()
					?
					m_item->rect().top()+movement.y()
					:
					m_item->rect().bottom());
			}
			break;
		case BottomHandle:
			{
				newData.setX(0);
				movement = newData - pos();
				newRect.setBottom(
					m_item->rect().bottom()+movement.y()
						> m_item->rect().top()
					?
					m_item->rect().bottom()+movement.y()
					:
					m_item->rect().top());
			}
			break;
		default: break;
		}
		m_item->setRect(newRect);
		if (view) view->emit_bb_update();
		return QGraphicsItem::itemChange(change, newData);
	}
	return QGraphicsItem::itemChange(change, data);
}

void HandleItem::mousePressEvent(
	QGraphicsSceneMouseEvent * e)
{
	m_pressed = true;  
	QGraphicsItem::mousePressEvent(e);
}

void HandleItem::mouseReleaseEvent(
	QGraphicsSceneMouseEvent * e)
{
	m_pressed = false;
	QGraphicsItem::mouseReleaseEvent(e);
}

