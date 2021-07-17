//
#include "browserwidget2.h"
#include <QtGlobal>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QProgressDialog>
#include <QDate>
#include <QUrl>
#include <QMimeData>
#include <QTextCodec>
#include <QByteArray>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QFileInfo>
#include <QMap>
#include <QVector>
#include <QDir>
#include <QApplication>
#ifdef USE_WORKSTATION_MODE
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include "ctkdialog.h"
#endif
#include "mdcmReader.h"
#include "mdcmScanner.h"
#include "mdcmAttribute.h"
#include "mdcmMediaStorage.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmGlobal.h"
#include "mdcmDicts.h"
#include "mdcmParseException.h"
#include "codecutils.h"
#include "dicomutils.h"
#include <vector>
#include <string>
#include <exception>

const mdcm::Tag tOffsetOfTheFirstDirectoryRecordOfTheRootDirectoryEntity(0x0004,0x1200);
const mdcm::Tag tDirectoryRecordSequence                    (0x0004,0x1220);
const mdcm::Tag tOffsetOfTheNextDirectoryRecord             (0x0004,0x1400);
const mdcm::Tag tOffsetOfReferencedLowerLevelDirectoryEntity(0x0004,0x1420);
const mdcm::Tag tDirectoryRecordType                        (0x0004,0x1430);
const mdcm::Tag tReferencedFileID                           (0x0004,0x1500);
const mdcm::Tag tSpecificCharacterSet                       (0x0008,0x0005);
const mdcm::Tag tSOPClassUID                                (0x0008,0x0016);
const mdcm::Tag tStudyDate                                  (0x0008,0x0020);
const mdcm::Tag tSeriesDate                                 (0x0008,0x0021);
const mdcm::Tag tModality                                   (0x0008,0x0060);
const mdcm::Tag tStudyDescription                           (0x0008,0x1030);
const mdcm::Tag tSeriesDescription                          (0x0008,0x103e);
const mdcm::Tag tPatientsName                               (0x0010,0x0010);
const mdcm::Tag tPatientsBirthDate                          (0x0010,0x0030);
const mdcm::Tag tSeriesInstanceUID                          (0x0020,0x000e);
const mdcm::Tag tRows                                       (0x0028,0x0010);
const mdcm::Tag tColumns                                    (0x0028,0x0011);
const mdcm::Tag tBitsAllocated                              (0x0028,0x0100);
const mdcm::Tag tPixelRepresentation                        (0x0028,0x0103);

mdcm::VL BrowserWidget2::compute_offset0(const mdcm::DataSet & ds)
{
	mdcm::VL len = 0;
	mdcm::DataSet::ConstIterator it = ds.Begin();
	for(; it != ds.End() && it->GetTag() != tDirectoryRecordSequence; ++it)
	{
		const mdcm::DataElement &de = *it;
		len += de.GetLength<mdcm::ExplicitDataElement>();
	}
	// Tag length: 2 bytes (group number) + 2 bytes (element number)
	len += 4;
	// http://dicom.nema.org/medical/dicom/current/output/html/part05.html#table_7.1-1
	// http://dicom.nema.org/medical/dicom/current/output/html/part05.html#table_7.1-2
	// VL field (for Explicit) has same size as VR field
	// in particular case (0x0004,0x1220 SQ) it is 4 bytes (VR field) + 4 bytes (VL field)
	len += it->GetVR().GetLength();
	len += it->GetVR().GetLength();
	return len;
}

void BrowserWidget2::compute_offsets(const mdcm::SequenceOfItems * sq, mdcm::VL start, std::vector<unsigned int> & offsets)
{
	const unsigned int n = sq->GetNumberOfItems();
	offsets.resize(n);
	offsets[0] = static_cast<unsigned int>(start);
	for(unsigned int i = 1; i < n; ++i)
	{
		const mdcm::Item & item = sq->GetItem(i);
		offsets[i] = offsets[i-1] + static_cast<unsigned int>(item.GetLength<mdcm::ExplicitDataElement>());
	}
}

BrowserWidget2::BrowserWidget2(float si)
{
	once = false;
	eye_icon  = QIcon(QString(":/bitmaps/eye.svg"));
	eye2_icon = QIcon(QString(":/bitmaps/eye2.svg"));
	setupUi(this);
#if 0
	ctk_pushButton->hide();
#endif
	const QSize s = QSize((int)(24*si),(int)(24*si));
	opendir1_pushButton->setIconSize(s);
	dicomdir_pushButton->setIconSize(s);
	ctk_pushButton->setIconSize(s);
	reload_pushButton->setIconSize(s);
	meta_pushButton->setIconSize(s);
	copy_pushButton->setIconSize(s);
	load_pushButton->setIconSize(s);
	//
	tableWidget->hideColumn(0);
	tableWidget->setColumnWidth(1, 24);
	tableWidget->setColumnWidth(3, 160);
	tableWidget->setColumnWidth(5, 200);
	tableWidget->setColumnWidth(7, 200);
	//
	selected_tags.insert(mdcm::Tag(0x0008,0x0005));
	selected_tags.insert(mdcm::Tag(0x0008,0x0016));
	selected_tags.insert(mdcm::Tag(0x0008,0x0020));
	selected_tags.insert(mdcm::Tag(0x0008,0x0021));
	selected_tags.insert(mdcm::Tag(0x0008,0x0060));
	selected_tags.insert(mdcm::Tag(0x0008,0x1030));
	selected_tags.insert(mdcm::Tag(0x0008,0x103e));
	selected_tags.insert(mdcm::Tag(0x0010,0x0010));
	selected_tags.insert(mdcm::Tag(0x0010,0x0030));
	selected_tags.insert(mdcm::Tag(0x0028,0x0010));
	selected_tags.insert(mdcm::Tag(0x0028,0x0011));
	selected_tags.insert(mdcm::Tag(0x0028,0x0100));
	selected_tags.insert(mdcm::Tag(0x0028,0x0103));
	selected_tags_short.insert(mdcm::Tag(0x0008,0x0016));
	selected_tags_short.insert(mdcm::Tag(0x0028,0x0010));
	selected_tags_short.insert(mdcm::Tag(0x0028,0x0011));
	selected_tags_short.insert(mdcm::Tag(0x0028,0x0100));
	selected_tags_short.insert(mdcm::Tag(0x0028,0x0103));
	//
	readSettings();
	//
	connect(opendir1_pushButton, SIGNAL(clicked()), this, SLOT(open_dicom_dir()));
	connect(dicomdir_pushButton, SIGNAL(clicked()), this, SLOT(open_DICOMDIR()));
	connect(reload_pushButton,   SIGNAL(clicked()), this, SLOT(reload_dir()));
	connect(copy_pushButton,     SIGNAL(clicked()), this, SLOT(copy_files()));
#ifdef USE_WORKSTATION_MODE
	connect(ctk_pushButton,      SIGNAL(clicked()), this, SLOT(open_CTK_db()));
#else
	ctk_pushButton->hide();
#endif
	refresh_sc = new QShortcut(QKeySequence::Refresh, this, SLOT(reload_dir()));
	refresh_sc->setAutoRepeat(false);
}

BrowserWidget2::~BrowserWidget2()
{
}

void BrowserWidget2::closeEvent(QCloseEvent * e)
{
	e->accept();
}

void BrowserWidget2::read_directory(const QString & p)
{
	if (!once) once = true;
	tableWidget->clearContents();
	tableWidget->setRowCount(0);
	qApp->processEvents();
	QProgressDialog * pd =
		new QProgressDialog(
			QString("Recursive scan"),QString("Stop"),0,0);
	pd->setWindowModality(Qt::ApplicationModal);
	pd->setWindowFlags(
		pd->windowFlags()^Qt::WindowContextHelpButtonHint);
	pd->show();
	try
	{
		const mdcm::Global & g = mdcm::GlobalInstance;
		const mdcm::Dicts & dicts = g.GetDicts();
		const mdcm::Dict & dict = dicts.GetPublicDict();
		process_directory(p, dict, pd);
	}
	catch(mdcm::ParseException & pe)
	{
		std::cout
			<< "mdcm::ParseException in BrowserWidget2::read_directory:\n"
			<< pe.GetLastElement().GetTag() << std::endl;
	}
	catch(std::exception & ex)
	{
		std::cout << "Exception in BrowserWidget2::read_directory:\n"
			<< ex.what() << std::endl;
	}
	pd->close();
	qApp->processEvents();
	delete pd;
}

void BrowserWidget2::process_directory(const QString & p, const mdcm::Dict & dict, QProgressDialog * pd)
{
	if (p.isEmpty()) return;
	QDir dir(p);
	QStringList dlist = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	QStringList flist = dir.entryList(QDir::Files|QDir::Readable,QDir::Name);
	std::vector<std::string> filenames;
	QStringList filenames_no_series_uid;
	for (int x = 0; x < flist.size(); ++x)
	{
		pd->setValue(-1);
		qApp->processEvents();
		if (pd->wasCanceled()) return;
		const QString tmp0 = dir.absolutePath() + QString("/") + flist.at(x);
		{
			mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
			reader.SetFileName(QDir::toNativeSeparators(tmp0).toUtf8().constData());
#else
			reader.SetFileName(QDir::toNativeSeparators(tmp0).toLocal8Bit().constData());
#endif
#else
			reader.SetFileName(tmp0.toLocal8Bit().constData());
#endif
			if (reader.ReadUpToTag(tSeriesInstanceUID))
			{
				const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
				if (ds.FindDataElement(tSeriesInstanceUID))
				{
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
					filenames.push_back(std::string(QDir::toNativeSeparators(tmp0).toUtf8().constData()));
#else
					filenames.push_back(std::string(QDir::toNativeSeparators(tmp0).toLocal8Bit().constData()));
#endif
#else
					filenames.push_back(std::string(tmp0.toLocal8Bit().constData()));
#endif
				}
				else
				{
					filenames_no_series_uid.push_back(tmp0);
				}
			}
		}
	}
	flist.clear();
	//
	{
		mdcm::SmartPointer<mdcm::Scanner> sp = new mdcm::Scanner;
		mdcm::Scanner & s0 = *sp;
		ScannerWatcher sw(&s0);
		s0.AddTag(tSeriesInstanceUID);
		s0.Scan(filenames, dict);
		pd->setValue(-1);
		qApp->processEvents();
		if (pd->wasCanceled()) return;
		mdcm::Scanner::ValuesType v = s0.GetValues();
		mdcm::Scanner::ValuesType::iterator vi = v.begin();
		for (; vi!=v.end(); ++vi)
		{
			QString modality    = QString("");
			QString name        = QString("");
			QString birthdate   = QString("");
			QString study       = QString("");
			QString study_date  = QString("");
			QString series      = QString("");
			QString series_date = QString("");
			bool is_image       = false;
			bool is_softcopy  = false;
			const int idx = tableWidget->rowCount();
			QString ids;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			ids = QString::asprintf("%010d", idx);
#else
			ids.sprintf("%010d", idx);
#endif
			TableWidgetItem * i = new TableWidgetItem(ids);
			std::vector<std::string> files__(
				s0.GetAllFilenamesFromTagToValue(
					tSeriesInstanceUID, (*vi).c_str()));
			for (unsigned int z = 0; z < files__.size(); ++z)
			{

				const QString tmp_filename =
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
					QString::fromUtf8(files__.at(z).c_str());
#else
					QString::fromLocal8Bit(files__.at(z).c_str());
#endif
				i->files.push_back(tmp_filename);
			}
			const unsigned int series_size = i->files.size();
			if (series_size > 0) read_tags_(
				i->files.at(0),
				name,birthdate,
				modality,
				study,study_date,
				series,series_date,
				&is_image,&is_softcopy);
			pd->setValue(-1);
			qApp->processEvents();
			if (pd->wasCanceled()) return;
			//
			if (!is_image && series_size > 1)
			{
				size_t series_idx = 0;
				do
				{
					++series_idx;
					qApp->processEvents();
					if (pd->wasCanceled()) return;
					bool is_image_tmp = false;
					read_tags_short_(
						i->files.at(series_idx),
						&is_image_tmp,&is_softcopy);
					if (is_image_tmp) { is_image = true; break; }
					else if (is_softcopy) { break; }
				}
				while (series_idx < (series_size - 1));
			}
			//
			tableWidget->setRowCount(idx + 1);
			tableWidget->setItem(idx,0,static_cast<QTableWidgetItem*>(i));
			if (is_image)
			{
				tableWidget->setItem(idx,1,new QTableWidgetItem(eye_icon,QString("")));
			}
			else if (is_softcopy)
			{
				tableWidget->setItem(idx,1,new QTableWidgetItem(eye2_icon,QString("")));
			}
			tableWidget->setItem(idx,2,new QTableWidgetItem(modality));
			tableWidget->setItem(idx,3,new QTableWidgetItem(
				DicomUtils::convert_pn_value(name.remove(QChar('\0')))));
			tableWidget->setItem(idx,4,new QTableWidgetItem(birthdate));
			tableWidget->setItem(idx,5,new QTableWidgetItem(study.remove(QChar('\0'))));
			tableWidget->setItem(idx,6,new QTableWidgetItem(study_date));
			tableWidget->setItem(idx,7,new QTableWidgetItem(series.remove(QChar('\0'))));
			tableWidget->setItem(idx,8,new QTableWidgetItem(series_date));
			tableWidget->setItem(idx,9,new QTableWidgetItem(QVariant(i->files.size()).toString()));
			//
			pd->setValue(-1);
			qApp->processEvents();
			if (pd->wasCanceled()) return;
		}
	}
	//
	for (int x = 0; x < filenames_no_series_uid.size(); ++x)
	{
		const QString tmp1 = filenames_no_series_uid.at(x);
		QString modality    = QString("");
		QString name        = QString("");
		QString birthdate   = QString("");
		QString study       = QString("");
		QString study_date  = QString("");
		QString series      = QString("");
		QString series_date = QString("");
		bool is_image       = false;
		bool is_softcopy    = false;
		const int idx = tableWidget->rowCount();
		QString ids;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		ids = QString::asprintf("%010d", idx);
#else
		ids.sprintf("%010d", idx);
#endif
		TableWidgetItem * i = new TableWidgetItem(ids);
		i->files.push_back(tmp1);
		read_tags_(
			tmp1,
			name,birthdate,
			modality,
			study,study_date,
			series,series_date,
			&is_image,&is_softcopy);
		tableWidget->setRowCount(idx+1);
		tableWidget->setItem(idx,0,static_cast<QTableWidgetItem*>(i));
		if (is_image)
		{
			tableWidget->setItem(idx,1,new QTableWidgetItem(eye_icon,QString("")));
		}
		else if (is_softcopy)
		{
			tableWidget->setItem(idx,1,new QTableWidgetItem(eye2_icon,QString("")));
		}
		tableWidget->setItem(idx,2,new QTableWidgetItem(modality));
		tableWidget->setItem(idx,3,new QTableWidgetItem(
			DicomUtils::convert_pn_value(name.remove(QChar('\0')))));
		tableWidget->setItem(idx,4,new QTableWidgetItem(birthdate));
		tableWidget->setItem(idx,5,new QTableWidgetItem(study.remove(QChar('\0'))));
		tableWidget->setItem(idx,6,new QTableWidgetItem(study_date));
		tableWidget->setItem(idx,7,new QTableWidgetItem(series.remove(QChar('\0'))));
		tableWidget->setItem(idx,8,new QTableWidgetItem(series_date));
		tableWidget->setItem(idx,9,new QTableWidgetItem(QString("1")));
		pd->setValue(-1);
		qApp->processEvents();
		if (pd->wasCanceled()) return;
	}
	filenames.clear();
	filenames_no_series_uid.clear();
	//
	if (!pd->wasCanceled())
	{
		for (int j = 0; j < dlist.size(); ++j)
			process_directory(
				dir.absolutePath() + QString("/") + dlist.at(j),
				dict,
				pd);
	}
	dlist.clear();
}

void BrowserWidget2::open_dicom_dir()
{
	QFileInfo fi(directory_lineEdit->text());
	const QString d__ = fi.isFile() ? fi.absolutePath() : directory_lineEdit->text();
	const QString dirname =
		QFileDialog::getExistingDirectory(this, QString("Select directory to scan"), d__,
											(QFileDialog::ShowDirsOnly|QFileDialog::ReadOnly
											/* | QFileDialog::DontUseNativeDialog*/
											));
	if (dirname.isEmpty()) return;
	directory_lineEdit->setText(QDir::toNativeSeparators(dirname));
	read_directory(dirname);
}

void BrowserWidget2::open_dicom_dir2(const QString & d)
{
	if (d.isEmpty()) return;
	directory_lineEdit->setText(QDir::toNativeSeparators(d));
	read_directory(d);
}

void BrowserWidget2::open_DICOMDIR2(const QString & f)
{
	if (f.isEmpty()) return;
	directory_lineEdit->setText(QDir::toNativeSeparators(f));
	QString warning;
	try
	{
		warning = read_DICOMDIR(f);
	}
	catch(mdcm::ParseException & pe)
	{
		std::cout
			<< "mdcm::ParseException in BrowserWidget2::open_DICOMDIR2:\n"
			<< pe.GetLastElement().GetTag() << std::endl;
	}
	catch(std::exception & ex)
	{
		std::cout << "Exception in BrowserWidget2::open_DICOMDIR2:\n"
			<< ex.what() << std::endl;
	}
	if (!warning.isEmpty())
	{
		QMessageBox mbox;
		mbox.setIcon(QMessageBox::Warning);
		mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		mbox.setDefaultButton(QMessageBox::Yes);
		mbox.setText(warning+QString("\nScan directory?"));
		int r = mbox.exec();
		bool scan = false;
		switch (r)
		{
		case QMessageBox::No:
			scan = false;
			break;
		case QMessageBox::Yes:
		default:
			scan = true;
			break;
		}
		if (scan)
		{
			QFileInfo fi(f);
			directory_lineEdit->setText(
				QDir::toNativeSeparators(fi.absolutePath()));
			reload_dir();
		}
	}
}

void BrowserWidget2::dropEvent(QDropEvent * e)
{
	const QMimeData * mimeData = e->mimeData();
	QStringList l;
	if (!mimeData) return;
	if (mimeData->hasUrls())
	{
		QList<QUrl> urls = mimeData->urls();
		for (int i = 0; i < urls.size() && i < 128; ++i)
			l.append(urls.at(i).toLocalFile());
	}
	if (l.size()>=1)
	{
		const QString f = l.at(0);
		QFileInfo fi(f);
		if (fi.isDir())
		{
			open_dicom_dir2(f);
		}
		else if (fi.isFile())
		{
			open_DICOMDIR2(f);
		}
		else
		{
			tableWidget->clearContents();
			tableWidget->setRowCount(0);
		}
	}
}

void BrowserWidget2::dragEnterEvent(QDragEnterEvent * e)
{
	e->acceptProposedAction();
}

void BrowserWidget2::dragMoveEvent(QDragMoveEvent * e)
{
	e->acceptProposedAction();
}

void BrowserWidget2::dragLeaveEvent(QDragLeaveEvent * e)
{
	e->accept();
}

bool BrowserWidget2::is_first_run() const
{
	return !once;
}

void BrowserWidget2::reload_dir()
{
	QFileInfo fi(QDir::fromNativeSeparators(
		directory_lineEdit->text()));
	if (fi.isDir())
	{
		read_directory(QDir::fromNativeSeparators(
			directory_lineEdit->text()));
	}
	else if (fi.isFile())
	{
#ifdef USE_WORKSTATION_MODE
		if (fi.fileName() == QString("ctkDICOM.sql"))
		{
			open_CTK_db();
		}
		else
#endif
		{
			QString warning;
			try
			{
				warning = read_DICOMDIR(QDir::fromNativeSeparators(
							directory_lineEdit->text()));
			}
			catch(mdcm::ParseException & pe)
			{
				std::cout
					<< "mdcm::ParseException in BrowserWidget2::reload_dir:\n"
					<< pe.GetLastElement().GetTag() << std::endl;
			}
			catch(std::exception & ex)
			{
				std::cout << "Exception in BrowserWidget2::reload_dir:\n"
					<< ex.what() << std::endl;
			}
			if (!warning.isEmpty())
			{
				QMessageBox mbox;
				mbox.setIcon(QMessageBox::Warning);
				mbox.setStandardButtons(
					QMessageBox::Yes | QMessageBox::No);
				mbox.setDefaultButton(QMessageBox::Yes);
				mbox.setText(warning+QString("\nScan directory?"));
				int r = mbox.exec();
				bool scan = false;
				switch (r)
				{
				case QMessageBox::No:
					scan = false;
					break;
				case QMessageBox::Yes:
				default:
					scan = true;
					break;
				}
				if (scan)
				{
					directory_lineEdit->setText(
						QDir::toNativeSeparators(fi.absolutePath()));
					reload_dir();
				}
			}
		}
	}
	else
	{
		tableWidget->clearContents();
		tableWidget->setRowCount(0);
	}
}

QStringList BrowserWidget2::get_files_of_1st()
{
	const QList<QTableWidgetItem *> selected_items =
		tableWidget->selectedItems();
	if (selected_items.empty()) return QStringList();
	if (!selected_items.at(0))  return QStringList();
	const int current_row = selected_items.at(0)->row();
	if (current_row < 0) return QStringList();
	const TableWidgetItem * current_item =
		static_cast<TableWidgetItem *>(tableWidget->item(current_row, 0));
	if (!current_item) return QStringList();
	if (current_item->files.empty()) return QStringList();
	return current_item->files;
}

void BrowserWidget2::copy_files()
{
	static unsigned long long count2 = 0;
	std::vector<int> rows;
	QModelIndexList selection =
		tableWidget->selectionModel()->selectedRows();
	for(int x = 0; x < selection.count(); ++x)
	{
		const QModelIndex index = selection.at(x);
		rows.push_back(index.row());
	}
	if (rows.empty()) return;
	const QString dirname = QFileDialog::getExistingDirectory(
		this,
		QString("Select Destination Directory"),
		saved_copy_dir,
		(QFileDialog::ShowDirsOnly));
	if (dirname.isEmpty()) return;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	saved_copy_dir = dirname;
	QList<QStringList> files;
	for (unsigned int x = 0; x < rows.size(); ++x)
	{
		const int row = rows.at(x);
		if (row < 0) continue;
		const TableWidgetItem * item =
			static_cast<TableWidgetItem *>(tableWidget->item(row, 0));
		if (!item) continue;
		if ((item->files.empty())) continue;
		files << item->files;
	}
	for (int x = 0; x < files.size(); ++x)
	{
		++count2;
		const QString tmp1 =
			QDateTime::currentDateTime()
				.toString(QString("yyyyMMddhhmmsszzz")) +
			QString("-") +
			QVariant(count2).toString();
		const QString dir1 =
			dirname +
			QString("/") +
			tmp1;
		QFileInfo fi2(dir1);
		if (!fi2.exists())
		{
			QDir d(dirname);
			d.mkdir(tmp1);
		}
		for (int y = 0; y < files.at(x).size(); ++y)
		{
			const QString f = files.at(x).at(y);
			QFileInfo fi(f);
			if (fi.exists())
			{
				const QString f1 = 
					dir1 +
					QString("/") +
					fi.fileName();
				QFile::copy(f, f1);
				qApp->processEvents();
			}
		}
	}
	QApplication::restoreOverrideCursor();
}

void BrowserWidget2::open_DICOMDIR()
{
	QFileInfo fi(directory_lineEdit->text());
	const QString d__ =
		fi.isFile() ? fi.absolutePath() : directory_lineEdit->text();
	const QString f = QFileDialog::getOpenFileName(
		this,
		QString("Open DICOMDIR"),
		d__,
		QString(),
		(QString*)NULL,
		(QFileDialog::ReadOnly
		/*| QFileDialog::DontUseNativeDialog*/
		));
	if (f.isEmpty()) return;
	directory_lineEdit->setText(QDir::toNativeSeparators(f));
	const QString warning = read_DICOMDIR(f);
	if (!warning.isEmpty())
	{
		QMessageBox mbox;
		mbox.setIcon(QMessageBox::Warning);
		mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		mbox.setDefaultButton(QMessageBox::Yes);
		mbox.setText(warning+QString("\nScan directory?"));
		int r = mbox.exec();
		bool scan = false;
		switch (r)
		{
		case QMessageBox::No:
			scan = false;
			break;
		case QMessageBox::Yes:
		default:
			scan = true;
			break;
		}
		if (scan)
		{
			QFileInfo fi1(f);
			directory_lineEdit->setText(
				QDir::toNativeSeparators(fi1.absolutePath()));
			reload_dir();
		}
	}
}

const QString BrowserWidget2::read_DICOMDIR(const QString & f)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	tableWidget->clearContents();
	tableWidget->setRowCount(0);
	//
	mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	reader.SetFileName(QDir::toNativeSeparators(f).toUtf8().constData());
#else
	reader.SetFileName(QDir::toNativeSeparators(f).toLocal8Bit().constData());
#endif
#else
	reader.SetFileName(f.toLocal8Bit().constData());
#endif
	if(!reader.Read())
	{
		QApplication::restoreOverrideCursor();
		return QString("Can not read DICOMDIR file.");
	}
	const mdcm::File & file = reader.GetFile();
	mdcm::MediaStorage ms;
	ms.SetFromFile(file);
	if(ms != mdcm::MediaStorage::MediaStorageDirectoryStorage)
	{
		QApplication::restoreOverrideCursor();
		return QString("Not Media Storage Directory Storage.");
	}
	//
	QFileInfo fi0(f);
	const QString dir_ = fi0.absolutePath();
	//
	QMap<unsigned int, EntryDICOMDIR> m;
	const mdcm::FileMetaInformation & header = file.GetHeader();
	const mdcm::DataSet & ds = file.GetDataSet();
	//
	if (!ds.FindDataElement(tDirectoryRecordSequence))
	{
		QApplication::restoreOverrideCursor();
		return QString("Can not find Directory Record Sequence.");
	}
	const mdcm::DataElement & eDirectoryRecordSequence =
		ds.GetDataElement(tDirectoryRecordSequence);
	mdcm::SmartPointer<mdcm::SequenceOfItems> sqDirectoryRecordSequence =
		eDirectoryRecordSequence.GetValueAsSQ();
	if(!(sqDirectoryRecordSequence && sqDirectoryRecordSequence->GetNumberOfItems()>0))
	{
		QApplication::restoreOverrideCursor();
		return QString("Directory Record Sequence is empty.");
	}
	// header.GetFullLength() includes Preamble(128) + Prefix (4)
	const mdcm::VL start = header.GetFullLength() + compute_offset0(ds);
	std::vector<unsigned int> offsets;
	compute_offsets(sqDirectoryRecordSequence,start,offsets);
	//
	bool not_patient_study_series_model = false;
	bool ok_first_root_off = false;
	unsigned int first_root_off = 0;
	if (ds.FindDataElement(tOffsetOfTheFirstDirectoryRecordOfTheRootDirectoryEntity))
	{
		ok_first_root_off =
			DicomUtils::get_ul_value(
				ds,
				tOffsetOfTheFirstDirectoryRecordOfTheRootDirectoryEntity,
				&first_root_off);
	}
	//
	std::set<mdcm::DataElement>::const_iterator it = ds.GetDES().cbegin();
	while(it != ds.GetDES().cend())
	{
		if (it->GetTag() == tDirectoryRecordSequence)
		{
			const mdcm::DataElement & de = *it;
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqi = de.GetValueAsSQ();
			for (unsigned int i = 0; i < sqi->GetNumberOfItems(); ++i)
			{
				const mdcm::Item    & item = sqi->GetItem(i+1);
				const mdcm::DataSet & nds  = item.GetNestedDataSet();

				EntryDICOMDIR ed;
				if (nds.FindDataElement(tOffsetOfTheNextDirectoryRecord))
				{
					unsigned int off1 = 0;
					const bool ok_off1 = DicomUtils::get_ul_value(nds,tOffsetOfTheNextDirectoryRecord,&off1);
					if (ok_off1)
					{
						ed.offsetOfTheNextDirectoryRecord = off1;
					}
					else
					{
						QApplication::restoreOverrideCursor();
						return QString("Error reading \"Offset of the Next Directory Record\".");
					}
				}
				if (nds.FindDataElement(tOffsetOfReferencedLowerLevelDirectoryEntity))
				{
					unsigned int off2 = 0;
					const bool ok_off2 = DicomUtils::get_ul_value(nds,tOffsetOfReferencedLowerLevelDirectoryEntity,&off2);
					if (ok_off2)
					{
						ed.offsetOfReferencedLowerLevelDirectoryEntity = off2;
					}
					else
					{
						QApplication::restoreOverrideCursor();
						return QString("Error reading \"Offset of Referenced Lower Level Directory Entity\".");
					}
				}
				if (nds.FindDataElement(tDirectoryRecordType))
				{
					const mdcm::DataElement & e = nds.GetDataElement(tDirectoryRecordType);
					if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
					{
						const QString directory_record_type =
							QString::fromLatin1(
								e.GetByteValue()->GetPointer(),
								e.GetByteValue()->GetLength()).toUpper().trimmed().remove(QChar('\0'));
						ed.directoryRecordType = directory_record_type;
						if (directory_record_type == QString("PATIENT"))
						{
							QString charset = QString("");
							if (nds.FindDataElement(tSpecificCharacterSet))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tSpecificCharacterSet);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
									charset = QString::fromLatin1(e1.GetByteValue()->GetPointer(),e1.GetByteValue()->GetLength());
							}
							if (nds.FindDataElement(tPatientsName))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tPatientsName);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									QByteArray ba(e1.GetByteValue()->GetPointer(), e1.GetByteValue()->GetLength());
									const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
									ed.patient = tmp0.trimmed().remove(QChar('\0'));
								}
							}
							if (nds.FindDataElement(tPatientsBirthDate))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tPatientsBirthDate);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									const QString birthdate_s =
										QString::fromLatin1(e1.GetByteValue()->GetPointer(),
															e1.GetByteValue()->GetLength()).trimmed();
									const QDate qd = QDate::fromString(birthdate_s, QString("yyyyMMdd"));
									ed.birthdate = qd.toString(QString("d MMM yyyy")) + QString("\n");
								}
							}
						}
						else if (directory_record_type == QString("STUDY"))
						{
							QString charset = QString("");
							if (nds.FindDataElement(tSpecificCharacterSet))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tSpecificCharacterSet);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
									charset = QString::fromLatin1(e1.GetByteValue()->GetPointer(),e1.GetByteValue()->GetLength());
							}
							if (nds.FindDataElement(tStudyDate))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tStudyDate);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									const QString date_s =
										QString::fromLatin1(e1.GetByteValue()->GetPointer(),
															e1.GetByteValue()->GetLength()).trimmed();
									const QDate qd = QDate::fromString(date_s, QString("yyyyMMdd"));
									ed.study_date = qd.toString(QString("d MMM yyyy")) + QString("\n");
								}
							}
							if (nds.FindDataElement(tStudyDescription))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tStudyDescription);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									QByteArray ba(e1.GetByteValue()->GetPointer(), e1.GetByteValue()->GetLength());
									const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
									ed.study_desc = tmp0.trimmed().remove(QChar('\0'));
								}
							}
						}
						else if (directory_record_type == QString("SERIES"))
						{
							QString charset = QString("");
							if (nds.FindDataElement(tSpecificCharacterSet))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tSpecificCharacterSet);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
									charset = QString::fromLatin1(e1.GetByteValue()->GetPointer(),e1.GetByteValue()->GetLength());
							}
							if (nds.FindDataElement(tModality))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tModality);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									const QString tmp0 =
										QString::fromLatin1(e1.GetByteValue()->GetPointer(),e1.GetByteValue()->GetLength());
									ed.modality = tmp0.trimmed();
								}
							}

							if (nds.FindDataElement(tSeriesDate))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tSeriesDate);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									const QString date_s =
										QString::fromLatin1(e1.GetByteValue()->GetPointer(),
															e1.GetByteValue()->GetLength()).trimmed();
									const QDate qd = QDate::fromString(date_s, QString("yyyyMMdd"));
									ed.series_date = qd.toString(QString("d MMM yyyy")) + QString("\n");
								}
							}

							if (nds.FindDataElement(tSeriesDescription))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tSeriesDescription);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									QByteArray ba(e1.GetByteValue()->GetPointer(), e1.GetByteValue()->GetLength());
									const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
									ed.series_desc = tmp0.trimmed().remove(QChar('\0'));
								}
							}
						}
						else
						{
							if (ed.offsetOfReferencedLowerLevelDirectoryEntity!=0) not_patient_study_series_model = true;
							QString charset = QString("");
							if (nds.FindDataElement(tSpecificCharacterSet))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tSpecificCharacterSet);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
									charset = QString::fromLatin1(e1.GetByteValue()->GetPointer(),e1.GetByteValue()->GetLength());
							}
							if (nds.FindDataElement(tReferencedFileID))
							{
								const mdcm::DataElement & e1 = nds.GetDataElement(tReferencedFileID);
								if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
								{
									QByteArray ba(e1.GetByteValue()->GetPointer(), e1.GetByteValue()->GetLength());
									const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
									const QStringList l2 = tmp0.trimmed().split(QString("\\"));
									const int l2size = l2.size();
									QString fpath("");
									for (int x = 0; x < l2size; ++x)
									{
										fpath.append(l2.at(x));
										if (x!=l2size-1) fpath.append(QString("/"));
									}
									ed.file = fpath;
								}
							}
						}
						m.insert(offsets[i], ed);
					}
				}
			}
		}
		++it;
	}
	//
	QString warning("");
	//
	QList<SeriesDICOMDIR> series;
	if (ok_first_root_off)
	{
		unsigned int offset_next = add_roots(m, first_root_off, series, &not_patient_study_series_model);
		while (offset_next > 0)
		{
			offset_next = add_roots(m, offset_next, series, &not_patient_study_series_model);
		}
	}
	else
	{
		QMapIterator<unsigned int,EntryDICOMDIR> mi(m);
		while (mi.hasNext())
		{
			mi.next();
			while (mi.value().directoryRecordType==QString("PATIENT"))
			{
				const EntryDICOMDIR & e = mi.value();
				unsigned int offset_next = add_study(
					m,
					e.offsetOfReferencedLowerLevelDirectoryEntity,
					series,
					e.patient,
					e.birthdate,
					&not_patient_study_series_model);
				while (offset_next > 0)
				{
					offset_next = add_study(
						m,
						offset_next,
						series,
						e.patient,
						e.birthdate,
						&not_patient_study_series_model);
				}
				if (mi.hasNext()) { mi.next(); }
				else { break; }
			}
		}
	}
	//
	if (series.size()==0)
	{
		QApplication::restoreOverrideCursor();
		return QString("Error, found no series");
	}
	if (not_patient_study_series_model)
	{
		warning = QString("Can not completely process DICOMDIR.");
	}
	//
	for (int x = 0; x < series.size(); ++x)
	{
		bool break__ = false;
		for (int z = 0; z < series.at(x).files.size(); ++z)
		{
			QFileInfo fi(dir_ + QString("/") + series.at(x).files.at(z));
			if (!fi.isFile())
			{
				if (!warning.isEmpty()) warning.append("\n");
				warning.append(QString("Some files don't exist."));
				break__ = true;
				break;
			}
		}
		if (break__) break;
	}
	//
	for (int x = 0; x < series.size(); ++x)
	{
		const int idx = tableWidget->rowCount();
		QString ids;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	 	ids = QString::asprintf("%010d", idx);
#else
	 	ids.sprintf("%010d", idx);
#endif
		TableWidgetItem * i = new TableWidgetItem(ids);
		for (int z = 0; z < series.at(x).files.size(); ++z)
			i->files.push_back(dir_ + QString("/") + series.at(x).files.at(z));
		tableWidget->setRowCount(idx+1);
		tableWidget->setItem(idx,0,static_cast<QTableWidgetItem*>(i));
		if (series.at(x).eye)
		{
			tableWidget->setItem(idx,1,new QTableWidgetItem(eye_icon,QString("")));
		}
		else if (series.at(x).eye2)
		{
			tableWidget->setItem(idx,1,new QTableWidgetItem(eye2_icon,QString("")));
		}
		tableWidget->setItem(idx,2,new QTableWidgetItem(series.at(x).modality));
		tableWidget->setItem(idx,3,new QTableWidgetItem(series.at(x).patient));
		tableWidget->setItem(idx,4,new QTableWidgetItem(series.at(x).birthdate));
		tableWidget->setItem(idx,5,new QTableWidgetItem(series.at(x).study));
		tableWidget->setItem(idx,6,new QTableWidgetItem(series.at(x).study_date));
		tableWidget->setItem(idx,7,new QTableWidgetItem(series.at(x).series));
		tableWidget->setItem(idx,8,new QTableWidgetItem(series.at(x).series_date));
		tableWidget->setItem(idx,9,new QTableWidgetItem(QVariant(i->files.size()).toString()));
	}
	//
	QApplication::restoreOverrideCursor();
	//
	return warning;
}

unsigned int BrowserWidget2::add_roots(
	const QMap<unsigned int, EntryDICOMDIR> & m,
	unsigned int offset,
	QList<SeriesDICOMDIR> & l,
	bool * warn)
{
	if (!m.contains(offset)) return 0;
	const EntryDICOMDIR & e = m.value(offset);
	if (e.directoryRecordType == QString("PATIENT"))
	{
		unsigned int offset_next = add_study(
			m,
			e.offsetOfReferencedLowerLevelDirectoryEntity,
			l,
			e.patient,
			e.birthdate,
			warn);
		while (offset_next > 0)
		{
			offset_next = add_study(
				m,
				offset_next,
				l,
				e.patient,
				e.birthdate,
				warn);
		}
	}
	else
	{
		*warn = true;
	}
	return e.offsetOfTheNextDirectoryRecord;
}

unsigned int BrowserWidget2::add_study(
	const QMap<unsigned int, EntryDICOMDIR> & m,
	unsigned int offset,
	QList<SeriesDICOMDIR> & l,
	const QString & patient,
	const QString & birthdate,
	bool * warn)
{
	if (!m.contains(offset)) return 0;
	const EntryDICOMDIR & e = m.value(offset);
	if (e.directoryRecordType == QString("STUDY"))
	{
		const QString study_desc = e.study_desc;
		const QString study_date = e.study_date;
		unsigned int offset_next = add_series(
			m,
			e.offsetOfReferencedLowerLevelDirectoryEntity,
			l,
			patient,
			birthdate,
			study_desc,
			study_date,
			warn);
		while (offset_next > 0)
		{
			offset_next = add_series(m,offset_next,l,patient,birthdate,study_desc,study_date,warn);
		}
	}
	else
	{
		*warn = true;
	}
	return e.offsetOfTheNextDirectoryRecord;
}

unsigned int BrowserWidget2::add_series(
	const QMap<unsigned int, EntryDICOMDIR> & m,
	unsigned int offset,
	QList<SeriesDICOMDIR> & l,
	const QString & patient,
	const QString & birthdate,
	const QString & study_desc,
	const QString & study_date,
	bool * warn)
{
	if (!m.contains(offset)) return 0;
	const EntryDICOMDIR & e = m.value(offset);
	if (e.directoryRecordType == QString("SERIES"))
	{
		SeriesDICOMDIR s;
		s.patient     = patient;
		s.birthdate   = birthdate;
		s.study       = study_desc;
		s.study_date  = study_date;
		s.modality    = e.modality;
		s.series      = e.series_desc;
		s.series_date = e.series_date;
		unsigned int offset_next = add_file(
			m,
			e.offsetOfReferencedLowerLevelDirectoryEntity,
			s);
		while (offset_next > 0)
		{
			offset_next = add_file(m,offset_next,s);
		}
		l.push_back(s);
	}
	else
	{
		*warn = true;
	}
	return e.offsetOfTheNextDirectoryRecord;
}

unsigned int BrowserWidget2::add_file(
	const QMap<unsigned int, EntryDICOMDIR> & m,
	unsigned int offset,
	SeriesDICOMDIR & s)
{
	if (!m.contains(offset)) return 0;
	const EntryDICOMDIR & e = m.value(offset);
	if (e.directoryRecordType==QString("IMAGE") ||
		e.directoryRecordType==QString("RT STRUCTURE SET") ||
		e.directoryRecordType==QString("SPECTROSCOPY"))
	{
		s.eye = true;
	}
	else if(
		e.directoryRecordType==QString("PRESENTATION") ||
		e.directoryRecordType==QString("SR DOCUMENT"))
	{
		s.eye2 = true;
	}
	s.files.push_back(e.file);
	return e.offsetOfTheNextDirectoryRecord;
}

void BrowserWidget2::read_tags_(
	const QString & f,
	QString & patient_name_,
	QString & birthdate_,
	QString & modality_,
	QString & studydesc_,
	QString & study_date_,
	QString & seriesdesc_,
	QString & series_date_,
	bool * is_image,
	bool * is_softcopy)
{
	mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	reader.SetFileName(QDir::toNativeSeparators(f).toUtf8().constData());
#else
	reader.SetFileName(QDir::toNativeSeparators(f).toLocal8Bit().constData());
#endif
#else
	reader.SetFileName(f.toLocal8Bit().constData());
#endif
	bool f_ok = reader.ReadSelectedTags(selected_tags);
	if (!f_ok) return;
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	if (ds.IsEmpty()) return;
	QString charset = QString("");
	QString sop = QString("");
	//
	if(ds.FindDataElement(tSpecificCharacterSet))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tSpecificCharacterSet);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
			charset =
				QString::fromLatin1(e.GetByteValue()->GetPointer(),e.GetByteValue()->GetLength());
	}
	//
	if (ds.FindDataElement(tSOPClassUID))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tSOPClassUID);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			sop =
				QString::fromLatin1(e.GetByteValue()->GetPointer(),
									e.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
		}
	}
	//
	if(ds.FindDataElement(tStudyDate))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tStudyDate);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			const QString date_s =
				QString::fromLatin1(e.GetByteValue()->GetPointer(),
									e.GetByteValue()->GetLength()).trimmed();
			const QDate qd = QDate::fromString(date_s, QString("yyyyMMdd"));
			study_date_ = qd.toString(QString("d MMM yyyy")) + QString("\n");
		}
	}
	//
	if (ds.FindDataElement(tModality))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tModality);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			modality_ =
				QString::fromLatin1(e.GetByteValue()->GetPointer(),e.GetByteValue()->GetLength());
		}
	}
	//
	if(ds.FindDataElement(tStudyDescription))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tStudyDescription);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty()) studydesc_ = tmp0.simplified();
		}
	}
	//
	if(ds.FindDataElement(tSeriesDescription))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tSeriesDescription);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty()) seriesdesc_ = tmp0.simplified();
		}
	}
	//
	if(ds.FindDataElement(tSeriesDate))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tSeriesDate);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			const QString date_s =
				QString::fromLatin1(e.GetByteValue()->GetPointer(),
									e.GetByteValue()->GetLength()).trimmed();
			const QDate qd = QDate::fromString(date_s, QString("yyyyMMdd"));
			series_date_ = qd.toString(QString("d MMM yyyy")) + QString("\n");
		}
	}
	//
	if(ds.FindDataElement(tPatientsName))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tPatientsName);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty()) patient_name_ = tmp0;
		}
	}
	//
	if(ds.FindDataElement(tPatientsBirthDate))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tPatientsBirthDate);
		std::stringstream ss;
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			const QString birthdate_s =
				QString::fromLatin1(e.GetByteValue()->GetPointer(),
									e.GetByteValue()->GetLength()).trimmed();
			const QDate qd = QDate::fromString(birthdate_s, QString("yyyyMMdd"));
			birthdate_ = qd.toString(QString("d MMM yyyy")) + QString("\n");
		}
	}
	//
	bool has_rows         = false;
	bool has_colums       = false;
	bool has_bitallocated = false;
	if(ds.FindDataElement(tRows))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tRows);
		if (!e.IsEmpty()) has_rows = true;
	}
	if(ds.FindDataElement(tColumns))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tColumns);
		if (!e.IsEmpty()) has_colums = true;
	}
	if(ds.FindDataElement(tBitsAllocated))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tBitsAllocated);
		if (!e.IsEmpty()) has_bitallocated = true;
	}
	bool is_image_tmp = has_rows && has_colums && has_bitallocated;
	//
	// RTSTRUCT, spectroscopy, meshes
	if (sop==QString("1.2.840.10008.5.1.4.1.1.481.3") ||
		sop==QString("1.2.840.10008.5.1.4.1.1.4.2")   ||
		sop==QString("1.2.840.10008.5.1.4.1.1.68.1")  ||
		sop==QString("1.2.840.10008.5.1.4.1.1.66.5"))
		is_image_tmp = true;
	*is_image = is_image_tmp;
	// Presentation, SR
	if (   sop==QString("1.2.840.10008.5.1.4.1.1.11.1")  // Grayscale Softcopy Presentation State Storage
#if 0
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.2")  // Color Softcopy Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.3")  // Pseudo-Color Softcopy Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.4")  // Blending Softcopy Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.5")  // XA/XRF Grayscale Softcopy Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.6")  // Grayscale Planar MPR Volumetric Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.7")  // Compositing Planar MPR Volumetric Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.8")  // Advanced Blending Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.9")  // Volume Rendering Volumetric Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.10") // Segmented Volume Rendering Volumetric Presentation State Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.11.11") // Multiple Volume Rendering Volumetric Presentation State Storage
#endif
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.11") // Basic Text SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.22") // Enhanced SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.33") // Comprehensive SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.34") // Comprehensive 3D SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.35") // Extensible SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.40") // Procedure Log Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.50") // Mammography CAD SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.59") // Key Object Selection Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.65") // Chest CAD SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.67") // X-Ray Radiation Dose SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.68") // Radiopharmaceutical Radiation Dose SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.69") // Colon CAD SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.70") // Implantation Plan SR Document Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.71") // Acquisition Context SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.72") // Simplified Adult Echo SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.73") // Patient Radiation Dose SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.74") // Planned Imaging Agent Administration SR Storage
		|| sop==QString("1.2.840.10008.5.1.4.1.1.88.75") // Performed Imaging Agent Administration SR Storage
		)
		*is_softcopy = true;
}

void BrowserWidget2::read_tags_short_(
	const QString & f,
	bool * is_image,
	bool * is_softcopy)
{
	mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	reader.SetFileName(QDir::toNativeSeparators(f).toUtf8().constData());
#else
	reader.SetFileName(QDir::toNativeSeparators(f).toLocal8Bit().constData());
#endif
#else
	reader.SetFileName(f.toLocal8Bit().constData());
#endif
	bool f_ok = reader.ReadSelectedTags(selected_tags_short);
	if (!f_ok) return;
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	if (ds.IsEmpty()) return;
	bool has_rows         = false;
	bool has_colums       = false;
	bool has_bitallocated = false;
	bool has_pixelrepres  = false;
	if(ds.FindDataElement(tRows))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tRows);
		if (!e.IsEmpty()) has_rows = true;
	}
	if(ds.FindDataElement(tColumns))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tColumns);
		if (!e.IsEmpty()) has_colums = true;
	}
	if(ds.FindDataElement(tBitsAllocated))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tBitsAllocated);
		if (!e.IsEmpty()) has_bitallocated = true;
	}
	if(ds.FindDataElement(tPixelRepresentation))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tPixelRepresentation);
		if (!e.IsEmpty()) has_pixelrepres = true;
	}
	bool is_image_tmp = has_rows && has_colums && has_bitallocated && has_pixelrepres;
	if (!is_image_tmp)
	{
		if (ds.FindDataElement(tSOPClassUID))
		{
			const mdcm::DataElement & e = ds.GetDataElement(tSOPClassUID);
			if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
			{
				const QString sop =
					QString::fromLatin1(
						e.GetByteValue()->GetPointer(),
						e.GetByteValue()->GetLength()).
							trimmed().remove(QChar('\0'));
				// RTSTRUCT
				if (sop == QString("1.2.840.10008.5.1.4.1.1.481.3")) 
				{
					is_image_tmp = true;
				}
				// Softcopy
				else if (
					sop == QString("1.2.840.10008.5.1.4.1.1.11.1") ||
					sop == QString("1.2.840.10008.5.1.4.1.1.11.2"))
					*is_softcopy = true;
			}
		}
	}
	*is_image = is_image_tmp;
}

void BrowserWidget2::writeSettings(QSettings & settings)
{
#ifdef USE_WORKSTATION_MODE
	settings.beginGroup(QString("BrowserWidget2"));
	settings.setValue(
		QString("saved_user_dir"),
		QVariant(directory_lineEdit->text()));
	settings.setValue(QString("ctk_dir"),   ctk_dir);
	settings.setValue(QString("ctk_pname"), ctk_pname);
	settings.setValue(QString("ctk_pid"),   ctk_pid);
	settings.setValue(QString("ctk_from"),  ctk_from);
	settings.setValue(QString("ctk_to"),    ctk_to);
	settings.value(
		QString("ctk_apply_range"),
		(ctk_apply_range ? QString("Y") : QString("N")));
	settings.endGroup();
#endif
}

void BrowserWidget2::readSettings()
{
#ifdef USE_WORKSTATION_MODE
	QSettings settings(
		QSettings::IniFormat,
		QSettings::UserScope,
		QApplication::organizationName(),
		QApplication::applicationName());
	settings.setFallbacksEnabled(true);
#if (defined _WIN32)
	const QString d = QString("./DICOM");
#elif (defined __APPLE__)
	const QString d = QString
		("/Applications/AlizaMS.app/Contents/Resources/DICOM");
#else
	const QString d = 
		QApplication::applicationDirPath() + QString("/../DICOM");
#endif
	settings.beginGroup(QString("BrowserWidget2"));
	directory_lineEdit->setText(
		settings.value(QString("saved_user_dir"), QDir::toNativeSeparators(d)).toString());
	ctk_dir   = settings.value(QString("ctk_dir"),   QString("")).toString();
	ctk_pname = settings.value(QString("ctk_pname"), QString("")).toString();
	ctk_pid   = settings.value(QString("ctk_pid"),   QString("")).toString();
	ctk_from  = settings.value(QString("ctk_from"),  QString("")).toString();
	ctk_to    = settings.value(QString("ctk_to"),    QString("")).toString();
	const QString r =
		settings.value(QString("ctk_apply_range"), QString("N")).toString();
	settings.endGroup();
	if (r == QString("Y")) ctk_apply_range = true;
	else ctk_apply_range = false;
#else
	{
		const QString f1_ =
			QApplication::applicationDirPath() + QString("/../DICOMDIR");
		QFileInfo fi1 = QFileInfo(f1_);
		if (fi1.exists() && fi1.isFile())
		{
			directory_lineEdit->setText(
				QDir::toNativeSeparators(fi1.absoluteFilePath()));
		}
		else
		{
			const QString f2_ =
				QApplication::applicationDirPath() + QString("/../DICOM");
			QFileInfo fi2 = QFileInfo(f2_);
			if (fi2.exists() && fi2.isDir())
			{
				directory_lineEdit->setText(
					QDir::toNativeSeparators(fi2.absoluteFilePath()));
			}
			else
			{
				directory_lineEdit->setText(QString(""));
			}
		}
	}
#endif
}

#ifdef USE_WORKSTATION_MODE
void BrowserWidget2::open_CTK_db()
{
	if (!once) once = true;
	tableWidget->clearContents();
	tableWidget->setRowCount(0);
	directory_lineEdit->clear();
	QString warning("");
	bool ok = false;
	QList<SeriesDICOMDIR> series;
	QSqlDatabase db;
	QString dbfile;
	QSqlQuery p0Query;
	QSqlQuery p1Query;
	QSqlQuery p2Query;
	QSqlQuery sQuery;
	QString p0;
	QString p1;
	QString p2;
	QString ss;
	QSet<int> ids;
	QSet<int>::const_iterator it0;
	size_t ids_count = 0;
	QStringList ctk_studies;
	QStringList ctk_series;
	if (ctk_from.isEmpty() || ctk_to.isEmpty())
	{
		QDate d1 = QDate::currentDate();
		QDate d0 = d1.addDays(-30);
		ctk_from = d0.toString("yyyyMMdd");
		ctk_to   = d1.toString("yyyyMMdd");
	}
	CTKDialog * d = new CTKDialog();
	d->set_dir(QDir::toNativeSeparators(ctk_dir));
	d->set_pname(ctk_pname);
	d->set_pid(ctk_pid);
	d->set_from(ctk_from);
	d->set_to(ctk_to);
	d->set_apply_range(ctk_apply_range);
	QString range_string("");
	if (d->exec() == QDialog::Accepted)
	{
		ok = true;
		ctk_dir   = QDir::toNativeSeparators(d->get_dir());
		ctk_pname = d->get_pname().trimmed();
		ctk_pid   = d->get_pid().trimmed();
		ctk_from  = QDate(d->get_from()).toString("yyyyMMdd");
		ctk_to    = QDate(d->get_to()).toString("yyyyMMdd");;
		ctk_apply_range = d->get_apply_range();
	}
	delete d;
	if (!ok) return;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	dbfile =
		ctk_dir +
		QString("/") +
		QString("ctkDICOM.sql");
	if (QSqlDatabase::contains(QString("CTKdb")))
		db = QSqlDatabase::database("CTKdb");
	else
		db = QSqlDatabase::addDatabase(QString("QSQLITE"), "CTKdb");
	db.setConnectOptions(QString("QSQLITE_OPEN_READONLY"));
	db.setDatabaseName(dbfile);
	if (!db.open(QString(""), QString("")))
	{
		warning = db.lastError().text();
		goto quit__;
	}
	p0 = QString("select UID from Patients");
	if (!ctk_pname.isEmpty()||!ctk_pid.isEmpty())
	{
		p0 += QString(" where ");
		if (!ctk_pname.isEmpty())
		{
			p0 += QString("PatientsName like '%") + ctk_pname + QString("%'");
		}
		if (!ctk_pid.isEmpty()) 
		{
			if (!ctk_pname.isEmpty())
			{
				p0 += " or ";
			}
			p0 += QString("PatientID like '%") + ctk_pid + QString("%'");
		}
	}
	p0Query = QSqlQuery(p0, db);
	while(p0Query.next())
	{
		ids << p0Query.value(0).toInt();
	}
	p0Query.clear();
	if (ctk_pname.isEmpty() && ctk_pid.isEmpty())
	{
		p1 = QString("select StudyInstanceUID from Studies");
		if (ctk_apply_range)
		{
			p1.append(QString(" where StudyDate between ") + ctk_from +
			QString(" and ") + ctk_to);
		}
	}
	else
	{
		p1 = QString("select StudyInstanceUID from Studies where PatientsUID in (");
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		it0 = ids.cbegin();
		while (it0 != ids.cend())
#else
		it0 = ids.constBegin();
		while (it0 != ids.constEnd())
#endif
		{
			p1.append(QVariant(*it0).toString());
			++ids_count;
			if (ids_count < (size_t)ids.size()) p1.append(QString(","));
			++it0;
		}
		p1.append(QString(")"));
		if (ctk_apply_range)
		{
			p1.append(QString("and StudyDate between ") + ctk_from +
			QString(" and ") + ctk_to);
		}
	}
	p1Query = QSqlQuery(p1, db);
	while(p1Query.next())
	{
		ctk_studies.push_back(p1Query.value(0).toString());
	}
	p1Query.clear();
	p2 = QString("select SeriesInstanceUID from Series where StudyInstanceUID in (");
	for (int x = 0; x < ctk_studies.size(); ++x)
	{
		p2.append(QString("\"") + ctk_studies.at(x) + QString("\""));
		if (x < ctk_studies.size() - 1) p2.append(QString(","));
	}
	p2.append(QString(")"));
	p2Query = QSqlQuery(p2, db);
	while(p2Query.next())
	{
		ctk_series.push_back(p2Query.value(0).toString());
	}
	p2Query.clear();
	for (int x = 0; x < ctk_series.size(); ++x)
	{
		SeriesDICOMDIR series0;
		series0.UID = ctk_series.at(x);
		QString sid;
		QString p0id;
		{
			const QString s =
				QString(
					"select Modality,SeriesDate,SeriesDescription,StudyInstanceUID"
					" from Series where SeriesInstanceUID = \"") +
				ctk_series.at(x) + QString("\"");
			QSqlQuery m = QSqlQuery(s, db);
			if (m.next())
			{
				series0.modality = m.value(0).toString().trimmed();
				const QDate tmp1 =
					QDate::fromString(
						m.value(1).toString(),
						QString("yyyy-MM-dd"));
				series0.series = m.value(2).toString().trimmed();
				sid = m.value(3).toString();
				series0.series_date = tmp1.toString(QString("d MMM yyyy"));

			}
			m.clear();
		}
		if (!sid.isEmpty())
		{
			const QString s =
				QString(
					"select PatientsUID,StudyDate,StudyDescription"
					" from Studies where StudyInstanceUID = \"") +
				sid + QString("\"");
			QSqlQuery m = QSqlQuery(s, db);
			if (m.next())
			{
				p0id = m.value(0).toString().trimmed();
				const QDate tmp1 =
					QDate::fromString(
						m.value(1).toString(),
						QString("yyyyMMdd"));
				series0.study_date = tmp1.toString(QString("d MMM yyyy"));
				series0.study = m.value(2).toString().trimmed(); 
			}
			m.clear();
		}
		{
			const QString s =
				QString(
					"select PatientsName,PatientsBirthDate"
					" from Patients where UID = ") + p0id;
			QSqlQuery m = QSqlQuery(s, db);
			if (m.next())
			{
				series0.patient = m.value(0).toString().trimmed();
				const QDate tmp1 =
					QDate::fromString(
						m.value(1).toString(),
						QString("yyyy-MM-dd"));
				series0.birthdate = tmp1.toString(QString("d MMM yyyy"));
			}
			m.clear();
		}
		{
			const QString s =
				QString(
					"select Filename"
					" from Images where SeriesInstanceUID = \"") +
				series0.UID + QString("\"");
			QSqlQuery m = QSqlQuery(s, db);
			while (m.next())
			{
				series0.files.push_back(
						m.value(0).toString().trimmed());
			}
			m.clear();
		}
		series.push_back(series0);
	}
	directory_lineEdit->setText(dbfile);
	if (series.empty())
	{
		db.close();
		goto quit__;
	}
	for (int x = 0; x < series.size(); ++x)
	{
		const int idx = tableWidget->rowCount();
		QString ids;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		ids = QString::asprintf("%010d", idx);
#else
		ids.sprintf("%010d", idx);
#endif
		TableWidgetItem * i = new TableWidgetItem(ids);
		for (int z = 0; z < series.at(x).files.size(); ++z)
			i->files.push_back(series.at(x).files.at(z));
		tableWidget->setRowCount(idx+1);
		tableWidget->setItem(idx,0,static_cast<QTableWidgetItem*>(i));
#if 0
		if (series.at(x).eye)
			tableWidget->setItem(idx,1,new QTableWidgetItem(eye_icon,QString("")));
		else if (series.at(x).eye2)
			tableWidget->setItem(idx,1,new QTableWidgetItem(eye2_icon,QString("")));
#endif
		tableWidget->setItem(idx,2,new QTableWidgetItem(series.at(x).modality));
		tableWidget->setItem(idx,3,new QTableWidgetItem(series.at(x).patient));
		tableWidget->setItem(idx,4,new QTableWidgetItem(series.at(x).birthdate));
		tableWidget->setItem(idx,5,new QTableWidgetItem(series.at(x).study));
		tableWidget->setItem(idx,6,new QTableWidgetItem(series.at(x).study_date));
		tableWidget->setItem(idx,7,new QTableWidgetItem(series.at(x).series));
		tableWidget->setItem(idx,8,new QTableWidgetItem(series.at(x).series_date));
		tableWidget->setItem(idx,9,new QTableWidgetItem(QVariant(i->files.size()).toString()));
	}
	db.close();
quit__:
	QApplication::restoreOverrideCursor();
	if (!warning.isEmpty())
	{
		QMessageBox mbox;
		mbox.setIcon(QMessageBox::Warning);
		mbox.setText(warning);
		mbox.exec();
	}
}
#endif

