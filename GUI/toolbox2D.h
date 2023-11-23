#ifndef A_TOOLBOX2D_H
#define A_TOOLBOX2D_H

#include "ui_toolbox2D.h"
#include <QPixmap>
#include <QIcon>

class ToolBox2D: public QWidget, public Ui::ToolBox2D
{
Q_OBJECT
public:
	ToolBox2D(float);
	~ToolBox2D() = default;
	void set_style_sheet();
	void set_indicator_red();
	void set_indicator_green();
	void set_indicator_blue();
	bool is_red() const;
	bool is_green() const;
	bool is_blue() const;
	void set_lut_function(int);
	void connect_sliders();
	void disconnect_sliders();
	bool get_locked() const;
	void toggle_locked_values(bool);
	void set_locked_center(double);
	void set_locked_width(double);

public slots:
	void set_window_upper(double);
	void set_window_lower(double);
	void set_max_width(double);
	void set_width(double);
	void set_center(double);
	void update_slider_center(double);
	void update_slider_width(double);
	void update_spinbox_center(int);
	void update_spinbox_width(int);
	void set_maxwindow(bool);
	void enable_maxwindow(bool);

private:
	QPixmap red;
	QPixmap green;
	QPixmap blue;
	QIcon lockon;
	QIcon lockoff;
	bool label_is_red{};
	bool label_is_green{};
	bool label_is_blue{};
};

#endif

