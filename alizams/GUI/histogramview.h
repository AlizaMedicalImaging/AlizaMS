#ifndef HISTOGRAMVIEW_H__
#define HISTOGRAMVIEW_H__

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
	void drawBackground(QPainter*, const QRectF&);
	void drawForeground(QPainter*, const QRectF&);
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);
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
    LevelRectItem(HistogramView*, qreal, qreal, qreal, qreal, QGraphicsItem(*)=NULL);
    ~LevelRectItem();
	qreal  get_width() const;
	void   set_width(double);
	QRectF boundingRect() const;
protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void keyPressEvent(QKeyEvent*);
    int  type() const;
private:
	HistogramView * view;
	qreal pwidth;
};

#endif // HISTOGRAMVIEW_H__

