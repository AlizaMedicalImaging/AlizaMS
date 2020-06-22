#include "imageinfodialog.h"

ImageInfoDialog::ImageInfoDialog(
	QWidget * p,
	Qt::WindowFlags f)
	:
	QDialog(p, f)
{
	setupUi(this);
}

ImageInfoDialog::~ImageInfoDialog()
{
}

void ImageInfoDialog::set_label0(const QString & t)
{
	label0->setText(QString("<b>") + t + QString("</b>"));
}

void ImageInfoDialog::set_label1(const QString & t)
{
	label1->setText(t);
}


void ImageInfoDialog::set_text(const QString & t)
{
	plainTextEdit->setPlainText(t);
}

void ImageInfoDialog::adjust()
{
	adjustSize();
}

