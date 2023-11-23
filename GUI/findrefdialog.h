#ifndef A_FINDREFDIALOG_H
#define A_FINDREFDIALOG_H

#include "ui_findrefdialog.h"

class FindRefDialog: public QDialog, private Ui::FindRefDialog
{
Q_OBJECT
public:
	FindRefDialog(float);
	~FindRefDialog() = default;
	void set_path(const QString&);
	QString get_path() const;
	void set_text(const QString&);

private slots:
	void select_dir();
};

#endif

