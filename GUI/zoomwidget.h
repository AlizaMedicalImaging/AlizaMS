#ifndef ZoomWidget__H
#define ZoomWidget__H

#include <QWidget>
#include <QPushButton>

class ZoomWidget : public QWidget
{
Q_OBJECT
public:
	ZoomWidget(float, QWidget(*)=NULL, Qt::WindowFlags=0);
	~ZoomWidget();
	QPushButton * plus_pushButton;
	QPushButton * minus_pushButton;
};

#endif // ZoomWidget__H
