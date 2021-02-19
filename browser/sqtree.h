#ifndef SQTREE__H__
#define SQTREE__H__

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
	void read_file(const QString&);
	void clear_tree();
	void set_list_of_files(const QStringList&);

public slots:
	void copy_to_clipboard();
	void collapse_item();
	void expand_item();
	void open_file();
	void file_from_slider(int);
	void open_file_and_series();

protected:
	void closeEvent(QCloseEvent*);
	void dropEvent(QDropEvent*);
	void dragEnterEvent(QDragEnterEvent*);
	void dragMoveEvent(QDragMoveEvent*);
	void dragLeaveEvent(QDragLeaveEvent*);

private:
	void process_element(
		const mdcm::DataSet&,
		const mdcm::DataElement&,
		const mdcm::Dicts&,
		QTreeWidgetItem*,
		const char*,
		const bool=false);
	void dump_csa(const mdcm::DataSet&);
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
	QMutex mutex;
#endif
};

#endif // SQTREE__H__
