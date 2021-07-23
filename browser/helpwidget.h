#ifndef HELPWIDGET___H
#define HELPWIDGET___H

#include "ui_helpwidget.h"
#include <QWidget>

class HelpWidget: public QWidget, private Ui::HelpWidget
{
Q_OBJECT
public:
	HelpWidget();
	~HelpWidget();
};

#endif // HELPWIDGET___H
