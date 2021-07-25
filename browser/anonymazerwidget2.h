#ifndef ANONYMAZERWIDGET2_H__
#define ANONYMAZERWIDGET2_H__

#include "ui_anonymazerwidget2.h"
#include "mdcmTag.h"
#include "mdcmDicts.h"
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

#define ALIZAMS_ROOT_UID "1.2.826.0.1.3680043.10.585"

class HelpWidget;

class AnonymazerWidget2 : public QWidget, private Ui::AnonymazerWidget2
{
Q_OBJECT
public:
	AnonymazerWidget2(float);
	~AnonymazerWidget2();
	void writeSettings(QSettings&);

public slots:
	void run_();
	void set_output_dir();
	void set_input_dir();
	void show_help();

protected:
	void closeEvent(QCloseEvent*) override;
	void readSettings();
	void dropEvent(QDropEvent*) override;
	void dragEnterEvent(QDragEnterEvent*) override;
	void dragMoveEvent(QDragMoveEvent*) override;
	void dragLeaveEvent(QDragLeaveEvent*) override;

private:
	void process_directory(
		const QString&,
		const QString&,
		const QMap<QString,QString> &,
		const QMap<QString,QString> &,
		const QMap<QString,QString> &,
		unsigned int*,
		unsigned int*,
		const mdcm::Dicts&,
		const int,
		const int,
		const int,
		const int,
		const bool,
		const QString&,
		const QString&,
		const bool preserve_uids,
		const bool remove_private,
		const bool remove_graphics,
		const bool remove_descriptions,
		const bool remove_struct,
		const bool retain_device_id,
		const bool retain_patient_chars,
		const bool retain_institution_id,
		const bool retain_dates_times,
		const bool confirm_clean_pixel,
		const bool confirm_no_recognizable,
		const bool rename_files,
		QProgressDialog*);
	void init_profile();
	std::set<mdcm::Tag> pn_tags;
	std::set<mdcm::Tag> id_tags;
	std::set<mdcm::Tag> uid_tags;
	std::set<mdcm::Tag> empty_tags;
	std::set<mdcm::Tag> remove_tags;
	std::set<mdcm::Tag> dev_remove_tags;
	std::set<mdcm::Tag> dev_empty_tags;
	std::set<mdcm::Tag> dev_replace_tags;
	std::set<mdcm::Tag> patient_tags;
	std::set<mdcm::Tag> inst_remove_tags;
	std::set<mdcm::Tag> inst_empty_tags;
	std::set<mdcm::Tag> inst_replace_tags;
	std::set<mdcm::Tag> time_tags;
	std::set<mdcm::Tag> descr_remove_tags;
	std::set<mdcm::Tag> descr_empty_tags;
	std::set<mdcm::Tag> descr_replace_tags;
	std::set<mdcm::Tag> struct_zero_tags;
	std::set<mdcm::Tag> zero_seq_tags;
	QString output_dir;
	QString input_dir;
	HelpWidget * help_widget;
};

#endif // ANONYMAZERWIDGET2_H__
