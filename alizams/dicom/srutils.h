#ifndef SRUTILS_H
#define SRUTILS_H

#include <mdcmDataSet.h>
#include <QString>
#include <QWidget>
#include <QTextBrowser>
#include <QProgressDialog>
#include <QStringList>
#include <vector>

class SRImage;

class SRGraphic
{
public:
	SRGraphic()  {}
	~SRGraphic() {}
	QString GraphicType;
	QString PixelOriginInterpretation;
	QString FiducialUID;
	std::vector<float> GraphicData;
};

class SRUtils
{
public:
	SRUtils()  {}
	~SRUtils() {}
	static void set_asked_for_path_once(bool);
	static void read_IMAGE(
		const mdcm::DataSet&,
		const QString&,
		const QString&,
		QString&,
		QStringList&,
		std::vector<SRImage>&,
		QTextBrowser*,
		const std::vector<SRGraphic>&,
		bool,
		const QWidget*,
		QProgressDialog*);
	static bool read_SCOORD(
		const mdcm::DataSet&,
		const QString&,
		const QString&,
		QString&,
		QStringList&,
		std::vector<SRImage>&,
		QTextBrowser*,
		bool,
		const QWidget*,
		QProgressDialog*);
	static void    read_PNAME(const mdcm::DataSet&,const QString&,QString&);
	static void    read_TEXT (const mdcm::DataSet&,const QString&,QString&);
	static void    read_NUM  (const mdcm::DataSet&,const QString&,QString&);
	static void    read_CODE (const mdcm::DataSet&,const QString&,QString&);
	static void    read_DATE (const mdcm::DataSet&, QString&);
	static void    read_DATETIME(const mdcm::DataSet&, QString&);
	static void    read_TIME    (const mdcm::DataSet&, QString&);
	static void    read_SCOORD3D(const mdcm::DataSet&, QString&);
	static QString read_UIDREF  (const mdcm::DataSet&, QString&);
	static QString read_sr_title1(const mdcm::DataSet&, const QString&);
	static QString read_sr_title2(const mdcm::DataSet&, const QString&);
	static QStringList read_referenced(const mdcm::DataSet&, QString&);
	static QString get_concept_code_meaning(
		const mdcm::DataSet&,
		const QString&);
	static QString read_sr_content_sq(
		const mdcm::DataSet&,
		const QString&,
		const QString&,
		const QWidget*,
		QTextBrowser*,
		QProgressDialog*,
		QStringList&,
		std::vector<SRImage>&,
		int,
		bool,
		const QString&,
		bool=false);
};

#endif // SRUTILS_H
