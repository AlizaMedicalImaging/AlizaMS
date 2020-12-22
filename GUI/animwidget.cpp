#include "animwidget.h"

AnimWidget::AnimWidget(float si, QWidget * p) : QWidget(p)
{
	setupUi(this);
	const QSize s = QSize((int)(18*si),(int)(18*si));
	start_pushButton->setIconSize(s);
	stop_pushButton->setIconSize(s);
	group_pushButton->setIconSize(s);
	remove_pushButton->setIconSize(s);
	stop_pushButton->setEnabled(false);
	acq_spinBox->hide();
	acq_spinBox->clear();
}

AnimWidget::~AnimWidget()
{
}
