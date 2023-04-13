#include "settingswidget.h"
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt5-core.h"
#else
#include "CG/glwidget-qt5.h"
#endif
#else
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt4-core.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#endif
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QFont>
#include <QFile>
#include <QFileInfo>
#include "commonutils.h"
#include "dicomutils.h"
#include "codecutils.h"

SettingsWidget::SettingsWidget(float si)
{
	setupUi(this);
	saved_idx = 0;
	scale_icons = si;
	x_comboBox->addItem(QString("256"));
	x_comboBox->addItem(QString("128"));
	x_comboBox->addItem(QString("64"));
	y_comboBox->addItem(QString("256"));
	y_comboBox->addItem(QString("128"));
	y_comboBox->addItem(QString("64"));
	{
		QStringList keys = QStyleFactory::keys();
		styleComboBox->addItem(QString("Dark Fusion"));
		styleComboBox->addItems(keys);
	}
	readSettings();
	styleComboBox->setCurrentIndex(saved_idx);
	connect(reload_pushButton,SIGNAL(clicked()),this,SLOT(set_default()));
	connect(pt_doubleSpinBox,SIGNAL(valueChanged(double)),this,SLOT(update_font_pt(double)));
	connect(cp1251_checkBox,SIGNAL(toggled(bool)),this,SLOT(set_force_cp1251(bool)));
}

SettingsWidget::~SettingsWidget()
{
}

short SettingsWidget::get_filtering() const
{
	if      (f_no_radioButton->isChecked())       return 0;
	else if (f_bilinear_radioButton->isChecked()) return 1;
	return 0;
}

bool SettingsWidget::get_resize() const
{
	if (resample_radioButton->isChecked()) return true;
	else return false;
}

int SettingsWidget::get_size_x() const
{
	int x;
	QVariant v(x_comboBox->currentText());
	bool ok = false;
	x = v.toInt(&ok);
	if (!ok) x = 0;
	return x;
}

int SettingsWidget::get_size_y() const
{
	int x;
	QVariant v(y_comboBox->currentText());
	bool ok = false;
	x = v.toInt(&ok);
	if (!ok) x = 0;
	return x;
}

bool SettingsWidget::get_rescale() const
{
	return rescale_checkBox->isChecked();
}

bool SettingsWidget::get_3d() const
{
	return (gl3D_checkBox->isChecked() &&
		textureoptions_groupBox->isChecked());
}

void SettingsWidget::set_gl_visible(bool t)
{
	textureoptions_groupBox->setVisible(t);
}

void SettingsWidget::set_default()
{
#if 1
	{
		QSettings settings(
			QSettings::IniFormat,
			QSettings::UserScope,
			QApplication::organizationName(),
			QApplication::applicationName());
		QFileInfo fi(settings.fileName());
		if (fi.exists())
		{
			const QString p = fi.absoluteFilePath();
			const bool ok = QFile::remove(p);
			if (!ok)
			{
				std::cout << "Could not remove config file"
					<< std::endl;
			}
		}
	}
#endif
	styleComboBox->setCurrentIndex(0);
	gl3D_checkBox->setChecked(true);
	si_doubleSpinBox->setValue(1.2);
	hidezoom_checkBox->setChecked(true);
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
	textureoptions_groupBox->setVisible(true);
	textureoptions_groupBox->setChecked(true);
	rescale_checkBox->setChecked(true);
	mosaic_checkBox->setChecked(true);
	time_s__checkBox->setChecked(false);
	overlays_checkBox->setChecked(true);
	clean_unused_checkBox->setChecked(true);
	pred6_checkBox->setChecked(false);
	cornell_checkBox->setChecked(false);
	pet_no_level_checkBox->setChecked(false);
	srinfo_checkBox->setChecked(false);
	srscale_checkBox->blockSignals(true);
	srscale_checkBox->setChecked(false);
	srwidth_spinBox->setValue(512);
	srwidth_spinBox->hide();
	srscale_checkBox->blockSignals(false);
	srchapters_checkBox->setChecked(true);
	srskipimage_checkBox->setChecked(false);
	icc_checkBox->setChecked(true);
	jpegprec_checkBox->setChecked(false);
	//
	enh_dim_uniform_radioButton->setChecked(true);
	enh_dim_strict_radioButton->setChecked(false);
	enh_dim_strict_1_radioButton->setChecked(false);
	enh_dim_skip_radioButton->setChecked(false);
	//
	pt_doubleSpinBox->setEnabled(false);
	disconnect(
		pt_doubleSpinBox,SIGNAL(valueChanged(double)),
		this,SLOT(update_font_pt(double)));
	pt_doubleSpinBox->setValue(0.0);
	//
	cp1251_checkBox->blockSignals(true);
	cp1251_checkBox->setChecked(false);
	set_force_cp1251(false);
	cp1251_checkBox->blockSignals(false);
}

void SettingsWidget::set_force_cp1251(bool b)
{
	CodecUtils::set_force_cp1251(b);
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

double SettingsWidget::get_font_pt() const
{
	return pt_doubleSpinBox->value();
}

void SettingsWidget::update_font_pt(double x)
{
	QFont f = QApplication::font();
	if (x < 6.0)
	{
		pt_doubleSpinBox->blockSignals(true);
		pt_doubleSpinBox->setValue(6.0);
		f.setPointSizeF(6.0);
		pt_doubleSpinBox->blockSignals(false);
	}
	else
	{
		f.setPointSizeF(x);
	}
	QApplication::setFont(f);
	QApplication::processEvents();
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
	QSettings settings(
		QSettings::IniFormat,
		QSettings::UserScope,
		QApplication::organizationName(),
		QApplication::applicationName());
	settings.setFallbacksEnabled(true);
	settings.beginGroup(QString("GlobalSettings"));
	const int tmp0  = settings.value(QString("enable_gl_3D"),    1).toInt();
	double tmp1     = settings.value(QString("scale_ui_icons"),1.2).toDouble();
	double tmp2     = settings.value(QString("app_font_pt"),   0.0).toDouble();
	const int tmp3  = settings.value(QString("sr_i_scale"),      0).toInt();
	const int tmp4  = settings.value(QString("sr_i_width"),    512).toInt();
	const int tmp5  = settings.value(QString("sr_info2"),        0).toInt();
	const int tmp6  = settings.value(QString("sr_chapters"),     1).toInt();
	const int tmp7  = settings.value(QString("sr_skip_images"),  0).toInt();
	const int tmp8  = settings.value(QString("dcm_overlays"),    1).toInt();
	const int tmp9  = settings.value(QString("dcm_mosaic"),      1).toInt();
	const int tmp10 = settings.value(QString("enh_strategy"),    1).toInt();
	const int tmp11 = settings.value(QString("apply_icc"),       1).toInt();
	const int tmp12 = settings.value(QString("clean_unused"),    1).toInt();
	const int tmp13 = settings.value(QString("hide_zoom"),       1).toInt();
	const int tmp14 = settings.value(QString("force_cp1241"),    0).toInt();
	settings.endGroup();
	settings.beginGroup(QString("StyleDialog"));
	saved_idx = settings.value(QString("saved_idx"), 0).toInt();
	settings.endGroup();
	gl3D_checkBox->setChecked((tmp0 == 1));
	si_doubleSpinBox->setValue(tmp1);
	hidezoom_checkBox->setChecked((tmp13 == 1));
	QFont f = QApplication::font();
	if (tmp2 < 6.0) tmp2 = 6.0; 
	else            tmp2 = f.pointSizeF();
	pt_doubleSpinBox->setValue(tmp2);
	if (tmp4 >= 64) srwidth_spinBox->setValue(tmp4);
	else            srwidth_spinBox->setValue(512);
	if (tmp3 == 1)
	{
		srscale_checkBox->blockSignals(true);
		srscale_checkBox->setChecked(true);
		srscale_checkBox->blockSignals(false);
	}
	else
	{
		srscale_checkBox->blockSignals(true);
		srscale_checkBox->setChecked(false);
		srwidth_spinBox->hide();
		srscale_checkBox->blockSignals(false);
	}
	srinfo_checkBox->setChecked((tmp5 == 1));
	srchapters_checkBox->setChecked((tmp6 == 1));
	srskipimage_checkBox->setChecked((tmp7 == 1));
	overlays_checkBox->setChecked((tmp8 == 1));
	mosaic_checkBox->setChecked((tmp9 == 1));
	if (tmp10 == 2)
	{
		enh_dim_strict_radioButton->setChecked(true);
		enh_dim_uniform_radioButton->setChecked(false); // not required
		enh_dim_strict_1_radioButton->setChecked(false); // not required
		enh_dim_skip_radioButton->setChecked(false); // not required
	}
	else if (tmp10 == 3)
	{
		enh_dim_strict_1_radioButton->setChecked(true);
		enh_dim_uniform_radioButton->setChecked(false); // not required
		enh_dim_skip_radioButton->setChecked(false); // not required
		enh_dim_strict_radioButton->setChecked(false); // not required
	}
	else if (tmp10 == 4)
	{
		enh_dim_skip_radioButton->setChecked(true);
		enh_dim_uniform_radioButton->setChecked(false); // not required
		enh_dim_strict_radioButton->setChecked(false); // not required
		enh_dim_strict_1_radioButton->setChecked(false); // not required
	}
	else // 1 is default
	{
		enh_dim_uniform_radioButton->setChecked(true);
		enh_dim_strict_radioButton->setChecked(false); // not required
		enh_dim_strict_1_radioButton->setChecked(false); // not required
		enh_dim_skip_radioButton->setChecked(false); // not required
	}
	icc_checkBox->setChecked((tmp11 == 1));
	clean_unused_checkBox->setChecked((tmp12 == 1));
	//
	const bool force_cp1241 = (tmp14 == 1);
	cp1251_checkBox->blockSignals(true);
	cp1251_checkBox->setChecked(force_cp1241);
	CodecUtils::set_force_cp1251(force_cp1241);
	cp1251_checkBox->blockSignals(false);
}

void SettingsWidget::writeSettings(QSettings & s)
{
	s.beginGroup(QString("GlobalSettings"));
	s.setValue(QString("enable_gl_3D"),  QVariant(gl3D_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("scale_ui_icons"),QVariant(si_doubleSpinBox->value()));
	s.setValue(QString("app_font_pt"),   QVariant(pt_doubleSpinBox->value()));
	s.setValue(QString("hide_zoom"),     QVariant(hidezoom_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("stylename"),     QVariant(styleComboBox->currentText().trimmed()));
	s.setValue(QString("sr_info2"),      QVariant(srinfo_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("sr_i_scale"),    QVariant(srscale_checkBox->isChecked() ? 1: 0));
	s.setValue(QString("sr_i_width"),    QVariant(srwidth_spinBox->value()));
	s.setValue(QString("sr_chapters"),   QVariant(srchapters_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("sr_skip_images"),QVariant(srskipimage_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("dcm_overlays"),  QVariant(overlays_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("dcm_mosaic"),    QVariant(mosaic_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("apply_icc"),     QVariant(icc_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("clean_unused"),  QVariant(clean_unused_checkBox->isChecked() ? 1 : 0));
	s.setValue(QString("force_cp1241"),  QVariant(cp1251_checkBox->isChecked() ? 1 : 0));
	if (enh_dim_skip_radioButton->isChecked())
	{
		s.setValue(QString("enh_strategy"), QVariant(4));
	}
	else if (enh_dim_strict_1_radioButton->isChecked())
	{
		s.setValue(QString("enh_strategy"), QVariant(3));
	}
	else if (enh_dim_strict_radioButton->isChecked())
	{
		s.setValue(QString("enh_strategy"), QVariant(2));
	}
	else // default
	{
		s.setValue(QString("enh_strategy"), QVariant(1));
	}
	s.endGroup();
	s.beginGroup(QString("StyleDialog"));
	s.setValue(QString("saved_idx"), QVariant(styleComboBox->currentIndex()));
	s.endGroup();
}

float SettingsWidget::get_scale_icons() const
{
	return (scale_icons * static_cast<float>(si_doubleSpinBox->value()));
}

void SettingsWidget::force_no_gl3()
{
	gl3D_checkBox->setChecked(false);
}

bool SettingsWidget::get_sr_info() const
{
	return srinfo_checkBox->isChecked();
}

int SettingsWidget::get_sr_image_width() const
{
	if (!srscale_checkBox->isChecked()) return 0;
	return srwidth_spinBox->value();
}

bool SettingsWidget::get_sr_chapters() const
{
	return srchapters_checkBox->isChecked();
}

bool SettingsWidget::get_sr_skip_images() const
{
	return srskipimage_checkBox->isChecked();
}

bool SettingsWidget::get_predictor_workaround() const
{
	return pred6_checkBox->isChecked();
}

bool SettingsWidget::get_cornell_workaround() const
{
	return cornell_checkBox->isChecked();
}

bool SettingsWidget::get_apply_icc() const
{
	return icc_checkBox->isChecked();
}

bool SettingsWidget::get_try_fix_jpeg_prec() const
{
	return jpegprec_checkBox->isChecked();
}

short SettingsWidget::get_enh_strategy() const
{
	if (enh_dim_skip_radioButton->isChecked())
	{
		return static_cast<short>(EnhancedIODLoadingType::SkipDimensionOrganization);
	}
	else if (enh_dim_strict_1_radioButton->isChecked())
	{
		return static_cast<short>(EnhancedIODLoadingType::StrictSingleImage);
	}
	else if (enh_dim_strict_radioButton->isChecked())
	{
		return static_cast<short>(EnhancedIODLoadingType::StrictMultipleImages);
	}
	else if (enh_dim_uniform_radioButton->isChecked())
	{
		return static_cast<short>(EnhancedIODLoadingType::PreferUniformVolumes);
	}
	return 0; // unreachable
}

