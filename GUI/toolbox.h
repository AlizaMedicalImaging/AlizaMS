#ifndef TOOLBOX_H
#define TOOLBOX_H

#include "ui_toolbox.h"

class ToolBox: public QWidget, public Ui::ToolBox
{
	Q_OBJECT
public:
	ToolBox(QWidget(*)=NULL);
	~ToolBox();
};

#endif // TOOLBOX_H
