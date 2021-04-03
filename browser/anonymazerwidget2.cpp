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
#include "dicomutils.h"
#include "alizams_version.h"
#include <cstdlib>
#include <random>
#include <chrono>

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
/* To maintain consistency of PET series with 'modified date' option
 * some additional processing is done.
 */
#if 1
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
#else
	(void)vr;
	(void)t;
	return false;
#endif
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						replace_pn_recurs__(nested, ts, m, implicit, dicts);
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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

static bool find_time_less_1h_recurs__(
	const mdcm::DataSet & ds,
	const std::set<mdcm::Tag> & ts,
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
		if (is_date_time(vr, t) || (ts.find(t) != ts.end()))
		{
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
					for (int x = 0; x < l_size; x++)
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						const mdcm::Item    & item   = sq->GetItem(i);
						const mdcm::DataSet & nested = item.GetNestedDataSet();
						const bool t = find_time_less_1h_recurs__(nested, ts, implicit, dicts);
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
	const std::set<mdcm::Tag> & ts,
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
		if (is_date_time(vr, t) || (ts.find(t) != ts.end()))
		{
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
					for (int x = 0; x < l_size; x++)
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
					de2.SetVR(vr);
					de2.SetByteValue(r.toLatin1(), r.length());
					ds.Replace(de2);
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
					{
						mdcm::Item    & item   = sq->GetItem(i);
						mdcm::DataSet & nested = item.GetNestedDataSet();
						modify_date_time_recurs__(
							nested, ts, less1h, implicit, dicts, y_off, m_off, d_off, s_off);
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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
	for (unsigned int x = 0; x < tmp0.size(); x++) ds.Remove(tmp0.at(x));
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
	for (unsigned int x = 0; x < tmp0.size(); x++) ds.Remove(tmp0.at(x));
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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

static void anonymize_file__(
	bool * ok,
	bool * overlay_in_data,
	const QString & filename,
	const QString & outfilename,
	const mdcm::Dicts & dicts,
	const std::set<mdcm::Tag> & pn_tags,
	const std::set<mdcm::Tag> & uid_tags,
	const std::set<mdcm::Tag> & empty_tags,
	const std::set<mdcm::Tag> & remove_tags,
	const std::set<mdcm::Tag> & device_tags,
	const std::set<mdcm::Tag> & patient_tags,
	const std::set<mdcm::Tag> & institution_tags,
	const std::set<mdcm::Tag> & time_tags,
	const QMap<QString, QString> & uids_map,
	const QMap<QString, QString> & pn_map,
	const bool preserve_uids,
	const bool remove_private,
	const bool remove_graphics,
	const bool retain_dates_times,
	const bool modify_dates_times,
	const bool retain_device_id,
	const bool retain_patient_chars,
	const bool retain_institution_id,
	const int y_off,
	const int m_off,
	const int d_off,
	const int s_off)
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
	empty_recurs__(ds, empty_tags, implicit, dicts);
	//
	remove_recurs__(ds, remove_tags, implicit, dicts);
	//
	if (!retain_device_id)
	{
		remove_recurs__(ds, device_tags, implicit, dicts);
	}
	//
	if (!retain_patient_chars)
	{
		remove_recurs__(ds, patient_tags, implicit, dicts);
	}
	//
	if (!retain_institution_id)
	{
		remove_recurs__(ds, institution_tags, implicit, dicts);
	}
	//
	if (!retain_dates_times)
	{
		if (modify_dates_times)
		{
			const bool less1h = find_time_less_1h_recurs__(
				const_cast<const mdcm::DataSet&>(ds), time_tags, implicit, dicts);
			modify_date_time_recurs__(
				ds, time_tags, less1h, implicit, dicts, y_off, m_off, d_off, s_off);
		}
		else
		{
			remove_date_time_recurs__(ds, time_tags, implicit, dicts);
		}
	}
	//
	if (!preserve_uids)
	{
		replace_uid_recurs__(ds, uid_tags, uids_map, implicit, dicts);
	}
	//
	replace_pn_recurs__(ds, pn_tags, pn_map, implicit, dicts);
	//
	remove_group_length__(ds, implicit, dicts);
	//
	const QString s0("YES");
	const QString s1("DICOM PS 3.15 E.1");
	replace__(ds, mdcm::Tag(0x0012,0x0062), s0.toLatin1().constData(), s0.toLatin1().length(), implicit, dicts);
	replace__(ds, mdcm::Tag(0x0012,0x0063), s1.toLatin1().constData(), s1.toLatin1().length(), implicit, dicts);
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
	const QSize s = QSize((int)(24*si),(int)(24*si));
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
	connect(out_pushButton,    SIGNAL(clicked()),    this,SLOT(set_output_dir()));
	connect(in_pushButton,     SIGNAL(clicked()),    this,SLOT(set_input_dir()));
	connect(run_pushButton,    SIGNAL(clicked()),    this,SLOT(run_()));
	connect(dates_radioButton, SIGNAL(toggled(bool)),this,SLOT(update_modify_dates(bool)));
	connect(mdates_radioButton,SIGNAL(toggled(bool)),this,SLOT(update_retain_dates(bool)));
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
	output_dir = settings.value(QString("output_dir"), QString("")).toString();
	const int tmp01 = settings.value(QString("remove_private"),        0).toInt();
	const int tmp05 = settings.value(QString("remove_overlays"),       0).toInt();
	const int tmp07 = settings.value(QString("preserve_uids"),         0).toInt();
	const int tmp08 = settings.value(QString("retain_dates_times"),    0).toInt();
	const int tmp09 = settings.value(QString("retain_device_id"),      0).toInt();
	const int tmp10 = settings.value(QString("retain_patient_chars"),  0).toInt();
	const int tmp11 = settings.value(QString("retain_institution_id"), 0).toInt();
	const int tmp12 = settings.value(QString("modify_dates_times"),    0).toInt();
	settings.endGroup();
	private_radioButton->setChecked(    (tmp01 == 1) ? true : false);
	graphics_radioButton->setChecked(   (tmp05 == 1) ? true : false);
	uids_radioButton->setChecked(       (tmp07 == 1) ? true : false);
	device_radioButton->setChecked(     (tmp09 == 1) ? true : false);
	chars_radioButton->setChecked(      (tmp10 == 1) ? true : false);
	institution_radioButton->setChecked((tmp11 == 1) ? true : false);
	dates_radioButton->setChecked(      (tmp08 == 1) ? true : false);
	if (tmp08 == 1) // not req.
		mdates_radioButton->setChecked(false);
	else
		mdates_radioButton->setChecked((tmp12 == 1) ? true : false);
}

void AnonymazerWidget2::writeSettings(QSettings & settings)
{
	const int tmp01 = (private_radioButton->isChecked())     ? 1 : 0;
	const int tmp05 = (graphics_radioButton->isChecked())    ? 1 : 0;
	const int tmp07 = (uids_radioButton->isChecked())        ? 1 : 0;
	const int tmp09 = (device_radioButton->isChecked())      ? 1 : 0;
	const int tmp10 = (chars_radioButton->isChecked())       ? 1 : 0;
	const int tmp11 = (institution_radioButton->isChecked()) ? 1 : 0;
	const int tmp08 = (dates_radioButton->isChecked())       ? 1 : 0;
	const int tmp12 = (mdates_radioButton->isChecked())      ? 1 : 0;
	settings.beginGroup(QString("AnonymazerWidget"));
	settings.setValue(QString("output_dir"),           QVariant(output_dir));
	settings.setValue(QString("remove_private"),       QVariant(tmp01));
	settings.setValue(QString("remove_overlays"),      QVariant(tmp05));
	settings.setValue(QString("preserve_uids"),        QVariant(tmp07));
	settings.setValue(QString("retain_device_id"),     QVariant(tmp09));
	settings.setValue(QString("retain_patient_chars"), QVariant(tmp10));
	settings.setValue(QString("retain_institution_id"),QVariant(tmp11));
	settings.setValue(QString("retain_dates_times"),   QVariant(tmp08));
	settings.setValue(QString("modify_dates_times"),   QVariant(tmp12));
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
		if (vr == mdcm::VR::PN || t == mdcm::Tag(0x0010,0x0020))
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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
					for (mdcm::SequenceOfItems::SizeType i = 1; i <= n; i++)
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

static QString generate_uid()
{
	mdcm::UIDGenerator g;
	const QString r = QString::fromLatin1(g.Generate());
	return r;
}

static void build_maps(
	const QStringList & l,
	const std::set<mdcm::Tag> & uid_tags,
	const mdcm::Dicts & dicts,
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
	}
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
	uset.clear();
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
	const mdcm::Dicts & dicts,
	QProgressDialog * pd,
	const int y_off,
	const int m_off,
	const int d_off,
	const int s_off)
{
	const bool preserve_uids         = uids_radioButton->isChecked();
	const bool remove_private        = private_radioButton->isChecked();
	const bool remove_graphics       = graphics_radioButton->isChecked();
	const bool retain_device_id      = device_radioButton->isChecked();
	const bool retain_patient_chars  = chars_radioButton->isChecked();
	const bool retain_institution_id = institution_radioButton->isChecked();
	const bool retain_dates_times    = dates_radioButton->isChecked();
	const bool modify_dates_times    = (retain_dates_times) ? false : mdates_radioButton->isChecked();
	QDir dir(p);
	QStringList dlist = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	QStringList flist = dir.entryList(QDir::Files);
	//
	{
		QStringList filenames;
		for (int x = 0; x < flist.size(); x++)
		{
			pd->setValue(-1);
			QApplication::processEvents();
			if (pd->wasCanceled()) return;
			const QString tmp0 = dir.absolutePath() + QString("/") + flist.at(x);
			filenames.push_back(tmp0);
		}
		for (int x = 0; x < filenames.size(); x++)
		{
			pd->setValue(-1);
			QApplication::processEvents();
			if (pd->wasCanceled()) return;
#if 0
			QString tmp200;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			tmp200 = QString::asprintf("%08d", x);
#else
			tmp200.sprintf("%08d", x);
#endif
			QDateTime date_ = QDateTime::currentDateTime();
			const QString date_str = date_.toString(QString("hhmmss")); // FIXME
			const QString out_filename = date_str + QString("-") + tmp200 + QString(".dcm");
#else
			QFileInfo fi(filenames.at(x));
			const QString out_filename = fi.fileName();
#endif
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
				empty_tags,
				remove_tags,
				device_tags,
				patient_tags,
				institution_tags,
				time_tags,
				uids_m,
				pn_m,
				preserve_uids,
				remove_private,
				remove_graphics,
				retain_dates_times,
				modify_dates_times,
				retain_device_id,
				retain_patient_chars,
				retain_institution_id,
				y_off,
				m_off,
				d_off,
				s_off);
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
	for (int j = 0; j < dlist.size(); j++)
	{
		QApplication::processEvents();
		QDir d(outp + QString("/") + dlist.at(j));
		if (!d.exists()) d.mkpath(d.absolutePath());
		process_directory(
			dir.absolutePath() + QString("/") + dlist.at(j),
			d.absolutePath(),
			uids_m,
			pn_m,
			count_overlay_in_data,
			count_errors,
			dicts,
			pd,
			y_off, m_off, d_off, s_off);
	}
}

void AnonymazerWidget2::run_()
{
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
		build_maps(filenames, uid_tags, dicts, uids_m, pn_m, pd);
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
		dicts,
		pd,
		y_off, m_off, d_off, s_off);
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

void AnonymazerWidget2::update_retain_dates(bool t)
{
	if (t && dates_radioButton->isChecked())
	{
		dates_radioButton->blockSignals(true);
		dates_radioButton->setChecked(false);
		dates_radioButton->blockSignals(false);
	}
}

void AnonymazerWidget2::update_modify_dates(bool t)
{
	if (t && mdates_radioButton->isChecked())
	{
		mdates_radioButton->blockSignals(true);
		mdates_radioButton->setChecked(false);
		mdates_radioButton->blockSignals(false);
	}
}

void AnonymazerWidget2::init_profile()
{
	empty_tags.insert(mdcm::Tag(0x0008,0x0050));// z
	remove_tags.insert(mdcm::Tag(0x0018,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0555));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0022));// date/time x/z
	time_tags.insert(mdcm::Tag(0x0008,0x002a));// date/time x/d
	remove_tags.insert(mdcm::Tag(0x0018,0x1400));// x/d
	remove_tags.insert(mdcm::Tag(0x0018,0x9424));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0032));// date/time x/z
	remove_tags.insert(mdcm::Tag(0x0040,0x4035));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x21b0));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xa353));// x	
	remove_tags.insert(mdcm::Tag(0x0038,0x0010));// x
	time_tags.insert(mdcm::Tag(0x0038,0x0020));// date/time x
	remove_tags.insert(mdcm::Tag(0x0008,0x1084));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1080));// x
	time_tags.insert(mdcm::Tag(0x0038,0x0021));// date/time x
	remove_tags.insert(mdcm::Tag(0x0010,0x2110));// x
	remove_tags.insert(mdcm::Tag(0x4000,0x0010));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xa078));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1081));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1007));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0280));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x9161));// u
	remove_tags.insert(mdcm::Tag(0x0040,0x3001));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x009d)); // x
	pn_tags.insert(mdcm::Tag(0x0008,0x009c));// z
	pn_tags.insert(mdcm::Tag(0x0070,0x0084));// z
	remove_tags.insert(mdcm::Tag(0x0070,0x0086));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0023)); // date/time z/d
	remove_tags.insert(mdcm::Tag(0x0040,0xa730));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0033)); // date/time z/d
	empty_tags.insert(mdcm::Tag(0x0018,0x0010));// z/d
	remove_tags.insert(mdcm::Tag(0x0018,0xa003));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2150));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xa307));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x0300));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0025));// date/time x
	time_tags.insert(mdcm::Tag(0x0008,0x0035));// date/time x
	remove_tags.insert(mdcm::Tag(0x0040,0xa07c));// x
	remove_tags.insert(mdcm::Tag(0xfffc,0xfffc));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x2111));// x
	device_tags.insert(mdcm::Tag(0x0018,0x700a));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1000));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1002));// u
	remove_tags.insert(mdcm::Tag(0x0400,0x0100));// x
	remove_tags.insert(mdcm::Tag(0xfffa,0xfffa));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x9164));// u
	remove_tags.insert(mdcm::Tag(0x0038,0x0040));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x011a));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0119));// x
	uid_tags.insert(mdcm::Tag(0x300a,0x0013));// u
	patient_tags.insert(mdcm::Tag(0x0010,0x2160));// x
	remove_tags.insert(mdcm::Tag(0x0040,4011)); // x
	remove_tags.insert(mdcm::Tag(0x0008,0x0058)); // u FIXME (VR::UI,VM::VM1_n) 
	uid_tags.insert(mdcm::Tag(0x0070,0x031a));// u
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
	time_tags.insert(mdcm::Tag(0x0008,0x0014));// date/time x
	uid_tags.insert(mdcm::Tag(0x0008,0x0014));// u
	institution_tags.insert(mdcm::Tag(0x0008,0x0081));// u
	institution_tags.insert(mdcm::Tag(0x0008,0x0082));//
	institution_tags.insert(mdcm::Tag(0x0008,0x0080));// x/z/d
	institution_tags.insert(mdcm::Tag(0x0008,0x1040));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x1050));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1011));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0111));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x010c));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0115));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0202));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0102));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x010b));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x010a));// x
	uid_tags.insert(mdcm::Tag(0x0008,0x3010));// u
	remove_tags.insert(mdcm::Tag(0x0038,0x0011));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x0021));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x0061));// x
	uid_tags.insert(mdcm::Tag(0x0028,0x1214));// u
	time_tags.insert(mdcm::Tag(0x0010,0x21d0));// date/time x
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
	time_tags.insert(mdcm::Tag(0x0040,0xa192)); // date/time x
	uid_tags.insert(mdcm::Tag(0x0040,0xa402));// u
	time_tags.insert(mdcm::Tag(0x0040,0xa193)); // date/time x
	uid_tags.insert(mdcm::Tag(0x0040,0xa171));// u
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
	time_tags.insert(mdcm::Tag(0x0008,0x0024));// date/time x
	time_tags.insert(mdcm::Tag(0x0008,0x0034));// date/time x
	uid_tags.insert(mdcm::Tag(0x0028,0x1199));// u
	remove_tags.insert(mdcm::Tag(0x0040,0xa07a));// x
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
	remove_tags.insert(mdcm::Tag(0x0010,0x21f0));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x0040));// z
	patient_tags.insert(mdcm::Tag(0x0010,0x1020));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2155));// x
	remove_tags.insert(mdcm::Tag(0x0010,0x2154));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x1030));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0243));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0254));// x
	time_tags.insert(mdcm::Tag(0x0040,0x0250));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x4051));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x0251));// date/time x
	remove_tags.insert(mdcm::Tag(0x0040,0x0253));// x
	time_tags.insert(mdcm::Tag(0x0040,0x0244));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x4050));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x0245));// date/time x
	device_tags.insert(mdcm::Tag(0x0040,0x0241));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4030));// x
	device_tags.insert(mdcm::Tag(0x0040,0x0242));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4028));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1052));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1050));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1102));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x1101));// d
	remove_tags.insert(mdcm::Tag(0x0040,0xa123));// d
	remove_tags.insert(mdcm::Tag(0x0040,0x1103));// x
	remove_tags.insert(mdcm::Tag(0x4008,0x0114));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1062));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1048));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1049));// x
	empty_tags.insert(mdcm::Tag(0x0040,0x2016));// x
	device_tags.insert(mdcm::Tag(0x0018,0x1004));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0012));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x21c0));// x
	uid_tags.insert(mdcm::Tag(0x0070,0x1101));// u
	uid_tags.insert(mdcm::Tag(0x0070,0x1102));// u
	remove_tags.insert(mdcm::Tag(0x0018,0x1030));// x
	remove_tags.insert(mdcm::Tag(0x300c,0x0113));// x	
	remove_tags.insert(mdcm::Tag(0x0040,0x2001));// x
	remove_tags.insert(mdcm::Tag(0x0032,0x1030));// x
	remove_tags.insert(mdcm::Tag(0x0400,0x0402));// x
	uid_tags.insert(mdcm::Tag(0x3006,0x0024));// u
	uid_tags.insert(mdcm::Tag(0x0040,0x4023));// u
	//mdcm::Tag(0x0008,0x1140));// x/z/u* Referenced Image Sequence
	uid_tags.insert(mdcm::Tag(0x0040,0xa172));// u
	remove_tags.insert(mdcm::Tag(0x0038,0x0004));// x
	remove_tags.insert(mdcm::Tag(0x0008,0x1120));// x
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
	uid_tags.insert(mdcm::Tag(0x3006,0x00c2));// u
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
	pn_tags.insert(mdcm::Tag(0x300e,0x0008));// x/z
	remove_tags.insert(mdcm::Tag(0x0040,0x4034));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x001e));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x000b));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0006));// x
	remove_tags.insert(mdcm::Tag(0x0040,0x0007));// x
	time_tags.insert(mdcm::Tag(0x0040,0x0004));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x0005));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x0008));// date/time x
	device_tags.insert(mdcm::Tag(0x0040,0x0011));// x
	time_tags.insert(mdcm::Tag(0x0040,0x0010));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x0002));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x4005));// date/time x
	time_tags.insert(mdcm::Tag(0x0040,0x0003));// date/time x
	device_tags.insert(mdcm::Tag(0x0040,0x0001));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4027));// x
	device_tags.insert(mdcm::Tag(0x0040,0x0010));// x
	device_tags.insert(mdcm::Tag(0x0040,0x4025));// x
	device_tags.insert(mdcm::Tag(0x0032,0x1020));// x
	device_tags.insert(mdcm::Tag(0x0032,0x1021));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0021));// date/time x/d
	remove_tags.insert(mdcm::Tag(0x0008,0x103e));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x000e));// u
	time_tags.insert(mdcm::Tag(0x0008,0x0031));// date/time x/d
	remove_tags.insert(mdcm::Tag(0x0038,0x0062));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x0060));// x
	patient_tags.insert(mdcm::Tag(0x0010,0x21a0));// x
	uid_tags.insert(mdcm::Tag(0x0008,0x0018));// u
	//mdcm::Tag(0x0008,0x2112);// x/d/u*
	device_tags.insert(mdcm::Tag(0x3008,0x0105)); // x
	remove_tags.insert(mdcm::Tag(0x0038,0x0050));// x
	device_tags.insert(mdcm::Tag(0x0008,0x1010));// x/d
	uid_tags.insert(mdcm::Tag(0x0088,0x0140));// u
	remove_tags.insert(mdcm::Tag(0x0032,0x4000));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0020));// date/time z
	empty_tags.insert(mdcm::Tag(0x0008,0x1030));// x
	empty_tags.insert(mdcm::Tag(0x0020,0x0010));// z
	remove_tags.insert(mdcm::Tag(0x0032,0x0012));// x
	uid_tags.insert(mdcm::Tag(0x0020,0x000d));// u
	time_tags.insert(mdcm::Tag(0x0008,0x0030));// date/time z
	uid_tags.insert(mdcm::Tag(0x0020,0x0200));// u
	uid_tags.insert(mdcm::Tag(0x0040,0xdb0d));// u
	uid_tags.insert(mdcm::Tag(0x0040,0xdb0c));// u
	remove_tags.insert(mdcm::Tag(0x4000,0x4000));// x
	remove_tags.insert(mdcm::Tag(0x2030,0x0020));// x
	time_tags.insert(mdcm::Tag(0x0008,0x0201));// date/time x
	remove_tags.insert(mdcm::Tag(0x0088,0x0910));// x
	remove_tags.insert(mdcm::Tag(0x0088,0x0912));// x
	remove_tags.insert(mdcm::Tag(0x0088,0x0906));// x
	remove_tags.insert(mdcm::Tag(0x0088,0x0904));//
	uid_tags.insert(mdcm::Tag(0x0008,0x1195));// u
	uid_tags.insert(mdcm::Tag(0x0040,0xa124));// u
	remove_tags.insert(mdcm::Tag(0x0040,0xa352));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xa358));// x
	remove_tags.insert(mdcm::Tag(0x0040,0xa088));// z
	remove_tags.insert(mdcm::Tag(0x0008,0xa075));// d
	remove_tags.insert(mdcm::Tag(0x0040,0xa073));// d
	remove_tags.insert(mdcm::Tag(0x0040,0xa027));// x
	remove_tags.insert(mdcm::Tag(0x0038,0x4000));// x
	// additional
	remove_tags.insert(mdcm::Tag(0x0012,0x0064));// old de-identification
}

