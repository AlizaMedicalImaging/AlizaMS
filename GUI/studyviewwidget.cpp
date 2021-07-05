#include "structures.h"
#include "matrixbutton.h"
#include "lutwidget.h"
#include "studyframewidget.h"
#include "studygraphicswidget.h"
#include "studyviewwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QVariant>
#if MATRIX_BUTTON_CUSTOM_ACT == 1
#include "tabledialog.h"
#endif
#if 0
#include <iostream>
#endif

StudyViewWidget::StudyViewWidget(float si)
{
	setupUi(this);
	//
	const int widgets_size = 25;
	//
	const QSize s1 = QSize((int)(18*si),(int)(18*si));
	lockon = QIcon(QString(":/bitmaps/lock.svg"));
	lockoff = QIcon(QString(":/bitmaps/unlock.svg"));
	resetlevel_pushButton->setIconSize(s1);
	lock_pushButton->setIconSize(s1);
	active_id = -1;
	mbutton = new MatrixButton(si);
	fitall_toolButton = new QToolButton(this);
	fitall_toolButton->setIconSize(s1);
	fitall_toolButton->setIcon(QIcon(QString(":/bitmaps/f2.svg")));
	fitall_toolButton->setToolTip(QString("Fit to view"));
	scouts_toolButton = new QToolButton(this);
	scouts_toolButton->setCheckable(true);
	scouts_toolButton->setChecked(true);
	scouts_toolButton->setIconSize(s1);
	scouts_toolButton->setIcon(QIcon(QString(":/bitmaps/collisions.svg")));
	scouts_toolButton->setToolTip(QString("Show intersections"));
	QWidget * spacer1 = new QWidget(this);
	spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	horizontal = true;
	QHBoxLayout * l1 = new QHBoxLayout(toolbar_frame);
	l1->setContentsMargins(0,0,0,0);
	l1->setSpacing(4);
	l1->addWidget(mbutton);
	l1->addWidget(fitall_toolButton);
	l1->addWidget(scouts_toolButton);
	l1->addWidget(spacer1);
	lutwidget  = new LUTWidget(si);
	lutwidget->add_items1();
	QVBoxLayout * l2 = new QVBoxLayout(lut_frame);
	l2->setContentsMargins(0,0,0,0);
	l2->setSpacing(0);
	l2->addWidget(lutwidget);
	QGridLayout * gridLayout = new QGridLayout(frame);
	for (int x = 0; x < widgets_size; ++x)
	{
		StudyGraphicsWidget * w = new StudyGraphicsWidget();
		StudyFrameWidget * f = new StudyFrameWidget(w);
		w->set_studyview(this);
		w->set_slider(f->slider);
		w->set_top_label(f->top_label);
		w->set_left_label(f->left_label);
		w->set_icon_label(f->icon_label);
		f->hide();
		widgets.push_back(f);
	}
	update_null();
	connect(
		mbutton, SIGNAL(matrix_selected(int, int)),
		this, SLOT(update_grid(int, int)));
	connect(
		fitall_toolButton, SIGNAL(clicked()),
		this, SLOT(all_to_fit()));
	connect(
		scouts_toolButton, SIGNAL(toggled(bool)),
		this, SLOT(toggle_scouts(bool)));
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	connect(
		mbutton->p_action, SIGNAL(triggered()),
		this, SLOT(update_grid2()));
#endif
	connect_tools();
#ifdef __APPLE__
	minimaze_sc = new QShortcut(QKeySequence("Ctrl+M"), this, SLOT(showMinimized()));
	minimaze_sc->setAutoRepeat(false);
	fullsceen_sc = new QShortcut(QKeySequence("Ctrl+Meta+F"),this,SLOT(showFullScreen()));
	fullsceen_sc->setAutoRepeat(false);
	normal_sc = new QShortcut(QKeySequence("Esc"),this,SLOT(showNormal()));
	normal_sc->setAutoRepeat(false);
#endif
}

StudyViewWidget::~StudyViewWidget()
{
	for (int x = 0; x < widgets.size(); ++x)
	{
		if (widgets.at(x))
		{
			if (widgets[x]->graphicswidget)
			{
				widgets[x]->graphicswidget->clear_();
			}
			delete widgets[x];
			widgets[x] = NULL;
		}
	}
	widgets.clear();
}

void StudyViewWidget::clear_()
{
	active_id = -1;
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->frame0->frameShape() != QFrame::StyledPanel)
				widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
		}
	}
	update_null();
    QGridLayout * layout = static_cast<QGridLayout*>(frame->layout());
	if (!layout) return;
	const int r = layout->rowCount();
	const int c = layout->columnCount();
	for (int x = 0; x < r; ++x)
	{
		for (int y = 0; y < c; ++y)
		{
			QLayoutItem * li = layout->itemAtPosition(x, y);
			if (li)
			{
				QWidget * w = li->widget();
				if (w)
				{
					layout->removeWidget(w);
					StudyFrameWidget * f = static_cast<StudyFrameWidget*>(w);
					f->graphicswidget->clear_();
					w->hide();
				}
			}
		}
	}
	delete layout;
	layout = new QGridLayout(frame);
}

void StudyViewWidget::set_horizontal(bool h)
{
	horizontal = h;
}

void StudyViewWidget::calculate_grid(int x)
{
	int r = 1;
	int c = 1;
	switch(x)
	{
	case 1:
		{
			r = 1;
			c = 1;
		}
		break;
	case 2:
		{
			if (horizontal)
			{
				r = 1;
				c = 2;
			}
			else
			{
				r = 2;
				c = 1;
			}
		}
		break;
	case 3:
		{
			if (horizontal)
			{
				r = 1;
				c = 3;
			}
			else
			{
				r = 3;
				c = 1;
			}
		}
		break;
	case 4:
		{
			r = 2;
			c = 2;
		}
		break;
	case 5:
	case 6:
		{
			if (horizontal)
			{
				r = 2;
				c = 3;
			}
			else
			{
				r = 3;
				c = 2;
			}
		}
		break;
	case 7:
	case 8:
		{
			if (horizontal)
			{
				r = 2;
				c = 4;
			}
			else
			{
				r = 4;
				c = 2;
			}
		}
		break;
	case 9:
		{
			r = 3;
			c = 3;
		}
		break;
	case 10:
		{
			if (horizontal)
			{
				r = 2;
				c = 5;
			}
			else
			{
				r = 5;
				c = 2;
			}
		}
		break;
	case 11:
	case 12:
		{
			if (horizontal)
			{
				r = 3;
				c = 4;
			}
			else
			{
				r = 4;
				c = 3;
			}
		}
		break;
	case 13:
	case 14:
	case 15:
	case 16:
		{
			r = 4;
			c = 4;
		}
		break;
	case 17:
	case 18:
	case 19:
	case 20:
		{
			if (horizontal)
			{
				r = 4;
				c = 5;
			}
			else
			{
				r = 5;
				c = 4;
			}
		}
		break;
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	default:
		{
			r = 5;
			c = 5;
		}
		break;
	}
	update_grid(r, c);
}

void StudyViewWidget::connect_tools()
{
	connect(center_horizontalSlider,SIGNAL(valueChanged(int)),       this,SLOT(set_center_slider(int)));
	connect(width_horizontalSlider, SIGNAL(valueChanged(int)),       this,SLOT(set_width_slider(int)));
	connect(center_doubleSpinBox,   SIGNAL(valueChanged(double)),    this,SLOT(set_center_spinbox(double)));
	connect(width_doubleSpinBox,    SIGNAL(valueChanged(double)),    this,SLOT(set_width_spinbox(double)));
	connect(comboBox,               SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut_function(int)));
	connect(lutwidget->comboBox,    SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut(int)));
	connect(lock_pushButton,        SIGNAL(toggled(bool)),           this,SLOT(toggle_lock_window(bool)));
	connect(resetlevel_pushButton,  SIGNAL(clicked()),               this,SLOT(reset_level()));
}

void StudyViewWidget::disconnect_tools()
{
	disconnect(center_horizontalSlider,SIGNAL(valueChanged(int)),       this,SLOT(set_center_slider(int)));
	disconnect(width_horizontalSlider, SIGNAL(valueChanged(int)),       this,SLOT(set_width_slider(int)));
	disconnect(center_doubleSpinBox,   SIGNAL(valueChanged(double)),    this,SLOT(set_center_spinbox(double)));
	disconnect(width_doubleSpinBox,    SIGNAL(valueChanged(double)),    this,SLOT(set_width_spinbox(double)));
	disconnect(comboBox,               SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut_function(int)));
	disconnect(lutwidget->comboBox,    SIGNAL(currentIndexChanged(int)),this,SLOT(set_lut(int)));
	disconnect(lock_pushButton,        SIGNAL(toggled(bool)),           this,SLOT(toggle_lock_window(bool)));
	disconnect(resetlevel_pushButton,  SIGNAL(clicked()),               this,SLOT(reset_level()));
}

void StudyViewWidget::block_signals(bool t)
{
	center_horizontalSlider->blockSignals(t);
	width_horizontalSlider->blockSignals(t);
	center_doubleSpinBox->blockSignals(t);
	width_doubleSpinBox->blockSignals(t);
	comboBox->blockSignals(t);
	lutwidget->comboBox->blockSignals(t);
	lock_pushButton->blockSignals(t);
	resetlevel_pushButton->blockSignals(t);
}

void StudyViewWidget::closeEvent(QCloseEvent * e)
{
	clear_();
	update_null();
	e->accept();
}

void StudyViewWidget::update_grid(int r, int c)
{
    QGridLayout * layout = static_cast<QGridLayout*>(frame->layout());
	if (layout)
	{
		const int ar = layout->rowCount();
		const int ac = layout->columnCount();
		for (int x = 0; x < ar; ++x)
		{
			for (int y = 0; y < ac; ++y)
			{
				QLayoutItem * li = layout->itemAtPosition(x, y);
				if (li)
				{
					QWidget * w = li->widget();
					if (w)
					{
						layout->removeWidget(w);
						w->hide();
					}
				}
			}
		}
		delete layout;
		layout = NULL;
	}
	QGridLayout * gridLayout = new QGridLayout(frame);
	int j = 0;
	for (int x = 0; x < r; ++x)
	{
		for (int y = 0; y < c; ++y)
		{
			gridLayout->addWidget(widgets.at(j), x, y);
			widgets[j]->show();
			widgets[j]->graphicswidget->show();
			++j;
		}
	}
	j = 0;
	for (int x = 0; x < r; ++x)
	{
		for (int y = 0; y < c; ++y)
		{
			widgets[j]->graphicswidget->update_image(1, true);
			++j;
		}
	}
	qApp->processEvents();
}

void StudyViewWidget::update_grid2()
{
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	bool ok = false;
	int r = 0, c = 0;
	TableDialog * d = new TableDialog();
	if (d->exec() == QDialog::Accepted)
	{
		ok = true;
		r = d->get_rows();
		c = d->get_columns();
	}
	delete d;
	if (ok) update_grid(r, c);
	qApp->processEvents();
#endif
}

int StudyViewWidget::get_active_id() const
{
	return active_id;
}

void StudyViewWidget::set_active_id(int x)
{
	active_id = x;
}

bool StudyViewWidget::get_scouts() const
{
	return scouts_toolButton->isChecked();
}

void StudyViewWidget::set_active_image(ImageContainer * c)
{
	if (!(c && c->image3D))
	{
		active_id = -1;
		for (int i = 0; i < widgets.size(); ++i)
		{
			if (widgets.at(i))
			{
				if (widgets.at(i)->frame0->frameShape() != QFrame::StyledPanel)
				{
					widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
				}
			}
		}
		update_null();
		return;
	}
	active_id = c->image3D->id;
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->frame0->setFrameShape(QFrame::Box);
					}
					else
					{
						widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
					}
				}
				else
				{
					widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
				}
			}
			else
			{
				widgets[i]->frame0->setFrameShape(QFrame::StyledPanel);
			}
		}
	}
	level_frame->setEnabled(true);
	update_full(c);
}

void StudyViewWidget::update_full(ImageContainer * c)
{
	block_signals(true);
	const ImageVariant * v = c->image3D; // checked NULL before
	update_max_width(v->di->rmax-v->di->rmin);
	update_window_upper(v->di->rmax);
	update_window_lower(v->di->rmin);
	update_locked_window(c->level_locked_ext);
	if (v->image_type >= 0 && v->image_type < 10)
	{
		if (v->frame_levels.contains(c->selected_z_slice_ext))
		{
			const FrameLevel & fl = v->frame_levels.value(c->selected_z_slice_ext);
			update_locked_center(fl.us_window_center);
			update_locked_width(fl.us_window_width);
		}
		else
		{
			update_locked_center(v->di->default_us_window_center);
			update_locked_width(v->di->default_us_window_width);
		}
	}
	else
	{
		update_locked_center(0.0);
		update_locked_width(0.0);
	}
	update_spinbox_width_d(c->us_window_width_ext);
	update_spinbox_center_d(c->us_window_center_ext);
	if (v->di->disable_int_level)
	{
		center_horizontalSlider->setEnabled(false);
		width_horizontalSlider->setEnabled(false);
		center_horizontalSlider->setValue(0);
		width_horizontalSlider->setValue(0);
	}
	else
	{
		center_horizontalSlider->setEnabled(true);
		width_horizontalSlider->setEnabled(true);
		center_horizontalSlider->setValue(
			static_cast<int>(c->us_window_center_ext));
		width_horizontalSlider->setValue(
			static_cast<int>(c->us_window_width_ext));
	}
	if (c->lut_function_ext == 2) update_lut_function(1);
	else                          update_lut_function(0);
	lutwidget->set_lut(c->selected_lut_ext);
	block_signals(false);
}

void StudyViewWidget::update_level(ImageContainer * c)
{
	if (!c) return;
	if (!c->image3D) return;
	if (c->image3D->id != active_id) return;
	const ImageVariant * v = c->image3D;
	if (c->level_locked_ext)
	{
		if (v->image_type >= 0 && v->image_type < 10)
		{
			if (v->frame_levels.contains(c->selected_z_slice_ext))
			{
				const FrameLevel & fl = v->frame_levels.value(c->selected_z_slice_ext);
				update_locked_center(fl.us_window_center);
				update_locked_width(fl.us_window_width);
			}
			else
			{
				update_locked_center(v->di->default_us_window_center);
				update_locked_width(v->di->default_us_window_width);
			}
		}
		else
		{
			update_locked_center(0.0);
			update_locked_width(0.0);
		}
	}
#if 0
	else // not required for changed slice
	{
		center_horizontalSlider->blockSignals(true);
		width_horizontalSlider->blockSignals(true);
		center_doubleSpinBox->blockSignals(true);
		width_doubleSpinBox->blockSignals(true);
		update_spinbox_center_d(c->us_window_center_ext);
		update_spinbox_width_d(c->us_window_width_ext);
		center_horizontalSlider->setValue(static_cast<int>(c->us_window_center_ext));
		width_horizontalSlider->setValue(static_cast<int>(c->us_window_width_ext));
		center_horizontalSlider->blockSignals(false);
		width_horizontalSlider->blockSignals(false);
		center_doubleSpinBox->blockSignals(false);
		width_doubleSpinBox->blockSignals(false);
	}
#endif
}

////////////////////////////////////

void StudyViewWidget::set_center_slider(int x)
{
	center_doubleSpinBox->blockSignals(true);
	const double k = static_cast<double>(x);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->set_center(k);
					}
				}
			}
		}
	}
	center_doubleSpinBox->setValue(k);
	center_doubleSpinBox->blockSignals(false);
}

void StudyViewWidget::set_width_slider(int x)
{
	width_doubleSpinBox->blockSignals(true);
	const double k = static_cast<double>(x);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->set_width(k);
					}
				}
			}
		}
	}
	width_doubleSpinBox->setValue(k);
	width_doubleSpinBox->blockSignals(false);
}

void StudyViewWidget::set_center_spinbox(double x)
{
	center_horizontalSlider->blockSignals(true);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->set_center(x);
					}
				}
			}
		}
	}
	center_horizontalSlider->setValue(static_cast<int>(x));
	center_horizontalSlider->blockSignals(false);
}

void StudyViewWidget::set_width_spinbox(double x)
{
	width_horizontalSlider->blockSignals(true);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->set_width(x);
					}
				}
			}
		}
	}
	width_horizontalSlider->setValue(static_cast<int>(x));
	width_horizontalSlider->blockSignals(false);
}

void StudyViewWidget::set_lut_function(int x)
{
	const short k = (x == 0) ? 1 : 2;
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->set_lut_function(k);
					}
				}
			}
		}
	}
}

void StudyViewWidget::set_lut(int x)
{
	const short k = static_cast<short>(x);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->set_lut(k);
					}
				}
			}
		}
	}
}

void StudyViewWidget::toggle_lock_window(bool t)
{
	center2_doubleSpinBox->setEnabled(t);
	width2_doubleSpinBox->setEnabled(t);
	if (t) lock_pushButton->setIcon(lockon);
	else   lock_pushButton->setIcon(lockoff);
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->set_locked_window(t);
					}
				}
			}
		}
	}
	level1_frame->setEnabled(!t);
}

void StudyViewWidget::reset_level()
{
	block_signals(true);
	double c = 0, w = 1;
	short l = 0;
	bool once = false;
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					if (widgets.at(i)->graphicswidget->image_container.image3D->id == active_id)
					{
						widgets[i]->graphicswidget->reset_level();
						if (!once)
						{
							c = widgets.at(i)->graphicswidget->image_container.us_window_center_ext;
							w = widgets.at(i)->graphicswidget->image_container.us_window_width_ext;
							l = widgets.at(i)->graphicswidget->image_container.lut_function_ext;
							once = true;
						}
					}
				}
			}
		}
	}
	center_horizontalSlider->setValue(static_cast<int>(c));
	width_horizontalSlider->setValue(static_cast<int>(w));
	center_doubleSpinBox->setValue(c);
	width_doubleSpinBox->setValue(w);
	comboBox->setCurrentIndex((l == 2) ? 1 : 0);
	block_signals(false);
}

void StudyViewWidget::all_to_fit()
{
	for (int i = 0; i < widgets.size(); ++i)
	{
		if (widgets.at(i))
		{
			if (widgets.at(i)->graphicswidget)
			{
				if (widgets.at(i)->graphicswidget->image_container.image3D)
				{
					widgets[i]->graphicswidget->update_image(1, true);
				}
			}
		}
	}
}

void StudyViewWidget::all_to_original()
{
}

void StudyViewWidget::toggle_scouts(bool t)
{
	if (t)
	{
		emit update_scouts_required();
	}
	else
	{
		for (int i = 0; i < widgets.size(); ++i)
		{
			if (widgets.at(i) &&widgets.at(i)->graphicswidget)
			{
				widgets[i]->graphicswidget->graphicsview->clear_collision_paths();;
			}
		}
	}
}

////////////////////

void StudyViewWidget::update_locked_window(bool t)
{
	lock_pushButton->setChecked(t);
	center2_doubleSpinBox->setEnabled(t);
	width2_doubleSpinBox->setEnabled(t);
	if (t) lock_pushButton->setIcon(lockon);
	else   lock_pushButton->setIcon(lockoff);
	level1_frame->setEnabled(!t);
}

void StudyViewWidget::update_lut_function(int x)
{
	comboBox->setCurrentIndex(x);
}

void StudyViewWidget::update_spinbox_width_d(double i)
{ 
	width_doubleSpinBox->setValue(i);
}

void StudyViewWidget::update_spinbox_center_d(double i)
{ 
	center_doubleSpinBox->setValue(i);
}

void StudyViewWidget::update_window_upper(double i)
{
	center_doubleSpinBox->setMaximum(i);
	center_horizontalSlider->setMaximum(static_cast<int>(i));
}

void StudyViewWidget::update_window_lower(double i)
{
	center_doubleSpinBox->setMinimum(i);
	center_horizontalSlider->setMinimum(static_cast<int>(i));
}

void StudyViewWidget::update_max_width(double i)
{
	width_doubleSpinBox->setMinimum(0.000001);
	width_horizontalSlider->setMinimum(0);
	width_doubleSpinBox->setMaximum(i);
	width_horizontalSlider->setMaximum(static_cast<int>(i));
}

void StudyViewWidget::update_locked_center(double x)
{
	center2_doubleSpinBox->setValue(x);
}

void StudyViewWidget::update_locked_width(double x)
{
	width2_doubleSpinBox->setValue(x);
}

void StudyViewWidget::update_null()
{
	block_signals(true);
	level_frame->setEnabled(false);
	center_horizontalSlider->setValue(0);
	width_horizontalSlider->setValue(1);
	update_max_width(1);
	update_window_upper(1);
	update_window_lower(0);
	update_locked_center(0.0);
	update_locked_width(0.0);
	update_spinbox_width_d(1);
	update_spinbox_center_d(0);
	update_lut_function(0);
	lutwidget->set_lut(0);
	block_signals(false);
}

void StudyViewWidget::update_scouts()
{
	if (scouts_toolButton->isChecked())
	{
		emit update_scouts_required();
	}
}

void StudyViewWidget::readSettings()
{
}

void StudyViewWidget::writeSettings(QSettings & s)
{
}
