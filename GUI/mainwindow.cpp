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
#include "commonutils.h"
#include "infodialog.h"

namespace
{

static bool init_done = false;

}

MainWindow::MainWindow(
	bool ok3d,
	bool hide_zoom_)
{
	setupUi(this);
	setDocumentMode(false);
	scale_icons = 1.0f;
	adjust_scale_icons = 1.2f;
	int dock_area{2};
	QString saved_style;
	//
	QDateTime date_(QDateTime::currentDateTime());
	QString date_str(date_.toString(QString("hhmmsszzz")));
	//
	int swidth{};
	int sheight{};
	int adj_fps{};
	int adj_fps_value{};
	desktop_layout(&swidth, &sheight);
	//
	{
		QSettings settings(
			QSettings::IniFormat, QSettings::UserScope,
			QApplication::organizationName(), QApplication::applicationName());
		settings.beginGroup(QString("GlobalSettings"));
		adjust_scale_icons =
			static_cast<float>(settings.value(QString("scale_ui_icons"), 1.2).toDouble());
		saved_style =
			settings.value(QString("stylename"), QVariant(QString("Dark Fusion"))).toString();
		const int mvsep = settings.value(QString("mvsep"), 0).toInt();
		adj_fps = settings.value(QString("adj_fps"), 0).toInt();
		adj_fps_value = settings.value(QString("adj_fps_value"), 14).toInt();
		settings.endGroup();
		multiview_tab = (mvsep != 1);
		const int w = static_cast<int>(static_cast<double>(swidth) * 0.7);
		const int h = static_cast<int>(static_cast<double>(sheight) * 0.7);
		const QSize s = (w > 0 && h > 0) ? QSize(w, h) : QSize(1280, 720);
		settings.beginGroup(QString("MainWindow"));
		dock_area = settings.value(QString("dock_area"), 2).toInt();
		resize(settings.value(QString("size"), s).toSize());
		move(settings.value(QString("pos"), QPoint(50, 50)).toPoint());
#ifdef USE_WORKSTATION_MODE
		CommonUtils::set_open_dir(settings.value(QString("open_dir"), QString("")).toString());
#endif
		settings.endGroup();
	}
	dockWidget1 = new QDockWidget(this);
	dockWidget1->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dockWidget1->setFloating(false);
	dockWidget1->setFeatures(QDockWidget::DockWidgetMovable);
	addDockWidget(
		(dock_area == 2) ? Qt::RightDockWidgetArea : Qt::LeftDockWidgetArea,
		dockWidget1);
	imagesbox_frame1 = new QFrame(this);
	imagesbox_frame1->setFrameShape(QFrame::NoFrame);
	imagesbox_frame1->setFrameShadow(QFrame::Plain);
	imagesbox = new ImagesBox(scale_icons * adjust_scale_icons);
	if (saved_style != QString("Dark Fusion"))
	{
		update_info_lines_bg();
		imagesbox->update_background_color(false);
	}
	QVBoxLayout * l1 = new QVBoxLayout(imagesbox_frame1);
	l1->setSpacing(0);
	l1->setContentsMargins(0, 0, 0, 0);
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
	if (force_vertical || (sheight > swidth))
	{
		QVBoxLayout * vl991 = new QVBoxLayout(views_frame);
		vl991->setContentsMargins(0, 0, 0, 0);
		vl991->setSpacing(0);
		vl991->addWidget(frame2D);
		vl991->addWidget(frame3D);
#if 1
		if (sheight > 1920) scale_icons = sheight / 1920.0f;
#endif
	}
	else
	{
		QHBoxLayout * vl991 = new QHBoxLayout(views_frame);
		vl991->setContentsMargins(0, 0, 0, 0);
		vl991->setSpacing(0);
		vl991->addWidget(frame2D);
		vl991->addWidget(frame3D);
#if 1
		if (swidth > 1920) scale_icons = swidth / 1920.0f;
#endif
	}
	aboutwidget = new AboutWidget(swidth, sheight);
	aboutwidget->hide();
	//
	toolbox = new ToolBox();
	QVBoxLayout * vl0 = new QVBoxLayout(toolbox3D_frame);
	vl0->setContentsMargins(0, 0, 0, 0);
	vl0->setSpacing(0);
	vl0->addWidget(toolbox);
	//
	anim3Dwidget = new AnimWidget(scale_icons*adjust_scale_icons);
	anim3Dwidget->label->setText(QString("3D+t"));
	QVBoxLayout * vl98 = new QVBoxLayout(anim3D_frame);
	vl98->setContentsMargins(0, 0, 0, 0);
	vl98->setSpacing(0);
	vl98->addWidget(anim3Dwidget);
	//
	anim2Dwidget = new AnimWidget(scale_icons*adjust_scale_icons);
	anim2Dwidget->label->setText(QString("2D+t"));
	anim2Dwidget->group_pushButton->hide();
	anim2Dwidget->remove_pushButton->hide();
	anim2Dwidget->t_checkBox->hide();
	QVBoxLayout * vl99 = new QVBoxLayout(anim2D_frame);
	vl99->setContentsMargins(0, 0, 0, 0);
	vl99->setSpacing(0);
	vl99->addWidget(anim2Dwidget);
	//
	toolbox2D = new ToolBox2D(scale_icons*adjust_scale_icons);
	QVBoxLayout * l2 = new QVBoxLayout(level_frame);
	l2->setSpacing(0);
	l2->setContentsMargins(0, 0, 0, 0);
	l2->addWidget(toolbox2D);
	//
	lutwidget2 = new LUTWidget(scale_icons*adjust_scale_icons);
	lutwidget2->add_items1();
	//
	if (hide_zoom)
	{
		zoomwidget2D = nullptr;
		zoomwidget3D = nullptr;
	}
	else
	{
		zoomwidget2D = new ZoomWidget(scale_icons*adjust_scale_icons);
		zoomwidget3D = new ZoomWidget(scale_icons*adjust_scale_icons);
	}
	//
	frame2D_viewer_layout = new QVBoxLayout(frame2D_viewer);
	frame2D_viewer_layout->setSpacing(0);
	frame2D_viewer_layout->setContentsMargins(0, 0, 0, 0);
	//
	frame2D_viewerZ_layout = new QVBoxLayout(frame2D_viewerZ);
	frame2D_viewerZ_layout->setSpacing(0);
	frame2D_viewerZ_layout->setContentsMargins(0, 0, 0, 0);
	//
	slider_m  = new SliderWidget();
	slider_frame_layout = new QVBoxLayout(slider_frame);
	slider_frame_layout->setSpacing(0);
	slider_frame_layout->setContentsMargins(0, 0, 0, 0);
	slider_frame_layoutZ = new QVBoxLayout(slider_frameZ);
	slider_frame_layoutZ->setSpacing(0);
	slider_frame_layoutZ->setContentsMargins(0, 0, 0, 0);
	//
	slider_y  = new SliderWidget();
	QVBoxLayout * l10 = new QVBoxLayout(slider_frameY);
	l10->setSpacing(0);
	l10->setContentsMargins(0, 0, 0, 0);
	l10->addWidget(slider_y);
	//
	slider_x  = new SliderWidget();
	QVBoxLayout * l11 = new QVBoxLayout(slider_frameX);
	l11->setSpacing(0);
	l11->setContentsMargins(0, 0, 0, 0);
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
	l7->setContentsMargins(0, 0, 0, 0);
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
	l8->setContentsMargins(0, 0, 0, 0);
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
	l13->setContentsMargins(0, 0, 0, 0);
	l13->addWidget(zrangewidget);
	//
	sqtree = new SQtree(true);
	QVBoxLayout * vl296 = new QVBoxLayout(metadata_frame);
	vl296->setContentsMargins(0, 0, 0, 0);
	vl296->setSpacing(0);
	vl296->addWidget(sqtree);
	//
	browser2 = new BrowserWidget2(scale_icons*adjust_scale_icons);
	QVBoxLayout * vl396 = new QVBoxLayout(browser2_frame);
	vl396->setContentsMargins(0, 0, 0, 0);
	vl396->setSpacing(0);
	vl396->addWidget(browser2);
	//
	anonymizer = new AnonymazerWidget2(scale_icons*adjust_scale_icons);
	QVBoxLayout * vl196 = new QVBoxLayout(deidentify_frame);
	vl196->setContentsMargins(0, 0, 0, 0);
	vl196->setSpacing(0);
	vl196->addWidget(anonymizer);
	//
	settingswidget = new SettingsWidget(scale_icons);
	QVBoxLayout * vl496 = new QVBoxLayout(settings_frame);
	vl496->setContentsMargins(0, 0, 0, 0);
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
#ifdef ALIZA_GL_3_2_CORE
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
		vl2->setContentsMargins(0, 0, 0, 0);
		vl2->addWidget(glwidget);
		//
		if (adj_fps == 1)
		{
			glwidget->set_adjust(adj_fps_value);
		}
		else
		{
			glwidget->set_adjust(0);
		}
	}
	else
	{
		glwidget = nullptr;
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
	if (multiview_tab)
	{
		studyview = new StudyViewWidget((sheight > swidth), true, scale_icons * adjust_scale_icons);
		studyview->setAutoFillBackground(true);
		tabWidget->setUpdatesEnabled(false);
		tabWidget->insertTab(1, static_cast<QWidget*>(studyview), QString("Multi view"));
		tabWidget->setUpdatesEnabled(true);
	}
	else
	{
		studyview = new StudyViewWidget((sheight > swidth), false, scale_icons * adjust_scale_icons);
	}
	//
	createActions();
	createMenus();
	createToolBars();
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
		oneAct,
		show3DAct);
	aliza->set_2D_views_actions(
		frames2DAct,
		distanceAct,
		rectAct,
		transp2dAct,
		cursorAct,
		collisionAct,
		show2DAct);
	aliza->set_uniq_string(date_str);
	aliza->connect_slots();
	//
	graphicswidget_m->set_aliza(aliza);
	graphicswidget_x->set_aliza(aliza);
	graphicswidget_y->set_aliza(aliza);
	//
	studyview->init_(aliza);
	sqtree->set_aliza(aliza);
	//
	histogramview = new HistogramView(
		this, static_cast<QObject*>(aliza), multi_frame, false);
	histogram_frame_layout = new QVBoxLayout(histogram_frame);
	histogram_frame_layout->setSpacing(0);
	histogram_frame_layout->setContentsMargins(0, 0, 0, 0);
	histogram_frame_layout->addWidget(histogramview);
	histogram_frame2_layout = new QVBoxLayout(histogram_frame2);
	histogram_frame2_layout->setSpacing(0);
	histogram_frame2_layout->setContentsMargins(0, 0, 0, 0);
	aliza->set_histogramview(histogramview);
	//
	first_image_loaded = false;
	connect(aliza, SIGNAL(report_load_to_mainwin()), this, SLOT(set_ui()));
	//
	if (ok3d && glwidget)
	{
		gl_frame->show();
		glwidget->show();
		view3d_label->setText(QString("Physical space, intensity projection, GPU"));
		slicesAct->setChecked(true);
		raycastAct->setChecked(false);
	}
	else
	{
		gl_frame->hide();
		slicesAct->setEnabled(false);
		raycastAct->setEnabled(false);
		trans3DAct->setEnabled(false);
		gloptionsAct->setEnabled(false);
		frames3DAct->setEnabled(false);
		frame3D->hide();
		settingswidget->set_gl_visible(false);
		show3DAct->setEnabled(false);
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
	connect(imagesbox->width_doubleSpinBox, SIGNAL(valueChanged(double)),this,SLOT(toggle_update_contours_width(double)));
	connect(setLevelAct,                    SIGNAL(triggered()),         this,SLOT(trigger_set_level()));
	connect(zlockAct,                       SIGNAL(toggled(bool)),       this,SLOT(set_zlock(bool)));
	connect(oneAct,                         SIGNAL(toggled(bool)),       this,SLOT(set_zlock_one(bool)));
	connect(browser2->meta_pushButton,      SIGNAL(clicked()),           this,SLOT(toggle_meta2()));
	connect(browser_open_dir_act,           SIGNAL(triggered()),         browser2,SLOT(open_dicom_dir()));
	connect(browser_open_dcmdir_act,        SIGNAL(triggered()),         browser2,SLOT(open_DICOMDIR()));
#ifdef USE_WORKSTATION_MODE
	connect(browser_open_ctk_act,           SIGNAL(triggered()),         browser2,SLOT(open_CTK_db()));
#endif
	connect(browser_reload_act,             SIGNAL(triggered()),         browser2,SLOT(reload_dir()));
	connect(browser_metadata_act,           SIGNAL(triggered()),         this,    SLOT(toggle_meta2()));
	connect(browser_copy_act,               SIGNAL(triggered()),         browser2,SLOT(copy_files()));
	connect(browser_load_act,               SIGNAL(triggered()),         this,    SLOT(load_dicom_series2()));
	connect(meta_open_act,                  SIGNAL(triggered()),         sqtree,  SLOT(open_file()));
	connect(meta_open_scan_act,             SIGNAL(triggered()),         sqtree,  SLOT(open_file_and_series()));
	connect(anon_open_in_dir,               SIGNAL(triggered()),         anonymizer,SLOT(set_input_dir()));
	connect(anon_open_out_dir,              SIGNAL(triggered()),         anonymizer,SLOT(set_output_dir()));
	connect(anon_run,                       SIGNAL(triggered()),         anonymizer,SLOT(run_()));
	connect(anon_help,                      SIGNAL(triggered()),         anonymizer,SLOT(show_help()));
	connect(tabWidget,                      SIGNAL(currentChanged(int)), this,    SLOT(tab_ind_changed(int)));
	connect(imagesbox->actionDICOMMeta,     SIGNAL(triggered()),         this,    SLOT(trigger_image_dicom_meta()));
	connect(aliza,                          SIGNAL(image_opened()),      this,    SLOT(set_image_view()));
	connect(imagesbox->actionStudy,         SIGNAL(triggered()),         this,    SLOT(trigger_studyview()));
	connect(imagesbox->actionStudyChecked,  SIGNAL(triggered()),         this,    SLOT(trigger_studyview_checked()));
	connect(imagesbox->actionStudyEmpty,    SIGNAL(triggered()),         this,    SLOT(trigger_studyview_empty()));
	connect(set_default_settings,           SIGNAL(triggered()),         this,    SLOT(trigger_default_settings()));
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	connect(settingswidget->styleComboBox,  SIGNAL(currentTextChanged(const QString&)),this, SLOT(set_style(const QString&)));
#else
	connect(settingswidget->styleComboBox,  SIGNAL(currentIndexChanged(const QString&)),this, SLOT(set_style(const QString&)));
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
	init_done = true;
}

void MainWindow::open_args(const QStringList & l)
{
	if (l.empty()) return;
	if (lock0) return;
	lock0 = true;
	QString message;
	bool skip_next{};
	QStringList l2;
	const bool dcm_thread = settingswidget->get_dcm_thread();
	for (int i = 0; i < l.size(); ++i)
	{
		if (!skip_next)
		{
			const QString f = l.at(i);
			if (f == QString("-platform")    ||
				f == QString("--platform")   ||
				f == QString("-style")       ||
				f == QString("--style")      ||
				f == QString("-stylesheet")  ||
				f == QString("--stylesheet"))
			{
				skip_next = true;
			}
			else if (!f.startsWith(QString("-")))
			{
				l2.push_back(f);
				skip_next = false;
			}
			else
			{
				skip_next = false;
			}
		}
	}
	if (l2.size() == 0)
	{
		lock0 = false;
		return;
	}
	QProgressDialog * pb = new QProgressDialog(
		QString("Loading ..."),
		QString("Exit"),
		0,
		0);
	{
		QList<QPushButton *> lb = pb->findChildren<QPushButton*>();
		for (int x = 0; x < lb.size(); ++x) // one button
		{
			if (dcm_thread)
			{
				lb[x]->setStyleSheet("QPushButton { color: #8B0000; }");
			}
			else
			{
				lb[x]->hide();
			}
		}
	}
	pb->setModal(true);
	pb->setWindowFlags(pb->windowFlags() ^ Qt::WindowContextHelpButtonHint);
	if (dcm_thread)
	{
		connect(pb,SIGNAL(canceled()), this, SLOT(exit_null()));
	}
	pb->setMinimumWidth(256);
	pb->setRange(0, 0);
	pb->setMinimumDuration(0);
	if (l2.size() == 1)
	{
		const int t = (multiview_tab) ? 2 : 1;
		const QString f = l2.at(0);
		QFileInfo fi(f);
		if (fi.isFile() &&
			fi.fileName().toUpper() == QString("DICOMDIR"))
		{
			browser2->open_DICOMDIR2(fi.absoluteFilePath());
			tabWidget->setCurrentIndex(t);
		}
		else if (fi.isDir())
		{
			browser2->open_dicom_dir2(fi.absoluteFilePath());
			tabWidget->setCurrentIndex(t);
		}
		else if (fi.isFile())
		{
			pb->setValue(0);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
			pb->show();
#endif
			qApp->processEvents();
			message = load_any_file(f, pb);
		}
	}
	else if (l2.size() > 1)
	{
		pb->setValue(0);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
		pb->show();
#endif
		qApp->processEvents();
		for (int x = 0; x < l2.size(); ++x)
		{
			const QString f = l2.at(x);
			QFileInfo fi(f);
			if (fi.isFile())
			{
				message = load_any_file(l2.at(x), pb);
			}
		}
	}
	if (dcm_thread)
	{
		disconnect(pb,SIGNAL(canceled()), this, SLOT(exit_null()));
	}
	pb->close();
	delete pb;
	if (!message.isEmpty())
	{
		InfoDialog info;
		info.set_text(message);
		info.exec();
	}
	lock0 = false;
}

/*
Closing is a little bit over-complicated, because there was an issue
with Citrix (Windows Server 2016). Can not reproduce the issue,
but is trying to workaround.
*/
void MainWindow::close_app()
{
	if (init_done)
	{
		tabWidget->disconnect(SIGNAL(currentChanged(int)));
		aliza->close_();
		delete aliza;
		aliza = nullptr;
		aboutwidget->close();
		delete aboutwidget;
		aboutwidget = nullptr;
		delete studyview;
		studyview = nullptr;
		// 'init_done' variable is not required, just for logic.
		init_done = false;
	}
	emit quit_app();
	qApp->processEvents();
}

// Connected to 'quit_app' signal in main.cpp.
void MainWindow::exit_app()
{
	qApp->closeAllWindows();
#if 1
	// S. setQuitOnLastWindowClosed in main.cpp, should be set to 'false',
	// default is 'true'.
	qApp->exit(0);
#endif
}

void MainWindow::closeEvent(QCloseEvent * e)
{
	writeSettings();
	e->accept();
	close_app();
}

void MainWindow::resizeEvent(QResizeEvent * e)
{
	QMainWindow::resizeEvent(e);
}

// It is used only to let a user abort some processes
// and exit if there is no better way to stop them,
// bad, should be used only as exception.
void MainWindow::exit_null()
{
	exit(0);
}

void MainWindow::about()
{
	const QString tmp0 = glwidget ? glwidget->get_system_info() : QString("");
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
		QString("Physical space, intensity projection, GPU"), this);
	slicesAct->setCheckable(true);
	slicesAct->setChecked(true);
	view_group->addAction(slicesAct);
	raycastAct = new QAction(QIcon(QString(":/bitmaps/ray.svg")),
		QString("Intensity projection, GPU"), this);
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
#ifdef USE_WORKSTATION_MODE
	browser_open_ctk_act    = new QAction(QIcon(QString(":/bitmaps/ctk.svg")),   QString("Open CTK database"), this);
#endif
	browser_reload_act      = new QAction(QIcon(QString(":/bitmaps/reload.svg")),QString("Reload"), this);
	browser_metadata_act    = new QAction(QIcon(QString(":/bitmaps/meta.svg")),  QString("Show metadata"), this);
	browser_copy_act        = new QAction(QIcon(QString(":/bitmaps/copy2.svg")), QString("Copy selected"), this);
	browser_load_act        = new QAction(QIcon(QString(":/bitmaps/right0.svg")),QString("Load selected"), this);
	meta_open_act           = new QAction(QIcon(QString(":/bitmaps/file.svg")),  QString("Open file"), this);
	meta_open_scan_act      = new QAction(QIcon(QString(":/bitmaps/align.svg")), QString("Open file and scan"), this);
	anon_open_in_dir        = new QAction(QIcon(QString(":/bitmaps/folder.svg")),QString("Open input directory"), this);
	anon_open_out_dir       = new QAction(QIcon(QString(":/bitmaps/folder.svg")),QString("Open output directory"), this);
	anon_run                = new QAction(QIcon(QString(":/bitmaps/right0.svg")),QString("De-identify"), this);
	anon_help               = new QAction(QIcon(QString(":/bitmaps/info2.svg")), QString("Help"), this);
	set_default_settings    = new QAction(QString("Reset to defaults (restart is required)"), this);
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
	actionViews2DMenu  = new QAction(QString("2D Views"), this);
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
	actionViews3DMenu  = new QAction(QString("3D Views"), this);
	QMenu * views3d_menu = new QMenu(this);
	views3d_menu->addAction(slicesAct);
	views3d_menu->addAction(raycastAct);
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
	//
	QAction * actionMultiView = new QAction(QString("Multi view"), this);
	QMenu * multi_view_menu = new QMenu(this);
	multi_view_menu->addAction(imagesbox->actionStudy);
	multi_view_menu->addAction(imagesbox->actionStudyEmpty);
	multi_view_menu->addAction(imagesbox->actionStudyChecked);
	actionMultiView->setMenu(multi_view_menu);
	tools_menu->addAction(actionMultiView);
	//
	tools_menu->addAction(imagesbox->actionReloadHistogram);
	tools_menu->addAction(animAct2d);
	tools_menu->addAction(animAct3d);
	tools_menu->addAction(cursorAct);
	tools_menu->addAction(collisionAct);
	tools_menu->addAction(distanceAct);
	tools_menu->addAction(frames2DAct);
	QAction * actionToolsSelectMenu = new QAction(QString("Select sub-image"), this);
	QMenu * tools_select_menu = new QMenu(this);
	tools_select_menu->addAction(rectAct);
	tools_select_menu->addAction(zrangeAct);
	actionToolsSelectMenu->setMenu(tools_select_menu);
	tools_menu->addAction(actionToolsSelectMenu);
	QAction * actionTools2DMenu = new QAction(QString("2D Views"), this);
	QMenu * tools2d_menu = new QMenu(this);
	tools2d_menu->addAction(transp2dAct);
	tools2d_menu->addAction(resetRectAct2);
	tools2d_menu->addAction(flipXAct);
	tools2d_menu->addAction(flipYAct);
	actionTools2DMenu->setMenu(tools2d_menu);
	tools_menu->addAction(actionTools2DMenu);
	QAction * actionTools3DMenu = new QAction(QString("3D Views"), this);
	QMenu * tools3d_menu = new QMenu(this);
	tools3d_menu->addAction(trans3DAct);
	tools3d_menu->addAction(frames3DAct);
	tools3d_menu->addAction(gloptionsAct);
	tools3d_menu->addAction(reset3DAct);
	actionTools3DMenu->setMenu(tools3d_menu);
	tools_menu->addAction(actionTools3DMenu);
	tools_menu->addAction(setLevelAct);
	//
	if (multiview_tab)
	{
		multiview_menu = menuBar()->addMenu(QString("Multi view"));
		multiview_menu->addAction(studyview->fitall_Action);
		multiview_menu->addAction(studyview->scouts_Action);
		multiview_menu->addAction(studyview->measure_Action);
		multiview_menu->addAction(studyview->anchor_Action);
		multiview_menu->menuAction()->setVisible(false);
	}
	else
	{
		multiview_menu = nullptr;
	}
	//
	browser_menu = menuBar()->addMenu(QString("DICOM scanner"));
	browser_menu->addAction(browser_open_dir_act);
	browser_menu->addAction(browser_open_dcmdir_act);
#ifdef USE_WORKSTATION_MODE
	browser_menu->addAction(browser_open_ctk_act);
#endif
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
	deidentify_menu->addAction(anon_help);
	deidentify_menu->menuAction()->setVisible(false);
	//
	settings_menu = menuBar()->addMenu(QString("Settings"));
	settings_menu->addAction(set_default_settings);
	settings_menu->menuAction()->setVisible(false);
}

void MainWindow::createToolBars()
{
	toolbar4 = new QToolBar(this);
	toolbar4->setIconSize(
		QSize(static_cast<int>(18 * scale_icons*adjust_scale_icons),
			static_cast<int>(18 * scale_icons*adjust_scale_icons)));
	if (toolbar4->layout())
	{
		toolbar4->layout()->setContentsMargins(0, 0, 0, 0);
		toolbar4->layout()->setSpacing(2);
	}
	QHBoxLayout * l4 = new QHBoxLayout(view2d_frame);
	l4->setContentsMargins(0, 0, 0, 0);
	l4->setSpacing(0);
	l4->addWidget(toolbar4);
	view2d_label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	view2d_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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
		QSize(static_cast<int>(18 * scale_icons*adjust_scale_icons),
			static_cast<int>(18 * scale_icons*adjust_scale_icons)));
	if (toolbar5->layout())
	{
		toolbar5->layout()->setContentsMargins(0, 0, 0, 0);
		toolbar5->layout()->setSpacing(2);
	}
	QHBoxLayout * l5 = new QHBoxLayout(view3d_frame);
	l5->setContentsMargins(0, 0, 0, 0);
	l5->setSpacing(0);
	l5->addWidget(toolbar5);
	view3d_label->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	view3d_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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
		QSize(static_cast<int>(18 * scale_icons*adjust_scale_icons),
			static_cast<int>(18 * scale_icons*adjust_scale_icons)));
	if (toolbar2->layout())
	{
		toolbar2->layout()->setContentsMargins(0, 0, 0, 0);
		toolbar2->layout()->setSpacing(2);
	}
	QHBoxLayout * l2 = new QHBoxLayout(toolbar2D_frame);
	l2->setContentsMargins(0, 0, 0, 0);
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
		QSize(static_cast<int>(18 * scale_icons*adjust_scale_icons),
			static_cast<int>(18 * scale_icons*adjust_scale_icons)));
	if (toolbar3->layout())
	{
		toolbar3->layout()->setContentsMargins(0, 0, 0, 0);
		toolbar3->layout()->setSpacing(2);
	}
	QHBoxLayout * l3 = new QHBoxLayout(toolbar3D_frame);
	l3->setContentsMargins(0, 0, 0, 0);
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
	const int t = (multiview_tab) ? 2 : 1;
	if (tabWidget->currentIndex() == t)
		browser2->open_dicom_dir();
	else
		tabWidget->setCurrentIndex(t);
	qApp->processEvents();
}

void MainWindow::toggle_settingswidget()
{
	const int t = (multiview_tab) ? 5 : 4;
	tabWidget->setCurrentIndex(t);
#ifdef __APPLE__
	if (isMinimized()) showNormal();
	raise();
	activateWindow();
#endif
	qApp->processEvents();
}

void MainWindow::toggle_showgl(bool t)
{
	if (t)
	{
		frame3D->show();
		toolbar3D_frame->show();
		view3d_frame->show();
	}
	else
	{
		frame3D->hide();
	}
	if (first_image_loaded)
	{
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
	if (first_image_loaded)
	{
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
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m) > 0)
		frame2D_viewerZ_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewer_layout->indexOf(graphicswidget_m) < 0)
	{
		frame2D_viewer_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_label);
		graphicswidget_m->set_left_label(left_label);
		graphicswidget_m->set_measure_label(measure_label);
		graphicswidget_m->set_info_line(info_line);
	}
	if (slider_frame_layoutZ->indexOf(slider_m) > 0)
		slider_frame_layoutZ->removeWidget(slider_m);
	if (slider_frame_layout->indexOf(slider_m) < 0)
		slider_frame_layout->addWidget(slider_m);
	if (graphicswidget_y) graphicswidget_y->clear_();
	if (graphicswidget_x) graphicswidget_x->clear_();
	graphicswidget_frame->show();
	aliza->set_axis_2D(0, rectAct->isChecked());
	qApp->processEvents();
}

void MainWindow::toggle_graphicswidget_m_y(bool t)
{
	view2d_label->setText("Reconstruction view (Y)");
	if (!t) return;
	if (!graphicswidget_m) return;
	multi_frame->hide();
	histogram_frame->hide();
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m) > 0)
		frame2D_viewerZ_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewer_layout->indexOf(graphicswidget_m) < 0)
	{
		frame2D_viewer_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_label);
		graphicswidget_m->set_left_label(left_label);
		graphicswidget_m->set_measure_label(measure_label);
		graphicswidget_m->set_info_line(info_line);
	}
	if (slider_frame_layoutZ->indexOf(slider_m) > 0)
		slider_frame_layoutZ->removeWidget(slider_m);
	if (slider_frame_layout->indexOf(slider_m) < 0)
		slider_frame_layout->addWidget(slider_m);
	if (graphicswidget_y) graphicswidget_y->clear_();
	if (graphicswidget_x) graphicswidget_x->clear_();
	graphicswidget_frame->show();
	aliza->set_axis_2D(1, rectAct->isChecked());
	qApp->processEvents();
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
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m) > 0)
		frame2D_viewerZ_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewer_layout->indexOf(graphicswidget_m) < 0)
	{
		frame2D_viewer_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_label);
		graphicswidget_m->set_left_label(left_label);
		graphicswidget_m->set_measure_label(measure_label);
		graphicswidget_m->set_info_line(info_line);
	}
	if (slider_frame_layoutZ->indexOf(slider_m) > 0)
		slider_frame_layoutZ->removeWidget(slider_m);
	if (slider_frame_layout->indexOf(slider_m) < 0)
		slider_frame_layout->addWidget(slider_m);
	if (graphicswidget_y) graphicswidget_y->clear_();
	if (graphicswidget_x) graphicswidget_x->clear_();
	graphicswidget_frame->show();
	aliza->set_axis_2D(2, rectAct->isChecked());
	qApp->processEvents();
}

void MainWindow::toggle_histogram(bool t)
{
	view2d_label->setText("Histogram view");
	if (!t) return;
	if (!graphicswidget_m) return;
	multi_frame->hide();
	graphicswidget_frame->hide();
	if (graphicswidget_m) graphicswidget_m->clear_();
	if (graphicswidget_y) graphicswidget_y->clear_();
	if (graphicswidget_x) graphicswidget_x->clear_();
	if (histogram_frame2_layout->indexOf(histogramview) > 0)
		histogram_frame2_layout->removeWidget(histogramview);
	if (histogram_frame_layout->indexOf(histogramview) < 0)
		histogram_frame_layout->addWidget(histogramview);
	histogram_frame->show();
	aliza->set_histogram();
	qApp->processEvents();
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
	if (frame2D_viewer_layout->indexOf(graphicswidget_m) > 0)
		frame2D_viewer_layout->removeWidget(graphicswidget_m);
	if (frame2D_viewerZ_layout->indexOf(graphicswidget_m) < 0)
	{
		frame2D_viewerZ_layout->addWidget(graphicswidget_m);
		graphicswidget_m->set_top_label(top_labelZ);
		graphicswidget_m->set_left_label(left_labelZ);
		graphicswidget_m->set_measure_label(measure_labelZ);
		graphicswidget_m->set_info_line(info_lineZ);
	}
	if (slider_frame_layout->indexOf(slider_m) > 0)
		slider_frame_layout->removeWidget(slider_m);
	if (slider_frame_layoutZ->indexOf(slider_m) < 0)
		slider_frame_layoutZ->addWidget(slider_m);
	if (histogram_frame_layout->indexOf(histogramview) > 0)
		histogram_frame_layout->removeWidget(histogramview);
	if (histogram_frame2_layout->indexOf(histogramview) < 0)
		histogram_frame2_layout->addWidget(histogramview);
	multi_frame->show();
	aliza->set_axis_zyx(rectAct->isChecked());
	qApp->processEvents();
}

void MainWindow::toggle_toolbox()
{
	if (toolbox3D_frame->isVisible()) toolbox3D_frame->hide();
	else toolbox3D_frame->show();
	qApp->processEvents();
}

void MainWindow::toggle_meta2()
{
	const QStringList & l = browser2->get_files_of_1st();
	if (l.empty()) return;
	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
	const int t = (multiview_tab) ? 3 : 2;
	sqtree->set_list_of_files(l, 0, true);
	sqtree->read_file(l.at(0), true);
	tabWidget->setCurrentIndex(t);
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
	qApp->processEvents();
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
	qApp->processEvents();
}

void MainWindow::dropEvent(QDropEvent * e)
{
	if (lock0) return;
	lock0 = true;
	const QMimeData * mimeData = e->mimeData();
	QStringList l;
	QString message;
	const bool dcm_thread = settingswidget->get_dcm_thread();
	if (mimeData && mimeData->hasUrls())
	{
		for (int i = 0; i < mimeData->urls().size(); ++i)
			l.push_back(mimeData->urls().at(i).toLocalFile());
	}
	if (l.size() > 0)
	{
		const int t = (multiview_tab) ? 2 : 1;
		const QString f = l.at(0);
		QFileInfo fi(f);
		if (fi.isDir())
		{
			browser2->open_dicom_dir2(f);
			if (tabWidget->currentIndex() != t) tabWidget->setCurrentIndex(t);
		}
		else if (
			fi.isFile() &&
			fi.fileName().toUpper() == QString("DICOMDIR"))
		{
			browser2->open_DICOMDIR2(f);
			if (tabWidget->currentIndex() != t) tabWidget->setCurrentIndex(t);
		}
		else
		{
			QProgressDialog * pb = new QProgressDialog(
				QString("Loading ..."),
				QString("Exit"),
				0,
				0);
			{
				QList<QPushButton *> lb = pb->findChildren<QPushButton*>();
				for (int x = 0; x < lb.size(); ++x) // one button
				{
					if (dcm_thread)
					{
						lb[x]->setStyleSheet("QPushButton { color: #8B0000; }");
					}
					else
					{
						lb[x]->hide();
					}
				}
			}
			pb->setModal(true);
			pb->setWindowFlags(pb->windowFlags() ^ Qt::WindowContextHelpButtonHint);
			if (dcm_thread)
			{
				connect(pb, SIGNAL(canceled()), this, SLOT(exit_null()));
			}
			pb->setMinimumWidth(256);
			pb->setRange(0, 0);
			pb->setMinimumDuration(0);
			pb->setValue(0);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
			pb->show();
#endif
			qApp->processEvents();
			for (int i = 0; i < l.size(); ++i)
			{
				if (i == 0 && fi.isFile())
				{
					message = load_any_file(f, pb);
				}
				else
				{
					const QString f1 = l.at(i);
					QFileInfo fi1(f1);
					if (fi1.isFile())
					{
						message = load_any_file(f1, pb);
					}
				}
			}
			if (dcm_thread)
			{
				disconnect(pb, SIGNAL(canceled()), this, SLOT(exit_null()));
			}
			pb->close();
			delete pb;
		}
	}
	if (!message.isEmpty())
	{
		InfoDialog info;
		info.set_text(message);
		info.exec();
	}
	lock0 = false;
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
	if (lock0) return;
	lock0 = true;
	QString message;
	const bool dcm_thread = settingswidget->get_dcm_thread();
	QStringList l = QFileDialog::getOpenFileNames(
		this,
		QString("Open Files"),
		CommonUtils::get_open_dir(),
		QString(),
		nullptr,
		(QFileDialog::ReadOnly
		//| QFileDialog::DontUseNativeDialog
		));
	QProgressDialog * pb = new QProgressDialog(QString("Loading ..."), QString("Exit"), 0, 0);
	{
		QList<QPushButton *> lb = pb->findChildren<QPushButton*>();
		for (int x = 0; x < lb.size(); ++x) // one button
		{
			if (dcm_thread)
			{
				lb[x]->setStyleSheet("QPushButton { color: #8B0000; }");
			}
			else
			{
				lb[x]->hide();
			}
		}
	}
	if (dcm_thread)
	{
		connect(pb, SIGNAL(canceled()), this, SLOT(exit_null()));
	}
	pb->setModal(true);
	pb->setWindowFlags(pb->windowFlags() ^ Qt::WindowContextHelpButtonHint);
	pb->setMinimumWidth(256);
	pb->setRange(0, 0);
	pb->setMinimumDuration(0);
	pb->setValue(0);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	pb->show();
#endif
	qApp->processEvents();
	bool is_dicomdir = false;
	for (int x = 0; x < l.size(); ++x)
	{
		QFileInfo fi(l.at(x));
		CommonUtils::set_open_dir(fi.absolutePath());
		if (x == 0 && fi.isFile() && fi.fileName().toUpper() == QString("DICOMDIR"))
		{
			is_dicomdir = true;
			browser2->open_DICOMDIR2(l.at(0));
			break;
		}
		else
		{
			message = load_any_file(l.at(x), pb);
		}
	}
	l.clear();
	if (dcm_thread)
	{
		disconnect(pb, SIGNAL(canceled()), this, SLOT(exit_null()));
	}
	pb->close();
	delete pb;
	if (is_dicomdir)
	{
		const int t = (multiview_tab) ? 2 : 1;
		if (tabWidget->currentIndex() != t) tabWidget->setCurrentIndex(t);
	}
	if (!message.isEmpty())
	{
		InfoDialog info;
		info.set_text(message);
		info.exec();
	}
	lock0 = false;
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
	view3d_label->setText(QString("Physical space, intensity projection, GPU"));
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
	qApp->processEvents();
}

void MainWindow::set_view_rc(bool t)
{
	if (!t) return;
	if (!aliza->check_3d()) return;
	view3d_label->setText(QString("Intensity projection, GPU"));
	frames3DAct->setVisible(false);
	trans3DAct->setVisible(false);
	toolbox->alpha_doubleSpinBox->show();
	toolbox->alpha_label->setEnabled(true);
	toolbox->contours_checkBox->setEnabled(false);
	gloptionsAct->setEnabled(true);
	if (gl_frame->isHidden()) gl_frame->show();
	glwidget->set_view_rc();
	glwidget->updateGL();
	qApp->processEvents();
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
		if (glwidget && !glwidget->no_opengl3)
		{
			show3DAct->setEnabled(true);
			actionViews3DMenu->setEnabled(true);
			toolbar3D_frame->show();
			view3d_frame->show();
		}
		first_image_loaded = true;
		qApp->processEvents();
	}
}

void MainWindow::load_dicom_series2()
{
	if (lock0) return;
	lock0 = true;
	const bool dcm_thread = settingswidget->get_dcm_thread();
	const bool selection =
		browser2->tableWidget->selectionModel()->hasSelection();
	if (!selection)
	{
		lock0 = false;
		return;
	}
	set_ui();
	QProgressDialog * pb =
		new QProgressDialog(QString("Loading ..."), QString("Exit"), 0, 0);
	{
		QList<QPushButton *> lb = pb->findChildren<QPushButton*>();
		for (int x = 0; x < lb.size(); ++x) // one button
		{
			if (dcm_thread)
			{
				lb[x]->setStyleSheet("QPushButton { color: #8B0000; }");
			}
			else
			{
				lb[x]->hide();
			}
		}
	}
	if (dcm_thread)
	{
		connect(pb, SIGNAL(canceled()), this, SLOT(exit_null()));
	}
	pb->setModal(true);
	pb->setWindowFlags(pb->windowFlags() ^ Qt::WindowContextHelpButtonHint);
	pb->setMinimumWidth(256);
	pb->setRange(0, 0);
	pb->setMinimumDuration(0);
	pb->setValue(0);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	pb->show();
#endif
	qApp->processEvents();
	const QString message = aliza->load_dicom_series(pb);
	if (dcm_thread)
	{
		disconnect(pb, SIGNAL(canceled()), this, SLOT(exit_null()));
	}
	pb->close();
	delete pb;
	if (!message.isEmpty())
	{
		InfoDialog info;
		info.set_text(message);
		info.exec();
	}
	lock0 = false;
}

QString MainWindow::load_any_file(
	const QString & f,
	QProgressDialog * pb)
{
	QFileInfo fi(f);
	if (!fi.isFile()) return QString("");
	set_ui();
	const QString message = aliza->load_dicom_file(f, pb);
	return message;
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
	QString message_;
	if (v->group_id < 0)
		message_ = aliza->create_group_(&ok, true);
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
	else
	{
		QMessageBox::information(nullptr, QString("Warning"), message_);
	}
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
	graphicswidget_m->graphicsview->zoom_in();
}

void MainWindow::zoom_minus_2d()
{
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

void MainWindow::toggle_update_contours_width(double x)
{
	graphicswidget_m->set_contours_width(x);
#if 0
	if (glwidget &&
		glwidget->opengl_init_done &&
		!glwidget->no_opengl3)
	{
		glwidget->set_contours_width(static_cast<float>(x));
	}
#endif
}

void MainWindow::trigger_set_level()
{
	QMessageBox::information(
		nullptr,
		QString("Set Level/Window"),
		QString(
			"Use sliders/spinboxes or histogram widget.\n"
			"Toggle \"Lock\" button to switch in 2D view\n"
			"for original slice (Z) between defined\n"
			"per-slicer level and and free selection."));
}

void MainWindow::tab_ind_changed(int i)
{
	if (multiview_tab)
	{
		switch (i)
		{
		case 0:
			file_menu->menuAction()->setVisible(true);
			settings_menu->menuAction()->setVisible(false);
			metadata_menu->menuAction()->setVisible(false);
			deidentify_menu->menuAction()->setVisible(false);
			browser_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(true);
			tools_menu->menuAction()->setVisible(true);
			multiview_menu->menuAction()->setVisible(false);
			break;
		case 1:
			file_menu->menuAction()->setVisible(true);
			multiview_menu->menuAction()->setVisible(true);
			settings_menu->menuAction()->setVisible(false);
			metadata_menu->menuAction()->setVisible(false);
			deidentify_menu->menuAction()->setVisible(false);
			browser_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(false);
			tools_menu->menuAction()->setVisible(false);
			break;
		case 2:
			browser_menu->menuAction()->setVisible(true);
			settings_menu->menuAction()->setVisible(false);
			metadata_menu->menuAction()->setVisible(false);
			deidentify_menu->menuAction()->setVisible(false);
			tools_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(false);
			file_menu->menuAction()->setVisible(true);
			multiview_menu->menuAction()->setVisible(false);
			if (browser2->is_first_run()) browser2->reload_dir();
			break;
		case 3:
			file_menu->menuAction()->setVisible(true);
			metadata_menu->menuAction()->setVisible(true);
			deidentify_menu->menuAction()->setVisible(false);
			settings_menu->menuAction()->setVisible(false);
			browser_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(false);
			tools_menu->menuAction()->setVisible(false);
			multiview_menu->menuAction()->setVisible(false);
			break;
		case 4:
			file_menu->menuAction()->setVisible(true);
			settings_menu->menuAction()->setVisible(false);
			metadata_menu->menuAction()->setVisible(false);
			deidentify_menu->menuAction()->setVisible(true);
			browser_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(false);
			tools_menu->menuAction()->setVisible(false);
			multiview_menu->menuAction()->setVisible(false);
			break;
		case 5:
			file_menu->menuAction()->setVisible(true);
			settings_menu->menuAction()->setVisible(true);
			metadata_menu->menuAction()->setVisible(false);
			deidentify_menu->menuAction()->setVisible(false);
			browser_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(false);
			tools_menu->menuAction()->setVisible(false);
			multiview_menu->menuAction()->setVisible(false);
			break;
		default:
			file_menu->menuAction()->setVisible(true);
			settings_menu->menuAction()->setVisible(false);
			metadata_menu->menuAction()->setVisible(false);
			deidentify_menu->menuAction()->setVisible(false);
			browser_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(false);
			tools_menu->menuAction()->setVisible(false);
			multiview_menu->menuAction()->setVisible(false);
			break;
		}
	}
	else
	{
		switch (i)
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
			file_menu->menuAction()->setVisible(true);
			browser_menu->menuAction()->setVisible(true);
			settings_menu->menuAction()->setVisible(false);
			metadata_menu->menuAction()->setVisible(false);
			deidentify_menu->menuAction()->setVisible(false);
			tools_menu->menuAction()->setVisible(false);
			views_menu->menuAction()->setVisible(false);
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
	}
	qApp->processEvents();
}

void MainWindow::set_zlock(bool t)
{
	if (t) zlockAct->setIcon(anchor_icon);
	else   zlockAct->setIcon(anchor2_icon);
	oneAct->setEnabled(t);
	aliza->toggle_zlock(t);
	qApp->processEvents();
}

void MainWindow::set_zlock_one(bool t)
{
	aliza->toggle_zlock_one(t);
	qApp->processEvents();
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
	if (!isMaximized() && !isFullScreen())
	{
		settings.setValue(QString("size"), QVariant(size()));
		settings.setValue(QString("pos"), QVariant(pos()));
	}
	settings.setValue(QString("dock_area"), QVariant(area));
#ifdef USE_WORKSTATION_MODE
	settings.setValue(QString("open_dir"),QVariant(CommonUtils::get_open_dir()));
#endif
	settings.setValue(
		QString("hide_3d_frame"),
		((show3DAct->isChecked()) ? QVariant(QString("N")) : QVariant(QString("Y"))));
	settings.endGroup();
	if (browser2) browser2->writeSettings(settings);
	if (anonymizer) anonymizer->writeSettings(settings);
	if (settingswidget) settingswidget->writeSettings(settings);
	if (studyview) studyview->writeSettings(settings);
}

void MainWindow::readSettings()
{
	int width_ = 0, height_ = 0;
	desktop_layout(&width_, &height_);
	const int w = static_cast<int>(static_cast<double>(width_) * 0.7);
	const int h = static_cast<int>(static_cast<double>(height_) * 0.7);
	QSettings settings(
		QSettings::IniFormat, QSettings::UserScope,
		QApplication::organizationName(), QApplication::applicationName());
	settings.setFallbacksEnabled(false);
	settings.beginGroup(QString("MainWindow"));
	resize(settings.value(QString("size"), QSize(w, h)).toSize());
	move(settings.value(QString("pos"), QPoint(50, 50)).toPoint());
#ifdef USE_WORKSTATION_MODE
	CommonUtils::set_open_dir(settings.value(QString("open_dir"), QString("")).toString());
#endif
	settings.endGroup();
}

void MainWindow::set_style(const QString & s)
{
	change_style(s.trimmed());
	qApp->processEvents();
}

void MainWindow::change_style(const QString & s)
{
	if (s.isEmpty()) return;
	if (s == QString("Dark Fusion"))
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
		//
		imagesbox->update_background_color(true);
	}
	else
	{
		QApplication::setStyle(s);
#if 0
		if (!((s.toUpper() == QString("WINDOWSVISTA")) ||
			(s.toUpper() == QString("MACOS"))))
#endif
		{
			QApplication::setPalette(
				QApplication::style()->standardPalette());
		}
		//
		imagesbox->update_background_color(false);
	}
	if (glwidget) glwidget->update_clear_color();
	graphicswidget_m->update_background_color();
	graphicswidget_y->update_background_color();
	graphicswidget_x->update_background_color();
	toolbox2D->set_style_sheet();
	slider_m->set_style_sheet();
	slider_y->set_style_sheet();
	slider_x->set_style_sheet();
	histogramview->update_bgcolor();
	studyview->update_background_color();
	update_info_lines_bg();
	update();
}

void MainWindow::check_3d_frame()
{
	QSettings settings(
		QSettings::IniFormat,
		QSettings::UserScope,
		QApplication::organizationName(),
		QApplication::applicationName());
	settings.setFallbacksEnabled(true);
	settings.beginGroup(QString("MainWindow"));
	const QString s = settings.value(QString("hide_3d_frame"), QString("N")).toString();
	settings.endGroup();
	if (s == QString("Y"))
	{
		show3DAct->blockSignals(true);
		toggle_showgl(false);
		show3DAct->setChecked(false);
		show3DAct->blockSignals(false);
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
			nullptr,
			QString("Information"),
			QString("Not a DICOM image or empty list"));
		return;
	}
	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
	qApp->processEvents();
	sqtree->set_list_of_files(l, 0, true);
	sqtree->read_file(l.at(0), true);
	const int t = (multiview_tab) ? 3 : 2;
	tabWidget->setCurrentIndex(t);
	qApp->restoreOverrideCursor();
}

void MainWindow::set_image_view()
{
	if (tabWidget->currentIndex() != 0) tabWidget->setCurrentIndex(0);
	qApp->processEvents();
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
	r = QMessageBox::question(nullptr, QString("Close Application"), QString("Close application?"));
	if (r == QMessageBox::Yes || r == QMessageBox::Ok)
	{
		this->close();
	}
}

void MainWindow::trigger_studyview()
{
	aliza->trigger_studyview();
	if (multiview_tab)
	{
		if (tabWidget->currentIndex() != 1) tabWidget->setCurrentIndex(1);
	}
	qApp->processEvents();
}

void MainWindow::trigger_studyview_checked()
{
	aliza->trigger_studyview_checked();
	if (multiview_tab)
	{
		if (tabWidget->currentIndex() != 1) tabWidget->setCurrentIndex(1);
	}
	qApp->processEvents();
}

void MainWindow::trigger_studyview_empty()
{
	aliza->trigger_studyview_empty();
	if (multiview_tab)
	{
		if (tabWidget->currentIndex() != 1) tabWidget->setCurrentIndex(1);
	}
	qApp->processEvents();
}

void MainWindow::trigger_default_settings()
{
	settingswidget->set_default();
}

