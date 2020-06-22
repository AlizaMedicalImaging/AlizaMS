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
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
	QRectF boundingRect() const;
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent*);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
	QVariant itemChange(GraphicsItemChange, const QVariant &);
private:
	HistogramView * view;
	QGraphicsRectItem * m_item;
	HandleRole m_role;
	bool m_pressed;
};
 
#endif // LEVELITEM_H

