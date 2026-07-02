#ifndef A_LOADDICOM_T_H
#define A_LOADDICOM_T_H

#include "structures.h"
#include <QThread>
#include <QString>
#include <QStringList>
#include <vector>

class ImageVariant;

class LoadDicom_T : public QThread
{
Q_OBJECT

public:
	LoadDicom_T(
		const QString root_,
		const QStringList & filenames_,
		const bool ok3d_,
		const CurrentSettings & settings_,
		const short load_type_,
		const short enh_type_)
		:
		root(root_),
		filenames(filenames_),
		ok3d(ok3d_),
		settings(settings_),
		load_type(load_type_),
		enh_type(enh_type_) {}
	~LoadDicom_T() = default;
	void run() override;
	QString message;
	std::vector<ImageVariant*> ivariants;
	QStringList pdf_files;
	QStringList stl_files;
	QStringList video_files;
	QStringList spectroscopy_files;
	QStringList sr_files;

private:
	const QString root;
	const QStringList filenames;
	const bool ok3d;
	const CurrentSettings settings;
	const short load_type;
	const short enh_type;
};

#endif

