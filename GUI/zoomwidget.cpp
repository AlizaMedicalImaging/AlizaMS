#include "zoomwidget.h"
#include <QHBoxLayout>

ZoomWidget::ZoomWidget(float si)
{
	const QSize s = QSize(
		static_cast<int>(18 * si), static_cast<int>(18 * si));
	QHBoxLayout * l  = new QHBoxLayout(this);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	plus_pushButton =
		new QPushButton(QIcon(QString(":/bitmaps/zoomin.svg")),
		QString(""),
		this);
	plus_pushButton->setIconSize(s);
	plus_pushButton->setAutoRepeat(true);
	minus_pushButton =
		new QPushButton(QIcon(QString(":/bitmaps/zoomout.svg")),
		QString(""),
		this);
	minus_pushButton->setIconSize(s);
	minus_pushButton->setAutoRepeat(true);
	l->addWidget(plus_pushButton);
	l->addWidget(minus_pushButton);
}

