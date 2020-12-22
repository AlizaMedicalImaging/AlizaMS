#ifndef ZRANGEWIDGET_H
#define ZRANGEWIDGET_H

#include "qxtspanslider.h"

class ZRangeWidget: public QWidget
{
	Q_OBJECT
public:
	ZRangeWidget(QWidget(*)=NULL);
	~ZRangeWidget();
	QxtSpanSlider * spanslider;
public slots:
	void set_spanslider_max(int);
	void set_from(int);
	void set_to(int);
	void set_span(int, int);
};

#endif // ZRANGEWIDGET_H
