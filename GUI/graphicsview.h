#ifndef A_GRAPHICSVIEW_H
#define A_GRAPHICSVIEW_H

#define DELETE_GRAPHICSIMAGEITEM

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
#include "rectitem.h"
#include "handleitem.h"
#include "graphicspathitem.h"

class ImageVariant;
class GraphicsWidget;

class GraphicsView : public QGraphicsView
{
Q_OBJECT
public:
    GraphicsView(GraphicsWidget*);
    ~GraphicsView();
	QGraphicsPixmapItem * image_item;
	RectItem            * handle_rect;
	RectItem            * selection_item;
	QGraphicsPathItem   * line_x;
	QGraphicsPathItem   * line_y;
	QGraphicsPathItem   * line_z;
	QGraphicsPathItem   * measurment_line;
	QGraphicsRectItem   * pr_area;
	QGraphicsPathItem   * paint_brush;
	QList<GraphicsPathItem*>     paths;
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
	void update_selection_rect_width();
	void clear_paths();
	void clear_collision_paths();
	void clear_us_regions();
	void clear_prtexts_items();
	void clear_prgraphicobjects_items();
	void clear_shutters();
	int m_angle;
	double m_scale;
	bool global_flip_x;
	bool global_flip_y;
	void emit_bb_update();
	void update_background_color();
	void draw_frames(short);
	void draw_us_regions();
	void set_empty_lines();
	void set_empty_distance();
	void set_widget_m(GraphicsWidget*);
	void set_widget_y(GraphicsWidget*);
	void set_widget_x(GraphicsWidget*);
	void set_handleitems_cursors(bool);
	void draw_prtexts(const ImageVariant*);
	void draw_prgraphics(const ImageVariant*);
	void draw_shutter(const ImageVariant*);

public slots:
    void zoom_in();
    void zoom_out();
	void flip();
	void animate_flip();

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

signals:
	void bb_changed();

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
	GraphicsWidget * parent;
	GraphicsWidget * widget_m;
	GraphicsWidget * widget_y;
	GraphicsWidget * widget_x;
	HandleItem * topHandle;
	HandleItem * bottomHandle;
	HandleItem * leftHandle;
	HandleItem * rightHandle;
	QColor shutter_color;
	void set_win_old_position(int, int);
	void set_win_new_position(int, int);
	void set_win_last_position(int, int);
	void lines_at(double,double);
	void measure();
	void get_pixel_value(double,double);
	void get_pixel_value2(double,double);
	void flipX();
	void flipY();
};

#endif
