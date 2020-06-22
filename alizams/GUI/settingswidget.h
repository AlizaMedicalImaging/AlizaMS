#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include "ui_settingswidget.h"
#include <QWidget>
#include <QSettings>

class SettingsWidget: public QWidget, public Ui::SettingsWidget
{
	Q_OBJECT

public:
	SettingsWidget(QWidget(*)=NULL, Qt::WindowFlags=0);
	~SettingsWidget();
	short   get_filtering() const;
	bool    get_resize() const;
	int     get_size_x() const;
	int     get_size_y() const;
	bool    get_rescale() const;
	bool    get_force_rescale() const;
	bool    get_3d() const;
	void    set_enable_texture_groupbox(bool);
	int     get_time_unit() const;
	bool    get_mosaic() const;
	bool    get_overlays() const;
	bool    get_vbos_max_65535() const;
	void    set_vbos65535_warn(bool);
	bool    get_vbos65535_warn() const;
	double  get_font_pt() const;
	bool    get_shutter() const;
	bool    get_level_for_PET() const;
	bool    get_clean_unused_bits() const;
	void    writeSettings(QSettings&);

private:
	bool vbos65535warn;

private slots:
	void set_default();
	void update_vbos_max_65535(bool);

public slots:
	void update_font_pt(double);

protected:
	void readSettings();
};

#endif // SETTINGSWIDGET_H
