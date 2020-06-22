#ifndef ABOUTWIDGET_H__
#define ABOUTWIDGET_H__

#include "ui_aboutwidget.h"
#include <QDialog>
#include <QString>

class AboutWidget : public QDialog, public Ui::AboutWidget
{
Q_OBJECT
public:
	AboutWidget(QDialog(*)=NULL,Qt::WindowFlags=0);
	~AboutWidget();
	bool has_opengl_info() const;
	void set_opengl_info(const QString&);
	void set_info();
private:
	QString get_build_info();
	QString opengl_info;
};

#endif // ABOUTWIDGET_H__
