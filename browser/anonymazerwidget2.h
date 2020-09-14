#ifndef ANONYMAZERWIDGET2_H__
#define ANONYMAZERWIDGET2_H__

#include "ui_anonymazerwidget2.h"
#include "mdcmTag.h"
#include <QStringList>
#include <QMap>
#include <QProgressDialog>
#include <QCloseEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QSettings>
#include <set>

#define ALIZAMS_ROOT_UID "1.2.826.0.1.3680043.9.4245"

class AnonymazerWidget2 : public QWidget, private Ui::AnonymazerWidget2
{
Q_OBJECT
public:
	AnonymazerWidget2(float, QWidget(*)=NULL,Qt::WindowFlags=0);
	~AnonymazerWidget2();
	void writeSettings(QSettings&);
public slots:
	void run_();
	void set_output_dir();
	void set_input_dir();
protected:
	void closeEvent(QCloseEvent*);
	void readSettings();
	void dropEvent(QDropEvent*);
	void dragEnterEvent(QDragEnterEvent*);
	void dragMoveEvent(QDragMoveEvent*);
	void dragLeaveEvent(QDragLeaveEvent*);
private:
	void process_directory(
		const QString&,
		const QString&,
		const QMap<QString,QString> &,
		const QMap<QString,QString> &,
		unsigned int*,
		unsigned int*,
		QProgressDialog*);
	void init_profile();
	std::set<mdcm::Tag> pn_tags;
	std::set<mdcm::Tag> uid_tags;
	std::set<mdcm::Tag> empty_tags;
	std::set<mdcm::Tag> remove_tags;
	std::set<mdcm::Tag> device_tags;
	std::set<mdcm::Tag> patient_tags;
	std::set<mdcm::Tag> institution_tags;
	QString output_dir;
	QString input_dir;
};

#endif // ANONYMAZERWIDGET2_H__
