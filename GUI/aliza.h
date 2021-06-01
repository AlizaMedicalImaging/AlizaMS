#ifndef ALIZA_H
#define ALIZA_H

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt5-core.h"
#else
#include "CG/glwidget-qt5.h"
#endif
#else
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt4-core.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#endif
#include "structures.h"
#include <QThread>
#include <vector>
#include "toolbox.h"
#include "imagesbox.h"
#include "toolbox.h"
#include "browser/browserwidget2.h"
#include "settingswidget.h"
#include "graphicswidget.h"
#include "lutwidget.h"
#include "histogramview.h"
#include "sliderwidget.h"
#include "zrangewidget.h"
#include "animwidget.h"
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QList>
#include <QGraphicsView>
#include <QProgressDialog>
#include <QTimer>
#include <QTableWidgetItem>

class Aliza : public QObject
{
Q_OBJECT
public:
	Aliza();
	~Aliza();
	void close_();
	ImageVariant * get_image(int);
	const ImageVariant * get_image(int) const;
	int  get_selected_image_id();
	ImageVariant * get_selected_image();
	const ImageVariant * get_selected_image_const() const;
	void update_toolbox(const ImageVariant*);
	void set_imagesbox(ImagesBox*);
	void set_browser2(BrowserWidget2*);
	void set_settingswidget(SettingsWidget*);
	void set_toolbox(ToolBox*);
	void set_toolbox2D(ToolBox2D*);
	void set_glwidget(GLWidget*);
	void set_graphicswidget_m(GraphicsWidget*);
	void set_graphicswidget_y(GraphicsWidget*);
	void set_graphicswidget_x(GraphicsWidget*);
	void set_sliderwidget_m(SliderWidget*);
	void set_sliderwidget_y(SliderWidget*);
	void set_sliderwidget_x(SliderWidget*);
	void set_zrangewidget(ZRangeWidget*);
	void set_histogramview(HistogramView*);
	void set_lutwidget2(LUTWidget*);
	void set_trans3D_action(QAction*);
	void set_axis_actions(
		QAction*,
		QAction*,
		QAction*,
		QAction*,
		QAction*);
	void set_3D_views_actions(
		QAction*,
		QAction*,
		QAction*);
	void set_2D_views_actions(
		QAction*,
		QAction*,
		QAction*,
		QAction*,
		QAction*,
		QAction*);
	void set_anim3Dwidget(AnimWidget*);
	void set_anim2Dwidget(AnimWidget*);
	void connect_slots();
	QString get_opengl_info();
	void set_axis_2D(int, bool /*rectangle selection mode*/);
	void set_histogram();
	void set_axis_zyx(bool /*rectangle selection mode*/);
	void toggle_rect(bool);
	bool check_3d();
	void set_view2d_mouse_modus(short);
	void set_show_frames_3d(bool);
	void load_dicom_series(QProgressDialog*);
	void start_anim();
	void stop_anim();
	void zoom_plus_3d();
	void zoom_minus_3d();
	void set_hide_zoom(bool);
	void set_histogram_mode(bool);
	bool is_animation_running() const;
	int  get_num_images() const;
	QString create_group_(bool*,bool/*lock_mutex*/);
	void width_from_histogram_min(double);
	void width_from_histogram_max(double);
	void center_from_histogram(double);
	void flipX();
	void flipY();
	void reset_3d();
	void set_uniq_string(const QString &);
	void toggle_collisions(bool);
	void update_slice_from_animation(const ImageVariant*);
	void load_dicom_file(int*,const QString&,QProgressDialog*,bool);

public slots:
	void delete_image();
	void delete_checked_images();
	void delete_unchecked_images();
	void clear_ram();
	void center_from_spinbox(double);
	void width_from_spinbox(double);
	void set_lut_function1(int);
	void set_from_slice(int);
	void set_to_slice(int);
	void set_lut(int);
	void set_selected_slice2D_m(int);
	void set_selected_slice2D_y(int);
	void set_selected_slice2D_x(int);
	void calculate_bb();
	void set_transparency(bool);
	void update_selection();
	void update_selection2();
	void update_visible_rois(QTableWidgetItem*);
	void reset_rect2();
	void reset_level();
	void trigger_reload_histogram();
	void create_group();
	void remove_group();
	void start_3D_anim();
	void stop_3D_anim();
	void animate_();
	void set_frametime_3D(int);
	void toggle_maxwindow(bool);
	void trigger_check_all();
	void trigger_uncheck_all();
	void trigger_tmp();
	void toggle_zlock(bool);
	void toggle_zlock_one(bool);
	void trigger_image_color();
	void toggle_lock_window(bool);
	void trigger_show_roi_info();

signals:
	void report_load_to_mainwin();
	void image_opened();

private:
	GLWidget       * glwidget;
	ImagesBox      * imagesbox;
	ToolBox        * toolbox;
	ToolBox2D      * toolbox2D;
	BrowserWidget2 * browser2;
	SettingsWidget * settingswidget;
	GraphicsWidget * graphicswidget_m;
	GraphicsWidget * graphicswidget_y;
	GraphicsWidget * graphicswidget_x;
	SliderWidget   * slider_m;
	SliderWidget   * slider_y;
	SliderWidget   * slider_x;
	ZRangeWidget   * zrangewidget;
	LUTWidget      * lutwidget2;
	HistogramView  * histogramview;
	AnimWidget     * anim3Dwidget;
	AnimWidget     * anim2Dwidget;
	QAction * graphicsAct_Z;
	QAction * graphicsAct_Y;
	QAction * graphicsAct_X;
	QAction * zyxAct;
	QAction * histogramAct;
	QAction * slicesAct;
	QAction * zlockAct;
	QAction * oneAct;
	QAction * trans3DAct;
	QAction * frames2DAct;
	QAction * distanceAct;
	QAction * rectAct;
	QAction * cursorAct;
	QAction * collisionAct;
	QAction * segmentAct;
	QIcon trans_icon;
	QIcon notrans_icon;
	QIcon cut_icon;
	QIcon nocut_icon;
	QIcon anchor_icon;
	QIcon anchor2_icon;
  	bool rect_selection;
	bool hide_zoom;
	bool multiview;
	bool histogram_mode;
	bool run__;
	bool load_reported_to_mainwin;
	int anim_idx;
	short saved_mouse_modus;
	bool saved_show_cursor;
	int frametime_3D;
	QString uniq_string;
	QTimer * anim3D_timer;
	QProgressDialog * create_filters_progress();
	QProgressDialog * create_filters_progress2();
	void close_filters_progress(QProgressDialog*);
	void connect_tools();
	void disconnect_tools();
	void reload_3d(
		ImageVariant*,
		bool=true,
		bool=false,
		bool=false,
		bool=false,
		bool=false);
	bool load_3d(
		ImageVariant*,bool=false,bool=false,bool=false,bool=false);
	void update_center(ImageVariant*);
	void add_histogram(ImageVariant*,QProgressDialog*,bool=true);
	void delete_image2(ImageVariant*);
	void update_group_center(const ImageVariant*);
	void update_group_width(const ImageVariant*);
	void set_us_center(ImageVariant*,double);
	void set_us_width(ImageVariant*,double);
	void set_lut_function0(int);
	void update_selection_common1(ImageVariant*);
	void update_selection_common2(QListWidgetItem*);
	void clear_views();
	void sort_4d(
		QList<ImageVariant*> &,
		QList<double> &,
		QMap<int, ImageVariant*> &,
		QMap<int, ImageVariant*> &,
		QMap<qlonglong, ImageVariant*> &,
		const int,
		const bool,
		const int=0,
		const int=0,
		const int=0);
	void delete_checked_unchecked(bool);
	void clear_contourstable();
	void set_contourstable(const ImageVariant*);
};

#endif // ALIZA_H
