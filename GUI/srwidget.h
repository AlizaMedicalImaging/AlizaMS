#ifndef A_SRWIDGET_H
#define A_SRWIDGET_H

#include "ui_srwidget.h"
#include <QCloseEvent>
#include <QStringList>
#include <QPrinter>
#include <QShortcut>

class SRImage;

class SRWidget: public QWidget, public Ui::SRWidget
{
Q_OBJECT
public:
	SRWidget(float);
	~SRWidget();
	void initSR(const QString&);
	QStringList tmpfiles;
	std::vector<SRImage> srimages;

protected:
	void closeEvent(QCloseEvent*) override;
	void readSettings();
	void writeSettings();

public slots:
	void preview_print(QPrinter*);
	void printSR();
	void saveSR();

private slots:
	void check_close();

private:
	QString tmpfile;
	QShortcut * print_sc;
	QShortcut * save_sc;
	QShortcut * close_sc;
#ifdef __APPLE__
	QShortcut * minimaze_sc;
	QShortcut * fullsceen_sc;
	QShortcut * normal_sc;
#endif
};

#endif

