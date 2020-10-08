#include "anonymazerwidget2.h"
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QSet>
#include <QDateTime>
#include <QMimeData>
#include <QUrl>
#include "mdcmUIDGenerator.h"
#include "mdcmGlobal.h"
#include "mdcmDicts.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmFile.h"
#include "mdcmMediaStorage.h"
#include "mdcmTag.h"
#include "mdcmReader.h"
#include "mdcmWriter.h"
#include "mdcmDataSetHelper.h"
#include "mdcmAttribute.h"
#include "mdcmVersion.h"
#include "mdcmImageReader.h"
#include "mdcmImage.h"
#include "mdcmOverlay.h"
#include "dicomutils.h"
#include "filepath.h"
#include "alizams_version.h"

static void replace__(
	mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const char * value,
	const size_t size_)
{
	if(t.GetGroup() < 0x0008) return;
	static const mdcm::Global & g    = mdcm::GlobalInstance;
	static const mdcm::Dicts & dicts = g.GetDicts();
	if (t.IsPrivate()) return;
	mdcm::VL vl = static_cast<mdcm::VL::Type>(size_);
	const mdcm::DictEntry &dictentry = dicts.GetDictEntry(t);
	if (dictentry.GetVR() == mdcm::VR::INVALID ||
		dictentry.GetVR() == mdcm::VR::UN ||
		dictentry.GetVR() == mdcm::VR::SQ)
	{
		if( dictentry.GetVR() == mdcm::VR::SQ && vl == 0 && value && *value == 0 )
		{
			mdcm::DataElement de(t);
			de.SetVR( mdcm::VR::SQ);
			ds.Replace(de);
		}
		else
		{
			std::cout
				<< "Cannot process tag: " << t << " with vr: "
				<< dictentry.GetVR() << std::endl;
		}
	}
	else if (dictentry.GetVR() & mdcm::VR::VRBINARY)
	{
		if (vl==0)
		{
			mdcm::DataElement de(t);
			if( ds.FindDataElement(t))
			{
				de.SetVR(ds.GetDataElement(t).GetVR());
			}
			else
			{
				de.SetVR(dictentry.GetVR());
			}
			de.SetByteValue("", 0);
			ds.Replace(de);
		}
		else
		{
			std::cout
				<<
					"You need to explicitely specify "
					"the length for this type of vr: "
				<< dictentry.GetVR() << std::endl;
		}
	}
	else
	{
		if (value)
		{
			std::string padded(value, vl);
			// ASCII VR needs to be padded with a space
			if(vl.IsOdd())
			{
				if(dictentry.GetVR() == mdcm::VR::UI)
				{
					// \0 is automatically added when using a ByteValue
				}
				else
				{
					padded += " ";
				}
			}
			mdcm::DataElement de(t);
			if(ds.FindDataElement(t))
			{
				de.SetVR(ds.GetDataElement(t).GetVR());
			}
			else
			{
				de.SetVR(dictentry.GetVR());
			}
			const mdcm::VL paddedSize =
				static_cast<mdcm::VL::Type>(padded.size());
			de.SetByteValue(padded.c_str(), paddedSize);
			ds.Replace(de);
		}
	}
}

static void replace_map_recurs__(
	const mdcm::File & f,
	mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const QMap<QString, QString> & m)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for(; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		if(de1.GetTag() == t)
		{
			QString s;
			if (DicomUtils::get_string_value(ds, t, s))
			{
				if (m.contains(s))
				{
					const QString v = m.value(s);
					replace__(ds, t, v.toLatin1().constData(), v.size());
				}
				else
				{
					replace__(ds, t, "", 0);
				}
			}
			else
			{
				replace__(ds, t, "", 0);
			}
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(f, ds, de.GetTag());
			if(vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if(sq && sq->GetNumberOfItems()>0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for(mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						replace_map_recurs__(f, nested, t, m);
					}
					mdcm::DataElement de_dup = *dup;
					de_dup.SetValue(*sq);
					de_dup.SetVLToUndefined();
					ds.Replace(de_dup);
				}
			}
		}
	}
}

static void remove_recurs__(
	const mdcm::File & f,
	mdcm::DataSet & ds,
	const mdcm::Tag & t)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for(; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		if(de1.GetTag() == t)
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(f, ds, de.GetTag());
			if(vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if(sq && sq->GetNumberOfItems()>0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for(mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_recurs__(f, nested, t);
					}
					mdcm::DataElement de_dup = *dup;
					de_dup.SetValue(*sq);
					de_dup.SetVLToUndefined();
					ds.Replace(de_dup);
				}
			}
		}
	}
}

// TODO modified date option
static void remove_date_time_recurs__(
	const mdcm::File & f,
	mdcm::DataSet & ds)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for(; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		mdcm::VR vr1 = mdcm::DataSetHelper::ComputeVR(f, ds, de1.GetTag());
		if (de1.GetTag() == mdcm::Tag(0x0008,0x0021))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0021), "", 0);
		}
		else if (de1.GetTag() == mdcm::Tag(0x0008,0x0031))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0031), "", 0);
		}
		else if (de1.GetTag() == mdcm::Tag(0x0008,0x0020))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0020), "", 0);
		}
		else if (de1.GetTag() == mdcm::Tag(0x0008,0x0030))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0030), "", 0);
		}
		else if (
			vr1 == mdcm::VR::DA ||
			vr1 == mdcm::VR::DT ||
			vr1 == mdcm::VR::TM)
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(f, ds, de.GetTag());
			if(vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if(sq && sq->GetNumberOfItems()>0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for(mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_date_time_recurs__(f, nested);
					}
					mdcm::DataElement de_dup = *dup;
					de_dup.SetValue(*sq);
					de_dup.SetVLToUndefined();
					ds.Replace(de_dup);
				}
			}
		}
	}
}

static void empty_recurs__(
	const mdcm::File & f,
	mdcm::DataSet & ds,
	const mdcm::Tag & t)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for(; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		if(de1.GetTag() == t)
		{
			replace__(ds, t, "", 0);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(f, ds, de.GetTag());
			if(vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if(sq && sq->GetNumberOfItems()>0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for(mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						empty_recurs__(f, nested, t);
					}
					mdcm::DataElement de_dup = *dup;
					de_dup.SetValue(*sq);
					de_dup.SetVLToUndefined();
					ds.Replace(de_dup);
				}
			}
		}
	}
}

static void remove_private__(const mdcm::File & file, mdcm::DataSet & ds)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for(;it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		if(de1.GetTag().IsPrivate())
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(file, ds, de.GetTag());
			if(vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if(sq && sq->GetNumberOfItems()>0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem( i );
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_private__(file, nested);
					}
					mdcm::DataElement de_dup = *dup;
					de_dup.SetValue(*sq);
					de_dup.SetVLToUndefined();
					ds.Replace(de_dup);
				}
			}
		}
	}
}

static void remove_overlays__(const mdcm::File & file, mdcm::DataSet & ds)
{
	std::vector<mdcm::Tag> tmp0;
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End(); ++it)
	{
		const mdcm::DataElement & de = *it;
		const mdcm::Tag t = de.GetTag();
		if ((t.GetGroup() >= 0x6000) && (t.GetGroup()<=0x601E) && (t.GetGroup()%2==0))
			tmp0.push_back(t);
	}
	for (unsigned int x = 0; x < tmp0.size(); x++) ds.Remove(tmp0.at(x));
}

static void remove_curves__(const mdcm::File & file, mdcm::DataSet & ds)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	std::vector<mdcm::Tag> tmp0;
	for(; it != ds.End(); ++it)
	{
		const mdcm::DataElement & de = *it;
		const mdcm::Tag t = de.GetTag();
		if ((t.GetGroup() >= 0x5000) && (t.GetGroup()<=0x501e) && (t.GetGroup()%2==0))
			tmp0.push_back(t);

	}
	for (unsigned int x = 0; x < tmp0.size(); x++) ds.Remove(tmp0.at(x));
}

static void remove_group_length__(const mdcm::File & file, mdcm::DataSet & ds)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for(; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		if(de1.GetTag().IsGroupLength())
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(file, ds, de.GetTag());
			if(vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if(sq && sq->GetNumberOfItems()>0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for(mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_group_length__(file, nested);
					}
					mdcm::DataElement de_dup = *dup;
					de_dup.SetValue(*sq);
					de_dup.SetVLToUndefined();
					ds.Replace(de_dup);
				}
			}
		}
	}
}

static void anonymize_file__(
	bool * ok,
	bool * overlay_in_data,
	QString filename,
	QString outfilename,
	const std::set<mdcm::Tag> & pn_tags,
	const std::set<mdcm::Tag> & uid_tags,
	const std::set<mdcm::Tag> & empty_tags,
	const std::set<mdcm::Tag> & remove_tags,
	const std::set<mdcm::Tag> & device_tags,
	const std::set<mdcm::Tag> & patient_tags,
	const std::set<mdcm::Tag> & institution_tags,
	bool preserve_uids,
	bool remove_private,
	bool remove_graphics,
	bool retain_dates_times,
	bool retain_device_id,
	bool retain_patient_chars,
	bool retain_institution_id,
	const QMap<QString, QString> & uids_map,
	const QMap<QString, QString> & pn_map)
{
	mdcm::Reader reader;
	reader.SetFileName(FilePath::getPath(filename));
	if(!reader.Read())
	{
		*ok = false;
		return;
	}
	mdcm::File    & file = reader.GetFile();
	mdcm::DataSet & ds   = file.GetDataSet();
	mdcm::FileMetaInformation & header = file.GetHeader();
#if 1
	mdcm::MediaStorage ms;
	ms.SetFromFile(file);
	if(ms == mdcm::MediaStorage::MediaStorageDirectoryStorage)
	{
		*ok = true;
		return;
	}
#endif
	//
	if (remove_private)
	{
		remove_private__(file, ds);
	}
	///
	if (remove_graphics)
	{
		remove_curves__(file, ds);
		//
		//
		//
		{
			mdcm::ImageReader image_reader;
			image_reader.SetFileName(FilePath::getPath(filename));
			if (image_reader.Read())
			{
				const mdcm::Image image = image_reader.GetImage();
				for (unsigned int ov = 0; ov < image.GetNumberOfOverlays(); ov++)
				{
					const mdcm::Overlay & overlay = image.GetOverlay(ov);
					if (overlay.IsInPixelData())
					{
						*overlay_in_data = true;
						break;
					}
				}
			}
		}
		//
		//
		//
		remove_overlays__(file, ds);
		remove_recurs__(file, ds, mdcm::Tag(0x0070,0x0001));
	}
	//
	{
		std::set<mdcm::Tag>::const_iterator it = empty_tags.begin();
		for(; it != empty_tags.end(); ++it)
		{
			empty_recurs__(file, ds, *it);
		}
	}
	//
	{
		std::set<mdcm::Tag>::const_iterator it = remove_tags.begin();
		for(it = remove_tags.begin(); it != remove_tags.end(); ++it)
		{
			remove_recurs__(file, ds, *it);
		}
	}
	//
	if (!retain_device_id)
	{
		std::set<mdcm::Tag>::const_iterator it = device_tags.begin();
		for(it = device_tags.begin(); it != device_tags.end(); ++it)
		{
			remove_recurs__(file, ds, *it);
		}
	}
	//
	if (!retain_patient_chars)
	{
		std::set<mdcm::Tag>::const_iterator it = patient_tags.begin();
		for(it = patient_tags.begin(); it != patient_tags.end(); ++it)
		{
			remove_recurs__(file, ds, *it);
		}
	}
	//
	if (!retain_institution_id)
	{
		std::set<mdcm::Tag>::const_iterator it = institution_tags.begin();
		for(it = institution_tags.begin(); it != institution_tags.end(); ++it)
		{
			remove_recurs__(file, ds, *it);
		}
	}
	//
	if (!retain_dates_times)
	{
		remove_date_time_recurs__(file, ds);
	}
	//
	if (!preserve_uids)
	{
		std::set<mdcm::Tag>::const_iterator it =
			uid_tags.begin();
		for(; it != uid_tags.end(); ++it)
		{
			replace_map_recurs__(file, ds, *it, uids_map);
		}
	}
	//
	{
		std::set<mdcm::Tag>::const_iterator it = pn_tags.begin();
		for(it = pn_tags.begin(); it != pn_tags.end(); ++it)
		{
			replace_map_recurs__(file, ds, *it, pn_map);
		}
	}
	//
	remove_group_length__(file, ds);
	//
	const QString s0("YES");
	const QString s1("");
	replace__(ds, mdcm::Tag(0x0012,0x0062), s0.toLatin1().constData(), s0.toLatin1().length());
	replace__(ds, mdcm::Tag(0x0012,0x0063), s1.toLatin1().constData(), s1.toLatin1().length());
	//
	*ok = true;
	//
	mdcm::Attribute<0x0002,0x0012, mdcm::VR::UI> impl_uid;
	const QString impl_uid_s =
		QVariant(ALIZAMS_ROOT_UID).toString() +
		QString(".0.0.0.0.") +
		QVariant(ALIZAMS_VERSION).toString();
	impl_uid.SetValue(impl_uid_s.toLatin1().constData());
	header.Replace(impl_uid.GetAsDataElement());

	QString name_s = QString("AlizaMS ") +
		QVariant(ALIZAMS_VERSION).toString();
	if (name_s.length()%2!=0) name_s.append(QString(" "));
	mdcm::Attribute<0x0002,0x0013, mdcm::VR::SH> impl_name;
	impl_name.SetValue(name_s.toLatin1().constData());
	header.Replace(impl_name.GetAsDataElement());

	mdcm::Attribute<0x0002,0x0016, mdcm::VR::AE> app_entity;
	app_entity.SetValue(QString("ALIZAMSAE ").toLatin1().constData());
	header.Replace(app_entity.GetAsDataElement());
	//
	mdcm::Writer writer;
	writer.SetFileName(FilePath::getPath(outfilename));
	writer.SetFile(file);
	if(!writer.Write()) *ok = false;
}

AnonymazerWidget2::AnonymazerWidget2(float si, QWidget * p, Qt::WindowFlags f) : QWidget(p,f)
{
	setupUi(this);
	const QSize s = QSize((int)(24*si),(int)(24*si));
	in_pushButton->setIconSize(s);
	out_pushButton->setIconSize(s);
	run_pushButton->setIconSize(s);
	mdates_radioButton->hide();
	run_pushButton->setEnabled(false);
	readSettings();
	init_profile();
#ifdef USE_WORKSTATION_MODE
	input_dir = QString("");
#else
#if (defined _WIN32)
	input_dir = 
		QString(".") +
		QDir::separator() +
		QString("DICOM");
#else
	input_dir = 
		QApplication::applicationDirPath() +
		QDir::separator() + QString("..") +
		QDir::separator() +
		QString("DICOM");
#endif
	QFileInfo fi(input_dir);
	if (fi.exists()) input_dir = fi.absoluteFilePath();
	else             input_dir = QString("");
	in_lineEdit->setText(QDir::toNativeSeparators(input_dir));
#endif
	connect(out_pushButton,SIGNAL(clicked()),this,SLOT(set_output_dir()));
	connect(in_pushButton,SIGNAL(clicked()),this,SLOT(set_input_dir()));
	connect(run_pushButton,SIGNAL(clicked()),this,SLOT(run_()));
}

AnonymazerWidget2::~AnonymazerWidget2()
{
}

void AnonymazerWidget2::closeEvent(QCloseEvent * e)
{
	e->accept();
}

void AnonymazerWidget2::readSettings()
{
	QSettings settings(
		QSettings::IniFormat, QSettings::UserScope,
		QApplication::organizationName(), QApplication::applicationName());
	settings.setFallbacksEnabled(true);
	settings.beginGroup(QString("AnonymazerWidget"));
	output_dir      = settings.value(QString("output_dir"), QString("")).toString();
	const int tmp1  = settings.value(QString("remove_private"),        0).toInt();
	const int tmp5  = settings.value(QString("remove_overlays"),       0).toInt();
	const int tmp7  = settings.value(QString("preserve_uids"),         0).toInt();
	const int tmp8  = settings.value(QString("retain_dates_times"),    0).toInt();
	const int tmp9  = settings.value(QString("retain_device_id"),      0).toInt();
	const int tmp10 = settings.value(QString("retain_patient_chars"),  0).toInt();
	const int tmp11 = settings.value(QString("retain_institution_id"), 0).toInt();
	settings.endGroup();
	if (tmp1==1) private_radioButton->setChecked(true);
	else private_radioButton->setChecked(false);
	if (tmp5==1) graphics_radioButton->setChecked(true);
	else graphics_radioButton->setChecked(false);
	if (tmp7==1) uids_radioButton->setChecked(true);
	else uids_radioButton->setChecked(false);
	if (tmp8==1) dates_radioButton->setChecked(true);
	else dates_radioButton->setChecked(false);
	if (tmp9==1) device_radioButton->setChecked(true);
	else device_radioButton->setChecked(false);
	if (tmp10==1) chars_radioButton->setChecked(true);
	else chars_radioButton->setChecked(false);
	if (tmp11==1) institution_radioButton->setChecked(true);
	else institution_radioButton->setChecked(false);
}

void AnonymazerWidget2::writeSettings(QSettings & settings)
{
	const int tmp1  = (private_radioButton->isChecked())     ? 1 : 0;
	const int tmp5  = (graphics_radioButton->isChecked())    ? 1 : 0;
	const int tmp7  = (uids_radioButton->isChecked())        ? 1 : 0;
	const int tmp8  = (dates_radioButton->isChecked())       ? 1 : 0;
	const int tmp9  = (device_radioButton->isChecked())      ? 1 : 0;
	const int tmp10 = (chars_radioButton->isChecked())       ? 1 : 0;
	const int tmp11 = (institution_radioButton->isChecked()) ? 1 : 0;
	settings.beginGroup(QString("AnonymazerWidget"));
	settings.setValue(QString("output_dir"),           QVariant(output_dir));
	settings.setValue(QString("remove_private"),       QVariant(tmp1));
	settings.setValue(QString("remove_overlays"),      QVariant(tmp5));
	settings.setValue(QString("preserve_uids"),        QVariant(tmp7));
	settings.setValue(QString("retain_dates_times"),   QVariant(tmp8));
	settings.setValue(QString("retain_device_id"),     QVariant(tmp9));
	settings.setValue(QString("retain_patient_chars"), QVariant(tmp10));
	settings.setValue(QString("retain_institution_id"),QVariant(tmp11));
	settings.endGroup();
}

void AnonymazerWidget2::dropEvent(QDropEvent * e)
{
	const QMimeData * mimeData = e->mimeData();
	if (!mimeData) return;
	QStringList l;
	if (mimeData->hasUrls())
	{
		QList<QUrl> urls = mimeData->urls();
		for (int i = 0; i < urls.size() && i < 1; ++i)
			l.append(urls.at(i).toLocalFile());
	}
	if (l.size() < 1) return;
	QFileInfo fi(l.at(0));
	if (fi.isDir())
	{
		input_dir = fi.absoluteFilePath();
		in_lineEdit->setText(QDir::toNativeSeparators(input_dir));
	}
}

void AnonymazerWidget2::dragEnterEvent(QDragEnterEvent * e)
{
	e->acceptProposedAction();
}
 
void AnonymazerWidget2::dragMoveEvent(QDragMoveEvent * e)
{
	e->acceptProposedAction();
}
 
void AnonymazerWidget2::dragLeaveEvent(QDragLeaveEvent * e)
{
	e->accept();
}

static void find_pn_recurs__(
	const mdcm::File & f,
	const mdcm::DataSet & ds,
	QStringList & l)
{
	mdcm::DataSet::ConstIterator it = ds.Begin();
	for(; it != ds.End();)
	{
		const mdcm::DataElement & de = *it;
		++it;
		const mdcm::Tag t = de.GetTag();
		mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(f, ds, t);
		if(vr == mdcm::VR::PN || t == mdcm::Tag(0x0010,0x0020))
		{
			QString s;
			if (DicomUtils::get_string_value(ds, t, s)) l.push_back(s);
		}
		else if (vr.Compatible(mdcm::VR::SQ))
		{
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
			if(sq && sq->GetNumberOfItems()>0)
			{
				const mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
				for(mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
				{
					const mdcm::Item    & item   = sq->GetItem(i);
					const mdcm::DataSet & nested = item.GetNestedDataSet();
					find_pn_recurs__(f, nested, l);
				}
			}
		}
	}
}

static void find_uids_recurs__(
	const mdcm::File & f,
	const mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	QStringList & l)
{
	mdcm::DataSet::ConstIterator it = ds.Begin();
	for(; it != ds.End();)
	{
		const mdcm::DataElement & de = *it;
		++it;
		const mdcm::Tag t = de.GetTag();
		if(ts.find(t) != ts.end())
		{
			QString s;
			if (DicomUtils::get_string_value(ds, t, s)) l.push_back(s);
		}
		else
		{
			mdcm::VR vr = mdcm::DataSetHelper::ComputeVR(f, ds, t);
			if(vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if(sq && sq->GetNumberOfItems()>0)
				{
					const mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for(mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						const mdcm::Item    & item   = sq->GetItem(i);
						const mdcm::DataSet & nested = item.GetNestedDataSet();
						find_uids_recurs__(f, nested, ts, l);
					}
				}
			}
		}
	}
}

static QString generate_uid()
{
	mdcm::UIDGenerator g;
	const QString r = QString::fromLatin1(g.Generate());
	return r;
}

static void build_maps(
	const QStringList & l,
	const std::set<mdcm::Tag> & uid_tags,
	QMap<QString, QString> & m0,
	QMap<QString, QString> & m1,
	QProgressDialog * pd)
{
	QStringList uids;
	QStringList pids;
	for (int x = 0; x < l.size(); x++)
	{
		QApplication::processEvents();
		pd->setValue(-1);
		if (pd->wasCanceled()) return;
		mdcm::Reader reader;
		reader.SetFileName(FilePath::getPath(l.at(x)));
		if (!reader.Read()) continue;
		const mdcm::File    & f  = reader.GetFile();
		const mdcm::DataSet & ds = f.GetDataSet();
		if (ds.IsEmpty()) continue;
		find_uids_recurs__(f, ds, uid_tags, uids);
		find_pn_recurs__(f, ds, pids);
	}
	QSet<QString> uset = uids.toSet();
	uids.clear();
	QSetIterator<QString> it0(uset);
	while (it0.hasNext())
	{
		QApplication::processEvents();
		pd->setValue(-1);
		if (pd->wasCanceled()) return;
		const QString s = it0.next();
		const QString v = generate_uid();
		m0[s] = v;
		//std::cout << s.toStdString() << " --> " << v.toStdString() << std::endl;
	}
	uset.clear();
	QSet<QString> pset = pids.toSet();
	pids.clear();
	QSetIterator<QString> it1(pset);
	while (it1.hasNext())
	{
		QApplication::processEvents();
		pd->setValue(-1);
		if (pd->wasCanceled()) return;
		const QString s = it1.next();
		const QString v = DicomUtils::generate_id();
		m1[s] = v;
		//std::cout << s.toStdString() << " --> " << v.toStdString() << std::endl;
	}
	pset.clear();
}

void AnonymazerWidget2::process_directory(
	const QString & p,
	const QString & outp,
	const QMap<QString,QString> & uids_m,
	const QMap<QString,QString> & pn_m,
	unsigned int * count_overlay_in_data,
	unsigned int * count_errors,
	QProgressDialog * pd)
{
	const bool preserve_uids         = uids_radioButton->isChecked();
	const bool remove_private        = private_radioButton->isChecked();
	const bool remove_graphics       = graphics_radioButton->isChecked();
	const bool retain_dates_times    = dates_radioButton->isChecked();
	const bool retain_device_id      = device_radioButton->isChecked();
	const bool retain_patient_chars  = chars_radioButton->isChecked();
	const bool retain_institution_id = institution_radioButton->isChecked();
	QDir dir(p);
	QStringList dlist = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	QStringList flist = dir.entryList(QDir::Files|QDir::Readable,QDir::Name);
	QStringList filenames;
	for (int x = 0; x < flist.size(); x++)
	{
		pd->setValue(-1);
		QApplication::processEvents();
		if (pd->wasCanceled()) return;
		const QString tmp0 = dir.absolutePath() + QDir::separator() + flist.at(x);
		filenames.push_back(tmp0);
	}
	flist.clear();
	for(int i = 0; i < filenames.size(); ++i)
	{
		pd->setValue(-1);
		QApplication::processEvents();
		if (pd->wasCanceled()) return;
#if 0
		QString tmp200;
		tmp200.sprintf("%08d", i);
		QDateTime date_ = QDateTime::currentDateTime();
		const QString date_str = date_.toString(QString("hhmmss")); // FIXME
		const QString out_filename = date_str + QString("-") + tmp200 + QString(".dcm");
#else
		QFileInfo fi(filenames.at(i));
		const QString out_filename = fi.fileName();
#endif
		const QString out_file = 
			outp +
			QDir::separator() +
			out_filename;
		bool ok_ = false;
		bool overlay_in_data = false;
		anonymize_file__(
			&ok_,
			&overlay_in_data,
			filenames.at(i),
			out_file,
			pn_tags,
			uid_tags,
			empty_tags,
			remove_tags,
			device_tags,
			patient_tags,
			institution_tags,
			preserve_uids,
			remove_private,
			remove_graphics,
			retain_dates_times,
			retain_device_id,
			retain_patient_chars,
			retain_institution_id,
			uids_m,
			pn_m);
		if (overlay_in_data)
		{
			(*count_overlay_in_data)++;
		}
		if (!ok_)
		{
			(*count_errors)++;
		}
		QApplication::processEvents();
	}
	filenames.clear();
	for (int j = 0; j < dlist.size(); j++)
	{
		QApplication::processEvents();
		QDir d(outp + QDir::separator() + dlist.at(j));
		if (!d.exists()) d.mkpath(d.absolutePath());
		process_directory(
			dir.absolutePath() + QDir::separator() + dlist.at(j),
			d.absolutePath(),
			uids_m,
			pn_m,
			count_overlay_in_data,
			count_errors,
			pd);
	}
	dlist.clear();
}

void AnonymazerWidget2::run_()
{
	const QString out_path = dir_lineEdit->text();
	if (out_path.isEmpty())
	{
		run_pushButton->setEnabled(false);
		dir_lineEdit->setText(QString(
			"Select output directory"));
		return;
	}
	const QString in_path = in_lineEdit->text();
	if (in_path.isEmpty())
	{
		in_lineEdit->setText(QString(
			"Select input directory"));
		return;
	}
	else if (in_path == QString("Select input directory"))
	{
		return;
	}
	if (out_path == in_path)
	{
		QMessageBox mb;
		mb.setWindowModality(Qt::ApplicationModal);
		mb.setText(QString(
			"Error: input and output are the same directory"));
		mb.exec();
		return;
	}
	const QString root_uid = QString::fromLatin1(ALIZAMS_ROOT_UID);
	mdcm::UIDGenerator::SetRoot(root_uid.toLatin1().constData());
	QMap<QString, QString> uids_m;
	QMap<QString, QString> pn_m;
	QProgressDialog * pd =
		new QProgressDialog(QString("De-identifying"),tr("Cancel"),0,0);
	pd->setWindowModality(Qt::ApplicationModal);
	pd->setWindowFlags(
		pd->windowFlags()^Qt::WindowContextHelpButtonHint);
	pd->show();
	{
		QStringList filenames;
		QDir dir(in_path);
		QDirIterator it(dir, QDirIterator::Subdirectories);
		while (it.hasNext())
		{
			QFileInfo fi(it.next());
			if (fi.isFile())
			{
				filenames.push_back(fi.absoluteFilePath());
			}
			pd->setValue(-1);
			QApplication::processEvents();
			if (pd->wasCanceled()) return;
		}
		build_maps(filenames, uid_tags, uids_m, pn_m, pd);
		filenames.clear();
	}
	unsigned int count_overlay_in_data = 0;
	unsigned int count_errors = 0;
	process_directory(
		in_path,
		out_path,
		uids_m,
		pn_m,
		&count_overlay_in_data,
		&count_errors,
		pd);
	QString message("");
	if (count_errors > 0)
	{
		message = QString("Warning:\n") + QVariant(count_errors).toString() +
			QString(" file(s) failed\n");
	}
	if (count_overlay_in_data > 0)
	{
		message.append(QString("\nWarning: some files may contain overlays in pixel data\n"));
	}
	pd->close();
	delete pd;
	QApplication::processEvents();
	if (!message.isEmpty())
	{
		QMessageBox mbox;
		mbox.setWindowModality(Qt::ApplicationModal);
		mbox.addButton(QMessageBox::Ok);
		mbox.setIcon(QMessageBox::Information);
		mbox.setText(message);
		mbox.exec();
	}
	dir_lineEdit->setText(QString("Select output directory"));
	in_lineEdit->setText(QString("Select input directory"));
	run_pushButton->setEnabled(false);
}

void AnonymazerWidget2::set_output_dir()
{
	const QString dirname= QFileDialog::getExistingDirectory(
		this,
		QString("Select Output Directory"),
		output_dir,
		(QFileDialog::ShowDirsOnly
		/* | QFileDialog::DontUseNativeDialog*/
		));
	if (dirname.isEmpty())
	{
		dir_lineEdit->setText(QString("Select output directory"));
		return;
	}
	output_dir = dirname;
	dir_lineEdit->setText(QDir::toNativeSeparators(output_dir));
	run_pushButton->setEnabled(true);
}

void AnonymazerWidget2::set_input_dir()
{
	const QString dirname= QFileDialog::getExistingDirectory(
		this,
		QString("Select Input Directory"),
		input_dir,
		(QFileDialog::ShowDirsOnly
		/* | QFileDialog::DontUseNativeDialog*/
		));
	if (dirname.isEmpty())
	{
		in_lineEdit->setText(QString("Select input directory"));
		return;
	}
	input_dir = dirname;
	in_lineEdit->setText(QDir::toNativeSeparators(input_dir));
}

void AnonymazerWidget2::init_profile()
{
	empty_tags.insert(mdcm::Tag(0x0008,0x0050));// z
	remove_tags.insert(mdcm::Tag(0x0018,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0555));// x
	//mdcm::Tag(0x0008,0x0022);// date/time x/z
	//mdcm::Tag(0x0008,0x002A);// date/time x/d
	remove_tags.insert(mdcm::Tag(0x0018,0x1400));// x/d
	remove_tags.insert(mdcm::Tag(0x0018,0x9424));// x
	//mdcm::Tag(0x0008,0x0032);// date/time x/z
	remove_tags.insert(mdcm::Tag(0x0040,0x4035));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x21B0));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xA353));// x	
	remove_tags.insert(mdcm::Tag(0x0038,0x0010));// x
	//mdcm::Tag(0x0038,0x0020);// date/time x
	remove_tags.insert(mdcm::Tag(0x0008,0x1084));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1080));// x
	//mdcm::Tag(0x0038,0x0021);// date/time x
	remove_tags.insert(mdcm::Tag(0x0010,0x2110));// x
	remove_tags.insert(mdcm::Tag(0x4000,0x0010));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xA078));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1081));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1007));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0280));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x9161));// u
	remove_tags.insert(mdcm::Tag(0x0040,0x3001));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x009D)); // x
	pn_tags.insert(mdcm::Tag(0x0008,0x009C));// z
	pn_tags.insert(mdcm::Tag(0x0070,0x0084));// z
	remove_tags.insert(mdcm::Tag(0x0070,0x0086));// x
	//mdcm::Tag(0x0008,0x0023); // date/time z/d
	remove_tags.insert(mdcm::Tag(0x0040,0xA730));// x
	//mdcm::Tag(0x0008,0x0033); // date/time z/d
	empty_tags.insert(mdcm::Tag(0x0018,0x0010));// z/d
	remove_tags.insert(mdcm::Tag(0x0018,0xA003));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2150));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xA307));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x0300));// x
	//mdcm::Tag(0x0008,0x0025);// date/time x
	//mdcm::Tag(0x0008,0x0035);// date/time x
	remove_tags.insert(mdcm::Tag(0x0040,0xA07C));// x
	remove_tags.insert(mdcm::Tag(0xFFFC,0xFFFC));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x2111));// x
	device_tags.insert(mdcm::Tag(0x0018,0x700A));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1000));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1002));// u
	remove_tags.insert(mdcm::Tag(0x0400,0x0100));// x
	remove_tags.insert(mdcm::Tag(0xFFFA,0xFFFA));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x9164));// u
	remove_tags.insert(mdcm::Tag(0x0038,0x0040));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x011A));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0119));// x
	uid_tags.insert(mdcm::Tag(0x300A,0x0013));// u
	patient_tags.insert(mdcm::Tag(0x0010,0x2160));// x
	remove_tags.insert(mdcm::Tag(0x0040,4011)); // x
	//mdcm::Tag(0x0008,0x0058);// u FIXME
	uid_tags.insert(mdcm::Tag(0x0070,0x031A));// u
	empty_tags.insert(mdcm::Tag(0x0040,0x2017));// z
	remove_tags.insert(mdcm::Tag(0x0020,0x9158));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x0052));// u
	device_tags.insert(mdcm::Tag(0x0018,0x1008));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1005));// x 
	//mdcm::Tag(0x0070,0x0001);// d s. anonymize_file__() 
	remove_tags.insert(mdcm::Tag(0x0040,0x4037));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x4036));// x
	remove_tags.insert(mdcm::Tag(0x0088,0x0200));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x0020,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x0028,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x2400));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0300));// x
	//mdcm::Tag(0x0008,0x0014));// date/time x
	uid_tags.insert(mdcm::Tag(0x0008,0x0014));// u
	institution_tags.insert(mdcm::Tag(0x0008,0x0081));// u
	institution_tags.insert(mdcm::Tag(0x0008,0x0082));//
	institution_tags.insert(mdcm::Tag(0x0008,0x0080));// x/z/d
	institution_tags.insert(mdcm::Tag(0x0008,0x1040));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1050));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1011));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0111));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x010C));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0115));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0202));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0102));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x010B));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x010A));// x
	uid_tags.insert(mdcm::Tag(0x0008,0x3010));// u
	remove_tags.insert(mdcm::Tag(0x0038,0x0011));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x0021));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x0061));// x
	uid_tags.insert(mdcm::Tag(0x0028,0x1214));// u
	//mdcm::Tag(0x0010,0x21D0);// date/time x
	remove_tags.insert(mdcm::Tag(0x0400,0x0404));// x
	//mdcm::Tag(0x0002,0x0003)
	remove_tags.insert(mdcm::Tag(0x0010,0x2000));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1090));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1080));// x
	remove_tags.insert(mdcm::Tag(0x0400,0x0550));// x
	remove_tags.insert(mdcm::Tag(0x0020,0x3406));// x
	remove_tags.insert(mdcm::Tag(0x0020,0x3401));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1060));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1010));// x
	// mdcm::Tag(0x0040,0xA192) // date/time x
	uid_tags.insert(mdcm::Tag(0x0040,0xA402));// u
	// mdcm::Tag(0x0040,0xA193) // date/time x
	uid_tags.insert(mdcm::Tag(0x0040,0xA171));// u
	remove_tags.insert(mdcm::Tag(0x0010,0x2180));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1072));// x
	pn_tags.insert(mdcm::Tag(0x0008,0x1070));// x
	remove_tags.insert(mdcm::Tag(0x0400,0x0561));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x2010));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x2008));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x2009));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1000));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1002));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1001));// x
	//mdcm::Tag(0x0008,0x0024);// date/time x
	//mdcm::Tag(0x0008,0x0034);// date/time x
	uid_tags.insert(mdcm::Tag(0x0028,0x1199));// u
	remove_tags.insert(mdcm::Tag(0x0040,0xA07A));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1040));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x4000));// x
	pn_tags.insert(mdcm::Tag(0x0010,0x0020));
	patient_tags.insert(mdcm::Tag(0x0010,0x2203));// x/z
	remove_tags.insert(mdcm::Tag(0x0038,0x0500));// x 
	remove_tags.insert(mdcm::Tag(0x0040,0x1004));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x1010));// x
	empty_tags.insert(mdcm::Tag(0x0010,0x0030));// z
	remove_tags.insert(mdcm::Tag(0x0010,0x1005));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x0032));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x0400));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x0050));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1060));// x
	pn_tags.insert(mdcm::Tag(0x0010,0x0010));// z
	remove_tags.insert(mdcm::Tag(0x0010,0x0101));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x0102));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x21F0));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x0040));// z
	patient_tags.insert(mdcm::Tag(0x0010,0x1020));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2155));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2154));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x1030));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0243));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0254));// x
	//mdcm::Tag(0x0040,0x0250)// date/time x
	//mdcm::Tag(0x0040,0x4051)// date/time x
	//mdcm::Tag(0x0040,0x0251);// date/time x
	remove_tags.insert(mdcm::Tag(0x0040,0x0253));// x
	//mdcm::Tag(0x0040,0x0244);// date/time x
	//mdcm::Tag(0x0040,0x4050);// date/time x
	//mdcm::Tag(0x0040,0x0245);// date/time x
	device_tags.insert(mdcm::Tag(0x0040,0x0241));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4030));// x
	device_tags.insert(mdcm::Tag(0x0040,0x0242));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4028));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1052));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1050));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1102));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1101));// d
	remove_tags.insert(mdcm::Tag(0x0040,0xA123));// d
	remove_tags.insert(mdcm::Tag(0x0040,0x1103));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0114));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1062));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1048));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1049));// x
	empty_tags.insert(mdcm::Tag(0x0040,0x2016));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1004));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0012));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x21C0));// x
	uid_tags.insert(mdcm::Tag(0x0070,0x1101));// u
	uid_tags.insert(mdcm::Tag(0x0070,0x1102));// u
	remove_tags.insert(mdcm::Tag(0x0018,0x1030));// x
	remove_tags.insert(mdcm::Tag(0x300C,0x0113));// x	
	remove_tags.insert(mdcm::Tag(0x0040,0x2001));// x
	remove_tags.insert(mdcm::Tag(0x0032,0x1030));// x
	remove_tags.insert(mdcm::Tag(0x0400,0x0402));// x
	uid_tags.insert(mdcm::Tag(0x3006,0x0024));// u
	uid_tags.insert(mdcm::Tag(0x0040,0x4023));// u
	//mdcm::Tag(0x0008,0x1140));// x/z/u*
	uid_tags.insert(mdcm::Tag(0x0040,0xA172));// u
	remove_tags.insert(mdcm::Tag(0x0038,0x0004));// x
	//mdcm::Tag(0x0008,0x1120);// x
	//mdcm::Tag(0x0008,0x1111);// x/z/d
	remove_tags.insert(mdcm::Tag(0x0400,0x0403));// x
	uid_tags.insert(mdcm::Tag(0x0008,0x1155));// u
	uid_tags.insert(mdcm::Tag(0x0004,0x1511));// u
	//mdcm::Tag(0x0008,0x1110);// x/z
	remove_tags.insert(mdcm::Tag(0x0008,0x0092));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x0096));// x
	pn_tags.insert(mdcm::Tag(0x0008,0x0090));// z
	remove_tags.insert(mdcm::Tag(0x0008,0x0094));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2152));// x
	uid_tags.insert(mdcm::Tag(0x3006,0x00C2));// u
	remove_tags.insert(mdcm::Tag(0x0040,0x0275));// x
	remove_tags.insert(mdcm::Tag(0x0032,0x1070));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1400));// x
	remove_tags.insert(mdcm::Tag(0x0032,0x1060));// x/z
	remove_tags.insert(mdcm::Tag(0x0040,0x1001));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1005));// x
	//0x0000,0x1001 // u
	remove_tags.insert(mdcm::Tag(0x0032,0x1032));// x
	remove_tags.insert(mdcm::Tag(0x0032,0x1033));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2299));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2297));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0118));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0042));// x
	pn_tags.insert(mdcm::Tag(0x300E,0x0008));// x/z
	remove_tags.insert(mdcm::Tag(0x0040,0x4034));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x001E));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x000B));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0006));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0007));// x
	//mdcm::Tag(0x0040,0x0004);// date/time x
	//mdcm::Tag(0x0040,0x0005);// date/time x
	//mdcm::Tag(0x0040,0x0008);// date/time x
	device_tags.insert(mdcm::Tag(0x0040,0x0011));// x
	//mdcm::Tag(0x0040,0x0010);// date/time x
	//mdcm::Tag(0x0040,0x0002);// date/time x
	//mdcm::Tag(0x0040,0x4005);// date/time x
	//mdcm::Tag(0x0040,0x0003);// date/time x
	device_tags.insert(mdcm::Tag(0x0040,0x0001));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4027));// x
	device_tags.insert(mdcm::Tag(0x0040,0x0010));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4025));// x
	device_tags.insert(mdcm::Tag(0x0032,0x1020));// x
	device_tags.insert(mdcm::Tag(0x0032,0x1021));// x
	//mdcm::Tag(0x0008,0x0021);// date/time x/d
	remove_tags.insert(mdcm::Tag(0x0008,0x103E));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x000E));// u
	//mdcm::Tag(0x0008,0x0031);// date/time x/d
	remove_tags.insert(mdcm::Tag(0x0038,0x0062));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x0060));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x21A0));// x
	uid_tags.insert(mdcm::Tag(0x0008,0x0018));// u
	//mdcm::Tag(0x0008,0x2112);// x/d/u*
	device_tags.insert(mdcm::Tag(0x3008,0x0105)); // x
	remove_tags.insert(mdcm::Tag(0x0038,0x0050));// x
	device_tags.insert(mdcm::Tag(0x0008,0x1010));// x/d
	uid_tags.insert(mdcm::Tag(0x0088,0x0140));// u
	remove_tags.insert(mdcm::Tag(0x0032,0x4000));// x
	//mdcm::Tag(0x0008,0x0020);// date/time z
	empty_tags.insert(mdcm::Tag(0x0008,0x1030));// x
	empty_tags.insert(mdcm::Tag(0x0020,0x0010));// z
	remove_tags.insert(mdcm::Tag(0x0032,0x0012));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x000D));// u
	//mdcm::Tag(0x0008,0x0030);// date/time z
	uid_tags.insert(mdcm::Tag(0x0020,0x0200));// u
	uid_tags.insert(mdcm::Tag(0x0040,0xDB0D));// u
	uid_tags.insert(mdcm::Tag(0x0040,0xDB0C));// u
	remove_tags.insert(mdcm::Tag(0x4000,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x2030,0x0020));// x
	//mdcm::Tag(0x0008,0x0201);// date/time x
	remove_tags.insert(mdcm::Tag(0x0088,0x0910));// x
	remove_tags.insert(mdcm::Tag(0x0088,0x0912));// x
	remove_tags.insert(mdcm::Tag(0x0088,0x0906));// x
	remove_tags.insert(mdcm::Tag(0x0088,0x0904));//
	uid_tags.insert(mdcm::Tag(0x0008,0x1195));// u
	uid_tags.insert(mdcm::Tag(0x0040,0xA124));// u
	remove_tags.insert(mdcm::Tag(0x0040,0xA352));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xA358));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xA088));// z
	remove_tags.insert(mdcm::Tag(0x0008,0xA075));// d
	remove_tags.insert(mdcm::Tag(0x0040,0xA073));// d
	remove_tags.insert(mdcm::Tag(0x0040,0xA027));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x4000));// x
	// additional
	uid_tags.insert(mdcm::Tag(0x0008,0x010D));//
	remove_tags.insert(mdcm::Tag(0x0020,0x3404));//
	remove_tags.insert(mdcm::Tag(0x0012,0x0064));// old de-identification
}

