#include "sqtree.h"
#include <mdcmGlobal.h>
#include <mdcmPrivateTag.h>
#include <mdcmReader.h>
#include <mdcmAttribute.h>
#include <mdcmElement.h>
#include <mdcmByteValue.h>
#include <mdcmImage.h>
#include <mdcmImageReader.h>
#include <mdcmWriter.h>
#include <mdcmVR.h>
#include <mdcmVM.h>
#include <mdcmOrientation.h>
#include <mdcmCSAHeader.h>
#include <mdcmMediaStorage.h>
#include <mdcmUIDs.h>
#include <mdcmFileMetaInformation.h>
#include <mdcmWriter.h>
#include <mdcmScanner.h>
#include <mdcmSequenceOfFragments.h>
#include <mdcmDict.h>
#include <mdcmSystem.h>
#include <mdcmParseException.h>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QTextCodec>
#include <QByteArray>
#include <QMimeData>
#include <QUrl>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextStream>
#include <QTextCodec>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QMap>
#include <QDate>
#include <QDateTime>
#include <QMessageBox>
#include <QAction>
#include <QAbstractItemModel>
#include "codecutils.h"
#include "dicomutils.h"
#include "commonutils.h"
#include <exception>

template <typename T, long long TVR>
void get_bin_values(
	const mdcm::DataElement & v,
	std::vector<T> & result)
{
	if (v.IsEmpty() || v.IsUndefinedLength())
		return;
	const mdcm::ByteValue * bv = v.GetByteValue();
	if (!bv) return;
	if ((bv->GetLength() < sizeof(T)) ||
		((bv->GetLength() % sizeof(T)) != 0))
		return;
	mdcm::Element<TVR, mdcm::VM::VM1_n> e;
	e.SetFromDataElement(v);
	for (unsigned long x = 0; x < e.GetLength(); ++x)
	{
		result.push_back(static_cast<T>(e.GetValue(x)));
	}
}

static void get_series_files(
	const QString & f,
	const QString & uid,
	QStringList & result)
{
	QFileInfo fi(f);
	const QString p = fi.absolutePath();
	std::vector<std::string> files;
	{
		QDir dir(p);
		const QStringList l = dir.entryList(QDir::Files|QDir::Readable, QDir::Name);
		for (int x = 0; x < l.size(); ++x)
		{
			const QString tmp2 = dir.absolutePath() + QString("/") + l.at(x);
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
			files.push_back(std::string(QDir::toNativeSeparators(tmp2).toUtf8().constData()));
#else
			files.push_back(std::string(QDir::toNativeSeparators(tmp2).toLocal8Bit().constData()));
#endif
#else
			files.push_back(std::string(tmp2.toLocal8Bit().constData()));
#endif
		}
	}
	const mdcm::Global & g  = mdcm::GlobalInstance;
	const mdcm::Dicts  & dicts = g.GetDicts();
	const mdcm::Dict & dict = dicts.GetPublicDict();
	const mdcm::Tag t(0x0020,0x000e);
	mdcm::Scanner s;
	s.AddTag(t);
	s.Scan(files, dict);
	mdcm::Scanner::ValuesType v = s.GetValues();
	mdcm::Scanner::ValuesType::iterator it = v.begin();
	while (it != v.end())
	{
		const QString tmp0 = QString::fromLatin1((*it).c_str());
		if (tmp0.trimmed().remove(QChar('\0')) == uid.trimmed().remove(QChar('\0')))
		{
			std::vector<std::string> f__ = s.GetAllFilenamesFromTagToValue(t, (*it).c_str());
			for (unsigned int j = 0; j < f__.size(); ++j)
			{
				const QString tmp1 =
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
					QDir::toNativeSeparators(QString::fromUtf8(f__.at(j).c_str()));
#else
					QDir::toNativeSeparators(QString::fromLocal8Bit(f__.at(j).c_str()));
#endif
#else
					QString::fromLocal8Bit(f__.at(j).c_str());
#endif
				mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
				reader.SetFileName(QDir::toNativeSeparators(tmp1).toUtf8().constData());
#else
				reader.SetFileName(QDir::toNativeSeparators(tmp1).toLocal8Bit().constData());
#endif
#else
				reader.SetFileName(tmp1.toLocal8Bit().constData());
#endif
				if (reader.ReadUpToTag(t))
				{
					result.push_back(tmp1);
				}
			}
			break;
		}
		++it;
	}
}

const QString css1 =
	QString(
		"span.y4 { color:#050505; font-size: medium; font-weight: bold;}\n"
		"span.y5 { color:#0d0d76; font-size: medium; font-weight: bold;}\n"
		"span.y  { color:#0d0d76; font-size: large;  font-weight: bold; font-style: italic }");
const QString head = QString(
	"<html><head><link rel='stylesheet' type='text/css' href='format.css'></head><body>");
const QString foot = QString("</body></html>");

SQtree::SQtree(bool t) : skip_settings_pos(t)
{
	setupUi(this);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	textEdit->hide();
	horizontalSlider->hide();
#if 1
	options_toolButton->hide();
#endif
	copyAct      = new QAction(QString("Copy selected text"),   this);
	expandAct    = new QAction(QString("Expand child items"),   this);
	collapseAct  = new QAction(QString("Collapse child items"), this);
	treeWidget->addAction(copyAct);
	treeWidget->addAction(expandAct);
	treeWidget->addAction(collapseAct);
	treeWidget->setColumnWidth(0, 200);
	treeWidget->setColumnWidth(1, 300);
	treeWidget->setColumnWidth(2,  60);
	treeWidget->setRootIsDecorated(true);
	connect(copyAct,         SIGNAL(triggered()),      this,SLOT(copy_to_clipboard()));
	connect(collapseAct,     SIGNAL(triggered()),      this,SLOT(collapse_item()));
	connect(expandAct,       SIGNAL(triggered()),      this,SLOT(expand_item()));
	connect(pushButton,      SIGNAL(clicked()),        this,SLOT(open_file()));
	connect(scan_pushButton, SIGNAL(clicked()),        this,SLOT(open_file_and_series()));
	connect(horizontalSlider,SIGNAL(valueChanged(int)),this,SLOT(file_from_slider(int)));
}

SQtree::~SQtree()
{
}

void SQtree::process_element(
	const mdcm::DataSet & ds,
	const mdcm::DataElement & e,
	const mdcm::Dicts & d,
	QTreeWidgetItem * wi,
	const char * charset,
	const bool duplicated)
{
	const mdcm::Tag & tag = e.GetTag();
	QString tname("");
	bool invalid_vr          = false;
	bool unknown_vr          = false;
	bool private_tag         = false;
	bool private_creator_tag = false;
	bool illegal_tag         = false;
	bool skipped             = false;
	bool hdr                 = false;
	if (tag.GetGroup() == 0x0002) hdr = true;
	mdcm::VR vr = e.GetVR();
	if (vr == mdcm::VR::INVALID) invalid_vr = true;
	if (vr == mdcm::VR::UN)      unknown_vr = true;
	if (tag.IsIllegal())
	{
		illegal_tag = true;
		tname = QString("Illegal Tag");
	}
	else if (tag.IsPrivateCreator())
	{
		private_creator_tag = true;
		tname = QString("Private Creator");
		if (invalid_vr||unknown_vr)
		{
			const mdcm::DictEntry & entry =
				d.GetDictEntry(tag,(const char *)NULL);
			vr = entry.GetVR();
		}
	}
	else if (tag.IsPrivate())
	{
		private_tag = true;
		const mdcm::PrivateDict & pdict = d.GetPrivateDict();
		const mdcm::Tag private_creator_t = tag.GetPrivateCreator();
		if(ds.FindDataElement(private_creator_t))
		{
			const mdcm::DataElement & private_creator_e =
				ds.GetDataElement(private_creator_t);
			if (!private_creator_e.IsEmpty() &&
				!private_creator_e.IsUndefinedLength() &&
				private_creator_e.GetByteValue())
			{
				const QString private_creator
					= QString::fromLatin1(
						private_creator_e.GetByteValue()->GetPointer(),
						private_creator_e.GetByteValue()->GetLength());
				const mdcm::PrivateTag ptag(
					tag.GetGroup(),
					tag.GetElement(),
					private_creator.toLatin1().constData());
				const mdcm::DictEntry & pentry =
					pdict.GetDictEntry(ptag); 
				tname = QString(pentry.GetName()).trimmed();
				if (invalid_vr||unknown_vr) vr = pentry.GetVR();
			}
		}
	}
	else
	{
		const mdcm::DictEntry & entry =
			d.GetDictEntry(tag,(const char *)NULL);
		tname = QString(entry.GetName());
		if (invalid_vr||unknown_vr) vr = entry.GetVR();
	}
	const QBrush brush0(QColor::fromRgbF(0.0,0.0,0.5));
	const QBrush brush2(QColor::fromRgbF(0.3,0.0,0.3));
	const QBrush brush4(QColor::fromRgbF(0.5,0.5,0.5));
	const QBrush brush5(QColor::fromRgbF(0.4,0.4,0.7));
	const QBrush brush6(QColor::fromRgbF(0.5,0.0,0.0));
	const QBrush brush7(QColor::fromRgbF(0.5,0.5,0.5));
	QString length_s("");
	if (vr == mdcm::VR::SQ)
	{
		mdcm::SmartPointer<mdcm::SequenceOfItems> sqi =
			e.GetValueAsSQ();
		if (!(sqi && sqi->GetNumberOfItems()>0))
		{
			QStringList l2;
			l2 << QString(tag.PrintAsPipeSeparatedString().c_str())
				<< tname
				<< QString("SQ")
				<< QString("")
				<< QString("[0]");
			QTreeWidgetItem * ci = new QTreeWidgetItem(l2);
			ci->setForeground(4, brush2); // binary
			if (private_tag) ci->setForeground(1, brush0);
			if (invalid_vr)  ci->setBackground(2, brush5);
			if (unknown_vr)  ci->setBackground(2, brush4);
			if (illegal_tag) ci->setForeground(1, brush6);
			if (hdr)         ci->setBackground(0, brush7); // impos.
			if (hdr)         ci->setBackground(1, brush7); // impos.
			if (duplicated)  ci->setForeground(0, brush6);
			wi->addChild(ci);
			return;
		}
		//
		QString sq_length("");
		if (!sqi->IsUndefinedLength())
		{
			const qlonglong length =
				static_cast<qlonglong>(sqi->GetLength());
			QString tmp1;
			if (length > 1024*1024)
			{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				tmp1 = QString::asprintf("%.2f", length / (1024.0*1024.0));
#else
				tmp1.sprintf("%.2f", length / (1024.0*1024.0));
#endif
				sq_length = tmp1 + QString(" MB");
			}
			else if (length > 1024)
			{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				tmp1 = QString::asprintf("%.2f", length/1024.0);
#else
				tmp1.sprintf("%.2f", length/1024.0);
#endif
				sq_length = tmp1 + QString(" KB");
			}
			else
			{
				sq_length = QVariant(length).toString() +
					QString(" B");
			}
		}
		//
		QStringList l;
		l << QString(tag.PrintAsPipeSeparatedString().c_str())
			<< tname
			<< QString("SQ")
			<< sq_length
			<< QString(" [")+
				QVariant(static_cast<unsigned int>(
					sqi->GetNumberOfItems())).toString() +
					QString("]");
		QTreeWidgetItem * ci = new QTreeWidgetItem(l);
		ci->setForeground(4, brush2); // binary
		if (invalid_vr)  ci->setBackground(2, brush5);
		if (private_tag) ci->setForeground(1, brush0);
		if (unknown_vr)  ci->setBackground(2, brush4);
		if (illegal_tag) ci->setForeground(1, brush6);
		if (hdr)         ci->setBackground(0, brush7); // impos.
		if (hdr)         ci->setBackground(1, brush7); // impos.
		if (duplicated)  ci->setForeground(0, brush6);
		wi->addChild(ci);
		for (unsigned int i = 0; i < sqi->GetNumberOfItems(); ++i)
		{
			QStringList l1;
			l1 << QString("Item")
				<< QVariant((int)(i+1)).toString()
				<< QString("")
				<< QString("")
				<< QString("");
			const mdcm::Item    & item = sqi->GetItem(i+1);
			const mdcm::DataSet & nds  = item.GetNestedDataSet();
			QTreeWidgetItem * cin = new QTreeWidgetItem(l1);
			cin->setForeground(0, brush2);
			cin->setForeground(1, brush2);
			if (duplicated) cin->setForeground(0, brush6);
			ci->addChild(cin);
			mdcm::DataElement tmp_tag;
			size_t ce = 0;
			for (mdcm::DataSet::ConstIterator it = nds.Begin();
				it!=nds.End();
				++it)
			{
				bool duplicated_ = false;
				const mdcm::DataElement & elem = *it;
				if (ce > 0 && tmp_tag == elem.GetTag()) duplicated_ = true;
				process_element(nds, elem, d, cin, charset, duplicated_);
				tmp_tag = elem.GetTag();
				++ce;
			}
		}
	}
	else
	{
		QString str_("");
		QString entry_qs("");
		QString keyword_qs = tname;
		std::string entry_s = tag.PrintAsPipeSeparatedString();
		bool bin_label = false;
		if (!entry_s.empty()) entry_qs = QString(entry_s.c_str());
		if (e.IsEmpty())
		{
			QStringList l;
			l << entry_qs
				<< keyword_qs
				<< QString(mdcm::VR::GetVRString(vr))
				<< QString("")
				<< QString("empty");
			QTreeWidgetItem * ii = new QTreeWidgetItem(l);
			ii->setForeground(4, brush2); // binary
			if (invalid_vr)  ii->setBackground(2, brush5);
			if (unknown_vr)  ii->setBackground(2, brush4);
			if (hdr)         ii->setBackground(0, brush7);
			if (hdr)         ii->setBackground(1, brush7);
			if (private_tag) ii->setForeground(1, brush0);
			if (illegal_tag) ii->setForeground(1, brush6);
			if (duplicated)  ii->setForeground(0, brush6);
			wi->addChild(ii);
		}
		else if (e.IsUndefinedLength())
		{
			QStringList l;
			l << entry_qs
				<< keyword_qs
				<< QString(mdcm::VR::GetVRString(vr))
				<< QString("")
				<< QString("undefined length");
			QTreeWidgetItem * ii = new QTreeWidgetItem(l);
			ii->setForeground(4, brush2); // binary
			if (invalid_vr)  ii->setBackground(2, brush5);
			if (unknown_vr)  ii->setBackground(2, brush4);
			if (hdr)         ii->setBackground(0, brush7);
			if (hdr)         ii->setBackground(1, brush7);
			if (private_tag) ii->setForeground(1, brush0);
			if (illegal_tag) ii->setForeground(1, brush6);
			if (duplicated)  ii->setForeground(0, brush6);
			wi->addChild(ii);
		}
		else
		{
			const mdcm::ByteValue * bv = e.GetByteValue();
			if (bv)
			{
				const qlonglong length =
					static_cast<qlonglong>(bv->GetLength());
				QString tmp1;
				if (length > 1024*1024)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp1 = QString::asprintf("%.2f", length / (1024.0*1024.0));
#else
					tmp1.sprintf("%.2f", length / (1024.0*1024.0));
#endif	
					length_s = tmp1 + QString(" MB");
				}
				else if (length > 1024)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp1 = QString::asprintf("%.2f", length / 1024.0);
#else
					tmp1.sprintf("%.2f", length / 1024.0);
#endif
					length_s = tmp1 + QString(" KB");
				}
				else
				{
					length_s = QVariant(length).toString() +
						QString(" B");
				}
				if (mdcm::VR::IsBinary(vr) || mdcm::VR::IsBinary2(vr))
				{
					bin_label = true;
					if (vr == mdcm::VR::US)
					{
						std::vector<unsigned short> values;
						get_bin_values<unsigned short, mdcm::VR::US>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(
									(int)values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::SS)
					{
						std::vector<signed short> values;
						get_bin_values<signed short, mdcm::VR::SS>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(
									(int)values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::FL)
					{
						std::vector<float> values;
						get_bin_values<float, mdcm::VR::FL>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::FD)
					{
						std::vector<double> values;
						get_bin_values<double, mdcm::VR::FD>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::UL)
					{
						std::vector<unsigned int> values;
						get_bin_values<unsigned int, mdcm::VR::UL>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::SL)
					{
						std::vector<signed int> values;
						get_bin_values<signed int, mdcm::VR::SL>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::SV)
					{
						std::vector<signed long long> values;
						get_bin_values<signed long long, mdcm::VR::SV>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::UV)
					{
						std::vector<unsigned long long> values;
						get_bin_values<unsigned long long, mdcm::VR::UV>(
							e, values);
						for (unsigned long j = 0; j < values.size(); ++j)
						{
							str_.append(
								QVariant(values[j]).toString() +
								QString(" "));
						}
					}
					else if (vr == mdcm::VR::AT)
					{
						unsigned short group, element;
						char group_[2]; char element_[2];
						char * buffer = new char[4];
						if (length==4)
						{
							const bool ok0 =
								bv->GetBuffer(buffer,4);
							if (ok0)
							{
								group_[0] = buffer[0];
								group_[1] = buffer[1];
								memcpy(&group,group_,2);
								element_[0] = buffer[2];
								element_[1] = buffer[3];
								memcpy(&element,element_,2);
								QString tmp3;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
								tmp3 = QString::asprintf("%04x",group);
#else
								tmp3.sprintf("%04x",group);
#endif
								QString tmp4;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
								tmp4 = QString::asprintf("%04x",element);
#else
								tmp4.sprintf("%04x",element);
#endif
								str_ = tmp3 + QString("|") + tmp4;
							}
						}
						delete [] buffer;
					}
					else if (vr == mdcm::VR::OB)
					{
						str_ = QString("binary (OB)");
						skipped = true;
					}
					else if (vr == mdcm::VR::OW)
					{
						str_ = QString("binary (OW)");
						skipped = true;
					}
					else if (vr == mdcm::VR::OL)
					{
						str_ = QString("binary (OL)");
						skipped = true;
					}
					else if (vr == mdcm::VR::OD)
					{
						str_ = QString("binary (OD)");
						skipped = true;
					}
					else if (vr == mdcm::VR::OF)
					{
						str_ = QString("binary (OF)");
						skipped = true;
					}
					else if (vr == mdcm::VR::OV)
					{
						str_ = QString("binary (OV)");
						skipped = true;
					}
					else if (vr == mdcm::VR::UN)
					{
						str_ = QString("binary (UN)");
						skipped = true;
					}
					else if (vr == mdcm::VR::OB_OW)
					{
						str_ = QString("binary (OB or OW)");
						skipped = true;
					}
					else if (vr == mdcm::VR::US_SS)
					{
						str_ = QString("binary (US or SS)");
						skipped = true;
					}
					else if (vr == mdcm::VR::US_SS_OW)
					{
						str_ = QString("binary (US or SS or OW)");
						skipped = true;
					}
					else
					{
						str_ = QString("binary (unknown)");
						skipped = true;
					}
					QStringList l;
					l << entry_qs
						<< keyword_qs
						<< QString(mdcm::VR::GetVRString(vr))
						<< length_s
						<< str_;
					QTreeWidgetItem * ii =  new QTreeWidgetItem(l);
					if (invalid_vr)  ii->setBackground(2, brush5);
					if (unknown_vr)  ii->setBackground(2, brush4);
					if (bin_label)   ii->setForeground(4, brush2);
					if (skipped)     ii->setForeground(4, brush2);
					if (hdr)         ii->setBackground(0, brush7);
					if (hdr)         ii->setBackground(1, brush7);
					if (private_tag) ii->setForeground(1, brush0);
					if (private_creator_tag) ii->setForeground(4, brush0);
					if (illegal_tag) ii->setForeground(1, brush6);
					if (duplicated)  ii->setForeground(0, brush6);
					wi->addChild(ii);
				}
				else if (vr == mdcm::VR::INVALID || vr >= mdcm::VR::VR_END)
				{
					QStringList l;
					l << entry_qs
						<< keyword_qs
						<< QString(mdcm::VR::GetVRString(vr)).
							remove(QChar('\0'))
						<< length_s
						<< QString("unknown");
					QTreeWidgetItem * ii =  new QTreeWidgetItem(l);
					if (invalid_vr)  ii->setBackground(2, brush5);
					if (unknown_vr)  ii->setBackground(2, brush4);
					if (hdr)         ii->setBackground(0, brush7);
					if (hdr)         ii->setBackground(1, brush7);
					if (private_tag) ii->setForeground(1, brush0);
					ii->setForeground(4, brush2);
					if (private_creator_tag) ii->setForeground(4, brush0);
					if (illegal_tag) ii->setForeground(1, brush6);
					if (duplicated)  ii->setForeground(0, brush6);
					wi->addChild(ii);
				}
				else if (vr == mdcm::VR::UI)
				{
					QString tmp0 = QString::fromLatin1(
						bv->GetPointer(),
						bv->GetLength());
					mdcm::UIDs uid;
					uid.SetFromUID(tmp0.toLatin1().constData());
					const QString tmp1 =
						QString::fromLatin1(uid.GetName());
					QStringList l;
					l << entry_qs
						<< keyword_qs
						<< QString(mdcm::VR::GetVRString(vr)).
							remove(QChar('\0'))
						<< length_s
						<< ((tmp1.isEmpty())
							? tmp0.remove(QChar('\0')) : tmp1);
					QTreeWidgetItem * ii =  new QTreeWidgetItem(l);
					if (!tmp1.isEmpty()) ii->setForeground(4, brush2);
					if (invalid_vr)      ii->setBackground(2, brush5);
					if (unknown_vr)      ii->setBackground(2, brush4);
					if (hdr)             ii->setBackground(0, brush7);
					if (hdr)             ii->setBackground(1, brush7);
					if (private_tag)     ii->setForeground(1, brush0);
					if (illegal_tag)     ii->setForeground(1, brush6);
					if (duplicated)      ii->setForeground(0, brush6);
					wi->addChild(ii);
				}
				else // ASCII
				{
					bool date_time = false;
					QStringList l;
					QString tmp0;
					if (vr == mdcm::VR::DA)
					{
						const QString date_s = QString::fromLatin1(
							bv->GetPointer(),
							bv->GetLength()).trimmed().remove(QChar('\0'));
						if (date_s.length() == 8)
						{
							const QDate date_ =
								QDate::fromString(
									date_s, QString("yyyyMMdd"));
							tmp0 = date_.toString(
								QString("d MMM yyyy"));
							date_time = true;
						}
						else
						{
							tmp0 = date_s;
						}
					}
					else if (vr == mdcm::VR::TM)
					{
						const QString time0_s = QString::fromLatin1(
							bv->GetPointer(),
							bv->GetLength()).
								trimmed().remove(QChar('\0'));
						if (!time0_s.isEmpty())
						{
							const int range_idx =
								time0_s.indexOf(QString("-"));
							const int multi_idx =
								time0_s.indexOf(QString("\\"));
							if (range_idx == -1 && multi_idx == -1)
							{
								const int point_idx =
									time0_s.indexOf(QString("."));
								if (point_idx == 6||point_idx == -1)
								{
									const QString time1_s =
										time0_s.left(6);
									const QDateTime time_ =
										QDateTime::fromString(
											time1_s,
											QString("HHmmss"));
									tmp0 = time_.toString(
										QString("HH:mm:ss"));
									if (point_idx == 6)
									{
										tmp0.append(QString(".") +
											time0_s.right(
												time0_s.length()-7));
									}
									date_time = true;
								}
								else
								{
									tmp0 = time0_s;
								}
							}
							else
							{
								tmp0 = time0_s;
							}
						}
					}
					else if (vr == mdcm::VR::DT)
					{
						const QString time0_s = QString::fromLatin1(
							bv->GetPointer(),
							bv->GetLength()).
								trimmed().remove(QChar('\0'));
						if (!time0_s.isEmpty())
						{
							if (time0_s.length() >= 14 &&
									time0_s.length() <= 26)
							{
								const int point_idx =
									time0_s.indexOf(QString("."));
								if (point_idx == 14 || point_idx == -1)
								{
									const QString time1_s =
										time0_s.left(14);
									const QDateTime time_ =
										QDateTime::fromString(
											time1_s,
											QString("yyyyMMddHHmmss"));
									tmp0 = time_.toString(
										QString("d MMM yyyy HH:mm:ss"));
									if (point_idx == 14)
									{
										tmp0.append(QString(".") +
											time0_s.right(
												time0_s.length()-15));
									}
									date_time = true;
								}
								else
								{
									tmp0 = time0_s;
								}
							}
							else
							{
								tmp0 = time0_s;
							}
						}
					}
					else
					{
						if (
							vr==mdcm::VR::LO ||
							vr==mdcm::VR::LT ||
							vr==mdcm::VR::PN ||
							vr==mdcm::VR::SH ||
							vr==mdcm::VR::ST ||
							vr==mdcm::VR::UT)
						{
							QByteArray ba(
								bv->GetPointer(),
								bv->GetLength());
							tmp0 = CodecUtils::toUTF8(&ba, charset);
							ba.clear();
						}
						else
						{
							if (tag == mdcm::Tag(0x3006,0x0050))
							{
								tmp0 = QString(
									"<skipped, save to file>");
								skipped = true;
							}
							else
							{
								tmp0 = QString::fromLatin1(
									bv->GetPointer(),
									bv->GetLength());
							}
						}
						tmp0 = tmp0.remove(QChar('\0'));
					}
					if (tmp0.length() > 1024)
					{
						tmp0.truncate(16);
						tmp0.append(QString("<skipped, save to file>"));
						skipped = true;
					}
					l << entry_qs
						<< keyword_qs
						<< QString(mdcm::VR::GetVRString(vr)).
								remove(QChar('\0'))
						<< length_s
						<< tmp0;
					QTreeWidgetItem * ii =  new QTreeWidgetItem(l);
					if (invalid_vr)  ii->setBackground(2, brush5);
					if (unknown_vr)  ii->setBackground(2, brush4);
					if (hdr)         ii->setBackground(0, brush7);
					if (hdr)         ii->setBackground(1, brush7);
					if (private_tag) ii->setForeground(1, brush0);
					if (date_time)   ii->setForeground(4, brush2);
					if (skipped)     ii->setForeground(4, brush2);
					if (private_creator_tag) ii->setForeground(4, brush0);
					if (illegal_tag) ii->setForeground(1, brush6);
					if (duplicated)  ii->setForeground(0, brush6);
					wi->addChild(ii);
				}
			}
			else
			{
				QStringList l;
				l << entry_qs
					<< keyword_qs
					<< QString(mdcm::VR::GetVRString(vr))
					<< QString("")
					<< QString("NULL");
				QTreeWidgetItem * ii = new QTreeWidgetItem(l);
				ii->setForeground(4, brush2);
				if (invalid_vr)  ii->setBackground(2, brush5);
				if (unknown_vr)  ii->setBackground(2, brush4);
				if (hdr)         ii->setBackground(0, brush7);
				if (hdr)         ii->setBackground(1, brush7);
				if (private_tag) ii->setForeground(1, brush0);
				if (illegal_tag) ii->setForeground(1, brush6);
				if (duplicated)  ii->setForeground(0, brush6);
				wi->addChild(ii);
			}
		}
	}
}

void SQtree::closeEvent(QCloseEvent * e)
{
	e->accept();
}

void SQtree::read_file(const QString & f, const bool use_lock)
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = (use_lock) ? mutex.tryLock() : false;
	if (use_lock && !lock) return;
#endif
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	clear_tree();
	QFileInfo fi(f);
	saved_dir = fi.absoluteDir().absolutePath();
	lineEdit->setText(QDir::toNativeSeparators(f));
	QStringList l;
	l << QString("") << QString("") <<
		QString("") << QString("") << QString("");
	QTreeWidgetItem * i = new QTreeWidgetItem(l);
	treeWidget->addTopLevelItem(i);
	try
	{
		mdcm::Reader reader;
		const mdcm::File & file = reader.GetFile();
		bool ok = false;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		reader.SetFileName(QDir::toNativeSeparators(f).toUtf8().constData());
#else
		reader.SetFileName(QDir::toNativeSeparators(f).toLocal8Bit().constData());
#endif
#else
		reader.SetFileName(f.toLocal8Bit().constData());
#endif
		ok = reader.Read();
		if (!ok)
		{
			ms_lineEdit->setText(QString("Error: file is not DICOM or broken."));
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
			if (lock) mutex.unlock();
#endif
			QApplication::restoreOverrideCursor();
			return;
		}
		mdcm::Global & g = mdcm::Global::GetInstance();
		const mdcm::Dicts & dicts = g.GetDicts();
		const mdcm::DataSet & ds = file.GetDataSet();
		const mdcm::FileMetaInformation & header = file.GetHeader();
		//
		QString tmp1("");
		QString ms0_ = QString::fromStdString(
			header.GetMediaStorageAsString());
		if (!ms0_.isEmpty())
		{
			mdcm::UIDs uid;
			uid.SetFromUID(ms0_.toLatin1().constData());
			const QString ms1_ = QString::fromLatin1(uid.GetName());
			if (!ms1_.isEmpty())
			{
				tmp1 = ms1_;
			}
			else
			{
				const QString ms2_ = QString::fromLatin1(uid.GetString());
				tmp1 = ms2_;
			}
		}
		else
		{
			QString tmp0;
			if (DicomUtils::get_string_value(
					ds, mdcm::Tag(0x0008, 0x0016), tmp0))
			{
				mdcm::UIDs uid;
				uid.SetFromUID(tmp0.toLatin1().constData());
				const QString ms1_ = QString::fromLatin1(uid.GetName());
				if (!ms1_.isEmpty())
				{
					tmp1 = ms1_;
				}
				else
				{
					const QString ms2_ = QString::fromLatin1(uid.GetString());
					tmp1 = ms2_;
				}
			}
		}
		ms_lineEdit->setText(tmp1);
		//
		const mdcm::TransferSyntax & ts =
			header.GetDataSetTransferSyntax();
		mdcm::UIDs uid;
		uid.SetFromUID(ts.GetString());
		const QString ts_string = QString::fromLatin1(uid.GetName());
		ts_lineEdit->setText(ts_string);
		//
		for (
			mdcm::FileMetaInformation::ConstIterator it = header.Begin();
			it!=header.End();
			++it)
		{
			const mdcm::DataElement & elem = *it;
			process_element(ds /* unused */, elem, dicts, i, "");
		}
		QString charset("");
		if (ds.FindDataElement(mdcm::Tag(0x0008,0x0005)))
		{
			const mdcm::DataElement & ce_ =
				ds.GetDataElement(mdcm::Tag(0x0008,0x0005));
			if (!ce_.IsEmpty() &&
				!ce_.IsUndefinedLength() &&
				ce_.GetByteValue())
			{
				charset = QString::fromLatin1(
					ce_.GetByteValue()->GetPointer(),
					ce_.GetByteValue()->GetLength());
			}
		}
		//
		{
			mdcm::Tag tmp_tag;
			size_t ce = 0;
			for (mdcm::DataSet::ConstIterator it = ds.Begin();
				it!=ds.End();
				++it)
			{
				bool duplicated = false;
				const mdcm::DataElement & elem = *it;
				if (ce > 0 && tmp_tag == elem.GetTag()) duplicated = true;
				process_element(
					ds, elem, dicts, i, charset.toLatin1().constData(), duplicated);
				tmp_tag = elem.GetTag();
				++ce;
			}
		}
		//
		//
		//
		dump_csa(ds);
		dump_gems(ds);
	}
	catch(mdcm::ParseException & pe)
	{
		std::cout << "mdcm::ParseException in SQtree::read_file("
			<< f.toStdString() << ", " << use_lock << "):\n"
			<< pe.GetLastElement().GetTag() << std::endl;
	}
	catch(std::exception & ex)
	{
		std::cout << "Exception in SQtree::read_file("
			<< f.toStdString() << ", " << use_lock << "):\n"
			<< ex.what() << std::endl;
	}
	treeWidget->expandToDepth(0);
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	if (lock) mutex.unlock();
#endif
	QApplication::restoreOverrideCursor();
}

void SQtree::read_file_and_series(const QString & ff, const bool use_lock)
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = (use_lock) ? mutex.tryLock() : false;
	if (use_lock && !lock) return;
#endif
	QString f(ff);
	QFileInfo fi(f);
	if (!fi.isFile())
	{
		clear_tree();
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
		if (lock) mutex.unlock();
#endif
		return;
	}
	f = fi.absoluteFilePath();
	QString series_uid;
	QStringList files;
	bool series_uid_ok = false;
	try
	{
		{
			std::set<mdcm::Tag> tags;
			tags.insert(mdcm::Tag(0x0020,0x000e));
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
			if (reader.ReadSelectedTags(tags))
			{
				const mdcm::File & file = reader.GetFile();
				const mdcm::DataSet & ds = file.GetDataSet();
				series_uid_ok =
					DicomUtils::get_string_value(
						ds,
						mdcm::Tag(0x0020,0x000e),
						series_uid);
			}
		}
		if (series_uid_ok)
		{
			get_series_files(f, series_uid, files);
		}
	}
	catch(mdcm::ParseException & pe)
	{
		std::cout
			<< "mdcm::ParseException in SQtree::open_file_and_series:\n"
			<< pe.GetLastElement().GetTag() << std::endl;
	}
	catch(std::exception & ex)
	{
		std::cout << "Exception in SQtree::open_file_and_series:\n"
			<< ex.what() << std::endl;
	}
	int idx = -1;
	const int files_size = files.size();
	if (files_size > 1)
	{
		for (int x = 0; x < files_size; ++x)
		{
			QFileInfo fi0(files.at(x));
			if (fi0.absoluteFilePath() == f)
			{
				idx = x;
				break;
			}
		}
	}
	//
	horizontalSlider->blockSignals(true);
	horizontalSlider->setMinimum(0);
	if (idx >= 0)
	{
		list_of_files = QStringList(files);
		horizontalSlider->setMaximum(files_size-1);
		horizontalSlider->setValue(idx);
		horizontalSlider->show();
	}
	else
	{
		list_of_files = QStringList(f);
		horizontalSlider->setMaximum(0);
		horizontalSlider->setValue(0);
		horizontalSlider->hide();
	}
	horizontalSlider->blockSignals(false);
	read_file(f, false);
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	if (lock) mutex.unlock();
#endif
}

void SQtree::dump_csa(const mdcm::DataSet & ds)
{
	mdcm::CSAHeader csa1, csa2;
	QString csa_text("");
	const mdcm::PrivateTag t1 = csa1.GetCSAImageHeaderInfoTag();
	const mdcm::PrivateTag t2 = csa2.GetCSASeriesHeaderInfoTag();
	const bool f_t1 = ds.FindDataElement(t1);
	const bool f_t2 = ds.FindDataElement(t2);
	if (f_t1||f_t2)
	{
		if (f_t1)
		{
			csa1.LoadFromDataElement(ds.GetDataElement(t1));
			if (csa1.GetFormat() == mdcm::CSAHeader::SV10 ||
				csa1.GetFormat() == mdcm::CSAHeader::NOMAGIC)
			{
				std::ostringstream os1;
				csa1.Print(os1);
				csa_text.append(QString::fromStdString(os1.str()));
			}
		}
		if (f_t2)
		{
			csa2.LoadFromDataElement(ds.GetDataElement(t2));
			if (csa2.GetFormat() == mdcm::CSAHeader::SV10 ||
				csa2.GetFormat() == mdcm::CSAHeader::NOMAGIC)
			{
				std::ostringstream os2;
				csa2.Print(os2);
				csa_text.append(QString("\n\n") +
					QString::fromStdString(os2.str()));
			}
		}
		if (!csa_text.isEmpty() && textEdit->document())
		{
			csa_text.prepend(QString("Siemens CSA\n\n"));
			textEdit->document()->setPlainText(csa_text);
			QTextCursor cursor = textEdit->textCursor();
			cursor.setPosition(0);
			textEdit->setTextCursor(cursor);
			textEdit->show();
		}
	}
}

void SQtree::dump_gems(const mdcm::DataSet & ds)
{
	const mdcm::PrivateTag tgems_us_movie(
		0x7fe1,0x1,"GEMS_Ultrasound_MovieGroup_001");
	if (!ds.FindDataElement(tgems_us_movie)) return;
	const mdcm::DataElement & e1 = ds.GetDataElement(tgems_us_movie);
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 = e1.GetValueAsSQ();
	if (!(sq1 && sq1->GetNumberOfItems()>0)) return;
	textEdit->document()->addResource(
		QTextDocument::StyleSheetResource, QUrl("format.css"), css1);
	QString gems_us_text = QString(head);
	const size_t n1 = sq1->GetNumberOfItems();
	for (size_t x = 0; x < n1; ++x)
	{
		gems_us_text.append(
			"<span class = 'y'>GEMS Ultrasound MovieGroup</span><br/>");
		QMap<QString,int> gdict;
		if (!DicomUtils::build_gems_dictionary(gdict,ds)) continue;
		const mdcm::Item & item1 = sq1->GetItem(x+1);
		const mdcm::DataSet & subds1 = item1.GetNestedDataSet();
		const mdcm::PrivateTag tname2(
			0x7fe1,0x2,"GEMS_Ultrasound_MovieGroup_001");
		QString qs2("");
		if (subds1.FindDataElement(tname2))
		{
			const mdcm::DataElement & e2 =
				subds1.GetDataElement(tname2);
			if (!e2.IsEmpty() && !e2.IsUndefinedLength())
			{
				const mdcm::ByteValue * v2 = e2.GetByteValue();
				if (v2)
				{
					const char * b2 = v2->GetPointer();
					const unsigned int s2   = v2->GetLength();
					qs2 =
						QString::fromLatin1(b2, s2).
							trimmed().remove(QChar('\0')) +
						QString("&#160;");
				}
			}
		}
		gems_us_text.append(
			QString("<span class = 'y5'>")+qs2+
			QString("</span><span class = 'y4'>(SQ 0x1)")+
			QString("</span><br/>"));
		const mdcm::PrivateTag t8(
			0x7fe1,0x8,"GEMS_Ultrasound_MovieGroup_001");
		if (subds1.FindDataElement(t8))
		{
			const mdcm::DataElement & e8 = subds1.GetDataElement(t8);
			QMap<int,GEMSParam> m8;
			DicomUtils::read_gems_params(m8, e8, gdict);
			QMapIterator<QString, int> itmd(gdict);
			gems_us_text.append(
				QString("<span class = 'y4'>")+
				QString("SQ 0x8:")+
				QString("</span><br/>"));
			while (itmd.hasNext())
			{
				itmd.next();
				if (m8.contains(itmd.value()))
				{
					const GEMSParam & gp = m8.value(itmd.value());
					const QList<QVariant> & l8 = gp.values;
					QString tmp8("");
					for (int x8 = 0; x8 < l8.size(); ++x8)
						tmp8.append(l8.at(x8).toString()+QString(" "));
					gems_us_text.append(itmd.key()+QString(": ")+tmp8+QString("<br/>")); 
				}
			}
		}
		const mdcm::PrivateTag t10(
			0x7fe1,0x10,"GEMS_Ultrasound_MovieGroup_001");
		if (subds1.FindDataElement(t10))
		{
			const mdcm::DataElement & e10 = subds1.GetDataElement(t10);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq10 =
				e10.GetValueAsSQ();
			if (sq10 && sq10->GetNumberOfItems()>0)
			{
				const size_t n10 = sq10->GetNumberOfItems();
				for (size_t x10 = 0; x10 < n10; ++x10)
				{
					mdcm::Item & item10 = sq10->GetItem(x10+1);
					mdcm::DataSet & subds10 =
						item10.GetNestedDataSet();
					const mdcm::PrivateTag tname12(
						0x7fe1,0x12,"GEMS_Ultrasound_MovieGroup_001");
					QString qs12("");
					if (subds10.FindDataElement(tname12))
					{
						const mdcm::DataElement & e12 =
							subds10.GetDataElement(tname12);
						if (!e12.IsEmpty() &&
							!e12.IsUndefinedLength())
						{
							const mdcm::ByteValue * v12 =
								e12.GetByteValue();
							if (v12)
							{
								const char * b12 = v12->GetPointer();
								const unsigned int s12   = v12->GetLength();
								qs12 = QString::fromLatin1(b12, s12).
									trimmed().remove(QChar('\0'))+
									QString("&#160;");
							}
						}
					}
					gems_us_text.append(
						QString("<span class = 'y5'>&#160;")+qs12+
						QString("</span><span class = 'y4'>(SQ 0x10)")+
						QString("</span><br/>"));
					const mdcm::PrivateTag t18(
						0x7fe1,0x18,"GEMS_Ultrasound_MovieGroup_001");
					if (subds10.FindDataElement(t18))
					{
						const mdcm::DataElement & e18 =
							subds10.GetDataElement(t18);
						QMap<int,GEMSParam> m18;
						DicomUtils::read_gems_params(m18, e18, gdict);
						QMapIterator<QString, int> itmd(gdict);
						gems_us_text.append(
							QString("<span class = 'y4'>&#160;")+
							QString("SQ 0x18:")+
							QString("</span><br/>"));
						while (itmd.hasNext())
						{
							itmd.next();
							if (m18.contains(itmd.value()))
							{
								const GEMSParam & gp = m18.value(itmd.value());
								const QList<QVariant> & l18 = gp.values;
								QString tmp18("");
								for (int x18 = 0;
									x18 < l18.size();
									++x18)
									tmp18.append(
										l18.at(x18).toString() +
										QString(" "));
								gems_us_text.append(
									QString("&#160;") +
									itmd.key() +
									QString(": ")+
									tmp18+QString("<br/>")); 
							}
						}
					}
					const mdcm::PrivateTag t20(
						0x7fe1,0x20,"GEMS_Ultrasound_MovieGroup_001");
					if (subds10.FindDataElement(t20))
					{
						const mdcm::DataElement & e20 =
							subds10.GetDataElement(t20);
						mdcm::SmartPointer<mdcm::SequenceOfItems>
							sq20 = e20.GetValueAsSQ();
						if (sq20 && sq20->GetNumberOfItems()>0)
						{
							const size_t n20 =
								sq20->GetNumberOfItems();
							for (size_t x20 = 0; x20 < n20; ++x20)
							{
								mdcm::Item & item20 =
									sq20->GetItem(x20+1);
								mdcm::DataSet & subds20 =
									item20.GetNestedDataSet();
								const mdcm::PrivateTag tname24(
									0x7fe1,
									0x24,
									"GEMS_Ultrasound_MovieGroup_001");
								QString qs24("");
								if (subds20.FindDataElement(tname24))
								{
									const mdcm::DataElement & e24 =
										subds20.GetDataElement(
											tname24);
									if (!e24.IsEmpty() &&
										!e24.IsUndefinedLength())
									{
										const mdcm::ByteValue * v24 =
											e24.GetByteValue();
										if (v24)
										{
											const char * b24 =
												v24->GetPointer();
											const unsigned int s24 =
												v24->GetLength();
											qs24 =
												QString::fromLatin1(
													b24, s24).
														trimmed().
														remove(
															QChar('\0'))+
												QString("&#160;");
										}
									}
								}
								gems_us_text.append(
									QString("<span class = 'y5'>&#160;&#160;&#160;&#160;")+
									qs24+QString("</span><span class = 'y4'>(SQ 0x20)")+
									QString("</span><br/>"));
								const mdcm::PrivateTag t26(0x7fe1,0x26,"GEMS_Ultrasound_MovieGroup_001");
								if (subds20.FindDataElement(t26))
								{
									const mdcm::DataElement & e26 = subds20.GetDataElement(t26);
									QMap<int,GEMSParam> m26;
									DicomUtils::read_gems_params(m26, e26, gdict);
									QMapIterator<QString, int> itmd(gdict);
									gems_us_text.append(
										QString("<span class = 'y4'>&#160;&#160;&#160;&#160;")+
										QString("SQ 0x26:")+QString("</span><br/>"));
									while (itmd.hasNext())
									{
										itmd.next();
										if (m26.contains(itmd.value()))
										{
											const GEMSParam & gp = m26.value(itmd.value());
											const QList<QVariant> & l26 = gp.values;
											QString tmp26("");
											for (int x26 = 0; x26 < l26.size(); ++x26)
												tmp26.append(l26.at(x26).toString()+QString(" "));
											gems_us_text.append(
												QString("&#160;&#160;&#160;&#160;")+itmd.key()+QString(": ")+
												tmp26+QString("<br/>")); 
										}
									}
								}
								/*
								const mdcm::PrivateTag t36(0x7fe1,0x36,"GEMS_Ultrasound_MovieGroup_001");
								if (subds20.FindDataElement(t36))
								{
									const mdcm::DataElement & e36 = subds20.GetDataElement(t36);
									mdcm::SmartPointer<mdcm::SequenceOfItems> sq36 = e36.GetValueAsSQ();
									const size_t n36 = sq36->GetNumberOfItems();
									for (size_t x36 = 0; x36 < n36; ++x36)
									{
										mdcm::Item & item36 = sq36->GetItem(x36+1);
										mdcm::DataSet & subds36 = item36.GetNestedDataSet();
									}
								}
								const mdcm::PrivateTag t3a(0x7fe1,0x3a,"GEMS_Ultrasound_MovieGroup_001");
								if (subds20.FindDataElement(t3a))
								{
									const mdcm::DataElement & e3a = subds20.GetDataElement(t3a);
									mdcm::SmartPointer<mdcm::SequenceOfItems> sq3a = e3a.GetValueAsSQ();
									const size_t n3a = sq3a->GetNumberOfItems();
									for (size_t x3a = 0; x3a < n3a; ++x3a)
									{
										mdcm::Item & item3a = sq3a->GetItem(x3a+1);
										mdcm::DataSet & subds3a = item3a.GetNestedDataSet();
										const mdcm::PrivateTag tname3a(0x7fe1,0x30,"GEMS_Ultrasound_MovieGroup_001");
										if (subds3a.FindDataElement(tname3a))
										{
											const mdcm::DataElement& e30 = subds3a.GetDataElement(tname3a);
											if (!e30.IsEmpty() && !e30.IsUndefinedLength())
											{
												const mdcm::ByteValue * v30 = e30.GetByteValue();
												if (v30)
												{
													const char * b30 = v30->GetPointer();
													const unsigned int s30   = v30->GetLength();
												}
											}
										}
									}
								}
								*/
								const mdcm::PrivateTag t83(0x7fe1,0x83,"GEMS_Ultrasound_MovieGroup_001");
								if (subds20.FindDataElement(t83))
								{
									const mdcm::DataElement & e83 = subds20.GetDataElement(t83);
									mdcm::SmartPointer<mdcm::SequenceOfItems> sq83 = e83.GetValueAsSQ();
									if (sq83 && sq83->GetNumberOfItems()>0)
									{
										const size_t n83 = sq83->GetNumberOfItems();
										for (size_t x83 = 0; x83 < n83; ++x83)
										{
											mdcm::Item & item83 = sq83->GetItem(x83+1);
											mdcm::DataSet & subds83 = item83.GetNestedDataSet();
											const mdcm::PrivateTag tname84(0x7fe1,0x84,"GEMS_Ultrasound_MovieGroup_001");
											QString qs84("");
											if (subds83.FindDataElement(tname84))
											{
												const mdcm::DataElement & e84 = subds83.GetDataElement(tname84);
												if (!e84.IsEmpty() && !e84.IsUndefinedLength())
												{
													const mdcm::ByteValue * v84 = e84.GetByteValue();
													if (v84)
													{
														const char * b84 = v84->GetPointer();
														const unsigned int s84   = v84->GetLength();
														qs84 = QString::fromLatin1(b84, s84)
															.trimmed().remove(QChar('\0'))+QString("&#160;");
													}
												}
											}
											gems_us_text.append(
												QString("<span class = 'y5'>&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;")+
												qs84+QString("</span><span class = 'y4'>(SQ 0x83)")+
												QString("</span><br/>"));
											const mdcm::PrivateTag t85(0x7fe1,0x85,"GEMS_Ultrasound_MovieGroup_001");
											if (subds83.FindDataElement(t85))
											{
												const mdcm::DataElement & e85 = subds83.GetDataElement(t85);
												QMap<int,GEMSParam> m85;
												DicomUtils::read_gems_params(m85, e85, gdict);
												QMapIterator<QString, int> itmd(gdict);
												gems_us_text.append(
													QString("<span class = 'y4'>&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;")+
													QString("SQ 0x85:")+QString("</span><br/>"));
												while (itmd.hasNext())
												{
													itmd.next();
													if (m85.contains(itmd.value()))
													{
														const GEMSParam & gp = m85.value(itmd.value());
														const QList<QVariant> & l85 = gp.values;
														QString tmp85("");
														for (int x85 = 0; x85 < l85.size(); ++x85)
															tmp85.append(l85.at(x85).toString()+QString(" "));
														gems_us_text.append(
															QString("&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;")+
															itmd.key()+QString(": ")+tmp85+QString("<br/>")); 
													}
												}
											}
										}
									}
								}
							}
						}
					}
					const mdcm::PrivateTag t83(
						0x7fe1,
						0x83,
						"GEMS_Ultrasound_MovieGroup_001");
					if (subds10.FindDataElement(t83))
					{
						const mdcm::DataElement & e83 =
							subds10.GetDataElement(t83);
						mdcm::SmartPointer<mdcm::SequenceOfItems>
							sq83 = e83.GetValueAsSQ();
						if (sq83 && sq83->GetNumberOfItems()>0)
						{
							const size_t n83 =
								sq83->GetNumberOfItems();
							for (size_t x83 = 0; x83 < n83; ++x83)
							{
								mdcm::Item & item83 =
									sq83->GetItem(x83+1);
								mdcm::DataSet & subds83 =
									item83.GetNestedDataSet();
								const mdcm::PrivateTag tname84(
									0x7fe1,
									0x84,
									"GEMS_Ultrasound_MovieGroup_001");
								QString qs84("");
								if (subds83.FindDataElement(tname84))
								{
									const mdcm::DataElement& e84 =
										subds83.GetDataElement(
											tname84);
									if (!e84.IsEmpty() &&
										!e84.IsUndefinedLength())
									{
										const mdcm::ByteValue * v84 =
											e84.GetByteValue();
										if (v84)
										{
											const char * b84 =
												v84->GetPointer();
											const unsigned int s84 =
												v84->GetLength();
											qs84 =
												QString::fromLatin1(
													b84,
													s84).
													trimmed().
													remove(QChar('\0')) +
												QString("&#160;");
										}
									}
								}
								gems_us_text.append(QString(
									"<span class = 'y5'>&#160;")+
									qs84+
									QString(
										"</span><span class = 'y4'>"
										"(SQ 0x83)</span><br/>"));
								const mdcm::PrivateTag t85(
									0x7fe1,
									0x85,
									"GEMS_Ultrasound_MovieGroup_001");
								if (subds83.FindDataElement(t85))
								{
									const mdcm::DataElement & e85 =
										subds83.GetDataElement(t85);
									QMap<int,GEMSParam> m85;
									DicomUtils::read_gems_params(
										m85, e85, gdict);
									QMapIterator<QString, int>
										itmd(gdict);
									gems_us_text.append(QString(
										"<span class = 'y4'>&#160;") +
										QString("SQ 0x85:")+
										QString("</span><br/>"));
									while (itmd.hasNext())
									{
										itmd.next();
										if (m85.contains(
											itmd.value()))
										{
											const GEMSParam & gp =
												m85.value(
													itmd.value());
											const QList<QVariant> &
												l85 = gp.values;
											QString tmp85("");
											for (int x85 = 0;
												x85 < l85.size();
												++x85)
												tmp85.append(
													l85.at(x85).
														toString() +
													QString(" "));
											gems_us_text.append(
												QString(
													"&#160;") +
												itmd.key()+
												QString(": ")+
												tmp85 +
												QString("<br/>")); 
										}
									}
								}
							}
						}
					}
				}
			}
		}
		const mdcm::PrivateTag t73(
			0x7fe1,
			0x73,
			"GEMS_Ultrasound_MovieGroup_001");
		if (subds1.FindDataElement(t73))
		{
			const mdcm::DataElement & e73 =
				subds1.GetDataElement(t73);
			mdcm::SmartPointer<mdcm::SequenceOfItems>
				sq73 = e73.GetValueAsSQ();
			if (sq73 && sq73->GetNumberOfItems()>0)
			{
				const size_t n73 = sq73->GetNumberOfItems();
				for (size_t x73 = 0; x73 < n73; ++x73)
				{
					mdcm::Item & item73 = sq73->GetItem(x73+1);
					mdcm::DataSet & subds73 =
						item73.GetNestedDataSet();
					const mdcm::PrivateTag tname74(
						0x7fe1,
						0x74,
						"GEMS_Ultrasound_MovieGroup_001");
					QString qs74("");
					if (subds73.FindDataElement(tname74))
					{
						const mdcm::DataElement & e74 =
							subds73.GetDataElement(tname74);
						if (!e74.IsEmpty() &&
							!e74.IsUndefinedLength())
						{
							const mdcm::ByteValue * v74 =
								e74.GetByteValue();
							if (v74)
							{
								const char * b74 = v74->GetPointer();
								const unsigned int s74   = v74->GetLength();
								qs74 = QString::fromLatin1(
									b74, s74).
										trimmed().remove(QChar('\0')) +
									QString("&#160;");
							}
						}
					}
					gems_us_text.append(
						QString("<span class = 'y5'>") +
						qs74 +
						QString(
							"</span><span class = 'y4'>"
							"(SQ 0x73)</span><br/>"));
					const mdcm::PrivateTag t75(
						0x7fe1,
						0x75,
						"GEMS_Ultrasound_MovieGroup_001");
					if (subds73.FindDataElement(t75))
					{
						const mdcm::DataElement & e75 =
							subds73.GetDataElement(t75);
						QMap<int,GEMSParam> m75;
						DicomUtils::read_gems_params(m75, e75, gdict);
						QMapIterator<QString, int> itmd(gdict);
						gems_us_text.append(
							QString("<span class = 'y4'>") +
							QString("SQ 0x75:") +
							QString("</span><br/>"));
						while (itmd.hasNext())
						{
							itmd.next();
							if (m75.contains(itmd.value()))
							{
								const GEMSParam & gp =
									m75.value(itmd.value());
								const QList<QVariant> & l75 = gp.values;
								QString tmp75("");
								for (int x75 = 0;
									x75 < l75.size();
									++x75)
									tmp75.append(
										l75.at(x75).toString() +
										QString(" "));
								gems_us_text.append(
									itmd.key() +
									QString(": ") +
									tmp75 +
									QString("<br/>")); 
							}
						}
					}
				}
			}
		}
	}
	gems_us_text.append(foot);
	if (textEdit->document())
	{
		textEdit->document()->setHtml(gems_us_text);
		QTextCursor cursor = textEdit->textCursor();
		cursor.setPosition(0);
		textEdit->setTextCursor(cursor);
		textEdit->show();
	}
}

void SQtree::copy_to_clipboard()
{
	const QTreeWidgetItem * item =
		treeWidget->currentItem();
	if (item && QApplication::clipboard())
	{
		QApplication::clipboard()->setText(
			item->text(treeWidget->currentColumn()));
	}
}

void SQtree::collapse_item()
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = mutex.tryLock();
	if (!lock) return;
#endif
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	treeWidget->blockSignals(true);
	collapse_children(treeWidget->currentIndex());
	treeWidget->blockSignals(false);
	QApplication::restoreOverrideCursor();
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutex.unlock();
#endif
}

static int expanded_items = 0;
void SQtree::expand_item()
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = mutex.tryLock();
	if (!lock) return;
#endif
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	treeWidget->blockSignals(true);
	expanded_items = 0;
	expand_children(treeWidget->currentIndex());
	treeWidget->blockSignals(false);
	QApplication::restoreOverrideCursor();
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutex.unlock();
#endif
}

void SQtree::expand_children(const QModelIndex & index)
{
	if (!index.isValid()) return;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QAbstractItemModel * m = index.model();
	if (m)
	{
		for (int i = 0; i < index.model()->rowCount(index); ++i)
		{
			++expanded_items;
			if (expanded_items > 65000) break;
			expand_children(m->index(i, 0, index));
		}
	}
	if (!treeWidget->isExpanded(index))
		treeWidget->expand(index);
#else
	for (int i = 0; i < index.model()->rowCount(index); ++i)
	{
		++expanded_items;
		if (expanded_items > 65000) break;
	    expand_children(index.child(i, 0));
	}
	if (!treeWidget->isExpanded(index))
		treeWidget->expand(index);
#endif
}

void SQtree::collapse_children(const QModelIndex & index)
{
	if (!index.isValid()) return;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	if (treeWidget->isExpanded(index))
		treeWidget->collapse(index);
	const QAbstractItemModel * m = index.model();
	if (m)
	{
		for (int i = 0; i < index.model()->rowCount(index); ++i)
		{
			collapse_children(m->index(i, 0, index));
		}
	}
#else
	if (treeWidget->isExpanded(index))
		treeWidget->collapse(index);
	for (int i = 0; i < index.model()->rowCount(index); ++i)
	{
	    collapse_children(index.child(i, 0));
	}
#endif
}

void SQtree::dropEvent(QDropEvent * e)
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = mutex.tryLock();
	if (!lock) return;
#endif
	QStringList l;
	QList<QUrl> urls;
	const QMimeData * mimeData = e->mimeData();
	if (mimeData && mimeData->hasUrls())
	{
		urls = mimeData->urls();
		for (int i = 0; i < urls.size(); ++i)
			l.push_back(urls.at(i).toLocalFile());
		if (!l.empty())
		{
			list_of_files = QStringList(l.at(0));
			horizontalSlider->blockSignals(true);
			horizontalSlider->setMinimum(0);
			horizontalSlider->setMaximum(0);
			horizontalSlider->setValue(0);
			horizontalSlider->hide();
			horizontalSlider->blockSignals(false);
			read_file(l.at(0), false);
		}
	}
	else
	{
		clear_tree();
	}
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutex.unlock();
#endif
}

void SQtree::dragEnterEvent(QDragEnterEvent * e)
{
	e->acceptProposedAction();
}

void SQtree::dragMoveEvent(QDragMoveEvent * e)
{
	e->acceptProposedAction();
}

void SQtree::dragLeaveEvent(QDragLeaveEvent * e)
{
	e->accept();
}

void SQtree::clear_tree()
{
	lineEdit->clear();
	ms_lineEdit->clear();
	ts_lineEdit->clear();
	treeWidget->clear();
	if (textEdit->document())
		textEdit->document()->clear();
	textEdit->clear();
	textEdit->hide();
}

void SQtree::open_file()
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = mutex.tryLock();
	if (!lock) return;
#endif
	clear_tree();
	const QString f = QFileDialog::getOpenFileName(
		this,
		QString("Open File"),
		saved_dir,
		QString(),
		(QString*)NULL,
		(QFileDialog::ReadOnly
		/*| QFileDialog::DontUseNativeDialog*/
		));
	QFileInfo fi(f);
	if (fi.isFile())
	{
		list_of_files = QStringList(f);
		horizontalSlider->blockSignals(true);
		horizontalSlider->setMinimum(0);
		horizontalSlider->setMaximum(0);
		horizontalSlider->setValue(0);
		horizontalSlider->hide();
		horizontalSlider->blockSignals(false);
		read_file(f, false);
	}
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutex.unlock();
#endif
}

void SQtree::open_file_and_series()
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = mutex.tryLock();
	if (!lock) return;
#endif
	QString f = QFileDialog::getOpenFileName(
		this,
		QString("Open file and scan series"),
		saved_dir,
		QString(),
		(QString*)NULL,
		(QFileDialog::ReadOnly
		/*| QFileDialog::DontUseNativeDialog*/
		));
	QFileInfo fi(f);
	if (!fi.isFile())
	{
		clear_tree();
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
		mutex.unlock();
#endif
		return;
	}
	read_file_and_series(f, false);
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutex.unlock();
#endif
}

void SQtree::set_list_of_files(const QStringList & l)
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = mutex.tryLock();
	if (!lock) return;
#endif
	list_of_files = QStringList(l);
	const size_t x = list_of_files.size();
	horizontalSlider->blockSignals(true);
	horizontalSlider->setMinimum(0);
	horizontalSlider->setMaximum(x > 0 ? x-1 : 0);
	horizontalSlider->setValue(0);
	if (x > 1) horizontalSlider->show();
	else       horizontalSlider->hide();
	horizontalSlider->blockSignals(false);
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutex.unlock();
#endif
}

void SQtree::file_from_slider(int x)
{
	if (x >= list_of_files.size()) return;
	read_file(list_of_files.at(x), true);
}

