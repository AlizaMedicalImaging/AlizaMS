#ifndef IMAGESBOX_H
#define IMAGESBOX_H

#include "ui_imagesbox.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QTableWidgetItem>
#include <QPixmap>
#include <QMap>
#include <QList>
#include <QToolBar>
#include <QMenu>
#include <QDoubleSpinBox>

class ImageVariant;

class ListWidgetItem2 : public QListWidgetItem
{
public:
	ListWidgetItem2(QListWidget(*)= NULL, int= Type);
	ListWidgetItem2(const QString&, QListWidget(*)=NULL, int=Type);
	ListWidgetItem2(const QIcon&, const QString&, QListWidget(*)=NULL, int=Type);
	ListWidgetItem2(int, ImageVariant*, const QIcon&, const QString&, QListWidget(*)=NULL, int=Type);
	ListWidgetItem2(int, ImageVariant*, QListWidget(*)=NULL, int=Type);
	~ListWidgetItem2();
	int get_id() const;
	ImageVariant * get_image_from_item();
	const ImageVariant * get_image_from_item_const() const;
private:
	int id;
	ImageVariant * ivariant;
};

class TableWidgetItem2 : public QTableWidgetItem
{
public:
	TableWidgetItem2()                  : QTableWidgetItem(QTableWidgetItem::UserType+1),    id(-1) {}
	TableWidgetItem2(const QString & s) : QTableWidgetItem(s, QTableWidgetItem::UserType+1), id(-1) {}
	~TableWidgetItem2() {}
	void set_id(int);
	int  get_id() const;
private:
	int id;
};

class ImagesBox: public QWidget, public Ui::ImagesBox
{
	Q_OBJECT
public:
	ImagesBox(float, QWidget(*)=NULL, Qt::WindowFlags f = 0);
	~ImagesBox();
	void add_image(int, ImageVariant*, QPixmap(*)=NULL);
	void set_html(const ImageVariant*);
	void check_all();
	void uncheck_all();
	QAction * actionNone;
	QAction * actionClear;
	QAction * actionClearChecked;
	QAction * actionClearUnChek;
	QAction * actionClearAll;
	QAction * actionInfo;
	QAction * actionCheck;
	QAction * actionUncheck;
	QAction * actionReloadHistogram;
	QAction * actionTmp;
	QAction * actionColor;

private slots:
	void toggle_info(bool);

private:
	QMap<char,QString> map;
	int     get_orientation_image_index(const QString&);
	QString get_orientation_image(const QString&);
};

#endif // IMAGESBOX_H
