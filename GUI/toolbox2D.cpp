#include "toolbox2D.h"

ToolBox2D::ToolBox2D(float si)
{
	setupUi(this);
	const QSize s1 = QSize(static_cast<int>(18*si),static_cast<int>(18*si));
	const QSize s2 = QSize(static_cast<int>(36*si),static_cast<int>(18*si));
	resetlevel_pushButton->setIconSize(s1);
	maxwin_pushButton->setIconSize(s1);
	lock_pushButton->setIconSize(s2);
	set_style_sheet();
	red   = QPixmap(QString(":/bitmaps/red.png"));
	green = QPixmap(QString(":/bitmaps/green.png"));
	blue  = QPixmap(QString(":/bitmaps/blue.png"));
	lockon = QIcon(QString(":/bitmaps/lock2.svg"));
	lockoff = QIcon(QString(":/bitmaps/unlock2.svg"));
	anim_label->hide();
	anim_label->setPixmap(green);
	label_is_red   = false;
	label_is_green = false;
	label_is_blue  = false;
	connect_sliders();
}

ToolBox2D::~ToolBox2D()
{
}

void ToolBox2D::set_style_sheet()
{
	const QString tmp0 = QString("QWidget{selection-background-color: #303947;}");
	center_horizontalSlider->setStyleSheet(tmp0);
	width_horizontalSlider->setStyleSheet(tmp0);
}

void ToolBox2D::set_window_upper(double i)
{
	center_doubleSpinBox->setMaximum(i);
	center_horizontalSlider->setMaximum(static_cast<int>(i));
}

void ToolBox2D::set_window_lower(double i)
{
	center_doubleSpinBox->setMinimum(i);
	center_horizontalSlider->setMinimum(static_cast<int>(i));
}

void ToolBox2D::set_max_width(double i)
{
	width_doubleSpinBox->setMinimum(0.000001);
	width_horizontalSlider->setMinimum(0);
	width_doubleSpinBox->setMaximum(i);
	width_horizontalSlider->setMaximum(static_cast<int>(i));
}

void ToolBox2D::set_width(double i)
{ 
	width_doubleSpinBox->setValue(i);
}

void ToolBox2D::set_center(double i)
{ 
	center_doubleSpinBox->setValue(i);
}

void ToolBox2D::set_indicator_red()
{
	label_is_red = true;
	label_is_blue = label_is_green = false;
	anim_label->setPixmap(red);
}

void ToolBox2D::set_indicator_green()
{
	label_is_green = true;
	label_is_blue = label_is_red = false;
	anim_label->setPixmap(green);
}

void ToolBox2D::set_indicator_blue()
{
	label_is_blue = true;
	label_is_green = label_is_red = false;
	anim_label->setPixmap(blue);
}

bool ToolBox2D::is_red() const
{
	return label_is_red;
}

bool ToolBox2D::is_green() const
{
	return label_is_green;
}

bool ToolBox2D::is_blue() const
{
	return label_is_blue;
}

void ToolBox2D::set_lut_function(int x)
{
	comboBox->setCurrentIndex(x);
}

void ToolBox2D::update_slider_center(double i)
{
	center_horizontalSlider->setValue(static_cast<int>(i));
}

void ToolBox2D::update_slider_width(double i)
{
	width_horizontalSlider->setValue(static_cast<int>(i));
}

void ToolBox2D::update_spinbox_center(int i)
{
	center_doubleSpinBox->setValue(static_cast<double>(i));
}

void ToolBox2D::update_spinbox_width(int i)
{
	width_doubleSpinBox->setValue(static_cast<double>(i));
}

void ToolBox2D::connect_sliders()
{
	connect(center_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(update_spinbox_center(int)));
	connect(width_horizontalSlider, SIGNAL(valueChanged(int)),this,SLOT(update_spinbox_width(int)));
}

void ToolBox2D::disconnect_sliders()
{
	disconnect(center_horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(update_spinbox_center(int)));
	disconnect(width_horizontalSlider, SIGNAL(valueChanged(int)),this,SLOT(update_spinbox_width(int)));
}

void ToolBox2D::set_maxwindow(bool i)
{
	maxwin_pushButton->setChecked(i);
}

void ToolBox2D::enable_maxwindow(bool i)
{
	maxwin_pushButton->setEnabled(i);
}

bool ToolBox2D::get_locked() const
{
	return lock_pushButton->isChecked();
}

void ToolBox2D::toggle_locked_values(bool t)
{
	lock_pushButton->setChecked(t);
	center2d_doubleSpinBox->setEnabled(t);
	width2_doubleSpinBox->setEnabled(t);
	if (t)
	{
		lock_pushButton->setIcon(lockon);
	}
	else
	{
		lock_pushButton->setIcon(lockoff);
	}
}

void ToolBox2D::set_locked_center(double x)
{
	center2d_doubleSpinBox->setValue(x);
}

void ToolBox2D::set_locked_width(double x)
{
	width2_doubleSpinBox->setValue(x);
}
