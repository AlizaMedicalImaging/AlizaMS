#ifndef A_IMAGESBOX_H
#define A_IMAGESBOX_H

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
	ListWidgetItem2(QListWidget(*) = nullptr, int = Type);
	ListWidgetItem2(const QString&, QListWidget(*) = nullptr, int = Type);
	ListWidgetItem2(const QIcon&, const QString&, QListWidget(*) = nullptr, int = Type);
	ListWidgetItem2(int, ImageVariant*, const QIcon&, const QString&, QListWidget(*) = nullptr, int = Type);
	ListWidgetItem2(int, ImageVariant*, QListWidget(*) = nullptr, int = Type);
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
	TableWidgetItem2()                  : QTableWidgetItem(QTableWidgetItem::UserType + 1) {}
	TableWidgetItem2(const QString & s) : QTableWidgetItem(s, QTableWidgetItem::UserType + 1) {}
	~TableWidgetItem2() {}
	void set_id(int);
	int  get_id() const;

private:
	int id{-1};
};

class ImagesBox: public QWidget, public Ui::ImagesBox
{
Q_OBJECT
public:
	ImagesBox(float);
	~ImagesBox();
	void add_image(int, ImageVariant*, QPixmap(*) = nullptr);
	void set_html(const ImageVariant*);
	void check_all();
	void uncheck_all();
	void set_contours(const ImageVariant*);
	int  get_selected_roi_id() const;
	void update_background_color(bool);
	QDoubleSpinBox * width_doubleSpinBox;
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
	QAction * actionDICOMMeta;
	QAction * actionContours;
	QAction * actionROIInfo;
	QMenu   * studyMenu;
	QAction * actionStudyMenu;
	QAction * actionStudy;
	QAction * actionStudyChecked;

private slots:
	void toggle_info(bool);
	void toggle_contours(bool);

private:
	QMap<char,QString> map;
	QString get_orientation_image(const QString&);
};

#endif

