#include "structures.h"
#include "srwidget.h"
#include <QtGlobal>
#include <QTextDocument>
#include <QFile>
#include <QUrl>
#include <QSettings>
#include <QApplication>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocumentWriter>
#include <QFileDialog>
#include <QFileInfo>
#include <QPrintPreviewDialog>
#include "commonutils.h"

const QString css3 = QString(
	"span.red2 { color:#aa0505; font-size: small; font-weight: bold; }\n"
	"span.y    { color:#050505; }\n"
	"span.yy   { color:#050505; font-size: small; font-weight: bold; }\n"
	"span.y3   { color:#053005; font-size: small; font-style: italic; }\n"
	"span.t1   { color:#050505; font-size: large; font-weight: bold; }\n"
	"span.t2   { color:#050505; font-size: large; font-style: italic; }\n"
	"span.y9   { color:#0d0d76; font-weight: bold; font-style: italic; }"
	"span.yy9  { color:#0d0d76; font-size: small; font-weight: bold; font-style: italic; }");

SRWidget::SRWidget(float si, QWidget * p, Qt::WindowFlags f) : QWidget(p, f)
{
	setupUi(this);
#if 1
	backward_toolButton->hide();
	forward_toolButton->hide();
#endif
	const QSize s = QSize((int)(18*si),(int)(18*si));
	print_toolButton->setIconSize(s);
	toolButton->setIconSize(s);
	readSettings();
	textBrowser->document()->addResource(
		QTextDocument::StyleSheetResource,
		QUrl("format.css"),
		css3);
	connect(print_toolButton, SIGNAL(pressed()), this, SLOT(printSR()));
	connect(save_toolButton,  SIGNAL(pressed()), this, SLOT(saveSR()));
}

void SRWidget::initSR(const QString & s)
{
	// FIXME
#if 0
	backward_toolButton->setEnabled(false);
	forward_toolButton->setEnabled(false);
	connect(textBrowser, SIGNAL(backwardAvailable(bool)), backward_toolButton, SLOT(setEnabled(bool)));
	connect(textBrowser, SIGNAL(forwardAvailable(bool)),  forward_toolButton,  SLOT(setEnabled(bool)));
	connect(backward_toolButton, SIGNAL(pressed()), textBrowser, SLOT(backward()));
	connect(forward_toolButton,  SIGNAL(pressed()), textBrowser, SLOT(forward()));
#endif
	textBrowser->setHtml(s);
}

SRWidget::~SRWidget()
{
}

void SRWidget::closeEvent(QCloseEvent * e)
{
	writeSettings();
	if (textBrowser->document())
	{
		textBrowser->document()->clear();
	}
	textBrowser->clear();
	for (int k = 0; k < tmpfiles.size(); k++)
	{
		QFile::remove(tmpfiles.at(k));
	}
	tmpfiles.clear();
	for (size_t k = 0; k < srimages.size(); k++)
	{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
		srimages[k].i = QImage();
#endif
		if (srimages.at(k).p) delete [] (srimages[k].p);
	}
	srimages.clear();
	e->accept();
}

void SRWidget::readSettings()
{
	QSettings settings(
		QSettings::IniFormat,
		QSettings::UserScope,
		QApplication::organizationName(),
		QApplication::applicationName());
	settings.setFallbacksEnabled(false);
	settings.beginGroup(QString("SRWidget"));
	if (settings.contains(QString("size")))
		resize(settings.value(QString("size")).toSize());
	if (settings.contains(QString("pos")))
		move(settings.value(QString("pos")).toPoint());
	settings.endGroup();
}

void SRWidget::writeSettings()
{
	QSettings settings(
		QSettings::IniFormat,
		QSettings::UserScope,
		QApplication::organizationName(),
		QApplication::applicationName());
	settings.setFallbacksEnabled(false);
	settings.beginGroup(QString("SRWidget"));
	if (!isMaximized())
	{
		settings.setValue(QString("size"), QVariant(size()));
		settings.setValue(QString("pos"),  QVariant(pos()));
	}
	settings.endGroup();
}

void SRWidget::preview_print(QPrinter * p)
{
	textBrowser->print(p);
}

void SRWidget::printSR()
{
	QPrinter * p = new QPrinter();
	QPrintPreviewDialog * v = new QPrintPreviewDialog(p);
	connect(
		v,    SIGNAL(paintRequested(QPrinter*)),
		this, SLOT(preview_print(QPrinter*)));
	v->exec();
	delete v;
	delete p;
}

void SRWidget::saveSR()
{
	QString f = QFileDialog::getSaveFileName(
		this,
		QString("Save as Open Document ODT"),
		QDir::toNativeSeparators(
			CommonUtils::get_save_dir() +
			QDir::separator() +
			CommonUtils::get_save_name() +
			QString(".odt")),
		QString(
			"Open Document files (*.odt);;"
			"All Files (*);;"));
	if (f.isEmpty()) return;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	QFileInfo fi(f);
	CommonUtils::set_save_dir(fi.absolutePath());
	if (!f.endsWith(".odt", Qt::CaseInsensitive))
	{
		f += QString(".odt");
	}
	QTextDocumentWriter writer(f);
	writer.setFormat("ODT");
	writer.write(textBrowser->document());
	QApplication::restoreOverrideCursor();
}
