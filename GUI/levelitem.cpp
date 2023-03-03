#include "histogramview.h"
#include "levelitem.h"
#include <QPainter>
#include <QPointF>
#include <cmath>
#include <iostream>

LevelItem::LevelItem(QGraphicsRectItem * item, int role, HistogramView * v)
	: QGraphicsItem(NULL)
{
	if (role == 0) m_role = LeftHandle;
	else           m_role = RightHandle;
	view = v;
	m_item = item;
	m_pressed = false;
	setZValue(1);
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
	setCursor(Qt::SizeHorCursor);
}
 
void LevelItem::paint(QPainter * paint, const QStyleOptionGraphicsItem *, QWidget *)
{
	//paint->drawRect(boundingRect());
}

QRectF LevelItem::boundingRect() const
{
	QRectF r = QRectF();
	switch (m_role)
	{
	case LeftHandle:
		{
			QPointF point(
				m_item->boundingRect().left()-pos().x(),
				m_item->boundingRect().top() + m_item->boundingRect().height() / 2.0);
			r = QRectF(
					point-QPointF(12.0, m_item->boundingRect().height() / 2.0),
					QSize(24, m_item->boundingRect().height()));
		}
		break;
	case RightHandle:
		{
			QPointF point(
				m_item->boundingRect().right()-pos().x(),
				m_item->boundingRect().top() + m_item->boundingRect().height() / 2.0);
			r = QRectF(
				point-QPointF(12.0, m_item->boundingRect().height() / 2.0),
				QSize(24, m_item->boundingRect().height()));
		}
		break;
	}
	return r;
}

QVariant LevelItem::itemChange(GraphicsItemChange change, const QVariant & data)
{
	if (change == ItemPositionChange && m_pressed)
	{
		QPointF movement;
		QPointF newData = data.toPointF();
		QRectF  newRect = m_item->rect();
		newData.setY(0);
		double dx = newData.x() - pos().x();
		switch (m_role)
		{
		case LeftHandle:
			{
				if ((m_item->rect().width() - dx) <= view->scene()->sceneRect().width())
				{
					newRect.setLeft(
						((m_item->rect().left() + dx) < m_item->rect().right())
							? m_item->rect().left() + dx
							: m_item->rect().right());
					m_item->setRect(newRect);
					view->window_width_update_min(newRect.width());
				}
			}
			break;
		case RightHandle:
			{
				if ((m_item->rect().width() + dx) <= view->scene()->sceneRect().width())
				{
					newRect.setRight(
						((m_item->rect().right() + dx) > m_item->rect().left())
							? m_item->rect().right() + dx
							: m_item->rect().left());
					m_item->setRect(newRect);
					view->window_width_update_max(newRect.width());
				}
			}
			break;
		}
		//
		return QGraphicsItem::itemChange(change, newData);
	}
	return QGraphicsItem::itemChange(change, data);
}
 
void LevelItem::mousePressEvent(QGraphicsSceneMouseEvent * e)
{
	m_pressed = true;
	QGraphicsItem::mousePressEvent(e);
}
 
void LevelItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
	m_pressed = false;
	QGraphicsItem::mouseReleaseEvent(e);
}

