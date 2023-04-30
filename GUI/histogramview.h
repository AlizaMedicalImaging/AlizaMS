#ifndef A_HISTOGRAMVIEW_H
#define A_HISTOGRAMVIEW_H

#include <QObject>
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QKeyEvent>

class ImageVariant;
class LevelRectItem;

class HistogramView : public QGraphicsView
{
Q_OBJECT
public:
	HistogramView(QWidget*,QObject*,QWidget*,bool);
	~HistogramView();
	void update__(const ImageVariant*);
	void update_window(const ImageVariant*);
	void clear__();
	void window_width_update_min(qreal);
	void window_width_update_max(qreal);
	void window_center_update(qreal);
	void update_bgcolor();

public slots:
	void get_screen();

protected:
	void drawBackground(QPainter*, const QRectF&) override;
	void drawForeground(QPainter*, const QRectF&) override;
	void keyPressEvent(QKeyEvent*) override;
	void keyReleaseEvent(QKeyEvent*) override;

private:
	QGraphicsPixmapItem * pixmap;
	LevelRectItem * rect_item;
	QObject * aliza;
	QWidget * multi_frame_ptr;
};

class LevelRectItem : public QGraphicsRectItem
{
public:
	enum { Type = QGraphicsItem::UserType + 4 };
	LevelRectItem(HistogramView*, qreal, qreal, qreal, qreal, QGraphicsItem(*) = nullptr);
	~LevelRectItem();
	qreal  get_width() const;
	void   set_width(double);
	QRectF boundingRect() const override;

protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &) override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
	void keyPressEvent(QKeyEvent*) override;
	int  type() const override;

private:
	HistogramView * view;
	qreal pwidth;
};

#endif

