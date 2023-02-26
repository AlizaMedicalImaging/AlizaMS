#include "aboutwidget.h"
#include <QStyle>
#include <QDate>
#include <QPoint>
#include "alizams_version.h"
#include <mdcmVersion.h>
#include <itkVersion.h>

AboutWidget::AboutWidget(int w, int h)
{
	setWindowFlags(Qt::FramelessWindowHint);
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
	const QString link1 = QString("Aliza MS ") + aliza_version + QString("</a>");
	const QString link2 = QString("</h3></body></html>");
	link_label->setText(link0 + link1 + link2);
	link_label->setOpenExternalLinks(true);
	//
	adjustSize();
	move(QPoint((w / 2) - (width() / 2), (h / 2) - (height() / 2)));
}

AboutWidget::~AboutWidget()
{
}

void AboutWidget::mouseReleaseEvent(QMouseEvent * e)
{
	close();
}

void AboutWidget::set_opengl_info(const QString & s)
{
	opengl_info = s;
}

void AboutWidget::set_info()
{
	info_label->setText(
#if 1
		get_build_info() +
		QString("\n") +
#else
		QString("OpenGL\n") +
#endif
		opengl_info);
}

QString AboutWidget::get_build_info()
{
	QString s;
#if 0
	s.append(QVariant(__DATE__).toString());
	s.append(QString(" "));
#endif
#if (defined _MSC_VER)
	s.append(QString("Visual Studio C++ "));
	s.append(QVariant(_MSC_VER).toString());
#elif (defined __VERSION__)
#ifndef __APPLE__
	s.append(QString("GCC "));
#endif
	s.append(QVariant(__VERSION__).toString());
#endif
#ifdef __APPLE__
#if defined __arm64__
	s.append("\narm64");
#elif defined __x86_64__
	s.append("\nx86_64");
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

