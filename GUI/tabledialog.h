#ifndef A_TABLEDIALOG_H
#define A_TABLEDIALOG_H

#include "ui_tabledialog.h"

class TableDialog : public QDialog, private Ui::TableDialog
{
Q_OBJECT
public:
	TableDialog();
	~TableDialog() = default;
	int get_rows() const;
	int get_columns() const;
};

#endif

