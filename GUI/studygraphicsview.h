#ifndef STUDYGRAPHICSVIEW_H
#define STUDYGRAPHICSVIEW_H

#define DELETE_STUDYGRAPHICSIMAGEITEM

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QImage>
#include <QWidget>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QMap>
#include <QList>

class ImageVariant;
class StudyGraphicsWidget;

class StudyGraphicsView : public QGraphicsView
{
Q_OBJECT
public:
    StudyGraphicsView(StudyGraphicsWidget*);
    ~StudyGraphicsView();
	QGraphicsPixmapItem * image_item;
	QGraphicsPathItem   * measurment_line;
	QGraphicsRectItem   * pr_area;
	QList<QGraphicsPathItem*>    collision_paths;
	QList<QGraphicsPathItem*>    us_regions;
	QList<QGraphicsTextItem*>    prtexts;
	QList<QGraphicsPathItem*>    prtextanchors;
	QList<QGraphicsPathItem*>    prpolylines;
	QList<QGraphicsPathItem*>    printerpolated;
	QList<QGraphicsEllipseItem*> prcircles;
	QList<QGraphicsPathItem*>    prellipses;
	QList<QGraphicsEllipseItem*> prpoints;
	QList<QGraphicsPathItem*>    display_shutters;
    void scale_view(double);
	void clear_collision_paths();
	void clear_us_regions();
	void clear_prtexts_items();
	void clear_prgraphicobjects_items();
	void clear_shutters();
	double m_scale;
	bool global_flip_x;
	bool global_flip_y;
	void update_background_color();
	void draw_frames(short);
	void draw_us_regions();
	void set_empty_distance();
	void set_widget_m(StudyGraphicsWidget*);
	void draw_prtexts(const ImageVariant*);
	void draw_prgraphics(const ImageVariant*);
	void draw_shutter(const ImageVariant*);

public slots:
    void zoom_in();
    void zoom_out();

protected:
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void wheelEvent(QWheelEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseDoubleClickEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	void drawBackground(QPainter*, const QRectF&) override;
	void drawForeground(QPainter*, const QRectF&) override;

private:
	int old_win_pos_x;
	int old_win_pos_y;
	int new_win_pos_x;
	int new_win_pos_y;
	int last_win_pos_x;
	int last_win_pos_y;
	double m0_win_pos_x;
	double m0_win_pos_y;
	double m1_win_pos_x;
	double m1_win_pos_y;
	bool m0_set;
	bool m1_set;
	StudyGraphicsWidget * parent;
	QColor shutter_color;
	void set_win_old_position(int, int);
	void set_win_new_position(int, int);
	void set_win_last_position(int, int);
	void measure();
	void flipX();
	void flipY();
};

#endif // STUDYGRAPHICSVIEW_H
