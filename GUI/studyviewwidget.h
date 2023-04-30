#ifndef A_STUDYVIEWWIDGET_H
#define A_STUDYVIEWWIDGET_H

#include "ui_studyviewwidget.h"
#include <QList>
#include <QIcon>
#include <QToolButton>
#include <QCloseEvent>
#include <QSettings>
#include <QShortcut>

class StudyFrameWidget;
class StudyGraphicsWidget;
class MatrixButton;
class ImageContainer;
class LUTWidget;

class StudyViewWidget : public QWidget, public Ui::StudyViewWidget
{

Q_OBJECT

public:
	StudyViewWidget(float, bool);
	~StudyViewWidget();
	void clear_();
	void set_horizontal(bool);
	void calculate_grid(int);
	int  get_active_id() const;
	void set_active_id(int);
	bool get_scouts() const;
	void set_active_image(ImageContainer*);
	void update_full(ImageContainer*);
	void update_level(ImageContainer*);
	void connect_tools();
	void disconnect_tools();
	void block_signals(bool);
	void update_null();
	void update_scouts();
	bool get_anchored_sliders() const;
	void update_all_sliders(int, int, int);
	void set_single(const unsigned long long);
	void restore_multi(const unsigned long long);
	void writeSettings(QSettings&);

	QList<StudyFrameWidget *> widgets;

protected:
	void closeEvent(QCloseEvent*) override;

private slots:
	void update_grid(int, int);
	void update_grid2();
	void set_center_slider(int);
	void set_width_slider(int);
	void set_center_spinbox(double);
	void set_width_spinbox(double);
	void set_lut_function(int);
	void set_lut(int);
	void toggle_lock_window(bool);
	void reset_level();
	void all_to_fit();
	void all_to_original();
	void toggle_scouts(bool);
	void toggle_measure(bool);
	void check_close();

signals:
	void update_scouts_required();

private:
	void update_locked_window(bool);
	void update_lut_function(int);
	void update_spinbox_center_d(double);
	void update_spinbox_width_d(double);
	void update_window_upper(double);
	void update_window_lower(double);
	void update_max_width(double);
	void update_locked_center(double);
	void update_locked_width(double);
	void readSettings();

	MatrixButton * mbutton;
	QToolButton * fitall_toolButton;
	QToolButton * scouts_toolButton;
	QToolButton * measure_toolButton;
	QToolButton * anchor_toolButton;
	LUTWidget * lutwidget;
	QIcon lockon;
	QIcon lockoff;
	bool horizontal;
	int active_id{-1};
	int saved_r{-1};
	int saved_c{-1};
	QShortcut * close_sc;
#ifdef __APPLE__
	QShortcut * minimaze_sc;
	QShortcut * fullsceen_sc;
	QShortcut * normal_sc;
#endif
};

#endif

