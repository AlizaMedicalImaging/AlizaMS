#ifndef A_LOADDICOM_H
#define A_LOADDICOM_H

#include <QThread>
#include <QWidget>
#include <QString>
#include <QStringList>
#include <vector>

class ImageVariant;

class LoadDicom : public QThread
{
Q_OBJECT

public:
	LoadDicom(
		const QString root_,
		const QStringList & filenames_,
		const bool ok3d_,
		const QWidget * const settingswidget_,
		const short load_type_,
		const short enh_type_)
		:
		root(root_),
		filenames(filenames_),
		ok3d(ok3d_),
		settingswidget(settingswidget_),
		load_type(load_type_),
		enh_type(enh_type_) {}
	virtual ~LoadDicom() {}
	void run();
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
	const QWidget * const settingswidget;
	const short load_type;
	const short enh_type;
};

#endif

