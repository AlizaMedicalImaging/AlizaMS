#ifndef A_STUDYGRAPHICSWIDGET_H
#define A_STUDYGRAPHICSWIDGET_H

#include "structures.h"
#include "studygraphicsview.h"
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QSlider>
#include <QList>
#include <QVector>
#include <QMap>
#include <QLineEdit>
#include <QThread>
#include <QPointF>
#include <QPen>
#include <QTimer>
#include <QMouseEvent>
#include <QEvent>
#include <QCloseEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>

class StudyViewWidget;
class Aliza;

class StudyGraphicsWidget : public QWidget
{

Q_OBJECT

public:
	StudyGraphicsWidget();
	~StudyGraphicsWidget();
	StudyGraphicsView * graphicsview;
	void set_id(int);
	int  get_id() const;
	void set_studyview(StudyViewWidget*);
	void set_aliza(Aliza*);
	void set_slider(QSlider*);
	void set_top_label(QLabel*);
	void set_left_label(QLabel*);
	void set_measure_label(QLabel*);
	void set_icon_button(QToolButton*);
	void set_top_string(const QString&);
	void set_left_string(const QString&);
	void set_measure_text(const QString&);
	void set_center(double);
	void set_width(double);
	void set_lut(short);
	void set_lut_function(short);
	void set_locked_window(bool);
	void reset_level();
	void update_image_color(int, int, int);
	void set_image(
		ImageVariant*,
		const short /* fit */,
		const bool /* alw usregions */);
	void update_image(const short /* fit */);
	void clear_();
	ImageContainer image_container;
	void  update_pr_area();
	void  update_background_color();
	void  set_smooth(bool);
	bool  get_smooth() const;
	void  set_mouse_modus(short,bool);
	short get_mouse_modus() const;
	bool  get_enable_shutter() const;
	bool  get_enable_overlays() const;
	void  set_enable_shutter(bool);
	void  set_enable_overlays(bool);
	void  set_active();
	void  update_measurement(double, double, double, double);
	void set_slider_only(int);
	void set_selected_slice2(int, bool);

private slots:
	void set_selected_slice(int);
	void toggle_single(bool);

signals:
	void slice_changed(int);

protected:
	void closeEvent(QCloseEvent*) override;
	void leaveEvent(QEvent*) override;
	void dropEvent(QDropEvent*) override;
	void dragEnterEvent(QDragEnterEvent*) override;
	void dragMoveEvent(QDragMoveEvent*) override;
	void dragLeaveEvent(QDragLeaveEvent*) override;

private:
	int m_id{-1};
	StudyViewWidget * studyview{};
	Aliza   * aliza{};
	QSlider * slider{};
	QLabel  * top_label{};
	QLabel  * left_label{};
	QLabel  * measure_label{};
	QToolButton * icon_button{};
	bool   smooth_{true};
	short  mouse_modus{};
	bool   enable_shutter{true};
	bool   enable_overlays{true};
};

#endif

