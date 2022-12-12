#ifndef A_SQTREE_H
#define A_SQTREE_H

#include "ui_sqtree.h"
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStringList>
#include <QMutex>
#include <QCloseEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QModelIndex>
#include <string>
#include <mdcmDataSet.h>
#include <mdcmDataElement.h>
#include <mdcmDicts.h>

#define SQTREE_LOCK_TREE 1

class SQtree: public QWidget, private Ui::SQtree
{
Q_OBJECT
public:
	SQtree(bool=true);
	~SQtree();
	void read_file(const QString&, const bool);
	void read_file_and_series(const QString&, const bool);
	void clear_tree();
	void set_list_of_files(const QStringList&);

public slots:
	void open_file();
	void open_file_and_series();

private slots:
	void copy_to_clipboard();
	void collapse_item();
	void expand_item();
	void file_from_slider(int);

protected:
	void closeEvent(QCloseEvent*) override;
	void dropEvent(QDropEvent*) override;
	void dragEnterEvent(QDragEnterEvent*) override;
	void dragMoveEvent(QDragMoveEvent*) override;
	void dragLeaveEvent(QDragLeaveEvent*) override;

private:
	void process_element(
		const mdcm::DataSet&,
		const mdcm::DataElement&,
		const mdcm::Dicts&,
		QTreeWidgetItem*,
		const char*,
		const bool=false);
	void dump_csa(const mdcm::DataSet&);
	void dump_gems(const mdcm::DataSet&);
	void expand_children(const QModelIndex&);
	void collapse_children(const QModelIndex&);
	void process_attribure(const short);
	QString saved_dir;
	QAction * copyAct;
	QAction * collapseAct;
	QAction * expandAct;
	bool skip_settings_pos;
	QStringList list_of_files;
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutable QMutex mutex;
#endif
};

#endif
