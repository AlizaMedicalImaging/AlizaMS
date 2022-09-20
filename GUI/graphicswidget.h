#ifndef GRAPHICSWIDGET_H__
#define GRAPHICSWIDGET_H__

#include "graphicsview.h"
#include "structures.h"
#include "toolbox2D.h"
#include "sliderwidget.h"
#include <QWidget>
#include <QLabel>
#include <QList>
#include <QVector>
#include <QMap>
#include <QLineEdit>
#include <QThread>
#include <QPointF>
#include <QPen>
#include <QTimer>
#include <QMutex>
#include <QMouseEvent>
#include <QEvent>
#include <QCloseEvent>
#include <QProgressDialog>

class QGraphicsPathItem;
class Aliza;

class GraphicsWidget : public QWidget
{
Q_OBJECT
public:
	GraphicsWidget(
		short/*axis*/,
		bool/*OpenGL*/,
		QLabel* /*top*/,
		QLabel* /*left*/,
		QLabel* /*measurement*/,
		QLineEdit* /*value*/,
		QWidget* /*sinle frame*/,
		QWidget* /*multi frame*/);
	~GraphicsWidget();
	void set_axis(int); // 0 x, 1 y, 2 z
	void set_top_label(QLabel*);
	void set_left_label(QLabel*);
	void set_measure_label(QLabel*);
	void set_info_line(QLineEdit*);
	void set_aliza(Aliza*);
	int  get_axis() const { return axis; }
	void start_animation();
	void stop_animation();
	GraphicsView * graphicsview;
	ToolBox2D    * toolbox2D;
	SliderWidget * slider_m;
	std::vector<ProcessImageThread_*> threads_;
	std::vector<QThread*> threadsLUT_;
	void set_slice_2D(
		ImageVariant*,
		const short/*fit*/,
		const bool/*alw usregions*/,
		const bool=false/*frame level, to avoid check map twice*/);
	void set_toolbox2D_widget(ToolBox2D*);
	void set_sliderwidget(SliderWidget*);
	void update_image(
		const short /*fit*/,
		const bool /*redraw_contours*/,
		const bool /*lock*/,
		const bool=false/*frame level, to avoid check map twice*/);
	void clear_(bool=true);
	void update_frames();
	ImageContainer image_container;
	float get_offset_x();
	float get_offset_y();
	QString labels_to_contours(
		QProgressDialog*,
		int,int,
		const QMap< int, QVector<int> > &,
		const QMap< int, QString> &);
	void  set_bb(bool);
	bool  get_bb() const;
	bool  run__;
	void  update_selection_rectangle();
	void  update_pr_area();
	void  update_selection_item();
	void  set_top_label_text(const QString&);
	void  set_left_label_text(const QString&);
	void  set_info_line_text(const QString&);
	void  set_measure_text(const QString&);
	void  update_reconstructed_geometry(ImageVariant*);
	void  update_background_color();
	void  set_main();
	bool  is_main() const;
	void  set_multiview(bool);
	bool  is_multiview() const;
	void  set_smooth(bool);
	bool  get_smooth() const;
	void  update_measurement(double, double, double, double);
	void  set_mouse_modus(short,bool);
	short get_mouse_modus() const;
	void  set_show_cursor(bool);
	bool  get_show_cursor() const;
	void  update_pixel_value(double, double);
	void  update_pixel_value2(double, double);
	void  set_alt_mode(bool);
	bool  get_alt_mode() const;
	int   get_frametime_2D() const;
	QString contours_from_selected_paths(ImageVariant*, ROI*);
	double get_contours_width() const;
	bool get_enable_shutter() const;
	bool get_enable_overlays() const;
	void set_enable_shutter(bool);
	void set_enable_overlays(bool);

public slots:
	void set_frame_time_unit(bool);
	void get_screen();
	void set_frametime_2D(int);
	void show_image_info();
	void set_contours_width(double);

private slots:
	void animate_();

signals:
	void slice_changed(int);

protected:
	void closeEvent(QCloseEvent*) override;
	void leaveEvent(QEvent*) override;

private:
	short  axis;
	bool   main;
	bool   multi;
	bool   bb;
	bool   smooth_;
	bool   gl;
	short  mouse_modus;
	bool   enable_shutter;
	bool   enable_overlays;
	int    frame_time_unit;
	int    frametime_2D;
	double contours_width;
	mutable     QMutex mutex;
	QTimer    * anim2D_timer;
	QLabel    * top_label;
	QLabel    * left_label;
	QLabel    * measure_label;
	QLineEdit * info_line;
	Aliza     * aliza;
	QWidget * single_frame_ptr;
	QWidget * multi_frame_ptr;
	bool alt_mode;
	bool show_cursor;
};

#endif // GRAPHICSWIDGET_H__
