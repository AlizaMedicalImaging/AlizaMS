#ifndef A_RECTITEM_H
#define A_RECTITEM_H

#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include "handleitem.h"

class RectItem : public QGraphicsRectItem
{
public:
	enum { Type = QGraphicsItem::UserType + 3 };
	RectItem(qreal, qreal, qreal, qreal, QGraphicsItem(*) = nullptr);
	~RectItem() = default;
	void set_pen1(
		qreal,
		unsigned int = 0xbc,
		unsigned int = 0x86,
		unsigned int = 0x2b);
	void set_pen2(
		qreal,
		unsigned int = 0xbc,
		unsigned int = 0x86,
		unsigned int = 0x2b);
	qreal  get_width() const;
	void   set_width(double);
	short  get_current_pen() const;
	QRectF boundingRect() const override;

protected:
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
	void keyPressEvent(QKeyEvent*) override;
	int  type() const override;

private:
	qreal pwidth;
	short current_pen;
};

#endif

