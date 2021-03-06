#ifndef SLIDERWIDGET_H
#define SLIDERWIDGET_H

#include "ui_sliderwidget.h"

class SliderWidget: public QWidget, public Ui::SliderWidget
{
Q_OBJECT
public:
	SliderWidget();
	~SliderWidget();
	void set_style_sheet();

public slots:
	void set_slice(int);
	void set_slider_max(int);
};

#endif // SLIDERWIDGET_H
