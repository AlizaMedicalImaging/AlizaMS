#include "zoomwidget.h"
#include <QHBoxLayout>

ZoomWidget::ZoomWidget(QWidget * p, Qt::WindowFlags f) : QWidget(p,f)
{
	QHBoxLayout * l  = new QHBoxLayout(this);
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(0);
	plus_pushButton  = new QPushButton(QIcon(":/bitmaps/add.svg"),QString(""), this);
	plus_pushButton->setAutoRepeat(true);
	minus_pushButton = new QPushButton(QIcon(":/bitmaps/remove.svg"),QString(""), this);
	minus_pushButton->setAutoRepeat(true);
	l->addWidget(plus_pushButton);
	l->addWidget(minus_pushButton);
}

ZoomWidget::~ZoomWidget()
{
}
