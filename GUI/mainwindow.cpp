#include "mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QTextStream>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMimeData>
#include <QUrl>
#include <QFileDialog>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QScreen>
#else
#include <QDesktopWidget>
#endif
#include <QDateTime>
#ifndef WIN32
#include <unistd.h>
#endif
#include "commonutils.h"

#define DISABLE_LEFT_TOOLBAR__

QMutex mutex;

MainWindow::MainWindow(
	bool ok3d,
	bool hide_zoom_)
{
	setupUi(this);
	setDocumentMode(false);
	scale_icons = 1.0f;
	adjust_scale_icons = 1.0f;
	hide_gl3_frame_later = false;
	saved_ok3d = false;
	int dock_area = 2;
	QString saved_style;
	{
		QSettings settings(
			QSettings::IniFormat, QSettings::UserScope,
			QApplication::organizationName(), QApplication::applicationName());
		settings.beginGroup(QString("GlobalSettings"));
		adjust_scale_icons =
			(float)settings.value(QString("scale_ui_icons"), 1.0).toDouble();
		saved_style =
			settings.value(
				QString("stylename"),
				QVariant(QString("Dark Fusion"))).toString();
		settings.endGroup();
		int width_ = 0, height_ = 0;
		desktop_layout(&width_,&height_);
		const int w = static_cast<int>(static_cast<double>(width_)*0.7);
		const int h = static_cast<int>(static_cast<double>(height_)*0.7);
		settings.beginGroup(QString("MainWindow"));
		dock_area = settings.value(QString("dock_area"), 2).toInt();
		resize(settings.value(QString("size"), QSize(w,h)).toSize());
		move(settings.value(QString("pos"), QPoint(50,50)).toPoint());
#ifdef USE_WORKSTATION_MODE
		CommonUtils::set_open_dir(settings.value(QString("open_dir"), QString("")).toString());
#endif
		settings.endGroup();
	}
	dockWidget1 = new QDockWidget(this);
	dockWidget1->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
	dockWidget1->setFloating(false);
	dockWidget1->setFeatures(QDockWidget::DockWidgetMovable);
	addDockWidget(
		(dock_area == 2) ? Qt::RightDockWidgetArea : Qt::LeftDockWidgetArea,
		dockWidget1);
	imagesbox_frame1 = new QFrame(this);
	imagesbox_frame1->setFrameShape(QFrame::NoFrame);
	imagesbox_frame1->setFrameShadow(QFrame::Plain);
	imagesbox = new ImagesBox(scale_icons*adjust_scale_icons);
	if (saved_style != QString("Dark Fusion"))
	{
		update_info_lines_bg();
		imagesbox->update_background_color(false);
	}
	QVBoxLayout * l1 = new QVBoxLayout(imagesbox_frame1);
	l1->setSpacing(0);
	l1->setContentsMargins(0,0,0,0);
	l1->addWidget(imagesbox);
	dockWidget1->setWidget(imagesbox_frame1);
	imagesbox_frame1->hide();
	//
	toolbox3D_frame->hide();
	anim2D_frame->hide();
	anim3D_frame->hide();
	view2d_frame->hide();
	view3d_frame->hide();
	annotations_frame->hide();
	info_line->hide();
	view2d_label = new QLabel(this);
	view3d_label = new QLabel(this);
	//
	histogram_frame->hide();
	//
	hide_zoom = hide_zoom_;
	//
	const bool force_vertical = false;
	int swidth = 0;
	int sheight = 0;
	desktop_layout(&swidth, &sheight);
	if (force_vertical || (sheight > swidth))
	{
		QVBoxLayout * vl991 = new QVBoxLayout(views_frame);
		vl991->setContentsMargins(0,0,0,0);
		vl991->setSpacing(0);
		vl991->addWidget(frame2D);
		vl991->addWidget(frame3D);
#if 1
		if (sheight > 1920) scale_icons = sheight/1920.0f;
#endif
	}
	else
	{
		QHBoxLayout * vl991 = new QHBoxLayout(views_frame);
		vl991->setContentsMargins(0,0,0,0);
		vl991->setSpacing(0);
		vl991->addWidget(frame2D);
		vl991->addWidget(frame3D);
#if 1
		if (swidth > 1920) scale_icons = swidth/1920.0f;
#endif
	}
	aboutwidget = new AboutWidget(swidth, sheight);
	aboutwidget->hide();
	//
	toolbox = new ToolBox();
	QVBoxLayout * vl0 = new QVBoxLayout(toolbox3D_frame);
	vl0->setContentsMargins(0,0,0,0);
	vl0->setSpacing(0);
	vl0->addWidget(toolbox);
	//
	anim3Dwidget = new AnimWidget(scale_icons*adjust_scale_icons);
	anim3Dwidget->label->setText(QString("3D+t"));
	QVBoxLayout * vl98 = new QVBoxLayout(anim3D_frame);
	vl98->setContentsMargins(0,0,0,0);
	vl98->setSpacing(0);
	vl98->addWidget(anim3Dwidget);
	//
	anim2Dwidget = new AnimWidget(scale_icons*adjust_scale_icons);
	anim2Dwidget->label->setText(QString("2D+t"));
	anim2Dwidget->group_pushButton->hide();
	anim2Dwidget->remove_pushButton->hide();
	anim2Dwidget->t_checkBox->hide();
	QVBoxLayout * vl99 = new QVBoxLayout(anim2D_frame);
	vl99->setContentsMargins(0,0,0,0);
	vl99->setSpacing(0);
	vl99->addWidget(anim2Dwidget);
	//
	toolbox2D = new ToolBox2D(scale_icons*adjust_scale_icons);
	QVBoxLayout * l2 = new QVBoxLayout(level_frame);
	l2->setSpacing(0);
	l2->setContentsMargins(0,0,0,0);
	l2->addWidget(toolbox2D);
	//
	lutwidget2 = new LUTWidget(scale_icons*adjust_scale_icons);
	lutwidget2->add_items1();
	//
	if (hide_zoom)
	{
		zoomwidget2D = NULL;
		zoomwidget3D = NULL;
	}
	else
	{
		zoomwidget2D = new ZoomWidget(scale_icons*adjust_scale_icons);
		zoomwidget3D = new ZoomWidget(scale_icons*adjust_scale_icons);
	}
	//
	frame2D_viewer_layout = new QVBoxLayout(frame2D_viewer);
	frame2D_viewer_layout->setSpacing(0);
	frame2D_viewer_layout->setContentsMargins(0,0,0,0);
	//
	frame2D_viewerZ_layout = new QVBoxLayout(frame2D_viewerZ);
	frame2D_viewerZ_layout->setSpacing(0);
	frame2D_viewerZ_layout->setContentsMargins(0,0,0,0);
	//
	slider_m  = new SliderWidget();
	slider_frame_layout = new QVBoxLayout(slider_frame);
	slider_frame_layout->setSpacing(0);
	slider_frame_layout->setContentsMargins(0,0,0,0);
	slider_frame_layoutZ = new QVBoxLayout(slider_frameZ);
	slider_frame_layoutZ->setSpacing(0);
	slider_frame_layoutZ->setContentsMargins(0,0,0,0);
	//
	slider_y  = new SliderWidget();
	QVBoxLayout * l10 = new QVBoxLayout(slider_frameY);
	l10->setSpacing(0);
	l10->setContentsMargins(0,0,0,0);
	l10->addWidget(slider_y);
	//
	slider_x  = new SliderWidget();
	QVBoxLayout * l11 = new QVBoxLayout(slider_frameX);
	l11->setSpacing(0);
	l11->setContentsMargins(0,0,0,0);
	l11->addWidget(slider_x);
	//
	graphicswidget_m  = new GraphicsWidget(
		2,
		false,
		top_label, left_label,
		measure_label,
		info_line,
		graphicswidget_frame, multi_frame);
	frame2D_viewer_layout->addWidget(graphicswidget_m);
	slider_frame_layout->addWidget(slider_m);
	multi_frame->hide();
	//
	graphicswidget_m->set_toolbox2D_widget(toolbox2D);
	graphicswidget_m->set_sliderwidget(slider_m);
	graphicswidget_m->set_main();
	//
	graphicswidget_y  = new GraphicsWidget(
		1,
		false,
		top_labelY, left_labelY,
		measure_labelY,
		info_lineY,
		graphicswidget_frame, multi_frame);
	QVBoxLayout * l7 = new QVBoxLayout(frame2D_viewerY);
	l7->setSpacing(0);
	l7->setContentsMargins(0,0,0,0);
	l7->addWidget(graphicswidget_y);
	graphicswidget_y->set_sliderwidget(slider_y);
	graphicswidget_y->set_toolbox2D_widget(toolbox2D);
	//
	graphicswidget_x  = new GraphicsWidget(
		0,
		false,
		top_labelX,
		left_labelX,
		measure_labelX,
		info_lineX,
		graphicswidget_frame, multi_frame);
	QVBoxLayout * l8 = new QVBoxLayout(frame2D_viewerX);
	l8->setSpacing(0);
	l8->setContentsMargins(0,0,0,0);
	l8->addWidget(graphicswidget_x);
	graphicswidget_x->set_sliderwidget(slider_x);
	graphicswidget_x->set_toolbox2D_widget(toolbox2D);
	//
	if (graphicswidget_m->graphicsview)
	{
		graphicswidget_m->graphicsview->set_widget_y(graphicswidget_y);
		graphicswidget_m->graphicsview->set_widget_x(graphicswidget_x);
	}
	if (graphicswidget_y->graphicsview)
	{
		graphicswidget_y->graphicsview->set_widget_m(graphicswidget_m);
		graphicswidget_y->graphicsview->set_widget_x(graphicswidget_x);
	}
	if (graphicswidget_x->graphicsview)
	{
		graphicswidget_x->graphicsview->set_widget_m(graphicswidget_m);
		graphicswidget_x->graphicsview->set_widget_y(graphicswidget_y);
	}
	//
	zrangewidget = new ZRangeWidget();
	QVBoxLayout * l13 = new QVBoxLayout(zrange_frame);
	l13->setSpacing(0);
	l13->setContentsMargins(0,0,0,0);
	l13->addWidget(zrangewidget);
	//
	sqtree = new SQtree(true);
	QVBoxLayout * vl296 = new QVBoxLayout(metadata_frame);
	vl296->setContentsMargins(0,0,0,0);
	vl296->setSpacing(0);
	vl296->addWidget(sqtree);
	//
	browser2 = new BrowserWidget2(scale_icons*adjust_scale_icons);
	QVBoxLayout * vl396 = new QVBoxLayout(browser2_frame);
	vl396->setContentsMargins(0,0,0,0);
	vl396->setSpacing(0);
	vl396->addWidget(browser2);
	//
	anonymizer = new AnonymazerWidget2(scale_icons*adjust_scale_icons);
	QVBoxLayout * vl196 = new QVBoxLayout(deidentify_frame);
	vl196->setContentsMargins(0,0,0,0);
	vl196->setSpacing(0);
	vl196->addWidget(anonymizer);
	//
	settingswidget = new SettingsWidget(scale_icons);
	QVBoxLayout * vl496 = new QVBoxLayout(settings_frame);
	vl496->setContentsMargins(0,0,0,0);
	vl496->setSpacing(0);
	vl496->addWidget(settingswidget);
	//
	if (ok3d)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		glwidget = new GLWidget();
#else
#ifdef USE_SET_GL_FORMAT
		QGLFormat fmt;
#ifdef USE_CORE_3_2_PROFILE
#ifdef USE_GL_MAJOR_3_MINOR_2
		fmt.setVersion(3, 2); // may be required sometimes
#endif
		fmt.setProfile(QGLFormat::CoreProfile);
#endif
		fmt.setDirectRendering(true);
		fmt.setDoubleBuffer(true);
		fmt.setAlpha(true);
		fmt.setRedBufferSize(8);
		fmt.setGreenBufferSize(8);
		fmt.setBlueBufferSize(8);
		fmt.setAlphaBufferSize(8);
		fmt.setDepth(true);
		fmt.setDepthBufferSize(24);
		//fmt.setSampleBuffers(true);
		//fmt.setSamples(4);
		glwidget = new GLWidget(fmt);
#else
		glwidget = new GLWidget();
#endif
#endif
		QVBoxLayout * vl2 = new QVBoxLayout(gl_frame);
		vl2->setContentsMargins(0,0,0,0);
		vl2->addWidget(glwidget);
	}
	else
	{
		glwidget = NULL;
	}
	//
	aliza = new Aliza();
	//
	trans_icon = QIcon(QString(":/bitmaps/trans1.svg"));
	notrans_icon = QIcon(QString(":/bitmaps/notrans1.svg"));
	cut_icon = QIcon(QString(":bitmaps/cut.svg"));
	nocut_icon = QIcon(QString(":bitmaps/nocut.svg"));
	anchor_icon = QIcon(QString(":/bitmaps/anchor.svg"));
	anchor2_icon = QIcon(QString(":/bitmaps/anchor2.svg"));
	//
	studyview = new StudyViewWidget(scale_icons*adjust_scale_icons);
	//
	createActions();
	createMenus();
	createToolBars();
	//
	QDateTime date_    = QDateTime::currentDateTime();
	QString   date_str = date_.toString(QString("hhmmsszzz"));
	//
	aliza->set_hide_zoom(hide_zoom);
	aliza->set_browser2(browser2);
	aliza->set_settingswidget(settingswidget);
	aliza->set_imagesbox(imagesbox);
	aliza->set_toolbox(toolbox);
	aliza->set_anim3Dwidget(anim3Dwidget);
	aliza->set_anim2Dwidget(anim2Dwidget);
	aliza->set_toolbox2D(toolbox2D);
	aliza->set_glwidget(glwidget);
	aliza->set_graphicswidget_m(graphicswidget_m);
	aliza->set_graphicswidget_y(graphicswidget_y);
	aliza->set_graphicswidget_x(graphicswidget_x);
	aliza->set_sliderwidget_m(slider_m);
	aliza->set_sliderwidget_y(slider_y);
	aliza->set_sliderwidget_x(slider_x);
	aliza->set_zrangewidget(zrangewidget);
	aliza->set_lutwidget2(lutwidget2);
	aliza->set_trans3D_action(trans3DAct);
	aliza->set_studyview(studyview);
	aliza->set_axis_actions(
		graphicsAct_Z,
		graphicsAct_Y,
		graphicsAct_X,
		zyxAct,
		histogramAct);
	aliza->set_3D_views_actions(
		slicesAct,
		zlockAct,
		oneAct);
	aliza->set_2D_views_actions(
		frames2DAct,
		distanceAct,
		rectAct,
		transp2dAct,
		cursorAct,
		collisionAct);
	aliza->set_uniq_string(date_str);
	aliza->connect_slots();
	//
	graphicswidget_m->set_aliza(aliza);
	graphicswidget_x->set_aliza(aliza);
	graphicswidget_y->set_aliza(aliza);
	//
	histogramview = new HistogramView(
		this, static_cast<QObject*>(aliza), multi_frame, false);
	histogram_frame_layout = new QVBoxLayout(histogram_frame);
	histogram_frame_layout->setSpacing(0);
	histogram_frame_layout->setContentsMargins(0,0,0,0);
	histogram_frame_layout->addWidget(histogramview);
	histogram_frame2_layout = new QVBoxLayout(histogram_frame2);
	histogram_frame2_layout->setSpacing(0);
	histogram_frame2_layout->setContentsMargins(0,0,0,0);
	aliza->set_histogramview(histogramview);
	//
	first_image_loaded = false;
	connect(aliza,SIGNAL(report_load_to_mainwin()),this,SLOT(set_ui()));
	//
	if (ok3d && glwidget)
	{
		saved_ok3d = true;
		connect(glwidget,SIGNAL(opengl3_not_available()),this,SLOT(set_no_gl3()));
		gl_frame->show();
		glwidget->show();
		view3d_label->setText(QString(
			"Physical space, intensity projection"));
		slicesAct->setChecked(true);
		raycastAct->setChecked(false);
	}
	else
	{
		saved_ok3d = false;
		gl_frame->hide();
		slicesAct->setChecked(false);
		raycastAct->setChecked(false);
		trans3DAct->setEnabled(false);
		gloptionsAct->setEnabled(false);
		frames3DAct->setEnabled(false);
		frame3D->hide();
		settingswidget->set_gl_visible(false);
		show3DAct->blockSignals(true);
		show3DAct->setChecked(false);
		show3DAct->blockSignals(false);
	}
	//
	toolbar2D_frame->hide();
	toolbar3D_frame->hide();
	level_frame->hide();
	zrange_frame->hide();
	slider_frame->hide();
	//
	connect(browser2->load_pushButton,SIGNAL(clicked()),this,SLOT(load_dicom_series2()));
	connect(browser2->tableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(load_dicom_series2()));
	if (!hide_zoom)
	{
		if (zoomwidget2D)
		{
			connect(zoomwidget2D->plus_pushButton, SIGNAL(clicked()),this,SLOT(zoom_plus_2d()));
			connect(zoomwidget2D->minus_pushButton,SIGNAL(clicked()),this,SLOT(zoom_minus_2d()));
		}
		if (zoomwidget3D)
		{
			connect(zoomwidget3D->plus_pushButton, SIGNAL(clicked()),this,SLOT(zoom_plus_3d()));
			connect(zoomwidget3D->minus_pushButton,SIGNAL(clicked()),this,SLOT(zoom_minus_3d()));
		}
	}
	//
	readSettings();
	//
	connect(openAct,                        SIGNAL(triggered()),         this,SLOT(toggle_browser()));
	connect(openanyAct,                     SIGNAL(triggered()),         this,SLOT(load_any()));
	connect(exitAct,                        SIGNAL(triggered()),         this,SLOT(close()));
	connect(aboutAct,                       SIGNAL(triggered()),         this,SLOT(about()));
	connect(settingsAct,                    SIGNAL(triggered()),         this,SLOT(toggle_settingswidget()));
	connect(graphicsAct_Z,                  SIGNAL(toggled(bool)),       this,SLOT(toggle_graphicswidget_m_z(bool)));
	connect(graphicsAct_Y,                  SIGNAL(toggled(bool)),       this,SLOT(toggle_graphicswidget_m_y(bool)));
	connect(graphicsAct_X,                  SIGNAL(toggled(bool)),       this,SLOT(toggle_graphicswidget_m_x(bool)));
	connect(histogramAct,                   SIGNAL(toggled(bool)),       this,SLOT(toggle_histogram(bool)));
	connect(zyxAct,                         SIGNAL(toggled(bool)),       this,SLOT(toggle_graphicswidget_zyx(bool)));
	connect(show3DAct,                      SIGNAL(toggled(bool)),       this,SLOT(toggle_showgl(bool)));
	connect(show2DAct,                      SIGNAL(toggled(bool)),       this,SLOT(toggle_show2D(bool)));
	connect(rectAct,                        SIGNAL(toggled(bool)),       this,SLOT(toggle_rect(bool)));
	connect(cursorAct,                      SIGNAL(toggled(bool)),       this,SLOT(toggle_cursor(bool)));
	connect(collisionAct,                   SIGNAL(toggled(bool)),       this,SLOT(toggle_collisions(bool)));
	connect(distanceAct,                    SIGNAL(toggled(bool)),       this,SLOT(toggle_distance(bool)));
	connect(transp2dAct,                    SIGNAL(toggled(bool)),       this,SLOT(toggle_segmentation(bool)));
	connect(slicesAct,                      SIGNAL(toggled(bool)),       this,SLOT(set_view_3d(bool)));
	connect(raycastAct,                     SIGNAL(toggled(bool)),       this,SLOT(set_view_rc(bool)));
	connect(gloptionsAct,                   SIGNAL(triggered()),         this,SLOT(toggle_toolbox()));
	connect(animAct2d,                      SIGNAL(toggled(bool)),       this,SLOT(toggle_animwidget2d(bool)));
	connect(animAct3d,                      SIGNAL(toggled(bool)),       this,SLOT(toggle_animwidget3d(bool)));
	connect(frames2DAct,                    SIGNAL(toggled(bool)),       this,SLOT(set_show_frames_2d(bool)));
	connect(frames3DAct,                    SIGNAL(toggled(bool)),       this,SLOT(set_show_frames_3d(bool)));
	connect(resetRectAct2,                  SIGNAL(triggered()),         this,SLOT(reset_rect2()));
	connect(reset3DAct,                     SIGNAL(triggered()),         this,SLOT(reset_3d()));
	connect(flipXAct,                       SIGNAL(triggered()),         this,SLOT(flipX()));
	connect(flipYAct,                       SIGNAL(triggered()),         this,SLOT(flipY()));
	connect(anim3Dwidget->start_pushButton, SIGNAL(clicked()),           this,SLOT(start_3D_anim()));
	connect(anim3Dwidget->stop_pushButton,  SIGNAL(clicked()),           this,SLOT(stop_3D_anim()));
	connect(anim2Dwidget->start_pushButton, SIGNAL(clicked()),           this,SLOT(start_anim()));
	connect(anim2Dwidget->stop_pushButton,  SIGNAL(clicked()),           this,SLOT(stop_anim()));
	connect(setLevelAct,                    SIGNAL(triggered()),         this,SLOT(trigger_set_level()));
	connect(zlockAct,                       SIGNAL(toggled(bool)),       this,SLOT(set_zlock(bool)));
	connect(oneAct,                         SIGNAL(toggled(bool)),       this,SLOT(set_zlock_one(bool)));
	connect(browser2->meta_pushButton,      SIGNAL(clicked()),           this,SLOT(toggle_meta2()));
	connect(browser_open_dir_act,           SIGNAL(triggered()),         browser2,SLOT(open_dicom_dir()));
	connect(browser_open_dcmdir_act,        SIGNAL(triggered()),         browser2,SLOT(open_DICOMDIR()));
	connect(browser_reload_act,             SIGNAL(triggered()),         browser2,SLOT(reload_dir()));
	connect(browser_metadata_act,           SIGNAL(triggered()),         this,    SLOT(toggle_meta2()));
	connect(browser_copy_act,               SIGNAL(triggered()),         browser2,SLOT(copy_files()));
	connect(browser_load_act,               SIGNAL(triggered()),         this,    SLOT(load_dicom_series2()));
	connect(meta_open_act,                  SIGNAL(triggered()),         sqtree,  SLOT(open_file()));
	connect(meta_open_scan_act,             SIGNAL(triggered()),         sqtree,  SLOT(open_file_and_series()));
	connect(anon_open_in_dir,               SIGNAL(triggered()),         anonymizer,SLOT(set_input_dir()));
	connect(anon_open_out_dir,              SIGNAL(triggered()),         anonymizer,SLOT(set_output_dir()));
	connect(anon_run,                       SIGNAL(triggered()),         anonymizer,SLOT(run_()));
	connect(tabWidget,                      SIGNAL(currentChanged(int)), this,    SLOT(tab_ind_changed(int)));
	connect(imagesbox->actionDICOMMeta,     SIGNAL(triggered()),         this,    SLOT(trigger_image_dicom_meta()));
	connect(aliza,                          SIGNAL(image_opened()),      this,    SLOT(set_image_view()));
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	connect(settingswidget->styleComboBox,  SIGNAL(currentTextChanged(const QString&)),this,SLOT(set_style(const QString&)));
#else
	connect(settingswidget->styleComboBox,  SIGNAL(currentIndexChanged(const QString&)),this,SLOT(set_style(const QString&)));
#endif
	//
	close_sc = new QShortcut(QKeySequence::Close, this, SLOT(ask_close()));
	close_sc->setAutoRepeat(false);
	open_sc = new QShortcut(QKeySequence::Open, this, SLOT(toggle_browser()));
	open_sc->setAutoRepeat(false);
#ifdef __APPLE__
	minimaze_sc = new QShortcut(QKeySequence("Ctrl+M"), this, SLOT(showMinimized()));
	minimaze_sc->setAutoRepeat(false);
	fullsceen_sc = new QShortcut(QKeySequence("Ctrl+Meta+F"), this, SLOT(showFullScreen()));
	fullsceen_sc->setAutoRepeat(false);
	normal_sc = new QShortcut(QKeySequence("Esc"), this, SLOT(showNormal()));
	normal_sc->setAutoRepeat(false);
#endif
	setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
}

void MainWindow::open_args(const QStringList & l)
{
	const int lsize = l.size();
	if (lsize < 1) return;
	bool lock = mutex.tryLock();
	if (!lock) return;
	QStringList l2;
	int i = 0;
	while (i < lsize)
	{
		if (l.at(i)==QString("-nogl") || l.at(i)==QString("--nogl"))
		{
			;;
		}
		else
		{
			l2.push_back(l.at(i));
		}
		++i;
	}
	if (l2.size()==0)
	{
		mutex.unlock();
		return;
	}
	QProgressDialog * pb = new QProgressDialog(
		QString("Loading..."),
		QString("Exit"),
		0,
		0);
	pb->setWindowModality(Qt::ApplicationModal);
	pb->setWindowFlags(
		pb->windowFlags()^Qt::WindowContextHelpButtonHint);
	connect(pb,SIGNAL(canceled()),this,SLOT(exit_null()));
	pb->setMinimumWidth(256);
	pb->show();
	pb->setValue(-1);
	if (l2.size()==1)
	{
		const QString f = l2.at(0);
		QFileInfo fi(f);
		if (
			fi.isFile() &&
			fi.fileName().toUpper() == QString("DICOMDIR"))
		{
			browser2->open_DICOMDIR2(fi.absoluteFilePath());
		}
		else if (fi.isDir())
		{
			browser2->open_dicom_dir2(fi.absoluteFilePath());
		}
		else if (fi.isFile())
		{
			load_any_file(f, pb, true);
		}
	}
	else if (l2.size()>1)
	{
		for (int x = 0; x < l2.size(); ++x)
		{
			pb->setValue(-1);
			const QString f = l2.at(x);
			QFileInfo fi(f);
			if (fi.isFile()) load_any_file(l2.at(x), pb, true);
		}
	}
	disconnect(pb,SIGNAL(canceled()),this,SLOT(exit_null()));
	pb->close();
	delete pb;
	pb = NULL;
	mutex.unlock();
}

void MainWindow::closeEvent(QCloseEvent * e)
{
	writeSettings();
	aliza->close_();
	delete aliza;
	aliza = NULL;
	delete aboutwidget;
	aboutwidget = NULL;
	delete studyview;
	studyview = NULL;
	e->accept();
#if QT_VERSION < QT_VERSION_CHECK(4,8,1)
	qApp->quit();
#else
	qApp->closeAllWindows();
#endif
}

void MainWindow::resizeEvent(QResizeEvent * e)
{
	QMainWindow::resizeEvent(e);
}

void MainWindow::exit_null()
{
	exit(0);
}

void MainWindow::about()
{
	const QString tmp0 = aliza->get_opengl_info();
	aboutwidget->set_opengl_info(tmp0);
	aboutwidget->set_info();
	aboutwidget->show();
	aboutwidget->raise();
}

void MainWindow::createActions()
{
	openAct       = new QAction(QIcon(QString(":/bitmaps/dcm.svg")),   QString("DICOM scanner"),this);
	openanyAct    = new QAction(QIcon(QString(":/bitmaps/file.svg")),  QString("Open file"),    this);
	exitAct       = new QAction(QIcon(QString(":/bitmaps/delete.svg")),QString("Exit"),         this);
	exitAct->setShortcuts(QKeySequence::Quit);
	aboutAct      = new QAction(QIcon(QString(":/bitmaps/info.svg")),  QString("About"),        this);
	settingsAct   = new QAction(QIcon(QString(":/bitmaps/tool.svg")),  QString("Settings"),     this);
#ifdef __APPLE__
	settingsAct->setShortcuts(QKeySequence::Preferences);
#endif
	axis_group = new QActionGroup(this);
	graphicsAct_Z = new QAction(QIcon(QString(":/bitmaps/align.svg")),
		QString("Slice view (Z)"), this);
	graphicsAct_Z->setCheckable(true);
	graphicsAct_Z->setChecked(true);
	axis_group->addAction(graphicsAct_Z);
	graphicsAct_Y = new QAction(QIcon(QString(":/bitmaps/2dy2.svg")),
		QString("Reconstruction view (Y)"), this);
	graphicsAct_Y->setCheckable(true);
	axis_group->addAction(graphicsAct_Y);
	graphicsAct_X = new QAction(QIcon(QString(":/bitmaps/2dx2.svg")),
		QString("Reconstruction view (X)"), this);
	graphicsAct_X->setCheckable(true);
	axis_group->addAction(graphicsAct_X);
	histogramAct = new QAction(QIcon(QString(":/bitmaps/chart0.svg")),
		QString("Histogram,\nclick histogram symbol\nin list widget to calculate"), this);
	histogramAct->setCheckable(true);
	axis_group->addAction(histogramAct);
	zyxAct = new QAction(QIcon(QString(":/bitmaps/grid.svg")),
		QString("Z, MPR Y, MPR X, Histogram"), this);
	zyxAct->setCheckable(true);
	axis_group->addAction(zyxAct);
	show3DAct = new QAction(QString("3D Window"), this);
	show3DAct->setCheckable(true);
	show3DAct->setChecked(true);
	show3DAct->setEnabled(false);
	show2DAct = new QAction(QString("2D Window"), this);
	show2DAct->setCheckable(true);
	show2DAct->setChecked(true);
	show2DAct->setEnabled(false);
	cursorAct = new QAction(QIcon(QString(":/bitmaps/cursor.svg")),
		QString("Show value under cursor"), this);
	cursorAct->setCheckable(true);
	cursorAct->setEnabled(true);
	collisionAct = new QAction(QIcon(QString(":/bitmaps/collisions.svg")),
		QString("Show slices intersections"), this);
	collisionAct->setCheckable(true);
	collisionAct->setChecked(true);
	collisionAct->setEnabled(true);
	rectAct = new QAction(cut_icon,
		QString("X, Y - selection rectangle"), this);
	rectAct->setCheckable(true);
	rectAct->setEnabled(true);
	distanceAct = new QAction(QIcon(QString(":/bitmaps/distance.svg")),
		QString("Measure distance"), this);
	distanceAct->setCheckable(true);
	distanceAct->setEnabled(true);
	transp2dAct = new QAction(notrans_icon,
		QString(
			"On: discard outside lower and upper.\n"
			"Off: discard outside lower, "
			"clamp outside upper to max"), this);
	transp2dAct->setCheckable(true);
	transp2dAct->setEnabled(true);
	view_group = new QActionGroup(this);
	slicesAct = new QAction(QIcon(QString(":/bitmaps/align.svg")),
		QString("Physical space, intensity projection"), this);
	slicesAct->setCheckable(true);
	slicesAct->setChecked(true);
	view_group->addAction(slicesAct);
	raycastAct = new QAction(QIcon(QString(":/bitmaps/ray.svg")),
		QString("Intensity projection"), this);
	raycastAct->setCheckable(true);
	view_group->addAction(raycastAct);
	frames2DAct = new QAction(QIcon(QString(":/bitmaps/cross.svg")),
		QString("MPR set position"), this);
	frames2DAct->setCheckable(true);
	frames3DAct = new QAction(QIcon(QString(":/bitmaps/square.svg")),
		QString("Show frames"), this);
	frames3DAct->setCheckable(true);
	frames3DAct->setChecked(false);
	resetRectAct2 = new QAction(QIcon(QString(":/bitmaps/reload.svg")),
		QString("Reset 2D view"), this);
	resetRectAct2->setEnabled(true);
	const QString tooltip0 = QString(
		"On: discard outside lower and upper (intensity to alpha).\n"
		"Off: discard outside lower, "
		"clamp outside upper to max (alpha 1)");
	trans3DAct = new QAction(trans_icon, tooltip0, this);
	trans3DAct->setCheckable(true);
	trans3DAct->setChecked(true);
	gloptionsAct = new QAction(QIcon(QString(":/bitmaps/tool.svg")),
		QString("3D view options"), this);
	reset3DAct = new QAction(QIcon(QString(":/bitmaps/reload.svg")),
		QString("Reset 3D view"), this);
	reset3DAct->setEnabled(true);
	animAct2d = new QAction(QIcon(QString(":/bitmaps/2dt.svg")),
		QString("2D+time"), this);
	animAct2d->setCheckable(true);
	animAct3d = new QAction(QIcon(QString(":/bitmaps/3dt.svg")),
		QString("3D+time"), this);
	animAct3d->setCheckable(true);
	flipXAct = new QAction(QIcon(QString(":/bitmaps/flipx.svg")),
		QString(
			"Flip horizontally,\n"
			"press \"x\" key\n"
			"(having widget in focus)"), this);
	flipXAct->setCheckable(false);
	flipYAct = new QAction(QIcon(QString(":/bitmaps/flipy.svg")),
		QString(
			"Flip vertically,\n"
			"press \"y\" key\n"
			"(having widget in focus)"), this);
	flipYAct->setCheckable(false);
	zrangeAct = new QAction(
		QString("Z - double-slicer below 2D view"), this);
	setLevelAct = new QAction(QString("Set level/window"), this);
	zlockAct = new QAction(anchor2_icon,
		QString("Anchor selected slice"), this);
	zlockAct->setCheckable(true);
	zlockAct->setChecked(false);
	oneAct = new QAction(QIcon(QString(":/bitmaps/one.svg")),
		QString("Single slice"), this);
	oneAct->setCheckable(true);
	oneAct->setChecked(false);
	oneAct->setEnabled(false);
	browser_open_dir_act    = new QAction(QIcon(QString(":/bitmaps/folder.svg")),QString("Open directory"), this);
	browser_open_dcmdir_act = new QAction(QIcon(QString(":/bitmaps/dcmdir.svg")),QString("Open DICOMDIR"), this);
	browser_reload_act      = new QAction(QIcon(QString(":/bitmaps/reload.svg")),QString("Reload"), this);
	browser_metadata_act    = new QAction(QIcon(QString(":/bitmaps/meta.svg")),QString("Show metadata"), this);
	browser_copy_act        = new QAction(QIcon(QString(":/bitmaps/copy2.svg")),QString("Copy selected"), this);
	browser_load_act        = new QAction(QIcon(QString(":/bitmaps/right0.svg")),QString("Load selected"), this);
	meta_open_act           = new QAction(QIcon(QString(":/bitmaps/file.svg")),QString("Open file"), this);
	meta_open_scan_act      = new QAction(QIcon(QString(":/bitmaps/align.svg")),QString("Open file and scan"), this);
	anon_open_in_dir        = new QAction(QIcon(QString(":/bitmaps/folder.svg")),QString("Open input directory"), this);
	anon_open_out_dir       = new QAction(QIcon(QString(":/bitmaps/folder.svg")),QString("Open output directory"), this);
	anon_run                = new QAction(QIcon(QString(":/bitmaps/right0.svg")),QString("De-identify"), this);
}

void MainWindow::createMenus()
{
	file_menu = menuBar()->addMenu(QString("Application"));
	file_menu->addAction(openAct);
	file_menu->addAction(openanyAct);
	file_menu->addAction(aboutAct);
	file_menu->addAction(settingsAct);
	file_menu->addAction(exitAct);
	//
	views_menu = menuBar()->addMenu(QString("Views"));
	views_menu->addAction(show2DAct);
	actionViews2DMenu  = new QAction(
		QString("2D Views"),this);
	QMenu * views2d_menu = new QMenu(this);
	views2d_menu->addAction((graphicsAct_Z));
	views2d_menu->addAction((zyxAct));
	views2d_menu->addAction((graphicsAct_Y));
	views2d_menu->addAction((graphicsAct_X));
	views2d_menu->addAction((histogramAct));
	actionViews2DMenu->setMenu(views2d_menu);
	actionViews2DMenu->setEnabled(false);
	views_menu->addAction(actionViews2DMenu);
	//
	views_menu->addAction(show3DAct);
	actionViews3DMenu  = new QAction(
		QString("3D Views"),this);
	QMenu * views3d_menu = new QMenu(this);
	views3d_menu->addAction((slicesAct));
	views3d_menu->addAction((raycastAct));
	actionViews3DMenu->setMenu(views3d_menu);
	views_menu->addAction(actionViews3DMenu);
	actionViews3DMenu->setEnabled(false);
	//
	tools_menu = menuBar()->addMenu(QString("Tools"));
	tools_menu->addAction(imagesbox->actionClear);
	tools_menu->addAction(imagesbox->actionClearAll);
	tools_menu->addAction(imagesbox->actionCheck);
	tools_menu->addAction(imagesbox->actionUncheck);
	tools_menu->addAction(imagesbox->actionClearChecked);
	tools_menu->addAction(imagesbox->actionClearUnChek);
	tools_menu->addAction(imagesbox->actionColor);
	tools_menu->addAction(imagesbox->actionDICOMMeta);
	tools_menu->addSeparator();
	tools_menu->addAction(imagesbox->actionReloadHistogram);
	tools_menu->addAction(animAct2d);
	tools_menu->addAction(animAct3d);
	tools_menu->addAction(cursorAct);
	tools_menu->addAction(collisionAct);
	tools_menu->addAction(distanceAct);
	tools_menu->addAction(frames2DAct);
	QAction * actionToolsSelectMenu  = new QAction(
		QString("Select sub-image"),this);
	QMenu * tools_select_menu = new QMenu(this);
	tools_select_menu->addAction(rectAct);
	tools_select_menu->addAction(zrangeAct);
	actionToolsSelectMenu->setMenu(tools_select_menu);
	tools_menu->addAction(actionToolsSelectMenu);
	QAction * actionTools2DMenu  = new QAction(
		QString("2D Views"),this);
	QMenu * tools2d_menu = new QMenu(this);
	tools2d_menu->addAction(transp2dAct);
	tools2d_menu->addAction(resetRectAct2);
	tools2d_menu->addAction(flipXAct);
	tools2d_menu->addAction(flipYAct);
	actionTools2DMenu->setMenu(tools2d_menu);
	tools_menu->addAction(actionTools2DMenu);
	QAction * actionTools3DMenu  = new QAction(
		QString("3D Views"),this);
	QMenu * tools3d_menu = new QMenu(this);
	tools3d_menu->addAction(trans3DAct);
	tools3d_menu->addAction(frames3DAct);
	tools3d_menu->addAction(gloptionsAct);
	tools3d_menu->addAction(reset3DAct);
	actionTools3DMenu->setMenu(tools3d_menu);
	tools_menu->addAction(actionTools3DMenu);
	tools_menu->addAction(setLevelAct);
#if 0
	QAction * actionToolsUtils  = new QAction(
		QString("Utilities"),this);
	QMenu * tools_utils_menu = new QMenu(this);
	tools_utils_menu->addAction(imagesbox->actionSqlBrowser);
	actionToolsUtils->setMenu(tools_utils_menu);
	tools_menu->addAction(actionToolsUtils);
#endif
	//
	browser_menu = menuBar()->addMenu(QString("DICOM scanner"));
	browser_menu->addAction(browser_open_dir_act);
	browser_menu->addAction(browser_open_dcmdir_act);
	browser_menu->addAction(browser_reload_act);
	browser_menu->addAction(browser_metadata_act);
	browser_menu->addAction(browser_copy_act);
	browser_menu->addAction(browser_load_act);
	browser_menu->menuAction()->setVisible(false);
	//
	metadata_menu = menuBar()->addMenu(QString("DICOM metadata"));
	metadata_menu->addAction(meta_open_act);
	metadata_menu->addAction(meta_open_scan_act);
	metadata_menu->menuAction()->setVisible(false);
	//
	deidentify_menu = menuBar()->addMenu(QString("De-identify"));
	deidentify_menu->addAction(anon_open_in_dir);
	deidentify_menu->addAction(anon_open_out_dir);
	deidentify_menu->addAction(anon_run);
	deidentify_menu->menuAction()->setVisible(false);
	//
	settings_menu = menuBar()->addMenu(QString("Settings"));
	settings_menu->menuAction()->setVisible(false);
	//
}

void MainWindow::createToolBars()
{
	toolbar4 = new QToolBar(this);
	toolbar4->setIconSize(
		QSize((int)(18*scale_icons*adjust_scale_icons),(int)(18*scale_icons*adjust_scale_icons)));
	if (toolbar4->layout())
	{
		toolbar4->layout()->setContentsMargins(0,0,0,0);
		toolbar4->layout()->setSpacing(2);
	}
	QHBoxLayout * l4 = new QHBoxLayout(view2d_frame);
	l4->setContentsMargins(0,0,0,0);
	l4->setSpacing(0);
	l4->addWidget(toolbar4);
	view2d_label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	view2d_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	toolbar4->addWidget(view2d_label);
	toolbar4->addAction(graphicsAct_Z);
	toolbar4->addAction(zyxAct);
	toolbar4->addAction(graphicsAct_Y);
	toolbar4->addAction(graphicsAct_X);
	toolbar4->addAction(histogramAct);
	QLabel * empty5 = new QLabel(QString("      "), this);
	toolbar4->addWidget(empty5);
	//
	//
	//
	toolbar5 = new QToolBar(this);
	toolbar5->setIconSize(
		QSize((int)(18*scale_icons*adjust_scale_icons),(int)(18*scale_icons*adjust_scale_icons)));
	if (toolbar5->layout())
	{
		toolbar5->layout()->setContentsMargins(0,0,0,0);
		toolbar5->layout()->setSpacing(2);
	}
	QHBoxLayout * l5 = new QHBoxLayout(view3d_frame);
	l5->setContentsMargins(0,0,0,0);
	l5->setSpacing(0);
	l5->addWidget(toolbar5);
	view3d_label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	view3d_label->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	toolbar5->addWidget(view3d_label);
	toolbar5->addAction(slicesAct);
	toolbar5->addAction(raycastAct);
	toolbar5->addSeparator();
	QLabel * empty6 = new QLabel(QString("      "), this);
	toolbar5->addWidget(empty6);
	//
	//
	//
	toolbar2 = new QToolBar(this);
	toolbar2->setIconSize(
		QSize((int)(18*scale_icons*adjust_scale_icons),(int)(18*scale_icons*adjust_scale_icons)));
	if (toolbar2->layout())
	{
		toolbar2->layout()->setContentsMargins(0,0,0,0);
		toolbar2->layout()->setSpacing(2);
	}
	QHBoxLayout * l2 = new QHBoxLayout(toolbar2D_frame);
	l2->setContentsMargins(0,0,0,0);
	l2->setSpacing(0);
	l2->addWidget(toolbar2);
	toolbar2->addAction(animAct2d);
	toolbar2->addAction(animAct3d);
	if (!hide_zoom && zoomwidget2D)
	{
		toolbar2->addWidget(zoomwidget2D);
	}
	toolbar2->addWidget(lutwidget2);
	toolbar2->addAction(transp2dAct);
	toolbar2->addAction(cursorAct);
	toolbar2->addAction(collisionAct);
	//
	//
	QWidget * spacer1 = new QWidget(this);
	spacer1->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	toolbar2->addWidget(spacer1);
	toolbar2->addSeparator();
	toolbar2->addAction(rectAct);
	toolbar2->addAction(frames2DAct);
	toolbar2->addAction(distanceAct);
	toolbar2->addSeparator();
	QWidget * spacer2 = new QWidget(this);
	spacer2->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	toolbar2->addWidget(spacer2);
	toolbar2->addAction(resetRectAct2);
	toolbar2->addAction(flipXAct);
	toolbar2->addAction(flipYAct);
	//
	//
	//
	toolbar3 = new QToolBar(this);
	toolbar3->setIconSize(
		QSize((int)(18*scale_icons*adjust_scale_icons),(int)(18*scale_icons*adjust_scale_icons)));
	if (toolbar3->layout())
	{
		toolbar3->layout()->setContentsMargins(0,0,0,0);
		toolbar3->layout()->setSpacing(2);
	}
	QHBoxLayout * l3 = new QHBoxLayout(toolbar3D_frame);
	l3->setContentsMargins(0,0,0,0);
	l3->setSpacing(0);
	l3->addWidget(toolbar3);
	if (!hide_zoom && zoomwidget3D)
	{
		toolbar3->addWidget(zoomwidget3D);
	}
	toolbar3->addAction(zlockAct);
	toolbar3->addAction(oneAct);
	toolbar3->addAction(trans3DAct);
	QWidget * spacer3 = new QWidget(this);
	spacer3->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	toolbar3->addWidget(spacer3);
	toolbar3->addAction(frames3DAct);
	toolbar3->addAction(gloptionsAct);
	toolbar3->addAction(reset3DAct);
}

void MainWindow::toggle_browser()
{
	if (tabWidget->currentIndex() == 1)
		browser2->open_dicom_dir();
	else
		tabWidget->setCurrentIndex(1);
}

void MainWindow::toggle_settingswidget()
{
	tabWidget->setCurrentIndex(4);
}

void MainWindow::toggle_showgl(bool t)
{
	if (t)
	{
		frame3D->show();
		if (saved_ok3d)
		{
			toolbar3D_frame->show();
			view3d_frame->show();
		}
		else
		{
			toolbar3D_frame->hide();
			view3d_frame->hide();
		}
	}
	else
	{
		frame3D->hide();
	}
	if (frame2D->isHidden() && frame3D->isHidden())
	{
		level_frame->hide();
		zrange_frame->hide();
	}
	else
	{
		level_frame->show();
		zrange_frame->show();
	}
	qApp->processEvents();
}

void MainWindow::toggle_show2D(bool t)
{
	if (t) frame2D->show();
	else   frame2D->hide();
	if (frame2D->isHidden() && frame3D->isHidden())
	{
		level_frame->hide();
		zrange_frame->hide();
	}
	else
	{
		level_frame->show();
		zrange_frame->show();
	}
	qApp->processEvents();
}

void MainWindow::toggle_graphicswidget_m_x(bool t)
{
	view2d_label->setText("Reconstruction view (X)");
	if (!t) return;
	if (!graphicswidget_m) return;
	multi_frame->hide();
	histogram_frame->hide();
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m)>0)
		frame2D_viewerZ_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewer_layout->indexOf(graphicswidget_m)<0)
	{
		frame2D_viewer_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_label);
		graphicswidget_m->set_left_label(left_label);
		graphicswidget_m->set_measure_label(measure_label);
		graphicswidget_m->set_info_line(info_line);
	}
	if (slider_frame_layoutZ->indexOf(slider_m)>0)
		slider_frame_layoutZ->removeWidget(slider_m);
	if (slider_frame_layout->indexOf(slider_m)<0)
		slider_frame_layout->addWidget(slider_m);
	if (graphicswidget_y) graphicswidget_y->clear_(true);
	if (graphicswidget_x) graphicswidget_x->clear_(true);
	graphicswidget_frame->show();
	aliza->set_axis_2D(0, rectAct->isChecked());
}

void MainWindow::toggle_graphicswidget_m_y(bool t)
{
	view2d_label->setText("Reconstruction view (Y)");
	if (!t) return;
	if (!graphicswidget_m) return;
	multi_frame->hide();
	histogram_frame->hide();
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m)>0)
		frame2D_viewerZ_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewer_layout->indexOf(graphicswidget_m)<0)
	{
		frame2D_viewer_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_label);
		graphicswidget_m->set_left_label(left_label);
		graphicswidget_m->set_measure_label(measure_label);
		graphicswidget_m->set_info_line(info_line);
	}
	if (slider_frame_layoutZ->indexOf(slider_m)>0)
		slider_frame_layoutZ->removeWidget(slider_m);
	if (slider_frame_layout->indexOf(slider_m)<0)
		slider_frame_layout->addWidget(slider_m);
	if (graphicswidget_y) graphicswidget_y->clear_(true);
	if (graphicswidget_x) graphicswidget_x->clear_(true);
	graphicswidget_frame->show();
	aliza->set_axis_2D(1, rectAct->isChecked());
}

void MainWindow::toggle_graphicswidget_m_z(bool t)
{
	view2d_label->setText("Slice view (Z)");
	if (!t) return;
	if (!graphicswidget_m) return;
	multi_frame->hide();
	histogram_frame->hide();
	top_label->clear();	 top_labelZ->clear();
	left_label->clear(); left_labelZ->clear();
	measure_label->clear(); measure_labelZ->clear();
	info_line->clear(); info_lineZ->clear();
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m)>0)
		frame2D_viewerZ_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewer_layout->indexOf(graphicswidget_m)<0)
	{
		frame2D_viewer_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_label);
		graphicswidget_m->set_left_label(left_label);
		graphicswidget_m->set_measure_label(measure_label);
		graphicswidget_m->set_info_line(info_line);
	}
	if (slider_frame_layoutZ->indexOf(slider_m)>0)
		slider_frame_layoutZ->removeWidget(slider_m);
	if (slider_frame_layout->indexOf(slider_m)<0)
		slider_frame_layout->addWidget(slider_m);
	if (graphicswidget_y) graphicswidget_y->clear_(true);
	if (graphicswidget_x) graphicswidget_x->clear_(true);
	graphicswidget_frame->show();
	aliza->set_axis_2D(2, rectAct->isChecked());
}

void MainWindow::toggle_histogram(bool t)
{
	view2d_label->setText("Histogram view");
	if (!t) return;
	if (!graphicswidget_m) return;
	multi_frame->hide();
	graphicswidget_frame->hide();
	if (graphicswidget_m) graphicswidget_m->clear_(true);
	if (graphicswidget_y) graphicswidget_y->clear_(true);
	if (graphicswidget_x) graphicswidget_x->clear_(true);
	if (histogram_frame2_layout->indexOf(histogramview)>0)
		histogram_frame2_layout->removeWidget(histogramview);
	if (histogram_frame_layout->indexOf(histogramview)<0)
		histogram_frame_layout->addWidget(histogramview);
	histogram_frame->show();
	aliza->set_histogram();
}

void MainWindow::toggle_graphicswidget_zyx(bool t)
{
	view2d_label->setText(QString("Z, MPR Y, MPR X, Histogram"));
	if (!t) return;
	if (!graphicswidget_m) return;
	if (!graphicswidget_y) return;
	if (!graphicswidget_x) return;
	top_label->clear();	 top_labelZ->clear();
	left_label->clear(); left_labelZ->clear();
	measure_label->clear(); measure_labelZ->clear();
	info_line->clear(); info_lineZ->clear();
	graphicswidget_frame->hide();
	histogram_frame->hide();
	if (frame2D_viewer_layout->indexOf(graphicswidget_m)>0)
		frame2D_viewer_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m)<0)
	{
		frame2D_viewerZ_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_labelZ);
		graphicswidget_m->set_left_label(left_labelZ);
		graphicswidget_m->set_measure_label(measure_labelZ);
		graphicswidget_m->set_info_line(info_lineZ);
	}
	if (slider_frame_layout->indexOf(slider_m)>0)
		slider_frame_layout->removeWidget(slider_m);
	if (slider_frame_layoutZ->indexOf(slider_m)<0)
		slider_frame_layoutZ->addWidget(slider_m);
	if (histogram_frame_layout->indexOf(histogramview)>0)
		histogram_frame_layout->removeWidget(histogramview);
	if (histogram_frame2_layout->indexOf(histogramview)<0)
		histogram_frame2_layout->addWidget(histogramview);
	multi_frame->show();
	aliza->set_axis_zyx(rectAct->isChecked());
}

void MainWindow::toggle_toolbox()
{
	if (toolbox3D_frame->isVisible()) toolbox3D_frame->hide();
	else toolbox3D_frame->show();
}

void MainWindow::toggle_meta2()
{
	const QStringList & l = browser2->get_files_of_1st();
	if (l.empty()) return;
	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
	qApp->processEvents();
	sqtree->set_list_of_files(l);
	sqtree->read_file(l.at(0));
	tabWidget->setCurrentIndex(2);
	qApp->restoreOverrideCursor();
	qApp->processEvents();
}

void MainWindow::toggle_animwidget3d(bool t)
{
	if (t)
	{
		animAct2d->blockSignals(true);
		animAct2d->setChecked(false);
		animAct2d->blockSignals(false);
		anim2D_frame->hide();
		anim3D_frame->show();
	}
	else
	{
		anim3D_frame->hide();
	}
}

void MainWindow::toggle_animwidget2d(bool t)
{
	if (t)
	{
		animAct3d->blockSignals(true);
		animAct3d->setChecked(false);
		animAct3d->blockSignals(false);
		anim3D_frame->hide();
		anim2D_frame->show();
	}
	else
	{
		anim2D_frame->hide();
	}
}

void MainWindow::dropEvent(QDropEvent * e)
{
	const bool lock = mutex.tryLock();
	if (!lock) return;
	const QMimeData * mimeData = e->mimeData();
	QStringList l;
	if (mimeData && mimeData->hasUrls())
	{
		for (int i = 0; i < mimeData->urls().size(); ++i)
			l.push_back(mimeData->urls().at(i).toLocalFile());
	}
	if (l.size() > 0)
	{
		const QString f = l.at(0);
		QFileInfo fi(f);
		if (fi.isDir())
		{
			browser2->open_dicom_dir2(f);
			if (tabWidget->currentIndex()!=1) tabWidget->setCurrentIndex(1);
		}
		else if (
			fi.isFile() &&
			fi.fileName().toUpper() == QString("DICOMDIR"))
		{
			browser2->open_dicom_dir2(f);
			browser2->open_DICOMDIR2(f);
			if (tabWidget->currentIndex()!=1) tabWidget->setCurrentIndex(1);
		}
		else
		{
			QProgressDialog * pb = new QProgressDialog(
				QString("Loading..."),
				QString("Exit"),
				0,
				0);
			pb->setWindowModality(Qt::ApplicationModal);
			pb->setWindowFlags(
				pb->windowFlags()^Qt::WindowContextHelpButtonHint);
			connect(pb,SIGNAL(canceled()),this,SLOT(exit_null()));
			pb->setMinimumWidth(256);
			pb->setValue(-1);
			pb->show();
			qApp->processEvents();
			for (int i = 0; i < l.size(); ++i)
			{
				if (i == 0 && fi.isFile())
				{
					load_any_file(f, pb, true);
				}
				else
				{
					const QString f1 = l.at(i);
					QFileInfo fi1(f1);
					if (fi1.isFile()) load_any_file(f1, pb, true);
				}
			}
			disconnect(pb,SIGNAL(canceled()),this,SLOT(exit_null()));
			pb->close();
			delete pb;
			pb = NULL;
		}
	}
	mutex.unlock();
}

void MainWindow::dragEnterEvent(QDragEnterEvent * e)
{
	e->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent * e)
{
	e->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent * e)
{
	e->accept();
}

void MainWindow::load_any()
{
	bool lock = mutex.tryLock();
	if (!lock) return;
	QStringList l = QFileDialog::getOpenFileNames(
		this,
		QString("Open Files"),
		CommonUtils::get_open_dir(),
		QString(),
		(QString*)NULL,
		(QFileDialog::ReadOnly
		//| QFileDialog::DontUseNativeDialog
		));
	QProgressDialog * pb = new QProgressDialog(QString("Loading..."), QString("Exit"), 0, 0);
	connect(pb,SIGNAL(canceled()),this,SLOT(exit_null()));
	pb->setWindowModality(Qt::ApplicationModal);
	pb->setWindowFlags(pb->windowFlags()^Qt::WindowContextHelpButtonHint);
	pb->setMinimumWidth(256);
	pb->setValue(-1);
	pb->show();
	qApp->processEvents();
	bool is_dicomdir = false;
	for (int x = 0; x < l.size(); ++x)
	{
		QFileInfo fi(l.at(x));
		CommonUtils::set_open_dir(fi.absolutePath());
		if (x == 0 && fi.isFile() && fi.fileName().toUpper()==QString("DICOMDIR"))
		{
			is_dicomdir = true;
			browser2->open_DICOMDIR2(l.at(0));
			break;
		}
		else
		{
			load_any_file(l.at(x),pb,true);
		}
	}
	l.clear();
	disconnect(pb,SIGNAL(canceled()),this,SLOT(exit_null()));
	pb->close();
	delete pb;
	pb = NULL;
	if (is_dicomdir)
	{
		if (tabWidget->currentIndex()!=1) tabWidget->setCurrentIndex(1);
	}
	qApp->processEvents();
	mutex.unlock();
}

void MainWindow::desktop_layout(int * width_, int * height_)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	const QScreen * screen = qApp->primaryScreen();
	if (!screen) return;
	const QRect rectr = screen->availableGeometry();
#else
	const QDesktopWidget * desktop = qApp->desktop();
	if (!desktop) return;
	const QRect rectr = desktop->availableGeometry();
#endif
	*width_  = rectr.width();
	*height_ = rectr.height();
}

void MainWindow::toggle_rect(bool t)
{
	aliza->toggle_rect(t);
}

void MainWindow::toggle_cursor(bool t)
{
	graphicswidget_m->set_show_cursor(t);
	graphicswidget_x->set_show_cursor(t);
	graphicswidget_y->set_show_cursor(t);
	graphicswidget_m->update_pixel_value(-1, -1);
	graphicswidget_x->update_pixel_value(-1, -1);
	graphicswidget_y->update_pixel_value(-1, -1);
}

void MainWindow::toggle_collisions(bool t)
{
	aliza->toggle_collisions(t);
}

void MainWindow::toggle_segmentation(bool t)
{
	if (t) transp2dAct->setIcon(trans_icon);
	else   transp2dAct->setIcon(notrans_icon);
	graphicswidget_m->set_alt_mode(t);
	graphicswidget_x->set_alt_mode(t);
	graphicswidget_y->set_alt_mode(t);
}

void MainWindow::set_view_3d(bool t)
{
	if (!t) return;
	if (!aliza) return;
	if (!aliza->check_3d()) return;
	view3d_label->setText(QString("Physical space, intensity projection"));
	frames3DAct->setVisible(true);
	trans3DAct->setVisible(true);
	toolbox->contours_checkBox->setEnabled(true);
	if (trans3DAct->isChecked())
	{
		toolbox->alpha_doubleSpinBox->show();
		toolbox->alpha_label->setEnabled(true);
	}
	else
	{
		toolbox->alpha_doubleSpinBox->hide();
		toolbox->alpha_label->setEnabled(false);
	}
	gloptionsAct->setEnabled(true);
	if (gl_frame->isHidden()) gl_frame->show();
	glwidget->set_view_3d();
	glwidget->updateGL();
}

void MainWindow::set_view_rc(bool t)
{
	if (!t) return;
	if (!aliza->check_3d()) return;
	view3d_label->setText(QString("Intensity projection"));
	frames3DAct->setVisible(false);
	trans3DAct->setVisible(false);
	toolbox->alpha_doubleSpinBox->show();
	toolbox->alpha_label->setEnabled(true);
	toolbox->contours_checkBox->setEnabled(false);
	gloptionsAct->setEnabled(true);
	if (gl_frame->isHidden()) gl_frame->show();
	glwidget->set_view_rc();
	glwidget->updateGL();
}

void MainWindow::set_show_frames_3d(bool t)
{
	aliza->set_show_frames_3d(t);
}

void MainWindow::set_ui()
{
	if (!first_image_loaded)
	{
		show2DAct->setEnabled(true);
		toolbar2D_frame->show();
		level_frame->show();
		imagesbox_frame1->show();
		zrange_frame->show();
		slider_frame->show();
		view2d_frame->show();
		info_line->show();
		view2d_label->setText("Slice view (Z)");
		actionViews2DMenu->setEnabled(true);
		if (saved_ok3d)
		{
			show3DAct->setEnabled(true);
			actionViews3DMenu->setEnabled(true);
			toolbar3D_frame->show();
			view3d_frame->show();
		}
		first_image_loaded = true;
	}
}

void MainWindow::load_dicom_series2()
{
	const bool lock = mutex.tryLock();
	if (!lock) return;
	const bool selection =
		browser2->tableWidget->selectionModel()->hasSelection();
	if (!selection)
	{
		mutex.unlock();
		return;
	}
	QProgressDialog * pb =
		new QProgressDialog(QString("Loading..."), QString("Exit"), 0, 0);
	connect(pb,SIGNAL(canceled()),this,SLOT(exit_null()));
	pb->setWindowModality(Qt::ApplicationModal);
	pb->setWindowFlags(pb->windowFlags()^Qt::WindowContextHelpButtonHint);
	pb->setMinimumWidth(256);
	pb->setValue(-1);
	pb->show();
	set_ui();
	qApp->processEvents();
	aliza->load_dicom_series(pb);
	disconnect(pb, SIGNAL(canceled()), this, SLOT(exit_null()));
	pb->close();
	delete pb;
	pb = NULL;
	qApp->processEvents();
	mutex.unlock();
}

void MainWindow::load_any_file(
	const QString & f,
	QProgressDialog * pb,
	bool lock)
{
	int image_id = -1;
	set_ui();
	aliza->load_dicom_file(&image_id, f, pb, lock);
}

void MainWindow::reset_rect2()
{
	aliza->reset_rect2();
}

void MainWindow::start_anim()
{
	if (graphicswidget_m->run__) return;
	if (aliza->is_animation_running()) return;
	const int id = aliza->get_selected_image_id();
	if (id < 0) return;
	if (histogramAct->isChecked()) return;
	if (aliza->get_image(id) &&	aliza->get_image(id)->di->idimz < 2)
		return;
	imagesbox->setEnabled(false);
	anim2Dwidget->start_pushButton->setEnabled(false);
	anim2Dwidget->stop_pushButton->setEnabled(true);
	graphicsAct_Y->setEnabled(false);
	graphicsAct_X->setEnabled(false);
	zyxAct->setEnabled(false);
	histogramview->setEnabled(false);
	toolbox2D->anim_label->show();
	aliza->start_anim();
}

void MainWindow::stop_anim()
{
	aliza->stop_anim();
	graphicsAct_Y->setEnabled(true);
	graphicsAct_X->setEnabled(true);
	zyxAct->setEnabled(true);
	histogramview->setEnabled(true);
	anim2Dwidget->start_pushButton->setEnabled(true);
	anim2Dwidget->stop_pushButton->setEnabled(false);
	if (!aliza->is_animation_running())
	{
		imagesbox->setEnabled(true);
		toolbox2D->anim_label->hide();
	}
}

void MainWindow::start_3D_anim()
{
	if (graphicswidget_m->run__)
		return;
	if (aliza->is_animation_running())
		return;
	if (aliza->get_num_images() < 2)
		return;
	const ImageVariant * v =
		aliza->get_selected_image_const();
	if (!v) return;
	bool ok = false;
	QString message_ = QString("");
	if (v->group_id < 0)
		message_ = aliza->create_group_(&ok,true);
	else
		ok = true;
	if (ok)
	{
		imagesbox->setEnabled(false);
		anim3Dwidget->start_pushButton->setEnabled(false);
		anim3Dwidget->stop_pushButton->setEnabled(true);
		toolbox2D->anim_label->show();
		aliza->start_3D_anim();
	}
	else QMessageBox::warning(
		NULL,QString("Warning"),message_);
}

void MainWindow::stop_3D_anim()
{
	aliza->stop_3D_anim();
	anim3Dwidget->start_pushButton->setEnabled(true);
	anim3Dwidget->stop_pushButton->setEnabled(false);
	if (!graphicswidget_m->run__)
	{
		toolbox2D->anim_label->hide();
		imagesbox->setEnabled(true);
	}
}

void MainWindow::zoom_plus_2d()
{
	if (graphicswidget_m && graphicswidget_m->graphicsview)
		graphicswidget_m->graphicsview->zoom_in();
}

void MainWindow::zoom_minus_2d()
{
	if (graphicswidget_m && graphicswidget_m->graphicsview)
		graphicswidget_m->graphicsview->zoom_out();
}

void MainWindow::zoom_plus_3d()
{
	aliza->zoom_plus_3d();
}

void MainWindow::zoom_minus_3d()
{
	aliza->zoom_minus_3d();
}

void MainWindow::flipX()
{
	aliza->flipX();
}

void MainWindow::flipY()
{
	aliza->flipY();
}

void MainWindow::reset_3d()
{
	aliza->reset_3d();
}

void MainWindow::set_show_frames_2d(bool t)
{
	distanceAct->blockSignals(true);
	if (t)
	{
		if (distanceAct->isChecked()) distanceAct->setChecked(false);
		rectAct->setIcon(nocut_icon);
	}
	else
	{
		rectAct->setIcon(cut_icon);
	}
	const short m = t ? 1 : 0;
	aliza->set_view2d_mouse_modus(m);
	distanceAct->blockSignals(false);
}

void MainWindow::toggle_distance(bool t)
{
	frames2DAct->blockSignals(true);
	if (t)
	{
		if (frames2DAct->isChecked()) frames2DAct->setChecked(false);
		rectAct->setIcon(nocut_icon);
	}
	else
	{
		rectAct->setIcon(cut_icon);
	}
	const short m = t ? 2 : 0;
	aliza->set_view2d_mouse_modus(m);
	frames2DAct->blockSignals(false);
}

/*
void MainWindow::toggle_update_contours_width(double x)
{
	graphicswidget_m->set_contours_width(x);
	if (glwidget &&
		glwidget->opengl_init_done &&
		!glwidget->no_opengl3)
		glwidget->set_contours_width((float)x);
}
*/

void MainWindow::trigger_set_level()
{
	QMessageBox::information(
		NULL,
		QString("Set Level/Window"),
		QString(
			"Use histogram widget or tool\n"
			"at the bottom of mainwindow,\n"
			"(image must be opened), use\n"
			"sliders/spinboxes."));
}

void MainWindow::tab_ind_changed(int i)
{
	switch(i)
	{
	case 0:
		file_menu->menuAction()->setVisible(true);
		settings_menu->menuAction()->setVisible(false);
		metadata_menu->menuAction()->setVisible(false);
		deidentify_menu->menuAction()->setVisible(false);
		browser_menu->menuAction()->setVisible(false);
		views_menu->menuAction()->setVisible(true);
		tools_menu->menuAction()->setVisible(true);
		break;
	case 1:
		settings_menu->menuAction()->setVisible(false);
		metadata_menu->menuAction()->setVisible(false);
		deidentify_menu->menuAction()->setVisible(false);
		tools_menu->menuAction()->setVisible(false);
		views_menu->menuAction()->setVisible(false);
		file_menu->menuAction()->setVisible(true);
		browser_menu->menuAction()->setVisible(true);
		if (browser2->is_first_run()) browser2->reload_dir();
		break;
	case 2:
		file_menu->menuAction()->setVisible(true);
		metadata_menu->menuAction()->setVisible(true);
		deidentify_menu->menuAction()->setVisible(false);
		settings_menu->menuAction()->setVisible(false);
		browser_menu->menuAction()->setVisible(false);
		views_menu->menuAction()->setVisible(false);
		tools_menu->menuAction()->setVisible(false);
		break;
	case 3:
		file_menu->menuAction()->setVisible(true);
		settings_menu->menuAction()->setVisible(false);
		metadata_menu->menuAction()->setVisible(false);
		deidentify_menu->menuAction()->setVisible(true);
		browser_menu->menuAction()->setVisible(false);
		views_menu->menuAction()->setVisible(false);
		tools_menu->menuAction()->setVisible(false);
		break;
	case 4:
		file_menu->menuAction()->setVisible(true);
		settings_menu->menuAction()->setVisible(true);
		metadata_menu->menuAction()->setVisible(false);
		deidentify_menu->menuAction()->setVisible(false);
		browser_menu->menuAction()->setVisible(false);
		views_menu->menuAction()->setVisible(false);
		tools_menu->menuAction()->setVisible(false);
		break;
	default:
		file_menu->menuAction()->setVisible(true);
		settings_menu->menuAction()->setVisible(false);
		metadata_menu->menuAction()->setVisible(false);
		deidentify_menu->menuAction()->setVisible(false);
		browser_menu->menuAction()->setVisible(false);
		views_menu->menuAction()->setVisible(false);
		tools_menu->menuAction()->setVisible(false);
		break;
	}
	qApp->processEvents();
}

void MainWindow::set_zlock(bool t)
{
	if (t) zlockAct->setIcon(anchor_icon);
	else   zlockAct->setIcon(anchor2_icon);
	oneAct->setEnabled(t);
	aliza->toggle_zlock(t);
}

void MainWindow::set_zlock_one(bool t)
{
	aliza->toggle_zlock_one(t);
}

void MainWindow::writeSettings()
{
	const Qt::DockWidgetArea da = dockWidgetArea(dockWidget1);
	const int area = (da == Qt::LeftDockWidgetArea) ? 1 : 2;
	QSettings settings(
		QSettings::IniFormat, QSettings::UserScope,
		QApplication::organizationName(), QApplication::applicationName());
	settings.setFallbacksEnabled(false);
	settings.beginGroup(QString("MainWindow"));
	if (!isMaximized())
	{
		settings.setValue(QString("size"), QVariant(size()));
		settings.setValue(QString("pos"), QVariant(pos()));
		settings.setValue(QString("dock_area"), QVariant(area));
	}
#ifdef USE_WORKSTATION_MODE
	settings.setValue(QString("open_dir"),QVariant(CommonUtils::get_open_dir()));
#endif
	settings.setValue(
		QString("hide_3d_frame"),
		((show3DAct->isChecked()) ? QVariant(QString("N")) : QVariant(QString("Y"))));
	settings.endGroup();
	browser2->writeSettings(settings);
	anonymizer->writeSettings(settings);
	settingswidget->writeSettings(settings);
}

void MainWindow::readSettings()
{
	int width_ = 0, height_ = 0;
	desktop_layout(&width_,&height_);
	const int w = static_cast<int>(static_cast<double>(width_)*0.7);
	const int h = static_cast<int>(static_cast<double>(height_)*0.7);
	QSettings settings(
		QSettings::IniFormat, QSettings::UserScope,
		QApplication::organizationName(), QApplication::applicationName());
	settings.setFallbacksEnabled(false);
	settings.beginGroup(QString("MainWindow"));
	resize(settings.value(QString("size"), QSize(w,h)).toSize());
	move(settings.value(QString("pos"), QPoint(50,50)).toPoint());
#ifdef USE_WORKSTATION_MODE
	CommonUtils::set_open_dir(settings.value(QString("open_dir"), QString("")).toString());
#endif
	settings.endGroup();
}

void MainWindow::set_style(const QString & s)
{
	change_style(s.trimmed());
}

void MainWindow::change_style(const QString & s)
{
	if (s.isEmpty()) return;
	if (s==QString("Dark Fusion"))
	{
		QColor bg(0x53, 0x59, 0x60);
		QColor tt(0x30, 0x39, 0x47);
		QPalette p;
		p.setColor(QPalette::Window, bg);
		p.setColor(QPalette::WindowText, Qt::white);
		p.setColor(QPalette::Text, Qt::white);
		p.setColor(QPalette::Disabled, QPalette::WindowText, Qt::gray);
		p.setColor(QPalette::Disabled, QPalette::Text, Qt::gray);
		p.setColor(QPalette::Base, bg);
		p.setColor(QPalette::AlternateBase, bg);
		p.setColor(QPalette::ToolTipBase, tt);
		p.setColor(QPalette::ToolTipText, Qt::white);
		p.setColor(QPalette::Button, bg);
		p.setColor(QPalette::ButtonText, Qt::white);
		p.setColor(QPalette::BrightText, Qt::white);
		p.setColor(QPalette::Link, Qt::darkBlue);
		p.setColor(QPalette::Highlight, Qt::lightGray);
		p.setColor(QPalette::HighlightedText, Qt::black);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QApplication::setStyle(QString("Fusion"));
#else
		QApplication::setStyle(QString("Plastique"));
#endif
		QApplication::setPalette(p);
	}
	else
	{
		QApplication::setStyle(s);
#if 0
		if (!(
			(s.toUpper() == QString("WINDOWSVISTA")) ||
			(s.toUpper() == QString("MACOS"))))
#endif
		{
			QApplication::setPalette(
				QApplication::style()->standardPalette());
		}
	}
	if (glwidget)         glwidget->update_clear_color();
	if (graphicswidget_m) graphicswidget_m->update_background_color();
	if (graphicswidget_y) graphicswidget_y->update_background_color();
	if (graphicswidget_x) graphicswidget_x->update_background_color();
	if (toolbox2D)        toolbox2D->set_style_sheet();
	if (slider_m)         slider_m->set_style_sheet();
	if (slider_y)         slider_y->set_style_sheet();
	if (slider_x)         slider_x->set_style_sheet();
	if (histogramview)    histogramview->update_bgcolor();
	if (s == QString("Dark Fusion"))
		imagesbox->update_background_color(true);
	else
		imagesbox->update_background_color(false);
	update_info_lines_bg();
}

void MainWindow::set_no_gl3()
{
	settingswidget->force_no_gl3();
	settingswidget->set_gl_visible(false);
	hide_gl3_frame_later = true;
	saved_ok3d = false;
#if 1
	const QString a("\nFailed to initialize OpenGL 3\n");
	std::cout << a.toStdString() << std::endl;
#endif
}

void MainWindow::check_3d_frame()
{
	if (hide_gl3_frame_later)
	{
		show3DAct->blockSignals(true);
		toggle_showgl(false);
		show3DAct->setChecked(false);
		show3DAct->blockSignals(false);
	}
	else
	{
		QSettings settings(
			QSettings::IniFormat,
			QSettings::UserScope,
			QApplication::organizationName(),
			QApplication::applicationName());
		settings.setFallbacksEnabled(true);
		settings.beginGroup(QString("MainWindow"));
		const QString s = settings.value(
			QString("hide_3d_frame"), QString("N")).toString();
		settings.endGroup();
		if (s == QString("Y"))
		{
			show3DAct->blockSignals(true);
			toggle_showgl(false);
			show3DAct->setChecked(false);
			show3DAct->blockSignals(false);
		}
	}
}

void MainWindow::trigger_image_dicom_meta()
{
	const ImageVariant * v = aliza->get_selected_image_const();
	if (!v) return;
	const QStringList & l = v->filenames;
	if (l.empty())
	{
		QMessageBox::information(
			NULL,
			QString("Information"),
			QString("DICOM source files are not available"));
		return;
	}
	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
	qApp->processEvents();
	sqtree->set_list_of_files(l);
	sqtree->read_file(l.at(0));
	tabWidget->setCurrentIndex(2);
	qApp->restoreOverrideCursor();
	qApp->processEvents();
}

void MainWindow::set_image_view()
{
	if (tabWidget->currentIndex() != 0)
	{
		tabWidget->setCurrentIndex(0);
	}
}

void MainWindow::update_info_lines_bg()
{
	const QColor c = qApp->palette().color(QPalette::Window);
	const QString s =
		QString("QLineEdit { background-color: rgb(") +
		QVariant(c.red()).toString() + QString(", ") +
		QVariant(c.green()).toString() + QString(", ") +
		QVariant(c.blue()).toString() + QString("); }");
	info_line->setStyleSheet(s);
	info_lineZ->setStyleSheet(s);
	info_lineY->setStyleSheet(s);
	info_lineX->setStyleSheet(s);
}

void MainWindow::ask_close()
{
	QMessageBox::StandardButton r;
	r = QMessageBox::question(NULL, QString("Close Application"), QString("Close application?"));
	if (r == QMessageBox::Yes || r == QMessageBox::Ok)
	{
		this->close();
	}
}
