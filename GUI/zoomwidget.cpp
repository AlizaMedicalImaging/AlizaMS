#include "zoomwidget.h"
#include <QHBoxLayout>

ZoomWidget::ZoomWidget(float si)
{
	const QSize s = QSize((int)(18*si),(int)(18*si));
	QHBoxLayout * l  = new QHBoxLayout(this);
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(0);
	plus_pushButton =
		new QPushButton(QIcon(QString(":/bitmaps/add.svg")),
		QString(""),
		this);
	plus_pushButton->setIconSize(s);
	plus_pushButton->setAutoRepeat(true);
	minus_pushButton =
		new QPushButton(QIcon(QString(":/bitmaps/remove.svg")),
		QString(""),
		this);
	minus_pushButton->setIconSize(s);
	minus_pushButton->setAutoRepeat(true);
	l->addWidget(plus_pushButton);
	l->addWidget(minus_pushButton);
}

ZoomWidget::~ZoomWidget()
{
}
