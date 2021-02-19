#ifndef BROWSERWIDGET2___H
#define BROWSERWIDGET2___H

#include "ui_browserwidget2.h"
#include <QApplication>
#include <QWidget>
#include <QModelIndex>
#include <QProgressDialog>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QCloseEvent>
#include <QIcon>
#include <QSettings>
#include <set>
#include "mdcmTag.h"
#include "mdcmVL.h"
#include "mdcmSimpleSubjectWatcher.h"
#include "mdcmEvent.h"
#include "mdcmDataSet.h"
#include "mdcmDict.h"

class ScannerWatcher : public mdcm::SimpleSubjectWatcher
{
public:
	ScannerWatcher(mdcm::Subject * s, const char * comment = "") : mdcm::SimpleSubjectWatcher(s, comment) {}
	void StartFilter()
	{
		QApplication::processEvents();
	}
	void EndFilter()
	{
		QApplication::processEvents();
	}
	void ShowProgress(mdcm::Subject*, const mdcm::Event&)
	{
		QApplication::processEvents();
	}
	void ShowFileName(mdcm::Subject*, const mdcm::Event&) {}
};

class EntryDICOMDIR
{
public:
	EntryDICOMDIR() :
		offsetOfTheNextDirectoryRecord(0),
		offsetOfReferencedLowerLevelDirectoryEntity(0) {}
	~EntryDICOMDIR() {}
	unsigned int offsetOfTheNextDirectoryRecord;
	unsigned int offsetOfReferencedLowerLevelDirectoryEntity;
	QString directoryRecordType;
	QString modality;
	QString patient;
	QString birthdate;
	QString study_date;
	QString study_desc;
	QString series_date;
	QString series_desc;
	QString file;
};

class SeriesDICOMDIR
{
public:
	SeriesDICOMDIR() :
		eye(false), eye2(false)
	{} 
	~SeriesDICOMDIR() {}
	bool    eye;
	bool    eye2;
	QString UID;
	QString modality;
	QString patient;
	QString birthdate;
	QString study;
	QString study_date;
	QString series;
	QString series_date;
	QStringList files;
};

class TableWidgetItem : public QTableWidgetItem
{
public:
	TableWidgetItem()                  : QTableWidgetItem(QTableWidgetItem::UserType+1)    {}
	TableWidgetItem(const QString & s) : QTableWidgetItem(s, QTableWidgetItem::UserType+1) {}
	~TableWidgetItem() {}
	QStringList files;
};

class BrowserWidget2: public QWidget, public Ui::BrowserWidget2
{
	Q_OBJECT
public:
	BrowserWidget2(float, QWidget(*)=NULL);
	~BrowserWidget2();
	bool          is_first_run()  const;
	const QString read_DICOMDIR(const QString&);
	QStringList   get_files_of_1st();
	void          writeSettings(QSettings&);

protected:
	void closeEvent(QCloseEvent*);
	void dropEvent(QDropEvent*);
	void dragEnterEvent(QDragEnterEvent*);
	void dragMoveEvent(QDragMoveEvent*);
	void dragLeaveEvent(QDragLeaveEvent*);
	void readSettings();

public slots:
	void read_directory(const QString&);
	void open_dicom_dir2(const QString&);
	void open_DICOMDIR2(const QString&);
	void reload_dir();
	void open_dicom_dir();
	void copy_files();
	void open_DICOMDIR();
#ifdef USE_WORKSTATION_MODE
	void open_CTK_db();
#endif

private:
	bool once;
	QString saved_copy_dir;
	QIcon eye_icon;
	QIcon eye2_icon;
	std::set<mdcm::Tag> selected_tags;
	std::set<mdcm::Tag> selected_tags_short;
	mdcm::VL compute_offset0(const mdcm::DataSet&);
	void compute_offsets(
		const mdcm::SequenceOfItems*,
		mdcm::VL,
		std::vector<unsigned int> &);
	void process_directory(const QString&, const mdcm::Dict&, QProgressDialog*);
	unsigned int add_roots(
		const QMap<unsigned int, EntryDICOMDIR> &,
		unsigned int,
		QList<SeriesDICOMDIR> &,
		bool*);
	unsigned int add_study(
		const QMap<unsigned int, EntryDICOMDIR> &,
		unsigned int,
		QList<SeriesDICOMDIR> &,
		const QString&,
		const QString&,
		bool*);
	unsigned int add_series(
		const QMap<unsigned int, EntryDICOMDIR> &,
		unsigned int,
		QList<SeriesDICOMDIR> &,
		const QString&,
		const QString&,
		const QString&,
		const QString&,
		bool*);
	unsigned int add_file(
		const QMap<unsigned int, EntryDICOMDIR> &,
		unsigned int,
		SeriesDICOMDIR&);
	void read_tags_(
		const QString &,
		QString&,QString&,QString&,
		QString&,QString&,QString&,QString&,
		bool*,bool*);
	void read_tags_short_(const QString&, bool*, bool*);
#ifdef USE_WORKSTATION_MODE
	QString ctk_dir;
	QString ctk_pname;
	QString ctk_pid;
	QString ctk_from;
	QString ctk_to;
	bool    ctk_apply_range;
#endif
};

#endif // BROWSERWIDGET2___H

