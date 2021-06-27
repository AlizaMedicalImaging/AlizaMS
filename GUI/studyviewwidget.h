#ifndef STUDYVIEWWIDGET__H
#define STUDYVIEWWIDGET__H

#include "ui_studyviewwidget.h"
#include <QList>
#include <QIcon>
#include <QCloseEvent>

class StudyFrameWidget;
class StudyGraphicsWidget;
class MatrixButton;
class ImageContainer;
class LUTWidget;

class StudyViewWidget : public QWidget, public Ui::StudyViewWidget
{

Q_OBJECT

public:
	StudyViewWidget(float);
	~StudyViewWidget();
	void clear_();
	void set_horizontal(bool);
	void calculate_grid(int);
	int  get_active_id() const;
	void set_active_id(int);
	void set_active_image(ImageContainer*);
	void update_full(ImageContainer*);
	void update_level(ImageContainer*);
	void connect_tools();
	void disconnect_tools();
	void block_signals(bool);
	void update_null();
	void update_scouts();

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

	MatrixButton * mbutton;
	LUTWidget * lutwidget;
	QIcon lockon;
	QIcon lockoff;
	bool horizontal;
	int active_id;
};

#endif // STUDYVIEWWIDGET__H
