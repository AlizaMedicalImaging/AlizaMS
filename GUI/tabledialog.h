#ifndef TABLEDIALOG_H
#define TABLEDIALOG_H

#include <QDialog>
#include "ui_tabledialog.h"

class TableDialog : public QDialog, private Ui::TableDialog
{
Q_OBJECT
public:
	TableDialog();
	~TableDialog() {}
	int get_rows() const;
	int get_columns() const;
};

#endif  // TABLEDIALOG_H
