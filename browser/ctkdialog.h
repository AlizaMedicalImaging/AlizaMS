#ifndef CTKDIALOG_H
#define CTKDIALOG_H

#include "ui_ctkdialog.h"
#include <QDate>

class CTKDialog: public QDialog, private Ui::CTKDialog
{
Q_OBJECT
public:
	CTKDialog(QWidget(*)=NULL);
	~CTKDialog() {}
	QString get_dir() const;
	QString get_pname() const;
	QString get_pid() const;
	QDate   get_from() const;
	QDate   get_to() const;
	bool    get_apply_range() const;
	void    set_dir(const QString&);
	void    set_pname(const QString&);
	void    set_pid(const QString&);
	void    set_from(const QString&);
	void    set_to(const QString&);
	void    set_apply_range(bool);
	void    set_tmp0();

private slots:
	void select_dir();
	void select_file();
};

#endif // CTKDIALOG_H
