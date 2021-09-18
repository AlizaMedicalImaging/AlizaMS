#include "helpwidget.h"
#include "anonymazerwidget2.h"
#include <QtGlobal>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QSet>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QMimeData>
#include <QUrl>
#include "mdcmUIDGenerator.h"
#include "mdcmGlobal.h"
#include "mdcmDicts.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmFile.h"
#include "mdcmMediaStorage.h"
#include "mdcmReader.h"
#include "mdcmWriter.h"
#include "mdcmAttribute.h"
#include "mdcmVR.h"
#include "mdcmVM.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmVersion.h"
#include "mdcmParseException.h"
#include "dicomutils.h"
#include "codecutils.h"
#include "alizams_version.h"
#include <cstdlib>
#include <chrono>
#include <random>

static mdcm::VR get_vr(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::VR vr = mdcm::VR::INVALID;
	bool priv = false;
	if (t.IsIllegal())
	{
		return vr; //
	}
	else if (t.IsPrivateCreator())
	{
		if (!implicit)
		{
			if (ds.FindDataElement(t))
			{
				vr = ds.GetDataElement(t).GetVR();
			}
		}
		else
		{
			vr = mdcm::VR::LO; //
		}
	}
	else if (t.IsPrivate())
	{
		priv = true;
	}
	if (!priv)
	{
		if (!implicit)
		{
			if (ds.FindDataElement(t))
			{
				vr = ds.GetDataElement(t).GetVR();
			}
			else
			{
				const mdcm::DictEntry & dictentry = dicts.GetDictEntry(t);
				vr = dictentry.GetVR();
			}
		}
		else
		{
			const mdcm::DictEntry & dictentry = dicts.GetDictEntry(t);
			vr = dictentry.GetVR();
		}
	}
	else
	{
		if (!implicit)
		{
			vr = ds.GetDataElement(t).GetVR();
		}
		else
		{
			const mdcm::PrivateDict & pdict = dicts.GetPrivateDict();
			mdcm::Tag private_creator_t = t.GetPrivateCreator();
			if (ds.FindDataElement(private_creator_t))
			{
				const mdcm::DataElement & private_creator_e =
					ds.GetDataElement(private_creator_t);
				if (!private_creator_e.IsEmpty() &&
					!private_creator_e.IsUndefinedLength() &&
					private_creator_e.GetByteValue())
				{
					const QString private_creator =
						QString::fromLatin1(
							private_creator_e.GetByteValue()->GetPointer(),
							private_creator_e.GetByteValue()->GetLength());
					mdcm::PrivateTag pt(
						t.GetGroup(),
						t.GetElement(),
						private_creator.toLatin1().constData());
					const mdcm::DictEntry & pentry = pdict.GetDictEntry(pt);
					vr = pentry.GetVR();
				}
			}
		}
	}
	return vr;
}

static bool compatible_sq(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	if (t.IsIllegal()) return false;
	mdcm::VR vr = get_vr(ds, t, implicit, dicts);
	if (vr.Compatible(mdcm::VR::SQ)) return true;
	return false;
}

static bool is_date_time(const mdcm::VR & vr, const mdcm::Tag & t)
{
	if (vr == mdcm::VR::DA ||
		vr == mdcm::VR::DT ||
		vr == mdcm::VR::TM)
	{
		// TODO
		if (t != mdcm::Tag(0x0008,0x0106) && // Context Group Version
			t != mdcm::Tag(0x0008,0x0107) && // Context Group Local Version
			t != mdcm::Tag(0x0040,0xdb06) && // Template Version
			t != mdcm::Tag(0x0040,0xdb07) && // Template Local Version
			t != mdcm::Tag(0x0044,0x000b))   // Product Expiration DateTime
		return true;
		
	}
	return false;
}

static void replace__(
	mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const char * value,
	const size_t size_,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	if (t.GetGroup() < 0x0008) return;
	mdcm::VR vr = get_vr(ds, t, implicit, dicts);
	mdcm::VL vl = static_cast<mdcm::VL::Type>(size_);
	if (vr.Compatible(mdcm::VR::SQ))
	{
		if (vr == mdcm::VR::SQ && vl == 0 && value && *value == 0 )
		{
			mdcm::SmartPointer<mdcm::SequenceOfItems>
				sq = new mdcm::SequenceOfItems();
			sq->SetLength(0);
			sq->SetNumberOfItems(0);
			mdcm::DataElement de(t);
			if (!implicit) de.SetVR(mdcm::VR::SQ);
			de.SetValue(*sq);
			de.SetVLToUndefined();
			ds.Replace(de);
		}
		else
		{
			mdcmAlwaysWarnMacro("Cannot process " << t << " " << vr);
		}
	}
	else if (vr & mdcm::VR::VRBINARY)
	{
		if (vl == 0)
		{
			mdcm::DataElement de(t);
			if (!implicit) de.SetVR(vr);
			de.SetByteValue("", 0);
			ds.Replace(de);
		}
		else
		{
			mdcmAlwaysWarnMacro("Error " << t << " " << vr);
		}
	}
	else
	{
		if (value)
		{
			std::string padded(value, vl);
			// ASCII VR needs to be padded with a space
			if (vl.IsOdd())
			{
				if (vr == mdcm::VR::UI)
				{
					// \0 is automatically added when using a ByteValue
				}
				else
				{
					padded += " ";
				}
			}
			mdcm::DataElement de(t);
			if (!implicit) de.SetVR(vr);
			mdcm::VL paddedSize = static_cast<mdcm::VL::Type>(padded.size());
			de.SetByteValue(padded.c_str(), paddedSize);
			ds.Replace(de);
		}
	}
}

static void replace_uid_recurs__(
	mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	const QMap<QString, QString> & m,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		mdcm::Tag t = de1.GetTag();
		mdcm::VR vr = get_vr(ds, t, implicit, dicts);
		++it;
		const mdcm::DataElement & de = *dup;
		if (ts.find(t) != ts.end() || vr == mdcm::VR::UI)
		{		
			const mdcm::ByteValue * bv = de.GetByteValue();
			if (bv)
			{
				const QString s = QString::fromLatin1(
					bv->GetPointer(),
					bv->GetLength()).trimmed();
				if (!s.isEmpty())
				{
					if (m.contains(s))
					{
						const QString v = m.value(s);
						replace__(ds, t, v.toLatin1().constData(), v.size(), implicit, dicts);
					}
				}
			}
		}
		else
		{
			if (vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						replace_uid_recurs__(nested, ts, m, implicit, dicts);
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

static void replace_pn_recurs__(
	mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	const QMap<QString, QString> & m,
	const bool implicit,
	const bool one_patient,
	const QString & single_name,
	const QString & charset,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::Tag t = de1.GetTag();
		mdcm::VR vr = get_vr(ds, t, implicit, dicts);
		mdcm::DataSet::Iterator dup = it;
		++it;
		const mdcm::DataElement & de = *dup;
		if (ts.find(t) != ts.end() || vr == mdcm::VR::PN)
		{
			if (!single_name.isEmpty() && one_patient && t == mdcm::Tag(0x0010,0x0010))
			{
				QString sn = single_name.simplified();
				sn.replace(QString(" "), QString("^"));
				if (!charset.isEmpty())
				{
					bool ok = false;
					const QByteArray ba = CodecUtils::fromUTF8(
						sn,
						charset.toLatin1().constData(),
						&ok);
					if (!ok)
					{
						std::cout << "Warning: provided Patient Name may be incorrectly encoded"
							<< std::endl;
					}
					replace__(ds, t, ba.constData(), ba.size(), implicit, dicts);
				}
				else
				{
					replace__(ds, t, sn.toLatin1().constData(), sn.toLatin1().size(), implicit, dicts);
				}
			}
			else
			{
				const mdcm::ByteValue * bv = de.GetByteValue();
				if (bv)
				{
					const QString s = QString::fromLatin1(
						bv->GetPointer(),
						bv->GetLength()).trimmed();
					if (!s.isEmpty())
					{
						if (m.contains(s))
						{
							const QString v = m.value(s);
							replace__(
								ds, t, v.toUtf8().constData(), v.size(), implicit, dicts);
						}
					}
				}
			}
		}
		else
		{
			if (vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						replace_pn_recurs__(
							nested,
							ts,
							m,
							implicit,
							one_patient, single_name, charset,
							dicts);
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

static void replace_id_recurs__(
	mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	const QMap<QString, QString> & m,
	const bool implicit,
	const bool one_patient,
	const QString & single_id,
	const QString & charset,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::Tag t = de1.GetTag();
		mdcm::VR vr = get_vr(ds, t, implicit, dicts);
		mdcm::DataSet::Iterator dup = it;
		++it;
		const mdcm::DataElement & de = *dup;
		if (ts.find(t) != ts.end())
		{
			if (one_patient && t == mdcm::Tag(0x0010,0x0020))
			{
				QString si = single_id.simplified();
				si.replace(QString(" "), QString("^"));
				if (!charset.isEmpty())
				{
					bool ok = false;
					const QByteArray ba = CodecUtils::fromUTF8(
						si,
						charset.toLatin1().constData(),
						&ok);
					if (!ok)
					{
						std::cout << "Warning: provided Patient ID may be incorrectly encoded"
							<< std::endl;
					}
					replace__(ds, t, ba.constData(), ba.size(), implicit, dicts);
				}
				else
				{
					replace__(ds, t, si.toLatin1().constData(), si.toLatin1().size(), implicit, dicts);
				}
			}
			else
			{
				const mdcm::ByteValue * bv = de.GetByteValue();
				if (bv)
				{
					const QString s = QString::fromLatin1(
						bv->GetPointer(),
						bv->GetLength()).trimmed();
					if (!s.isEmpty())
					{
						if (m.contains(s))
						{
							const QString v = m.value(s);
							replace__(
								ds, t, v.toUtf8().constData(), v.size(), implicit, dicts);
						}
					}
				}
			}
		}
		else
		{
			if (vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						replace_id_recurs__(
							nested,
							ts,
							m,
							implicit,
							one_patient, single_id, charset,
							dicts);
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
	mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::Tag t = de1.GetTag();
		mdcm::DataSet::Iterator dup = it;
		++it;
		if (ts.find(t) != ts.end())
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			if (compatible_sq(const_cast<const mdcm::DataSet&>(ds), t, implicit, dicts))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_recurs__(nested, ts, implicit, dicts);
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

static void zero_sq_recurs__(
	mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::Tag t = de1.GetTag();
		mdcm::DataSet::Iterator dup = it;
		++it;
		if (ts.find(t) != ts.end())
		{
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
				new mdcm::SequenceOfItems();
			sq->SetLength(0);
			sq->SetNumberOfItems(0);
			mdcm::DataElement e(t);
			if (!implicit) e.SetVR(mdcm::VR::SQ);
			e.SetValue(*sq);
			e.SetVLToUndefined();
			ds.Replace(e);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			if (compatible_sq(const_cast<const mdcm::DataSet&>(ds), t, implicit, dicts))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						zero_sq_recurs__(nested, ts, implicit, dicts);
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

#if 0
static void remove_date_time_recurs__(
	mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::Tag t = de1.GetTag();
		mdcm::VR vr = get_vr(ds, t, implicit, dicts);
		mdcm::DataSet::Iterator dup = it;
		++it;
		if (t == mdcm::Tag(0x0008,0x0021))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0021), "", 0, implicit, dicts);
		}
		else if (t == mdcm::Tag(0x0008,0x0031))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0031), "", 0, implicit, dicts);
		}
		else if (t == mdcm::Tag(0x0008,0x0020))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0020), "", 0, implicit, dicts);
		}
		else if (t == mdcm::Tag(0x0008,0x0030))
		{
			replace__(ds, mdcm::Tag(0x0008,0x0030), "", 0, implicit, dicts);
		}
		else if (is_date_time(vr, t) || (ts.find(t) != ts.end()))
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			if (vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_date_time_recurs__(nested, ts, implicit, dicts);
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
#endif

static bool find_time_less_1h_recurs__(
	const mdcm::DataSet & ds,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::ConstIterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::Tag t = de1.GetTag();
		mdcm::VR vr = get_vr(ds, t, implicit, dicts);
		mdcm::DataSet::ConstIterator dup = it;
		++it;
		if (is_date_time(vr, t))
		{
			if (t == mdcm::Tag(0x0008,0x0201)) continue; // UTC offset
			const mdcm::DataElement & de = *dup;
			const mdcm::ByteValue * bv = de.GetByteValue();
			if (bv)
			{
				QString r("");
				const QString s = QString::fromLatin1(
					bv->GetPointer(),
					bv->GetLength()).trimmed().remove(QChar('\0'));
				if (!s.isEmpty())
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
  					const QStringList l = s.split(QString("\\"), Qt::KeepEmptyParts);
#else
  					const QStringList l = s.split(QString("\\"), QString::KeepEmptyParts);
#endif
					const int l_size = l.size();
					for (int x = 0; x < l_size; ++x)
					{
						const QString s0 = l.at(x).trimmed();
						if (!s0.isEmpty())
						{
							if (vr == mdcm::VR::TM)
							{
								const QString t0 = s0.left(2);
								bool b0 = false;
								int h0 = QVariant(t0).toInt(&b0);
								if (b0 && h0 < 1)
								{
									return true;
								}
							}
							else if (vr == mdcm::VR::DT)
							{
								if (s0.length() >= 10)
								{
									const int tmp0 = s0.indexOf(QString("-"));
									const int tmp1 = s0.indexOf(QString("+"));
									if ((tmp0 == -1 || tmp0 > 8) && (tmp1 == -1 || tmp1 > 8))
									{
										const QString t0 = s0.mid(8,2);
										bool b0 = false;
										int h0 = QVariant(t0).toInt(&b0);
										if (b0 && h0 < 1)
										{
											return true;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			if (vr.Compatible(mdcm::VR::SQ))
			{
				const mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					const mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						const mdcm::Item    & item   = sq->GetItem(i);
						const mdcm::DataSet & nested = item.GetNestedDataSet();
						const bool t = find_time_less_1h_recurs__(nested, implicit, dicts);
						if (t) return true;
					}
				}
			}
		}
	}
	return false;
}

static void modify_date_time_recurs__(
	mdcm::DataSet & ds,
	const bool less1h,
	const bool implicit,
	const mdcm::Dicts & dicts,
	const int y_off,
	const int m_off,
	const int d_off,
	const int s_off)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::Tag t = de1.GetTag();
		mdcm::VR vr = get_vr(ds, t, implicit, dicts);
		mdcm::DataSet::Iterator dup = it;
		++it;
		if (is_date_time(vr, t))
		{
			const mdcm::DataElement & de = *dup;
			const mdcm::ByteValue * bv = de.GetByteValue();
			if (t == mdcm::Tag(0x0008,0x0201))
			{
				const QString r = QString("+0000");
				mdcm::DataElement de2(t);
				if (!implicit) de2.SetVR(mdcm::VR::SH);
				de2.SetByteValue(r.toLatin1(), r.length());
				ds.Replace(de2);
			}
			else
			{
				if (bv)
				{
					QString r("");
					const QString s = QString::fromLatin1(
						bv->GetPointer(),
						bv->GetLength()).trimmed().remove(QChar('\0'));
					if (!s.isEmpty())
					{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
  						const QStringList l = s.split(QString("\\"), Qt::KeepEmptyParts);
#else
  						const QStringList l = s.split(QString("\\"), QString::KeepEmptyParts);
#endif
						const int l_size = l.size();
						for (int x = 0; x < l_size; ++x)
						{
							const QString s0 = l.at(x).trimmed();
							if (!s0.isEmpty())
							{
								const int s0_length = s0.length();
								if (vr == mdcm::VR::DA)
								{
									if (s0_length == 8)
									{
										QDate d = QDate::fromString(s0, QString("yyyyMMdd"));
										d = d.addYears(-y_off);
										d = d.addMonths(-m_off);
										d = d.addDays(-d_off);
										r += d.toString(QString("yyyyMMdd"));
									}
								}
								else if (vr == mdcm::VR::TM)
								{
									const int tmp0 = s0.indexOf(QString("."));
									if (tmp0 == 6)
									{
										const QString s1 = s0.left(6);
										if (less1h)
										{
											r += s1 + QString(".000000");
										}
										else
										{
											QTime t = QTime::fromString(s1, QString("HHmmss"));
											t = t.addSecs(-s_off);
											r += t.toString(QString("HHmmss")) + QString(".000000");
										}
									}
									else if (tmp0 == -1)
									{
										if (less1h)
										{
											if (s0_length == 6)
											{
												r += s0 + QString(".000000");
											}
											else if (s0_length == 4 || s0_length == 2)
											{
												r += s0;
											}
										}
										else
										{
											if (s0_length == 2)
											{
												r += s0;
											}
											else if (s0_length == 4)
											{
												QTime t = QTime::fromString(s0, QString("HHmm"));
												t = t.addSecs(-s_off);
												r += t.toString(QString("HHmm"));
											}
											else if (s0_length == 6)
											{
												QTime t = QTime::fromString(s0, QString("HHmmss"));
												t = t.addSecs(-s_off);
												r += t.toString(QString("HHmmss"));
											}
										}
									}
								}
								else if (vr == mdcm::VR::DT)
								{
									const int tmp0 = s0.indexOf(QString("-"));
									const int tmp1 = s0.indexOf(QString("+"));
									QString s1("");
									if (tmp0 == -1 && tmp1 == -1)
									{
										s1 = s0;
									}
									else
									{
										if (tmp0 >= 0 && tmp1 >= 0)
										{
											;; // error
										}
										else if (tmp0 >= 4)
										{
											s1 = s0.left(tmp0);
										}
										else if (tmp1 >= 4)
										{
											s1 = s0.left(tmp1);
										}
									}
									const int s1_length = s1.length();
									if (s1_length == 4)
									{
										QDate d = QDate::fromString(s1, QString("yyyy"));
										d = d.addYears(-y_off);
										r += d.toString(QString("yyyy"));
									}
									else if (s1_length == 6)
									{
										QDate d = QDate::fromString(s1, QString("yyyyMM"));
										d = d.addYears(-y_off);
										d = d.addMonths(-m_off);
										r += d.toString(QString("yyyyMM"));
									}
									else if (s1_length == 8)
									{
										QDate d = QDate::fromString(s1, QString("yyyyMMdd"));
										d = d.addYears(-y_off);
										d = d.addMonths(-m_off);
										d = d.addDays(-d_off);
										r += d.toString(QString("yyyyMMdd"));
									}
									else if (s1_length == 10)
									{
										QDateTime d = QDateTime::fromString(s1, QString("yyyyMMddHH"));
										d = d.addYears(-y_off);
										d = d.addMonths(-m_off);
										d = d.addDays(-d_off);
										if (less1h)
										{
											;;
										}
										else
										{
											d = d.addSecs(-s_off);
										}
										r += d.toString("yyyyMMddHH");
									}
									else if (s1_length == 12)
									{
										QDateTime d = QDateTime::fromString(s1, QString("yyyyMMddHHmm"));
										d = d.addYears(-y_off);
										d = d.addMonths(-m_off);
										d = d.addDays(-d_off);
										if (less1h)
										{
											;;
										}
										else
										{
											d = d.addSecs(-s_off);
										}
										r += d.toString("yyyyMMddHHmm");
									}
									else if (s1_length == 14)
									{
										QDateTime d = QDateTime::fromString(s1, QString("yyyyMMddHHmmss"));
										d = d.addYears(-y_off);
										d = d.addMonths(-m_off);
										d = d.addDays(-d_off);
										if (less1h)
										{
											;;
										}
										else
										{
											d = d.addSecs(-s_off);
										}
										r += d.toString("yyyyMMddHHmmss");
									}
									else if (s1_length >= 15 && s1.indexOf(QString(".")) == 14)
									{
										const QString s2 = s1.left(14);
										QDateTime d = QDateTime::fromString(s2, QString("yyyyMMddHHmmss"));
										d = d.addYears(-y_off);
										d = d.addMonths(-m_off);
										d = d.addDays(-d_off);
										if (less1h)
										{
											;;
										}
										else
										{
											d = d.addSecs(-s_off);
										}
										r += d.toString("yyyyMMddHHmmss") + QString(".000000");
									}
								}
							}
							if ((l_size > 1) && (x < (l_size - 1)))
							{
								r += QString("\\");
							}
						}
						mdcm::DataElement de2(t);
						if (!implicit) de2.SetVR(vr);
						de2.SetByteValue(r.toLatin1(), r.length());
						ds.Replace(de2);
					}
				}
			}
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			if (vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						modify_date_time_recurs__(
							nested, less1h, implicit, dicts, y_off, m_off, d_off, s_off);
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
	mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		mdcm::Tag t = de1.GetTag();
		++it;
		if (ts.find(t) != ts.end())
		{
			replace__(ds, t, "", 0, implicit, dicts);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			if (compatible_sq(const_cast<const mdcm::DataSet&>(ds), t, implicit, dicts))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						empty_recurs__(nested, ts, implicit, dicts);
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

static void remove_private__(
	mdcm::DataSet & ds,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (;it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		if (de1.GetTag().IsPrivate())
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::Tag t = de.GetTag();
			if (compatible_sq(const_cast<const mdcm::DataSet&>(ds), t, implicit, dicts))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_private__(nested, implicit, dicts);
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

static bool check_overlay_in_pixeldata(const mdcm::DataSet & ds)
{
	mdcm::Tag t(0x6000,0x0000);
	while(true)
	{
		const mdcm::DataElement & de = ds.FindNextDataElement(t);
		if(de.GetTag().GetGroup() > 0x60ff)
		{
			break;
		}
		else if(de.GetTag().IsPrivate())
		{
			t.SetGroup((uint16_t)(de.GetTag().GetGroup() + 1));
			t.SetElement(0);
		}
		else
		{
			t = de.GetTag();
			mdcm::Tag tOverlayData(t.GetGroup(),0x3000);
			mdcm::Tag tOverlayBitpos(t.GetGroup(),0x0102);
			if(ds.FindDataElement(tOverlayData))
			{
				;;
			}
			else if(ds.FindDataElement(tOverlayBitpos))
			{
				return true;
			}
			t.SetGroup((uint16_t)(t.GetGroup() + 2));
			t.SetElement(0);
		}
	}
	return false;
}

static void remove_overlays__(mdcm::DataSet & ds)
{
	std::vector<mdcm::Tag> tmp0;
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End(); ++it)
	{
		const mdcm::DataElement & de = *it;
		mdcm::Tag t = de.GetTag();
		if (t.GetGroup() >= 0x6000 && t.GetGroup() <= 0x60ff && (t.GetGroup() % 2 == 0))
		{
			tmp0.push_back(t);
		}
		if (t.GetGroup() > 0x60ff) break;
	}
	for (unsigned int x = 0; x < tmp0.size(); ++x) ds.Remove(tmp0.at(x));
}

static void remove_curves__(mdcm::DataSet & ds)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	std::vector<mdcm::Tag> tmp0;
	for (; it != ds.End(); ++it)
	{
		const mdcm::DataElement & de = *it;
		mdcm::Tag t = de.GetTag();
		if (t.GetGroup() >= 0x5000 && t.GetGroup() <= 0x50ff && (t.GetGroup() % 2 == 0))
		{
			tmp0.push_back(t);
		}
		if (t.GetGroup() > 0x50ff) break;
	}
	for (unsigned int x = 0; x < tmp0.size(); ++x) ds.Remove(tmp0.at(x));
}

static void remove_group_length__(
	mdcm::DataSet & ds,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::Iterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de1 = *it;
		mdcm::DataSet::Iterator dup = it;
		++it;
		if (de1.GetTag().IsGroupLength())
		{
			ds.GetDES().erase(dup);
		}
		else
		{
			const mdcm::DataElement & de = *dup;
			mdcm::Tag t = de.GetTag();
			if (compatible_sq(const_cast<const mdcm::DataSet&>(ds), t, implicit, dicts))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						remove_group_length__(nested, implicit, dicts);
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

static QString generate_random_name(const bool random_names)
{
	QStringList surnames;
	if (random_names)
	{
		surnames.push_back(QString("Smith"));
		surnames.push_back(QString("Jones"));
		surnames.push_back(QString("Taylor"));
		surnames.push_back(QString("Brown"));
		surnames.push_back(QString("Williams"));
		surnames.push_back(QString("Wilson"));
		surnames.push_back(QString("Johnson"));
		surnames.push_back(QString("Davies"));
		surnames.push_back(QString("Robinson"));
		surnames.push_back(QString("Miller"));
		surnames.push_back(QString("Thompson"));
		surnames.push_back(QString("Evans"));
		surnames.push_back(QString("Walker"));
		surnames.push_back(QString("White"));
		surnames.push_back(QString("Roberts"));
		surnames.push_back(QString("Green"));
		surnames.push_back(QString("Lewis"));
		surnames.push_back(QString("Thomas"));
		surnames.push_back(QString("Clarke"));
		surnames.push_back(QString("Jackson"));
		surnames.push_back(QString("Wood"));
		surnames.push_back(QString("Harris"));
		surnames.push_back(QString("Edwards"));
		surnames.push_back(QString("Turner"));
		surnames.push_back(QString("Adams"));
		surnames.push_back(QString("Baker"));
		surnames.push_back(QString("Hall"));
		surnames.push_back(QString("Mitchell"));
		surnames.push_back(QString("Garcia"));
		surnames.push_back(QString("Rodriguez"));
		surnames.push_back(QString("Martinez"));
		surnames.push_back(QString("Hernandez"));
		surnames.push_back(QString("Gonzales"));
		surnames.push_back(QString("Perez"));
		surnames.push_back(QString("Sanchez"));
		surnames.push_back(QString("Torres"));
		surnames.push_back(QString("Gomez"));
		surnames.push_back(QString("Diaz"));
		surnames.push_back(QString("Cruz"));
		surnames.push_back(QString("Reyes"));
		surnames.push_back(QString("Morales"));
		surnames.push_back(QString("Chavez"));
		surnames.push_back(QString("Alvarez"));
	}
	QStringList abbs;
	abbs.push_back(QString("A."));
	abbs.push_back(QString("B."));
	abbs.push_back(QString("C."));
	abbs.push_back(QString("D."));
	abbs.push_back(QString("E."));
	abbs.push_back(QString("F."));
	abbs.push_back(QString("G."));
	abbs.push_back(QString("H."));
	abbs.push_back(QString("I."));
	abbs.push_back(QString("J."));
	abbs.push_back(QString("K."));
	abbs.push_back(QString("L."));
	abbs.push_back(QString("M."));
	abbs.push_back(QString("N."));
	abbs.push_back(QString("O."));
	abbs.push_back(QString("Q."));
	abbs.push_back(QString("P."));
	abbs.push_back(QString("R."));
	abbs.push_back(QString("S."));
	abbs.push_back(QString("T."));
	abbs.push_back(QString("U."));
	abbs.push_back(QString("V."));
	abbs.push_back(QString("W."));
	abbs.push_back(QString("X."));
	abbs.push_back(QString("Y."));
	abbs.push_back(QString("Z."));
	const unsigned long long seed =
		std::chrono::high_resolution_clock::now()
			.time_since_epoch()
			.count();
	std::mt19937_64 mtrand(seed);
	const int tmp0 = abbs.size();
	const int tmp1 = random_names ? surnames.size() : tmp0;
	const int surname_idx = mtrand() % tmp1;
	const int abbs_idx1 = mtrand() % tmp0;
	const int abbs_idx2 = mtrand() % tmp0;
	const QString r =
		(random_names ? surnames.at(surname_idx) : abbs.at(surname_idx)) +
		QString("^") +
		abbs.at(abbs_idx1) +
		QString("^") +
		abbs.at(abbs_idx2) +
		QString("^^");
	return r;
}

static void anonymize_file__(
	bool * ok,
	bool * overlay_in_data,
	const QString & filename,
	const QString & outfilename,
	const mdcm::Dicts & dicts,
	const std::set<mdcm::Tag> & pn_tags,
	const std::set<mdcm::Tag> & uid_tags,
	const std::set<mdcm::Tag> & id_tags,
	const std::set<mdcm::Tag> & empty_tags,
	const std::set<mdcm::Tag> & remove_tags,
	const std::set<mdcm::Tag> & zero_seq_tags,
	const std::set<mdcm::Tag> & dev_remove_tags,
	const std::set<mdcm::Tag> & dev_empty_tags,
	const std::set<mdcm::Tag> & dev_replace_tags,
	const std::set<mdcm::Tag> & patient_tags,
	const std::set<mdcm::Tag> & inst_remove_tags,
	const std::set<mdcm::Tag> & inst_empty_tags,
	const std::set<mdcm::Tag> & inst_replace_tags,
	const std::set<mdcm::Tag> & time_tags,
	const std::set<mdcm::Tag> & descr_remove_tags,
	const std::set<mdcm::Tag> & descr_empty_tags,
	const std::set<mdcm::Tag> & descr_replace_tags,
	const std::set<mdcm::Tag> & struct_zero_tags,
	const QMap<QString, QString> & uid_map,
	const QMap<QString, QString> & pn_map,
	const QMap<QString, QString> & id_map,
	const bool preserve_uids,
	const bool remove_private,
	const bool remove_graphics,
	const bool remove_descriptions,
	const bool remove_struct,
	const bool retain_dates_times,
	const bool retain_device_id,
	const bool retain_patient_chars,
	const bool retain_institution_id,
	const bool confirm_clean_pixel,
	const bool confirm_no_recognizable,
	const int y_off,
	const int m_off,
	const int d_off,
	const int s_off,
	const bool one_patient,
	const QString & single_name,
	const QString & single_id)
{
	mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	reader.SetFileName(QDir::toNativeSeparators(filename).toUtf8().constData());
#else
	reader.SetFileName(QDir::toNativeSeparators(filename).toLocal8Bit().constData());
#endif
#else
	reader.SetFileName(filename.toLocal8Bit().constData());
#endif
	if (!reader.Read())
	{
		if (DicomUtils::is_dicom_file(filename)) *ok = false;
		else *ok = true;
		return;
	}
	mdcm::File    & file = reader.GetFile();
	mdcm::DataSet & ds   = file.GetDataSet();
	mdcm::FileMetaInformation & header = file.GetHeader();
	mdcm::TransferSyntax ts = header.GetDataSetTransferSyntax();
	const bool implicit = ts.IsImplicit();
#if 1
	mdcm::MediaStorage ms;
	ms.SetFromFile(file);
	if (ms == mdcm::MediaStorage::MediaStorageDirectoryStorage)
	{
		*ok = true;
		return;
	}
#endif
	//
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
				ce_.GetByteValue()->GetLength()).trimmed();
		}
	}
	//
	if (remove_private)
	{
		remove_private__(ds, implicit, dicts);
	}
	//
	if (remove_graphics)
	{
		remove_curves__(ds);
		std::set<mdcm::Tag> ts;
		ts.insert(mdcm::Tag(0x0070,0x0001));
		remove_recurs__(ds, ts, implicit, dicts);
		if (ds.FindDataElement(mdcm::Tag(0x0028,0x0100)) &&
			ds.FindDataElement(mdcm::Tag(0x0028,0x0101)))
		{
			mdcm::Attribute<0x0028,0x0100> at1;
			at1.Set(ds);
			const unsigned short bitsallocated = at1.GetValue();
			mdcm::Attribute<0x0028,0x0101> at2;
			at2.Set(ds);
			const unsigned short bitsstored = at2.GetValue();
			if (bitsallocated > bitsstored)
			{
				*overlay_in_data = check_overlay_in_pixeldata(
					const_cast<const mdcm::DataSet&>(ds));
			}
		}
		remove_overlays__(ds);
	}
	//
	zero_sq_recurs__(ds, zero_seq_tags, implicit, dicts);
	//
	if (remove_struct)
	{
		zero_sq_recurs__(ds, struct_zero_tags, implicit, dicts);
	}
	//
	empty_recurs__(ds, empty_tags, implicit, dicts);
	//
	remove_recurs__(ds, remove_tags, implicit, dicts);
	//
	if (remove_descriptions)
	{
		remove_recurs__(ds, descr_remove_tags, implicit, dicts);
		empty_recurs__(ds, descr_empty_tags, implicit, dicts);
		empty_recurs__(ds, descr_replace_tags, implicit, dicts); // FIXME
	}
	//
	if (!retain_device_id)
	{
		remove_recurs__(ds, dev_remove_tags, implicit, dicts);
		empty_recurs__(ds, dev_empty_tags, implicit, dicts);
		empty_recurs__(ds, dev_replace_tags, implicit, dicts); // FIXME
	}
	//
	if (!retain_patient_chars)
	{
		remove_recurs__(ds, patient_tags, implicit, dicts);
	}
	//
	if (!retain_institution_id)
	{
		remove_recurs__(ds, inst_remove_tags, implicit, dicts);
		empty_recurs__(ds, inst_empty_tags, implicit, dicts);
		empty_recurs__(ds, inst_replace_tags, implicit, dicts); // FIXME
	}
	//
	if (!retain_dates_times)
	{
		// always modify
#if 0
		if (true)
		{
#endif
			const bool less1h = find_time_less_1h_recurs__(
				const_cast<const mdcm::DataSet&>(ds), implicit, dicts);
			modify_date_time_recurs__(
				ds, less1h, implicit, dicts, y_off, m_off, d_off, s_off);
#if 0
		}
		else
		{
			remove_date_time_recurs__(ds, time_tags, implicit, dicts);
		}
#endif
	}
	//
	if (preserve_uids)
	{
		std::set<mdcm::Tag> always_replace;
		always_replace.insert(mdcm::Tag(0x0400,0x0100)); // Digital Signature UID, replace always
		replace_uid_recurs__(ds, always_replace, uid_map, implicit, dicts);
	}
	else
	{
		replace_uid_recurs__(ds, uid_tags, uid_map, implicit, dicts);
	}
	//
	replace_pn_recurs__(ds, pn_tags, pn_map, implicit, one_patient, single_name, charset, dicts);
	//
	replace_id_recurs__(ds, id_tags, id_map, implicit, one_patient, single_id, charset, dicts);
	//
	remove_group_length__(ds, implicit, dicts);
	//
	const QString s0("YES");
	const QString s1("DICOM PS 3.15 E.1 2021b");
	replace__(ds, mdcm::Tag(0x0012,0x0062), s0.toLatin1().constData(), s0.toLatin1().length(), implicit, dicts);
	replace__(ds, mdcm::Tag(0x0012,0x0063), s1.toLatin1().constData(), s1.toLatin1().length(), implicit, dicts);
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
		const QLatin1String dcm("DCM");
#else
		const QString dcm("DCM");
#endif
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq = new mdcm::SequenceOfItems();
		sq->SetLengthToUndefined();
		//
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113100");
			const QLatin1String cm("Basic Application Confidentiality Profile");
#else
			const QString cv("113100");
			const QString cm("Basic Application Confidentiality Profile");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (confirm_clean_pixel)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113101");
			const QLatin1String cm("Clean Pixel Data Option");
#else
			const QString cv("113101");
			const QString cm("Clean Pixel Data Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (confirm_no_recognizable)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113102");
			const QLatin1String cm("Clean Recognizable Visual Features Option");
#else
			const QString cv("113102");
			const QString cm("Clean Recognizable Visual Features Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (!remove_graphics)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113103");
			const QLatin1String cm("Clean Graphics Option");
#else
			const QString cv("113103");
			const QString cm("Clean Graphics Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (!remove_struct)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113104");
			const QLatin1String cm("Clean Structured Content Option");
#else
			const QString cv("113104");
			const QString cm("Clean Structured Content Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (!remove_descriptions)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113105");
			const QLatin1String cm("Clean Descriptors Option");
#else
			const QString cv("113105");
			const QString cm("Clean Descriptors Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv =
				retain_dates_times
				?
				QLatin1String("113106")
				:
				QLatin1String("113107");
			const QLatin1String cm =
				retain_dates_times
				?
				QLatin1String("Retain Longitudinal Temporal Information Full Dates Option")
				:
				QLatin1String("Retain Longitudinal Temporal Information Modified Dates Option");
#else
			const QString cv =
				retain_dates_times
				?
				QString("113106")
				:
				QString("113107");
			const QString cm =
				retain_dates_times
				?
				QString("Retain Longitudinal Temporal Information Full Dates Option")
				:
				QString("Retain Longitudinal Temporal Information Modified Dates Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (retain_patient_chars)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113108");
			const QLatin1String cm("Retain Patient Characteristics Option");
#else
			const QString cv("113108");
			const QString cm("Retain Patient Characteristics Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (retain_device_id)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113109");
			const QLatin1String cm("Retain Device Identity Option");
#else
			const QString cv("113109");
			const QString cm("Retain Device Identity Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (preserve_uids)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113110");
			const QLatin1String cm("Retain UIDs Option");
#else
			const QString cv("113110");
			const QString cm("Retain UIDs Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (!remove_private)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113111");
			const QLatin1String cm("Retain Safe Private Option");
#else
			const QString cv("113111");
			const QString cm("Retain Safe Private Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		//
		if (retain_institution_id)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
			const QLatin1String cv("113112");
			const QLatin1String cm("Retain Institution Identity Option");
#else
			const QString cv("113112");
			const QString cm("Retain Institution Identity Option");
#endif
			mdcm::Item item;
			item.SetVLToUndefined();
			mdcm::DataSet & nds = item.GetNestedDataSet();
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0100));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cv.latin1(), cv.size());
#else
				e.SetByteValue(cv.toLatin1().constData(), cv.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0102));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(dcm.latin1(), dcm.size());
#else
				e.SetByteValue(dcm.toLatin1().constData(), dcm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::SH);
				nds.Replace(e);
			}
			{
				mdcm::DataElement e(mdcm::Tag(0x0008, 0x0104));
#if QT_VERSION >= QT_VERSION_CHECK(5,10,0)
				e.SetByteValue(cm.latin1(), cm.size());
#else
				e.SetByteValue(cm.toLatin1().constData(), cm.toLatin1().size());
#endif
				if (!implicit) e.SetVR(mdcm::VR::LO);
				nds.Replace(e);
			}
			sq->AddItem(item);
		}
		mdcm::DataElement eDeidentificationMethodCodeSequence(mdcm::Tag(0x0012,0x0064));
		if (!implicit) eDeidentificationMethodCodeSequence.SetVR(mdcm::VR::SQ);
		eDeidentificationMethodCodeSequence.SetValue(*sq);
		ds.Replace(eDeidentificationMethodCodeSequence);
	}
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
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	writer.SetFileName(QDir::toNativeSeparators(outfilename).toUtf8().constData());
#else
	writer.SetFileName(QDir::toNativeSeparators(outfilename).toLocal8Bit().constData());
#endif
#else
	writer.SetFileName(outfilename.toLocal8Bit().constData());
#endif
	writer.SetFile(file);
	if (!writer.Write()) *ok = false;
}

AnonymazerWidget2::AnonymazerWidget2(float si)
{
	setupUi(this);
	const QSize s = QSize((int)(16*si),(int)(16*si));
	in_pushButton->setIconSize(s);
	out_pushButton->setIconSize(s);
	run_pushButton->setIconSize(s);
	run_pushButton->setEnabled(false);
	readSettings();
	init_profile();
#ifdef USE_WORKSTATION_MODE
	input_dir = QString("");
#else
#if (defined _WIN32)
	input_dir = 
		QString(".") +
		QString("/") +
		QString("DICOM");
#else
	input_dir = 
		QApplication::applicationDirPath() +
		QString("/") + QString("..") +
		QString("/") +
		QString("DICOM");
#endif
	QFileInfo fi(input_dir);
	if (fi.exists()) input_dir = fi.absoluteFilePath();
	else             input_dir = QString("");
	in_lineEdit->setText(QDir::toNativeSeparators(input_dir));
#endif
	help_widget = new HelpWidget();
	help_widget->hide();
	connect(out_pushButton,    SIGNAL(clicked()),    this,SLOT(set_output_dir()));
	connect(in_pushButton,     SIGNAL(clicked()),    this,SLOT(set_input_dir()));
	connect(help_pushButton,   SIGNAL(clicked()),    this,SLOT(show_help()));
	connect(run_pushButton,    SIGNAL(clicked()),    this,SLOT(run_()));
}

AnonymazerWidget2::~AnonymazerWidget2()
{
	delete help_widget;
	help_widget = NULL;
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
	output_dir = settings.value(QString("output_dir"), QString("")).toString();
	const int tmp01 = settings.value(QString("remove_private"),        0).toInt();
	const int tmp02 = settings.value(QString("remove_overlays"),       0).toInt();
	const int tmp03 = settings.value(QString("preserve_uids"),         0).toInt();
	const int tmp04 = settings.value(QString("retain_device_id"),      0).toInt();
	const int tmp05 = settings.value(QString("retain_dates_times"),    0).toInt();
	const int tmp06 = settings.value(QString("retain_patient_chars"),  0).toInt();
	const int tmp07 = settings.value(QString("retain_institution_id"), 0).toInt();
	const int tmp08 = settings.value(QString("remove_desc"),           0).toInt();
	const int tmp09 = settings.value(QString("remove_struct"),         0).toInt();
	const int tmp10 = settings.value(QString("random_names"),          1).toInt();
	const int tmp11 = settings.value(QString("rename_files"),          0).toInt();
	settings.endGroup();
	private_checkBox->setChecked(    (tmp01 == 1) ? true : false);
	graphics_checkBox->setChecked(   (tmp02 == 1) ? true : false);
	uids_checkBox->setChecked(       (tmp03 == 1) ? true : false);
	device_checkBox->setChecked(     (tmp04 == 1) ? true : false);
	dates_checkBox->setChecked(      (tmp05 == 1) ? true : false);
	chars_checkBox->setChecked(      (tmp06 == 1) ? true : false);
	institution_checkBox->setChecked((tmp07 == 1) ? true : false);
	desc_checkBox->setChecked(       (tmp08 == 1) ? true : false);
	struct_checkBox->setChecked(     (tmp09 == 1) ? true : false);
	random_checkBox->setChecked(     (tmp10 == 1) ? true : false);
	rename_checkBox->setChecked(     (tmp11 == 1) ? true : false);
}

void AnonymazerWidget2::writeSettings(QSettings & settings)
{
	const int tmp01 = (private_checkBox->isChecked())     ? 1 : 0;
	const int tmp02 = (graphics_checkBox->isChecked())    ? 1 : 0;
	const int tmp03 = (uids_checkBox->isChecked())        ? 1 : 0;
	const int tmp04 = (device_checkBox->isChecked())      ? 1 : 0;
	const int tmp05 = (chars_checkBox->isChecked())       ? 1 : 0;
	const int tmp06 = (institution_checkBox->isChecked()) ? 1 : 0;
	const int tmp07 = (dates_checkBox->isChecked())       ? 1 : 0;
	const int tmp08 = (desc_checkBox->isChecked())        ? 1 : 0;
	const int tmp09 = (struct_checkBox->isChecked())      ? 1 : 0;
	const int tmp10 = (random_checkBox->isChecked())      ? 1 : 0;
	const int tmp11 = (rename_checkBox->isChecked())      ? 1 : 0;
	settings.beginGroup(QString("AnonymazerWidget"));
	settings.setValue(QString("output_dir"),           QVariant(output_dir));
	settings.setValue(QString("remove_private"),       QVariant(tmp01));
	settings.setValue(QString("remove_overlays"),      QVariant(tmp02));
	settings.setValue(QString("preserve_uids"),        QVariant(tmp03));
	settings.setValue(QString("retain_device_id"),     QVariant(tmp04));
	settings.setValue(QString("retain_patient_chars"), QVariant(tmp05));
	settings.setValue(QString("retain_institution_id"),QVariant(tmp06));
	settings.setValue(QString("retain_dates_times"),   QVariant(tmp07));
	settings.setValue(QString("remove_desc"),          QVariant(tmp08));
	settings.setValue(QString("remove_struct"),        QVariant(tmp09));
	settings.setValue(QString("random_names"),         QVariant(tmp10));
	settings.setValue(QString("rename_files"),         QVariant(tmp11));
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
	const mdcm::DataSet & ds,
	QStringList & l,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::ConstIterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de = *it;
		mdcm::Tag t = de.GetTag();
		mdcm::VR vr = get_vr(ds, t, implicit, dicts);
		++it;
		if (vr == mdcm::VR::PN)
		{
			QString s;
			if (DicomUtils::get_string_value(ds, t, s)) l.push_back(s);
		}
		else
		{
			if (vr.Compatible(mdcm::VR::SQ))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					const mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						const mdcm::Item    & item   = sq->GetItem(i);
						const mdcm::DataSet & nested = item.GetNestedDataSet();
						find_pn_recurs__(nested, l, implicit, dicts);
					}
				}
			}
		}
	}
}

static void find_uids_recurs__(
	const mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	QStringList & l,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::ConstIterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de = *it;
		mdcm::Tag t = de.GetTag();
		++it;
		if (ts.find(t) != ts.end())
		{
			QString s;
			if (DicomUtils::get_string_value(ds, t, s)) l.push_back(s);
		}
		else
		{
			if (compatible_sq(const_cast<const mdcm::DataSet&>(ds), t, implicit, dicts))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					const mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						const mdcm::Item    & item   = sq->GetItem(i);
						const mdcm::DataSet & nested = item.GetNestedDataSet();
						find_uids_recurs__(nested, ts, l, implicit, dicts);
					}
				}
			}
		}
	}
}

static void find_ids_recurs__(
	const mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
	QStringList & l,
	QStringList & l2,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::DataSet::ConstIterator it = ds.Begin();
	for (; it != ds.End();)
	{
		const mdcm::DataElement & de = *it;
		mdcm::Tag t = de.GetTag();
		++it;
		if (ts.find(t) != ts.end())
		{
			QString s;
			if (DicomUtils::get_string_value(ds, t, s)) l.push_back(s);
			if (t == mdcm::Tag(0x0010,0x0020)) l2.push_back(s);
		}
		else
		{
			if (compatible_sq(const_cast<const mdcm::DataSet&>(ds), t, implicit, dicts))
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 0)
				{
					const mdcm::SequenceOfItems::SizeType n = sq->GetNumberOfItems();
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; ++i)
					{
						const mdcm::Item    & item   = sq->GetItem(i);
						const mdcm::DataSet & nested = item.GetNestedDataSet();
						find_ids_recurs__(nested, ts, l, l2, implicit, dicts);
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
	const std::set<mdcm::Tag> & id_tags,
	const mdcm::Dicts & dicts,
	QMap<QString, QString> & m0,
	QMap<QString, QString> & m1,
	QMap<QString, QString> & m2,
	QSet<QString> & pat_ids_set,   // to check for single patient
	const bool random_names,
	QProgressDialog * pd)
{
	QStringList uids;
	QStringList pids;
	QStringList ids;
	QStringList pat_ids_l;   // only to check for single patient
	for (int x = 0; x < l.size(); ++x)
	{
		QApplication::processEvents();
		pd->setValue(-1);
		if (pd->wasCanceled()) return;
		try
		{
			mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
			reader.SetFileName(QDir::toNativeSeparators(l.at(x)).toUtf8().constData());
#else
			reader.SetFileName(QDir::toNativeSeparators(l.at(x)).toLocal8Bit().constData());
#endif
#else
			reader.SetFileName(l.at(x).toLocal8Bit().constData());
#endif
			if (!reader.Read()) continue;
			const mdcm::File    & f  = reader.GetFile();
			const mdcm::DataSet & ds = f.GetDataSet();
			if (ds.IsEmpty()) continue;
			const mdcm::FileMetaInformation & h = f.GetHeader();
			const mdcm::TransferSyntax ts = h.GetDataSetTransferSyntax();
			const bool implicit = ts.IsImplicit();
			find_uids_recurs__(ds, uid_tags, uids, implicit, dicts);
			find_pn_recurs__(ds, pids, implicit, dicts);
			find_ids_recurs__(ds, id_tags, ids, pat_ids_l, implicit, dicts);
		}
		catch(mdcm::ParseException & pe)
		{
			std::cout << "mdcm::ParseException in build_maps:\n"
				<< pe.GetLastElement().GetTag() << std::endl;
		}
		catch(std::exception & ex)
		{
			std::cout << "Exception in build_maps:\n"
				<< ex.what() << std::endl;
		}
	}
// UIDs
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		QSet<QString> uset =
			(uids.empty())
			?
			QSet<QString>()
			:
			QSet<QString>(uids.begin(),uids.end());
#else
		QSet<QString> uset = uids.toSet();
#endif
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
	}
// PNs
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		QSet<QString> pset =
			(pids.empty())
			?
			QSet<QString>()
			:
			QSet<QString>(pids.begin(),pids.end());
#else
		QSet<QString> pset = pids.toSet();
#endif
		pids.clear();
		QSetIterator<QString> it1(pset);
		while (it1.hasNext())
		{
			QApplication::processEvents();
			pd->setValue(-1);
			if (pd->wasCanceled()) return;
			const QString s = it1.next();
			const QString v = generate_random_name(random_names);
			m1[s] = v;
			//std::cout << s.toStdString() << " --> " << v.toStdString() << std::endl;
		}
	}
// IDs
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		QSet<QString> iset =
			(ids.empty())
			?
			QSet<QString>()
			:
			QSet<QString>(ids.begin(),ids.end());
#else
		QSet<QString> iset = ids.toSet();
#endif
		ids.clear();
		QSetIterator<QString> it2(iset);
		while (it2.hasNext())
		{
			QApplication::processEvents();
			pd->setValue(-1);
			if (pd->wasCanceled()) return;
			const QString s = it2.next();
			const QString v = DicomUtils::generate_id();
			m2[s] = v;
			//std::cout << s.toStdString() << " --> " << v.toStdString() << std::endl;
		}
	}
//
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		pat_ids_set =
			(pat_ids_l.empty())
			?
			QSet<QString>()
			:
			QSet<QString>(pat_ids_l.begin(),pat_ids_l.end());
#else
		pat_ids_set = pat_ids_l.toSet();
#endif
	}
}

static unsigned int count_files = 0;
static unsigned int count_dirs = 0;
void AnonymazerWidget2::process_directory(
	const QString & p,
	const QString & outp,
	const QMap<QString,QString> & uid_m,
	const QMap<QString,QString> & pn_m,
	const QMap<QString,QString> & id_m,
	unsigned int * count_overlay_in_data,
	unsigned int * count_errors,
	const mdcm::Dicts & dicts,
	const int y_off,
	const int m_off,
	const int d_off,
	const int s_off,
	const bool one_patient,
	const QString & single_name,
	const QString & single_id,
	const bool preserve_uids,
	const bool remove_private,
	const bool remove_graphics,
	const bool remove_descriptions,
	const bool remove_struct,
	const bool retain_device_id,
	const bool retain_patient_chars,
	const bool retain_institution_id,
	const bool retain_dates_times,
	const bool confirm_clean_pixel,
	const bool confirm_no_recognizable,
	const bool rename_files,
	QProgressDialog * pd)
{
	QDir dir(p);
	QStringList dlist = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	QStringList flist = dir.entryList(QDir::Files);
	//
	{
		count_files = 0;
		QStringList filenames;
		for (int x = 0; x < flist.size(); ++x)
		{
			pd->setValue(-1);
			QApplication::processEvents();
			if (pd->wasCanceled()) return;
			const QString tmp0 = dir.absolutePath() + QString("/") + flist.at(x);
			filenames.push_back(tmp0);
		}
		for (int x = 0; x < filenames.size(); ++x)
		{
			++count_files;
			pd->setValue(-1);
			QApplication::processEvents();
			if (pd->wasCanceled()) return;
			QString out_filename;
			if (rename_files)
			{
				QString tmp000;
				if (count_files <= 9999)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp000 = QString::asprintf("%04d", count_files);
#else
					tmp000.sprintf("%04d", count_files);
#endif
				}
				else if (count_files <= 99999999)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp000 = QString::asprintf("%08d", count_files);
#else
					tmp000.sprintf("%08d", count_files);
#endif
				}
				else
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp000 = QString::asprintf("%010d", count_files);
#else
					tmp000.sprintf("%010d", count_files);
#endif
				}
				out_filename = tmp000 + QString(".dcm");
			}
			else
			{
				QFileInfo fi(filenames.at(x));
				out_filename = fi.fileName();
			}
			const QString out_file = 
				outp +
				QString("/") +
				out_filename;
			bool ok_ = false;
			bool overlay_in_data = false;
			anonymize_file__(
				&ok_,
				&overlay_in_data,
				filenames.at(x),
				out_file,
				dicts,
				pn_tags,
				uid_tags,
				id_tags,
				empty_tags,
				remove_tags,
				zero_seq_tags,
				dev_remove_tags,
				dev_empty_tags,
				dev_replace_tags,
				patient_tags,
				inst_remove_tags,
				inst_empty_tags,
				inst_replace_tags,
				time_tags,
				descr_remove_tags,
				descr_empty_tags,
				descr_replace_tags,
				struct_zero_tags,
				uid_m,
				pn_m,
				id_m,
				preserve_uids,
				remove_private,
				remove_graphics,
				remove_descriptions,
				remove_struct,
				retain_dates_times,
				retain_device_id,
				retain_patient_chars,
				retain_institution_id,
				confirm_clean_pixel,
				confirm_no_recognizable,
				y_off,
				m_off,
				d_off,
				s_off,
				one_patient,
				single_name,
				single_id);
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
	}
	//
	{
		for (int j = 0; j < dlist.size(); ++j)
		{
			++count_dirs;
			QApplication::processEvents();
			QDir d;
			if (rename_files)
			{
				QString tmp000;
				if (count_dirs <= 99999999)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp000 = QString::asprintf("%08d", count_dirs);
#else
					tmp000.sprintf("%08d", count_dirs);
#endif
				}
				else
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp000 = QString::asprintf("%010d", count_dirs);
#else
					tmp000.sprintf("%010d", count_dirs);
#endif
				}
				d = QDir(outp + QString("/") + tmp000);
			}
			else
			{
				d = QDir(outp + QString("/") + dlist.at(j));
			}
			if (!d.exists()) d.mkpath(d.absolutePath());
#if 0
			const std::string tmp_in =
				(dir.absolutePath() + QString("/") + dlist.at(j)).toStdString();
			const std::string tmp_out = d.absolutePath().toStdString();
			std::cout << "in=" << tmp_in << " out=" << tmp_out << std::endl;
#endif
			process_directory(
				dir.absolutePath() + QString("/") + dlist.at(j),
				d.absolutePath(),
				uid_m,
				pn_m,
				id_m,
				count_overlay_in_data,
				count_errors,
				dicts,
				y_off, m_off, d_off, s_off,
				one_patient,
				single_name,
				single_id,
				preserve_uids,
				remove_private,
				remove_graphics,
				remove_descriptions,
				remove_struct,
				retain_device_id,
				retain_patient_chars,
				retain_institution_id,
				retain_dates_times,
				confirm_clean_pixel,
				confirm_no_recognizable,
				rename_files,
				pd);
		}
	}
}

void AnonymazerWidget2::run_()
{
	count_dirs = 0;
	const QString in_path = in_lineEdit->text();
	const QString out_path = dir_lineEdit->text();
	if (in_path.isEmpty() || out_path.isEmpty())
	{
		run_pushButton->setEnabled(false);
		QMessageBox::information(
			NULL,
			QString("De-identify"),
			QString("Select input and output directories"));
		return;
	}
	//
	if (out_path == in_path)
	{
		QMessageBox::information(
			NULL,
			QString("De-identify"),
			QString("Error: input and output are the same directory"));
		return;
	}
	//
	bool one_patient = false;
	const bool random_names            = random_checkBox->isChecked();
	const bool preserve_uids           = uids_checkBox->isChecked();
	const bool remove_private          = private_checkBox->isChecked();
	const bool remove_graphics         = graphics_checkBox->isChecked();
	const bool remove_descriptions     = desc_checkBox->isChecked();
	const bool remove_struct           = struct_checkBox->isChecked();
	const bool retain_device_id        = device_checkBox->isChecked();
	const bool retain_patient_chars    = chars_checkBox->isChecked();
	const bool retain_institution_id   = institution_checkBox->isChecked();
	const bool retain_dates_times      = dates_checkBox->isChecked();
	const bool confirm_clean_pixel     = confirm1_checkBox->isChecked();
	const bool confirm_no_recognizable = confirm2_checkBox->isChecked();
	const bool rename_files            = rename_checkBox->isChecked();
	const QString single_name = name_lineEdit->text().trimmed();
	const QString single_id   = id_lineEdit->text().trimmed();
	if (!single_name.isEmpty() || !single_id.isEmpty())
	{
		one_patient = true;
	}
	//
	const unsigned long long seed =
		std::chrono::high_resolution_clock::now()
			.time_since_epoch()
			.count();
	std::mt19937_64 mtrand(seed);
	const int y_off = mtrand() % 2 + 1;
	const int m_off = mtrand() % 3 + 1;
	const int d_off = mtrand() % 4 + 1;
	const int s_off = mtrand() % 1999 + 1;
	//
	const mdcm::Global & g     = mdcm::GlobalInstance;
	const mdcm::Dicts  & dicts = g.GetDicts();
	//
	const QString root_uid = QString::fromLatin1(ALIZAMS_ROOT_UID);
	mdcm::UIDGenerator::SetRoot(root_uid.toLatin1().constData());
	QMap<QString, QString> uid_m;
	QMap<QString, QString> pn_m;
	QMap<QString, QString> id_m;
	QSet<QString> pat_ids_set; // to check for one patient
	QProgressDialog * pd =
		new QProgressDialog(QString("De-identifying"),QString("Cancel"),0,0);
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
		build_maps(
			filenames,
			uid_tags, id_tags,
			dicts, uid_m, pn_m, id_m,
			pat_ids_set,
			random_names,
			pd);
		filenames.clear();
	}
	if (one_patient)
	{
		if (pat_ids_set.size() > 1)
		{
			pd->close();
			delete pd;
			QMessageBox::information(
				NULL,
				QString("De-identify"),
				QString(
					"Error: can not replace Patient Name/ID -\n"
					"multiple patients IDs found in the folder.\n"
					"Use a folder with a single patient or let\n"
					"auto-generate names and IDs."));
			return;
		}
	}
	unsigned int count_overlay_in_data = 0;
	unsigned int count_errors = 0;
	try
	{
		process_directory(
			in_path,
			out_path,
			uid_m,
			pn_m,
			id_m,
			&count_overlay_in_data,
			&count_errors,
			dicts,
			y_off, m_off, d_off, s_off,
			one_patient,
			single_name,
			single_id,
			preserve_uids,
			remove_private,
			remove_graphics,
			remove_descriptions,
			remove_struct,
			retain_device_id,
			retain_patient_chars,
			retain_institution_id,
			retain_dates_times,
			confirm_clean_pixel,
			confirm_no_recognizable,
			rename_files,
			pd);
	}
	catch(mdcm::ParseException & pe)
	{
		std::cout
			<< "mdcm::ParseException in AnonymazerWidget2::run_\n"
			<< pe.GetLastElement().GetTag() << std::endl;
	}
	catch(std::exception & ex)
	{
		std::cout << "Exception in AnonymazerWidget2::run_\n"
			<< ex.what() << std::endl;
	}
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
		mbox.setWindowTitle(QString("De-identify"));
		mbox.setIcon(QMessageBox::Information);
		mbox.setText(message);
		mbox.exec();
	}
	dir_lineEdit->setText(QString(""));
	in_lineEdit->setText(QString(""));
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
		dir_lineEdit->setText(QString(""));
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
		in_lineEdit->setText(QString(""));
		return;
	}
	input_dir = dirname;
	in_lineEdit->setText(QDir::toNativeSeparators(input_dir));
}

void AnonymazerWidget2::show_help()
{
	if (help_widget->isMinimized()) help_widget->showNormal();
	else help_widget->show();
	help_widget->raise();
	help_widget->activateWindow();
}

// clang-format off
void AnonymazerWidget2::init_profile()
{
/*
 *
 * Table E.1-1. Application Level Confidentiality Profile Attributes
 * 2021b
 *
 */
	empty_tags        .insert(mdcm::Tag(0x0008,0x0050));// Accession Number
	descr_remove_tags .insert(mdcm::Tag(0x0018,0x4000));// Acquisition Comments
	struct_zero_tags  .insert(mdcm::Tag(0x0040,0x0555));// Acquisition Context Sequence
	time_tags         .insert(mdcm::Tag(0x0008,0x0022));// Acquisition Date
	time_tags         .insert(mdcm::Tag(0x0008,0x002a));// Acquisition DateTime
	descr_remove_tags .insert(mdcm::Tag(0x0018,0x1400));// Acquisition Device Processing Description
	descr_replace_tags.insert(mdcm::Tag(0x0018,0x11bb));// Acquisition Field Of View Label
	descr_remove_tags .insert(mdcm::Tag(0x0018,0x9424));// Acquisition Protocol Description
	time_tags         .insert(mdcm::Tag(0x0008,0x0032));// Acquisition Time
	remove_tags       .insert(mdcm::Tag(0x0040,0x4035));// ActualHuman Performers Sequence
	descr_remove_tags .insert(mdcm::Tag(0x0010,0x21b0));// Additional Patient History
	remove_tags       .insert(mdcm::Tag(0x0040,0xa353));// Address (Trial)	
	remove_tags       .insert(mdcm::Tag(0x0038,0x0010));// Admission ID
	time_tags         .insert(mdcm::Tag(0x0038,0x0020));// Admitting Date
	descr_remove_tags .insert(mdcm::Tag(0x0008,0x1084));// Admitting Diagnoses Code Sequence
	descr_remove_tags .insert(mdcm::Tag(0x0008,0x1080));// Admitting Diagnoses Description
	time_tags         .insert(mdcm::Tag(0x0038,0x0021));// Admitting Time
	//                                 (0x0000,0x1000)  // Affected SOP Instance UID
	patient_tags      .insert(mdcm::Tag(0x0010,0x2110));// Allergies
	remove_tags       .insert(mdcm::Tag(0x4000,0x0010));// Arbitrary
	remove_tags       .insert(mdcm::Tag(0x0040,0xa078));// Author Observer Sequence
	empty_tags        .insert(mdcm::Tag(0x2200,0x0005));// Barcode Value
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x00c3));// Beam Description
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x00dd));// Bolus Description
	remove_tags       .insert(mdcm::Tag(0x0010,0x1081));// Branch of Service
	remove_tags       .insert(mdcm::Tag(0x0016,0x004d));// Camera Owner Name
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x1007));// Cassette ID
	inst_empty_tags   .insert(mdcm::Tag(0x0012,0x0060));// Clinical Trial Coordinating Center Name
	remove_tags       .insert(mdcm::Tag(0x0012,0x0082));// Clinical Trial Protocol Ethics Committee Approval Number
	inst_replace_tags .insert(mdcm::Tag(0x0012,0x0081));// Clinical Trial Protocol Ethics Committee Name
	id_tags           .insert(mdcm::Tag(0x0012,0x0020));// Clinical Trial Protocol ID
	remove_tags       .insert(mdcm::Tag(0x0012,0x0021));// Clinical Trial Protocol Name
	descr_remove_tags .insert(mdcm::Tag(0x0012,0x0072));// Clinical Trial Series Description
	remove_tags       .insert(mdcm::Tag(0x0012,0x0071));// Clinical Trial Series ID
	inst_empty_tags   .insert(mdcm::Tag(0x0012,0x0030));// Clinical Trial Site ID
	inst_empty_tags   .insert(mdcm::Tag(0x0012,0x0031));// Clinical Trial Site Name
	pn_tags           .insert(mdcm::Tag(0x0012,0x0010));// Clinical Trial Sponsor Name
	id_tags           .insert(mdcm::Tag(0x0012,0x0040));// Clinical Trial Subject ID
	id_tags           .insert(mdcm::Tag(0x0012,0x0042));// Clinical Trial Subject Reading ID
	descr_remove_tags .insert(mdcm::Tag(0x0012,0x0051));// Clinical Trial Time Point Description
	empty_tags        .insert(mdcm::Tag(0x0012,0x0050));// Clinical Trial Time Point ID
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x0310));// Comments on Radiation Dose
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x0280));// Comments on the Performed Procedure Step
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x02eb));// Compensator Description
	uid_tags          .insert(mdcm::Tag(0x0020,0x9161));// Concatenation UID
	descr_empty_tags  .insert(mdcm::Tag(0x3010,0x000f));// Conceptual Volume Combination Description
	descr_empty_tags  .insert(mdcm::Tag(0x3010,0x0017));// Conceptual Volume Description
	uid_tags          .insert(mdcm::Tag(0x3010,0x0006));// Conceptual Volume UID
	remove_tags       .insert(mdcm::Tag(0x0040,0x3001));// Confidentiality Constrainton Patient Data Description
	uid_tags          .insert(mdcm::Tag(0x3010,0x0013));// Constituent Conceptual Volume UID
	pn_tags           .insert(mdcm::Tag(0x0008,0x009c));// Consulting Physician's Name
	remove_tags       .insert(mdcm::Tag(0x0008,0x009d));// Consulting Physician Identification Sequence
	remove_tags       .insert(mdcm::Tag(0x0050,0x001b));// Container Component ID
	descr_empty_tags  .insert(mdcm::Tag(0x0040,0x051a));// Container Description
	id_tags           .insert(mdcm::Tag(0x0040,0x0512));// Container Identifier
	remove_tags       .insert(mdcm::Tag(0x0070,0x0086));// Content Creator's Identification Code Sequence
	pn_tags           .insert(mdcm::Tag(0x0070,0x0084));// Content Creator's Name
	time_tags         .insert(mdcm::Tag(0x0008,0x0023));// Content Date
	struct_zero_tags  .insert(mdcm::Tag(0x0040,0xa730));// Content Sequence
	time_tags         .insert(mdcm::Tag(0x0008,0x0033));// Content Time
	descr_empty_tags  .insert(mdcm::Tag(0x0018,0x0010));// Contrast / Bolus Agent
	descr_remove_tags .insert(mdcm::Tag(0x0018,0xa003));// Contribution Description
	remove_tags       .insert(mdcm::Tag(0x0010,0x2150));// Country of Residence
	remove_tags       .insert(mdcm::Tag(0x0040,0xa307));// Current Observer (Trial)
	remove_tags       .insert(mdcm::Tag(0x0038,0x0300));// Current Patient Location
	//                                 (0x50GG,0xEEEE)  // Curve Data
	time_tags         .insert(mdcm::Tag(0x0008,0x0025));// Curve Date
	time_tags         .insert(mdcm::Tag(0x0008,0x0035));// Curve Time
	remove_tags       .insert(mdcm::Tag(0x0040,0xa07c));// Custodial Organization Sequence
	remove_tags       .insert(mdcm::Tag(0xfffc,0xfffc));// Data Set Trailing Padding
	descr_remove_tags .insert(mdcm::Tag(0x0018,0x937f));// Decomposition Description
	descr_remove_tags .insert(mdcm::Tag(0x0008,0x2111));// Derivation Description
	dev_replace_tags  .insert(mdcm::Tag(0x0018,0x700a));// Detector ID
	empty_tags        .insert(mdcm::Tag(0x3010,0x001b));// Device Alternate Identifier
	dev_remove_tags   .insert(mdcm::Tag(0x0050,0x0020));// Device Description
	dev_replace_tags  .insert(mdcm::Tag(0x3010,0x002d));// Device Label
	dev_replace_tags  .insert(mdcm::Tag(0x0018,0x1000));// Device Serial Number
	descr_remove_tags .insert(mdcm::Tag(0x0016,0x004b));// Device Setting Description
	uid_tags          .insert(mdcm::Tag(0x0018,0x1002));// Device UID
	remove_tags       .insert(mdcm::Tag(0xfffa,0xfffa));// Digital Signatures Sequence
	uid_tags          .insert(mdcm::Tag(0x0400,0x0100));// Digital Signature UID
	uid_tags          .insert(mdcm::Tag(0x0020,0x9164));// Dimension Organization UID
	descr_remove_tags .insert(mdcm::Tag(0x0038,0x0040));// Discharge Diagnosis Description
	remove_tags       .insert(mdcm::Tag(0x4008,0x011a));// Distribution Address
	remove_tags       .insert(mdcm::Tag(0x4008,0x0119));// Distribution Name
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x0016));// Dose Reference Description
	uid_tags          .insert(mdcm::Tag(0x300a,0x0013));// Dose Reference UID
	uid_tags          .insert(mdcm::Tag(0x3010,0x006e));// Dosimetric Objective UID
	time_tags         .insert(mdcm::Tag(0x0018,0x9517));// End Acquisition DateTime
	descr_remove_tags .insert(mdcm::Tag(0x3010,0x0037));// Entity Description
	descr_replace_tags.insert(mdcm::Tag(0x3010,0x0035));// Entity Label
	descr_replace_tags.insert(mdcm::Tag(0x3010,0x0038));// Entity Long Label
	descr_remove_tags .insert(mdcm::Tag(0x3010,0x0036));// Entity Name
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x0676));// Equipment Frame of Reference Description
	patient_tags      .insert(mdcm::Tag(0x0010,0x2160));// Ethnic Group
	time_tags         .insert(mdcm::Tag(0x0040,0x4011));// Expected Completion DateTime
	remove_tags       .insert(mdcm::Tag(0x0008,0x0058));// Failed SOP Instance UID List FIXME
	uid_tags          .insert(mdcm::Tag(0x0070,0x031a));// Fiducial UID
	empty_tags        .insert(mdcm::Tag(0x0040,0x2017));// Filler Order Number / Imaging Service Request
	time_tags         .insert(mdcm::Tag(0x3008,0x0054));// First Treatment Date
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x0196));// Fixation Device Description
	id_tags           .insert(mdcm::Tag(0x0034,0x0002));// Flow Identifier
	//                                 (0x0034,0x0001)  // Flow Identifier Sequence
	descr_empty_tags  .insert(mdcm::Tag(0x3010,0x007f));// Fractionation Notes
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x0072));// Fraction Group Description
	descr_remove_tags .insert(mdcm::Tag(0x0020,0x9158));// Frame Comments
	uid_tags          .insert(mdcm::Tag(0x0020,0x0052));// Frame of Reference UID
	time_tags         .insert(mdcm::Tag(0x0034,0x0007));// Frame Origin Timestamp
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x1008));// Gantry ID
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x1005));// Generator ID
	remove_tags       .insert(mdcm::Tag(0x0016,0x0076));// GPS Altitude
	remove_tags       .insert(mdcm::Tag(0x0016,0x0075));// GPS Altitude Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x008c));// GPS Area Information
	time_tags         .insert(mdcm::Tag(0x0016,0x008d));// GPS Date Stamp
	remove_tags       .insert(mdcm::Tag(0x0016,0x0088));// GPS Dest Bearing
	remove_tags       .insert(mdcm::Tag(0x0016,0x0087));// GPS Dest Bearing Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x008a));// GPS Dest Distance
	remove_tags       .insert(mdcm::Tag(0x0016,0x0089));// GPS Dest Distance Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x0084));// GPS Dest Latitude
	remove_tags       .insert(mdcm::Tag(0x0016,0x0083));// GPS Dest Latitude Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x0086));// GPS Dest Longitude
	remove_tags       .insert(mdcm::Tag(0x0016,0x0085));// GPS Dest Longitude Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x008e));// GPS Differential
	remove_tags       .insert(mdcm::Tag(0x0016,0x007b));// GPS DOP
	remove_tags       .insert(mdcm::Tag(0x0016,0x0081));// GPS Img Direction
	remove_tags       .insert(mdcm::Tag(0x0016,0x0080));// GPS Img Direction Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x0072));// GPS Latitude
	remove_tags       .insert(mdcm::Tag(0x0016,0x0071));// GPS Latitude Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x0074));// GPS Longitude
	remove_tags       .insert(mdcm::Tag(0x0016,0x0073));// GPS Longitude Ref
	remove_tags       .insert(mdcm::Tag(0x0016,0x0082));// GPS Map Datum
	remove_tags       .insert(mdcm::Tag(0x0016,0x007a));// GPS Measure Mode
	remove_tags       .insert(mdcm::Tag(0x0016,0x008b));// GPS Processing Method
	remove_tags       .insert(mdcm::Tag(0x0016,0x0078));// GPS Satellites
	remove_tags       .insert(mdcm::Tag(0x0016,0x007d));// GPS Speed
	remove_tags       .insert(mdcm::Tag(0x0016,0x007c));// GPS SpeedRef
	remove_tags       .insert(mdcm::Tag(0x0016,0x0079));// GPS Status
	remove_tags       .insert(mdcm::Tag(0x0016,0x0077));// GPS TimeStamp
	remove_tags       .insert(mdcm::Tag(0x0016,0x007f));// GPS Track
	remove_tags       .insert(mdcm::Tag(0x0016,0x007e));// GPS TrackRef
	remove_tags       .insert(mdcm::Tag(0x0016,0x0070));// GPS Version ID
	//                                 (0x0070,0x0001)  // Graphic Annotation Sequence
	remove_tags       .insert(mdcm::Tag(0x0040,0x4037));// Human Performer's Name
	remove_tags       .insert(mdcm::Tag(0x0040,0x4036));// Human Performer's Organization
	remove_tags       .insert(mdcm::Tag(0x0088,0x0200));// Icon Image Sequence
	descr_remove_tags .insert(mdcm::Tag(0x0008,0x4000));// Identifying Comments
	descr_remove_tags .insert(mdcm::Tag(0x0020,0x4000));// Image Comments
	remove_tags       .insert(mdcm::Tag(0x0028,0x4000));// Image Presentation Comments
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x2400));// Imaging Service Request Comments
	time_tags         .insert(mdcm::Tag(0x003a,0x0314));// Impedance Measurement DateTime
	remove_tags       .insert(mdcm::Tag(0x4008,0x0300));// Impressions
	time_tags         .insert(mdcm::Tag(0x0008,0x0015));// Instance Coercion DateTime
	uid_tags          .insert(mdcm::Tag(0x0008,0x0014));// Instance Creator UID
	remove_tags       .insert(mdcm::Tag(0x0400,0x0600));// Instance Origin Status
	inst_remove_tags  .insert(mdcm::Tag(0x0008,0x0081));// Institution Address
	inst_remove_tags  .insert(mdcm::Tag(0x0008,0x1040));// Institution Department Name
	inst_remove_tags  .insert(mdcm::Tag(0x0008,0x1041));// Institution Department Type Code Sequence
	inst_remove_tags  .insert(mdcm::Tag(0x0008,0x0082));// Institution Code Sequence
	inst_remove_tags  .insert(mdcm::Tag(0x0008,0x0080));// Institution Name
	remove_tags       .insert(mdcm::Tag(0x0010,0x1050));// Insurance Plan Identification
	time_tags         .insert(mdcm::Tag(0x3010,0x004d));// Intended Phase End Date
	time_tags         .insert(mdcm::Tag(0x3010,0x004c));// Intended Phase Start Date
	remove_tags       .insert(mdcm::Tag(0x0040,0x1011));// Intended Recipients of Results Identification Sequence
	time_tags         .insert(mdcm::Tag(0x300a,0x0741));// Interlock DateTime
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x0741));// Interlock Description
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x0783));// Interlock Origin Description
	remove_tags       .insert(mdcm::Tag(0x4008,0x0111));// Interpretation Approver Sequence
	remove_tags       .insert(mdcm::Tag(0x4008,0x010c));// Interpretation Author
	descr_remove_tags .insert(mdcm::Tag(0x4008,0x0115));// Interpretation Diagnosis Description
	remove_tags       .insert(mdcm::Tag(0x4008,0x0202));// Interpretation ID Issuer
	remove_tags       .insert(mdcm::Tag(0x4008,0x0102));// Interpretation Recorder
	descr_remove_tags .insert(mdcm::Tag(0x4008,0x010b));// Interpretation Text
	remove_tags       .insert(mdcm::Tag(0x4008,0x010a));// Interpretation Transcriber
	uid_tags          .insert(mdcm::Tag(0x0008,0x3010));// Irradiation Event UID
	remove_tags       .insert(mdcm::Tag(0x0038,0x0011));// Issuer of AdmissionID
	remove_tags       .insert(mdcm::Tag(0x0038,0x0014));// Issuer of Admission ID Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x0021));// Issuer of Patient ID
	remove_tags       .insert(mdcm::Tag(0x0038,0x0061));// Issuer of Service Episode ID
	remove_tags       .insert(mdcm::Tag(0x0038,0x0064));// Issuer of Service Episode ID Sequence
	zero_seq_tags     .insert(mdcm::Tag(0x0040,0x0513));// Issuer of the Container Identifier Sequence
	zero_seq_tags     .insert(mdcm::Tag(0x0040,0x0562));// Issuer of the Specimen Identifier Sequence
	descr_empty_tags  .insert(mdcm::Tag(0x2200,0x0002));// Label Text
	uid_tags          .insert(mdcm::Tag(0x0028,0x1214));// Large Palette Color Lookup Table UID
	time_tags         .insert(mdcm::Tag(0x0010,0x21d0));// Last Menstrual Date
	dev_remove_tags   .insert(mdcm::Tag(0x0016,0x004f));// Lens Make
	dev_remove_tags   .insert(mdcm::Tag(0x0016,0x0050));// Lens Model
	dev_remove_tags   .insert(mdcm::Tag(0x0016,0x0051));// Lens Serial Number
	dev_remove_tags   .insert(mdcm::Tag(0x0016,0x004e));// Lens Specification
	descr_remove_tags .insert(mdcm::Tag(0x0050,0x0021));// Long Device Description
	remove_tags       .insert(mdcm::Tag(0x0400,0x0404));// MAC
	descr_remove_tags .insert(mdcm::Tag(0x0016,0x002b));// Maker Note
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x100b));// Manufacturer's Device Class UID
	dev_empty_tags    .insert(mdcm::Tag(0x3010,0x0043));// Manufacturer's Device Identifier
	//                                 (0x0002,0x0003)  // Media Storage SOP Instance UID
	remove_tags       .insert(mdcm::Tag(0x0010,0x2000));// Medical Alerts
	remove_tags       .insert(mdcm::Tag(0x0010,0x1090));// Medical Record Locator
	remove_tags       .insert(mdcm::Tag(0x0010,0x1080));// Military Rank
	remove_tags       .insert(mdcm::Tag(0x0400,0x0550));// Modified Attributes Sequence
	remove_tags       .insert(mdcm::Tag(0x0020,0x3406));// Modified Image Description
	remove_tags       .insert(mdcm::Tag(0x0020,0x3401));// Modifying Device ID
	time_tags         .insert(mdcm::Tag(0x3008,0x0056));// Most Recent Treatment Date
	descr_remove_tags .insert(mdcm::Tag(0x0018,0x937b));// Multi-energy Acquisition Description
	uid_tags          .insert(mdcm::Tag(0x003a,0x0310));// Multiplex Group UID
	remove_tags       .insert(mdcm::Tag(0x0008,0x1060));// Name of Physician(s) Reading Study
	remove_tags       .insert(mdcm::Tag(0x0040,0x1010));// Names of Intended Recipients of Results
	remove_tags       .insert(mdcm::Tag(0x0400,0x0551));// Nonconforming Modified Attributes Sequence
	remove_tags       .insert(mdcm::Tag(0x0400,0x0552));// Nonconforming Data Element Value
	time_tags         .insert(mdcm::Tag(0x0040,0xa192));// Observation Date (Trial)
	uid_tags          .insert(mdcm::Tag(0x0040,0xa402));// Observation Subject UID (Trial)
	time_tags         .insert(mdcm::Tag(0x0040,0xa193));// Observation Time (Trial)
	uid_tags          .insert(mdcm::Tag(0x0040,0xa171));// Observation UID
	descr_remove_tags .insert(mdcm::Tag(0x0010,0x2180));// Occupation
	remove_tags       .insert(mdcm::Tag(0x0008,0x1072));// Operator Identification Sequence
	pn_tags           .insert(mdcm::Tag(0x0008,0x1070));// Operators' Name
	remove_tags       .insert(mdcm::Tag(0x0040,0x2010));// Order Callback Phone Number
	remove_tags       .insert(mdcm::Tag(0x0040,0x2011));// Order Callback Telecom Information
	remove_tags       .insert(mdcm::Tag(0x0040,0x2008));// Order Entered By
	remove_tags       .insert(mdcm::Tag(0x0040,0x2009));// Order Enterer's Location
	remove_tags       .insert(mdcm::Tag(0x0400,0x0561));// Original Attributes Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x1000));// Other Patient IDs
	remove_tags       .insert(mdcm::Tag(0x0010,0x1002));// Other Patient IDs Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x1001));// Other Patient Names
	//                                 (0x60xx,0x4000)  // Overlay Comments
	//                                 (0x60xx,0x3000)  // Overlay Data
	time_tags         .insert(mdcm::Tag(0x0008,0x0024));// Overlay Date
	time_tags         .insert(mdcm::Tag(0x0008,0x0034));// Overlay Time
	time_tags         .insert(mdcm::Tag(0x300a,0x0760));// Overlay DateTime
	uid_tags          .insert(mdcm::Tag(0x0028,0x1199));// Palette Color Lookup Table UID
	remove_tags       .insert(mdcm::Tag(0x0040,0xa07a));// Participant Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x1040));// Patient's Address
	patient_tags      .insert(mdcm::Tag(0x0010,0x1010));// Patient's Age
	empty_tags        .insert(mdcm::Tag(0x0010,0x0030));// Patient's Birth Date
	remove_tags       .insert(mdcm::Tag(0x0010,0x1005));// Patient's Birth Name
	remove_tags       .insert(mdcm::Tag(0x0010,0x0032));// Patient's Birth Time
	remove_tags       .insert(mdcm::Tag(0x0038,0x0400));// Patient's Institution Residence
	remove_tags       .insert(mdcm::Tag(0x0010,0x0050));// Patient's Insurance Plan Code Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x1060));// Patient's Mother's Birth Name
	pn_tags           .insert(mdcm::Tag(0x0010,0x0010));// Patient's Name
	remove_tags       .insert(mdcm::Tag(0x0010,0x0101));// Patient's Primary Language Code Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x0102));// Patient's Primary Language Modifier Code Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x21f0));// Patient's Religious Preference
	patient_tags      .insert(mdcm::Tag(0x0010,0x0040));// Patient's Sex
	patient_tags      .insert(mdcm::Tag(0x0010,0x2203));// Patient's Sex Neutered
	patient_tags      .insert(mdcm::Tag(0x0010,0x1020));// Patient's Size
	remove_tags       .insert(mdcm::Tag(0x0010,0x2155));// Patient's Telecom Information
	remove_tags       .insert(mdcm::Tag(0x0010,0x2154));// Patient's Telephone Numbers
	patient_tags      .insert(mdcm::Tag(0x0010,0x1030));// Patient's Weight
	descr_remove_tags .insert(mdcm::Tag(0x0010,0x4000));// Patient Comments
	id_tags           .insert(mdcm::Tag(0x0010,0x0020));// Patient ID
	uid_tags          .insert(mdcm::Tag(0x300a,0x0650));// Patient Setup UID
	descr_remove_tags .insert(mdcm::Tag(0x0038,0x0500));// Patient State
	remove_tags       .insert(mdcm::Tag(0x0040,0x1004));// Patient Transport Arrangements
	remove_tags       .insert(mdcm::Tag(0x0040,0x0243));// Performed Location
	remove_tags       .insert(mdcm::Tag(0x0040,0x0254));// Performed Procedure Step Description
	time_tags         .insert(mdcm::Tag(0x0040,0x0250));// Performed Procedure Step End Date
	time_tags         .insert(mdcm::Tag(0x0040,0x4051));// Performed Procedure Step End DateTime
	time_tags         .insert(mdcm::Tag(0x0040,0x0251));// Performed Procedure Step End Time
	remove_tags       .insert(mdcm::Tag(0x0040,0x0253));// Performed Procedure Step ID
	time_tags         .insert(mdcm::Tag(0x0040,0x0244));// Performed Procedure Step Start Date
	time_tags         .insert(mdcm::Tag(0x0040,0x4050));// Performed Procedure Step Start DateTime
	time_tags         .insert(mdcm::Tag(0x0040,0x0245));// Performed Procedure Step Start Time
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x0241));// Performed Station AE Title
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x4030));// Performed Station Geographic Location Code Sequence
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x0242));// Performed Station Name
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x4028));// Performed Station Name Code Sequence
	remove_tags       .insert(mdcm::Tag(0x0008,0x1050));// Performing Physician's Name
	remove_tags       .insert(mdcm::Tag(0x0008,0x1052));// Performing Physician Identification Sequence
	remove_tags       .insert(mdcm::Tag(0x0040,0x1102));// Person's Address
	remove_tags       .insert(mdcm::Tag(0x0040,0x1104));// Person's Telecom Information
	remove_tags       .insert(mdcm::Tag(0x0040,0x1103));// Person's Telephone Numbers
	//                                 (0x0040,0x1101)  // Person Identification Code Sequence
	pn_tags           .insert(mdcm::Tag(0x0040,0xa123));// Person Name
	remove_tags       .insert(mdcm::Tag(0x0008,0x1048));// Physician(s) of Record
	remove_tags       .insert(mdcm::Tag(0x0008,0x1049));// Physician(s) of Record Identification Sequence
	remove_tags       .insert(mdcm::Tag(0x0008,0x1062));// Physician(s) Reading Study Identification Sequence
	remove_tags       .insert(mdcm::Tag(0x4008,0x0114));// Physician Approving Interpretation
	empty_tags        .insert(mdcm::Tag(0x0040,0x2016));// Placer Order Number / Imaging Service Request
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x1004));// Plate ID
	patient_tags      .insert(mdcm::Tag(0x0010,0x21c0));// Pregnancy Status
	patient_tags      .insert(mdcm::Tag(0x0040,0x0012));// Pre-Medication
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x000e));// Prescription Description
	descr_empty_tags  .insert(mdcm::Tag(0x3010,0x007b));// Prescription Notes
	descr_remove_tags .insert(mdcm::Tag(0x3010,0x0081));// Prescription Notes Sequence TODO
	uid_tags          .insert(mdcm::Tag(0x0070,0x1101));// Presentation Display Collection UID
	uid_tags          .insert(mdcm::Tag(0x0070,0x1102));// Presentation Sequence Collection UID
	descr_remove_tags .insert(mdcm::Tag(0x3010,0x0061));// Prior Treatment Dose Description
	//                                 (0xGGGG,0xEEEE)  // Private attributes (GGGG is odd)
	time_tags         .insert(mdcm::Tag(0x0040,0x4052));// Procedure Step Cancellation DateTime
	descr_remove_tags .insert(mdcm::Tag(0x0018,0x1030));// Protocol Name
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x0619));// Radiation Dose Identification Label
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x0623));// Radiation DoseIn-Vivo Measurement Label
	descr_empty_tags  .insert(mdcm::Tag(0x300a,0x067d));// Radiation Generation Mode Description
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x067c));// Radiation Generation Mode Label
	descr_remove_tags .insert(mdcm::Tag(0x300c,0x0113));// Reason for Omission Description
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x100a));// Reason for Requested Procedure Code Sequence
	descr_remove_tags .insert(mdcm::Tag(0x0032,0x1030));// Reason for Study
	descr_empty_tags  .insert(mdcm::Tag(0x3010,0x005c));// Reason for Superseding
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x2001));// Reason for the Imaging Service Request
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x1002));// Reason for the Requested Procedure
	descr_remove_tags .insert(mdcm::Tag(0x0032,0x1066));// Reason for Visit
	descr_remove_tags .insert(mdcm::Tag(0x0032,0x1067));// Reason for Visit Code Sequence
	time_tags         .insert(mdcm::Tag(0x300a,0x073a));// Recorded RT Control Point Date Time
	uid_tags          .insert(mdcm::Tag(0x3010,0x000b));// Referenced Conceptual Volume UID
	remove_tags       .insert(mdcm::Tag(0x0400,0x0402));// Referenced Digital Signature Sequence
	uid_tags          .insert(mdcm::Tag(0x300a,0x0083));// Referenced Dose Reference UID
	uid_tags          .insert(mdcm::Tag(0x3010,0x006f));// Referenced Dosimetric Objective UID
	uid_tags          .insert(mdcm::Tag(0x3010,0x0031));// Referenced Fiducials UID
	uid_tags          .insert(mdcm::Tag(0x3006,0x0024));// Referenced Frame of Reference UID
	uid_tags          .insert(mdcm::Tag(0x0040,0x4023));// Referenced General Purpose Scheduled Procedure Step Transaction UID
	//                                 (0x0008,0x1140)  // Referenced Image Sequence
	uid_tags          .insert(mdcm::Tag(0x0040,0xa172));// Referenced Observation UID (Trial)
	remove_tags       .insert(mdcm::Tag(0x0038,0x0004));// Referenced Patient Alias Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x1100));// Referenced Patient Photo Sequence
	remove_tags       .insert(mdcm::Tag(0x0008,0x1120));// Referenced Patient Sequence
	//                                 (0x0008,0x1111)  // Referenced Performed Procedure Step Sequence
	remove_tags       .insert(mdcm::Tag(0x0400,0x0403));// Referenced SOP Instance MAC Sequence
	uid_tags          .insert(mdcm::Tag(0x0008,0x1155));// Referenced SOP Instance UI
	uid_tags          .insert(mdcm::Tag(0x0004,0x1511));// Referenced SOP Instance UID in File
	//                                 (0x0008,0x1110)  // Referenced Study Sequence
	remove_tags       .insert(mdcm::Tag(0x0008,0x0092));// Referring Physician's Address
	pn_tags           .insert(mdcm::Tag(0x0008,0x0090));// Referring Physician's Name
	remove_tags       .insert(mdcm::Tag(0x0008,0x0094));// Referring Physician's Telephone Numbers
	remove_tags       .insert(mdcm::Tag(0x0008,0x0096));// Referring Physician Identification Sequence
	remove_tags       .insert(mdcm::Tag(0x0010,0x2152));// Region of Residence
	uid_tags          .insert(mdcm::Tag(0x3006,0x00c2));// Related Frame of Reference UID
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x0275));// Request Attributes Sequence
	descr_remove_tags .insert(mdcm::Tag(0x0032,0x1070));// Requested Contrast Agent
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x1400));// Requested Procedure Comments
	descr_remove_tags .insert(mdcm::Tag(0x0032,0x1060));// Requested Procedure Description
	remove_tags       .insert(mdcm::Tag(0x0040,0x1001));// Requested Procedure ID
	remove_tags       .insert(mdcm::Tag(0x0040,0x1005));// Requested Procedure Location
	//                                 (0x0000,0x1001)  // Requested SOP Instance UID
	remove_tags       .insert(mdcm::Tag(0x0032,0x1032));// Requesting Physician
	remove_tags       .insert(mdcm::Tag(0x0032,0x1033));// Requesting Service
	descr_remove_tags .insert(mdcm::Tag(0x0018,0x9185));// Respiratory Motion Compensation Technique Description
	remove_tags       .insert(mdcm::Tag(0x0010,0x2299));// Responsible Organization
	remove_tags       .insert(mdcm::Tag(0x0010,0x2297));// Responsible Person
	descr_remove_tags .insert(mdcm::Tag(0x4008,0x4000));// Results Comments
	remove_tags       .insert(mdcm::Tag(0x4008,0x0118));// Results Distribution List Sequence
	remove_tags       .insert(mdcm::Tag(0x4008,0x0042));// Results ID Issuer
	pn_tags           .insert(mdcm::Tag(0x300e,0x0008));// Reviewer Name
	descr_remove_tags .insert(mdcm::Tag(0x3006,0x0028));// ROI Description
	descr_remove_tags .insert(mdcm::Tag(0x3006,0x0038));// ROI Generation Description
	pn_tags           .insert(mdcm::Tag(0x3006,0x00a6));// ROI Interpreter
	descr_empty_tags  .insert(mdcm::Tag(0x3006,0x0026));// ROI Name
	descr_empty_tags  .insert(mdcm::Tag(0x3006,0x0088));// ROI Observation Description
	descr_empty_tags  .insert(mdcm::Tag(0x3006,0x0085));// ROI Observation Label
	descr_empty_tags  .insert(mdcm::Tag(0x300a,0x0615));// RT Accessory Device Slot ID
	descr_empty_tags  .insert(mdcm::Tag(0x300a,0x0611));// RT Accessory Holder Slot ID
	descr_empty_tags  .insert(mdcm::Tag(0x3010,0x005a));// RT Physician Intent Narrative
	time_tags         .insert(mdcm::Tag(0x300a,0x0006));// RT Plan Date
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x0004));// RT Plan Description
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x0002));// RT Plan Label
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x0003));// RT Plan Name
	time_tags         .insert(mdcm::Tag(0x300a,0x0007));// RT Plan Time
	descr_replace_tags.insert(mdcm::Tag(0x3010,0x0054));// RT Prescription Label
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x062a));// RT Tolerance Set Label
	descr_remove_tags .insert(mdcm::Tag(0x3010,0x0056));// RT Treatment Approach Label
	uid_tags          .insert(mdcm::Tag(0x3010,0x003b));// RT Treatment Phase UID
	remove_tags       .insert(mdcm::Tag(0x0040,0x4034));// Scheduled Human Performers Sequence
	remove_tags       .insert(mdcm::Tag(0x0038,0x001e));// Scheduled Patient Institution Residence
	remove_tags       .insert(mdcm::Tag(0x0040,0x0006));// Scheduled Performing Physician's Name
	remove_tags       .insert(mdcm::Tag(0x0040,0x000b));// Scheduled Performing Physician Identification Sequence
	remove_tags       .insert(mdcm::Tag(0x0040,0x0007));// Scheduled Procedure Step Description
	time_tags         .insert(mdcm::Tag(0x0040,0x0004));// Scheduled Procedure Step End Date
	time_tags         .insert(mdcm::Tag(0x0040,0x0005));// Scheduled Procedure Step End Time
	time_tags         .insert(mdcm::Tag(0x0040,0x0008));// Scheduled Procedure Step Expiration DateTime
	remove_tags       .insert(mdcm::Tag(0x0040,0x0009));// Scheduled Procedure Step ID
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x0011));// Scheduled Procedure Step Location
	time_tags         .insert(mdcm::Tag(0x0040,0x0010));// Scheduled Procedure Step Modification DateTime
	time_tags         .insert(mdcm::Tag(0x0040,0x0002));// Scheduled Procedure Step Start Date
	time_tags         .insert(mdcm::Tag(0x0040,0x4005));// Scheduled Procedure Step Start DateTime
	time_tags         .insert(mdcm::Tag(0x0040,0x0003));// Scheduled Procedure Step Start Time
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x0001));// Scheduled Station AE Title
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x4027));// Scheduled Station Geographic Location Code Sequence
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x0010));// Scheduled Station Name
	dev_remove_tags   .insert(mdcm::Tag(0x0040,0x4025));// Scheduled Station Name Code Sequence
	dev_remove_tags   .insert(mdcm::Tag(0x0032,0x1020));// Scheduled Study Location
	dev_remove_tags   .insert(mdcm::Tag(0x0032,0x1021));// Scheduled Study Location AE Title
	time_tags         .insert(mdcm::Tag(0x0008,0x0021));// Series Date
	descr_remove_tags .insert(mdcm::Tag(0x0008,0x103e));// Series Description
	uid_tags          .insert(mdcm::Tag(0x0020,0x000e));// Series Instance UID
	time_tags         .insert(mdcm::Tag(0x0008,0x0031));// Series Time
	descr_remove_tags .insert(mdcm::Tag(0x0038,0x0062));// Service Episode Description
	remove_tags       .insert(mdcm::Tag(0x0038,0x0060));// Service Episode ID
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x01b2));// Setup Technique Description
	descr_remove_tags .insert(mdcm::Tag(0x300a,0x01a6));// Shielding Device Description
	remove_tags       .insert(mdcm::Tag(0x0040,0x06fa));// Slide Identifier
	patient_tags      .insert(mdcm::Tag(0x0010,0x21a0));// Smoking Status
	uid_tags          .insert(mdcm::Tag(0x0008,0x0018));// SOP InstanceUID
	uid_tags          .insert(mdcm::Tag(0x3010,0x0015));// Source Conceptual Volume UID
	time_tags         .insert(mdcm::Tag(0x0018,0x936a));// Source End DateTime
	empty_tags        .insert(mdcm::Tag(0x0034,0x0005));// Source Identifier TODO
	//                                 (0x0008,0x2112)  // Source Image Sequence
	dev_remove_tags   .insert(mdcm::Tag(0x300a,0x0216));// Source Manufacturer
	dev_empty_tags    .insert(mdcm::Tag(0x3008,0x0105));// Source Serial Number
	time_tags         .insert(mdcm::Tag(0x0018,0x9369));// Source Start DateTime
	patient_tags      .insert(mdcm::Tag(0x0038,0x0050));// Special Needs
	remove_tags       .insert(mdcm::Tag(0x0040,0x050a));// Specimen Accession Number
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x0602));// Specimen Detailed Description
	id_tags           .insert(mdcm::Tag(0x0040,0x0551));// Specimen Identifier
	struct_zero_tags  .insert(mdcm::Tag(0x0040,0x0610));// Specimen Preparation Sequence
	descr_remove_tags .insert(mdcm::Tag(0x0040,0x0600));// Specimen Short Description
	uid_tags          .insert(mdcm::Tag(0x0040,0x0554));// Specimen UID
	time_tags         .insert(mdcm::Tag(0x0018,0x9516));// Start Acquisition DateTime
	dev_empty_tags    .insert(mdcm::Tag(0x0008,0x1010));// Station Name
	uid_tags          .insert(mdcm::Tag(0x0088,0x0140));// Storage Media File-set UID
	time_tags         .insert(mdcm::Tag(0x3006,0x0008));// Structure Set Date
	descr_remove_tags .insert(mdcm::Tag(0x3006,0x0006));// Structure Set Description
	descr_replace_tags.insert(mdcm::Tag(0x3006,0x0002));// Structure Set Label
	descr_remove_tags .insert(mdcm::Tag(0x3006,0x0004));// Structure Set Name
	time_tags         .insert(mdcm::Tag(0x3006,0x0009));// Structure Set Time
	descr_remove_tags .insert(mdcm::Tag(0x0032,0x4000));// Study Comments
	time_tags         .insert(mdcm::Tag(0x0008,0x0020));// Study Date
	descr_remove_tags .insert(mdcm::Tag(0x0008,0x1030));// Study Description
	empty_tags        .insert(mdcm::Tag(0x0020,0x0010));// Study ID
	remove_tags       .insert(mdcm::Tag(0x0032,0x0012));// Study ID Issuer
	uid_tags          .insert(mdcm::Tag(0x0020,0x000d));// Study Instance UID
	time_tags         .insert(mdcm::Tag(0x0008,0x0030));// Study Time
	uid_tags          .insert(mdcm::Tag(0x0020,0x0200));// Synchronization Frame of Reference UID
	uid_tags          .insert(mdcm::Tag(0x0018,0x2042));// Target UID
	remove_tags       .insert(mdcm::Tag(0x0040,0xa354));// Telephone Number (Trial)
	uid_tags          .insert(mdcm::Tag(0x0040,0xdb0d));// Template Extension Creator UID
	uid_tags          .insert(mdcm::Tag(0x0040,0xdb0c));// Template Extension Organization UID
	remove_tags       .insert(mdcm::Tag(0x4000,0x4000));// Text Comments
	remove_tags       .insert(mdcm::Tag(0x2030,0x0020));// Text String
	time_tags         .insert(mdcm::Tag(0x0008,0x0201));// Timezone Offset From UTC
	remove_tags       .insert(mdcm::Tag(0x0088,0x0910));// Topic Author
	remove_tags       .insert(mdcm::Tag(0x0088,0x0912));// Topic Keywords
	remove_tags       .insert(mdcm::Tag(0x0088,0x0906));// Topic Subject
	remove_tags       .insert(mdcm::Tag(0x0088,0x0904));// Topic Title
	uid_tags          .insert(mdcm::Tag(0x0062,0x0021));// Tracking UID
	uid_tags          .insert(mdcm::Tag(0x0008,0x1195));// Transaction UID
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x5011));// Transducer Identification Sequence
	time_tags         .insert(mdcm::Tag(0x3008,0x0250));// Treatment Date
	dev_empty_tags    .insert(mdcm::Tag(0x300a,0x00b2));// Treatment Machine Name
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x0608));// Treatment Position Group Label
	uid_tags          .insert(mdcm::Tag(0x300a,0x0609));// Treatment Position Group UID
	uid_tags          .insert(mdcm::Tag(0x300a,0x0700));// Treatment Session UID
	descr_replace_tags.insert(mdcm::Tag(0x3010,0x0077));// Treatment Site
	descr_empty_tags  .insert(mdcm::Tag(0x3010,0x007a));// Treatment Technique Notes
	time_tags         .insert(mdcm::Tag(0x3008,0x0251));// Treatment Time
	time_tags         .insert(mdcm::Tag(0x300a,0x0736));// Treatment Tolerance Violation DateTime
	descr_replace_tags.insert(mdcm::Tag(0x300a,0x0734));// Treatment Tolerance Violation Description
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x100a));// UDI Sequence
	uid_tags          .insert(mdcm::Tag(0x0040,0xa124));// UID
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x1009));// Unique Device Identifier
	descr_replace_tags.insert(mdcm::Tag(0x3010,0x0033));// User Content Label
	descr_replace_tags.insert(mdcm::Tag(0x3010,0x0034));// User Content Label Long
	remove_tags       .insert(mdcm::Tag(0x0040,0xa352));// Verbal Source (Trial)
	remove_tags       .insert(mdcm::Tag(0x0040,0xa358));// Verbal Source Identifier Code Sequence (Trial)
	zero_seq_tags     .insert(mdcm::Tag(0x0040,0xa088));// Verifying Observer Identification Code Sequence
	pn_tags           .insert(mdcm::Tag(0x0008,0xa075));// Verifying Observer Name
	//                                 (0x0040,0xa073)  // Verifying Observer Sequence
	id_tags           .insert(mdcm::Tag(0x0040,0xa027));// Verifying Organization
	descr_remove_tags .insert(mdcm::Tag(0x0038,0x4000));// Visit Comments
	dev_replace_tags  .insert(mdcm::Tag(0x0018,0x9371));// X-Ray Detector ID
	dev_remove_tags   .insert(mdcm::Tag(0x0018,0x9373));// X-Ray Detector Label
	dev_replace_tags  .insert(mdcm::Tag(0x0018,0x9367));// X-Ray Source ID
}
// clang-format on

