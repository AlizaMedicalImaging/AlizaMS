#include "sliderwidget.h"

SliderWidget::SliderWidget()
{
	setupUi(this);
	set_style_sheet();
}

SliderWidget::~SliderWidget()
{
}

void SliderWidget::set_slice(int i)
{
	slices_slider->setValue(i);
}

void SliderWidget::set_slider_max(int i)
{
	slices_slider->setMaximum(i);
}

void SliderWidget::set_style_sheet()
{
	const QString tmp0 = QString("QWidget{selection-background-color: #303947;}");
	slices_slider->setStyleSheet(tmp0);
}
