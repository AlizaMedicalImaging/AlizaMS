#ifndef A_SETTINGSWIDGET_H
#define A_SETTINGSWIDGET_H

#include "ui_settingswidget.h"
#include <QSettings>

class SettingsWidget: public QWidget, public Ui::SettingsWidget
{
Q_OBJECT
public:
	SettingsWidget(float);
	~SettingsWidget();
	short  get_filtering() const;
	bool   get_resize() const;
	int    get_size_x() const;
	int    get_size_y() const;
	bool   get_rescale() const;
	bool   get_force_rescale() const;
	bool   get_3d() const;
	void   set_gl_visible(bool);
	int    get_time_unit() const;
	bool   get_mosaic() const;
	bool   get_overlays() const;
	double get_font_pt() const;
	bool   get_shutter() const;
	bool   get_level_for_PET() const;
	bool   get_clean_unused_bits() const;
	void   writeSettings(QSettings&);
	float  get_scale_icons() const;
	bool   get_sr_info() const;
	int    get_sr_image_width() const;
	bool   get_sr_chapters() const;
	bool   get_sr_skip_images() const;
	bool   get_predictor_workaround() const;
	bool   get_cornell_workaround() const;
	bool   get_apply_icc() const;
	bool   get_try_fix_jpeg_prec() const;
	short  get_enh_strategy() const;

private:
	int   saved_idx;
	float scale_icons;

private slots:
	void set_default();

public slots:
	void update_font_pt(double);
	void force_no_gl3();


protected:
	void readSettings();
};

#endif

