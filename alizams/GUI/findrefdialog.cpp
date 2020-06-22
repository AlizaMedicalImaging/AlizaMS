#include "findrefdialog.h"
#include <QFileDialog>
#include <QDir>

FindRefDialog::FindRefDialog(QWidget * p, Qt::WindowFlags f)
	: QDialog(p, f)
{
	setupUi(this);
	connect(toolButton, SIGNAL(clicked()), this, SLOT(select_dir()));
}

FindRefDialog::~FindRefDialog()
{
}

void FindRefDialog::set_path(const QString & p)
{
	lineEdit->setText(p);
}

QString FindRefDialog::get_path() const
{
	return lineEdit->text();
}

void FindRefDialog::select_dir()
{
	const QString d =
		QFileDialog::getExistingDirectory(
			this,
			QString("Select directory"),
			lineEdit->text(),
			(QFileDialog::ShowDirsOnly|QFileDialog::ReadOnly
			/* | QFileDialog::DontUseNativeDialog*/
			));
	if (!d.isEmpty())
		lineEdit->setText(QDir::toNativeSeparators(d));
}

void FindRefDialog::set_text(const QString & s)
{
	label->setText(s);
}

