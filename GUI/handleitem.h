#ifndef A_HANDLEITEM_H
#define A_HANDLEITEM_H
 
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
	HandleItem(GraphicsView*, QGraphicsRectItem*, HandleRole);
	QRectF boundingRect() const override;

protected:
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
	QVariant itemChange(GraphicsItemChange, const QVariant &) override;

private:
	GraphicsView * view;
	QGraphicsRectItem * m_item;
	HandleRole m_role;
	bool m_pressed;
};
 
#endif

