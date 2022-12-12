#ifndef A_ANIMWIDGET_H
#define A_ANIMWIDGET_H

#include "ui_animwidget.h"

class AnimWidget: public QWidget, public Ui::AnimWidget
{
Q_OBJECT
public:
	AnimWidget(float);
	~AnimWidget();
};

#endif
