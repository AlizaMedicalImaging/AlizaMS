#ifndef LEVELITEM_H
#define LEVELITEM_H
 
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>

class HistogramView;

class LevelItem : public QGraphicsItem
{
public:
	enum HandleRole {LeftHandle,RightHandle};
	LevelItem(QGraphicsRectItem*,int,HistogramView*);
	QRectF boundingRect() const override;

protected:
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
	QVariant itemChange(GraphicsItemChange, const QVariant &) override;

private:
	HistogramView * view;
	QGraphicsRectItem * m_item;
	HandleRole m_role;
	bool m_pressed;
};
 
#endif // LEVELITEM_H

