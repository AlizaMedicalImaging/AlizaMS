#ifndef AnimWidget_H
#define AnimWidget_H

#include "ui_animwidget.h"

class AnimWidget: public QWidget, public Ui::AnimWidget
{
Q_OBJECT
public:
	AnimWidget(float);
	~AnimWidget();
};

#endif // AnimWidget_H
