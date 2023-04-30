#include "ctkdialog.h"
#include <QDate>
#include <QFileDialog>
#include <QDir>

CTKDialog::CTKDialog()
{
	setupUi(this);
	file_toolButton->hide();
	connect(dir_toolButton, SIGNAL(clicked()), this, SLOT(select_dir()));
}

QString CTKDialog::get_dir() const
{
	return dir_lineEdit->text();
}

QString CTKDialog::get_pname() const
{
	return pname_lineEdit->text();
}

QString CTKDialog::get_pid() const
{
	return pid_lineEdit->text();
}

QDate CTKDialog::get_from() const
{
	return from_dateEdit->date();
}

QDate CTKDialog::get_to() const
{
	return to_dateEdit->date();
}

bool CTKDialog::get_apply_range() const
{
	return range_checkBox->isChecked();
}

void CTKDialog::set_dir(const QString & d)
{
	dir_lineEdit->setText(d);
}

void CTKDialog::set_pname(const QString & t)
{
	pname_lineEdit->setText(t);
}

void CTKDialog::set_pid(const QString & t)
{
	pid_lineEdit->setText(t);
}

void CTKDialog::set_from(const QString & s)
{
	from_dateEdit->setDate(QDate::fromString(s, QString("yyyyMMdd")));
}

void CTKDialog::set_to(const QString & s)
{
	to_dateEdit->setDate(QDate::fromString(s, QString("yyyyMMdd")));
}

void CTKDialog::set_apply_range(bool t)
{
	range_checkBox->setChecked(t);
}

void CTKDialog::select_dir()
{
	const QString d =
		QFileDialog::getExistingDirectory(
			this, QString("Select CTK database directory"),
			dir_lineEdit->text(),
			(QFileDialog::ShowDirsOnly|QFileDialog::ReadOnly
			/* | QFileDialog::DontUseNativeDialog*/
			));
	if (!d.isEmpty())
		dir_lineEdit->setText(QDir::toNativeSeparators(d));
}

void CTKDialog::select_file()
{
	const QString f = QFileDialog::getOpenFileName(
		this,
		QString("Select file"),
		dir_lineEdit->text(),
		QString(),
		nullptr,
		(QFileDialog::ReadOnly
		/*| QFileDialog::DontUseNativeDialog*/
		));
	if (!f.isEmpty())
		dir_lineEdit->setText(QDir::toNativeSeparators(f));
}

void CTKDialog::set_tmp0()
{
	connect(file_toolButton, SIGNAL(clicked()), this, SLOT(select_file()));
	title_label->setText(QString("Open SQLite database"));
	label->setText(QString("File"));
	filter_label->hide();
	pname_label->hide();
	pid_label->hide();
	pname_lineEdit->hide();
	pid_lineEdit->hide();
	pname_toolButton->hide();
	pid_toolButton->hide();
	range_checkBox->hide();
	frame->hide();
	dir_toolButton->hide();
	file_toolButton->show();
}

