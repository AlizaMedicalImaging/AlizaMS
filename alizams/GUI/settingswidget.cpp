#include "settingswidget.h"
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include "CG/glwidget-qt5.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#include <QStyleFactory>
#include <QApplication>
#include <QDir>
#include <QFont>
#include <QMessageBox>
#include "commonutils.h"
#include "dicomutils.h"

SettingsWidget::SettingsWidget(float si, QWidget * p, Qt::WindowFlags f) : QWidget(p, f)
{
	setupUi(this);
	dark_checkBox->hide(); // TODO
	scale_icons = si;
	x_comboBox->addItem(QString("256"));
	x_comboBox->addItem(QString("128"));
	x_comboBox->addItem(QString("64"));
	y_comboBox->addItem(QString("256"));
	y_comboBox->addItem(QString("128"));
	y_comboBox->addItem(QString("64"));
#if 1
	readSettings();
#else
	{
		QFont f = QApplication::font();
		const double app_font_pt = f.pointSizeF();
		if (app_font_pt > 0 && (f.pointSize() != -1))
		{
			pt_doubleSpinBox->setValue(app_font_pt);
		}
	}
#endif
	vbos65535warn = true;
	connect(reload_pushButton,SIGNAL(clicked()),this,SLOT(set_default()));
	connect(maxvbos_checkBox,SIGNAL(toggled(bool)),this,SLOT(update_vbos_max_65535(bool)));
	connect(pt_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(update_font_pt(double)));
}

SettingsWidget::~SettingsWidget()
{
}

short SettingsWidget::get_filtering() const
{
	if      (f_no_radioButton->isChecked())        return 0;
	else if (f_bilinear_radioButton->isChecked())  return 1;
	return 0;
}

bool SettingsWidget::get_resize() const
{
	if (resample_radioButton->isChecked()) return true;
	else return false;
}

int SettingsWidget::get_size_x() const
{
	int x; QVariant v(x_comboBox->currentText());
	bool ok = false; x = v.toInt(&ok); if (!ok) x = 0;
	return x;
}

int SettingsWidget::get_size_y() const
{
	int x; QVariant v(y_comboBox->currentText());
	bool ok = false; x = v.toInt(&ok); if (!ok) x = 0;
	return x;
}

bool SettingsWidget::get_rescale() const
{
	return rescale_checkBox->isChecked();
}

bool SettingsWidget::get_3d() const
{
	return (
		textureoptions_groupBox->isEnabled() &&
		textureoptions_groupBox->isChecked());
}

void SettingsWidget::set_enable_texture_groupbox(bool t)
{
	textureoptions_groupBox->setEnabled(t);
	textureoptions_groupBox->setVisible(t);
	maxvbos_checkBox->setEnabled(t);
	maxvbos_checkBox->setVisible(t);
}

void SettingsWidget::set_default()
{
	dark_checkBox->setChecked(true);
	si_doubleSpinBox->setValue(1.0);
	pt_doubleSpinBox->setValue(12.0);
	//
	original_radioButton->setChecked(true);
	resample_radioButton->setChecked(false);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	x_comboBox->setCurrentText(QString("256"));
	y_comboBox->setCurrentText(QString("256"));
#else
	x_comboBox->setItemText(0,QString("256"));
	y_comboBox->setItemText(0,QString("256"));
#endif
	f_bilinear_radioButton->setChecked(false);
	f_no_radioButton->setChecked(true);
	textureoptions_groupBox->setEnabled(true);
	textureoptions_groupBox->setChecked(true);
	maxvbos_checkBox->setChecked(false);
	//
	rescale_checkBox->setChecked(true);
	mosaic_checkBox->setChecked(true);
	time_s__checkBox->setChecked(false);
	overlays_checkBox->setChecked(true);
	clean_unused_checkBox->setChecked(false);
	//
	pet_no_level_checkBox->setChecked(false);
}

int SettingsWidget::get_time_unit() const
{
	if (time_s__checkBox->isChecked()) return 1;
	return 0;
}

bool SettingsWidget::get_mosaic() const
{
	return mosaic_checkBox->isChecked();
}

bool SettingsWidget::get_overlays() const
{
	return overlays_checkBox->isChecked();
}

bool SettingsWidget::get_vbos_max_65535() const
{
	return maxvbos_checkBox->isChecked();
}

void SettingsWidget::update_vbos_max_65535(bool t)
{
	GLWidget::set_max_vbos_65535(t);
}

void SettingsWidget::set_vbos65535_warn(bool t)
{
	vbos65535warn = t;
}

bool SettingsWidget::get_vbos65535_warn() const
{
	return vbos65535warn;
}

double SettingsWidget::get_font_pt() const
{
	return pt_doubleSpinBox->value();
}

void SettingsWidget::update_font_pt(double x)
{
	QFont f = QApplication::font();
	if (f.pointSize() != -1)
	{
		f.setPointSizeF(x);
		QApplication::setFont(f);
		QApplication::processEvents();
	}
}

bool SettingsWidget::get_level_for_PET() const
{
	return !pet_no_level_checkBox->isChecked();
}

bool SettingsWidget::get_clean_unused_bits() const
{
	return clean_unused_checkBox->isChecked();
}

void SettingsWidget::readSettings()
{
#if 1
	QSettings settings(
		QSettings::IniFormat,
		QSettings::UserScope,
		QApplication::organizationName(),
		QApplication::applicationName());
	settings.setFallbacksEnabled(true);
	settings.beginGroup(QString("GlobalSettings"));
	double tmp1 = settings.value(QString("scale_ui_icons"), 1.0).toDouble();
	double tmp2 = settings.value(QString("app_font_pt"), 0.0).toDouble();
	settings.endGroup();
	si_doubleSpinBox->setValue(tmp1);
	if (tmp2 <= 0.0) tmp2 = 12.0;
	pt_doubleSpinBox->setValue(tmp2);
#endif
}

void SettingsWidget::writeSettings(QSettings & settings)
{
#if 1
	const double scale_ui_icons = si_doubleSpinBox->value();
	const double x =
		pt_doubleSpinBox->value() < 6.0 ? 6.0 : pt_doubleSpinBox->value();
	settings.beginGroup(QString("GlobalSettings"));
	settings.setValue(QString("scale_ui_icons"),   QVariant(scale_ui_icons));
	settings.setValue(QString("app_font_pt"), QVariant(x));
	settings.endGroup();
#endif
}

float SettingsWidget::get_scale_icons() const
{
	return scale_icons*(float)si_doubleSpinBox->value();
}

bool SettingsWidget::get_dark_theme() const
{
	return dark_checkBox->isChecked();
}
