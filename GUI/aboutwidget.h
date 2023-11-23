#ifndef A_ABOUTWIDGET_H
#define A_ABOUTWIDGET_H

#include "ui_aboutwidget.h"
#include <QMouseEvent>

class AboutWidget : public QWidget, public Ui::AboutWidget
{
Q_OBJECT
public:
	AboutWidget(int, int);
	~AboutWidget() = default;
	bool has_opengl_info() const;
	void set_opengl_info(const QString&);
	void set_info();

protected:
	void mouseReleaseEvent(QMouseEvent*) override;

private:
	QString get_build_info();
	QString opengl_info;
};

#endif

