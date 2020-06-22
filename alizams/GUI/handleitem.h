#ifndef HANDLEITEM_H
#define HANDLEITEM_H
 
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>

class GraphicsView;

class HandleItem : public QGraphicsItem
{
public:
	enum HandleRole
	{
		RightHandle,
		TopHandle,
		LeftHandle,
		BottomHandle
	};
	HandleItem(GraphicsView*,QGraphicsRectItem*, HandleRole);
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
	QRectF boundingRect() const;
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent*);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
	QVariant itemChange(GraphicsItemChange, const QVariant &);
private:
	GraphicsView * view;
	QGraphicsRectItem * m_item;
	HandleRole m_role;
	bool m_pressed;
};
 
#endif // HANDLEITEM_H

