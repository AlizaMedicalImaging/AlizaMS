#define USE_SPLASH_SCREEN 0
#define LOG_STDOUT_TO_FILE 0
#define PRINT_HOST_INFO 0
#define SILENCE_QT_LOGGING
#define TMP_USE_GL_TEST

#include "alizams_version.h"
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt5-core.h"
#else
#include "CG/glwidget-qt5.h"
#endif
#include <QSurfaceFormat>
#else
#include "CG/glew/include/GL/glew.h"
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt4-core.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#endif
#ifdef TMP_USE_GL_TEST
#include "CG/testgl.h"
#endif
#include "GUI/mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QStyle>
#include <QPalette>
#include <QMessageBox>
#include <QDir>
#include <QFont>
#include <QProcess>
#include <QObject>
#include <cstdlib>
#include <iostream>
#include "browser/sqtree.h"

#if (defined(SILENCE_QT_LOGGING) && QT_VERSION >= QT_VERSION_CHECK(5,2,0))
#include <QLoggingCategory>
#endif

#if (defined LOG_STDOUT_TO_FILE && LOG_STDOUT_TO_FILE==1)
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
#include <QMessageLogContext>
#include <QDebug>
#endif
#ifdef _WIN32
#include "Windows.h"
#endif
#endif

#if (defined USE_SPLASH_SCREEN && USE_SPLASH_SCREEN==1)
#include <QLocale>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#endif

#ifdef _WIN32
#include <itkTextOutput.h>
#endif

#if (defined PRINT_HOST_INFO && PRINT_HOST_INFO==1)
#include <climits>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <new>
#endif

namespace
{

static_assert(
	sizeof(char) == 1 &&
	sizeof(short) == 2 &&
	sizeof(int) == 4 &&
	sizeof(long long) == 8 &&
	sizeof(float) == 4 &&
	sizeof(double) == 8, "");

#if (defined LOG_STDOUT_TO_FILE && LOG_STDOUT_TO_FILE==1)
void close_log()
{
	std::cout
		<< "closing logfile \"log.txt\"..."
		<< std::endl;
	fflush(stdout);
	fclose(stdout);
}

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
void redirect_qdebug(
	QtMsgType type,
	const QMessageLogContext & context,
	const QString & msg)
{
	switch (type)
	{
	case QtDebugMsg:
	case QtWarningMsg:
	case QtCriticalMsg:
		std::cout << msg.toStdString() << std::endl;
		break;
	case QtFatalMsg:
		std::cout << msg.toStdString() << std::endl;
		abort();
	default:
		break;
	}
}
#endif
#endif

#if (defined(SILENCE_QT_LOGGING) && QT_VERSION < QT_VERSION_CHECK(5,2,0))
void myMessageOutput(QtMsgType type, const char * msg)
{
	switch (type)
	{
	case QtDebugMsg:
	case QtWarningMsg:
	case QtCriticalMsg:
		break;
	case QtFatalMsg:
		std::cout << "Fatal: " << msg << std::endl;
		abort();
	}
}
#endif

}

int main(int argc, char * argv[])
{
#ifdef FORCE_PLATFORM_XCB
#ifndef _WIN32
#ifndef __APPLE__
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
	setenv("QT_QPA_PLATFORM", "xcb", 1);
#endif
#endif
#endif
#endif
	bool force_disable_opengl{};
	bool metadata_only{};
	bool metadata_series_only{};
	int count{1};
	bool ok3d{};
	bool hide_zoom{};
	while (count < argc)
	{
		if (!strcmp(argv[count], "-nogl"))
		{
			force_disable_opengl = true;
		}
		else if (!strcmp(argv[count], "--nogl"))
		{
			force_disable_opengl = true;
		}
		else if (!strcmp(argv[count], "-s"))
		{
			metadata_series_only = true;
			force_disable_opengl = true;
			break;
		}
		else if (!strcmp(argv[count], "--s"))
		{
			metadata_series_only = true;
			force_disable_opengl = true;
			break;
		}
		else if (!strcmp(argv[count], "-m"))
		{
			metadata_only = true;
			force_disable_opengl = true;
			break;
		}
		else if (!strcmp(argv[count], "--m"))
		{
			metadata_only = true;
			force_disable_opengl = true;
			break;
		}
		count += 1;
	}
	//
#if (defined LOG_STDOUT_TO_FILE && LOG_STDOUT_TO_FILE==1)
	if (freopen("log.txt", "w", stdout))
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
		qInstallMessageHandler(redirect_qdebug);
#endif
		atexit(close_log);
	}
#endif
	//
// clang-format off
#if (defined PRINT_HOST_INFO && PRINT_HOST_INFO==1)
	{
		const unsigned int endian = 1;
		if (*((const unsigned char*)&endian) == 0)
		{
			std::cout << "System is big-endian\n\n";
		}
		else
		{
			std::cout << "System is little-endian\n\n";
		}
#ifdef __cplusplus
		std::cout << "__cplusplus is defined: " << __cplusplus << "\n\n";
#else
		std::cout << "__cplusplus is not defined\n\n";
#endif
#ifdef __x86_64__
		std::cout << "__x86_64__ is defined\n";
#endif
#ifdef __x86_64
		std::cout << "__x86_64 is defined\n";
#endif
#ifdef __amd64__
		std::cout << "__amd64__ is defined\n";
#endif
#ifdef __amd64
		std::cout << "__amd64 is defined\n";
#endif
#ifdef __arm__
		std::cout << "__arm__ is defined\n";
#endif
#ifdef __arm64__
		std::cout << "__arm64__ is defined\n";
#endif
#ifdef __aarch64__
		std::cout << "__aarch64__ is defined\n";
#endif
#ifdef _M_AMD64
		std::cout << "_M_AMD64 is defined\n";
#endif
#ifdef _M_X64
		std::cout << "_M_X64 is defined\n";
#endif
#ifdef _M_IX86_FP
		std::cout << "_M_IX86_FP is defined:" << _M_IX86_FP << '\n'
#endif
#ifdef _M_ARM
		std::cout << "_M_ARM is defined\n";
#endif
#ifdef _M_ARM64EC
		std::cout << "_M_ARM64EC is defined\n";
#endif
#if 1
		std::cout << "\nalignof(std::max_align_t) = " << alignof(std::max_align_t)
			<< "\n";
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
		std::cout << "__STDCPP_DEFAULT_NEW_ALIGNMENT__ = "
			<< __STDCPP_DEFAULT_NEW_ALIGNMENT__ << '\n';
#else
		std::cout << "__STDCPP_DEFAULT_NEW_ALIGNMENT__ is not defined\n";
#endif
#endif
#ifdef __cpp_lib_hardware_interference_size
		std::cout << "\n__cpp_lib_hardware_interference_size is defined, "
			<< std::hardware_constructive_interference_size << '\n';
#else
		std::cout << "\n__cpp_lib_hardware_interference_size is not defined\n";
#endif
#ifdef _WIN32
#ifdef UNICODE
		std::cout << "UNICODE is defined\n";
#else
		std::cout << "UNICODE is not defined\n";
#endif
#if 1
		UINT acp = GetACP();
		if (activeCodePage == 65001)
		{
			std::cout << "Active code page is UTF-8\n";
		}
		else
		{
			std::cout << "Active code page: " << acp << '\n';
		}
#endif
#endif
		std::cout << "\nchar is " << (std::is_signed<char>() ? "signed\n" : "unsigned\n");
		enum { Va = (short)SHRT_MAX                } Ea;
		enum { V0 = (unsigned short)USHRT_MAX      } E0;
		enum { V1 = (int)INT_MAX                   } E1;
		enum { V2 = (unsigned int)UINT_MAX         } E2;
		enum { V3 = (long long)LLONG_MAX           } E3;
		enum { V4 = (unsigned long long)ULLONG_MAX } E4;
		enum           { V5 = 1                    } E5;
		enum:short     { V6 = 1                    } E6;
		enum:int       { V7 = 1                    } E7;
		enum:long      { V8 = 1                    } E8;
		enum:long long { V9 = 1                    } E9;
		std::cout
			<< "\nsize of void*       = "          << sizeof(void*)
			<< "\nsize of char*       = "          << sizeof(char*)
			<< "\nsize of char        = "          << sizeof(char)
			<< "\nsize of wchar_t     = "          << sizeof(wchar_t)
			<< "\nsize of bool        = "          << sizeof(bool)
			<< "\nsize of short       = "          << sizeof(short)
			<< "\nsize of int         = "          << sizeof(int)
			<< "\nsize of long        = "          << sizeof(long)
			<< "\nsize of long long   = "          << sizeof(long long)
			<< "\nsize of size_t      = "          << sizeof(size_t)
			<< "\nsize of off_t       = "          << sizeof(off_t)
			<< "\nsize of uintptr_t   = "          << sizeof(uintptr_t)
			<< "\nsize of float       = "          << sizeof(float)
			<< "\nsize of double      = "          << sizeof(double)
			<< "\nsize of long double = "          << sizeof(long double)
			<< "\nsize of enum { SHRT_MAX    } = " << sizeof(Ea)
			<< "\nsize of enum { USHRT_MAX   } = " << sizeof(E0)
			<< "\nsize of enum { INT_MAX     } = " << sizeof(E1)
			<< "\nsize of enum { UINT_MAX    } = " << sizeof(E2)
			<< "\nsize of enum { LLONG_MAX   } = " << sizeof(E3)
			<< "\nsize of enum { ULLONG_MAX  } = " << sizeof(E4)
			<< "\nsize of enum           { 1 } = " << sizeof(E5)
			<< "\nsize of enum:short     { 1 } = " << sizeof(E6)
			<< "\nsize of enum:int       { 1 } = " << sizeof(E7)
			<< "\nsize of enum:long      { 1 } = " << sizeof(E8)
			<< "\nsize of enum:long long { 1 } = " << sizeof(E9)
			<< std::endl;
	}
#endif

// clang-format on
#if (defined(SILENCE_QT_LOGGING) && (QT_VERSION < QT_VERSION_CHECK(5,2,0)))
	qInstallMsgHandler(myMessageOutput);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	if (!force_disable_opengl)
	{
		QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#ifdef USE_SET_DEFAULT_GL_FORMAT
		QSurfaceFormat format;
		format.setRenderableType(QSurfaceFormat::OpenGL);
#ifdef ALIZA_GL_3_2_CORE
#ifdef USE_GL_MAJOR_3_MINOR_2
		format.setVersion(3, 2); // may be required sometimes
#endif
		format.setProfile(QSurfaceFormat::CoreProfile);
#endif
#if 0
		format.setRedBufferSize(8);
		format.setGreenBufferSize(8);
		format.setBlueBufferSize(8);
		format.setAlphaBufferSize(8);
		format.setDepthBufferSize(24);
		//format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
		//format.setSwapInterval(0);
		//format.setSamples(4);
#endif
		QSurfaceFormat::setDefaultFormat(format);
#endif
	}
#endif
	QApplication app(argc, argv);
#if 1
	app.setQuitOnLastWindowClosed(false);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	{
		// Have to set some stuff related to OpenGL before the QApplication is initialized,
		// but can check platform only after (and maybe disable OpenGL for VNC).
		// But there seems to be no negative effect either.
		const QString platform_name = QApplication::platformName().trimmed().remove(QChar('\0'));
#if (defined PRINT_HOST_INFO && PRINT_HOST_INFO==1)
		std::cout << "\nQt platform: " << platform_name.toStdString() <<
			"\nLocale: " << QLocale::system().name().toStdString() << std::endl;
#endif
		// From this list the only useful one on a usual computer is VNC.
		if (platform_name == QString("vnc")      ||
			platform_name == QString("linuxfb")  ||
			platform_name == QString("directfb") ||
			platform_name == QString("eglfs")) // EGLFS probably doesn't work with OpenGL 3
		{
			force_disable_opengl = true;
		}
	}
#endif
	app.setOrganizationName(QString("Aliza"));
	app.setOrganizationDomain(QString("aliza-dicom-viewer.com"));
	app.setApplicationName(QString("AlizaMS"));
#if (defined(SILENCE_QT_LOGGING) && QT_VERSION >= QT_VERSION_CHECK(5,2,0))
	QLoggingCategory::setFilterRules(QString("*.debug=false\n*.warning=false\n*.info=false"));
#endif
	//
	{
		double app_font_pt{};
		QSettings settings(
			QSettings::IniFormat,
			QSettings::UserScope,
			app.organizationName(),
			app.applicationName());
		settings.setFallbacksEnabled(true);
		settings.beginGroup(QString("GlobalSettings"));
		app_font_pt = settings.value(QString("app_font_pt"), 0.0).toDouble();
		QString saved_style = settings.value(
			QString("stylename"),
			QVariant(QString("Dark Fusion"))).toString();
#ifdef TMP_USE_GL_TEST
		const int enable_gltest = settings.value(QString("enable_gltest"), 1).toInt();
#endif
		int enable_gl_3d_ = settings.value(QString("enable_gl_3D"), 1).toInt();
		const int hide_zoom_ = settings.value(QString("hide_zoom"), 1).toInt();
		settings.endGroup();
		hide_zoom = hide_zoom_ == 1;
		QFont f = QApplication::font();
		if (app_font_pt <= 0.0)
		{
			app_font_pt = f.pointSizeF();
		}
		else
		{
			if (app_font_pt < 6.0) app_font_pt = 6.0;
		}
		f.setPointSizeF(app_font_pt);
		app.setFont(f);
		settings.beginGroup(QString("GlobalSettings"));
		settings.setValue(QString("app_font_pt"), QVariant(app_font_pt));
		settings.endGroup();
		settings.sync();
		//
		if (!force_disable_opengl)
		{
#ifdef TMP_USE_GL_TEST
			if (enable_gl_3d_ == 1 && enable_gltest == 1)
			{
				TestGL * testgl = new TestGL();
				testgl->show();
				if (testgl->isValid())
				{
					ok3d = (testgl->opengl_init_done && !testgl->no_opengl3);
					testgl->hide();
					testgl->doneCurrent();
				}
				if (ok3d)
				{
					settings.beginGroup(QString("GlobalSettings"));
					settings.setValue(QString("enable_gltest"), QVariant(0));
					settings.endGroup();
					settings.sync();
					delete testgl;
				}
				else
				{
					settings.beginGroup(QString("GlobalSettings"));
					settings.setValue(QString("enable_gl_3D"), QVariant(0));
					settings.endGroup();
					settings.sync();
					std::cout << "Aliza MS: OpenGL 3 is not available" << std::endl;
					delete testgl;
					//
					const QStringList aa_ = QApplication::arguments();
					QStringList aa;
					aa.push_back(QString("--nogl"));
					for (int y = 1; y < aa_.size(); ++y)
					{
						aa.push_back(aa_.at(y));
					}
					QProcess::startDetached(aa_.at(0), aa);
					exit(0);
				}
			}
			else
#endif
			{
				if (enable_gl_3d_ == 1) ok3d = true;
			}
		}
		//
		if (!saved_style.isEmpty())
		{
			if (saved_style == QString("Dark Fusion"))
			{
				QColor bg(0x53, 0x59, 0x60);
				QColor tt(0x30, 0x39, 0x47);
				QPalette p;
				p.setColor(QPalette::Window,          bg);
				p.setColor(QPalette::WindowText,      Qt::white);
				p.setColor(QPalette::Text,            Qt::white);
				p.setColor(QPalette::Disabled,        QPalette::WindowText, Qt::gray);
				p.setColor(QPalette::Disabled,        QPalette::Text,       Qt::gray);
				p.setColor(QPalette::Base,            bg);
				p.setColor(QPalette::AlternateBase,   bg);
				p.setColor(QPalette::ToolTipBase,     tt);
				p.setColor(QPalette::ToolTipText,     Qt::white);
				p.setColor(QPalette::Button,          bg);
				p.setColor(QPalette::ButtonText,      Qt::white);
				p.setColor(QPalette::BrightText,      Qt::white);
				p.setColor(QPalette::Link,            Qt::darkBlue);
				p.setColor(QPalette::Highlight,       Qt::lightGray);
				p.setColor(QPalette::HighlightedText, Qt::black);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
				app.setStyle(QString("Fusion"));
#else
				app.setStyle(QString("Plastique"));
#endif
				app.setPalette(p);
			}
			else
			{
				app.setStyle(saved_style);
				if (!(
					saved_style.toUpper() == QString("WINDOWSVISTA") ||
					saved_style.toUpper() == QString("MACOS")))
				{
					app.setPalette(app.style()->standardPalette());
				}
			}
		}
	}
	//
#if (defined USE_SPLASH_SCREEN && USE_SPLASH_SCREEN==1)
	const QString splash_info =
		QString("\n        Aliza MS ") +
		QString::fromLatin1(
			QVariant(ALIZAMS_VERSION).toByteArray().constData()) +
		QString("\n\n");
	QSplashScreen * splash = new QSplashScreen(QPixmap(":/bitmaps/empty_splash.png"));
	splash->setWindowFlags(Qt::SplashScreen|Qt::WindowStaysOnTopHint);
	splash->setAttribute(Qt::WA_DeleteOnClose);
	splash->show();
	splash->showMessage(splash_info + QString("  Loading ..."));
#endif
	//
#ifdef _WIN32
	itk::OutputWindow::SetInstance(itk::TextOutput::New());
#endif
	//
	if (metadata_only || metadata_series_only)
	{
#if 1
		app.setQuitOnLastWindowClosed(true);
#endif
#if (defined USE_SPLASH_SCREEN && USE_SPLASH_SCREEN==1)
		QTimer::singleShot(500, splash, SLOT(close()));
#endif
		QStringList l;
		bool skip_next{};
		for (int x = 1; x < argc; ++x)
		{
			if (!skip_next)
			{
				const QString f = QString::fromLocal8Bit(argv[x]);
				if (f == QString("-platform")    ||
					f == QString("--platform")   ||
					f == QString("-style")       ||
					f == QString("--style")      ||
					f == QString("-stylesheet")  ||
					f == QString("--stylesheet"))
				{
					skip_next = true;
				}
				else if (!f.startsWith(QString("-")))
				{
					l.push_back(f);
					skip_next = false;
				}
				else
				{
					skip_next = false;
				}
			}
		}
		if (!l.empty())
		{
			for (int x = 0; x < l.size(); ++x)
			{
				if (x > 4)
				{
					QMessageBox::information(
						nullptr,
						QString("Warning"),
						QString(
							"Max 4 Metadata Viewers,\n"
							"use -s to open and scan series"));
					break;
				}
				SQtree * sqtree = new SQtree(false);
				sqtree->setAttribute(Qt::WA_DeleteOnClose);
				sqtree->show();
				if (metadata_series_only)
				{
					sqtree->read_file_and_series(l.at(x), true);
				}
				else if (metadata_only)
				{
					sqtree->read_file(l.at(x), true);
				}
			}
		}
		else
		{
			SQtree * sqtree = new SQtree(false);
			sqtree->setAttribute(Qt::WA_DeleteOnClose);
			sqtree->show();
		}
		return app.exec();
	}
	else
	{
		MainWindow mainWin(ok3d, hide_zoom);
		//
		QObject::connect(&mainWin, SIGNAL(quit_app()), &mainWin, SLOT(exit_app()), Qt::QueuedConnection);
		//
		mainWin.show();
		mainWin.check_3d_frame();
#if (defined USE_SPLASH_SCREEN && USE_SPLASH_SCREEN==1)
		QTimer::singleShot(1500, splash, SLOT(close()));
#endif
		if (argc > 1)
		{
			QStringList l;
			for (int x = 1; x < argc; ++x)
				l.push_back(QString::fromLocal8Bit(argv[x]));
			mainWin.open_args(l);
		}
		return app.exec();
	}
}

#undef USE_SPLASH_SCREEN
#undef LOG_STDOUT_TO_FILE
#undef PRINT_HOST_INFO
#ifdef TMP_USE_GL_TEST
#undef TMP_USE_GL_TEST
#endif
#ifdef SILENCE_QT_LOGGING
#undef SILENCE_QT_LOGGING
#endif
