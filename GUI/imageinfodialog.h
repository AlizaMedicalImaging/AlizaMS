#ifndef IMAGEINFODIALOG_H
#define IMAGEINFODIALOG_H

#include "ui_imageinfodialog.h"

class ImageInfoDialog: public QDialog, public Ui::ImageInfoDialog
{
Q_OBJECT
public:
	ImageInfoDialog(QWidget(*)=NULL, Qt::WindowFlags=0);
	~ImageInfoDialog();
	void set_label0(const QString&);
	void set_label1(const QString&);
	void set_text(const QString&);
	void adjust();
};

#endif // IMAGEINFODIALOG_H
