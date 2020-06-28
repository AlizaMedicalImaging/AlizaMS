#include "aboutwidget.h"
#include <QStyle>
#include <QDate>
#include "alizams_version.h"
#include "mdcmVersion.h"
#include "itkVersion.h"
#ifdef __GNUC__
#include <cpuid.h>
#endif

AboutWidget::AboutWidget(QDialog * p, Qt::WindowFlags f) : QDialog(p,f)
{
	setupUi(this);
	setStyleSheet(
		"QWidget { background-color : rgba(255,255,255,255);"
		" color: black; }");
	//
	const QString aliza_version =
		QString::fromLatin1(
			QVariant(ALIZAMS_VERSION).toByteArray().constData());
	const QString link0 =
		QString(
			"<html><body><h3><a href=\"http://www.aliza-dicom-viewer.com\" style=\"color: #00674d;\""
			" target=\"_blank\">");
	const QString link1 = QString("Aliza MS ")+aliza_version+QString("</a>");
	const QString link2 = QString("</h3></body></html>");
	link_label->setText(link0+link1+link2);
	link_label->setOpenExternalLinks(true);
	//
	adjustSize();
}

AboutWidget::~AboutWidget()
{
}

void AboutWidget::set_opengl_info(const QString & s)
{
	opengl_info = s;
}

void AboutWidget::set_info()
{
	info_label->setText(
		get_build_info() + QString("\n\n") +
		opengl_info);
}

QString AboutWidget::get_build_info()
{
	QString s;
	s.append(QVariant(__DATE__).toString());
	s.append(QString(" "));
#if (defined _MSC_VER)
	s.append(QString("Visual Studio C++ "));
	s.append(QVariant(_MSC_VER).toString());
#elif (defined __GNUC__)
	s.append(QString("GCC "));
	s.append(QVariant(__VERSION__).toString());
#endif
#ifdef _WIN32
#ifdef _WIN64
	s.append(QString(" 64 Bit"));
#else
	s.append(QString(" 32 Bit"));
#endif
#endif
#if 0
	s.append(QString("\nMDCM ") +
		QString::fromLatin1(mdcm::Version::GetVersion()));
#endif
#if 0
	s.append(QString("\nITK ") +
		QString::fromLatin1(itk::Version::GetITKVersion()));
#endif
#if 0
	s.append(QString("\nQt ") + QVariant(QT_VERSION_STR).toString());
#endif
	return s;
}

