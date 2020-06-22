#include "animwidget.h"

AnimWidget::AnimWidget(QWidget * p, Qt::WindowFlags f) : QWidget(p, f)
{
	setupUi(this);
	stop_pushButton->setEnabled(false);
	acq_spinBox->hide();
	acq_spinBox->clear();
}

AnimWidget::~AnimWidget()
{
}
