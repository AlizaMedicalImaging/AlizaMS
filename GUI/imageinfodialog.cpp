#include "imageinfodialog.h"

ImageInfoDialog::ImageInfoDialog()
{
	setupUi(this);
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

