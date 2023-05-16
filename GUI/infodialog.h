#ifndef A_INFODIALOG_H
#define A_INFODIALOG_H

#include "ui_infodialog.h"
#include <QDialog>
#include <QString>

class InfoDialog: public QDialog, public Ui::InfoDialog
{
Q_OBJECT
public:
	InfoDialog();
	~InfoDialog();
	void set_text(const QString&);
};

#endif
