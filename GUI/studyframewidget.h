#ifndef STUDYFRAMEWIDGET__H
#define STUDYFRAMEWIDGET__H

#include "ui_studyframewidget.h"
#include <QWidget>

class StudyGraphicsWidget;

class StudyFrameWidget : public QWidget, public Ui::StudyFrameWidget
{

Q_OBJECT

public:
	StudyFrameWidget(StudyGraphicsWidget*);
	~StudyFrameWidget();
	StudyGraphicsWidget * graphicswidget;
};

#endif // STUDYFRAMEWIDGET__H
