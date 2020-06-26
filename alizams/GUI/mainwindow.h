#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGlobal>
#include "ui_mainwindow.h"
#if QT_VERSION >= 0x050000
#include "CG/glwidget-qt5.h"
#else
#include "CG/glwidget-qt4.h"
#include <QGLFormat>
#endif
#include "aliza.h"
#include <QMainWindow>
#include <QStringList>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QResizeEvent>
#include <QSize>
#include <QPoint>
#include "zoomwidget.h"
#include "aboutwidget.h"
#include "browser/sqtree.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(
		bool /*3D*/,
		bool /*hide zoom*/
		);
	~MainWindow();
	void open_args(const QStringList&);
	QAction * graphicsAct_Z;
	QAction * graphicsAct_Y;
	QAction * graphicsAct_X;
	QAction * histogramAct;
	QAction * zyxAct;
	QAction * slicesAct;
	QAction * trans3DAct;
	QAction * settingsAct;
	QAction * gloptionsAct;
	QAction * animAct2d;
	QAction * animAct3d;

protected:
	void closeEvent(QCloseEvent*);
	void dropEvent(QDropEvent*);
	void dragEnterEvent(QDragEnterEvent*);
	void dragMoveEvent(QDragMoveEvent*);
	void dragLeaveEvent(QDragLeaveEvent*);
	void resizeEvent(QResizeEvent*);
	void readSettings();
	void writeSettings();

public slots:
	void exit_null();

private slots:
	void about();
	void toggle_browser();
	void toggle_settingswidget();
	void toggle_showgl(bool);
	void toggle_show2D(bool);
	void toggle_graphicswidget_m_x(bool);
	void toggle_graphicswidget_m_y(bool);
	void toggle_graphicswidget_m_z(bool);
	void toggle_histogram(bool);
	void toggle_graphicswidget_zyx(bool);
	void load_any();
	void toggle_toolbox();
	void toggle_meta();
	void toggle_meta2();
	void toggle_rect(bool);
	void toggle_cursor(bool);
	void toggle_collisions(bool);
	void toggle_segmentation(bool);
	void set_view_3d(bool);
	void set_view_rc(bool);
	void set_show_frames_3d(bool);
	void load_dicom_series2();
	void reset_rect2();
	void start_anim();
	void stop_anim();
	void zoom_plus_2d();
	void zoom_minus_2d();
	void zoom_plus_3d();
	void zoom_minus_3d();
	void start_3D_anim();
	void stop_3D_anim();
	void toggle_animwidget2d(bool);
	void toggle_animwidget3d(bool);
	void set_ui();
	void flipX();
	void flipY();
	void reset_3d();
	void set_show_frames_2d(bool);
	void toggle_distance(bool);
	void trigger_set_level();
	void tab_ind_changed(int);
	void set_zlock(bool);
	void set_zlock_one(bool);

private:
	void createActions();
	void createMenus();
	void createToolBars();
	void load_dicom_dir();
	void desktop_layout(int*,int*);
	void load_any_file(
		const QString&,
		QProgressDialog*,
		bool/*lock*/);
	//
	QSize  mainwindow_size;
	QPoint mainwindow_pos;
	float  scale_icons;
	float  adjust_scale_icons;
	//
	Aliza            * aliza;
	GLWidget         * glwidget;
	ToolBox          * toolbox;
	ImagesBox        * imagesbox;
	BrowserWidget2   * browser2;
	SQtree           * sqtree;
	SettingsWidget   * settingswidget;
	GraphicsWidget   * graphicswidget_m;
	GraphicsWidget   * graphicswidget_y;
	GraphicsWidget   * graphicswidget_x;
	SliderWidget     * slider_m;
	SliderWidget     * slider_y;
	SliderWidget     * slider_x;
	ZRangeWidget     * zrangewidget;
	ToolBox2D        * toolbox2D;
	LabelWidget      * labelwidget;
	LUTWidget        * lutwidget;
	LUTWidget        * lutwidget2;
	ZoomWidget	     * zoomwidget2D;
	ZoomWidget       * zoomwidget3D;
	HistogramView    * histogramview;
	AnimWidget       * anim3Dwidget;
	AnimWidget       * anim2Dwidget;
	AboutWidget      * aboutwidget;
	bool saved_ok3d;
	bool first_image_loaded;
	bool hide_zoom;
	QMenu * file_menu;
	QMenu * views_menu;
	QMenu * tools_menu;
	QMenu * browser_menu;
	QMenu * metadata_menu;
	QMenu * settings_menu;
	QToolBar * toolbar2;
	QToolBar * toolbar3;
	QToolBar * toolbar4;
	QToolBar * toolbar5;
	QLabel   * view2d_label;
	QLabel   * view3d_label;
	QAction * openAct;
	QAction * openanyAct;
	QAction * exitAct;
	QAction * aboutAct;
	QActionGroup * axis_group;
	QAction * show2DAct;
	QAction * show3DAct;
	QAction * rectAct;
	QAction * cursorAct;
	QAction * collisionAct;
	QAction * distanceAct;
	QAction * transp2dAct;
	QAction * raycastAct;
	QActionGroup * view_group;
	QAction  * frames2DAct;
	QAction  * frames3DAct;
	QAction  * resetRectAct2;
	QAction  * flipXAct;
	QAction  * flipYAct;
	QAction  * reset3DAct;
	QAction  * showMetadata;
	QAction  * zrangeAct;
	QAction  * setLevelAct;
	QAction  * setSeedsAct;
	QAction  * actionViews2DMenu;
	QAction  * actionViews3DMenu;
	QAction  * zlockAct;
	QAction  * oneAct;
	QAction  * browser_open_dir_act;
	QAction  * browser_open_dcmdir_act;
	QAction  * browser_reload_act;
	QAction  * browser_metadata_act;
	QAction  * browser_copy_act;
	QAction  * browser_load_act;
	QAction  * meta_open_act;
	QVBoxLayout * frame2D_viewerZ_layout;
	QVBoxLayout * frame2D_viewer_layout;
	QVBoxLayout * histogram_frame_layout;
	QVBoxLayout * histogram_frame2_layout;
	QVBoxLayout * slider_frame_layout;
	QVBoxLayout * slider_frame_layoutZ;
	QLabel  * anim_label;
};

#endif
