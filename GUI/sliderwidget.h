#ifndef SLIDERWIDGET_H
#define SLIDERWIDGET_H

#include "ui_sliderwidget.h"

class SliderWidget: public QWidget, public Ui::SliderWidget
{
	Q_OBJECT
public:
	SliderWidget(QWidget(*)=NULL, Qt::WindowFlags=0);
	~SliderWidget();
	void set_style_sheet();
public slots:
	void set_slice(int);
	void set_slider_max(int);
private:
};

#endif // SLIDERWIDGET_H
