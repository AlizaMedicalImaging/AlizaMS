#include "zrangewidget.h"

#include <QHBoxLayout>

ZRangeWidget::ZRangeWidget()
{
	spanslider  = new QxtSpanSlider(Qt::Horizontal, this);
	QHBoxLayout * l = new QHBoxLayout(this);
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(0);
	l->addWidget(spanslider);
}

ZRangeWidget::~ZRangeWidget()
{
}

void ZRangeWidget::set_spanslider_max(int i)
{
	spanslider->setMaximum(i);
}

void ZRangeWidget::set_from(int i)
{
	spanslider->setLowerValue(i);
}

void ZRangeWidget::set_to(int i)
{
	spanslider->setUpperValue(i);
}

void ZRangeWidget::set_span(int l, int h)
{
	spanslider->setSpan(l,h);
}

