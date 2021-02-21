#ifndef ABOUTWIDGET_H__
#define ABOUTWIDGET_H__

#include "ui_aboutwidget.h"
#include <QString>
#include <QMouseEvent>

class AboutWidget : public QWidget, public Ui::AboutWidget
{
Q_OBJECT
public:
	AboutWidget(int, int);
	~AboutWidget();
	bool has_opengl_info() const;
	void set_opengl_info(const QString&);
	void set_info();

protected:
	void mouseReleaseEvent(QMouseEvent*) override;

private:
	QString get_build_info();
	QString opengl_info;
};

#endif // ABOUTWIDGET_H__
