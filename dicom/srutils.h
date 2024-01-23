#ifndef A_SRUTILS_H
#define A_SRUTILS_H

#include <mdcmDataSet.h>
#include <QString>
#include <QWidget>
#include <QTextBrowser>
#include <QProgressDialog>
#include <QStringList>
#include <QImage>
#include <vector>

class SRImage
{
public:
	SRImage() = default;
	~SRImage() = default;
	SRImage & operator=(const SRImage & j)
	{
		sx = j.sx;
		sy = j.sy;
		p  = j.p;
		i  = j.i;
		return *this;
	}
	SRImage & operator=(SRImage & j)
	{
		sx = j.sx;
		sy = j.sy;
		p  = j.p;
		i  = j.i;
		return *this;
	}
	double sx{1.0};
	double sy{1.0};
	unsigned char * p{};
	QImage i;
};

class SRGraphic
{
public:
	QString GraphicType;
	QString PixelOriginInterpretation;
	QString FiducialUID;
	std::vector<float> GraphicData;
};

class SRUtils
{
public:
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
	static void    read_TCOORD  (const mdcm::DataSet&, QString&, bool);
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
		bool = false);
};

#endif

