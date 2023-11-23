#ifndef A_ZRANGEWIDGET_H
#define A_ZRANGEWIDGET_H

#include "qxtspanslider.h"

class ZRangeWidget: public QWidget
{
Q_OBJECT
public:
	ZRangeWidget();
	~ZRangeWidget() = default;
	QxtSpanSlider * spanslider;

public slots:
	void set_spanslider_max(int);
	void set_from(int);
	void set_to(int);
	void set_span(int, int);
};

#endif

