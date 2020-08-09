#ifndef RECTITEM_H
#define RECTITEM_H

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include "handleitem.h"
class RectItem : public QGraphicsRectItem
{
public:
    enum { Type = QGraphicsItem::UserType + 3 };
    RectItem(qreal, qreal, qreal, qreal, QGraphicsItem(*)=NULL);
    ~RectItem();
	void set_pen1(
		qreal,
		unsigned int=0xbc,
		unsigned int=0x86,
		unsigned int=0x2b);
	void set_pen2(
		qreal,
		unsigned int=0xbc,
		unsigned int=0x86,
		unsigned int=0x2b);
	qreal  get_width() const;
	void   set_width(double);
	short  get_current_pen() const;
	QRectF boundingRect() const;
protected:
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void keyPressEvent(QKeyEvent*);
    int  type() const;
private:
	qreal pwidth;
	short current_pen;
};

#endif // RECTITEM_H
