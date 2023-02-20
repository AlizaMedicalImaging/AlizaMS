#include "studygraphicswidget.h"
#include "studyframewidget.h"
#include <QVBoxLayout>

StudyFrameWidget::StudyFrameWidget(StudyGraphicsWidget * g)
{
	setupUi(this);
	graphicswidget = g;
	QVBoxLayout * l = new QVBoxLayout(frame);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	l->addWidget(graphicswidget);
}

StudyFrameWidget::~StudyFrameWidget()
{
}

