#ifndef A_IMAGEINFODIALOG_H
#define A_IMAGEINFODIALOG_H

#include "ui_imageinfodialog.h"

class ImageInfoDialog: public QDialog, public Ui::ImageInfoDialog
{
Q_OBJECT
public:
	ImageInfoDialog();
	~ImageInfoDialog();
	void set_label0(const QString&);
	void set_label1(const QString&);
	void set_text(const QString&);
	void adjust();
};

#endif
