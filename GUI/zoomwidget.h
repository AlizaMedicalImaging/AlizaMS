#ifndef A_ZOOMWIDGET_H
#define A_ZOOMWIDGET_H

#include <QWidget>
#include <QPushButton>

class ZoomWidget : public QWidget
{
Q_OBJECT
public:
	ZoomWidget(float);
	~ZoomWidget() = default;
	QPushButton * plus_pushButton;
	QPushButton * minus_pushButton;
};

#endif

