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
#include "codecutils.h"
#include "dicomutils.h"
#include "commonutils.h"
#include "filepath.h"

template <typename T, long long TVR>
void get_bin_values(
	const mdcm::DataElement & v,
	std::vector<T> & result)
{
	if (v.IsEmpty() ||
		v.IsUndefinedLength())
		return;
	const mdcm::ByteValue * bv = v.GetByteValue();
	if (!bv) return;
	if ((bv->GetLength() < sizeof(T)) ||
		((bv->GetLength() % sizeof(T)) != 0))
		return;
	mdcm::Element<TVR, mdcm::VM::VM1_n> e;
	e.SetFromDataElement(v);
	for (unsigned long x = 0; x < e.GetLength(); x++)
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
		const QStringList l =
			dir.entryList(QDir::Files|QDir::Readable, QDir::Name);
		for (int x = 0; x < l.size(); x++)
		{
			const QString tmp0 = QDir::toNativeSeparators(
				dir.absolutePath() + QDir::separator() + l.at(x));
			files.push_back(std::string(FilePath::getPath(tmp0)));
		}
	}
	const mdcm::Tag t(0x0020,0x000e);
	mdcm::Scanner s;
	s.AddTag(t);
	if(!s.Scan(files)) return;
	mdcm::Scanner::ValuesType v = s.GetValues();
	mdcm::Scanner::ValuesType::iterator it = v.begin();
	while (it != v.end())
	{
		const QString tmp0 = QString::fromLatin1((*it).c_str());
		if (
			tmp0.trimmed().remove(QChar('\0')) ==
				uid.trimmed().remove(QChar('\0')))
		{
			std::vector<std::string> f__ =
				s.GetAllFilenamesFromTagToValue(t, (*it).c_str());
			for (unsigned int j = 0; j < f__.size(); j++)
			{
				result.push_back(QString(f__[j].c_str()));
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

SQtree::SQtree(QWidget * p, bool t) : QWidget(p), skip_settings_pos(t)
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
		QStringList l;
		l << QString(tag.PrintAsPipeSeparatedString().c_str())
			<< tname
			<< QString("SQ")
			<< QString("")
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
				ce++;
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
					tmp1.sprintf(
						"%.2f",
						length / (1024.0*1024.0));
					length_s = tmp1 + QString(" MB");
				}
				else if (length > 1024)
				{
					tmp1.sprintf(
						"%.2f",
						bv->GetLength()/1024.0);
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
						for (unsigned long j = 0; j < values.size(); j++)
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
						for (unsigned long j = 0; j < values.size(); j++)
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
						for (unsigned long j = 0; j < values.size(); j++)
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
						for (unsigned long j = 0; j < values.size(); j++)
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
						for (unsigned long j = 0; j < values.size(); j++)
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
						for (unsigned long j = 0; j < values.size(); j++)
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
						for (unsigned long j = 0; j < values.size(); j++)
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
						for (unsigned long j = 0; j < values.size(); j++)
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
								tmp3.sprintf("%04x",group);
								QString tmp4;
								tmp4.sprintf("%04x",element);
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
							bv->GetLength()).trimmed();
						const QDate date_ =
							QDate::fromString(
								date_s, QString("yyyyMMdd"));
						tmp0 = date_.toString(
							QString("d MMM yyyy"));
						date_time = true;
					}
					else if (vr == mdcm::VR::TM)
					{
						const QString time0_s = QString::fromLatin1(
							bv->GetPointer(),
							bv->GetLength()).
								trimmed().remove(QChar('\0'));
						if (!time0_s.isEmpty())
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
					}
					else if (vr == mdcm::VR::DT)
					{
						const QString time0_s = QString::fromLatin1(
							bv->GetPointer(),
							bv->GetLength()).
								trimmed().remove(QChar('\0'));
						if (!time0_s.isEmpty())
						{
							const int point_idx =
								time0_s.indexOf(QString("."));
							if (point_idx == 14||point_idx == -1)
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

void SQtree::read_file(const QString & f)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	clear_tree();
	QFileInfo fi(f);
	saved_dir =
		QDir::toNativeSeparators(fi.absoluteDir().absolutePath());
	lineEdit->setText(QDir::toNativeSeparators(f));
	QStringList l;
	l << QString("") << QString("") <<
		QString("") << QString("") << QString("");
	QTreeWidgetItem * i = new QTreeWidgetItem(l);
	treeWidget->addTopLevelItem(i);
	{
		mdcm::Reader reader;
		const mdcm::File & file = reader.GetFile();
		bool ok = false;
		reader.SetFileName(FilePath::getPath(f));
		ok = reader.Read();
		if (!ok)
		{
			ms_lineEdit->setText(
				QString("Error: file is not DICOM or broken."));
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
		dump_csa(ds);
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
			ce++;
		}
	}
	treeWidget->expandToDepth(0);
	QApplication::restoreOverrideCursor();
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
	for (int i = 0; i < index.model()->rowCount(index); i++)
	{
		expanded_items++;
	if (expanded_items > 65000) break;
	    expand_children(index.child(i, 0));
	}
	if (!treeWidget->isExpanded(index))
		treeWidget->expand(index);
}

void SQtree::collapse_children(const QModelIndex & index)
{
	if (!index.isValid()) return;
	if (treeWidget->isExpanded(index))
		treeWidget->collapse(index);
	for (int i = 0; i < index.model()->rowCount(index); i++)
	{
	    collapse_children(index.child(i, 0));
	}
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
		for (int i = 0; i < urls.size(); i++)
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
			read_file(QDir::toNativeSeparators(l.at(0)));
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
		tr("Open File"),
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
		read_file(QDir::toNativeSeparators(f));
	}
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
	read_file(list_of_files.at(x));
}

void SQtree::open_file_and_series()
{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	const bool lock = mutex.tryLock();
	if (!lock) return;
#endif
	clear_tree();
	QString f = QFileDialog::getOpenFileName(
		this,
		tr("Open file and scan series"),
		saved_dir,
		QString(),
		(QString*)NULL,
		(QFileDialog::ReadOnly
		/*| QFileDialog::DontUseNativeDialog*/
		));
	QFileInfo fi(f);
	if (!fi.isFile())
	{
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
		mutex.unlock();
#endif
		return;
	}
	f = QDir::toNativeSeparators(fi.absoluteFilePath());
	QString series_uid;
	QStringList files;
	bool series_uid_ok = false;
	{
		std::set<mdcm::Tag> tags;
		tags.insert(mdcm::Tag(0x0020,0x000e));
		mdcm::Reader reader;
		reader.SetFileName(FilePath::getPath(f));
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
	int idx = -1;
	const int files_size = files.size();
	if (files_size > 1)
	{
		for (int x = 0; x < files_size; x++)
		{
			QFileInfo fi0(files.at(x));
			if (QDir::toNativeSeparators(fi0.absoluteFilePath()) == f)
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
	read_file(QDir::toNativeSeparators(f));
#if (defined SQTREE_LOCK_TREE && SQTREE_LOCK_TREE==1)
	mutex.unlock();
#endif
}
