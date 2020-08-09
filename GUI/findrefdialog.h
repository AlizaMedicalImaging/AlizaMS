#ifndef FINDREFDIALOG_H___
#define FINDREFDIALOG_H___

#include "ui_findrefdialog.h"

class FindRefDialog: public QDialog, private Ui::FindRefDialog
{
Q_OBJECT
public:
	FindRefDialog(float, QWidget(*)=NULL,Qt::WindowFlags=0);
	~FindRefDialog();
	void set_path(const QString&);
	QString get_path() const;
	void set_text(const QString&);
private slots:
	void select_dir();
};

#endif // FINDREFDIALOG_H___
