#define WARN_RAM_SIZE
//#define ENHANCED_PRINT_INFO
//#define ENHANCED_PRINT_GROUPS
//#define TMP_ALWAYS_GEOM_FROM_IMAGE

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt5-core.h"
#else
#include "CG/glwidget-qt5.h"
#endif
#else
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt4-core.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#endif
#include "dicomutils.h"
#include "commonutils.h"
#include "codecutils.h"
#include "contourutils.h"
#include "prconfigutils.h"
#include "ultrasoundregiondata.h"
#include "ultrasoundregionutils.h"
#include "colorspace/colorspace.h"
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include <itkByteSwapper.h>
#include <mdcmSystem.h>
#include <mdcmReader.h>
#include <mdcmFile.h>
#include <mdcmFileMetaInformation.h>
#include <mdcmAttribute.h>
#include <mdcmVersion.h>
#include <mdcmGlobal.h>
#include <mdcmDicts.h>
#include <mdcmDict.h>
#include <mdcmUIDGenerator.h>
#include <mdcmImageWriter.h>
#include <mdcmImageReader.h>
#include <mdcmImage.h>
#include <mdcmElement.h>
#include <mdcmRescaler.h>
#include <mdcmImageChangePlanarConfiguration.h>
#include <mdcmImageApplyLookupTable.h>
#include <mdcmApplySupplementalLUT.h>
#include <mdcmImageHelper.h>
#include <mdcmOverlay.h>
#include <mdcmSplitMosaicFilter.h>
#include <mdcmDataElement.h>
#include <mdcmScanner.h>
#include <mdcmCSAHeader.h>
#include <mdcmVM.h>
#include <mdcmVR.h>
#include <mdcmUIDs.h>
#include "splituihgridfilter.h"
#include <QSet>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#ifndef ALIZA_LOAD_DCM_THREAD
#include <QApplication>
#endif
#include "settingswidget.h"
#include "updateqtcommand.h"
#include <iostream>
#include <list>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>
#include <atomic>
#include "vectormath/scalar/vectormath.h"
#ifdef ALIZA_USE_SYSTEM_LCMS2
#include <lcms2.h>
#else
#include <alizalcms/lcms2.h>
#endif
#include "mmath.h"

namespace
{

typedef Vectormath::Scalar::Vector3 sVector3;
typedef Vectormath::Scalar::Vector4 sVector4;
typedef Vectormath::Scalar::Matrix4 sMatrix4;

struct MixedDicomSeriesInfo
{
	int rows{-1};
	int columns{-1};
	short allocated{};
	bool localizer{};
	bool icc{};
	QString file;
	QString photometric;
	QString spacing;
	QString sop;
};

struct IPPIOP
{
	IPPIOP(
		unsigned int idx_,
		double ipp0_, double ipp1_, double ipp2_,
		double iop0_, double iop1_, double iop2_, double iop3_, double iop4_, double iop5_)
	:
		idx (idx_),
		ipp0(ipp0_), ipp1(ipp1_), ipp2(ipp2_),
		iop0(iop0_), iop1(iop1_), iop2(iop2_), iop3(iop3_), iop4(iop4_), iop5(iop5_) {}
	unsigned int idx;
	double ipp0, ipp1, ipp2;
	double iop0, iop1, iop2, iop3, iop4, iop5;
};

struct less_than_ipp
{
	inline bool operator() (const IPPIOP & s1, const IPPIOP & s2)
	{
		const double t{0.001};
		if (!(
			((s1.iop0 + t) > s2.iop0) && ((s1.iop0 - t) < s2.iop0) &&
			((s1.iop1 + t) > s2.iop1) && ((s1.iop1 - t) < s2.iop1) &&
			((s1.iop2 + t) > s2.iop2) && ((s1.iop2 - t) < s2.iop2) &&
			((s1.iop3 + t) > s2.iop3) && ((s1.iop3 - t) < s2.iop3) &&
			((s1.iop4 + t) > s2.iop4) && ((s1.iop4 - t) < s2.iop4) &&
			((s1.iop5 + t) > s2.iop5) && ((s1.iop5 - t) < s2.iop5)))
			return false;
		double normal[3];
		normal[0] = (s1.iop1 * s1.iop5) - (s1.iop2 * s1.iop4);
		normal[1] = (s1.iop2 * s1.iop3) - (s1.iop0 * s1.iop5);
		normal[2] = (s1.iop0 * s1.iop4) - (s1.iop1 * s1.iop3);
		double dist1{};
		double dist2{};
		dist1 += normal[0] * s1.ipp0;
		dist2 += normal[0] * s2.ipp0;
		dist1 += normal[1] * s1.ipp1;
		dist2 += normal[1] * s2.ipp1;
		dist1 += normal[2] * s1.ipp2;
		dist2 += normal[2] * s2.ipp2;
		const bool r = (dist1 < dist2);
		return r;
	}
};

// Also IPV/IOV (currently only Enhanced US Volume and PA)
bool sort_frames_ippiop(
	const std::map< unsigned int, unsigned int, std::less<unsigned int> > & in,
	std::map< unsigned int, unsigned int, std::less<unsigned int> > & out,
	const FrameGroupValues & values)
{
	if (in.empty()) return false;
	bool ipv_iov{};
	std::vector<IPPIOP> tmp0;
	std::map< unsigned int, unsigned int, std::less<unsigned int> >::const_iterator it =
		in.cbegin();
	{
		const unsigned int x = it->first;
		if (values.at(x).vol_pos_ok && values.at(x).vol_orient_ok)
		{
			ipv_iov = true;
		}
	}
	while (it != in.cend())
	{
		const unsigned int x = it->first;
		double ipp[3];
		double iop[6];
		if (!ipv_iov)
		{
			const bool ok_p = DicomUtils::get_patient_position(values.at(x).pat_pos, ipp);
			const bool ok_o = DicomUtils::get_patient_orientation(values.at(x).pat_orient, iop);
			if (!(ok_o && ok_p))
			{
				return false;
			}
		}
		else
		{
			if (values.at(x).vol_pos_ok && values.at(x).vol_orient_ok)
			{
				ipp[0] = values.at(x).vol_pos[0];
				ipp[1] = values.at(x).vol_pos[1];
				ipp[2] = values.at(x).vol_pos[2];
				iop[0] = values.at(x).vol_orient[0];
				iop[1] = values.at(x).vol_orient[1];
				iop[2] = values.at(x).vol_orient[2];
				iop[3] = values.at(x).vol_orient[3];
				iop[4] = values.at(x).vol_orient[4];
				iop[5] = values.at(x).vol_orient[5];
			}
			else
			{
				return false;
			}
		}
		IPPIOP tmp1(
			x,
			ipp[0], ipp[1], ipp[2],
			iop[0], iop[1], iop[2], iop[3], iop[4], iop[5]);
		tmp0.push_back(tmp1);
		++it;
	}
	std::stable_sort(tmp0.begin(), tmp0.end(), less_than_ipp());
#ifdef ENHANCED_PRINT_INFO
#if 0
	std::cout << "Sorting frames by IPP/IOP ..." << std::endl;
#endif
#endif
	for (unsigned int j = 0; j < tmp0.size(); ++j)
	{
		const IPPIOP & k = tmp0.at(j);
		out[k.idx] = j;
#if 0
		std::cout << "out[" << k.idx << "] = " << j << std::endl;
#endif
	}
	return true;
}

struct files_less_than_ipp
{
	inline bool operator() (const QString & f1, const QString & f2)
	{
		const double t{0.001};
		mdcm::Tag ipp(0x0020,0x0032);
		mdcm::Tag iop(0x0020,0x0037);
		std::set<mdcm::Tag> tags;
		tags.insert(ipp);
		tags.insert(iop);
		mdcm::Reader reader1;
		mdcm::Reader reader2;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		reader1.SetFileName(QDir::toNativeSeparators(f1).toUtf8().constData());
		reader2.SetFileName(QDir::toNativeSeparators(f2).toUtf8().constData());
#else
		reader1.SetFileName(QDir::toNativeSeparators(f1).toLocal8Bit().constData());
		reader2.SetFileName(QDir::toNativeSeparators(f2).toLocal8Bit().constData());
#endif
#else
		reader1.SetFileName(f1.toLocal8Bit().constData());
		reader2.SetFileName(f2.toLocal8Bit().constData());
#endif
		if (reader1.ReadSelectedTags(tags) && reader2.ReadSelectedTags(tags))
		{
			const mdcm::DataSet & ds1 = reader1.GetFile().GetDataSet();
			const mdcm::DataSet & ds2 = reader2.GetFile().GetDataSet();
			if (!ds1.FindDataElement(ipp) || !ds2.FindDataElement(ipp))
			{
				return false;
			}
			if (!ds1.FindDataElement(iop) || !ds2.FindDataElement(iop))
			{
				return false;
			}
			mdcm::Attribute<0x0020,0x0032> ipp1;
			ipp1.Set(ds1);
			mdcm::Attribute<0x0020,0x0037> iop1;
			iop1.Set(ds1);
			mdcm::Attribute<0x0020,0x0032> ipp2;
			ipp2.Set(ds2);
			mdcm::Attribute<0x0020,0x0037> iop2;
			iop2.Set(ds2);
			if (ipp1.GetNumberOfValues() < 3 || ipp2.GetNumberOfValues() < 3)
			{
				return false;
			}
			if (iop1.GetNumberOfValues() < 6 || iop2.GetNumberOfValues() < 6)
			{
				return false;
			}
			if (!(
				((iop1[0] + t) > iop2[0]) && ((iop1[0] - t) < iop2[0]) &&
				((iop1[1] + t) > iop2[1]) && ((iop1[1] - t) < iop2[1]) &&
				((iop1[2] + t) > iop2[2]) && ((iop1[2] - t) < iop2[2]) &&
				((iop1[3] + t) > iop2[3]) && ((iop1[3] - t) < iop2[3]) &&
				((iop1[4] + t) > iop2[4]) && ((iop1[4] - t) < iop2[4]) &&
				((iop1[5] + t) > iop2[5]) && ((iop1[5] - t) < iop2[5])))
			{
				return false;
			}
			double normal[3];
			normal[0] = (iop1[1] * iop1[5]) - (iop1[2] * iop1[4]);
			normal[1] = (iop1[2] * iop1[3]) - (iop1[0] * iop1[5]);
			normal[2] = (iop1[0] * iop1[4]) - (iop1[1] * iop1[3]);
			double dist1{};
			double dist2{};
			for (int i = 0; i < 3; ++i) dist1 += normal[i] * ipp1[i];
			for (int i = 0; i < 3; ++i) dist2 += normal[i] * ipp2[i];
			const bool r = (dist1 < dist2);
			return r;
		}
		return false;
	}
};

void sort_dicom_files_ippiop(
	const std::vector<QString> & images,
	std::vector<QString> & images_ipp)
{
	for (size_t x = 0; x < images.size(); ++x)
	{
		images_ipp.push_back(images.at(x));
	}
	std::stable_sort(images_ipp.begin(), images_ipp.end(), files_less_than_ipp());
}

bool acqtime_less_than(const QString & s1, const QString & s2)
{
	return s1 < s2;
}

bool acqtime_more_than(const QString & s1, const QString & s2)
{
	return s1 > s2;
}

QString generate_string_0(
	const mdcm::DataSet & ds,
	const mdcm::Tag t,
	const QString & name,
	const QString & unit)
{
	QString s;
	QString tmp0;
	if (DicomUtils::get_string_value(ds, t, tmp0))
	{
		const QString s2 = tmp0.trimmed();
		if (!s2.isEmpty())
		{
			s += QString("<span class='y9'>");
			s += name;
			s += QString("</span><br /><span class='y8'>");
			s += s2;
			if (!unit.isEmpty())
			{
				s += QString(" ");
				s += unit;
			}
			s += QString("</span><br />");
		}
	}
	return s;
}

QString read_MRImageModule(const mdcm::DataSet & ds)
{
	QString s;
	const mdcm::Tag tSequenceName(0x0018,0x0024);
	s += generate_string_0(
		ds,
		tSequenceName,
		QString("Sequence Name"),
		QString(""));
	const mdcm::Tag tMRAcquisitionType(0x0018,0x0023);
	QString MRAcquisitionType;
	if (DicomUtils::get_string_value(
		ds, tMRAcquisitionType, MRAcquisitionType))
	{
		const QString tmp2 = MRAcquisitionType.trimmed();
		if (!tmp2.isEmpty())
		{
			s += QString("<span class='y9'>MR Acquisition Type </span><br />");
			MRAcquisitionType = MRAcquisitionType.trimmed();
			s += QString("<span class='y8'>");
			if (MRAcquisitionType == QString("2D"))
				s += QString("frequency*phase");
			else if (MRAcquisitionType == QString("3D"))
				s += QString("frequency*phase*phase");
			else
				s += MRAcquisitionType;
			s += QString("</span><br />");
		}
	}
	const mdcm::Tag tScanningSequence(0x0018,0x0020);
	QString ScanningSequence;
	if (DicomUtils::get_string_value(
			ds, tScanningSequence, ScanningSequence))
	{
		const QString tmp2 = ScanningSequence.trimmed();
		if (!tmp2.isEmpty())
		{
			s += QString("<span class='y9'>Scanning Sequence</span><br />");
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			const QStringList tmp0 =
				tmp2.split(
					QString("\\"), Qt::SkipEmptyParts);
#else
			const QStringList tmp0 =
				tmp2.split(
					QString("\\"), QString::SkipEmptyParts);
#endif
			const int tmp0_size = tmp0.size();
			s += QString("<span class='y8'>");
			for (int x = 0; x < tmp0_size; ++x)
			{
				const QString tmp1 = tmp0.at(x).trimmed();
				if (tmp1 == QString("SE"))
					s += QString("Spin Echo");
				else if (tmp1 == QString("IR"))
					s += QString("Inversion Recovery");
				else if (tmp1 == QString("GR"))
					s += QString("Gradient Recalled");
				else if (tmp1 == QString("EP"))
					s += QString("Echo Planar");
				else if (tmp1 == QString("RM"))
					s += QString("Research Mode");
				else
					s += tmp1;
				if (x != tmp0_size - 1) s += QString("<br />");
			}
			s += QString("</span><br />");
			const mdcm::Tag tSequenceVariant(0x0018,0x0021);
			QString SequenceVariant;
			if (DicomUtils::get_string_value(
				ds, tSequenceVariant, SequenceVariant))
			{
				const QString tmp22 = SequenceVariant.trimmed();
				if (!tmp22.isEmpty())
				{
					s += QString("<span class='y9'>Variant</span><br />");
					{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
						const QStringList tmp00 =
							tmp22.split(
								QString("\\"), Qt::SkipEmptyParts);
#else
						const QStringList tmp00 =
							tmp22.split(
								QString("\\"), QString::SkipEmptyParts);
#endif
						const int tmp00_size = tmp00.size();
						s += QString("<span class='y8'>");
						for (int x = 0; x < tmp00_size; ++x)
						{
							const QString tmp1 = tmp00.at(x).trimmed();
							if (tmp1 == QString("SK"))
								s += QString("Segmented k-space");
							else if (tmp1 == QString("MTC"))
								s += QString("Magnetization transfer contrast");
							else if (tmp1 == QString("SS"))
								s += QString("Steady state");
							else if (tmp1 == QString("TRSS"))
								s += QString("Time reversed steady state");
							else if (tmp1 == QString("SP"))
								s += QString("Spoiled");
							else if (tmp1 == QString("MP"))
								s += QString("MAG prepared");
							else if (tmp1 == QString("OSP"))
								s += QString("Oversampling phase");
							else if (tmp1 == QString("NONE"))
								s += QString("No sequence variant");
							else
								s += tmp1;
							if (x != tmp00_size - 1) s += QString("<br />");
						}
						s += QString("</span>");
					}
					s += QString("<br />");
				}
			}
			const mdcm::Tag tScanOptions(0x0018,0x0022);
			QString ScanOptions;
			if (DicomUtils::get_string_value(
				ds, tScanOptions, ScanOptions))
			{
				const QString tmp22 = ScanOptions.trimmed();
				if (!tmp22.isEmpty())
				{
					s += QString("<span class='y9'>Options</span><br />");
					{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
						const QStringList tmp00 =
							tmp22.split(
								QString("\\"), Qt::SkipEmptyParts);
#else
						const QStringList tmp00 =
							tmp22.split(
								QString("\\"), QString::SkipEmptyParts);
#endif
						const int tmp00_size = tmp00.size();
						s += QString("<span class='y8'>");
						for (int x = 0; x < tmp00_size; ++x)
						{
							const QString tmp1 = tmp00.at(x).trimmed();
							if (tmp1 == QString("PER"))
								s += QString("Phase Encode Reordering");
							else if (tmp1 == QString("RG"))
								s += QString("Respiratory Gating");
							else if (tmp1 == QString("CG"))
								s += QString("Cardiac Gating");
							else if (tmp1 == QString("PPG"))
								s += QString("Peripheral Pulse Gating");
							else if (tmp1 == QString("FC"))
								s += QString("Flow Compensation");
							else if (tmp1 == QString("PFF"))
								s += QString("Partial Fourier-Frequency");
							else if (tmp1 == QString("PFP"))
								s += QString("Partial Fourier-Phase");
							else if (tmp1 == QString("SP"))
								s += QString("Spatial Presaturation");
							else if (tmp1 == QString("FS"))
								s += QString("Fat Saturation");
							else
								s += tmp1;
							if (x != tmp00_size - 1) s += QString("<br />");
						}
						s += QString("</span>");
					}
					s += QString("<br />");
				}
			}
		}
	}
	const mdcm::Tag tRepetitionTime(0x0018,0x0080);
	s += generate_string_0(
		ds,
		tRepetitionTime,
		QString("Repetition Time"),
		QString("ms"));
	const mdcm::Tag tEchoTime(0x0018,0x0081);
	s += generate_string_0(
		ds,
		tEchoTime,
		QString("Echo Time"),
		QString("ms"));
	const mdcm::Tag tEchoTrainLength(0x0018,0x0091);
	s += generate_string_0(
		ds,
		tEchoTrainLength,
		QString("Echo Train Length"),
		QString(""));
	const mdcm::Tag tInversionTime(0x0018,0x0082);
	s += generate_string_0(
		ds,
		tInversionTime,
		QString("Inversion Time"),
		QString("ms"));
	const mdcm::Tag tTriggerTime(0x0018,0x1060);
	s += generate_string_0(
		ds,
		tTriggerTime,
		QString("Trigger Time"),
		QString("ms"));
/*
	const mdcm::Tag tImagingFrequency(0x0018,0x0084);
	s += generate_string_0(
		ds,
		tImagingFrequency,
		QString("Imaging Frequency"),
		QString("MHz"));
*/
	const mdcm::Tag tImagedNucleus(0x0018,0x0085);
	s += generate_string_0(
		ds,
		tImagedNucleus,
		QString("Imaged Nucleus"),
		QString(""));
	const mdcm::Tag tAngioFlag(0x0018,0x0025);
	s += generate_string_0(
		ds,
		tAngioFlag,
		QString("Angio Flag"),
		QString(""));
	const mdcm::Tag tNumberofAverages(0x0018,0x0083);
	s += generate_string_0(
		ds,
		tNumberofAverages,
		QString("Number of Averages"),
		QString(""));
	const mdcm::Tag tSpacingBetweenSlices(0x0018,0x0088);
	s += generate_string_0(
		ds,
		tSpacingBetweenSlices,
		QString("Spacing Between Slices"),
		QString("mm"));
	const mdcm::Tag tNumberofPhaseEncodingSteps(0x0018,0x0089);
	s += generate_string_0(
		ds,
		tNumberofPhaseEncodingSteps,
		QString("Number of Phase Encoding Steps"),
		QString(""));
	const mdcm::Tag tPercentSampling(0x0018,0x0093);
	s += generate_string_0(
		ds,
		tPercentSampling,
		QString("Percent Sampling"),
		QString("&#37;"));
	const mdcm::Tag tPixelBandwidth(0x0018,0x0095);
	s += generate_string_0(
		ds,
		tPixelBandwidth,
		QString("Pixel Bandwidth"),
		QString("Hz"));
	const mdcm::Tag tFlipAngle(0x0018,0x1314);
	s += generate_string_0(
		ds,
		tFlipAngle,
		QString("Flip Angle"),
		QString("&#176;"));
	const mdcm::Tag tVariableFlipAngleFlag(0x0018,0x1315);
	s += generate_string_0(
		ds,
		tVariableFlipAngleFlag,
		QString("Variable Flip Angle Flag"),
		QString(""));
	const mdcm::Tag tSAR(0x0018,0x1316);
	s += generate_string_0(
		ds,
		tSAR,
		QString("SAR"),
		QString("W/kg"));
	const mdcm::Tag tDBdt(0x0018,0x1318);
	s += generate_string_0(
		ds,
		tDBdt,
		QString("DB/dt"),
		QString("T/s"));
	// TODO
	//
	const mdcm::Tag tB1rms(0x0018,0x1320);
	float fB1rms;
	if (DicomUtils::get_fl_value(ds, tB1rms, &fB1rms))
	{
		QString B1rms =
			QVariant(static_cast<double>(fB1rms)).toString();
		B1rms = B1rms.trimmed();
		if (!B1rms.isEmpty())
		{
			s += QString(
				"<span class='y9'>B1 + rms</span>"
				"<br /><span class='y8'>");
			s += B1rms;
			s += QString(" uT</span><br />");
		}
	}
	//
	//
	//
	if (!s.isEmpty())
	{
		s.prepend(QString("<span class='y7'>MR Image Module</span><br />"));
		s += QString("<br />");
	}
	return s;
}

QString read_CTImageModule(const mdcm::DataSet & ds)
{
	QString s;
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x9361),
		QString("Multienergy CT Acquisition"),
		QString(""));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x0060),
		QString("KVP"),
		QString(""));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x0022),
		QString("Scan Options"),
		QString(""));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x0090),
		QString("DataCollectionDiameter"),
		QString("mm"));
	std::vector<double> DataCollectionCenterPatient;
	if (DicomUtils::get_fd_values(
		ds,
		mdcm::Tag(0x0018,0x9313),
		DataCollectionCenterPatient))
	{
		s += QString("<span class='y9'>Data Collection Center Patient</span><br />");
		for (size_t x = 0; x < DataCollectionCenterPatient.size(); ++x)
		{
			const QString j =
				QVariant(DataCollectionCenterPatient.at(x))
					.toString().trimmed();
			if (!j.isEmpty())
			{
				s += QString("<span class='y8'> ");
				s += j;
				s += QString(" </span><br />");
			}
		}
		s += QString("<br />");
	}
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1100),
		QString("ReconstructionDiameter"),
		QString("mm"));
	std::vector<double> ReconstructionTargetCenterPatient;
	if (DicomUtils::get_fd_values(
		ds,
		mdcm::Tag(0x0018,0x9318),
		ReconstructionTargetCenterPatient))
	{
		s += QString("<span class='y9'>Reconstruction Target Center Patient</span><br />");
		for (size_t x = 0; x < ReconstructionTargetCenterPatient.size(); ++x)
		{
			const QString j =
				QVariant(ReconstructionTargetCenterPatient.at(x))
					.toString().trimmed();
			if (!j.isEmpty())
			{
				s += QString("<span class='y8'> ");
				s += j;
				s += QString(" </span><br />");
			}
		}
	}
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1110),
		QString("Distance Source To Detector"),
		QString("mm"));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1111),
		QString("Distance Source To Patient"),
		QString("mm"));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1120),
		QString("Gantry Detector Tilt"),
		QString(QChar(0xb0)));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1130),
		QString("Table Height"),
		QString("mm"));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1140),
		QString("Rotation Direction"),
		QString(""));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1150),
		QString("Exposure Time"),
		QString("ms"));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1152),
		QString("Exposure"),
		QString("mAs"));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1153),
		QString("Exposure In uAs"),
		QString("µAs"));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1160),
		QString("Filter Type"),
		QString(""));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1170),
		QString("Generator Power"),
		QString("kW"));
	{
		std::vector<double> FocalSpots;
		if (DicomUtils::get_ds_values(ds, mdcm::Tag(0x0018,0x1190), FocalSpots))
		{
			s += QString("<span class='y9'>Focal Spots</span><br />");
			for (size_t x = 0; x < FocalSpots.size(); ++x)
			{
				const QString j =
					QVariant(FocalSpots.at(x))
						.toString().trimmed();
				if (!j.isEmpty())
				{
					s += QString("<span class='y8'> ");
					s += j;
					s += QString(" mm</span><br />");
				}
			}
		}
	}
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x1210),
		QString("Convolution Kernel"),
		QString("")); // TODO VM 1-n
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9305), &k))
		{
			s += QString("<span class='y9'>Revolution Time</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" s</span><br />");
		}
	}
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9306), &k))
		{
			s += QString("<span class='y9'>Single Collimation Width</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" mm</span><br />");
		}
	}
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9307), &k))
		{
			s += QString("<span class='y9'>Total Collimation Width</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" mm</span><br />");
		}
	}
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9309), &k))
		{
			s += QString("<span class='y9'>Table Speed</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" mm/s</span><br />");
		}
	}
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9310), &k))
		{
			s += QString("<span class='y9'>Table Feed Per Rotation</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" mm</span><br />");
		}
	}
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9311), &k))
		{
			s += QString("<span class='y9'>Spiral Pitch Factor</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString("</span><br />");
		}
	}
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x9323),
		QString("Exposure Modulation Type"),
		QString("")); // TODO VM 1-n
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9324), &k))
		{
			s += QString("<span class='y9'>Estimated Dose Saving</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" %</span><br />");
		}
	}
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9345), &k))
		{
			s += QString("<span class='y9'>CTDIvol</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" mGy</span><br />");
		}
	}
/*
Tag	(0018,9346)
Type	Optional (3)
Keyword	CTDIPhantomTypeCodeSequence
Value Multiplicity	1
Value Representation	Sequence (SQ)
The type of phantom used for CTDI measurement according to [IEC 60601-2-44].

Only a single Item is permitted in this Sequence.

Tag	(0008,0104)
Type	Required (1)
Keyword	CodeMeaning
Value Multiplicity	1
Value Representation	Long String (LO)
Text that conveys the meaning of the Coded Entry.
*/
	{
		double k;
		if (DicomUtils::get_fd_value(ds, mdcm::Tag(0x0018,0x9371), &k))
		{
			s += QString("<span class='y9'>Water Equivalent Diameter</span><br /><span class='y8'>");
			s += QVariant(k).toString();
			s += QString(" mm</span><br />");
		}
	}
/*
(0018,1272)
Type	Conditionally Required (1C)
Keyword	WaterEquivalentDiameterCalculationMethodCodeSequence
Value Multiplicity	1
Value Representation	Sequence (SQ)
The method of calculation of Water Equivalent Diameter (0018,1271).

Required if Water Equivalent Diameter (0018,1271) is present.

Only a single Item is permitted in this Sequence.

Tag	(0008,0104)
Type	Required (1)
Keyword	CodeMeaning
Value Multiplicity	1
Value Representation	Long String (LO)
*/

	s += generate_string_0(
		ds,
		mdcm::Tag(0x0018,0x115E),
		QString("Image And Fluoroscopy Area Dose Product"),
		QString("dGy*cm*cm"));
	{
		std::vector<double> IsocenterPosition;
		if (DicomUtils::get_fd_values(
			ds,
			mdcm::Tag(0x300A,0x012C),
			IsocenterPosition))
		{
			s += QString("<span class='y9'>Isocenter Position</span><br />");
			for (size_t x = 0; x < IsocenterPosition.size(); ++x)
			{
				const QString j =
					QVariant(IsocenterPosition.at(x))
						.toString().trimmed();
				if (!j.isEmpty())
				{
					s += QString("<span class='y8'> ");
					s += j;
					s += QString(" mm</span><br />");
				}
			}
			s += QString("<br />");
		}
	}
// TODO all
	//
	//
	//
	if (!s.isEmpty())
	{
		s.prepend(QString("<span class='y7'>CT Image Module</span><br />"));
		s += QString("<br />");
	}
	return s;
}

QString read_CommonCTMRImageDescriptionMacro(const mdcm::DataSet & ds)
{
	QString s;
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0008,0x9205),
		QString("Pixel Presentation"),
		QString(""));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0008,0x9206),
		QString("Volumetric Properties"),
		QString(""));
	s += generate_string_0(
		ds,
		mdcm::Tag(0x0008,0x9207),
		QString("Volume Based Calculation Technique"),
		QString(""));
	if (!s.isEmpty())
	{
		s.prepend(QString("<span class='y7'>Common CT/MR Image Description Macro</span><br />"));
	}
	return s;
}

QString read_PhotoacousticImage(const mdcm::DataSet & ds)
{
// TODO check again

	QString s;

	//////////////////////////////////////////////
	//
	// Photoacoustic Image Data Type
	//
	//
	{
		const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
		const mdcm::Tag tImageDataTypeSequence(0x0018,0x9807);
		const mdcm::Tag tImageDataTypeCodeSequence(0x0018,0x9836);
		const mdcm::Tag tCodeMeaning(0x0008,0x0104);
		QString s0;
		{
			const mdcm::DataElement & e = ds.GetDataElement(tSharedFunctionalGroupsSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq && sq->GetNumberOfItems() == 1)
			{
				const mdcm::Item & item = sq->GetItem(1);
				const mdcm::DataSet& nds = item.GetNestedDataSet();
				{
					const mdcm::DataElement & e1 = nds.GetDataElement(tImageDataTypeSequence);
					mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 = e1.GetValueAsSQ();
					if (sq1 && sq1->GetNumberOfItems() == 1)
					{
						const mdcm::Item & item1 = sq1->GetItem(1);
						const mdcm::DataSet& nds1 = item1.GetNestedDataSet();
						{
							const mdcm::DataElement & e2 = nds1.GetDataElement(tImageDataTypeCodeSequence);
							mdcm::SmartPointer<mdcm::SequenceOfItems> sq2 = e2.GetValueAsSQ();
							if (sq2 && sq2->GetNumberOfItems() == 1)
							{
								const mdcm::Item & item2 = sq2->GetItem(1);
								const mdcm::DataSet& nds2 = item2.GetNestedDataSet();
								QString CodeMeaning;
								if (DicomUtils::get_string_value(nds2, tCodeMeaning, CodeMeaning))
								{
									s0 += QString(
											"<span class='y9'>Image Data Type</span><br />"
											"<span class='y8'>&#160;&#160;") +
										CodeMeaning +
										QString("</span><br />");
								}
							}
						}
					}
				}
			}
			if (!s0.isEmpty())
			{
				s += QString("<span class='y7'Photoacoustic Image Data Type</span><br />") + s0;
			}
		}
	}
	//
	//
	//
	//
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	//
	// Photoacoustic Image Module Attributes
	//
	//
	{
		const mdcm::Tag tExcitationWavelengthSequence(0x0018,0x9825);
		const mdcm::Tag tExcitationWavelength(0x0018,0x9826);
		const mdcm::Tag tIlluminationTypeCodeSequence(0x0022,0x0016);
		const mdcm::Tag tIlluminationTranslationFlag(0x0018,0x9828);
		const mdcm::Tag tAcousticCouplingMediumCodeSequence(0x0018,0x982a);
		const mdcm::Tag tAcousticCouplingMediumFlag(0x0018,0x9829);
		const mdcm::Tag tCouplingMediumTemperature(0x0018,0x982b);
		const mdcm::Tag tPositionMeasuringDeviceUsed(0x0018,0x980c);
		const mdcm::Tag tCodeMeaning(0x0008,0x0104);
		QString s0;
		{
			const mdcm::DataElement & e = ds.GetDataElement(tExcitationWavelengthSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq)
			{
				s0 += QString("<span class='y9'>Excitation Wavelength Sequence</span><br />");
				const unsigned int n = sq->GetNumberOfItems();
				for (unsigned int x = 0; x < n; ++x)
				{
					const mdcm::Item & item = sq->GetItem(x + 1);
					const mdcm::DataSet & nds = item.GetNestedDataSet();
					double ExcitationWavelength;
					if (DicomUtils::get_fd_value(
							nds,
							tExcitationWavelength,
							&ExcitationWavelength))
					{
						s0 += QString("<span class='y8'>&#160;&#160;") +
							QVariant(static_cast<qreal>(ExcitationWavelength)).toString() +
							QString("&#160;nm</span><br />");
					}
				}
			}
		}
		{
			QString IlluminationTranslationFlag;
			if (DicomUtils::get_string_value(
					ds,
					tIlluminationTranslationFlag,
					IlluminationTranslationFlag))
			{
				s0 += QString(
						"<span class='y9'>Illumination Translation Flag</span><br />"
						"<span class='y8'>&#160;&#160;") +
					IlluminationTranslationFlag +
					QString("</span><br />");
			}
		}
		{
			const mdcm::DataElement & e = ds.GetDataElement(tIlluminationTypeCodeSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq)
			{
				const unsigned int n = sq->GetNumberOfItems();
				if (n == 1)
				{
					const mdcm::Item & item = sq->GetItem(1);
					const mdcm::DataSet & nds = item.GetNestedDataSet();
					QString CodeMeaning;
					if (DicomUtils::get_string_value(
							nds,
							tCodeMeaning,
							CodeMeaning))
					{
						s0 += QString(
								"<span class='y9'>Illumination Type</span><br />"
								"<span class='y8'>&#160;&#160;") +
							CodeMeaning +
							QString("</span><br />");
					}
				}
			}
		}
		{
			QString AcousticCouplingMediumFlag;
			if (DicomUtils::get_string_value(
					ds,
					tAcousticCouplingMediumFlag,
					AcousticCouplingMediumFlag))
			{
				s0 += QString(
						"<span class='y9'>Acoustic Coupling Medium Flag</span><br />"
						"<span class='y8'>&#160;&#160;") +
					AcousticCouplingMediumFlag +
					QString("</span><br />");
			}
		}
		{
			const mdcm::DataElement & e = ds.GetDataElement(tAcousticCouplingMediumCodeSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq)
			{
				const unsigned int n = sq->GetNumberOfItems();
				if (n == 1)
				{
					const mdcm::Item & item = sq->GetItem(1);
					const mdcm::DataSet & nds = item.GetNestedDataSet();
					QString CodeMeaning;
					if (DicomUtils::get_string_value(
							nds,
							tCodeMeaning,
							CodeMeaning))
					{
						s0 += QString(
								"<span class='y9'>Acoustic Coupling Medium</span><br />"
								"<span class='y8'>&#160;&#160;") +
							CodeMeaning +
							QString("</span><br />");
					}
				}
			}
		}
		{
			float CouplingMediumTemperature;
			if (DicomUtils::get_fl_value(
					ds,
					tCouplingMediumTemperature,
					&CouplingMediumTemperature))
			{
				s0 += QString(
						"<span class='y9'>Coupling Medium Temperature</span><br />"
						"<span class='y8'>&#160;&#160;") +
					QVariant(static_cast<qreal>(CouplingMediumTemperature)).toString() +
					QString("&#160;") +
					QString(QChar(0x00B0)) +
					QString("</span><br />");
			}
		}
		{
			QString PositionMeasuringDeviceUsed;
			if (DicomUtils::get_string_value(
					ds,
					tPositionMeasuringDeviceUsed,
					PositionMeasuringDeviceUsed))
			{
				s0 += QString(
						"<span class='y9'>Position Measuring Device Used</span><br />"
						"<span class='y8'>&#160;&#160;") +
					PositionMeasuringDeviceUsed +
					QString("</span><br />");
			}
		}
		if (!s0.isEmpty())
		{
			s += QString("<span class='y7'>Photoacoustic Image Module</span><br />") + s0;
		}
	}
	//
	//
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	//
	// Photoacoustic Transducer Module
	//
	//
	{
		const mdcm::Tag tTransducerGeometryCodeSequence(0x0018,0x980d);
		const mdcm::Tag tTransducerTechnologySequence(0x0018,0x9831);
		const mdcm::Tag tTransducerResponseSequence(0x0018,0x982c);
		const mdcm::Tag tUpperCutoffFrequency(0x0018,0x9830);
		const mdcm::Tag tLowerCutoffFrequency(0x0018,0x982f);
		const mdcm::Tag tFractionalBandwidth(0x0018,0x982e);
		const mdcm::Tag tCenterFrequency(0x0018,0x982d);
		const mdcm::Tag tCodeMeaning(0x0008,0x0104);
		QString s1;
		{
			const mdcm::DataElement & e = ds.GetDataElement(tTransducerGeometryCodeSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq)
			{
				const unsigned int n = sq->GetNumberOfItems();
				if (n == 1)
				{
					const mdcm::Item & item = sq->GetItem(1);
					const mdcm::DataSet & nds = item.GetNestedDataSet();
					QString CodeMeaning;
					if (DicomUtils::get_string_value(
							nds,
							tCodeMeaning,
							CodeMeaning))
					{
						s1 += QString(
								"<span class='y9'>Transducer Geometry</span><br />"
								"<span class='y8'>&#160;&#160;") +
							CodeMeaning +
							QString("</span><br />");
					}
				}
			}
		}
		{
			const mdcm::DataElement & e = ds.GetDataElement(tTransducerTechnologySequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq)
			{
				const unsigned int n = sq->GetNumberOfItems();
				if (n == 1)
				{
					const mdcm::Item & item = sq->GetItem(1);
					const mdcm::DataSet & nds = item.GetNestedDataSet();
					QString CodeMeaning;
					if (DicomUtils::get_string_value(
							nds,
							tCodeMeaning,
							CodeMeaning))
					{
						s1 += QString(
								"<span class='y9'>Transducer Technology</span><br />"
								"<span class='y8'>&#160;&#160;") +
							CodeMeaning +
							QString("</span><br />");
					}
				}
			}
		}
		{
			const mdcm::DataElement & e = ds.GetDataElement(tTransducerResponseSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq)
			{
				const unsigned int n = sq->GetNumberOfItems();
				if (n == 1)
				{
					QString s2;
					const mdcm::Item & item = sq->GetItem(1);
					const mdcm::DataSet & nds = item.GetNestedDataSet();
					double UpperCutoffFrequency;
					double LowerCutoffFrequency;
					double FractionalBandwidth;
					double CenterFrequency;
					if (DicomUtils::get_fd_value(
							nds,
							tUpperCutoffFrequency,
							&UpperCutoffFrequency))
					{
						s2 += QString("<span class='y8'>&#160;&#160;Upper Cutoff Frequency<br />&#160;&#160;") +
							QVariant(UpperCutoffFrequency).toString() +
							QString("&#160;MHz</span><br />");
					}
					if (DicomUtils::get_fd_value(
							nds,
							tLowerCutoffFrequency,
							&LowerCutoffFrequency))
					{
						s2 += QString("<span class='y8'>&#160;&#160;Lower Cutoff Frequency&#160;") +
							QVariant(LowerCutoffFrequency).toString() +
							QString("&#160;MHz</span><br />");
					}
					if (DicomUtils::get_fd_value(
							nds,
							tFractionalBandwidth,
							&FractionalBandwidth))
					{
						s2 += QString("<span class='y8'>&#160;&#160;Fractional Bandwidth&#160;") +
							QVariant(FractionalBandwidth).toString() +
							QString("&#160;MHz</span><br />");
					}
					if (DicomUtils::get_fd_value(
							nds,
							tCenterFrequency,
							&CenterFrequency))
					{
						s2 += QString("<span class='y8'>&#160;&#160;Center Frequency&#160;") +
							QVariant(CenterFrequency).toString() +
							QString("&#160;MHz</span><br />");
					}
					if (!s2.isEmpty())
					{
						s2.prepend(QString("<span class='y9'>Transducer Response</span><br />"));
						s1 += s2;
					}
				}
			}
		}
		if (!s1.isEmpty())
		{
			s += QString("<span class='y7'>Photoacoustic Transducer Module</span><br />") + s1;
		}
	}
	//
	//
	//////////////////////////////////////////////

	//////////////////////////////////////////////
	//
	// Photoacoustic Reconstruction Module
	//
	//
	{
		QString s3;
		const mdcm::Tag tSoundSpeedCorrectionMechanismCodeSequence(0x0018,0x9832);
		const mdcm::Tag tObjectSoundSpeed(0x0018,0x9833);
		const mdcm::Tag tCouplingMediumSoundSpeed(0x0018,0x9834);
		const mdcm::Tag tCodeMeaning(0x0008,0x0104);
		{
			const mdcm::DataElement & e = ds.GetDataElement(tSoundSpeedCorrectionMechanismCodeSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq)
			{
				const unsigned int n = sq->GetNumberOfItems();
				if (n == 1)
				{
					QString s4;
					const mdcm::Item & item = sq->GetItem(1);
					const mdcm::DataSet & nds = item.GetNestedDataSet();
					double ObjectSoundSpeed;
					double CouplingMediumSoundSpeed;
					QString CodeMeaning;
					if (DicomUtils::get_string_value(
							nds,
							tCodeMeaning,
							CodeMeaning))
					{
						s4 += QString("<span class='y8'>&#160;&#160;") +
							CodeMeaning +
							QString("</span><br />");
					}
					if (DicomUtils::get_fd_value(
							nds,
							tObjectSoundSpeed,
							&ObjectSoundSpeed))
					{
						s4 += QString("<span class='y8'>&#160;&#160;Object Sound Speed&#160;") +
							QVariant(ObjectSoundSpeed).toString() +
							QString("&#160;m/s</span><br />");
					}
					if (DicomUtils::get_fd_value(
							nds,
							tCouplingMediumSoundSpeed,
							&CouplingMediumSoundSpeed))
					{
						s4 += QString("<span class='y8'>&#160;&#160;Coupling Medium Sound Speed&#160;") +
							QVariant(CouplingMediumSoundSpeed).toString() +
							QString("&#160;m/s</span><br />");
					}
					if (!s4.isEmpty())
					{
						s4.prepend(QString("<span class='y9'>Sound Speed Correction Mechanism</span><br />"));
						s3 += s4;
					}
				}
			}
		}
		if (!s3.isEmpty())
		{
			s += QString("<span class='y7'>Photoacoustic Reconstruction Module</span><br />") + s3;
		}
	}
	//
	//
	//////////////////////////////////////////////

	if (!s.isEmpty())
	{
		s.append(QString("<br />"));
	}
	return s;
}

template <typename T, long long TVR>
bool get_vm1_bin_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	T * result)
{
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() || v.IsUndefinedLength() || !v.GetByteValue())
		return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (tvr_ != static_cast<long long>(vr) &&
		vr != mdcm::VR::UN &&
		vr != mdcm::VR::INVALID)
	{
		std::cout << "unexpected VR " << vr << std::endl;
	}
#endif
	if (v.GetByteValue()->GetLength() != sizeof(T))
		return false;
	mdcm::Element<TVR, mdcm::VM::VM1> e;
	e.SetFromDataElement(v);
	*result = static_cast<T>(e.GetValue());
	return true;
}

template <typename T, long long TVR>
bool get_priv_vm1_bin_value(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	T * result)
{
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() || v.IsUndefinedLength() || !v.GetByteValue())
		return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (tvr_ != static_cast<long long>(vr) &&
		vr != mdcm::VR::UN &&
		vr != mdcm::VR::INVALID)
	{
		std::cout << "unexpected VR " << vr << std::endl;
	}
#endif
	if (v.GetByteValue()->GetLength() != sizeof(T))
		return false;
	mdcm::Element<TVR, mdcm::VM::VM1> e;
	e.SetFromDataElement(v);
	*result = static_cast<T>(e.GetValue());
	return true;
}

template <typename T, long long TVR>
bool get_vm1_n_bin_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<T> & result)
{
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() || v.IsUndefinedLength() || !v.GetByteValue())
		return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (tvr_ != static_cast<long long>(vr) &&
		vr != mdcm::VR::UN &&
		vr != mdcm::VR::INVALID)
	{
		std::cout << "unexpected VR " << vr << std::endl;
	}
#endif
	if ((v.GetByteValue()->GetLength() < sizeof(T)) ||
		((v.GetByteValue()->GetLength() % sizeof(T)) != 0))
		return false;
	mdcm::Element<TVR, mdcm::VM::VM1_n> e;
	e.SetFromDataElement(v);
	const unsigned int l = e.GetLength();
	if (l < 1) return false;
	for (unsigned int x = 0; x < l; ++x)
	{
		result.push_back(static_cast<T>(e.GetValue(x)));
	}
	return true;
}

template <typename T, long long TVR>
bool get_priv_vm1_n_bin_values(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	std::vector<T> & result)
{
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() || v.IsUndefinedLength() || !v.GetByteValue())
		return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (tvr_ != static_cast<long long>(vr) &&
		vr != mdcm::VR::UN &&
		vr != mdcm::VR::INVALID)
	{
		std::cout << "unexpected VR " << vr << std::endl;
	}
#endif
	if ((v.GetByteValue()->GetLength() < sizeof(T)) ||
		((v.GetByteValue()->GetLength() % sizeof(T)) != 0))
		return false;
	mdcm::Element<TVR, mdcm::VM::VM1_n> e;
	e.SetFromDataElement(v);
	const unsigned int l = e.GetLength();
	if (l < 1) return false;
	for (unsigned int x = 0; x < l; ++x)
	{
		result.push_back(static_cast<T>(e.GetValue(x)));
	}
	return true;
}

#if 1 // no example file
void delta_decode_rgb(
	const unsigned char * data_in,
	size_t data_size,
	std::vector<unsigned char> & new_stream,
	unsigned short pc,
	size_t w,
	size_t h)
{
	enum
	{
		COLORMODE = 0x81,
		ESCMODE = 0x82,
		REPEATMODE = 0x83
	};
	const size_t plane_size = h * w;
	const size_t outputlen = 3 * plane_size;
	new_stream.resize(outputlen);
	if (data_size == outputlen) return;
	const unsigned char * src  = data_in;
	unsigned char * dest = new_stream.data();
	union
	{
		unsigned char gray;
		unsigned char rgb[3];
	} pixel;
	pixel.rgb[0] = pixel.rgb[1] = pixel.rgb[2] = 0;
	// Start in grayscale mode
	bool graymode{true};
	size_t dx{1};
	size_t dy{3};
	// Algorithm works with both planar configurations.
	// It does produce surprising greenish background color for
	// planar configuration is 0, while the nested Icon SQ display
	// a nice black background.
	if (pc)
	{
		dx = plane_size;
		dy = 1;
	}
	size_t ps = plane_size;
	// Need to switch from one algorithm to other (RGB <-> GRAY).
	while (ps)
	{
		// Next byte
		unsigned char b = *src++;
		assert(src < data_in + data_size);
		// Mode selection
		switch (b)
		{
		case ESCMODE:
			// Used to treat a byte 81/82/83 as a normal byte
			if (graymode)
			{
				pixel.gray += *src++;
				dest[0*dx] = pixel.gray;
				dest[1*dx] = pixel.gray;
				dest[2*dx] = pixel.gray;
			}
			else
			{
				pixel.rgb[0] += *src++;
				pixel.rgb[1] += *src++;
				pixel.rgb[2] += *src++;
				dest[0*dx] = pixel.rgb[0];
				dest[1*dx] = pixel.rgb[1];
				dest[2*dx] = pixel.rgb[2];
			}
			dest += dy;
			--ps;
			break;
		case REPEATMODE:
			// Repeat mode (RLE)
			b = *src++;
			ps -= b;
			if (graymode)
			{
				while (b-- > 0)
				{
					dest[0*dx] = pixel.gray;
					dest[1*dx] = pixel.gray;
					dest[2*dx] = pixel.gray;
					dest += dy;
				}
			}
			else
			{
				while (b-- > 0)
				{
					dest[0*dx] = pixel.rgb[0];
					dest[1*dx] = pixel.rgb[1];
					dest[2*dx] = pixel.rgb[2];
					dest += dy;
				}
			}
			break;
		case COLORMODE:
			// We are swithing from one mode to the other. The stream
			// contains an intermixed compression of RGB codec and GRAY
			// codec. Each one not knowing of the other reset old value to 0.
			if (graymode)
			{
				graymode = false;
				pixel.rgb[0] = pixel.rgb[1] = pixel.rgb[2] = 0;
			}
			else
			{
				graymode = true;
				pixel.gray = 0;
			}
			break;
		default:
			// This is identical to ESCMODE,
			// it would be nicer to use fall-through
			if (graymode)
			{
				pixel.gray += b;
				dest[0*dx] = pixel.gray;
				dest[1*dx] = pixel.gray;
				dest[2*dx] = pixel.gray;
			}
			else
			{
				pixel.rgb[0] += b;
				pixel.rgb[1] += *src++;
				pixel.rgb[2] += *src++;
				dest[0*dx] = pixel.rgb[0];
				dest[1*dx] = pixel.rgb[1];
				dest[2*dx] = pixel.rgb[2];
			}
			dest += dy;
			--ps;
			break;
		}
	}
}
#endif

void delta_decode(
	const char * inbuffer,
	size_t length,
	std::vector<unsigned short> & output)
{
	// RLE pass
	std::vector<signed char> temp;
	for (size_t i = 0; i < length; ++i)
	{
		if (static_cast<unsigned char>(inbuffer[i]) == 0xa5)
		{
			int repeat = static_cast<unsigned char>(inbuffer[i + 1]) + 1;
			const signed char value = static_cast<signed char>(inbuffer[i + 2]);
			while (repeat > 0)
			{
				temp.push_back(value);
				--repeat;
			}
			i += 2;
		}
		else
		{
			temp.push_back(static_cast<unsigned char>(inbuffer[i]));
		}
	}
	// Delta encoding pass
	unsigned short delta{};
	for (size_t i = 0; i < temp.size(); ++i)
	{
		if (static_cast<unsigned char>(temp[i]) == 0x5a)
		{
			const unsigned char v1 = static_cast<unsigned char>(temp[i + 1]);
			const unsigned char v2 = static_cast<unsigned char>(temp[i + 2]);
			const unsigned short value = static_cast<unsigned short>(v2 * 256 + v1);
			output.push_back(value);
			delta = value;
			i += 2;
		}
		else
		{
			const unsigned short value = static_cast<unsigned short>(temp[i] + delta);
			output.push_back(value);
			delta = value;
		}
	}
	if (output.size() % 2 != 0)
	{
		output.resize(output.size() - 1);
	}
}

template <typename T> QString supp_palette_grey_to_rgbUS_(
	RGBImageTypeUS::Pointer & out_image,
	const typename T::Pointer & image,
	const RGBImageTypeUS::Pointer & color_image,
	const int red_subscript,
	const ImageVariant * v)
{
	if (!v) return QString("Image is null");
	if (image.IsNull()) return QString("Image is null");
	if (out_image.IsNull()) return QString("Out image is null");
	if (!(red_subscript > INT_MIN))
		return QString("Internal error, Subscript <= INT_MIN");
	try
	{
		out_image->SetRegions(
			static_cast<RGBImageTypeUS::RegionType>(
				image->GetLargestPossibleRegion()));
		out_image->SetOrigin(
			static_cast<RGBImageTypeUS::PointType>(
				image->GetOrigin()));
		out_image->SetSpacing(
			static_cast<RGBImageTypeUS::SpacingType>(
				image->GetSpacing()));
		out_image->SetDirection(
			static_cast<RGBImageTypeUS::DirectionType>(
				image->GetDirection()));
		out_image->Allocate();
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	//
	//
	const double wmin  = v->di->us_window_center - v->di->us_window_width * 0.5;
	const double wmax  = v->di->us_window_center + v->di->us_window_width * 0.5;
	const double diff_ = (wmax - wmin);
	const double div_  = (diff_ != 0.0) ? diff_ : 1.0;
	try
	{
		itk::ImageRegionConstIterator<T> it1(image, image->GetLargestPossibleRegion());
		it1.GoToBegin();
		itk::ImageRegionIterator<RGBImageTypeUS> it0(out_image, out_image->GetLargestPossibleRegion());
		it0.GoToBegin();
		itk::ImageRegionConstIterator<RGBImageTypeUS> it2(color_image, color_image->GetLargestPossibleRegion());
		it2.GoToBegin();
		while (!(it1.IsAtEnd()||it0.IsAtEnd()||it2.IsAtEnd()))
		{
			const RGBPixelUS & pixel = it2.Get();
			if (pixel.GetRed() > 0 || pixel.GetGreen() > 0 || pixel.GetBlue() > 0)
			{
				it0.Set(pixel);
			}
			else
			{
				const double k = static_cast<double>(it1.Get());
				unsigned short c{};
				if (k < static_cast<double>(red_subscript))
				{
					const double r = (k + (-wmin)) / div_;
					if ((k >= wmin) && (k <= wmax))
					{
						c = static_cast<unsigned short>(USHRT_MAX * r);
					}
					else if (k>wmax)
					{
						c = static_cast<unsigned short>(USHRT_MAX);
					}
				}
				RGBPixelUS pixel0;
				pixel0.SetRed  (c);
				pixel0.SetGreen(c);
				pixel0.SetBlue (c);
				it0.Set(pixel0);
			}
			++it1;
			++it0;
			++it2;
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	return QString("");
}

template <typename T> QString supp_palette_grey_to_rgbUC_(
	RGBImageTypeUC::Pointer & out_image,
	const typename T::Pointer & image,
	const RGBImageTypeUC::Pointer & color_image,
	const int red_subscript,
	const ImageVariant * v)
{
	if (!v) return QString("Image is null");
	if (image.IsNull()) return QString("Image is null");
	if (out_image.IsNull()) return QString("Out image is null");
	if (!(red_subscript > INT_MIN))
		return QString("Internal error, Subscript <= INT_MIN");
	try
	{
		out_image->SetRegions(
			static_cast<RGBImageTypeUC::RegionType>(
				image->GetLargestPossibleRegion()));
		out_image->SetOrigin(
			static_cast<RGBImageTypeUC::PointType>(
				image->GetOrigin()));
		out_image->SetSpacing(
			static_cast<RGBImageTypeUC::SpacingType>(
				image->GetSpacing()));
		out_image->SetDirection(
			static_cast<RGBImageTypeUC::DirectionType>(
				image->GetDirection()));
		out_image->Allocate();
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	//
	const double wmin  = v->di->us_window_center - v->di->us_window_width * 0.5;
	const double wmax  = v->di->us_window_center + v->di->us_window_width * 0.5;
	const double diff_ = (wmax - wmin);
	const double div_  = (diff_ != 0.0) ? diff_ : 1.0;
	try
	{
		itk::ImageRegionConstIterator<T> it1(image, image->GetLargestPossibleRegion());
		it1.GoToBegin();
		itk::ImageRegionIterator<RGBImageTypeUC> it0(out_image, out_image->GetLargestPossibleRegion());
		it0.GoToBegin();
		itk::ImageRegionConstIterator<RGBImageTypeUC> it2(color_image, color_image->GetLargestPossibleRegion());
		it2.GoToBegin();
		while (!(it1.IsAtEnd() || it0.IsAtEnd() || it2.IsAtEnd()))
		{
			//
			const RGBPixelUC & pixel = it2.Get();
			if (pixel.GetRed() > 0 || pixel.GetGreen() > 0 || pixel.GetBlue() > 0)
			{
				it0.Set(pixel);
			}
			else
			{
				const double k = static_cast<double>(it1.Get());
				unsigned char c{};
				if (k < static_cast<double>(red_subscript))
				{
					const double r = (k + (-wmin)) / div_;
					if ((k >= wmin) && (k <= wmax))
					{
						c = static_cast<unsigned char>(UCHAR_MAX * r);
					}
					else if (k>wmax)
					{
						c = static_cast<unsigned char>(UCHAR_MAX);
					}
				}
				RGBPixelUC pixel0;
				pixel0.SetRed(c);
				pixel0.SetGreen(c);
				pixel0.SetBlue(c);
				it0.Set(pixel0);
			}
			++it1;
			++it0;
			++it2;
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	return QString("");
}

QString supp_palette_grey_to_rgbUS(
	RGBImageTypeUS::Pointer & out_image,
	const RGBImageTypeUS::Pointer & color_image,
	const int red_subscript,
	const ImageVariant * v)
{
	QString result;
	switch(v->image_type)
	{
	case 0:
		result = supp_palette_grey_to_rgbUS_<ImageTypeSS>(out_image, v->pSS, color_image, red_subscript, v);
		break;
	case 1:
		result = supp_palette_grey_to_rgbUS_<ImageTypeUS>(out_image, v->pUS, color_image, red_subscript, v);
		break;
	case 2:
		result = supp_palette_grey_to_rgbUS_<ImageTypeSI>(out_image, v->pSI, color_image, red_subscript, v);
		break;
	case 3:
		result = supp_palette_grey_to_rgbUS_<ImageTypeUI>(out_image, v->pUI, color_image, red_subscript, v);
		break;
	case 4:
		result = supp_palette_grey_to_rgbUS_<ImageTypeUC>(out_image, v->pUC, color_image, red_subscript, v);
		break;
	case 5:
		result = supp_palette_grey_to_rgbUS_<ImageTypeF>(out_image, v->pF, color_image, red_subscript, v);
		break;
	case 6:
		result = supp_palette_grey_to_rgbUS_<ImageTypeD>(out_image, v->pD, color_image, red_subscript, v);
		break;
	case 7:
		result = supp_palette_grey_to_rgbUS_<ImageTypeSLL>(out_image, v->pSLL, color_image, red_subscript, v);
		break;
	case 8:
		result = supp_palette_grey_to_rgbUS_<ImageTypeULL>(out_image, v->pULL, color_image, red_subscript, v);
		break;
	default:
		result = QString("Wrong image type");
		break;
	}
	return result;
}

QString supp_palette_grey_to_rgbUC(
	RGBImageTypeUC::Pointer & out_image,
	const RGBImageTypeUC::Pointer & color_image,
	const int red_subscript,
	const ImageVariant * v)
{
	QString result;
	switch(v->image_type)
	{
	case 0:
		result = supp_palette_grey_to_rgbUC_<ImageTypeSS>(out_image, v->pSS, color_image, red_subscript, v);
		break;
	case 1:
		result = supp_palette_grey_to_rgbUC_<ImageTypeUS>(out_image, v->pUS, color_image, red_subscript, v);
		break;
	case 2:
		result = supp_palette_grey_to_rgbUC_<ImageTypeSI>(out_image, v->pSI, color_image, red_subscript, v);
		break;
	case 3:
		result = supp_palette_grey_to_rgbUC_<ImageTypeUI>(out_image, v->pUI, color_image, red_subscript, v);
		break;
	case 4:
		result = supp_palette_grey_to_rgbUC_<ImageTypeUC>(out_image, v->pUC, color_image, red_subscript, v);
		break;
	case 5:
		result = supp_palette_grey_to_rgbUC_<ImageTypeF>(out_image, v->pF, color_image, red_subscript, v);
		break;
	case 6:
		result = supp_palette_grey_to_rgbUC_<ImageTypeD>(out_image, v->pD, color_image, red_subscript, v);
		break;
	case 7:
		result = supp_palette_grey_to_rgbUC_<ImageTypeSLL>(out_image, v->pSLL, color_image, red_subscript, v);
		break;
	case 8:
		result = supp_palette_grey_to_rgbUC_<ImageTypeULL>(out_image, v->pULL, color_image, red_subscript, v);
		break;
	default:
		result = QString("Wrong image type");
		break;
	}
	return result;
}

unsigned int process_gsps(
	const QStringList & grey_softcopy_pr_files,
	const QString & p,
	const QWidget * settings,
	const bool ok3d,
	std::vector<ImageVariant*> & ivariants,
	QString & message_)
{
	unsigned int count{};
	const SettingsWidget * wsettings = static_cast<const SettingsWidget *>(settings);
	for (int x = 0; x < grey_softcopy_pr_files.size(); ++x)
	{
		QList<PrRefSeries> refs;
		DicomUtils::read_pr_ref(p, grey_softcopy_pr_files.at(x), refs);
		for (int y = 0; y < refs.size(); ++y)
		{
			QStringList ref_files;
			for (int z = 0; z < refs.at(y).images.size(); ++z)
			{
				ref_files.push_back(refs.at(y).images.at(z).file);
			}
			if (ref_files.size() < 1) continue;
			ref_files.sort();
			std::vector<ImageVariant*> ref_ivariants;
			QStringList dummy;
			const QString message_pr_ref =
				DicomUtils::read_dicom(
					ref_ivariants,
					dummy,
					dummy,
					dummy,
					dummy,
					dummy,
					QString(""),
					ref_files,
					false,
					settings,
					1,
					true);
			for (unsigned int z = 0; z < ref_ivariants.size(); ++z)
			{
				const int ref_ivariant_type = ref_ivariants.at(z)->image_type;
				if (!(ref_ivariant_type >= 0 && ref_ivariant_type < 10))
				{
#ifdef ALIZA_VERBOSE
					std::cout << "Not a scalar image for GSPS, skipped" << std::endl;
#endif
					continue;
				}
				++count;
				bool spatial_transform{};
				ImageVariant * pr_image =
					PrConfigUtils::make_pr_monochrome(
						ref_ivariants.at(z),
						refs.at(y),
						wsettings,
						ok3d,
						&spatial_transform);
				if (pr_image)
				{
					const bool pr_skip_texture = pr_image->di->skip_texture;
					//
					if (ref_ivariants.at(z)->di->slices_generated)
					{
						CommonUtils::copy_slices(
							pr_image,
							ref_ivariants.at(z));
					}
					else
					{
						CommonUtils::copy_essential(
							pr_image,
							ref_ivariants.at(z));
					}
					CommonUtils::copy_imagevariant_overlays(
						pr_image,
						ref_ivariants.at(z));
					//
					//
					//
					pr_image->di->skip_texture = pr_skip_texture;
					pr_image->iod = QString("Grayscale Softcopy Presentation State");
					pr_image->sop = QString("");
					pr_image->rescale_disabled = false;
					pr_image->filenames = QStringList(grey_softcopy_pr_files.at(x));
					//
					//
					//
					if (spatial_transform)
					{
						pr_image->equi = false;
						pr_image->di->hide_orientation = true;
						pr_image->di->filtering = 0;
					}
					bool pr_load_ok{};
					pr_load_ok = CommonUtils::reload_monochrome(
						pr_image,
						ok3d,
						nullptr,
						0,
						wsettings->get_resize(),
						wsettings->get_size_x(),
						wsettings->get_size_y());
					if (pr_load_ok)
					{
						if (pr_image->equi)
						{
							if (pr_image->di->idimz < 7)
								pr_image->di->transparency = false;
						}
						else
						{
							if (!pr_image->one_direction)
								pr_image->di->transparency = false;
							pr_image->di->filtering = 0;
						}
						CommonUtils::reset_bb(pr_image);
						ivariants.push_back(pr_image);
					}
					else
					{
						delete pr_image;
						pr_image = nullptr;
					}
				}
				if (ref_ivariants.at(z))
				{
					delete ref_ivariants[z];
					ref_ivariants[z] = nullptr;
				}
			}
			if (!message_pr_ref.isEmpty())
				message_.append(QChar('\n') + message_pr_ref);
		}
	}
	return count;
}

static std::atomic<short> force_suppllut(0);

}

static cmsUInt32Number cms_error = 0;
extern "C"
{
	static void AlizaLCMS2LogErrorHandler(cmsContext id, cmsUInt32Number e, const char * t)
	{
		cms_error = e;
		printf("lcms2 error: %s\n", t);
		(void)id;
	}
}

QString DicomUtils::convert_pn_value(const QString & n)
{
	// family name, given name, middle name, name prefix, name suffix
	QString s;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList tmp1 = n.split(QString("="), Qt::KeepEmptyParts);
#else
	const QStringList tmp1 = n.split(QString("="), QString::KeepEmptyParts);
#endif
	for (int x = 0; x < tmp1.size(); ++x)
	{
		if (!s.isEmpty()) s += QString(" ");
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		const QStringList tmp2 = tmp1.at(x).split(QString("^"), Qt::KeepEmptyParts);
#else
		const QStringList tmp2 = tmp1.at(x).split(QString("^"), QString::KeepEmptyParts);
#endif
		const int tmp2_size = tmp2.size();
		if (tmp2_size == 1)
		{
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 2)
		{
			const QString s1 = tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" ");
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 3)
		{
			const QString s1 = tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" ");
			const QString s2 = tmp2.at(2).trimmed().remove(QChar('\0'));
			s += s2;
			if (!s2.isEmpty()) s += QString(" ");
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 4)
		{
			const QString s3 = tmp2.at(3).trimmed().remove(QChar('\0'));
			s += s3;
			if (!s3.isEmpty()) s += QString(" ");
			const QString s1 = tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" ");
			const QString s2 = tmp2.at(2).trimmed().remove(QChar('\0'));
			s += s2;
			if (!s2.isEmpty()) s += QString(" ");
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 5)
		{
			const QString s3 = tmp2.at(3).trimmed().remove(QChar('\0'));
			s += s3;
			if (!s3.isEmpty()) s += QString(" ");
			const QString s1 = tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" ");
			const QString s2 = tmp2.at(2).trimmed().remove(QChar('\0'));
			s += s2;
			if (!s2.isEmpty()) s += QString(" ");
			const QString s0 = tmp2.at(0).trimmed().remove(QChar('\0'));
			s += s0;
			const QString s4 = tmp2.at(4).trimmed().remove(QChar('\0'));
			if (!s4.isEmpty()) s += QString(", ") + s4;
		}
		else // error
		{
			s += tmp1.at(x);
		}
	}
	return s;
}

QString DicomUtils::get_pn_value2(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const char * charset)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty() || e.IsUndefinedLength())
		return QString("");
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return QString("");
	const QByteArray ba(bv->GetPointer(), bv->GetLength());
	const QString tmp0 = CodecUtils::toUTF8(&ba, charset);
	return convert_pn_value(tmp0);
}

bool DicomUtils::get_us_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	unsigned short * result)
{
	const bool ok = get_vm1_bin_value<unsigned short, mdcm::VR::US>(ds, t, result);
	return ok;
}

bool DicomUtils::get_ss_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	signed short * result)
{
	const bool ok = get_vm1_bin_value<signed short, mdcm::VR::SS>(ds, t, result);
	return ok;
}

bool DicomUtils::get_sl_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	int * result)
{
	const bool ok = get_vm1_bin_value<int, mdcm::VR::SL>(ds, t, result);
	return ok;
}

bool DicomUtils::get_ul_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	unsigned int * result)
{
	const bool ok = get_vm1_bin_value<unsigned int, mdcm::VR::UL>(ds, t, result);
	return ok;
}

bool DicomUtils::get_fd_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	double * result)
{
	const bool ok = get_vm1_bin_value<double, mdcm::VR::FD>(ds, t, result);
	return ok;
}

bool DicomUtils::priv_get_fd_value(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	double * result)
{
	const bool ok = get_priv_vm1_bin_value<double, mdcm::VR::FD>(ds, t, result);
	return ok;
}

bool DicomUtils::get_fl_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	float * result)
{
	const bool ok = get_vm1_bin_value<float, mdcm::VR::FL>(ds, t, result);
	return ok;
}

bool DicomUtils::priv_get_fl_value(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	float * result)
{
	const bool ok = get_priv_vm1_bin_value<float, mdcm::VR::FL>(ds, t, result);
	return ok;
}

bool DicomUtils::get_us_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<unsigned short> & result)
{
	const bool ok = get_vm1_n_bin_values<unsigned short, mdcm::VR::US>(ds, t, result);
	return ok;
}

bool DicomUtils::get_ss_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<signed short> & result)
{
	const bool ok = get_vm1_n_bin_values<signed short, mdcm::VR::SS>(ds, t, result);
	return ok;
}

bool DicomUtils::get_sl_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<int> & result)
{
	const bool ok = get_vm1_n_bin_values<int, mdcm::VR::SL>(ds, t, result);
	return ok;
}

bool DicomUtils::get_ul_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<unsigned int> & result)
{
	const bool ok = get_vm1_n_bin_values<unsigned int, mdcm::VR::UL>(ds, t, result);
	return ok;
}

bool DicomUtils::get_fd_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<double> & result)
{
	const bool ok = get_vm1_n_bin_values<double, mdcm::VR::FD>(ds, t, result);
	return ok;
}

bool DicomUtils::priv_get_fd_values(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	std::vector<double> & result)
{
	const bool ok = get_priv_vm1_n_bin_values<double, mdcm::VR::FD>(ds, t, result);
	return ok;
}

bool DicomUtils::get_fl_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<float> & result)
{
	const bool ok = get_vm1_n_bin_values<float, mdcm::VR::FL>(ds, t, result);
	return ok;
}

bool DicomUtils::get_ds_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<double> & result)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	QString tmp0 = QString::fromLatin1(bv->GetPointer(), bv->GetLength());
	if (tmp0.contains(QString(",")))
	{
		// Workaround invalid VR
		tmp0.replace(QString(","), QString("."));
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList tmp1 = tmp0.split(QString("\\"), Qt::SkipEmptyParts);
#else
	const QStringList tmp1 = tmp0.split(QString("\\"), QString::SkipEmptyParts);
#endif
	if (tmp1.empty()) return false;
	for (int x = 0; x < tmp1.size(); ++x)
	{
		bool ok{};
		const double tmp3 = QVariant(tmp1.at(x).trimmed().remove(QChar('\0'))).toDouble(&ok);
		if (!ok) return false;
		result.push_back(tmp3);
	}
	return true;
}

bool DicomUtils::priv_get_ds_values(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	std::vector<double> & result)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	QString tmp0 = QString::fromLatin1(bv->GetPointer(), bv->GetLength());
	if (tmp0.contains(QString(",")))
	{
		// Workaround invalid VR
		tmp0.replace(QString(","), QString("."));
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList tmp1 = tmp0.split(QString("\\"), Qt::SkipEmptyParts);
#else
	const QStringList tmp1 = tmp0.split(QString("\\"), QString::SkipEmptyParts);
#endif
	if (tmp1.empty()) return false;
	for (int x = 0; x < tmp1.size(); ++x)
	{
		bool ok{};
		const double tmp3 = QVariant(tmp1.at(x).trimmed().remove(QChar('\0'))).toDouble(&ok);
		if (!ok) return false;
		result.push_back(tmp3);
	}
	return true;
}

bool DicomUtils::get_is_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	int * result)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty() || e.IsUndefinedLength() || !e.GetByteValue())
		return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (bv)
	{
		const QString tmp0 = QString::fromLatin1(
			bv->GetPointer(),
			bv->GetLength()).trimmed().remove(QChar('\0'));
		bool ok{};
		const int tmp1 = QVariant(tmp0).toInt(&ok);
		if (ok)
		{
			*result = tmp1;
			return true;
		}
	}
	return false;
}

bool DicomUtils::get_is_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<int> & result)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	const QString tmp0 = QString::fromLatin1(bv->GetPointer(), bv->GetLength());
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList tmp1 = tmp0.split(QString("\\"), Qt::SkipEmptyParts);
#else
	const QStringList tmp1 = tmp0.split(QString("\\"), QString::SkipEmptyParts);
#endif
	if (tmp1.empty()) return false;
	for (int x = 0; x < tmp1.size(); ++x)
	{
		bool ok{};
		const int tmp3 = QVariant(tmp1.at(x).trimmed().remove(QChar('\0'))).toInt(&ok);
		if (!ok) return false;
		result.push_back(tmp3);
	}
	return true;
}

bool DicomUtils::get_at_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	unsigned short * group,
	unsigned short * element)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	if (4 == bv->GetLength())
	{
		char * buffer = new char[4];
		const bool ok = bv->GetBuffer(buffer, 4);
		if (ok)
		{
			char g_[2];
			char e_[2];
			g_[0] = buffer[0];
			g_[1] = buffer[1];
			e_[0] = buffer[2];
			e_[1] = buffer[3];
			memcpy(group,   g_, 2);
			memcpy(element, e_, 2);
		}
		delete [] buffer;
		return ok;
	}
	return false;
}

bool DicomUtils::get_string_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	QString & s)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty() || e.IsUndefinedLength())
		return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	s = QString::fromLatin1(bv->GetPointer(), bv->GetLength()).trimmed();
	return true;
}

bool DicomUtils::priv_get_string_value(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	QString & s)
{
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty() || e.IsUndefinedLength())
		return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	s = QString::fromLatin1(bv->GetPointer(), bv->GetLength()).trimmed();
	return true;
}

QString DicomUtils::generate_id()
{
	char c[12] = "\0\0\0\0\0\0\0\0\0\0\0";
	const char s[63] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const unsigned long long seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	std::mt19937_64 mtrand(seed);
	for (unsigned int i = 0; i < 11; ++i)
	{
		c[i] = s[mtrand() % 62];
	}
	return QString::fromLatin1(c);
}

bool DicomUtils::is_image(
	const mdcm::DataSet & ds,
	unsigned short * r,
	unsigned short * c,
	unsigned short * ba,
	unsigned short * bs,
	unsigned short * hb,
	short * pr,
	bool  * l)
{
	if (ds.IsEmpty()) return false;
	bool ok = get_us_value(ds, mdcm::Tag(0x0028,0x0100), ba);
	if (!ok) return false;
	ok = get_us_value(ds, mdcm::Tag(0x0028,0x0101), bs);
	ok = get_us_value(ds, mdcm::Tag(0x0028,0x0102), hb);
	unsigned short pixelrepres;
	ok = get_us_value(ds, mdcm::Tag(0x0028,0x0103), &pixelrepres);
	if (ok) *pr = static_cast<short>(pixelrepres);
	ok = get_us_value(ds, mdcm::Tag(0x0028,0x0010), r);
	if (!ok) return false;
	ok = get_us_value(ds, mdcm::Tag(0x0028,0x0011), c);
	if (!ok) return false;
	if (*r <= 0 || *c <= 0) return false;
	QString s;
	ok = get_string_value(ds, mdcm::Tag(0x0008,0x0008), s);
	if (ok && s.toUpper().contains(QString("LOCALIZER"))) *l = true;
	else *l = false;
	return true;
}

bool DicomUtils::has_functional_groups(const mdcm::DataSet & ds)
{
	{
		const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
		const mdcm::DataElement & e = ds.GetDataElement(tSharedFunctionalGroupsSequence);
		if (!e.IsEmpty())
		{
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq && sq->GetNumberOfItems() > 0) return true;
		}
	}
	{
		const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
		const mdcm::DataElement & e = ds.GetDataElement(tPerFrameFunctionalGroupsSequence);
		if (!e.IsEmpty())
		{
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq && sq->GetNumberOfItems() > 0) return true;
		}
	}
	return false;
}

bool DicomUtils::has_supp_palette(const mdcm::DataSet & ds)
{
	const mdcm::Tag tPhotometricInterpretation(0x0028,0x0004);
	const mdcm::Tag tPixelPresentation(0x0008,0x9205);
	const mdcm::Tag tRedPaletteColorLookupTableDescriptor(0x0028,0x1101);
	const mdcm::Tag tGreenPaletteColorLookupTableDescriptor(0x0028,0x1102);
	const mdcm::Tag tBluePaletteColorLookupTableDescriptor(0x0028,0x1103);
	QString PixelPresentation;
	if (get_string_value(ds, tPixelPresentation, PixelPresentation))
	{
		if (PixelPresentation.toUpper().trimmed() == QString("COLOR"))
		{
			QString PhotometricInterpretation;
			if (get_string_value(ds, tPhotometricInterpretation, PhotometricInterpretation))
			{
				if (PhotometricInterpretation.toUpper().trimmed() == QString("MONOCHROME2") &&
					ds.FindDataElement(tRedPaletteColorLookupTableDescriptor) &&
					ds.FindDataElement(tGreenPaletteColorLookupTableDescriptor) &&
					ds.FindDataElement(tBluePaletteColorLookupTableDescriptor))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool DicomUtils::has_modality_lut_sq(const mdcm::DataSet & ds)
{
	const mdcm::DataElement& e = ds.GetDataElement(mdcm::Tag(0x0028,0x3000));
	if (!e.IsEmpty())
	{
		const mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems() == 1)
		{
			const mdcm::Item & item = sq->GetItem(1);
			const mdcm::DataSet & nds = item.GetNestedDataSet();
			const mdcm::DataElement & e1 = nds.GetDataElement(mdcm::Tag(0x0028,0x3006));
			const mdcm::DataElement & e2 = nds.GetDataElement(mdcm::Tag(0x0028,0x3002));
			if (!e1.IsEmpty() && !e2.IsEmpty())
			{
				return true;
			}
		}
	}
	return false;
}

bool DicomUtils::check_encapsulated(const mdcm::DataSet & ds)
{
	const mdcm::Tag t(0x0042,0x0011);
	if (ds.FindDataElement(t)) return true;
	return false;
}

bool DicomUtils::is_multiframe(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return false;
	const mdcm::Tag tnumframes(0x0028,0x0008);
	QString numframes;
	if (get_string_value(ds, tnumframes, numframes))
	{
		const QVariant v(numframes.remove(QChar('\0')));
		bool ok{};
		const int k = v.toInt(&ok);
		if (ok && k > 1) return true;
	}
	return false;
}

void DicomUtils::read_image_info(
	const QString & f,
	unsigned short * rows_,
	unsigned short * columns_,
	QString        & position,
	QString        & orientation,
	QString        & spacing,
	QString        & sop_instance_uid,
	QString        & orientation_20_20)
{
	std::set<mdcm::Tag> tags;
	mdcm::Tag tsopinstance(0x0008,0x0018); tags.insert(tsopinstance);
	// Imager Pixel Spacing
	mdcm::Tag tspacing1(0x0018,0x1164);    tags.insert(tspacing1);
	// Nominal Scanned Pixel Spacing
	mdcm::Tag tspacing2(0x0018,0x2010);    tags.insert(tspacing2);
	mdcm::Tag tpos_old(0x0020,0x0030);     tags.insert(tpos_old);
	mdcm::Tag tpos(0x0020,0x0032);         tags.insert(tpos);
	mdcm::Tag torie_old(0x0020,0x0035);    tags.insert(torie_old);
	mdcm::Tag torie(0x0020,0x0037);        tags.insert(torie);
	mdcm::Tag trows(0x0028,0x0010);        tags.insert(trows);
	mdcm::Tag tcolumns(0x0028,0x0011);     tags.insert(tcolumns);
	// Pixel Spacing
	mdcm::Tag tspacing0(0x0028,0x0030);    tags.insert(tspacing0);
	// Patient Orientation
	mdcm::Tag t2020(0x0020,0x0020);        tags.insert(t2020);
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
	const bool f_ok = reader.ReadSelectedTags(tags);
	if (!f_ok) return;
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	if (ds.IsEmpty()) return;
	//
	const bool b0 = get_us_value(ds, trows, rows_);
	const bool b1 = get_us_value(ds, tcolumns, columns_);
	if (!(b0 && b1))
	{
		;
	}
	//
	{
		QString sop_instance_uid_;
		if (get_string_value(ds, tsopinstance, sop_instance_uid_))
			sop_instance_uid = sop_instance_uid_.remove(QChar('\0'));
	}
	//
	{
		QString position_;
		if (get_string_value(ds, tpos, position_))
			position = position_.remove(QChar('\0'));
		else if (get_string_value(ds, tpos_old, position_))
			position = position_.remove(QChar('\0'));
	}
	//
	{
		QString orientation_;
		if (get_string_value(ds, torie, orientation_))
			orientation = orientation_.remove(QChar('\0'));
		else if (get_string_value(ds, torie_old, orientation_))
			orientation = orientation_.remove(QChar('\0'));
	}
	//
	{
		QString spacing_;
		if (get_string_value(ds, tspacing0, spacing_))
			spacing = spacing_.remove(QChar('\0'));
		else if (get_string_value(ds, tspacing1, spacing_))
			spacing = spacing_.remove(QChar('\0'));
		else if (get_string_value(ds, tspacing2, spacing_))
			spacing = spacing_.remove(QChar('\0'));
	}
	//
	{
		QString o20_20;
		if (get_string_value(ds, t2020, o20_20))
			orientation_20_20 = o20_20.remove(QChar('\0'));
	}
}

void DicomUtils::read_image_info_rtdose(const QString & f,
	unsigned short * num_frames_,
	unsigned short * rows_,
	unsigned short * columns_,
	QString        & position,
	QString        & orientation,
	QString        & spacing,
	std::vector<double> & z_offsets)
{
	std::set<mdcm::Tag> tags;
	mdcm::Tag tframes(0x0028,0x0008);      tags.insert(tframes);
	mdcm::Tag trows(0x0028,0x0010);        tags.insert(trows);
	mdcm::Tag tcolumns(0x0028,0x0011);     tags.insert(tcolumns);
	mdcm::Tag tpos(0x0020,0x0032);         tags.insert(tpos);
	mdcm::Tag torie(0x0020,0x0037);        tags.insert(torie);
	mdcm::Tag tspacing(0x0028,0x0030);     tags.insert(tspacing);
	mdcm::Tag tframeoffset(0x3004,0x000c); tags.insert(tframeoffset);
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
	const bool f_ok = reader.ReadSelectedTags(tags);
	if (!f_ok) return;
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	if (ds.IsEmpty()) return;
	//
	const bool b0 = get_us_value(ds, trows, rows_);
	const bool b1 = get_us_value(ds, tcolumns, columns_);
	const bool b2 = get_ds_values(ds, tframeoffset, z_offsets);
#ifdef ALIZA_VERBOSE
	if (!(b0 && b1 && b2))
	{
		std::cout << "read_image_info_rtdose warning (1)" << std::endl;
	}
#else
	(void)b0;
	(void)b1;
	(void)b2;
#endif
	//
	{
		QString numframes;
		if (get_string_value(ds, tframes, numframes))
		{
			const QVariant v(numframes.remove(QChar('\0')));
			bool ok{};
			const int k = v.toInt(&ok);
			if (ok && k > 0) *num_frames_ = static_cast<unsigned short>(k);
		}
	}
	//
	{
		QString position_;
		if (get_string_value(ds, tpos, position_))
			position = position_.remove(QChar('\0'));
	}
	//
	{
		QString orientation_;
		if (get_string_value(ds, torie, orientation_))
			orientation = orientation_.remove(QChar('\0'));
	}
	//
	{
		QString spacing_;
		if (get_string_value(ds, tspacing, spacing_))
			spacing = spacing_.remove(QChar('\0'));
	}
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
}

//#define TMP_PRINT_LOAD_CONTOUR
void DicomUtils::load_contour(
	const mdcm::DataSet & ds,
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	QString charset;
	const mdcm::Tag tcharset(0x0008,0x0005);
	{
		QString charset_;
		if (get_string_value(ds, tcharset, charset_))
			charset = charset_.remove(QChar('\0'));
	}
	// StructureSetROISequence
	const mdcm::Tag tssroisq(0x3006,0x0020);
	const mdcm::DataElement & ssroisq = ds.GetDataElement(tssroisq);
	if (ssroisq.IsEmpty()) return;
	mdcm::SmartPointer<mdcm::SequenceOfItems> ssqi = ssroisq.GetValueAsSQ();
	if (!(ssqi && ssqi->GetNumberOfItems() > 0)) return;
	// ROIContourSequence
	const mdcm::Tag troicsq(0x3006,0x0039);
	const mdcm::DataElement & roicsq = ds.GetDataElement(troicsq);
	if (roicsq.IsEmpty()) return;
	mdcm::SmartPointer<mdcm::SequenceOfItems> sqi = roicsq.GetValueAsSQ();
	if (!(sqi && sqi->GetNumberOfItems() > 0)) return;
	if (!(ssqi && ssqi->GetNumberOfItems() > 0)) return;
	// RTROIObservationsSequence
	mdcm::SmartPointer<mdcm::SequenceOfItems> obssq = nullptr;
	const mdcm::Tag tobservationssq(0x3006,0x0080);
	const mdcm::DataElement & eobservation = ds.GetDataElement(tobservationssq);
	obssq = eobservation.GetValueAsSQ(); // null is empty
	//
	for (unsigned int pd = 0; pd < sqi->GetNumberOfItems(); ++pd)
	{
		ROI roi;
		const mdcm::Item & item = sqi->GetItem(pd + 1);
		const mdcm::DataSet & nestedds = item.GetNestedDataSet();
		const mdcm::DataElement & de_3006_84 = nestedds.GetDataElement(mdcm::Tag(0x3006,0x0084));
		if (de_3006_84.IsEmpty()) continue;
		// Referenced ROI Number
		mdcm::Attribute<0x3006,0x0084> roinumber;
		roinumber.SetFromDataElement(de_3006_84);
		// Find structure_set_roi_sequence corresponding
		// to roi_contour_sequence (by comparing id numbers)
		unsigned int spd{};
		mdcm::Item sitem;
		mdcm::DataSet snestedds;
		mdcm::Attribute<0x3006,0x0022> sroinumber; // ROI Number
		int roi_number{-1};
		do
		{
			sitem = ssqi->GetItem(spd + 1);
			snestedds = sitem.GetNestedDataSet();
			const mdcm::DataElement & sroinumber_de = snestedds.GetDataElement(mdcm::Tag(0x3006,0x0022));
			if (!sroinumber_de.IsEmpty())
			{
				sroinumber.SetFromDataElement(sroinumber_de);
				roi_number = sroinumber.GetValue();
			}
			++spd;
			if (spd >= ssqi->GetNumberOfItems()) break;
		} while (sroinumber.GetValue() != roinumber.GetValue());
		if (roi_number < 0) continue;
		roi.id = roi_number;
#ifdef TMP_PRINT_LOAD_CONTOUR
		std::cout << "0x3006,0x0022 ROI Number " << roi_number << std::endl;
#endif
		if (obssq && obssq->GetNumberOfItems() > 0)
		{
			const mdcm::Tag tinterpretedtype(0x3006,0x00a4);
			for (unsigned int obsidx = 0;
				obsidx < obssq->GetNumberOfItems();
				++obsidx)
			{
				const mdcm::Item & obsitem = obssq->GetItem(obsidx + 1);
				const mdcm::DataSet & obsnestedds = obsitem.GetNestedDataSet();
				const mdcm::DataElement & roinumber_de = obsnestedds.GetDataElement(mdcm::Tag(0x3006,0x0084));
				if (!roinumber_de.IsEmpty())
				{
					roinumber.SetFromDataElement(obsnestedds.GetDataElement(roinumber.GetTag()));
					if (roinumber.GetValue() == roi_number)
					{
						const mdcm::DataElement & einterpretedtype = obsnestedds.GetDataElement(tinterpretedtype);
						if (!einterpretedtype.IsEmpty() && !einterpretedtype.IsUndefinedLength() &&
							einterpretedtype.GetByteValue())
						{
							roi.interpreted_type = QString::fromLatin1(
								einterpretedtype.GetByteValue()->GetPointer(),
								einterpretedtype.GetByteValue()->GetLength()).trimmed();
						}
						break;
					}
				}
			}
		}
		// Referenced Frame of Reference UID
		{
			const mdcm::Tag trefframeofref(0x3006,0x0024);
			const mdcm::DataElement & erefframeofref = snestedds.GetDataElement(trefframeofref);
			if (!erefframeofref.IsEmpty() && !erefframeofref.IsUndefinedLength() &&
				erefframeofref.GetByteValue())
			{
				roi.ref_frame_of_ref =
					QString::fromLatin1(
						erefframeofref.GetByteValue()->GetPointer(),
						erefframeofref.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
#ifdef TMP_PRINT_LOAD_CONTOUR
				std::cout << "0x3006,0x0024 Referenced Frame of Reference " <<
					roi.ref_frame_of_ref.toStdString() << std::endl;
#endif
			}
		}
		// ROIName
		{
			const mdcm::Tag stcsq(0x3006,0x0026);
			const mdcm::DataElement & sde = snestedds.GetDataElement(stcsq);
			if (!sde.IsEmpty() && !sde.IsUndefinedLength() && sde.GetByteValue())
			{
				QByteArray ba(
					sde.GetByteValue()->GetPointer(),
					sde.GetByteValue()->GetLength());
				const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
				roi.name = tmp0.trimmed().remove(QChar('\0'));
#ifdef TMP_PRINT_LOAD_CONTOUR
				std::cout << "0x3006,0x0026 ROIName " << tmp0.toStdString() << std::endl;
#endif
			}
		}
		int color_r{255};
		int color_g{255};
		int color_b{255};
		// ROI Display Color
		const mdcm::Tag troidc(0x3006, 0x002a);
		mdcm::Attribute<0x3006, 0x002a> color = {};
		{
			const mdcm::DataElement & decolor = nestedds.GetDataElement(troidc);
			if (!decolor.IsEmpty())
			{
				color.SetFromDataElement(decolor);
				const int * color_p = color.GetValues();
				if (color.GetNumberOfValues() == 3)
				{
					color_r = color_p[0];
					color_g = color_p[1];
					color_b = color_p[2];
				}
			}
		}
		roi.color.r = color_r / 255.0f;
		roi.color.g = color_g / 255.0f;
		roi.color.b = color_b / 255.0f;
		// ContourSequence
		const mdcm::Tag tcsq(0x3006, 0x0040);
		const mdcm::DataElement & csq = nestedds.GetDataElement(tcsq);
		if (csq.IsEmpty()) continue;
		mdcm::SmartPointer<mdcm::SequenceOfItems> sqi2 = csq.GetValueAsSQ();
		if (!(sqi2 && sqi2->GetNumberOfItems() > 0)) continue;
		unsigned int nitems = sqi2->GetNumberOfItems();
		//
		for (unsigned int i = 0; i < nitems; ++i)
		{
			const mdcm::Item & item2 = sqi2->GetItem(i + 1);
			const mdcm::DataSet& nestedds2 = item2.GetNestedDataSet();
			// ContourGeometricType
			const mdcm::Tag tcontour_geometric_type(0x3006, 0x0042);
			const mdcm::DataElement & contour_geometric_type = nestedds2.GetDataElement(tcontour_geometric_type);
			QString qtr_contour_geometric_type;
			if (!contour_geometric_type.IsEmpty() && !contour_geometric_type.IsUndefinedLength() &&
				contour_geometric_type.GetByteValue())
			{
				qtr_contour_geometric_type = QString::fromLatin1(
					contour_geometric_type.GetByteValue()->GetPointer(),
					contour_geometric_type.GetByteValue()->GetLength()).trimmed().remove(QChar('\0')).toUpper();
			}
			// ContourData
			const mdcm::Tag tcontourdata(0x3006, 0x0050);
			const mdcm::DataElement & contourdata = nestedds2.GetDataElement(tcontourdata);
			mdcm::Attribute<0x3006,0x0050> at1;
			at1.SetFromDataElement(contourdata);
			const double * varray_p = at1.GetValues();
			const unsigned int vertices = at1.GetNumberOfValues() / 3;
			Contour * contour = new Contour();
			contour->id = i;
			contour->roiid = roi.id;
			if (qtr_contour_geometric_type == QString("CLOSED_PLANAR"))
			{
				contour->type = 1;
			}
			else if (qtr_contour_geometric_type == QString("OPEN_PLANAR"))
			{
				contour->type = 2;
			}
			else if (qtr_contour_geometric_type == QString("OPEN_NONPLANAR"))
			{
				contour->type = 3;
			}
			else if (qtr_contour_geometric_type == QString("POINT"))
			{
				contour->type = 4;
			}
			else if (qtr_contour_geometric_type == QString("CLOSEDPLANAR_XOR"))
			{
				contour->type = 5;
			}
			else
			{
				contour->type = 0;
			}
			for (unsigned int j = 0; j < vertices * 3; j += 3)
			{
				DPoint point;
				point.x = static_cast<float>(varray_p[j + 0]);
				point.y = static_cast<float>(varray_p[j + 1]);
				point.z = static_cast<float>(varray_p[j + 2]);
#if 1
				// initialize to silence Coverity warning
				point.u = 0.0f;
				point.v = 0.0f;
				point.t = -1;
#endif
				contour->dpoints.push_back(point);
			}
			// Contour Image Sequence
			{
				const mdcm::Tag timageseq(0x3006,0x0016);
				const mdcm::DataElement & imageseq = nestedds2.GetDataElement(timageseq);
				if (!imageseq.IsEmpty())
				{
					mdcm::SmartPointer<mdcm::SequenceOfItems> sqimageseq = imageseq.GetValueAsSQ();
					if (sqimageseq && sqimageseq->GetNumberOfItems() > 0)
					{
						for (unsigned int n_imageseq = 0; n_imageseq < sqimageseq->GetNumberOfItems(); ++n_imageseq)
						{
							const mdcm::Item & imageseqitem = sqimageseq->GetItem(n_imageseq + 1);
							const mdcm::DataSet & imageseqds = imageseqitem.GetNestedDataSet();
							// Referenced SOP Instance UID
							const mdcm::Tag trefsopinstuid(0x0008, 0x1155);
							const mdcm::DataElement & erefsopinstuid = imageseqds.GetDataElement(trefsopinstuid);
							if (!erefsopinstuid.IsEmpty() && !erefsopinstuid.IsUndefinedLength() &&
								erefsopinstuid.GetByteValue())
							{
								const QString tmp688 = QString::fromLatin1(
									erefsopinstuid.GetByteValue()->GetPointer(),
									erefsopinstuid.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
								contour->ref_sop_instance_uids << tmp688;
#ifdef TMP_PRINT_LOAD_CONTOUR
								std::cout << " 0x0008, 0x1155 Referenced SOP Instance UID " <<
									n_imageseq << " " << tmp688.toStdString() << std::endl;
#endif
							}
						}
					}
				}
			}
			//
			// maybe TODO retired Contour Slab Thickness, Contour Offset Vector
			//
			contour->vao_initialized = false;
			roi.contours[contour->id] = contour;
		}
		if (ivariant) ivariant->di->rois.push_back(roi);
	}
	ivariant->image_type = 100;
	read_ivariant_info_tags(ds, ivariant);
	read_acquisition_time(ds, ivariant->acquisition_date, ivariant->acquisition_time);
}
#ifdef TMP_PRINT_LOAD_CONTOUR
#undef TMP_PRINT_LOAD_CONTOUR
#endif

int DicomUtils::read_instance_number(
	const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return -1;
	const mdcm::Tag tInstanceNumber(0x0020, 0x0013);
	const mdcm::DataElement & e = ds.GetDataElement(tInstanceNumber);
	if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
	{
		const QString s = QString::fromLatin1(
			e.GetByteValue()->GetPointer(),
			e.GetByteValue()->GetLength());
		bool ok;
		const int tmp0 = QVariant(s.trimmed().remove(QChar('\0'))).toInt(&ok);
		if (ok) return tmp0;
	}
	return -1;
}

QString DicomUtils::read_instance_uid(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return QString("");
	const mdcm::Tag tInstanceUID(0x0008,0x0018);
	const mdcm::DataElement & e = ds.GetDataElement(tInstanceUID);
	if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
	{
		const QString s = QString::fromLatin1(
			e.GetByteValue()->GetPointer(),
			e.GetByteValue()->GetLength());
		return s;
	}
	return QString("");
}

void DicomUtils::read_window(
	const mdcm::DataSet & ds,
	double * center_,
	double * width_,
	short  * lut_function)
{
	if (ds.IsEmpty()) return;
	short lut_function_{1};
	QString s;
	if (get_string_value(ds, mdcm::Tag(0x0028,0x1056), s))
	{
		s = s.trimmed().toUpper().remove(QChar('\0'));
		if (s == QString("LINEAR_EXACT"))
		{
			lut_function_ = 0;
		}
		else if (s == QString("SIGMOID"))
		{
			lut_function_ = 2;
		}
	}
	*lut_function = lut_function_;
	std::vector<double> c;
	std::vector<double> w;
	if (get_ds_values(ds, mdcm::Tag(0x0028,0x1050), c) &&
		get_ds_values(ds, mdcm::Tag(0x0028,0x1051), w))
	{
		if (!c.empty())
		{
			*center_ = c.at(0);
		}
		if (!w.empty())
		{
			*width_ = w.at(0);
		}
	}
}

void DicomUtils::read_us_regions(
	const mdcm::DataSet & ds,
	ImageVariant * ivariant)
{
	UltrasoundRegionUtils::Read(ds, ivariant->usregions);
}

bool DicomUtils::read_slices(
	const QStringList & filenames_, ImageVariant * ivariant,
	float tolerance)
{
	if (!ivariant) return false;
	bool ok{};
	bool failed{};
	const int unsigned size_z = filenames_.size();
	std::vector<double*> values;
	unsigned short rows{};
	unsigned short columns{};
	double spacing_x{};
	double spacing_y{};
	double spacing_z{};
	double origin_x{};
	double origin_y{};
	double origin_z{};
	float
		slices_dir_x,
		slices_dir_y,
		slices_dir_z,
		up_dir_x,
		up_dir_y,
		up_dir_z;
	float center_x, center_y, center_z;
	double dircos[9]{};
	for (unsigned int i = 0; i < size_z; ++i)
	{
		QString
			sop_instance_uid,
			pat_pos_s,
			pat_orient_s,
			pix_spacing_s,
			orientation_20_20;
		unsigned short rows_{};
		unsigned short columns_{};
		read_image_info(
			filenames_.at(i),
			&rows_, &columns_,
			pat_pos_s,
			pat_orient_s,
			pix_spacing_s,
			sop_instance_uid,
			orientation_20_20);
		ivariant->image_instance_uids[i] = std::move(sop_instance_uid);
		if (!orientation_20_20.isEmpty())
			ivariant->orientations_20_20[i] = std::move(orientation_20_20);
		double pat_pos[3];
		double pat_orient[6];
		double pix_spacing[2];
		const bool ok_pos = get_patient_position(pat_pos_s, pat_pos);
		const bool ok_orient = get_patient_orientation(pat_orient_s, pat_orient);
		const bool pix_spacing_ok = get_pixel_spacing(pix_spacing_s, pix_spacing);
		if (ok_pos && ok_orient && pix_spacing_ok)
		{
			bool ok1{true};
			if (i == 0)
			{
				rows = rows_;
				columns = columns_;
				spacing_x = pix_spacing[1];
				spacing_y = pix_spacing[0];
			}
			else
			{
				if (rows != rows_ || columns != columns_)
				{
					ok1 = false;
				}
				if (spacing_x > pix_spacing[1] + 0.01 ||
					spacing_x < pix_spacing[1] - 0.01 ||
					spacing_y > pix_spacing[0] + 0.01 ||
					spacing_y < pix_spacing[0] - 0.01)
				{
					ok1 = false;
				}
			}
			if (ok1)
			{
				double * p = new double[9];
				p[0] = pat_pos[0];
				p[1] = pat_pos[1];
				p[2] = pat_pos[2];
				p[3] = pat_orient[0];
				p[4] = pat_orient[1];
				p[5] = pat_orient[2];
				p[6] = pat_orient[3];
				p[7] = pat_orient[4];
				p[8] = pat_orient[5];
				values.push_back(p);
			}
			else
			{
				failed = true;
			}
		}
		else
		{
			failed = true;
		}
	}
	if (failed) goto quit_;
	ok = generate_geometry(
			ivariant->di->image_slices,
			values,
			rows, columns,
			spacing_x, spacing_y, &spacing_z,
			&ivariant->equi, &ivariant->one_direction,
			&origin_x, &origin_y, &origin_z,
			dircos,
			&slices_dir_x, &slices_dir_y, &slices_dir_z,
			&up_dir_x, &up_dir_y, &up_dir_z,
			&center_x, &center_y, &center_z,
			tolerance);
	if (ok)
	{
		ivariant->di->slices_direction_x = slices_dir_x;
		ivariant->di->slices_direction_y = slices_dir_y;
		ivariant->di->slices_direction_z = slices_dir_z;
		ivariant->di->up_direction_x = up_dir_x;
		ivariant->di->up_direction_y = up_dir_y;
		ivariant->di->up_direction_z = up_dir_z;
		// for validation, will be taken from final itk image later
		ivariant->di->ix_origin = origin_x;
		ivariant->di->iy_origin = origin_y;
		ivariant->di->iz_origin = origin_z;
		ivariant->di->ix_spacing = spacing_x;
		ivariant->di->iy_spacing = spacing_y;
		ivariant->di->iz_spacing = size_z == 1 ? 1 : spacing_z;
		for (int x = 0; x < 6; ++x)
		{
			ivariant->di->dircos[x] = static_cast<float>(dircos[x]); ///////
		}
		//
		if (ivariant->equi == false)
		{
			float cx{};
			float cy{};
			float cz{};
			CommonUtils::calculate_center_notuniform(ivariant->di->image_slices, &cx, &cy, &cz);
			ivariant->di->default_center_x = ivariant->di->center_x = cx;
			ivariant->di->default_center_y = ivariant->di->center_y = cy;
			ivariant->di->default_center_z = ivariant->di->center_z = cz;
		}
		else
		{
			ivariant->di->default_center_x = ivariant->di->center_x = center_x;
			ivariant->di->default_center_y = ivariant->di->center_y = center_y;
			ivariant->di->default_center_z = ivariant->di->center_z = center_z;
		}
		ivariant->di->slices_generated = true;
		ivariant->di->slices_from_dicom = true;
	}
quit_:
	for (unsigned int x = 0; x < values.size(); ++x)
	{
		delete [] values[x];
	}
	return ok;
}

bool DicomUtils::read_slices_uihgrid(
	const mdcm::DataSet & ds, ImageVariant * ivariant,
	float tolerance)
{
	if (!ivariant) return false;
	//
	const mdcm::PrivateTag tMRNumberOfSliceInVolume(0x0065, 0x50, "Image Private Header");
	int num_slices{};
	{
		std::vector<double> result;
		if (DicomUtils::priv_get_ds_values(ds, tMRNumberOfSliceInVolume, result))
		{
			num_slices = static_cast<int>(result[0]);
		}
		else if (DicomUtils::get_ds_values(ds, mdcm::Tag(0x0065,0x1050), result))
		{
			num_slices = static_cast<int>(result[0]);
		}
	}
	if (num_slices < 1) return false;
	//
	const mdcm::Tag tImageOrientationPatient(0x0020,0x0037);
	std::vector<double> pat_orient;
	if (get_ds_values(ds, tImageOrientationPatient, pat_orient))
	{
		if (pat_orient.size() != 6) return false;
	}
	//
	double spacing_x{};
	double spacing_y{};
	double spacing_z{};
	bool spacing_ok{};
	{
		const mdcm::Tag tspacing0(0x0028,0x0030);
		const mdcm::Tag tspacing1(0x0018,0x1164);
		const mdcm::Tag tspacing2(0x0018,0x2010);
		const mdcm::Tag tspacing3(0x0028,0x0034);
		QString spacing_s;
		{
			QString spacing_;
			if (get_string_value(ds, tspacing0, spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
			else if (get_string_value(ds, tspacing1, spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
			else if (get_string_value(ds, tspacing2, spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
			else if (get_string_value(ds, tspacing3, spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
		}
		double pix_spacing[2];
		if (get_pixel_spacing(spacing_s, pix_spacing))
		{
			spacing_x = pix_spacing[1];
			spacing_y = pix_spacing[0];
			spacing_ok = true;
		}
	}
	if (!spacing_ok) return false;
	//
	const mdcm::PrivateTag tMRVFrameSequence(0x0065,0x51,"Image Private Header");
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq;
	{
		const mdcm::DataElement & e1 = ds.GetDataElement(tMRVFrameSequence);
		if (!e1.IsEmpty())
		{
			sq = e1.GetValueAsSQ();
			if (!(sq && sq->GetNumberOfItems() > 0)) return false;
		}
		else
		{
			const mdcm::DataElement & e = ds.GetDataElement(mdcm::Tag(0x0065,0x1051));
			if (!e.IsEmpty())
			{
				sq = e.GetValueAsSQ();
				if (!(sq && sq->GetNumberOfItems() > 0)) return false;
			}
			else
			{
				return false;
			}
		}
	}
	const unsigned int num_items = sq->GetNumberOfItems();
	if (static_cast<unsigned int>(num_slices) != num_items) return false;
	//
	//
	// FIXME
	unsigned short image_rows;
	const mdcm::Tag tRows(0x0028,0x0010);
	if (!get_us_value(ds, tRows, &image_rows)) return false;
	const mdcm::Tag tColumns(0x0028,0x0011);
	unsigned short image_columns;
	if (!get_us_value(ds, tColumns, &image_columns)) return false;
	const unsigned int xx = (image_columns >= image_rows)
		? image_columns / ceil(sqrt(num_slices))
		: image_rows / ceil(sqrt(num_slices));
	unsigned short rows = xx;
	unsigned short columns = xx;
	//
	//
	//
	QList<QString> acqtimes;
	std::vector<double*> values;
	for (unsigned int x = 0; x < num_items; ++x)
	{
		const mdcm::Item & item = sq->GetItem(x + 1);
		const mdcm::DataSet & nds = item.GetNestedDataSet();
		const mdcm::Tag tImagePositionPatient(0x0020,0x0032);
		std::vector<double> pat_pos;
		if (DicomUtils::get_ds_values(nds, tImagePositionPatient, pat_pos))
		{
			if (pat_pos.size() != 3) return false;
		}
		double * p = new double[9];
		p[0] = pat_pos[0];
		p[1] = pat_pos[1];
		p[2] = pat_pos[2];
		p[3] = pat_orient[0];
		p[4] = pat_orient[1];
		p[5] = pat_orient[2];
		p[6] = pat_orient[3];
		p[7] = pat_orient[4];
		p[8] = pat_orient[5];
		values.push_back(p);
		const mdcm::Tag tAcquisitionDate(0x0008,0x0022);
		const mdcm::Tag tAcquisitionTime(0x0008,0x0022);
		const mdcm::Tag tAcquisitionDateTime(0x0008,0x002a);
		QString AcquisitionDateTime;
		if (DicomUtils::get_string_value(
				nds,
				tAcquisitionDateTime,
				AcquisitionDateTime))
		{
			AcquisitionDateTime =
				AcquisitionDateTime.trimmed().remove(QChar('\0'));
		}
		else
		{
			QString AcquisitionDate;
			QString AcquisitionTime;
			if (DicomUtils::get_string_value(
					nds,
					tAcquisitionDate,
					AcquisitionDate) &&
				DicomUtils::get_string_value(
					nds,
					tAcquisitionTime,
					AcquisitionTime))
			{
				if (!AcquisitionDate.isEmpty() && !AcquisitionTime.isEmpty())
				{
					AcquisitionDateTime =
						AcquisitionDate.trimmed().remove(QChar('\0')) +
						AcquisitionTime.trimmed().remove(QChar('\0'));
				}
			}
		}
		if (!AcquisitionDateTime.isEmpty())
		{
			acqtimes.push_back(AcquisitionDateTime);
		}
	}
	//
	if (!acqtimes.empty())
	{
		std::sort(acqtimes.begin(), acqtimes.end(), acqtime_less_than);
		QString tmp57 = acqtimes.at(0);
		tmp57 = tmp57.trimmed().remove(QChar('\0'));
		if (tmp57.size() >= 14)
		{
			ivariant->acquisition_date = tmp57.left(8);
			ivariant->acquisition_time = tmp57.right(tmp57.size() - 8);
		}
	}
	//
	double origin_x{};
	double origin_y{};
	double origin_z{};
	float slices_dir_x;
	float slices_dir_y;
	float slices_dir_z;
	float up_dir_x;
	float up_dir_y;
	float up_dir_z;
	float center_x;
	float center_y;
	float center_z;
	double dircos[9]{};
	const bool ok = generate_geometry(
			ivariant->di->image_slices,
			values,
			rows, columns,
			spacing_x, spacing_y, &spacing_z,
			&ivariant->equi, &ivariant->one_direction,
			&origin_x, &origin_y, &origin_z,
			dircos,
			&slices_dir_x, &slices_dir_y, &slices_dir_z,
			&up_dir_x, &up_dir_y, &up_dir_z,
			&center_x, &center_y, &center_z,
			tolerance);
	if (ok)
	{
		ivariant->di->slices_direction_x = slices_dir_x;
		ivariant->di->slices_direction_y = slices_dir_y;
		ivariant->di->slices_direction_z = slices_dir_z;
		ivariant->di->up_direction_x = up_dir_x;
		ivariant->di->up_direction_y = up_dir_y;
		ivariant->di->up_direction_z = up_dir_z;
		// for validation, will be taken from final itk image later
		ivariant->di->ix_origin = origin_x;
		ivariant->di->iy_origin = origin_y;
		ivariant->di->iz_origin = origin_z;
		ivariant->di->ix_spacing = spacing_x;
		ivariant->di->iy_spacing = spacing_y;
		ivariant->di->iz_spacing = spacing_z;
		for (int x = 0; x < 6; ++x)
		{
			ivariant->di->dircos[x] = static_cast<float>(dircos[x]); ///////
		}
		//
		if (ivariant->equi == false)
		{
			float cx{};
			float cy{};
			float cz{};
			CommonUtils::calculate_center_notuniform(
				ivariant->di->image_slices,&cx,&cy,&cz);
			ivariant->di->default_center_x = ivariant->di->center_x = cx;
			ivariant->di->default_center_y = ivariant->di->center_y = cy;
			ivariant->di->default_center_z = ivariant->di->center_z = cz;
		}
		else
		{
			ivariant->di->default_center_x = ivariant->di->center_x = center_x;
			ivariant->di->default_center_y = ivariant->di->center_y = center_y;
			ivariant->di->default_center_z = ivariant->di->center_z = center_z;
		}
		ivariant->di->slices_generated = true;
		ivariant->di->slices_from_dicom = true;
	}
	for (unsigned int x = 0; x < values.size(); ++x)
	{
		delete [] values[x];
	}
	return ok;
}

bool DicomUtils::read_slices_rtdose(
	const QString & filename_, ImageVariant * ivariant,
	float tolerance)
{
	if (!ivariant) return false;
	//
	bool ok{};
	std::vector<double*> values;
	unsigned short numframes{};
	unsigned short rows{};
	unsigned short columns{};
	std::vector<double> z_offsets;
	double pat_pos[3];
	double pat_orient[6];
	double pix_spacing[2];
	double spacing_x, spacing_y, spacing_z, origin_x, origin_y, origin_z;
	float slices_dir_x, slices_dir_y, slices_dir_z, up_dir_x, up_dir_y, up_dir_z;
	float center_x, center_y, center_z;
	double dircos[9]{};
	QString pat_pos_s, pat_orient_s, pix_spacing_s;
	read_image_info_rtdose(filename_,
		&numframes, &rows, &columns,
		pat_pos_s, pat_orient_s, pix_spacing_s, z_offsets);
	const bool ok_pos         = get_patient_position(pat_pos_s, pat_pos);
	const bool ok_orient      = get_patient_orientation(pat_orient_s, pat_orient);
	const bool pix_spacing_ok = get_pixel_spacing(pix_spacing_s, pix_spacing);
	if (numframes == 0 || rows == 0 || columns == 0) goto quit_;
	if (!ok_pos || !ok_orient || !pix_spacing_ok) goto quit_;
	if (z_offsets.empty()) goto quit_;
	if (numframes != z_offsets.size()) goto quit_;
	spacing_x = pix_spacing[0];
	spacing_y = pix_spacing[1];
	for (unsigned int i = 0; i < numframes; ++i)
	{
		double * p = new double[9];
		p[0] = pat_pos[0];
		p[1] = pat_pos[1];
		p[2] = pat_pos[2] + static_cast<float>(z_offsets.at(i));
		p[3] = pat_orient[0];
		p[4] = pat_orient[1];
		p[5] = pat_orient[2];
		p[6] = pat_orient[3];
		p[7] = pat_orient[4];
		p[8] = pat_orient[5];
		values.push_back(p);
	}
	ok = generate_geometry(
			ivariant->di->image_slices,
			values,
			rows, columns,
			spacing_x, spacing_y, &spacing_z,
			&ivariant->equi,
			&ivariant->one_direction,
			&origin_x, &origin_y, &origin_z,
			dircos,
			&slices_dir_x, &slices_dir_y, &slices_dir_z,
			&up_dir_x, &up_dir_y, &up_dir_z,
			&center_x, &center_y, &center_z,
			tolerance);
	if (ok)
	{
		ivariant->di->slices_direction_x = slices_dir_x;
		ivariant->di->slices_direction_y = slices_dir_y;
		ivariant->di->slices_direction_z = slices_dir_z;
		ivariant->di->up_direction_x = up_dir_x;
		ivariant->di->up_direction_y = up_dir_y;
		ivariant->di->up_direction_z = up_dir_z;
		// for validation, will be taken from final itk image later
		ivariant->di->ix_origin = origin_x;
		ivariant->di->iy_origin = origin_y;
		ivariant->di->iz_origin = origin_z;
		ivariant->di->ix_spacing = spacing_x;
		ivariant->di->iy_spacing = spacing_y;
		ivariant->di->iz_spacing = spacing_z;
		for (int x = 0; x < 6; ++x)
		{
			ivariant->di->dircos[x] = static_cast<float>(dircos[x]); ///////
		}
		//
		if (ivariant->equi == false)
		{
			float cx{};
			float cy{};
			float cz{};
			CommonUtils::calculate_center_notuniform(ivariant->di->image_slices,&cx,&cy,&cz);
			ivariant->di->default_center_x = ivariant->di->center_x = cx;
			ivariant->di->default_center_y = ivariant->di->center_y = cy;
			ivariant->di->default_center_z = ivariant->di->center_z = cz;
		}
		else
		{
			ivariant->di->default_center_x = ivariant->di->center_x = center_x;
			ivariant->di->default_center_y = ivariant->di->center_y = center_y;
			ivariant->di->default_center_z = ivariant->di->center_z = center_z;
		}
		ivariant->di->slices_generated = true;
		ivariant->di->slices_from_dicom = true;
	}
quit_:
	for (unsigned int x = 0; x < values.size(); ++x)
	{
		delete [] values[x];
	}
	return ok;
}

void DicomUtils::global_force_suppllut(short x)
{
	// 1 force apply
	// 2 force not apply
	force_suppllut.store(x);
}

void DicomUtils::read_dimension_index_sq(
	const mdcm::DataSet & ds,
	DimIndexSq & sq)
{
	if (ds.IsEmpty()) return;
	const mdcm::Tag tDimensionIndexSequence(0x0020,0x9222);
	const mdcm::Tag tDimensionOrganizationUID(0x0020,0x9164);
	const mdcm::Tag tDimensionIndexPointer(0x0020,0x9165);
	const mdcm::Tag tFunctionalGroupPointer(0x0020,0x9167);
	const mdcm::DataElement & deDimensionIndexSequence =
		ds.GetDataElement(tDimensionIndexSequence);
	if (!deDimensionIndexSequence.IsEmpty())
	{
		mdcm::SmartPointer<mdcm::SequenceOfItems>
			sqDimensionIndexSequence =
				deDimensionIndexSequence.GetValueAsSQ();
		if (!sqDimensionIndexSequence) return;
		const unsigned int number_of_items =
			sqDimensionIndexSequence->GetNumberOfItems();
		for (unsigned int x = 0; x < number_of_items; ++x)
		{
			const mdcm::Item & item =
				sqDimensionIndexSequence->GetItem(x + 1);
			const mdcm::DataSet & nestedds =
				item.GetNestedDataSet();
			unsigned short group0{};
			unsigned short element0{};
			unsigned short group1{};
			unsigned short element1{};
			QString dim_uid;
			const bool ok0 =
				get_at_value(
					nestedds,
					tDimensionIndexPointer,
					&group0,
					&element0);
			const bool ok1 =
				get_at_value(
					nestedds,
					tFunctionalGroupPointer,
					&group1,&element1);
			const bool ok2 =
				get_string_value(
					nestedds,
					tDimensionOrganizationUID,
					dim_uid);
			(void)ok2;
			if (ok0 && ok1)
			{
				DimIndex idx;
				idx.uid = dim_uid.trimmed().remove(QChar('\0')).toStdString();
				idx.index_pointer = mdcm::Tag(group0, element0);
				idx.group_pointer = mdcm::Tag(group1, element1);
				sq.push_back(std::move(idx));
			}
			else if (ok0)
			{
				DimIndex idx;
				idx.uid = dim_uid.trimmed().remove(QChar('\0')).toStdString();
				idx.index_pointer = mdcm::Tag(group0, element0);
				idx.group_pointer = mdcm::Tag(0xffff,0xffff);
				sq.push_back(std::move(idx));
			}
		}
	}
}

bool DicomUtils::read_dimension_index_values(
	const mdcm::DataSet & ds,
	const unsigned int idx_sq_size,
	DimIndexValues & values)
{
	if (ds.IsEmpty()) return false;
	const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
	const mdcm::Tag tFrameContentSequence(0x0020,0x9111);
	const mdcm::Tag tDimensionIndexValues(0x0020,0x9157);
	const mdcm::DataElement & dePerFrameFunctionalGroupsSequence =
		ds.GetDataElement(tPerFrameFunctionalGroupsSequence);
	if (!dePerFrameFunctionalGroupsSequence.IsEmpty())
	{
		mdcm::SmartPointer<mdcm::SequenceOfItems>
			sqPerFrameFunctionalGroupsSequence =
				dePerFrameFunctionalGroupsSequence.GetValueAsSQ();
		if (sqPerFrameFunctionalGroupsSequence &&
			sqPerFrameFunctionalGroupsSequence->GetNumberOfItems() > 0)
		{
			for (unsigned int x = 0;
				x < sqPerFrameFunctionalGroupsSequence->GetNumberOfItems();
				++x)
			{
				const mdcm::Item & item = sqPerFrameFunctionalGroupsSequence->GetItem(x + 1);
				const mdcm::DataSet & nestedds = item.GetNestedDataSet();
				const mdcm::DataElement & deFrameContentSequence =
					nestedds.GetDataElement(tFrameContentSequence);
				if (!deFrameContentSequence.IsEmpty())
				{
					mdcm::SmartPointer<mdcm::SequenceOfItems> sqFrameContentSequence =
						deFrameContentSequence.GetValueAsSQ();
					if (sqFrameContentSequence && sqFrameContentSequence->GetNumberOfItems() == 1)
					{
						const mdcm::Item & item1 = sqFrameContentSequence->GetItem(1);
						const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
						{
							DimIndexValue index_value;
							index_value.id = x;
							const bool ok = get_ul_values(
								nestedds1,
								tDimensionIndexValues,
								index_value.idx);
							if (ok && index_value.idx.size() == idx_sq_size)
							{
								values.push_back(std::move(index_value));
							}
							else
							{
								return false;
							}
						}
					}
				}
			}
			return true;
		}
	}
	return false;
}

bool DicomUtils::read_group_sq(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const DimIndexSq & sq,
	DimIndexValues & dim_idx_values,
	FrameGroupValues & values)
{
	if (ds.IsEmpty()) return false;
	const mdcm::Tag tSpecificCharacterSet(0x0008,0x0005);
	const mdcm::Tag tFrameContentSequence(0x0020,0x9111);
	const mdcm::Tag tPixelMeasuresSequence(0x0028,0x9110);
	const mdcm::Tag tFrameVOILUTSequence(0x0028,0x9132);
	const mdcm::Tag tPlaneOrientationSequence(0x0020,0x9116);
	const mdcm::Tag tPlanePositionSequence(0x0020,0x9113);
	const mdcm::Tag tTemporalPositionSequence(0x0020,0x9310);
	const mdcm::Tag tPlanePositionVolumeSequence(0x0020,0x930e);
	const mdcm::Tag tPlaneOrientationVolumeSequence(0x0020,0x930f);
	const mdcm::Tag tImageDataTypeSequence(0x0018,0x9807);
	const mdcm::Tag tFrameAnatomySequence(0x0020,0x9071);
	const mdcm::Tag tAnatomicRegionSequence(0x0008,0x2218);
	const mdcm::Tag tCodeMeaning(0x0008,0x0104);
	const mdcm::Tag tFrameLaterality(0x0020,0x9072);
	const mdcm::Tag tDimensionIndexValues(0x0020,0x9157);
	const mdcm::Tag tStackID(0x0020,0x9056);
	const mdcm::Tag tInStackPositionNumber(0x0020,0x9057);
	const mdcm::Tag tTemporalPositionIndex(0x0020,0x9128);
	const mdcm::Tag tImagePositionPatient(0x0020,0x0032);
	const mdcm::Tag tImageOrientationPatient(0x0020,0x0037);
	const mdcm::Tag tPixelSpacing(0x0028,0x0030);               // Pixel Spacing
	const mdcm::Tag tImagerPixelSpacing(0x0018,0x1164);         // Imager Pixel Spacing
	const mdcm::Tag tNominalScannedPixelSpacing(0x0018,0x2010); // Nominal Scanned Pixel Spacing
	const mdcm::Tag tPixelAspectRatio(0x0028,0x0034);           // Pixel Aspect Ratio
	const mdcm::Tag tSliceThickness(0x0018,0x0050);
	const mdcm::Tag tWindowCenter(0x0028,0x1050);
	const mdcm::Tag tWindowWidth(0x0028,0x1051);
	const mdcm::Tag tLUTFunction(0x0028,0x1056);
	const mdcm::Tag tTemporalPositionTimeOffset(0x0020,0x930d);
	const mdcm::Tag tImagePositionVolume(0x0020,0x9301);
	const mdcm::Tag tImageOrientationVolume(0x0020,0x9302);
	const mdcm::Tag tDataType(0x0018,0x9808);
	const mdcm::Tag tPixelValueTransformationSequence(0x0028,0x9145);
	const mdcm::Tag tRescaleIntercept(0x0028,0x1052);
	const mdcm::Tag tRescaleSlope(0x0028,0x1053);
	const mdcm::Tag tRescaleType(0x0028,0x1054);
	const mdcm::Tag tFrameAcquisitionDateTime(0x0018,0x9074);
	const mdcm::Tag tFrameReferenceDateTime(0x0018,0x9151);
	const mdcm::Tag tSegmentIdentificationSequence(0x0062,0x000a);
	const mdcm::Tag tReferencedSegmentNumber(0x0062,0x000b);
	//
	const mdcm::DataElement & deGroup = ds.GetDataElement(t);
	if (deGroup.IsEmpty()) return false;
	QString charset;
	{
		const mdcm::DataElement & eSpecificCharacterSet = ds.GetDataElement(tSpecificCharacterSet);
		if (!eSpecificCharacterSet.IsEmpty() && !eSpecificCharacterSet.IsUndefinedLength() &&
			eSpecificCharacterSet.GetByteValue())
		{
			charset = QString::fromLatin1(
				eSpecificCharacterSet.GetByteValue()->GetPointer(),
				eSpecificCharacterSet.GetByteValue()->GetLength()).trimmed();
		}
	}
	//
	mdcm::SmartPointer<mdcm::SequenceOfItems> sqGroup = deGroup.GetValueAsSQ();
	if (!(sqGroup && sqGroup->GetNumberOfItems() > 0))
	{
		return false;
	}
	for (unsigned int x = 0; x < sqGroup->GetNumberOfItems(); ++x)
	{
		FrameGroup fg;
		fg.id = x;
		const mdcm::Item & item = sqGroup->GetItem(x + 1);
		const mdcm::DataSet & nestedds = item.GetNestedDataSet();
		{
			const mdcm::DataElement & deFrameContentSequence = nestedds.GetDataElement(tFrameContentSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqFrameContentSequence = deFrameContentSequence.GetValueAsSQ();
			if (sqFrameContentSequence && sqFrameContentSequence->GetNumberOfItems() == 1)
			{
				const mdcm::Item & item1 = sqFrameContentSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				{
					DimIndexValue index_value;
					index_value.id = x;
					const bool ok = get_ul_values(nestedds1, tDimensionIndexValues, index_value.idx);
					if (ok && index_value.idx.size() == sq.size())
					{
#ifdef ENHANCED_PRINT_INFO
						std::cout << "Dimension Index Values";
						for (size_t y = 0; y < sq.size(); ++y)
						{
							std::cout << " " << index_value.idx.at(y);
						}
						std::cout << " (id = " << index_value.id << ")" << std::endl;
#endif
						dim_idx_values.push_back(std::move(index_value));
					}
#ifdef ENHANCED_PRINT_INFO
					else
					{
						std::cout << "Failed: Dimension Index Values (id) = " << index_value.id << std::endl;
					}
#endif
				}
				{
					const mdcm::DataElement & deStackID = nestedds1.GetDataElement(tStackID);
					if (!deStackID.IsEmpty() && !deStackID.IsUndefinedLength() && deStackID.GetByteValue())
					{
						fg.stack_id =
							QVariant(QString::fromLatin1(
								deStackID.GetByteValue()->GetPointer(),
								deStackID.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'))).toInt(&fg.stack_id_ok);
					}
				}
				{
					const mdcm::DataElement & deInStackPositionNumber = nestedds1.GetDataElement(tInStackPositionNumber);
					if (!deInStackPositionNumber.IsEmpty() && !deInStackPositionNumber.IsUndefinedLength() &&
						deInStackPositionNumber.GetByteValue())
					{
						unsigned int tmp678;
						fg.in_stack_pos_num_ok = get_ul_value(nestedds1, tInStackPositionNumber, &tmp678);
						fg.in_stack_pos_num = static_cast<int>(tmp678);
					}
				}
				{
					QString FrameAcquisitionDateTime;
					if (get_string_value(nestedds1, tFrameAcquisitionDateTime, FrameAcquisitionDateTime))
					{
						fg.frame_acquisition_datetime = std::move(FrameAcquisitionDateTime);
					}
				}
				{
					QString FrameReferenceDateTime;
					if (get_string_value(nestedds1, tFrameReferenceDateTime, FrameReferenceDateTime))
					{
						fg.frame_reference_datetime = std::move(FrameReferenceDateTime);
					}
				}
			}
		}
		{
			const mdcm::DataElement & dePlanePositionSequence = nestedds.GetDataElement(tPlanePositionSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPlanePositionSequence = dePlanePositionSequence.GetValueAsSQ();
			if (sqPlanePositionSequence && sqPlanePositionSequence->GetNumberOfItems() == 1)
			{
				const mdcm::Item & item1 = sqPlanePositionSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				const mdcm::DataElement & deImagePositionPatient = nestedds1.GetDataElement(tImagePositionPatient);
				if (!deImagePositionPatient.IsEmpty() && !deImagePositionPatient.IsUndefinedLength() &&
					deImagePositionPatient.GetByteValue())
				{
					fg.pat_pos =
						QString::fromLatin1(
							deImagePositionPatient.GetByteValue()->GetPointer(),
							deImagePositionPatient.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
				}
			}
		}
		{
			const mdcm::DataElement & dePlaneOrientationSequence = nestedds.GetDataElement(tPlaneOrientationSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPlaneOrientationSequence =
				dePlaneOrientationSequence.GetValueAsSQ();
			if (sqPlaneOrientationSequence && sqPlaneOrientationSequence->GetNumberOfItems() == 1)
			{
				const mdcm::Item & item1 = sqPlaneOrientationSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				const mdcm::DataElement & deImageOrientationPatient = nestedds1.GetDataElement(tImageOrientationPatient);
				if (!deImageOrientationPatient.IsEmpty() && !deImageOrientationPatient.IsUndefinedLength() &&
					deImageOrientationPatient.GetByteValue())
				{
					fg.pat_orient =
						QString::fromLatin1(
							deImageOrientationPatient.GetByteValue()->GetPointer(),
							deImageOrientationPatient.GetByteValue()->GetLength()).
								trimmed().remove(QChar('\0'));
				}
			}
		}
		{
			const mdcm::DataElement & dePixelMeasuresSequence = nestedds.GetDataElement(tPixelMeasuresSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPixelMeasuresSequence = dePixelMeasuresSequence.GetValueAsSQ();
			if (sqPixelMeasuresSequence && sqPixelMeasuresSequence->GetNumberOfItems() == 1)
			{
				const mdcm::Item & item1 = sqPixelMeasuresSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				const mdcm::DataElement & dePixelSpacing = nestedds1.GetDataElement(tPixelSpacing);
				if (!dePixelSpacing.IsEmpty() &&
					!dePixelSpacing.IsUndefinedLength() &&
					dePixelSpacing.GetByteValue())
				{
					fg.pix_spacing =
						QString::fromLatin1(
							dePixelSpacing.GetByteValue()->GetPointer(),
							dePixelSpacing.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
				}
				else
				{
					const mdcm::DataElement & deImagerPixelSpacing = nestedds1.GetDataElement(tImagerPixelSpacing);
					if (!deImagerPixelSpacing.IsEmpty() &&
						!deImagerPixelSpacing.IsUndefinedLength() &&
						deImagerPixelSpacing.GetByteValue())
					{
						fg.pix_spacing =
							QString::fromLatin1(
								deImagerPixelSpacing.GetByteValue()->GetPointer(),
								deImagerPixelSpacing.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
					}
					else
					{
						const mdcm::DataElement & deNominalScannedPixelSpacing =
							nestedds1.GetDataElement(tNominalScannedPixelSpacing);
						if (!deNominalScannedPixelSpacing.IsEmpty() &&
							!deNominalScannedPixelSpacing.IsUndefinedLength() &&
							deNominalScannedPixelSpacing.GetByteValue())
						{
							fg.pix_spacing =
								QString::fromLatin1(
									deNominalScannedPixelSpacing.GetByteValue()->GetPointer(),
									deNominalScannedPixelSpacing.GetByteValue()->GetLength()).
										trimmed().remove(QChar('\0'));
						}
						else
						{
							const mdcm::DataElement & dePixelAspectRatio = nestedds1.GetDataElement(tPixelAspectRatio);
							if (!dePixelAspectRatio.IsEmpty() &&
								!dePixelAspectRatio.IsUndefinedLength() &&
								dePixelAspectRatio.GetByteValue())
							{
								fg.pix_spacing =
									QString::fromLatin1(
										dePixelAspectRatio.GetByteValue()->GetPointer(),
										dePixelAspectRatio.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
							}
						}
					}
				}
				{
					const mdcm::DataElement & deSliceThickness = nestedds1.GetDataElement(tSliceThickness);
					if (!deSliceThickness.IsEmpty() &&
						!deSliceThickness.IsUndefinedLength() &&
						deSliceThickness.GetByteValue())
					{
						fg.slice_thick =
							QString::fromLatin1(
								deSliceThickness.GetByteValue()->GetPointer(),
								deSliceThickness.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
					}
				}
			}
		}
		{
			const mdcm::DataElement & deFrameAnatomySequence = nestedds.GetDataElement(tFrameAnatomySequence);
			if (!deFrameAnatomySequence.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqFrameAnatomySequence =
					deFrameAnatomySequence.GetValueAsSQ();
				if (sqFrameAnatomySequence && sqFrameAnatomySequence->GetNumberOfItems() == 1)
				{
					const mdcm::Item & item1 = sqFrameAnatomySequence->GetItem(1);
					const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
					{
						const mdcm::DataElement & deFrameLaterality = nestedds1.GetDataElement(tFrameLaterality);
						if (!deFrameLaterality.IsEmpty() &&
							!deFrameLaterality.IsUndefinedLength() &&
							deFrameLaterality.GetByteValue())
						{
							fg.frame_laterality =
								QString::fromLatin1(
									deFrameLaterality.GetByteValue()->GetPointer(),
									deFrameLaterality.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
						}
					}
					{
						const mdcm::DataElement & deAnatomicRegionSequence =
							nestedds1.GetDataElement(tAnatomicRegionSequence);
						if (!deAnatomicRegionSequence.IsEmpty())
						{
							mdcm::SmartPointer<mdcm::SequenceOfItems> sqAnatomicRegionSequence =
								deAnatomicRegionSequence.GetValueAsSQ();
							if (sqAnatomicRegionSequence && sqAnatomicRegionSequence->GetNumberOfItems() == 1)
							{
								const mdcm::Item & item2 = sqAnatomicRegionSequence->GetItem(1);
								const mdcm::DataSet & nestedds2 = item2.GetNestedDataSet();
								{
									const mdcm::DataElement & deCodeMeaning = nestedds2.GetDataElement(tCodeMeaning);
									if (!deCodeMeaning.IsEmpty() &&
										!deCodeMeaning.IsUndefinedLength() &&
										deCodeMeaning.GetByteValue())
									{
										QByteArray baCodeMeaning(
											deCodeMeaning.GetByteValue()->GetPointer(),
											deCodeMeaning.GetByteValue()->GetLength());
										const QString frame_body_part =
											CodecUtils::toUTF8(&baCodeMeaning, charset.toLatin1().constData());
										if (!frame_body_part.isEmpty())
										{
											fg.frame_body_part = frame_body_part.trimmed().remove(QChar('\0'));
										}
									}
								}
							}
						}
					}
				}
			}
		}
		{
			const mdcm::DataElement & deFrameVOILUTSequence = nestedds.GetDataElement(tFrameVOILUTSequence);
			if (!deFrameVOILUTSequence.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqFrameVOILUTSequence =
					deFrameVOILUTSequence.GetValueAsSQ();
				if (sqFrameVOILUTSequence && sqFrameVOILUTSequence->GetNumberOfItems() == 1)
				{
					const mdcm::Item & item1 = sqFrameVOILUTSequence->GetItem(1);
					const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
					{
						const mdcm::DataElement & deWindowCenter = nestedds1.GetDataElement(tWindowCenter);
						if (!deWindowCenter.IsEmpty() &&
							!deWindowCenter.IsUndefinedLength() &&
							deWindowCenter.GetByteValue())
						{
							fg.window_center =
								QString::fromLatin1(
									deWindowCenter.GetByteValue()->GetPointer(),
									deWindowCenter.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
						}
					}
					{
						const mdcm::DataElement & deWindowWidth = nestedds1.GetDataElement(tWindowWidth);
						if (!deWindowWidth.IsEmpty() &&
							!deWindowWidth.IsUndefinedLength() &&
							deWindowWidth.GetByteValue())
						{
							fg.window_width =
								QString::fromLatin1(
									deWindowWidth.GetByteValue()->GetPointer(),
									deWindowWidth.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
						}
					}
					{
						const mdcm::DataElement & deLUTFunction = nestedds1.GetDataElement(tLUTFunction);
						if (!deLUTFunction.IsEmpty() &&
							!deLUTFunction.IsUndefinedLength() &&
							deLUTFunction.GetByteValue())
						{
							fg.lut_function =
								QString::fromLatin1(
									deLUTFunction.GetByteValue()->GetPointer(),
									deLUTFunction.GetByteValue()->GetLength()).
										trimmed().remove(QChar('\0'));
						}
					}
				}
			}
		}
		{
			const mdcm::DataElement & deTemporalPositionSequence =
				nestedds.GetDataElement(tTemporalPositionSequence);
			if (!deTemporalPositionSequence.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqTemporalPositionSequence =
					deTemporalPositionSequence.GetValueAsSQ();
				if (sqTemporalPositionSequence && sqTemporalPositionSequence->GetNumberOfItems() == 1)
				{
					const mdcm::Item & item1 = sqTemporalPositionSequence->GetItem(1);
					const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
					fg.temp_pos_off_ok =
						get_fd_value(
							nestedds1,
							tTemporalPositionTimeOffset,
							&fg.temp_pos_off);
				}
			}
		}
		{
			const mdcm::DataElement & dePlanePositionVolumeSequence =
				nestedds.GetDataElement(tPlanePositionVolumeSequence);
			if (!dePlanePositionVolumeSequence.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqPlanePositionVolumeSequence =
					dePlanePositionVolumeSequence.GetValueAsSQ();
				if (sqPlanePositionVolumeSequence && sqPlanePositionVolumeSequence->GetNumberOfItems() == 1)
				{
					const mdcm::Item & item1 = sqPlanePositionVolumeSequence->GetItem(1);
					const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
					std::vector<double> tmp1;
					if (get_fd_values(
							nestedds1,
							tImagePositionVolume,
							tmp1))
					{
						if (tmp1.size() == 3)
						{
							fg.vol_pos_ok = true;
							fg.vol_pos[0] = tmp1.at(0);
							fg.vol_pos[1] = tmp1.at(1);
							fg.vol_pos[2] = tmp1.at(2);
						}
					}
				}
			}
		}
		{
			const mdcm::DataElement & dePlaneOrientationVolumeSequence =
				nestedds.GetDataElement(tPlaneOrientationVolumeSequence);
			if (!dePlaneOrientationVolumeSequence.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqPlaneOrientationVolumeSequence =
					dePlaneOrientationVolumeSequence.GetValueAsSQ();
				if (sqPlaneOrientationVolumeSequence &&
					sqPlaneOrientationVolumeSequence->GetNumberOfItems() == 1)
				{
					const mdcm::Item & item1 = sqPlaneOrientationVolumeSequence->GetItem(1);
					const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
					std::vector<double> tmp1;
					if (get_fd_values(nestedds1, tImageOrientationVolume, tmp1))
					{
						if (tmp1.size() == 6)
						{
							fg.vol_orient_ok = true;
							fg.vol_orient[0] = tmp1.at(0);
							fg.vol_orient[1] = tmp1.at(1);
							fg.vol_orient[2] = tmp1.at(2);
							fg.vol_orient[3] = tmp1.at(3);
							fg.vol_orient[4] = tmp1.at(4);
							fg.vol_orient[5] = tmp1.at(5);
						}
					}
				}
			}
		}
		{
			const mdcm::DataElement & dePixelValueTransformationSequence =
				nestedds.GetDataElement(tPixelValueTransformationSequence);
			if (!dePixelValueTransformationSequence.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqPixelValueTransformationSequence =
					dePixelValueTransformationSequence.GetValueAsSQ();
				if (sqPixelValueTransformationSequence &&
					sqPixelValueTransformationSequence->GetNumberOfItems() == 1)
				{
					const mdcm::Item & item1 = sqPixelValueTransformationSequence->GetItem(1);
					const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
					std::vector<double> tmp1;
					std::vector<double> tmp2;
					if (get_ds_values(nestedds1, tRescaleIntercept, tmp1) &&
						get_ds_values(nestedds1, tRescaleSlope, tmp2))
					{
						fg.rescale_ok = true;
						fg.rescale_intercept = tmp1.at(0);
						fg.rescale_slope = tmp2.at(0);
					}
					QString tmp3;
					if (get_string_value(nestedds1, tRescaleType, tmp3))
					{
						fg.rescale_type = std::move(tmp3);
					}
				}
			}
		}
		{
			const mdcm::DataElement & deSegmentIdentificationSequence =
				nestedds.GetDataElement(tSegmentIdentificationSequence);
			if (!deSegmentIdentificationSequence.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqSegmentIdentificationSequence =
					deSegmentIdentificationSequence.GetValueAsSQ();
				if (sqSegmentIdentificationSequence && sqSegmentIdentificationSequence->GetNumberOfItems() == 1)
				{
					const mdcm::Item & item1 = sqSegmentIdentificationSequence->GetItem(1);
					const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
					const mdcm::DataElement & de = nestedds1.GetDataElement(tReferencedSegmentNumber);
					if (!de.IsEmpty() && !de.IsUndefinedLength() && de.GetByteValue())
					{
						unsigned short ReferencedSegmentNumber;
						if (get_us_value(nestedds1, tReferencedSegmentNumber, &ReferencedSegmentNumber))
						{
							fg.ref_segment_num = ReferencedSegmentNumber;
						}
					}
				}
			}
		}
		//
		values.push_back(fg);
	}
	return true;
}

void DicomUtils::read_frame_times(const mdcm::DataSet & ds, ImageVariant * ivariant, int dimz)
{
	if (ds.IsEmpty()) return;
	const mdcm::Tag tframeincrementpointer(0x0028,0x0009);
	const mdcm::Tag tframetime(0x0018,0x1063);
	const mdcm::Tag tframetimes(0x0018,0x1065);
	double frametime{100.0};
	const mdcm::DataElement & e0 = ds.GetDataElement(tframeincrementpointer);
	if (!e0.IsEmpty())
	{
		unsigned short group, element;
		char group_[2];
		char element_[2];
		const mdcm::ByteValue * bv = e0.GetByteValue();
		if (bv)
		{
			const unsigned long long length =
				static_cast<unsigned long long>(bv->GetLength());
			if (length == 4)
			{
				char * buffer = new char[4];
				const bool ok0 = bv->GetBuffer(buffer, 4);
				if (ok0)
				{
					group_[0] = buffer[0];
					group_[1] = buffer[1];
					memcpy(&group, group_, 2);
					element_[0] = buffer[2];
					element_[1] = buffer[3];
					memcpy(&element, element_, 2);
					if (group == 0x0018 && element == 0x1063)
					{
						const mdcm::DataElement & e = ds.GetDataElement(tframetime);
						if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
						{
							const QString s =
								QString::fromLatin1(
									e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
							bool ok = false;
							double tmp0 = QVariant(s.trimmed().remove(QChar('\0'))).toDouble(&ok);
							if (ok) frametime = tmp0;
						}
						for (int x = 0; x < dimz; ++x) ivariant->frame_times.push_back(frametime);
					}
					else if (group == 0x0018 && element == 0x1065)
					{
						const mdcm::DataElement & e = ds.GetDataElement(tframetimes);
						if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
						{
							const QString s =
								QString::fromLatin1(
									e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
							bool ok;
							QStringList l = s.trimmed().remove(QChar('\0')).split(QString("\\"));
							for (int x = 0; x < l.size(); ++x)
							{
								double tmp0 = QVariant(l.at(x).trimmed().remove(QChar('\0'))).toDouble(&ok);
								if (ok) ivariant->frame_times.push_back(tmp0);
							}
						}
					}
				}
				delete [] buffer;
			}
		}
	}
	else
	{
		const mdcm::DataElement & e = ds.GetDataElement(tframetime);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			const QString s = QString::fromLatin1(
				e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			bool ok;
			double tmp0 = QVariant(s.trimmed().remove(QChar('\0'))).toDouble(&ok);
			if (ok) frametime = tmp0;
		}
		for (int x = 0; x < dimz; ++x) ivariant->frame_times.push_back(frametime);
	}
}

QString DicomUtils::read_anatomic_sq(const mdcm::DataSet & ds)
{
	const mdcm::Tag tAnatomicRegionSequence(0x0008,0x2218);
	const mdcm::Tag tCodeMeaning(0x0008,0x0104);
	const mdcm::DataElement & e = ds.GetDataElement(tAnatomicRegionSequence);
	if (!e.IsEmpty())
	{
		const mdcm::Tag tSpecificCharacterSet(0x0008,0x0005);
		QString charset;
		{
			const mdcm::DataElement & eSpecificCharacterSet = ds.GetDataElement(tSpecificCharacterSet);
			if (!eSpecificCharacterSet.IsEmpty() && !eSpecificCharacterSet.IsUndefinedLength() &&
				eSpecificCharacterSet.GetByteValue())
			{
				charset = QString::fromLatin1(
					eSpecificCharacterSet.GetByteValue()->GetPointer(),
					eSpecificCharacterSet.GetByteValue()->GetLength()).trimmed();
			}
		}
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems() == 1)
		{
			const mdcm::Item & i = sq->GetItem(1);
			const mdcm::DataSet & nds = i.GetNestedDataSet();
			const mdcm::DataElement & e2 = nds.GetDataElement(tCodeMeaning);
			if (!e2.IsEmpty() && !e2.IsUndefinedLength() && e2.GetByteValue())
			{
				QByteArray ba(
					e2.GetByteValue()->GetPointer(),
					e2.GetByteValue()->GetLength());
				const QString frame_body_part =
					CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
				if (!frame_body_part.isEmpty())
					return frame_body_part.trimmed().remove(QChar('\0'));
			}
		}
	}
	return QString("");
}

QString DicomUtils::read_series_laterality(const mdcm::DataSet & ds)
{
	const mdcm::Tag tlaterality(0x0020,0x0060);
	const mdcm::DataElement & e = ds.GetDataElement(tlaterality);
	if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
	{
		const QString s = QString::fromLatin1(
			e.GetByteValue()->GetPointer(),
			e.GetByteValue()->GetLength());
		if (!s.isEmpty()) return s.trimmed().remove(QChar('\0'));
	}
	return QString("");
}

QString DicomUtils::read_image_laterality(const mdcm::DataSet & ds)
{
	const mdcm::Tag tilaterality(0x0020,0x0062);
	const mdcm::DataElement & e = ds.GetDataElement(tilaterality);
	if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
	{
		const QString s = QString::fromLatin1(
			e.GetByteValue()->GetPointer(),
			e.GetByteValue()->GetLength());
		if (!s.isEmpty()) return s.trimmed().remove(QChar('\0'));
	}
	return QString("");
}

QString DicomUtils::read_body_part(const mdcm::DataSet & ds)
{
	const mdcm::Tag tbodypart(0x0018,0x0015);
	const mdcm::DataElement & e = ds.GetDataElement(tbodypart);
	if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
	{
		const QString s = QString::fromLatin1(
			e.GetByteValue()->GetPointer(),
			e.GetByteValue()->GetLength());
		if (!s.isEmpty()) return s.trimmed().toLower().remove(QChar('\0'));
	}
	return QString("");
}

void DicomUtils::read_acquisition_time(
	const mdcm::DataSet & ds,
	QString & acquisitiondate,
	QString & acquisitiontime)
{
	bool acqdatetime_ok{};
	{
		const mdcm::Tag tacquisitiondatetime(0x0008,0x002a);
		QString acquisitiondatetime;
		if (get_string_value(ds, tacquisitiondatetime, acquisitiondatetime))
		{
			acquisitiondatetime = acquisitiondatetime.trimmed().remove(QChar('\0'));
			if (acquisitiondatetime.size() >= 14)
			{
				acquisitiondate = acquisitiondatetime.left(8);
				acquisitiontime = acquisitiondatetime.right(acquisitiondatetime.size() - 8);
				acqdatetime_ok = true;
			}
		}
	}
	if (!acqdatetime_ok)
	{
		const mdcm::Tag tacquisitiondate(0x0008,0x0022);
		const mdcm::Tag tacquisitiontime(0x0008,0x0032);
		QString acquisitiondate_tmp;
		QString acquisitiontime_tmp;
		if (get_string_value(ds, tacquisitiondate, acquisitiondate_tmp) &&
			get_string_value(ds, tacquisitiontime, acquisitiontime_tmp))
		{
			acquisitiondate = acquisitiondate_tmp.trimmed().remove(QChar('\0'));
			acquisitiontime = acquisitiontime_tmp.trimmed().remove(QChar('\0'));
		}
		else
		{
			acquisitiondate = QString("");
			acquisitiontime = QString("");
		}
	}
}

void DicomUtils::read_ivariant_info_tags(const mdcm::DataSet & ds, ImageVariant * ivariant)
{
	if (ds.IsEmpty()) return;
	if (!ivariant)    return;
	QString charset;
	const mdcm::Tag tcharset(0x0008,0x0005);
	const mdcm::Tag timagetype(0x0008,0x0008);
	const mdcm::Tag tsop(0x0008,0x0016);
	const mdcm::Tag tstudydate(0x0008,0x0020);
	const mdcm::Tag tstudytime(0x0008,0x0030);
	const mdcm::Tag tseriesdate(0x0008,0x0021);
	const mdcm::Tag tseriestime(0x0008,0x0031);
	const mdcm::Tag tmodality(0x0008,0x0060);
	const mdcm::Tag tmanufacturer(0x0008,0x0070);
	const mdcm::Tag tmodel(0x0008,0x1090);
	const mdcm::Tag tinstituion(0x0008,0x0080);
	const mdcm::Tag tstudydesc(0x0008,0x1030);
	const mdcm::Tag tseriesdesc(0x0008,0x103e);
	const mdcm::Tag tpatientname(0x0010,0x0010);
	const mdcm::Tag tpatientid(0x0010,0x0020);
	const mdcm::Tag tbirthdate(0x0010,0x0030);
	const mdcm::Tag tpatweight(0x0010,0x1030);
	const mdcm::Tag tsex(0x0010,0x0040);
	const mdcm::Tag tfieldstrength(0x0018,0x0087);
	const mdcm::Tag tstudyuid(0x0020,0x000d);
	const mdcm::Tag tseriesuid(0x0020,0x000e);
	const mdcm::Tag tstudyid(0x0020,0x0010);
	const mdcm::Tag tframe_of_refuid(0x0020,0x0052);
	const mdcm::Tag tcomment(0x0020,0x4000);
	const mdcm::Tag tinterpretation(0x0028,0x0004);
	const mdcm::Tag tpixelrepresentation(0x0028,0x0103);
	const mdcm::PrivateTag tprivcomment(0x0067,0x01,"ALIZA 001");
	{
		QString charset_tmp;
		if (get_string_value(ds, tcharset, charset_tmp))
			charset = std::move(charset_tmp);
	}
	{
		QString imagetype;
		if (get_string_value(ds, timagetype, imagetype))
		{
			imagetype = imagetype.trimmed();
			if (!imagetype.isEmpty())
			{
				QString imagetype_;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				const QStringList l = imagetype.toLower().split(
					QString("\\"),
					Qt::SkipEmptyParts);
#else
				const QStringList l = imagetype.toLower().split(
					QString("\\"),
					QString::SkipEmptyParts);
#endif
				imagetype_.append(QString(
					"<br /><span class='y7'>DICOM image type</span>"
					"<br /><span class='y6'>"));
				for (int x = 0; x < l.size(); ++x)
					imagetype_.append(l.at(x) + QString("<br />"));
				if (!l.empty())
					imagetype_.append(QString("</span><br />"));
				ivariant->imagetype = std::move(imagetype_);
			}
		}
	}
	{
		QString sop;
		if (get_string_value(ds, tsop, sop))
		{
			ivariant->sop = sop.remove(QChar('\0'));
			mdcm::UIDs uid;
			uid.SetFromUID(ivariant->sop.toLatin1().constData());
			ivariant->iod = QString::fromLatin1(uid.GetName());
		}
	}
	{
		QString studydate;
		if (get_string_value(ds, tstudydate, studydate))
			ivariant->study_date = std::move(studydate);
	}
	{
		QString seriesdate;
		if (get_string_value(ds, tseriesdate, seriesdate))
			ivariant->series_date = std::move(seriesdate);
	}
	{
		QString studytime;
		if (get_string_value(ds, tstudytime, studytime))
			ivariant->study_time = std::move(studytime);
	}
	{
		QString seriestime;
		if (get_string_value(ds, tseriestime, seriestime))
			ivariant->series_time = std::move(seriestime);
	}
	//
	{
		QString modality;
		if (get_string_value(ds, tmodality, modality))
			ivariant->modality = modality.remove(QChar('\0'));
	}
	{
		QString manufacturer_s;
		QString model_s;
		const mdcm::DataElement & e = ds.GetDataElement(tmanufacturer);
		if (!e.IsEmpty() &&
			!e.IsUndefinedLength() &&
			e.GetByteValue())
		{
			QByteArray ba(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(
				&ba, charset.toLatin1().constData());
			manufacturer_s = tmp0.trimmed().remove(QChar('\0'));
			{
				const mdcm::DataElement & e1 = ds.GetDataElement(tmodel);
				if (!e1.IsEmpty() &&
					!e1.IsUndefinedLength() &&
					e1.GetByteValue())
				{
					QByteArray ba1(
						e1.GetByteValue()->GetPointer(),
						e1.GetByteValue()->GetLength());
					const QString tmp1 =
						CodecUtils::toUTF8(&ba1, charset.toLatin1().constData());
					model_s = tmp1.trimmed().remove(QChar('\0'));
				}
			}
		}
		if (!model_s.isEmpty())
		{
			if (!manufacturer_s.isEmpty())
				manufacturer_s.append(QString("<br />"));
			manufacturer_s.append(model_s);
		}
		if (!manufacturer_s.isEmpty())
			ivariant->hardware = std::move(manufacturer_s);
	}
	{
		const mdcm::DataElement & e = ds.GetDataElement(tinstituion);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			ivariant->institution = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	{
		const mdcm::DataElement & e = ds.GetDataElement(tstudydesc);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			ivariant->study_description = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	{
		const mdcm::DataElement & e = ds.GetDataElement(tseriesdesc);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			ivariant->series_description = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	{
		const mdcm::DataElement & e = ds.GetDataElement(tpatientname);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty()) ivariant->pat_name = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	{
		const mdcm::DataElement & e = ds.GetDataElement(tpatientid);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty()) ivariant->pat_id = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	{
		QString birthdate;
		if (get_string_value(ds, tbirthdate, birthdate))
			ivariant->pat_birthdate = birthdate.remove(QChar('\0'));
	}
	{
		QString patweight;
		if (get_string_value(ds, tpatweight, patweight))
			ivariant->pat_weight = std::move(patweight);
	}
	{
		QString sex;
		if (get_string_value(ds, tsex, sex))
			ivariant->pat_sex = sex.remove(QChar('\0'));
	}
	{
		QString fieldstrength;
		if (get_string_value(ds, tfieldstrength, fieldstrength))
		{
			fieldstrength = fieldstrength.trimmed();
			bool conv_ok{};
			const double fieldstrength_ =
				QVariant(fieldstrength).toDouble(&conv_ok);
			if (conv_ok)
			{
				fieldstrength.prepend(
					QString("Magnet field strength "));
				if (fieldstrength_ < 50.0)
					fieldstrength.append(QString(" T")); // FIXME
				ivariant->hardware_info = std::move(fieldstrength);
			}
		}
	}
	{
		QString studyuid;
		if (get_string_value(ds, tstudyuid, studyuid))
			ivariant->study_uid = studyuid.remove(QChar('\0'));
	}
	{
		QString seriesuid;
		if (get_string_value(ds, tseriesuid, seriesuid))
			ivariant->series_uid = seriesuid.remove(QChar('\0'));
	}
	{
		QString studyid;
		if (get_string_value(ds, tstudyid, studyid))
			ivariant->study_id = studyid.remove(QChar('\0'));
	}
	{
		QString frame_of_refuid;
		if (get_string_value(ds, tframe_of_refuid, frame_of_refuid))
			ivariant->frame_of_ref_uid = frame_of_refuid.remove(QChar('\0'));
	}
	{
		const mdcm::DataElement & e = ds.GetDataElement(tcomment);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			const QString tmp0 =
				CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty())
			{
				ivariant->comment = tmp0.trimmed().remove(QChar('\0'));
			}
		}
	}
	{
		QString interpretation;
		if (get_string_value(ds, tinterpretation, interpretation))
			ivariant->interpretation = interpretation.remove(QChar('\0'));
	}
	{
		const mdcm::DataElement & e = ds.GetDataElement(tprivcomment);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			const QString tmp0 =
				CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty())
			{
				if (!ivariant->comment.isEmpty())
					ivariant->comment += QString("\n\n");
				ivariant->comment += tmp0.trimmed().remove(QChar('\0'));
			}
		}
	}
	{
		unsigned short PixelRepresentation;
		if (get_us_value(ds, tpixelrepresentation, &PixelRepresentation))
		{
			if (PixelRepresentation == 1) ivariant->dicom_pixel_signed = true;
		}
	}
}

bool DicomUtils::get_patient_position(
	const QString & p,
	double * pp)
{
	if (pp == nullptr || p.isEmpty()) return false;
	QString tmp0 = p.trimmed().remove(QChar('\0'));
	if (tmp0.contains(QString(",")))
	{
		// Workaround invalid VR
		tmp0.replace(QString(","), QString("."));
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList list = tmp0.split(QString("\\"), Qt::SkipEmptyParts);
#else
	const QStringList list = tmp0.split(QString("\\"), QString::SkipEmptyParts);
#endif
	if (list.size() == 3)
	{
		bool ok{};
		pp[0] = list.at(0).toDouble(&ok);
		if (!ok) pp[0] = list.at(0).toInt(&ok);
		if (!ok) return false;
		pp[1] = list.at(1).toDouble(&ok);
		if (!ok) pp[1] = list.at(1).toInt(&ok);
		if (!ok) return false;
		pp[2] = list.at(2).toDouble(&ok);
		if (!ok) pp[2] = list.at(2).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	return false;
}

bool DicomUtils::get_patient_orientation(
	const QString & o,
	double * po)
{
	if (po == nullptr || o.isEmpty()) return false;
	QString tmp0 = o.trimmed().remove(QChar('\0'));
	if (tmp0.contains(QString(",")))
	{
		// Workaround invalid VR
		tmp0.replace(QString(","), QString("."));
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList list = tmp0.split(QString("\\"), Qt::SkipEmptyParts);
#else
	const QStringList list = tmp0.split(QString("\\"), QString::SkipEmptyParts);
#endif
	if (list.size() == 6)
	{
		bool ok{};
		po[0] = list.at(0).toDouble(&ok);
		if (!ok) po[0] = list.at(0).toInt(&ok);
		if (!ok) return false;
		po[1] = list.at(1).toDouble(&ok);
		if (!ok) po[1] = list.at(1).toInt(&ok);
		if (!ok) return false;
		po[2] = list.at(2).toDouble(&ok);
		if (!ok) po[2] = list.at(2).toInt(&ok);
		if (!ok) return false;
		po[3] = list.at(3).toDouble(&ok);
		if (!ok) po[3] = list.at(3).toInt(&ok);
		if (!ok) return false;
		po[4] = list.at(4).toDouble(&ok);
		if (!ok) po[4] = list.at(4).toInt(&ok);
		if (!ok) return false;
		po[5] = list.at(5).toDouble(&ok);
		if (!ok) po[5] = list.at(5).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	return false;
}

bool DicomUtils::get_pixel_spacing(
	const QString & s,
	double * ps)
{
	if (s.isEmpty()) return false;
	QString tmp0 = s.trimmed().remove(QChar('\0'));
	if (tmp0.contains(QString(",")))
	{
		// Workaround invalid VR
		tmp0.replace(QString(","), QString("."));
	}
	const QStringList list = tmp0.split(QString("\\"));
	bool ok{};
	if (list.size() == 1)
	{
		ps[0] = ps[1] = list.at(0).toDouble(&ok);
		if (!ok) ps[0] = ps[1] = list.at(0).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	else if (list.size() == 2)
	{
		ps[0] = list.at(0).toDouble(&ok);
		if (!ok) ps[0] = list.at(0).toInt(&ok);
		if (!ok) return false;
		ps[1] = list.at(1).toDouble(&ok);
		if (!ok) ps[1] = list.at(1).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	return false;
}

bool DicomUtils::generate_geometry(
		std::vector<ImageSlice*> & cubeslices,
		const std::vector<double*> & values,
		const unsigned int rows_, const unsigned int columns_,
		const double spacing_x, const double spacing_y, double * spacing_z,
		bool * equi_,
		bool * one_direction_,
		double * origin_x,  double * origin_y,  double * origin_z,
		double * dircos,
		float * slices_dir_x, float * slices_dir_y, float * slices_dir_z,
		float * up_dir_x, float * up_dir_y, float * up_dir_z,
		float * center_x, float * center_y, float * center_z,
		float tolerance)
{
	const unsigned int size_ = values.size();
	if (size_ < 1) return false;
	sVector3 first = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 last  = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 v0 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 v1 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 up = sVector3(0.0f, 0.0f, 0.0f);
	QString tmp0;
	bool tmp1{true};
	bool tmp2{true};
	sVector3 tmp_p0 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 tmp_p1 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 tmp_p2 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 tmp_p3 = sVector3(0.0f, 0.0f, 0.0f);
	float tmp_length0{};
	float tmp_length1{};
	float tmp_length2{};
	float tmp_length3{};
	double spacing_z_{};
	bool invalidate_volume{};
	for (unsigned int i = 0; i < size_; ++i)
	{
		const double * ipp_iop = values.at(i);
		const sVector4 c0 = sVector4(
					static_cast<float>(ipp_iop[3] * spacing_x),
					static_cast<float>(ipp_iop[4] * spacing_x),
					static_cast<float>(ipp_iop[5] * spacing_x),
					0.0f);
		const sVector4 c1 = sVector4(
					static_cast<float>(ipp_iop[6] * spacing_y),
					static_cast<float>(ipp_iop[7] * spacing_y),
					static_cast<float>(ipp_iop[8] * spacing_y),
					0.0f);
		const sVector4 c2 = sVector4(0.0f, 0.0f, 0.0f, 0.0f);
		const sVector4 c3 = sVector4(
					static_cast<float>(ipp_iop[0]),
					static_cast<float>(ipp_iop[1]),
					static_cast<float>(ipp_iop[2]),
					1.0f);
#if 0
		std::cout << "ipp[0]=" << ipp_iop[0] << std::endl;
		std::cout << "ipp[1]=" << ipp_iop[1] << std::endl;
		std::cout << "ipp[2]=" << ipp_iop[2] << std::endl;
#endif
		sMatrix4 m0 = sMatrix4::identity();
		m0.setCol0(c0);
		m0.setCol1(c1);
		m0.setCol2(c2);
		m0.setCol3(c3);
		const float r_ = rows_ - 1;
		const float c_ = columns_ - 1;
		const sVector4 ind0 = sVector4(0.0f ,  r_, 0.0f, 1.0f);
		const sVector4 ind1 = sVector4(0.0f, 0.0f, 0.0f, 1.0f);
		const sVector4 ind2 = sVector4(  c_,   r_, 0.0f, 1.0f);
		const sVector4 ind3 = sVector4(  c_, 0.0f, 0.0f, 1.0f);
		const sVector4 p0 = m0*ind0;
		const sVector4 p1 = m0*ind1;
		const sVector4 p2 = m0*ind2;
		const sVector4 p3 = m0*ind3;
		const float x0 = p0.getX(), y0 = p0.getY(), z0 = p0.getZ();
		const float x1 = p1.getX(), y1 = p1.getY(), z1 = p1.getZ();
		const float x2 = p2.getX(), y2 = p2.getY(), z2 = p2.getZ();
		const float x3 = p3.getX(), y3 = p3.getY(), z3 = p3.getZ();
#if 0
		std::cout << "x0=" << x0 << " y0=" << y0 << " z0=" << z0 << std::endl;
		std::cout << "x1=" << x1 << " y1=" << y1 << " z1=" << z1 << std::endl;
		std::cout << "x2=" << x2 << " y2=" << y2 << " z2=" << z2 << std::endl;
		std::cout << "x3=" << x3 << " y3=" << y3 << " z3=" << z3 << std::endl;
#endif
		const QString orientation_string = CommonUtils::get_orientation2(&ipp_iop[3]);
		if (i == 0)
		{
			first = sVector3(x0, y0, z0);
			v0 = (p0.getXYZ() + p3.getXYZ()) * 0.5f;
			sVector3 tmp_up0 = sVector3(x1, y1, z1);
			sVector3 tmp_up1 = sVector3(x0, y0, z0);
			up = normalize(tmp_up1 - tmp_up0);
			*origin_x = ipp_iop[0];
			*origin_y = ipp_iop[1];
			*origin_z = ipp_iop[2];
			for (int j = 0; j < 6; ++j) dircos[j] = ipp_iop[3 + j];
		}
		if (i == size_ - 1)
		{
			last = sVector3(x0, y0, z0);
			v1 = (p0.getXYZ()+p3.getXYZ()) * 0.5f;
		}
		CommonUtils::generate_cubeslice(
			cubeslices,
			orientation_string,
			size_, i,
			x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3,
			ipp_iop);
		const float length0 = Vectormath::Scalar::length(p0.getXYZ()-tmp_p0);
		const float length1 = Vectormath::Scalar::length(p1.getXYZ()-tmp_p1);
		const float length2 = Vectormath::Scalar::length(p2.getXYZ()-tmp_p2);
		const float length3 = Vectormath::Scalar::length(p3.getXYZ()-tmp_p3);
		// check orientation
		if (i != 0)
		{
			if (tmp1)
			{
				if (tmp0 != orientation_string) tmp1 = false;
				if ((length0 > (length1+tolerance))||(length0 < (length1-tolerance)))
					tmp1 = false;
				if ((length1 > (length2+tolerance))||(length1 < (length2-tolerance)))
					tmp1 = false;
				if ((length2 > (length3+tolerance))||(length2 < (length3-tolerance)))
					tmp1 = false;
				if ((length3 > (length0+tolerance))||(length3 < (length0-tolerance)))
					tmp1 = false;
			}
		}
		// check equidistance, requires 3 slices
		if (i >= 2)
		{
			if (tmp2)
			{
				if ((length0 > (tmp_length0+tolerance))||(length0 < (tmp_length0-tolerance)))
					tmp2 = false;
				if ((length1 > (tmp_length1+tolerance))||(length1 < (tmp_length1-tolerance)))
					tmp2 = false;
				if ((length2 > (tmp_length2+tolerance))||(length2 < (tmp_length2-tolerance)))
					tmp2 = false;
				if ((length3 > (tmp_length3+tolerance))||(length3 < (tmp_length3-tolerance)))
					tmp2 = false;
			}
		}
#if 0
		std::cout << "length0=" << length0 << std::endl;
		std::cout << "length1=" << length1 << std::endl;
		std::cout << "length2=" << length2 << std::endl;
		std::cout << "length3=" << length3 << std::endl;
		std::cout << "tmp1=" << tmp1 << std::endl;
		std::cout << "tmp2=" << tmp2 << std::endl;
		std::cout << "------------------------" << std::endl;
#endif
		if (i == size_ - 1)
		{
			spacing_z_ = length1;
#if 0
			std::cout << "spacing_z_=" << spacing_z_ << std::endl;
#endif
		}
		tmp0 = orientation_string;
		tmp_p0 = p0.getXYZ();
		tmp_p1 = p1.getXYZ();
		tmp_p2 = p2.getXYZ();
		tmp_p3 = p3.getXYZ();
		tmp_length0 = length0;
		tmp_length1 = length1;
		tmp_length2 = length2;
		tmp_length3 = length3;
	}
	const float row_dircos_x = static_cast<float>(dircos[0]);
	const float row_dircos_y = static_cast<float>(dircos[1]);
	const float row_dircos_z = static_cast<float>(dircos[2]);
	const float col_dircos_x = static_cast<float>(dircos[3]);
	const float col_dircos_y = static_cast<float>(dircos[4]);
	const float col_dircos_z = static_cast<float>(dircos[5]);
	const float nrm_dircos_x = row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y;
	const float nrm_dircos_y = row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const float nrm_dircos_z = row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	const sVector3 direction1 = normalize(sVector3(nrm_dircos_x, nrm_dircos_y, nrm_dircos_z));
	if (size_ > 1)
	{
		const sVector3 direction0_tmp = last - first;
		if (!(
			(direction0_tmp.getX() > -0.00001f && direction0_tmp.getX() < 0.00001f) &&
			(direction0_tmp.getY() > -0.00001f && direction0_tmp.getY() < 0.00001f) &&
			(direction0_tmp.getZ() > -0.00001f && direction0_tmp.getZ() < 0.00001f)
			))
		{
			const sVector3 direction0 = normalize(direction0_tmp);
			*slices_dir_x = direction0.getX();
			*slices_dir_y = direction0.getY();
			*slices_dir_z = direction0.getZ();
			if (tmp1 &&
				((tmp2 && (size_ > 2)) || size_ == 2) &&
				!(
				(direction0.getX() < direction1.getX() + 0.001f && direction0.getX() > direction1.getX() - 0.001f) &&
				(direction0.getY() < direction1.getY() + 0.001f && direction0.getY() > direction1.getY() - 0.001f) &&
				(direction0.getZ() < direction1.getZ() + 0.001f && direction0.getZ() > direction1.getZ() - 0.001f)
				))
			{
				invalidate_volume = true;
#ifdef ALIZA_VERBOSE
				std::cout
					<< "Warning:\n"
					<< "Direction cosines defined in DICOM file: "
					<< row_dircos_x << "\\" << row_dircos_y << "\\" << row_dircos_z << '\\'
					<< col_dircos_x << "\\" << col_dircos_y << "\\" << col_dircos_z << '\n'
					<< " Z direction calculated from defined cosines: "
					<< direction1.getX() << "," << direction1.getY() << "," << direction1.getZ() << '\n'
					<< " Z direction from geometry (real): "
					<< direction0.getX() << "," << direction0.getY() << "," << direction0.getZ() << '\n'
					<< " ... using image as non-uniform.\n"
					<< std::endl;
#endif
			}
		}
		else
		{
			*slices_dir_x = direction1.getX();
			*slices_dir_y = direction1.getY();
			*slices_dir_z = direction1.getZ();
			invalidate_volume = true;
		}
	}
	else
	{
		*slices_dir_x = direction1.getX();
		*slices_dir_y = direction1.getY();
		*slices_dir_z = direction1.getZ();
		*equi_ = true;
	}
	//
	if (invalidate_volume)
	{
		*equi_ = false;
	}
	else
	{
		if      (size_ >  2) *equi_ = (tmp1 && tmp2);
		else if (size_ == 2) *equi_ = tmp1;
	}
	//
	*up_dir_x = up.getX();
	*up_dir_y = up.getY();
	*up_dir_z = up.getZ();
	if (*equi_ == true)
	{
		const sVector3 cube_center = 0.5f * (v0 + v1);
		*center_x = cube_center.getX();
		*center_y = cube_center.getY();
		*center_z = cube_center.getZ();
		*one_direction_ = true;
	}
	else
	{
		if (tmp1 && size_ > 1) *one_direction_ = true;
		else *one_direction_ = false;
	}
	*spacing_z = (size_ > 1) ? spacing_z_ : 1.0;
	return true;
}

bool DicomUtils::build_gems_dictionary(QMap<QString, int> & m, const mdcm::DataSet & ds)
{
	const mdcm::PrivateTag t0(0x7fe1,0x1, "GEMS_Ultrasound_MovieGroup_001");
	const mdcm::PrivateTag t1(0x7fe1,0x70,"GEMS_Ultrasound_MovieGroup_001");
	const mdcm::PrivateTag t2(0x7fe1,0x71,"GEMS_Ultrasound_MovieGroup_001");
	const mdcm::PrivateTag t3(0x7fe1,0x72,"GEMS_Ultrasound_MovieGroup_001");
	const mdcm::DataElement & s0 = ds.GetDataElement(t0);
	if (s0.IsEmpty()) return false;
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq0 = s0.GetValueAsSQ();
	if (!(sq0 && sq0->GetNumberOfItems() == 1)) return false; // 1 item ?
	mdcm::Item & i0 = sq0->GetItem(1);
	mdcm::DataSet & subds0 = i0.GetNestedDataSet();
	const mdcm::DataElement& s1 = subds0.GetDataElement(t1);
	if (s1.IsEmpty()) return false;
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 = s1.GetValueAsSQ();
	if (!(sq1 && sq1->GetNumberOfItems() > 0)) return false;
	const size_t n = sq1->GetNumberOfItems();
	for (size_t x = 0; x < n; ++x)
	{
		mdcm::Item & i = sq1->GetItem(x + 1);
		mdcm::DataSet & s = i.GetNestedDataSet();
		const mdcm::DataElement & index = s.GetDataElement(t2);
		const mdcm::DataElement & name  = s.GetDataElement(t3);
		if (index.IsEmpty() || name.IsEmpty()) continue;
		//
		const mdcm::ByteValue * bv0 = index.GetByteValue();
		if (!bv0) continue;
		const unsigned int l0 = bv0->GetLength();
		std::vector<unsigned int> result0;
		if (l0 % 4 == 0 && l0 >= 4)
		{
			char * buffer0 = new char[l0];
			const bool ok0 = bv0->GetBuffer(buffer0, l0);
			if (ok0)
			{
				void * vtmp0 = static_cast<void*>(buffer0);
				int * tmp0 = static_cast<int*>(vtmp0);
				for (unsigned int x0 = 0; x0 < l0 / 4; ++x0)
				{
					result0.push_back(tmp0[x0]);
				}
				delete [] buffer0;
			}
			else
			{
				delete [] buffer0;
				continue;
			}
		}
		else
		{
			continue;
		}
		//
		const mdcm::ByteValue * bv1 = name.GetByteValue();
		if (!bv1) continue;
		QString tmp0 = QString::fromLatin1(bv1->GetPointer(), bv1->GetLength());
		if (!result0.empty()) m[tmp0.trimmed()] = result0[0];
	}
	return true;
}

void DicomUtils::read_gems_params(
	QMap<int, GEMSParam> & m,
	const mdcm::DataElement & de,
	const QMap<QString, int> & dict)
{
	const mdcm::PrivateTag tindex  (0x7fe1,0x48,"GEMS_Ultrasound_MovieGroup_001"); // index
	const mdcm::PrivateTag t_int   (0x7fe1,0x49,"GEMS_Ultrasound_MovieGroup_001"); // UL
	const mdcm::PrivateTag t_float1(0x7fe1,0x51,"GEMS_Ultrasound_MovieGroup_001"); // FL
	const mdcm::PrivateTag t_float (0x7fe1,0x52,"GEMS_Ultrasound_MovieGroup_001"); // FD
	const mdcm::PrivateTag t_ul    (0x7fe1,0x53,"GEMS_Ultrasound_MovieGroup_001"); // UL
	const mdcm::PrivateTag t_sl    (0x7fe1,0x54,"GEMS_Ultrasound_MovieGroup_001"); // SL
	const mdcm::PrivateTag t_ob    (0x7fe1,0x55,"GEMS_Ultrasound_MovieGroup_001"); // OB
	const mdcm::PrivateTag t_text  (0x7fe1,0x57,"GEMS_Ultrasound_MovieGroup_001"); // LT
	const mdcm::PrivateTag t_fd_n1 (0x7fe1,0x77,"GEMS_Ultrasound_MovieGroup_001"); // FD / 1-N
	const mdcm::PrivateTag t_sl_n  (0x7fe1,0x79,"GEMS_Ultrasound_MovieGroup_001"); // SL / 1-N
	const mdcm::PrivateTag t_sl4   (0x7fe1,0x86,"GEMS_Ultrasound_MovieGroup_001"); // SL / 4
	const mdcm::PrivateTag t_fd_n  (0x7fe1,0x87,"GEMS_Ultrasound_MovieGroup_001"); // FD / 1-N
	const mdcm::PrivateTag t_fd2   (0x7fe1,0x88,"GEMS_Ultrasound_MovieGroup_001"); // FD / 2

	mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
	if (!(sq && sq->GetNumberOfItems() > 0)) return;
	const size_t n = sq->GetNumberOfItems();
	for (size_t i = 1; i <= n; ++i)
	{
		mdcm::Item & item = sq->GetItem(i);
		mdcm::DataSet & subds = item.GetNestedDataSet();
		const mdcm::DataElement & index = subds.GetDataElement(tindex);
		if (index.IsEmpty()) continue;
		const mdcm::ByteValue * bv0 = index.GetByteValue();
		if (!bv0) continue;
		const unsigned int l0 = bv0->GetLength();
		int idx0{-1};
		if (l0 % 8 == 0 && l0 >= 8)
		{
			std::vector<double> result0;
			char * buffer0 = new char[l0];
			const bool ok0 = bv0->GetBuffer(buffer0, l0);
			if (ok0)
			{
				void * vtmp0 = static_cast<void*>(buffer0);
				double * tmp0 = static_cast<double*>(vtmp0);
				for (unsigned int x0 = 0; x0 < l0 / 8; ++x0)
				{
					result0.push_back(tmp0[x0]);
				}
			}
			delete [] buffer0;
			if (!result0.empty()) idx0 = static_cast<int>(result0[0]);
		}
		else if (l0 == 4)
		{
			std::vector<int> result0;
			char * buffer0 = new char[l0];
			const bool ok0 = bv0->GetBuffer(buffer0, l0);
			if (ok0)
			{
				void * vtmp0 = static_cast<void*>(buffer0);
				int * tmp0 = static_cast<int*>(vtmp0);
				for (size_t x0 = 0; x0 < l0 / 4; ++x0)
				{
					result0.push_back(tmp0[x0]);
				}
			}
			delete [] buffer0;
			if (!result0.empty()) idx0 = result0[0];
		}
		if (idx0 == -1) continue;
		if (subds.FindDataElement(t_int))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_int);
			mdcm::Element<mdcm::VR::UL, mdcm::VM::VM1> e;
			e.SetFromDataElement(v);
			const int j = e.GetValue();
			GEMSParam p;
			QList<QVariant> l;
			l << QVariant(j);
			p.type = 0x49;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_float1))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_float1);
			mdcm::Element<mdcm::VR::FL, mdcm::VM::VM1> e;
			e.SetFromDataElement(v);
			const double j = e.GetValue();
			GEMSParam p;
			QList<QVariant> l;
			l << QVariant(j);
			p.type = 0x51;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_float))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_float);
			mdcm::Element<mdcm::VR::FD, mdcm::VM::VM1> e;
			e.SetFromDataElement(v);
			const double j = e.GetValue();
			GEMSParam p;
			QList<QVariant> l;
			l << QVariant(j);
			p.type = 0x52;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_ul))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_ul);
			mdcm::Element<mdcm::VR::UL, mdcm::VM::VM1> e;
			e.SetFromDataElement(v);
			const int j = e.GetValue();
			GEMSParam p;
			QList<QVariant> l;
			l << QVariant(j);
			p.type = 0x53;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_sl))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_sl);
			mdcm::Element<mdcm::VR::SL, mdcm::VM::VM1> e;
			e.SetFromDataElement(v);
			const int j = e.GetValue();
			GEMSParam p;
			QList<QVariant> l;
			l << QVariant(j);
			p.type = 0x54;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_ob))
		{
			const mdcm::DataElement & e = subds.GetDataElement(t_ob);
			const mdcm::ByteValue * v = e.GetByteValue();
			if (v)
			{
				GEMSParam p;
				QList<QVariant> l;
#if 1
				l << QVariant(QString("<span class='y5'>&#160;binary (OB)</span>"));
#else
				const char * b = v->GetPointer();
				const unsigned int s = v->GetLength();
				const QByteArray j = QByteArray(b, s);
				l << QVariant(j);
#endif
				p.type = 0x55;
				p.values = std::move(l);
				m[idx0] = std::move(p);
			}
		}
		else if (subds.FindDataElement(t_text))
		{
			const mdcm::DataElement & e = subds.GetDataElement(t_text);
			const mdcm::ByteValue * v = e.GetByteValue();
			QString j;
			if (v)
			{
				const char * b = v->GetPointer();
				const unsigned long long s = v->GetLength();
				j = QString::fromLatin1(b, s);
			}
			GEMSParam p;
			QList<QVariant> l;
			l << QVariant(j);
			p.type = 0x57;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_sl_n))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_sl_n);
			mdcm::Element<mdcm::VR::SL, mdcm::VM::VM1_n> e;
			e.SetFromDataElement(v);
			GEMSParam p;
			QList<QVariant> l;
			for (unsigned int z = 0; z < e.GetLength(); ++z)
			{
				const int j = e.GetValue(z);
				l << QVariant(j);
			}
			p.type = 0x79;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_sl4))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_sl4);
			mdcm::Element<mdcm::VR::SL, mdcm::VM::VM4> e;
			e.SetFromDataElement(v);
			if (e.GetLength() == 4)
			{
				GEMSParam p;
				QList<QVariant> l;
				for (unsigned int z = 0; z < 4; ++z)
				{
					const int j = e.GetValue(z);
					l << QVariant(j);
				}
				p.type = 0x86;
				p.values = std::move(l);
				m[idx0] = std::move(p);
			}
		}
		else if (subds.FindDataElement(t_fd_n1))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_fd_n1);
			mdcm::Element<mdcm::VR::FD, mdcm::VM::VM1_n> e;
			e.SetFromDataElement(v);
			GEMSParam p;
			QList<QVariant> l;
			for (unsigned int z = 0; z < e.GetLength(); ++z)
			{
				const double j = e.GetValue(z);
				l << QVariant(j);
			}
			p.type = 0x77;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_fd_n))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_fd_n);
			mdcm::Element<mdcm::VR::FD, mdcm::VM::VM1_n> e;
			e.SetFromDataElement(v);
			GEMSParam p;
			QList<QVariant> l;
			for (unsigned int z = 0; z < e.GetLength(); ++z)
			{
				const double j = e.GetValue(z);
				l << QVariant(j);
			}
			p.type = 0x87;
			p.values = std::move(l);
			m[idx0] = std::move(p);
		}
		else if (subds.FindDataElement(t_fd2))
		{
			const mdcm::DataElement & v = subds.GetDataElement(t_fd2);
			mdcm::Element<mdcm::VR::FD, mdcm::VM::VM1_n> e;
			e.SetFromDataElement(v);
			if (e.GetLength() == 2)
			{
				GEMSParam p;
				QList<QVariant> l;
				for (unsigned int z = 0; z < 2; ++z)
				{
					const double j = e.GetValue(z);
					l << QVariant(j);
				}
				p.type = 0x88;
				p.values = std::move(l);
				m[idx0] = std::move(p);
			}
		}
		else
		{
			GEMSParam p;
			QList<QVariant> l;
			l << QVariant(QString("Unknown"));
			p.type = 0;
			p.values = std::move(l);
			m[idx0] = std::move(p);
#if 0
			std::cout << "Unknown !" << idx0 << std::endl;
#endif
		}
	}
//
//
// print GEMS parameters
//
//
#if 0
	std::cout << std::endl;
	QMap<int, GEMSParam>::const_iterator it = m.cbegin();
	QList<int> dict_values = dict.values();
	while (it != m.cend())
	{
		if (dict_values.count(it.key()) == 1)
		{
			QList<QString> keys = dict.keys(it.key());
			if (keys.size() == 1)
			{
				const QString name = keys.at(0);
				std::cout << name.toStdString() << ": ";
				const GEMSParam & p = it.value();
				for (int x = 0; x < p.values.size(); ++x)
				{
					std::cout << " "
						<< p.values.at(x).toString().toStdString();
				}
				std::cout << std::endl;
			}
			else
			{
				std::cout
					<< "Something went wrong (1)? key="
					<< it.key()
					<< " dictionary keys size ="
					<< keys.size() << std::endl;
			}
		}
		else
		{
			std::cout
				<< "Something went wrong (2)? dictionary values count("
				<< it.key() << ")="
				<< dict_values.count(it.key())
				<< std::endl;
		}
		++it;
	}
	std::cout << std::endl;
#endif
}

void DicomUtils::enhanced_get_indices(
	const DimIndexSq & sq,
	int * dim8th,
	int * dim7th,
	int * dim6th,
	int * dim5th,
	int * dim4th,
	int * dim3rd,
	int * enh_id,
	const short enh_loading_type)
{
	// If the function fails to process indices,
	// an image still will have correct spatial information
	// and acquisition time for every slice, if available.
	const size_t sq_size = sq.size();
	if (sq_size < 1)
	{
#ifdef ENHANCED_PRINT_INFO
		std::cout << "Warning: empty Dimension Index Sequence" << std::endl;
#endif
		return;
	}
	const EnhancedIODLoadingType loading_type = static_cast<EnhancedIODLoadingType>(enh_loading_type);
	int stack_id_idx{-1};
	int in_stack_pos_idx{-1};
	int ipp_idx{-1};
	int temporal_pos_idx{-1};
	int temporal_off_idx{-1};
	int contrast_idx{-1};
	int b_value_idx{-1};
	int gradients_idx{-1};
	int lut_label_idx{-1};
	int plane_pos_idx{-1};
	int datatype_idx{-1};
	int mr_frame_type_idx{-1};
	int mr_eff_echo_idx{-1};
	int segment_idx{-1};
	int pa_datatype_idx{-1};
	int pa_excwave_idx{-1};
	int rec_alg_idx{-1};
	std::string dim_uid;
	std::vector<std::string> dim_uids;
	for (size_t x = 0; x < sq_size; ++x)
	{
		dim_uids.push_back(sq.at(x).uid);
	}
	const std::set<std::string> dim_uids_set(dim_uids.begin(), dim_uids.end());
#ifdef ENHANCED_PRINT_INFO
	if (dim_uids_set.size() > 1)
	{
		std::cout << "Warning: multiple dimension organization UIDs, using 1st" << std::endl;
	}
#endif
	{
		std::set<std::string>::const_iterator it = dim_uids_set.cbegin();
		dim_uid = *it;
	}
	DimIndexSq sq1;
	for (size_t x = 0; x < sq_size; ++x)
	{
		if (sq.at(x).uid == dim_uid)
		{
			sq1.push_back(sq.at(x));
		}
	}
	const int sq1_size = static_cast<int>(sq1.size());
	if (sq1_size > 6)
	{
#ifdef ENHANCED_PRINT_INFO
		std::cout << "Size of Dimension Organization > 6 "
			<< "(" << sq1_size << ") is currently not supported" << std::endl;
#endif
		return;
	}
#ifdef ENHANCED_PRINT_INFO
	std::cout << "Using Dimension Organization UID " << dim_uid << std::endl;
#endif
	//
	if (loading_type == EnhancedIODLoadingType::StrictMultipleImages)
	{
#ifdef ENHANCED_PRINT_INFO
		std::cout << "Strictly using \"Dimension Organization\" (multiple images)" << std::endl;
#endif
		goto strict;
	}
	else if (loading_type == EnhancedIODLoadingType::StrictSingleImage)
	{
#ifdef ENHANCED_PRINT_INFO
		std::cout << "Strictly using \"Dimension Organization\" (single image)" << std::endl;
#endif
		goto strict;
	}
	else if (loading_type == EnhancedIODLoadingType::PreferUniformVolumes)
	{
#ifdef ENHANCED_PRINT_INFO
		std::cout << "Using \"Dimension Organization\", trying to generate uniform volumes" << std::endl;
#endif
	}
	else
	{
#ifdef ENHANCED_PRINT_INFO
		std::cout << "Not using \"Dimension Organization\"" << std::endl;
#endif
		return;
	}
	// Search known pointers for well-know combinations (optional).
	for (int x = 0; x < sq1_size; ++x)
	{
		const size_t i = static_cast<size_t>(x);
		if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9056))
		{
			stack_id_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9057))
		{
			in_stack_pos_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9113) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x0032))
		{
			ipp_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9128))
		{
			temporal_pos_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9310) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x930d))
		{
			temporal_off_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9117) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9087))
		{
			b_value_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9117) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9089))
		{
			gradients_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9341) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9344))
		{
			contrast_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0040,0x9096) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0040,0x9210))
		{
			lut_label_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x930e) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9301))
		{
			plane_pos_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9807) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9808))
		{
			datatype_idx = x;
		}
		//
		//
		// Photoacoustic
		// TODO check again
		//
		// Photoacoustic seems to skip group pointer sometimes.
		//
		// Seems to be similar with the above one.
		else if (sq1.at(i).group_pointer == mdcm::Tag(0xffff,0xffff) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9807))
		{
			pa_datatype_idx = x;
		}
		else if ((sq1.at(i).group_pointer == mdcm::Tag(0xffff,0xffff) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9825)) // In the example in sup229 (final)
			||
			(sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9825) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9826)) // Trying to guess another possible
			||
			// In C.8.34.1.2 Photoacoustic Dimension Organization Type is
			// mentioned that Excitation Characteristics Sequence (0018,9821),
			// may be used as dimension, not clear with group pointer or without.
			(sq1.at(i).group_pointer == mdcm::Tag(0xffff,0xffff) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9821))
			||
			(sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9821) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9826)))
		{
			pa_excwave_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0xffff,0xffff) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x993d)) // Trying to guess
		{
			rec_alg_idx = x;
		}
		//
		//
		//
		//
		//
		//
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9226) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0008,0x9007))
		{
			mr_frame_type_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0018,0x9114) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0018,0x9082))
		{
			mr_eff_echo_idx = x;
		}
		else if (sq1.at(i).group_pointer == mdcm::Tag(0x0062,0x000a) &&
			sq1.at(i).index_pointer == mdcm::Tag(0x0062,0x000b))
		{
			segment_idx = x;
		}
		// currently unused
		(void)segment_idx;
		(void)ipp_idx;
	}
	//
	// Known combinations, re-order indices in some special situations to
	// try to get a uniform volume instead of an image with repeating slices.
	// S. "Settings" widget for "Strict" options to skip.
	//
	// Data type
	if (sq1_size == 3 &&
		temporal_off_idx >= 0 &&
		plane_pos_idx >= 0 &&
		datatype_idx >= 0)
	{
		*dim5th = datatype_idx;
		*dim4th = temporal_off_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 100;
	}
	else if (sq1_size == 3 &&
		temporal_pos_idx >= 0 &&
		plane_pos_idx >= 0 &&
		datatype_idx >= 0)
	{
		*dim5th = datatype_idx;
		*dim4th = temporal_pos_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 101;
	}
	else if (sq1_size == 2 &&
		plane_pos_idx == 0 &&
		datatype_idx == 1)
	{
		*dim4th = datatype_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 102;
	}
	// Contrast
	else if (sq1_size == 4 &&
		temporal_pos_idx >= 0 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		contrast_idx >= 0)
	{
		*dim6th = contrast_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 103;
	}
	else if (sq1_size == 4 &&
		temporal_off_idx >= 0 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		contrast_idx >= 0)
	{
		*dim6th = contrast_idx;
		*dim5th = temporal_off_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 104;
	}
	else if (sq1_size == 3 &&
		in_stack_pos_idx == 0 &&
		stack_id_idx == 1 &&
		contrast_idx == 2)
	{
		*dim5th = contrast_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 105;
	}
	else if (sq1_size == 2 &&
		in_stack_pos_idx == 1 &&
		contrast_idx == 0)
	{
		*dim4th = contrast_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 106;
	}
	// MR frame type
	else if (sq1_size == 4 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		temporal_pos_idx >= 0 &&
		mr_frame_type_idx >= 0)
	{
		*dim6th = mr_frame_type_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 107;
	}
	else if (sq1_size == 4 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		temporal_off_idx >= 0 &&
		mr_frame_type_idx >= 0)
	{
		*dim6th = mr_frame_type_idx;
		*dim5th = temporal_off_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 108;
	}
	else if (sq1_size == 3 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		mr_frame_type_idx >= 0)
	{
		*dim5th = mr_frame_type_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 109;
	}
	// MR eff. echo
	else if (sq1_size == 4 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		temporal_pos_idx >= 0 &&
		mr_eff_echo_idx >= 0)
	{
		*dim6th = mr_eff_echo_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 110;
	}
	else if (sq1_size == 4 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		temporal_off_idx >= 0 &&
		mr_eff_echo_idx >= 0)
	{
		*dim6th = mr_eff_echo_idx;
		*dim5th = temporal_off_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 111;
	}
	else if (sq1_size == 3 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		mr_eff_echo_idx >= 0)
	{
		*dim5th = mr_eff_echo_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 112;
	}
	// Diffusion
	else if (sq1_size == 4 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		b_value_idx >= 0 &&
		gradients_idx >= 0)
	{
		*dim6th = b_value_idx;
		*dim5th = gradients_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 113;
	}
	else if (sq1_size == 3 &&
		in_stack_pos_idx >= 0 &&
		b_value_idx >= 0 &&
		gradients_idx >= 0)
	{
		*dim5th = b_value_idx;
		*dim4th = gradients_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 114;
	}
	else if (sq1_size == 3 &&
		stack_id_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		gradients_idx >= 0)
	{
		*dim5th = gradients_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 115;
	}
	else if (sq1_size == 3 &&
		b_value_idx >= 0 &&
		in_stack_pos_idx >= 0 &&
		stack_id_idx >= 0)
	{
		*dim5th = b_value_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 116;
	}
	// LUT
	else if (sq1_size == 4 &&
		temporal_pos_idx == 0 &&
		stack_id_idx == 1 &&
		in_stack_pos_idx == 2 &&
		lut_label_idx == 3)
	{
		*dim6th = lut_label_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 117;
	}
	else if (sq1_size == 3 &&
		temporal_pos_idx == 0 &&
		in_stack_pos_idx == 1 &&
		lut_label_idx == 2)
	{
		*dim5th = lut_label_idx;
		*dim4th = temporal_pos_idx;
		*dim3rd = in_stack_pos_idx;
		*enh_id = 118;
	}
	// Photoacoustic (not sure)
	else if (sq1_size == 3 &&
		temporal_pos_idx >= 0 &&
		plane_pos_idx >= 0 &&
		pa_datatype_idx >= 0)
	{
		*dim5th = pa_datatype_idx;
		*dim4th = temporal_pos_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 119;
	}
	else if (sq1_size == 4 &&
		temporal_pos_idx >= 0 &&
		plane_pos_idx >= 0 &&
		pa_datatype_idx >= 0 &&
		pa_excwave_idx >= 0)
	{
		*dim6th = pa_excwave_idx;
		*dim5th = pa_datatype_idx;
		*dim4th = temporal_pos_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 120;
	}
	else if (sq1_size == 4 &&
		temporal_pos_idx >= 0 &&
		plane_pos_idx >= 0 &&
		pa_datatype_idx >= 0 &&
		rec_alg_idx >= 0)
	{
		*dim6th = rec_alg_idx;
		*dim5th = pa_datatype_idx;
		*dim4th = temporal_pos_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 121;
	}
	//
	if (*enh_id < 0)
	{
		if (sq1_size == 3 &&
			stack_id_idx >= 0 &&
			in_stack_pos_idx >= 0)
		{
			int dim5th_tmp{-1};
			for (int x = 0; x < 3; ++x)
			{
				const size_t i = static_cast<size_t>(x);
				if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9056))
				{
					;
				}
				else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9057))
				{
					;
				}
				else
				{
					dim5th_tmp = x;
					break;
				}
			}
			*dim5th = dim5th_tmp;
			*dim4th = stack_id_idx;
			*dim3rd = in_stack_pos_idx;
			*enh_id = 200;
		}
		else if (sq1_size == 4 &&
			stack_id_idx >= 0 &&
			in_stack_pos_idx >= 0 &&
			temporal_pos_idx >= 0)
		{
			int dim6th_tmp{-1};
			for (int x = 0; x < 4; ++x)
			{
				const size_t i = static_cast<size_t>(x);
				if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9056))
				{
					;
				}
				else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9057))
				{
					;
				}
				else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9128))
				{
					;
				}
				else
				{
					dim6th_tmp = x;
					break;
				}
			}
			*dim6th = dim6th_tmp;
			*dim5th = temporal_pos_idx;
			*dim4th = stack_id_idx;
			*dim3rd = in_stack_pos_idx;
			*enh_id = 201;
		}
		else if (sq1_size == 4 &&
			stack_id_idx >= 0 &&
			in_stack_pos_idx >= 0 &&
			temporal_off_idx >= 0)
		{
			int dim6th_tmp{-1};
			for (int x = 0; x < 4; ++x)
			{
				const size_t i = static_cast<size_t>(x);
				if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9056))
				{
					;
				}
				else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x9057))
				{
					;
				}
				else if (sq1.at(i).group_pointer == mdcm::Tag(0x0020,0x9310) &&
					sq1.at(i).index_pointer == mdcm::Tag(0x0020,0x930d))
				{
					;
				}
				else
				{
					dim6th_tmp = x;
					break;
				}
			}
			*dim6th = dim6th_tmp;
			*dim5th = temporal_off_idx;
			*dim4th = stack_id_idx;
			*dim3rd = in_stack_pos_idx;
			*enh_id = 202;
		}
	}
	//
	//
	//
strict:
	if (*enh_id < 0)
	{
		switch (sq1_size)
		{
		case 6:
			*dim3rd = 5;
			*dim4th = 4;
			*dim5th = 3;
			*dim6th = 2;
			*dim7th = 1;
			*dim8th = 0;
			*enh_id = 1;
			break;
		case 5:
			*dim3rd = 4;
			*dim4th = 3;
			*dim5th = 2;
			*dim6th = 1;
			*dim7th = 0;
			*enh_id = 2;
			break;
		case 4:
			*dim3rd = 3;
			*dim4th = 2;
			*dim5th = 1;
			*dim6th = 0;
			*enh_id = 3;
			break;
		case 3:
			*dim3rd = 2;
			*dim4th = 1;
			*dim5th = 0;
			*enh_id = 4;
			break;
		case 2:
			*dim3rd = 1;
			*dim4th = 0;
			*enh_id = 5;
			break;
		case 1:
			*dim3rd = 0;
			*enh_id = 6;
			break;
		default:
			break;
		}
	}
}

void DicomUtils::enhanced_process_values(
	FrameGroupValues & values,
	const FrameGroupValues & shared_values)
{
	bool vol_pos_miss{};
	bool vol_orient_miss{};
	bool pat_pos_miss{};
	bool pat_orient_miss{};
	bool pix_spacing_miss{};
	bool window_miss{};
	bool laterality_miss{};
	bool body_part_miss{};
	bool rescale_miss{};
	for (unsigned int x = 0; x < values.size(); ++x)
	{
		if (!values.at(x).vol_pos_ok) vol_pos_miss = true;
		if (!values.at(x).vol_orient_ok) vol_orient_miss = true;
		if (values.at(x).pat_pos.isEmpty()) pat_pos_miss = true;
		if (values.at(x).pat_orient.isEmpty()) pat_orient_miss = true;
		if (values.at(x).pix_spacing.isEmpty()) pix_spacing_miss = true;
		if (values.at(x).window_center.isEmpty() || values.at(x).window_width.isEmpty())
		{
			window_miss = true;
		}
		if (values.at(x).frame_laterality.isEmpty()) laterality_miss = true;
		if (values.at(x).frame_body_part.isEmpty()) body_part_miss = true;
		if (!values.at(x).rescale_ok) rescale_miss = true;
	}
	if (vol_pos_miss && shared_values.size() == 1 && shared_values.at(0).vol_pos_ok)
	{
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			values[x].vol_pos[0] = shared_values.at(0).vol_pos[0];
			values[x].vol_pos[1] = shared_values.at(0).vol_pos[1];
			values[x].vol_pos[2] = shared_values.at(0).vol_pos[2];
			values[x].vol_pos_ok = true;
		}
	}
	if (vol_orient_miss && shared_values.size() == 1 && shared_values.at(0).vol_orient_ok)
	{
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			values[x].vol_orient[0] = shared_values.at(0).vol_orient[0];
			values[x].vol_orient[1] = shared_values.at(0).vol_orient[1];
			values[x].vol_orient[2] = shared_values.at(0).vol_orient[2];
			values[x].vol_orient[3] = shared_values.at(0).vol_orient[3];
			values[x].vol_orient[4] = shared_values.at(0).vol_orient[4];
			values[x].vol_orient[5] = shared_values.at(0).vol_orient[5];
			values[x].vol_orient_ok = true;
		}
	}
	if (pat_pos_miss && shared_values.size() == 1 && !shared_values.at(0).pat_pos.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); ++x)
			values[x].pat_pos = shared_values.at(0).pat_pos;
	}
	if (pat_orient_miss && shared_values.size() == 1 && !shared_values.at(0).pat_orient.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); ++x)
			values[x].pat_orient = shared_values.at(0).pat_orient;
	}
	if (pix_spacing_miss && shared_values.size() == 1 && !shared_values.at(0).pix_spacing.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); ++x)
			values[x].pix_spacing = shared_values.at(0).pix_spacing;
	}
	if (window_miss && shared_values.size() == 1 && !shared_values.at(0).window_center.isEmpty() &&
		!shared_values.at(0).window_width.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			values[x].window_center = shared_values.at(0).window_center;
			values[x].window_width  = shared_values.at(0).window_width;
			values[x].lut_function  = shared_values.at(0).lut_function;
		}
	}
	if (laterality_miss && shared_values.size() == 1 && !shared_values.at(0).frame_laterality.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			values[x].frame_laterality = shared_values.at(0).frame_laterality;
		}
	}
	if (body_part_miss && shared_values.size() == 1 && !shared_values.at(0).frame_body_part.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			values[x].frame_body_part = shared_values.at(0).frame_body_part;
		}
	}
	if (rescale_miss)
	{
		double rescale_intercept{};
		double rescale_slope{1.0};
		QString rescale_type;
		if (shared_values.size() == 1 && shared_values.at(0).rescale_ok)
		{
			rescale_intercept = shared_values.at(0).rescale_intercept;
			rescale_slope = shared_values.at(0).rescale_slope;
			rescale_type = shared_values.at(0).rescale_type;
		}
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			values[x].rescale_intercept = rescale_intercept;
			values[x].rescale_slope = rescale_slope;
			values[x].rescale_type = rescale_type;
		}
	}
}

void DicomUtils::enhanced_check_rescale(
	const mdcm::DataSet & ds,
	FrameGroupValues & v)
{
	bool rescale_miss{};
	for (unsigned int x = 0; x < v.size(); ++x)
	{
		if (!v.at(x).rescale_ok)
		{
			rescale_miss = true;
			break;
		}
	}
	if (!rescale_miss) return;
	const mdcm::Tag tRescaleIntercept(0x0028,0x1052);
	const mdcm::Tag tRescaleSlope(0x0028,0x1053);
	std::vector<double> tmp0;
	std::vector<double> tmp1;
	const bool ok0 = DicomUtils::get_ds_values(ds, tRescaleIntercept, tmp0);
	const bool ok1 = DicomUtils::get_ds_values(ds, tRescaleSlope, tmp1);
	if (ok0 && ok1 && !tmp0.empty() && !tmp1.empty())
	{
		for (unsigned int x = 0; x < v.size(); ++x)
		{
			v.at(x).rescale_intercept = tmp0.at(0);
			v.at(x).rescale_slope = tmp1.at(0);
			v.at(x).rescale_ok = true;
		}
	}
}

void DicomUtils::print_sq(const DimIndexSq & sq)
{
#ifdef ENHANCED_PRINT_INFO
	if (!sq.empty())
	{
		std::cout << "Dimension Index Sequence:" << std::endl;
	}
	else
	{
		std::cout << "Dimension Index Sequence is empty" << std::endl;
		return;
	}
	for (size_t i = 0; i < sq.size(); ++i)
	{
		std::cout
			<< " " << i << " "
			<< sq.at(i).group_pointer << " "
			<< sq.at(i).index_pointer << " ";
		if (sq.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0020,0x9056))
		{
			std::cout << "stack_id_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0020,0x9057))
		{
			std::cout << "in_stack_pos_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0020,0x9113) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0020,0x0032))
		{
			std::cout << "ipp_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0020,0x9111) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0020,0x9128))
		{
			std::cout << "temporal_pos_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0018,0x9117) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0018,0x9087))
		{
			std::cout << "b_value_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0018,0x9117) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0018,0x9089))
		{
			std::cout << "gradients_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0018,0x9341) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0018,0x9344))
		{
			std::cout << "contrast_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0040,0x9096) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0040,0x9210))
		{
			std::cout << "lut_label_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0020,0x9310) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0020,0x930d))
		{
			std::cout << "temporal_off_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0020,0x930e) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0020,0x9301))
		{
			std::cout << "plane_pos_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0018,0x9807) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0018,0x9808))
		{
			std::cout << "datatype_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0018,0x9226) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0008,0x9007))
		{
			std::cout << "mr_frame_type_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0018,0x9114) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0018,0x9082))
		{
			std::cout << "mr_eff_echo_idx";
		}
		else if (sq.at(i).group_pointer == mdcm::Tag(0x0062,0x000a) &&
			sq.at(i).index_pointer == mdcm::Tag(0x0062,0x000b))
		{
			std::cout << "segment_idx";
		}
		else
		{
			std::cout << " ***";
		}
		std::cout << " UID " << sq.at(i).uid << std::endl;
	}
#else
	(void)sq;
#endif
}

void DicomUtils::print_func_group(const FrameGroupValues & values)
{
#ifdef ENHANCED_PRINT_GROUPS
	for (unsigned int x = 0; x < values.size(); ++x)
	{
		std::cout << "ID=" << values.at(x).id << std::endl;
		if (values.at(x).stack_id_ok)
		{
			std::cout
				<< "stack_id "
				<< values.at(x).stack_id
				<< std::endl;
		}
		if (values.at(x).in_stack_pos_num_ok)
		{
			std::cout
				<< "in_stack_pos "
				<< values.at(x).in_stack_pos_num
				<< std::endl;
		}
		if (values.at(x).temp_pos_idx_ok)
		{
			std::cout
				<< "temp_pos_idx "
				<< values.at(x).temp_pos_idx
				<< std::endl;
		}
		if (!values.at(x).data_type.isEmpty())
		{
			std::cout
				<< "data_type "
				<< values.at(x).data_type.toStdString()
				<< std::endl;
		}
		if (values.at(x).vol_pos_ok)
		{
			std::cout
				<< "vol_pos "
				<< values.at(x).vol_pos[0]
				<< " " << values.at(x).vol_pos[1]
				<< " " << values.at(x).vol_pos[2]
				<< std::endl;
		}
		if (values.at(x).temp_pos_off_ok)
		{
			std::cout
				<< "temp_pos_off "
				<< values.at(x).temp_pos_off
				<< std::endl;
		}
		if (values.at(x).us_temp_pos_unknown_ok)
		{
			std::cout
				<< "us_temp_pos_unknown "
				<< values.at(x).us_temp_pos_unknown
				<< std::endl;
		}
		if (!values.at(x).pat_pos.isEmpty())
		{
			std::cout
				<< "pat_pos "
				<< values.at(x).pat_pos.toStdString()
				<< std::endl;
		}
		if (!values.at(x).pat_orient.isEmpty())
		{
			std::cout
				<< "pat_orient "
				<< values.at(x).pat_orient.toStdString()
				<< std::endl;
		}
		if (!values.at(x).pix_spacing.isEmpty())
		{
			std::cout
				<< "pix_spacing "
				<< values.at(x).pix_spacing.toStdString()
				<< std::endl;
		}
		if (!values.at(x).slice_thick.isEmpty())
		{
			std::cout
				<< "slice_thick "
				<< values.at(x).slice_thick.toStdString()
				<< std::endl;
		}
		if (!values.at(x).window_center.isEmpty())
		{
			std::cout
				<< "window_center "
				<< values.at(x).window_center.toStdString()
				<< std::endl;
		}
		if (!values.at(x).window_width.isEmpty())
		{
			std::cout
				<< "window_width "
				<< values.at(x).window_width.toStdString()
				<< std::endl;
		}
		if (!values.at(x).lut_function.isEmpty())
		{
			std::cout
				<< "lut_function "
				<< values.at(x).lut_function.toStdString()
				<< std::endl;
		}
		if (!values.at(x).frame_laterality.isEmpty())
		{
			std::cout
				<< "frame_laterality "
				<< values.at(x).frame_laterality.toStdString()
				<< std::endl;
		}
		if (!values.at(x).frame_body_part.isEmpty())
		{
			std::cout
				<< "frame_body_part "
				<< values.at(x).frame_body_part.toStdString()
				<< std::endl;
		}
		if (values.at(x).rescale_ok)
		{
			std::cout
				<< "rescale "
				<< " " << values.at(x).rescale_type.toStdString()
				<< " " << values.at(x).rescale_intercept
				<< " " << values.at(x).rescale_slope
				<< std::endl;
		}
		std::cout << "-----------" << std::endl;
	}
#else
	(void)values;
#endif
}

bool DicomUtils::read_shutter(const mdcm::DataSet & ds, PRDisplayShutter & a)
{
	QString ShutterShape;
	if (get_string_value(ds, mdcm::Tag(0x0018,0x1600), ShutterShape))
	{
		const mdcm::Tag tShutterLeftVerticalEdge(0x0018,0x1602);
		const mdcm::Tag tShutterRightVerticalEdge(0x0018,0x1604);
		const mdcm::Tag tShutterUpperHorizontalEdge(0x0018,0x1606);
		const mdcm::Tag tShutterLowerHorizontalEdge(0x0018,0x1608);
		const mdcm::Tag tCenterofCircularShutter(0x0018,0x1610);
		const mdcm::Tag tRadiusofCircularShutter(0x0018,0x1612);
		const mdcm::Tag tVerticesofthePolygonalShutter(0x0018,0x1620);
		const mdcm::Tag tShutterPresentationValue(0x0018,0x1622);
		const mdcm::Tag tShutterPresentationColorCIELabValue(0x0018,0x1624);
		a.ShutterShape = ShutterShape.trimmed().remove(QChar('\0'));
		int ShutterLeftVerticalEdge;
		int ShutterRightVerticalEdge;
		int ShutterUpperHorizontalEdge;
		int ShutterLowerHorizontalEdge;
		if (get_is_value(ds, tShutterLeftVerticalEdge, &ShutterLeftVerticalEdge) &&
			get_is_value(ds, tShutterRightVerticalEdge, &ShutterRightVerticalEdge) &&
			get_is_value(ds, tShutterUpperHorizontalEdge, &ShutterUpperHorizontalEdge) &&
			get_is_value(ds, tShutterLowerHorizontalEdge, &ShutterLowerHorizontalEdge))
		{
			a.ShutterLeftVerticalEdge    = ShutterLeftVerticalEdge;
			a.ShutterRightVerticalEdge   = ShutterRightVerticalEdge;
			a.ShutterUpperHorizontalEdge = ShutterUpperHorizontalEdge;
			a.ShutterLowerHorizontalEdge = ShutterLowerHorizontalEdge;
		}
		std::vector<int> CenterofCircularShutter;
		int RadiusofCircularShutter;
		if (get_is_values(ds, tCenterofCircularShutter, CenterofCircularShutter) &&
			(CenterofCircularShutter.size() == 2) &&
			DicomUtils::get_is_value(
				ds,
				tRadiusofCircularShutter,
				&RadiusofCircularShutter))
		{
			a.CenterofCircularShutter_x = CenterofCircularShutter.at(1);
			a.CenterofCircularShutter_y = CenterofCircularShutter.at(0);
			a.RadiusofCircularShutter   = RadiusofCircularShutter;
		}
		std::vector<int> VerticesofthePolygonalShutter;
		if (get_is_values(ds, tVerticesofthePolygonalShutter, VerticesofthePolygonalShutter) &&
			(VerticesofthePolygonalShutter.size() > 1) &&
			(VerticesofthePolygonalShutter.size() % 2 == 0))
		{
			for (unsigned int x15 = 0; x15 < VerticesofthePolygonalShutter.size(); ++x15)
			{
				a.VerticesofthePolygonalShutter.push_back(
					VerticesofthePolygonalShutter.at(x15));
			}
		}
		unsigned short ShutterPresentationValue;
		if (get_us_value(ds, tShutterPresentationValue, &ShutterPresentationValue))
		{
			a.ShutterPresentationValue = static_cast<int>(ShutterPresentationValue);
		}
		std::vector<unsigned short> ShutterPresentationColorCIELabValue;
		if (get_us_values(ds, tShutterPresentationColorCIELabValue, ShutterPresentationColorCIELabValue) &&
			(ShutterPresentationColorCIELabValue.size() == 3))
		{
			a.ShutterPresentationColorCIELabValue_L = static_cast<int>(ShutterPresentationColorCIELabValue.at(0));
			a.ShutterPresentationColorCIELabValue_a = static_cast<int>(ShutterPresentationColorCIELabValue.at(1));
			a.ShutterPresentationColorCIELabValue_b = static_cast<int>(ShutterPresentationColorCIELabValue.at(2));
		}
		return true;
	}
	return false;
}

QString DicomUtils::read_enhanced(
	bool * ok,
	const QString & f,
	std::vector<ImageVariant*> & ivariants,
	bool ok3d,
	const bool min_load,
	const short enh_loading_type,
	const QWidget * settings,
	const float tolerance,
	const bool apply_rescale)
{
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_enhanced() begin");
#endif
#ifdef ENHANCED_PRINT_INFO
	std::cout << "EnhancedIODLoadingType = " << enh_loading_type << std::endl;
#endif
	*ok = false;
	QString message_;
	const SettingsWidget * wsettings = static_cast<const SettingsWidget*>(settings);
	std::vector<char*> data;
	DimIndexSq sq;
	DimIndexValues idx_values;
	const EnhancedIODLoadingType loading_type = static_cast<EnhancedIODLoadingType>(enh_loading_type);
	FrameGroupValues values;
	FrameGroupValues shared_values;
	ImageOverlays image_overlays;
	mdcm::PhotometricInterpretation pi;
	mdcm::PixelFormat pixelformat;
	unsigned short rows_{};
	unsigned short columns_{};
	bool rows_ok{};
	bool cols_ok{};
	bool icc_ok{};
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	const bool pred6_bug = wsettings->get_predictor_workaround();
	const bool cornell_bug = wsettings->get_cornell_workaround();
	const bool fix_jpeg_prec = wsettings->get_try_fix_jpeg_prec();
	const bool use_icc = wsettings->get_apply_icc();
	const bool skip_too_large = wsettings->get_skip_too_large();
	QString sop;
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
		if (!reader.Read())
		{
			return QString("Can not read file");
		}
		const mdcm::File & file = reader.GetFile();
		const mdcm::DataSet & ds = file.GetDataSet();
		if (ds.IsEmpty())
		{
			return QString("ds.IsEmpty()");
		}
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		const mdcm::Tag tRows(0x0028,0x0010);
		const mdcm::Tag tColumns(0x0028,0x0011);
		rows_ok = get_us_value(ds, tRows, &rows_);
		cols_ok = get_us_value(ds, tColumns, &columns_);
		QString iod;
		const mdcm::Tag tSOPClassUID(0x0008,0x0016);
		const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
		const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
		QString sop_tmp;
		if (get_string_value(ds, tSOPClassUID, sop_tmp))
		{
			sop = sop_tmp.remove(QChar('\0'));
			mdcm::UIDs uid;
			uid.SetFromUID(sop.toLatin1().constData());
			iod = QString::fromLatin1(uid.GetName());
		}
		read_dimension_index_sq(ds, sq);
		//
		const bool ok_f =
			read_group_sq(
				ds,
				tPerFrameFunctionalGroupsSequence,
				sq, idx_values, values);
		const bool ok_g =
			read_group_sq(
				ds,
				tSharedFunctionalGroupsSequence,
				sq, idx_values, shared_values);
		if (!ok_f && !ok_g)
		{
			return QString(
				"Functional groups not found, "
				"probably broken DICOM file.");
		}
#ifdef ENHANCED_PRINT_INFO
		if (!min_load)
		{
			print_sq(sq);
#ifdef ENHANCED_PRINT_GROUPS
			std::cout << "\nGroup values:" << std::endl;
			print_func_group(values);
			std::cout << "Shared group values:" << std::endl;
			print_func_group(shared_values);
#endif
		}
#endif
		enhanced_process_values(values, shared_values);
		enhanced_check_rescale(ds, values);
	}
	//
	double dircos_read[6]{};
	unsigned int dimx_read, dimy_read, dimz_read;
	double origin_x_read,  origin_y_read,  origin_z_read;
	double spacing_x_read, spacing_y_read, spacing_z_read;
	double unsused0{};
	double unsused1{1.0};
	AnatomyMap empty_;
	message_ =
		read_buffer(
			ok, data,
			image_overlays, -1,
			empty_, -1, // unused in enhanced
			f,
			false, // do not use MDCM's rescale for enhanced
			pixelformat, false,
			pi,
			&dimx_read, &dimy_read, &dimz_read,
			&origin_x_read,  &origin_y_read,  &origin_z_read,
			&spacing_x_read, &spacing_y_read, &spacing_z_read,
			dircos_read,
			&unsused0,
			&unsused1, // MDCM's rescale, unused
			clean_unused_bits,
			false, false, false,
			false,
			pred6_bug,
			cornell_bug,
			fix_jpeg_prec,
			skip_too_large,
			nullptr,
			nullptr,
			use_icc, &icc_ok);
	if (*ok == false) return message_;
	if (rows_ok && cols_ok &&
		(dimx_read != columns_ || dimy_read != rows_))
	{
		*ok = false;
		for (unsigned int x = 0; x < data.size(); ++x)
		{
			delete [] data[x];
		}
		data.clear();
		return QString("Dimensions mismatch");
	}
	if (dimz_read != data.size())
	{
		*ok = false;
		for (unsigned int x = 0; x < data.size(); ++x)
		{
			delete [] data[x];
		}
		data.clear();
		return QString("Mismatch in data size and number of frames");
	}
	//
	//
	//
	int dim8th{-1};
	int dim7th{-1};
	int dim6th{-1};
	int dim5th{-1};
	int dim4th{-1};
	int dim3rd{-1};
	int enh_id{-1};
	if (loading_type == EnhancedIODLoadingType::PreferUniformVolumes ||
		loading_type == EnhancedIODLoadingType::StrictMultipleImages ||
		loading_type == EnhancedIODLoadingType::StrictSingleImage)
	{
		if (!idx_values.empty())
		{
			enhanced_get_indices(
				sq,
				&dim8th, &dim7th, &dim6th, &dim5th, &dim4th, &dim3rd,
				&enh_id,
				enh_loading_type);
		}
		else
		{
			// stack id/position number without dimension organisation?
			// try to re-build
			bool idx_values_rebuild{};
			bool tmp12{};
			DimIndexValues idx_values_tmp;
			const size_t values_size = values.size();
			if (values_size == 2)
			{
				for (unsigned int x = 0; x < values_size; ++x)
				{
					if (!(values.at(x).stack_id_ok && values.at(x).in_stack_pos_num_ok))
					{
						tmp12 = true;
						break;
					}
				}
			}
			if (!tmp12)
			{
				for (unsigned int x = 0; x < values_size; ++x)
				{
					DimIndexValue tmp13;
					tmp13.id = values.at(x).id;
					tmp13.idx.push_back(values.at(x).stack_id);
					tmp13.idx.push_back(values.at(x).in_stack_pos_num);
					idx_values_tmp.push_back(std::move(tmp13));
				}
				for (unsigned int x = 0; x < idx_values_tmp.size(); ++x)
				{
					idx_values.push_back(idx_values_tmp[x]);
				}
				idx_values_tmp.clear();
				idx_values_rebuild = true;
#ifdef ENHANCED_PRINT_INFO
				if (!min_load)
				{
					std::cout
						<< "Using Stack ID and In-stack Position without Dimension Organization SQ"
						<< std::endl;
				}
#endif
			}
			if (idx_values_rebuild)
			{
				enh_id = 0;
				dim4th = 0;
				dim3rd = 1;
			}
		}
	}
#ifdef ENHANCED_PRINT_INFO
	if (!min_load)
	{
		std::cout << "ID " << enh_id;
		std::cout
			<< " dim8th=" << dim8th
			<< " dim7th=" << dim7th
			<< " dim6th=" << dim6th
			<< " dim5th=" << dim5th
			<< " dim4th=" << dim4th
			<< " dim3rd=" << dim3rd
			<< std::endl;
	}
#endif
	message_ = read_enhanced_3d_8d(
		ok, ivariants, sop, f,
		data,
		image_overlays,
		rows_, columns_, pixelformat, pi,
		dim8th, dim7th, dim6th, dim5th, dim4th, dim3rd,
		idx_values, values,
		ok3d,
		min_load,
		settings,
		dircos_read,
		INT_MIN,
		spacing_x_read, spacing_y_read, spacing_z_read,
		origin_x_read, origin_y_read, origin_z_read,
		unsused0, unsused1,
		apply_rescale,
		(use_icc && icc_ok),
		tolerance,
		enh_loading_type);
#ifdef ENHANCED_PRINT_INFO
	if (!min_load && !message_.isEmpty())
		std::cout << message_.toStdString() << std::endl;
#endif
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("before delete[]");
#endif
	for (unsigned int x = 0; x < data.size(); ++x)
	{
		delete [] data[x];
	}
	data.clear();
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("after delete[] / read_enhanced() end");
#endif
	return message_;
}

QString DicomUtils::read_enhanced_supp_palette(
	bool * ok,
	const QString & f,
	std::vector<ImageVariant*> & ivariants,
	bool ok3d,
	const bool min_load,
	const short enh_loading_type,
	const QWidget * settings,
	const float tolerance)
{
#ifdef ENHANCED_PRINT_INFO
	std::cout << "EnhancedIODLoadingType = " << enh_loading_type << std::endl;
#endif
	*ok = false;
	QString message_;
	const SettingsWidget * wsettings = static_cast<const SettingsWidget*>(settings);
	std::vector<char*> data;
	DimIndexSq sq;
	DimIndexValues idx_values;
	const EnhancedIODLoadingType loading_type = static_cast<EnhancedIODLoadingType>(enh_loading_type);
	FrameGroupValues values;
	FrameGroupValues shared_values;
	ImageOverlays image_overlays; // unused
	mdcm::PhotometricInterpretation pi;
	mdcm::PixelFormat pixelformat;
	QString sop;
	bool ok_f{};
	bool ok_g{};
	unsigned short rows_{};
	unsigned short columns_{};
	bool rows_ok{};
	bool cols_ok{};
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	const bool pred6_bug = wsettings->get_predictor_workaround();
	const bool cornell_bug = wsettings->get_cornell_workaround();
	const bool fix_jpeg_prec = wsettings->get_try_fix_jpeg_prec();
	const bool skip_too_large = wsettings->get_skip_too_large();
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
		if (!reader.Read())
		{
			return QString("Can not read file");
		}
		const mdcm::File & file = reader.GetFile();
		const mdcm::DataSet & ds = file.GetDataSet();
		if (ds.IsEmpty())
		{
			return QString("ds.IsEmpty()");
		}
		if (!has_supp_palette(ds))
		{
			return QString("No Supplemental LUT");
		}
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		const mdcm::Tag tRows(0x0028,0x0010);
		const mdcm::Tag tColumns(0x0028,0x0011);
		rows_ok = get_us_value(ds, tRows, &rows_);
		cols_ok = get_us_value(ds, tColumns, &columns_);
		const mdcm::Tag tSOPClassUID(0x0008,0x0016);
		const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
		const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
		QString iod;
		QString sop_tmp;
		if (get_string_value(ds, tSOPClassUID, sop_tmp))
		{
			sop = sop_tmp.remove(QChar('\0'));
			mdcm::UIDs uid;
			uid.SetFromUID(sop.toLatin1().constData());
			iod = QString::fromLatin1(uid.GetName());
		}
		read_dimension_index_sq(ds, sq);
		//
		ok_f = read_group_sq(
			ds,
			tPerFrameFunctionalGroupsSequence,
			sq, idx_values, values);
		ok_g = read_group_sq(
			ds,
			tSharedFunctionalGroupsSequence,
			sq, idx_values, shared_values);
		if (!ok_f && !ok_g)
		{
			return QString(
				"Functional groups not found,\n"
				"probably broken DICOM file.");
		}
	}
	//
#ifdef ENHANCED_PRINT_INFO
	if (!min_load)
	{
		print_sq(sq);
#ifdef ENHANCED_PRINT_GROUPS
		std::cout << "\nGroup values:" << std::endl;
		print_func_group(values);
		std::cout << "Shared group values:" << std::endl;
		print_func_group(shared_values);
#endif
	}
#endif
	enhanced_process_values(values, shared_values);
	//
	double dircos_read[6]{};
	unsigned int dimx_read{};
	unsigned int dimy_read{};
	unsigned int dimz_read{};
	double origin_x_read{};
	double origin_y_read{};
	double origin_z_read{};
	double spacing_x_read{};
	double spacing_y_read{};
	double spacing_z_read{};
	double unsused0{};
	double unsused1{1.0};
	int red_subscript{INT_MIN};
	bool icc_ok_dummy{};
	AnatomyMap empty_;
	// do not use MDCM's rescale for enhanced
	message_ =
		read_buffer(
			ok, data,
			image_overlays, -2, // unused
			empty_, -1, // unused in enhanced
			f,
			false,
			pixelformat, false,
			pi,
			&dimx_read, &dimy_read, &dimz_read,
			&origin_x_read,  &origin_y_read,  &origin_z_read,
			&spacing_x_read, &spacing_y_read, &spacing_z_read,
			dircos_read,
			&unsused0,
			&unsused1, // MDCM's rescale, unused
			clean_unused_bits,
			false, false, false,
			true,
			pred6_bug,
			cornell_bug,
			fix_jpeg_prec,
			skip_too_large,
			&red_subscript,
			nullptr,
			false, &icc_ok_dummy);
#if 0
	std::cout << "subscript = " << red_subscript << std::endl;
#endif
	if (*ok == false) return message_;
	if (rows_ok && cols_ok &&
		(dimx_read != columns_ || dimy_read != rows_))
	{
		*ok = false;
		for (unsigned int x = 0; x < data.size(); ++x)
		{
			delete [] data[x];
		}
		data.clear();
		return QString("Dimensions mismatch");
	}
	if (dimz_read != data.size())
	{
		*ok = false;
		for (unsigned int x = 0; x < data.size(); ++x)
		{
			delete [] data[x];
		}
		data.clear();
		return QString("Mismatch in data size and number of frames");
	}
	//
	//
	//
	int dim8th{-1};
	int dim7th{-1};
	int dim6th{-1};
	int dim5th{-1};
	int dim4th{-1};
	int dim3rd{-1};
	int enh_id{-1};
	if (loading_type == EnhancedIODLoadingType::PreferUniformVolumes ||
		loading_type == EnhancedIODLoadingType::StrictMultipleImages ||
		loading_type == EnhancedIODLoadingType::StrictSingleImage)
	{
		if (!idx_values.empty())
		{
			enhanced_get_indices(
				sq,
				&dim8th, &dim7th, &dim6th, &dim5th, &dim4th, &dim3rd,
				&enh_id,
				enh_loading_type);
		}
		else
		{
			// stack id/position number without dimension organisation?
			// try to re-build
			bool idx_values_rebuild{};
			bool tmp12{};
			DimIndexValues idx_values_tmp;
			const size_t values_size = values.size();
			if (values_size == 2)
			{
				for (unsigned int x = 0; x < values_size; ++x)
				{
					if (!(values.at(x).stack_id_ok && values.at(x).in_stack_pos_num_ok))
					{
						tmp12 = true;
						break;
					}
				}
			}
			if (!tmp12)
			{
				for (unsigned int x = 0; x < values_size; ++x)
				{
					DimIndexValue tmp13;
					tmp13.id = values.at(x).id;
					tmp13.idx.push_back(values.at(x).stack_id);
					tmp13.idx.push_back(values.at(x).in_stack_pos_num);
					idx_values_tmp.push_back(std::move(tmp13));
				}
				for (unsigned int x = 0; x < idx_values_tmp.size(); ++x)
				{
					idx_values.push_back(idx_values_tmp[x]);
				}
				idx_values_tmp.clear();
				idx_values_rebuild = true;
#ifdef ENHANCED_PRINT_INFO
				if (!min_load)
				{
					std::cout
						<< "Using Stack ID and In-stack Position without Dimension Organization SQ"
						<< std::endl;
				}
#endif
			}
			if (idx_values_rebuild)
			{
				enh_id = 0;
				dim4th = 0;
				dim3rd = 1;
			}
		}
	}
#ifdef ENHANCED_PRINT_INFO
	if (!min_load)
	{
		std::cout << "ID " << enh_id;
		std::cout
			<< " dim8th=" << dim8th
			<< " dim7th=" << dim7th
			<< " dim6th=" << dim6th
			<< " dim5th=" << dim5th
			<< " dim4th=" << dim4th
			<< " dim3rd=" << dim3rd
			<< std::endl;
	}
#endif
	message_ = read_enhanced_3d_8d(
		ok, ivariants, sop, f,
		data,
		image_overlays,
		rows_, columns_, pixelformat, pi,
		dim8th, dim7th, dim6th, dim5th, dim4th, dim3rd,
		idx_values, values,
		ok3d,
		min_load,
		settings,
		dircos_read,
		red_subscript,
		spacing_x_read, spacing_y_read, spacing_z_read,
		origin_x_read, origin_y_read, origin_z_read,
		unsused0, unsused1,
		false,
		false,
		tolerance,
		enh_loading_type);
#ifdef ENHANCED_PRINT_INFO
	if (!min_load && !message_.isEmpty())
	{
		std::cout << message_.toStdString() << std::endl;
	}
#endif
	for (unsigned int x = 0; x < data.size(); ++x)
	{
		delete [] data[x];
	}
	data.clear();
	return message_;
}

QString DicomUtils::read_ultrasound(
	bool * ok, const short load_type, ImageVariant * ivariant,
	const QStringList & images_ipp,
	const QWidget * settings)
{
	if (!ok) return QString("read_ultrasound : error (1)");
	*ok = false;
	const bool overwrite_mdcm_spacing = true;
	if (!ivariant)
	{
		return QString("Image is null");
	}
	if (images_ipp.size() != 1)
	{
		return QString("read_ultrasound reads 1 image");
	}
	const SettingsWidget * wsettings = static_cast<const SettingsWidget *>(settings);
	unsigned int dimx{};
	unsigned int dimy{};
	unsigned int dimz{};
	double origin_x{};
	double origin_y{};
	double origin_z{};
	double spacing_x{};
	double spacing_y{};
	double spacing_z{};
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	const bool pred6_bug = wsettings->get_predictor_workaround();
	const bool cornell_bug = wsettings->get_cornell_workaround();
	const bool fix_jpeg_prec = wsettings->get_try_fix_jpeg_prec();
	const bool use_icc = wsettings->get_apply_icc();
	const bool skip_too_large = wsettings->get_skip_too_large();
	bool icc_ok{};
	std::vector<char*> data;
	itk::Matrix<itk::SpacePrecisionType, 3, 3> direction;
	mdcm::PixelFormat pixelformat;
	mdcm::PhotometricInterpretation pi;
	const mdcm::Tag tnumframes(0x0028,0x0008);
	const mdcm::Tag tPixelAspectRatio(0x0028,0x0034);
	const mdcm::PrivateTag tPhilipsVoxelSpacing(0x200d,0x03,"Philips US Imaging DD 036");
	const bool overlays_enabled = wsettings->get_overlays();
	const int overlays_idx = overlays_enabled ? 0 : -2;
	int number_of_frames{};
	double tmp_c{-999999.0};
	double tmp_w{-999999.0};
	short tmp_lut_function{1};
	//
	{
		mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		reader.SetFileName(QDir::toNativeSeparators(images_ipp.at(0)).toUtf8().constData());
#else
		reader.SetFileName(QDir::toNativeSeparators(images_ipp.at(0)).toLocal8Bit().constData());
#endif
#else
		reader.SetFileName(images_ipp.at(0).toLocal8Bit().constData());
#endif
		*ok = reader.Read();
		if (*ok == false)
		{
			return (QString("can not read file ") + images_ipp.at(0));
		}
		const mdcm::File    & file = reader.GetFile();
		const mdcm::DataSet & ds   = file.GetDataSet();
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		{
			const mdcm::DataElement & e = ds.GetDataElement(tnumframes);
			if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
			{
				QString numframes;
				numframes = QString::fromLatin1(
					e.GetByteValue()->GetPointer(),
					e.GetByteValue()->GetLength());
				const QVariant v(numframes.trimmed().remove(QChar('\0')));
				bool c_ok{};
				const int k = v.toInt(&c_ok);
				if (c_ok) number_of_frames = k;
			}
		}
		//
		read_ivariant_info_tags(ds, ivariant);
		read_acquisition_time(ds, ivariant->acquisition_date, ivariant->acquisition_time);
		//
		if (number_of_frames > 1)
			read_frame_times(ds, ivariant, number_of_frames);
		//
		read_us_regions(ds, ivariant);
		//
		read_window(ds, &tmp_c, &tmp_w, &tmp_lut_function);
		//
		if (overwrite_mdcm_spacing)
		{
			bool bPhilipsVoxelSpacing{};
			bool bPixelAspectRatio{};
			{
				// Partial support Philips private 3D storage, only spacing,
				// TODO complete
				const mdcm::DataElement & e = ds.GetDataElement(tPhilipsVoxelSpacing);
				if (!e.IsEmpty())
				{
					const mdcm::ByteValue * bv = e.GetByteValue();
					if (bv)
					{
						QString tmp0 = QString::fromLatin1(bv->GetPointer(), bv->GetLength());
						if (tmp0.contains(QString(",")))
						{
							// Workaround invalid VR
							tmp0.replace(QString(","), QString("."));
						}
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
						const QStringList tmp1 = tmp0.split(QString("\\"), Qt::SkipEmptyParts);
#else
						const QStringList tmp1 = tmp0.split(QString("\\"), QString::SkipEmptyParts);
#endif
						if (tmp1.size() == 3)
						{
							bool okx{};
							bool oky{};
							bool okz{};
							const double tmp_spacing_x = QVariant(tmp1.at(0).trimmed()).toDouble(&okx);
							const double tmp_spacing_y = QVariant(tmp1.at(1).trimmed()).toDouble(&oky);
							const double tmp_spacing_z = QVariant(tmp1.at(2).trimmed()).toDouble(&okz);
							if (okx && oky && okz)
							{
								bPhilipsVoxelSpacing = true;
								spacing_x = tmp_spacing_x;
								spacing_y = tmp_spacing_y;
								spacing_z = tmp_spacing_z;
							}
						}
					}
				}
			}
			{
				std::vector<int> tmp0;
				if (!bPhilipsVoxelSpacing && get_is_values(ds, tPixelAspectRatio, tmp0))
				{
					if (tmp0.size() == 2)
					{
						bPixelAspectRatio = true;
						spacing_x = tmp0[1];
						spacing_y = tmp0[0];
						spacing_z = 1.0;
					}
				}
			}
			if (!(bPhilipsVoxelSpacing || bPixelAspectRatio))
			{
				spacing_x = 1.0;
				spacing_y = 1.0;
				spacing_z = 1.0;
			}
		}
	}
	ivariant->di->default_us_window_center = ivariant->di->us_window_center = tmp_c;
	ivariant->di->default_us_window_width = ivariant->di->us_window_width = tmp_w;
	ivariant->di->default_lut_function = ivariant->di->lut_function = tmp_lut_function;
	//
	double dircos_[6]{};
	unsigned int dimx_, dimy_, dimz_;
	double origin_x_, origin_y_, origin_z_;
	double spacing_x_, spacing_y_, spacing_z_;
	double shift_tmp{};
	double scale_tmp{1.0};
	QString buff_error;
	buff_error = read_buffer(
		ok,
		data,
		ivariant->image_overlays, overlays_idx,
		ivariant->anatomy, 0, // TODO check
		images_ipp.at(0),
		(load_type == 0 || load_type == 2 || load_type == 3) ? wsettings->get_rescale() : false,
		pixelformat, false,
		pi,
		&dimx_, &dimy_, &dimz_,
		&origin_x_, &origin_y_, &origin_z_,
		&spacing_x_, &spacing_y_, &spacing_z_,
		dircos_,
		&shift_tmp, &scale_tmp,
		clean_unused_bits,
		false, false, false,
		false,
		pred6_bug,
		cornell_bug,
		fix_jpeg_prec,
		skip_too_large,
		nullptr,
		nullptr,
		use_icc, &icc_ok);
	if (*ok == false) return buff_error;
	//
	if (!overwrite_mdcm_spacing)
	{
		spacing_x = spacing_x_ > 0 ? spacing_x_ : 1.0;
		spacing_y = spacing_y_ > 0 ? spacing_y_ : 1.0;
		spacing_z = spacing_z_ > 0 ? spacing_z_ : 1.0;
	}
	//
	dimx = dimx_;
	dimy = dimy_;
	dimz = dimz_;
	//
	// not required, origin always 0 0 0 and RAI cosines?
	origin_x = origin_x_;
	origin_y = origin_y_;
	origin_z = origin_z_;
	const float row_dircos_x = dircos_[0];
	const float row_dircos_y = dircos_[1];
	const float row_dircos_z = dircos_[2];
	const float col_dircos_x = dircos_[3];
	const float col_dircos_y = dircos_[4];
	const float col_dircos_z = dircos_[5];
	const float nrm_dircos_x =
		row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y;
	const float nrm_dircos_y =
		row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const float nrm_dircos_z =
		row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	direction[0][0] = row_dircos_x;
	direction[1][0] = row_dircos_y;
	direction[2][0] = row_dircos_z;
	direction[0][1] = col_dircos_x;
	direction[1][1] = col_dircos_y;
	direction[2][1] = col_dircos_z;
	direction[0][2] = nrm_dircos_x;
	direction[1][2] = nrm_dircos_y;
	direction[2][2] = nrm_dircos_z;
	//
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("before gen_itk_image()");
#endif
	QString error = CommonUtils::gen_itk_image(ok,
		data,
		pixelformat, pi,
		ivariant,
		direction,
		dimx, dimy, dimz,
		origin_x, origin_y, origin_z,
		spacing_x, spacing_y, spacing_z,
#ifdef TMP_ALWAYS_GEOM_FROM_IMAGE
		true, false,
#else
		false, false,
#endif
		wsettings->get_resize(),
		wsettings->get_size_x(), wsettings->get_size_y(),
		wsettings->get_rescale(),
		(use_icc && icc_ok),
		false);
	for (unsigned int x = 0; x < data.size(); ++x)
	{
		delete [] data[x];
	}
	data.clear();
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("after gen_itk_image()");
#endif
	if (*ok == false)
	{
		return error;
	}
	if (!ivariant->frame_times.empty() &&
		(static_cast<int>(ivariant->frame_times.size()) != ivariant->di->idimz))
	{
		ivariant->frame_times.clear();
	}
	//
	CommonUtils::reset_bb(ivariant);
	ivariant->di->selected_z_slice = 0;
	return QString("");
}

QString DicomUtils::read_nuclear(
	bool * ok, const short load_type, ImageVariant * ivariant,
	const QStringList & images_ipp,
	bool /* ok3d */,
	const QWidget * settings)
{
// TODO for image type RECON TOMO volume might be possible
	if (!ok) return QString("read_nuclear : error (1)");
	*ok = false;
	if (!ivariant) return QString("Image is null");
	if (images_ipp.size() != 1) return QString("read_nuclear reads 1 image");
	const SettingsWidget * wsettings = static_cast<const SettingsWidget *>(settings);
	unsigned int dimx{};
	unsigned int dimy{};
	unsigned int dimz{};
	double origin_x{};
	double origin_y{};
	double origin_z{};
	double spacing_x{};
	double spacing_y{};
	double spacing_z{};
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	const bool pred6_bug = wsettings->get_predictor_workaround();
	const bool cornell_bug = wsettings->get_cornell_workaround();
	const bool fix_jpeg_prec = wsettings->get_try_fix_jpeg_prec();
	const bool use_icc = wsettings->get_apply_icc();
	const bool skip_too_large = wsettings->get_skip_too_large();
	bool icc_ok{};
	std::vector<char*> data;
	itk::Matrix<itk::SpacePrecisionType, 3, 3> direction;
	mdcm::PixelFormat pixelformat;
	mdcm::PhotometricInterpretation pi;
	const mdcm::Tag tnumframes(0x0028,0x0008);
	const bool overlays_enabled = wsettings->get_overlays();
	const int overlays_idx = overlays_enabled ? 0 : -2;
#if 0
	int number_of_frames = 0;
#endif
	double tmp_c{-999999.0};
	double tmp_w{-999999.0};
	short tmp_lut_function{1};
	//
	{
		mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		reader.SetFileName(QDir::toNativeSeparators(images_ipp.at(0)).toUtf8().constData());
#else
		reader.SetFileName(QDir::toNativeSeparators(images_ipp.at(0)).toLocal8Bit().constData());
#endif
#else
		reader.SetFileName(images_ipp.at(0).toLocal8Bit().constData());
#endif
		*ok = reader.Read();
		if (*ok == false)
		{
			return (QString("can not read file ") + images_ipp.at(0));
		}
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		const mdcm::File    & file = reader.GetFile();
		const mdcm::DataSet & ds   = file.GetDataSet();
#if 0
		{
			const mdcm::DataElement & e = ds.GetDataElement(tnumframes);
			if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
			{
				QString numframes;
				numframes = QString::fromLatin1(
					e.GetByteValue()->GetPointer(),
					e.GetByteValue()->GetLength());
				const QVariant v(numframes.trimmed().remove(QChar('\0')));
				bool c_ok{};
				const int k = v.toInt(&c_ok);
				if (c_ok) number_of_frames = k;
			}
		}
#endif
		//
		read_ivariant_info_tags(ds, ivariant);
		read_acquisition_time(ds, ivariant->acquisition_date, ivariant->acquisition_time);
		read_window(ds, &tmp_c, &tmp_w, &tmp_lut_function);
	}
	ivariant->di->default_us_window_center = ivariant->di->us_window_center = tmp_c;
	ivariant->di->default_us_window_width = ivariant->di->us_window_width = tmp_w;
	ivariant->di->default_lut_function = ivariant->di->lut_function = tmp_lut_function;
	//
	double dircos_[6]{};
	unsigned int dimx_, dimy_, dimz_;
	double origin_x_, origin_y_, origin_z_, spacing_x_, spacing_y_, spacing_z_;
	double shift_tmp{};
	double scale_tmp{1.0};
	QString buff_error;
	buff_error = read_buffer(
		ok,
		data,
		ivariant->image_overlays, overlays_idx,
		ivariant->anatomy, 0, // TODO check
		images_ipp.at(0),
		(load_type == 0 || load_type == 2 || load_type == 3) ? wsettings->get_rescale() : false,
		pixelformat, false,
		pi,
		&dimx_, &dimy_, &dimz_,
		&origin_x_, &origin_y_, &origin_z_,
		&spacing_x_, &spacing_y_, &spacing_z_,
		dircos_,
		&shift_tmp, &scale_tmp,
		clean_unused_bits,
		false, false, false,
		false,
		pred6_bug,
		cornell_bug,
		fix_jpeg_prec,
		skip_too_large,
		nullptr,
		nullptr,
		use_icc, &icc_ok);
	if (*ok == false) return buff_error;
#if 0
	std::cout << "Origin: "  << origin_x_  << " " << origin_y_  << " " << origin_z_ << std::endl;
	std::cout << "Spacing: " << spacing_x_ << " " << spacing_y_ << " " << spacing_z_ << std::endl;
	std::cout << "Cosines: " << dircos_[0] << " " << dircos_[1] << " " << dircos_[2] << std::endl;
	std::cout << "         " << dircos_[3] << " " << dircos_[4] << " " << dircos_[5] << std::endl;
#endif
	//
	dimx = dimx_;
	dimy = dimy_;
	dimz = dimz_;
	origin_x = origin_x_;
	origin_y = origin_y_;
	origin_z = origin_z_;
	spacing_x = spacing_x_;
	spacing_y = spacing_y_;
/*
Spacing between slices, in mm, measured from center-to-center of
each slice along the normal to the first image. The sign of the
Spacing Between Slices (0018,0088) determines the direction of
stacking. The normal is determined by the cross product of the
direction cosines of the first row and first column of the first
frame, such that a positive spacing indicates slices are stacked
behind the first slice and a negative spacing indicates slices
are stacked in front of the first slice. See Image Orientation
(0020,0037) in the NM Detector Module.
*/
	spacing_z = spacing_z_ > 0.0 ? spacing_z_ : -spacing_z_;
	//
	const float row_dircos_x = dircos_[0];
	const float row_dircos_y = dircos_[1];
	const float row_dircos_z = dircos_[2];
	const float col_dircos_x = dircos_[3];
	const float col_dircos_y = dircos_[4];
	const float col_dircos_z = dircos_[5];
	const float nrm_dircos_x =
		row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y;
	const float nrm_dircos_y =
		row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const float nrm_dircos_z =
		row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	direction[0][0] = row_dircos_x;
	direction[1][0] = row_dircos_y;
	direction[2][0] = row_dircos_z;
	direction[0][1] = col_dircos_x;
	direction[1][1] = col_dircos_y;
	direction[2][1] = col_dircos_z;
	direction[0][2] = (spacing_z_ > 0) ? nrm_dircos_x : -nrm_dircos_x;
	direction[1][2] = (spacing_z_ > 0) ? nrm_dircos_y : -nrm_dircos_y;
	direction[2][2] = (spacing_z_ > 0) ? nrm_dircos_z : -nrm_dircos_z;
#if 0
	std::cout << "Direction: " << direction[0][0] << " " << direction[1][0] << " " << direction[2][0] << std::endl;
	std::cout << "           " << direction[0][1] << " " << direction[1][1] << " " << direction[2][1] << std::endl;
	std::cout << "           " << direction[0][2] << " " << direction[1][2] << " " << direction[2][2] << std::endl;
#endif
	//
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("before gen_itk_image()");
#endif
	QString error = CommonUtils::gen_itk_image(ok,
		data,
		pixelformat, pi,
		ivariant,
		direction,
		dimx, dimy, dimz,
		origin_x, origin_y, origin_z,
		spacing_x, spacing_y, spacing_z,
#ifdef TMP_ALWAYS_GEOM_FROM_IMAGE
		true, false,
#else
		false, false,
#endif
		wsettings->get_resize(),
		wsettings->get_size_x(), wsettings->get_size_y(),
		wsettings->get_rescale(),
		(use_icc && icc_ok),
		false);
	for (unsigned int x = 0; x < data.size(); ++x)
	{
		delete [] data[x];
	}
	data.clear();
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("after gen_itk_image()");
#endif
	if (*ok == false)
	{
		return error;
	}
	//
	CommonUtils::reset_bb(ivariant);
	//
	return QString("");
}

QString DicomUtils::read_series(
	bool * ok,
	const bool min_load,
	const bool mosaic,
	const bool uihgrid,
	const bool elscint,
	ImageVariant * ivariant,
	const QStringList & images_ipp,
	bool ok3d,
	const QWidget * settings,
	float tolerance,
	bool apply_rescale)
{
	*ok = false;
	if (!ivariant) return QString("Image is null");
	const SettingsWidget * wsettings = static_cast<const SettingsWidget *>(settings);
	unsigned int dimx{};
	unsigned int dimy{};
	unsigned int dimz{};
	double origin_x{};
	double origin_y{};
	double origin_z{};
	double spacing_x{};
	double spacing_y{};
	double spacing_z{};
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	const bool pred6_bug = wsettings->get_predictor_workaround();
	const bool cornell_bug = wsettings->get_cornell_workaround();
	const bool fix_jpeg_prec = wsettings->get_try_fix_jpeg_prec();
	const bool use_icc = wsettings->get_apply_icc();
	const bool skip_too_large = wsettings->get_skip_too_large();
	bool icc_ok{};
	std::vector<char*> data;
	itk::Matrix<itk::SpacePrecisionType, 3, 3> direction;
	mdcm::PixelFormat pixelformat;
	mdcm::PixelFormat previous_pixelformat;
	mdcm::PhotometricInterpretation pi;
	bool geometry_from_image = min_load;
	bool slices_ok{};
	const mdcm::Tag tnumframes(0x0028,0x0008);
	const bool overlays_enabled = wsettings->get_overlays();
	std::vector<double> levels_;
	std::vector<double> windows_;
	std::vector<short>  luts_;
	QList<QString>      acqtimes;
	//
#ifdef WARN_RAM_SIZE
	const double total_ram = CommonUtils::get_total_memory_saved();
#endif
	unsigned long long count_buffers_size{};
	for (int j = 0; j < images_ipp.size(); ++j)
	{
		{
			int number_of_frames{};
			mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
			reader.SetFileName(QDir::toNativeSeparators(images_ipp.at(j)).toUtf8().constData());
#else
			reader.SetFileName(QDir::toNativeSeparators(images_ipp.at(j)).toLocal8Bit().constData());
#endif
#else
			reader.SetFileName(images_ipp.at(j).toLocal8Bit().constData());
#endif
			*ok = reader.Read();
			if (*ok == false)
			{
				for (unsigned int x = 0; x < data.size(); ++x)
				{
					delete [] data[x];
				}
				data.clear();
				return (QString("can not read file ") + images_ipp.at(j));
			}
#ifndef ALIZA_LOAD_DCM_THREAD
			QApplication::processEvents();
#endif
			const mdcm::File & file = reader.GetFile();
			const mdcm::DataSet & ds = file.GetDataSet();
			if (j == 0)
			{
				{
					const mdcm::DataElement & e = ds.GetDataElement(tnumframes);
					if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
					{
						QString numframes = QString::fromLatin1(
							e.GetByteValue()->GetPointer(),
							e.GetByteValue()->GetLength());
						const QVariant v(numframes.trimmed().remove(QChar('\0')));
						bool c_ok{};
						const int k = v.toInt(&c_ok);
						if (c_ok) number_of_frames = k;
					}
				}
				if (!min_load)
				{
					read_ivariant_info_tags(ds, ivariant);
					//
					if (ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.4"))
					{
						ivariant->iinfo = read_MRImageModule(ds);
					}
					else if (ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.2"))
					{
						ivariant->iinfo = read_CTImageModule(ds);
					}
				}
				if (mosaic)
				{
					geometry_from_image = true;
				}
				else if (uihgrid)
				{
					if (!min_load)
					{
						slices_ok =	read_slices_uihgrid(
							ds,
							ivariant,
							tolerance);
						if (slices_ok)
						{
							ivariant->iod_supported = true;
						}
						else
						{
							ivariant->di->skip_texture = true;
#ifdef TMP_ALWAYS_GEOM_FROM_IMAGE
							geometry_from_image = true;
#endif
						}
					}
				}
				else
				{
					if (ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.128") || // PET
						ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.2")   || // CT
						ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.4"))     // MR
					{
						if (!min_load)
						{
							slices_ok =	read_slices(
								images_ipp,
								ivariant,
								tolerance);
							if (slices_ok)
							{
								ivariant->iod_supported = true;
								ivariant->unit_str = QString(" mm");
							}
							else
							{
								ivariant->di->skip_texture = true;
#ifdef TMP_ALWAYS_GEOM_FROM_IMAGE
								geometry_from_image = true;
#endif
							}
						}
					}
					else if (ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.481.2")) // RTDOSE
					{
						if (!min_load)
						{
							slices_ok =	read_slices_rtdose(
								images_ipp.at(j),
								ivariant,
								0.01f);
							if (slices_ok)
							{
								ivariant->iod_supported = true;
								load_contour(ds, ivariant);
							}
							else
							{
								ivariant->di->skip_texture = true;
#ifdef TMP_ALWAYS_GEOM_FROM_IMAGE
								geometry_from_image = true;
#endif
							}
						}
					}
					else
					{
						if (!min_load)
						{
							slices_ok =	read_slices(
								images_ipp,
								ivariant,
								tolerance);
							if (!slices_ok)
							{
								ivariant->di->skip_texture = true;
#ifdef TMP_ALWAYS_GEOM_FROM_IMAGE
								geometry_from_image = true;
#endif
							}
						}
					}
				}
				//
				if (!min_load) read_frame_times(ds, ivariant, number_of_frames); //////////
				//
				if (!min_load && images_ipp.size() == 1)
				{
					const int inst_num = read_instance_number(ds);
					if (inst_num >= 0) ivariant->instance_number = inst_num;
				}
				//
				if (!min_load && !mosaic && !uihgrid)
				{
					PRDisplayShutter a;
					if (read_shutter(ds, a))
					{
						if (images_ipp.size() == 1 && number_of_frames > 1)
						{
							ivariant->pr_display_shutters.insert(-1, a);
						}
						else
						{
							ivariant->pr_display_shutters.insert(0, a);
						}
					}
				}
			}
			else if (j > 0)
			{
				if (!min_load && !mosaic && !uihgrid)
				{
					PRDisplayShutter a;
					if (read_shutter(ds, a))
					{
						ivariant->pr_display_shutters.insert(j, a);
					}
				}
			}
			//
			if (!min_load)
			{
				{
					double tmp_c{-999999.0};
					double tmp_w{-999999.0};
					short lut_function{1};
					if (wsettings->get_level_for_PET() || !(
						(ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.128")) ||
						(ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.130")) ||
						(ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.128.1"))))
					{
						read_window(ds, &tmp_c, &tmp_w, &lut_function);
					}
					levels_.push_back(tmp_c);
					windows_.push_back(tmp_w);
					luts_.push_back(lut_function);
				}
				//
				{
					QString acqdate_tmp;
					QString acqtime_tmp;
					read_acquisition_time(ds, acqdate_tmp, acqtime_tmp);
					if (!acqdate_tmp.isEmpty() && !acqtime_tmp.isEmpty())
					{
						acqtimes.push_back(acqdate_tmp + acqtime_tmp);
					}
				}
			}
		}
		//
		double dircos_[6]{};
		unsigned int dimx_{};
		unsigned int dimy_{};
		unsigned int dimz_{};
		double origin_x_{};
		double origin_y_{};
		double origin_z_{};
		double spacing_x_{};
		double spacing_y_{};
		double spacing_z_{};
		double shift_tmp{};
		double scale_tmp{1.0};
		QString buff_error;
		const int overlays_idx = overlays_enabled ? j : -2;
		const bool rescale = (!apply_rescale) ? false : wsettings->get_rescale();
		const bool force_double_pf = (ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.128"));
		unsigned long long buffers_size{};
		std::vector<char*> data_;
		if (images_ipp.size() > 1)
		{
			buff_error = read_buffer(
				ok,
				data_,
				ivariant->image_overlays,
				overlays_idx,
				ivariant->anatomy,
				j,
				images_ipp.at(j),
				rescale,
				pixelformat, force_double_pf,
				pi,
				&dimx_, &dimy_, &dimz_,
				&origin_x_, &origin_y_, &origin_z_,
				&spacing_x_, &spacing_y_, &spacing_z_,
				dircos_,
				&shift_tmp, &scale_tmp,
				clean_unused_bits,
				false, false, elscint,
				false,
				pred6_bug,
				cornell_bug,
				fix_jpeg_prec,
				skip_too_large,
				nullptr,
				&buffers_size,
				use_icc, &icc_ok);
			if (dimz_ > 1)
			{
				*ok = false;
			}
			if (!(!data_.empty() && data_.at(0)))
			{
				*ok = false;
			}
			if (*ok == false)
			{
				for (unsigned int x = 0; x < data_.size(); ++x)
				{
					delete [] data_[x];
				}
				data_.clear();
				for (unsigned int x = 0; x < data.size(); ++x)
				{
					delete [] data[x];
				}
				data.clear();
				return QString("Buffer read failed");
			}
			data.push_back(&data_[0][0]);
			count_buffers_size += buffers_size;
#ifdef WARN_RAM_SIZE
			if (skip_too_large && total_ram > 0.0)
			{
				const double count_buffers_gb = count_buffers_size / 1073741824.0;
				if ((count_buffers_gb * 3) >= total_ram)
				{
					*ok = false;
					for (unsigned int x = 0; x < data.size(); ++x)
					{
						delete [] data[x];
					}
					data.clear();
					return QString(
						"The file seems to be too large, "
						"to try to load check \"Ignore RAM shortage\" in Settings");
				}
			}
#endif
		}
		else if (images_ipp.size() == 1)
		{
			buff_error = read_buffer(
				ok,
				data,
				ivariant->image_overlays,
				overlays_idx,
				ivariant->anatomy,
				j,
				images_ipp.at(j),
				rescale,
				pixelformat, force_double_pf,
				pi,
				&dimx_, &dimy_, &dimz_,
				&origin_x_, &origin_y_, &origin_z_,
				&spacing_x_, &spacing_y_, &spacing_z_,
				dircos_,
				&shift_tmp, &scale_tmp,
				clean_unused_bits,
				mosaic, uihgrid, elscint,
				false,
				pred6_bug,
				cornell_bug,
				fix_jpeg_prec,
				skip_too_large,
				nullptr,
				nullptr,
				use_icc, &icc_ok);
		}
		if (*ok == false)
		{
			for (unsigned int x = 0; x < data.size(); ++x)
			{
				delete [] data[x];
			}
			data.clear();
			return buff_error;
		}
		if (j == 0)
		{
			if (images_ipp.size() == 1)
			{
				ivariant->di->shift_tmp = shift_tmp;
				ivariant->di->scale_tmp = scale_tmp;
			}
			else
			{
				ivariant->di->shift_tmp = 0.0;
				ivariant->di->scale_tmp = 1.0;
			}
			dimx = dimx_;
			dimy = dimy_;
			if (images_ipp.size() == 1) dimz = dimz_;
			else dimz = images_ipp.size();
			if (slices_ok)
			{
				bool invalidate{};
				if (ivariant->equi)
				{
					const float tmp0_spacing_x = static_cast<float>(spacing_x_);
					const float tmp0_spacing_y = static_cast<float>(spacing_y_);
					const float tmp1_spacing_x = static_cast<float>(ivariant->di->ix_spacing);
					const float tmp1_spacing_y = static_cast<float>(ivariant->di->iy_spacing);
					const float tmp1_spacing_z = static_cast<float>(ivariant->di->iz_spacing);
					if (tmp1_spacing_x <= 0)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "ivariant->di->ix_spacing <= 0" << std::endl;
#endif
						invalidate = true;
						ivariant->di->ix_spacing = 1.0;
					}
					if (tmp1_spacing_y <= 0)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "ivariant->di->iy_spacing <= 0" << std::endl;
#endif
						invalidate = true;
						ivariant->di->iy_spacing = 1.0;
					}
					if (tmp1_spacing_z <= 0)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "ivariant->di->iz_spacing <= 0" << std::endl;
#endif
						invalidate = true;
						ivariant->di->iz_spacing = 0.00001;
					}
					if ((tmp0_spacing_x + 0.001f) < tmp1_spacing_x ||
						(tmp0_spacing_x - 0.001f) > tmp1_spacing_x)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_spacing_x != tmp1_spacing_x "
							<< tmp0_spacing_x << ' ' << tmp1_spacing_x << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_spacing_y + 0.001f) < tmp1_spacing_y ||
						(tmp0_spacing_y - 0.001f) > tmp1_spacing_y)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_spacing_y != tmp1_spacing_y "
							<< tmp0_spacing_y << ' ' << tmp1_spacing_y << std::endl;
#endif
						invalidate = true;
					}
					const float tmp0_origin_x = static_cast<float>(origin_x_);
					const float tmp0_origin_y = static_cast<float>(origin_y_);
					const float tmp0_origin_z = static_cast<float>(origin_z_);
					const float tmp1_origin_x = ivariant->di->ix_origin;
					const float tmp1_origin_y = ivariant->di->iy_origin;
					const float tmp1_origin_z = ivariant->di->iz_origin;
					if ((tmp0_origin_x + 0.001f) < tmp1_origin_x ||
						(tmp0_origin_x - 0.001f) > tmp1_origin_x)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_origin_x != tmp1_origin_x "
							<< tmp0_origin_x << ' ' << tmp1_origin_x << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_origin_y + 0.001f) < tmp1_origin_y ||
						(tmp0_origin_y - 0.001f) > tmp1_origin_y)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_origin_y != tmp1_origin_y "
							<< tmp0_origin_y << ' ' << tmp1_origin_y << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_origin_z + 0.001f) < tmp1_origin_z ||
						(tmp0_origin_z - 0.001f) > tmp1_origin_z)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_origin_z != tmp1_origin_z "
							<< tmp0_origin_z << ' ' << tmp1_origin_z << std::endl;
#endif
						invalidate = true;
					}
					const float tmp0_dircos_0 = static_cast<float>(dircos_[0]);
					const float tmp0_dircos_1 = static_cast<float>(dircos_[1]);
					const float tmp0_dircos_2 = static_cast<float>(dircos_[2]);
					const float tmp0_dircos_3 = static_cast<float>(dircos_[3]);
					const float tmp0_dircos_4 = static_cast<float>(dircos_[4]);
					const float tmp0_dircos_5 = static_cast<float>(dircos_[5]);
					const float tmp1_dircos_0 = ivariant->di->dircos[0];
					const float tmp1_dircos_1 = ivariant->di->dircos[1];
					const float tmp1_dircos_2 = ivariant->di->dircos[2];
					const float tmp1_dircos_3 = ivariant->di->dircos[3];
					const float tmp1_dircos_4 = ivariant->di->dircos[4];
					const float tmp1_dircos_5 = ivariant->di->dircos[5];
					if ((tmp0_dircos_0 + 0.001f) < tmp1_dircos_0 ||
						(tmp0_dircos_0 - 0.001f) > tmp1_dircos_0)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_dircos_0 != tmp1_dircos_0 "
							<< tmp0_dircos_0 << ' ' << tmp1_dircos_0 << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_dircos_1 + 0.001f) < tmp1_dircos_1 ||
						(tmp0_dircos_1 - 0.001f) > tmp1_dircos_1)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_dircos_1 != tmp1_dircos_1 "
							<< tmp0_dircos_1 << ' ' << tmp1_dircos_1 << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_dircos_2 + 0.001f) < tmp1_dircos_2 ||
						(tmp0_dircos_2 - 0.001f) > tmp1_dircos_2)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_dircos_2 != tmp1_dircos_2 "
							<< tmp0_dircos_2 << ' ' << tmp1_dircos_2 << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_dircos_3 + 0.001f) < tmp1_dircos_3 ||
						(tmp0_dircos_3 - 0.001f) > tmp1_dircos_3)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_dircos_3 != tmp1_dircos_3 "
							<< tmp0_dircos_3 << ' ' << tmp1_dircos_3 << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_dircos_4 + 0.001f) < tmp1_dircos_4 ||
						(tmp0_dircos_4 - 0.001f) > tmp1_dircos_4)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_dircos_4 != tmp1_dircos_4 "
							<< tmp0_dircos_4 << ' ' << tmp1_dircos_4 << std::endl;
#endif
						invalidate = true;
					}
					if ((tmp0_dircos_5 + 0.001f) < tmp1_dircos_5 ||
						(tmp0_dircos_5 - 0.001f) > tmp1_dircos_5)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "tmp0_dircos_5 != tmp1_dircos_5 "
							<< tmp0_dircos_5 << ' ' << tmp1_dircos_5 << std::endl;
#endif
						invalidate = true;
					}
				}
				if (invalidate)
				{
					ivariant->equi = false;
					ivariant->orientation = 0;
					ivariant->orientation_string = QString("");
#ifdef ALIZA_VERBOSE
					std::cout << "warning: could not validate image, using as non-uniform" << std::endl;
#endif
				}
				spacing_x = ivariant->di->ix_spacing > 0 ?  ivariant->di->ix_spacing : 1;
				spacing_y = ivariant->di->iy_spacing > 0 ?  ivariant->di->iy_spacing : 1;
				spacing_z = ivariant->di->iz_spacing > 0.00001 ?  ivariant->di->iz_spacing : 0.00001;
				origin_x = ivariant->di->ix_origin;
				origin_y = ivariant->di->iy_origin;
				origin_z = ivariant->di->iz_origin;
				const float row_dircos_x = ivariant->di->dircos[0];
				const float row_dircos_y = ivariant->di->dircos[1];
				const float row_dircos_z = ivariant->di->dircos[2];
				const float col_dircos_x = ivariant->di->dircos[3];
				const float col_dircos_y = ivariant->di->dircos[4];
				const float col_dircos_z = ivariant->di->dircos[5];
				const float nrm_dircos_x =
					row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y;
				const float nrm_dircos_y =
					row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
				const float nrm_dircos_z =
					row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
				direction[0][0] = row_dircos_x;
				direction[1][0] = row_dircos_y;
				direction[2][0] = row_dircos_z;
				direction[0][1] = col_dircos_x;
				direction[1][1] = col_dircos_y;
				direction[2][1] = col_dircos_z;
				direction[0][2] = nrm_dircos_x;
				direction[1][2] = nrm_dircos_y;
				direction[2][2] = nrm_dircos_z;
			}
			else
			{
				spacing_x = spacing_x_ > 0 ? spacing_x_ : 1;
				spacing_y = spacing_y_ > 0 ? spacing_y_ : 1;
				spacing_z = spacing_z_ > 0.00001 ? spacing_z_ : 0.00001;
				origin_x = origin_x_;
				origin_y = origin_y_;
				origin_z = origin_z_;
				const float row_dircos_x = dircos_[0];
				const float row_dircos_y = dircos_[1];
				const float row_dircos_z = dircos_[2];
				const float col_dircos_x = dircos_[3];
				const float col_dircos_y = dircos_[4];
				const float col_dircos_z = dircos_[5];
				const float nrm_dircos_x = row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y;
				const float nrm_dircos_y = row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
				const float nrm_dircos_z = row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
				direction[0][0] = row_dircos_x;
				direction[1][0] = row_dircos_y;
				direction[2][0] = row_dircos_z;
				direction[0][1] = col_dircos_x;
				direction[1][1] = col_dircos_y;
				direction[2][1] = col_dircos_z;
				direction[0][2] = nrm_dircos_x;
				direction[1][2] = nrm_dircos_y;
				direction[2][2] = nrm_dircos_z;
			}
			if (ivariant->di->iz_spacing <= 0.00001 && ivariant->one_direction)
			{
				ivariant->di->iz_spacing = 0.00001;
				ivariant->equi = false;
#if 0
				ivariant->di->skip_texture = true;
#endif
			}
		}
		else if (j > 0)
		{
			if ((previous_pixelformat.GetBitsAllocated() != pixelformat.GetBitsAllocated()) ||
				(previous_pixelformat.GetScalarType() != pixelformat.GetScalarType()) ||
				(previous_pixelformat.GetSamplesPerPixel() != pixelformat.GetSamplesPerPixel()))
			{
				*ok = false;
				for (unsigned int x = 0; x < data.size(); ++x)
				{
					delete [] data[x];
				}
				data.clear();
				return QString(
					"Files in series seem to have different "
					"pixel format. Try to load separately.");
			}
		}
		previous_pixelformat = pixelformat;
	}
	//
	if ((images_ipp.size() > 1) && (data.size() != dimz))
	{
		*ok = false;
		for (unsigned int x = 0; x < data.size(); ++x)
		{
			delete [] data[x];
		}
		data.clear();
		return QString("Mismatch data size and number of slices");
	}
	//
	{
		const size_t levels_size = levels_.size();
		if (levels_size > 0)
		{
			bool one_level{true};
			bool one_lut{true};
			for (size_t x = 0; x < levels_size; ++x)
			{
				FrameLevel fl;
				fl.lut_function = luts_.at(x);
				fl.us_window_center = levels_.at(x);
				fl.us_window_width = windows_.at(x);
				ivariant->frame_levels[x] = fl;
				//
				if (x > 0)
				{
					const bool b3 = (luts_.at(x) == luts_.at(x - 1));
					if (!b3) one_lut = false;
					const bool b1 = MMath::AlmostEqual(levels_.at(x), levels_.at(x - 1));
					const bool b2 = MMath::AlmostEqual(windows_.at(x), windows_.at(x - 1));
					if (!b1 || !b2) one_level = false;
				}
			}
			if (one_level)
			{
				ivariant->di->default_us_window_center =
					ivariant->di->us_window_center = levels_.at(0);
				ivariant->di->default_us_window_width  =
					ivariant->di->us_window_width = windows_.at(0);
			}
			if (one_lut)
			{
				ivariant->di->default_lut_function =
					ivariant->di->lut_function = luts_.at(0);
			}
		}
	}
	//
	const bool allow_geometry_from_image = (mosaic || uihgrid);
	const bool no_warn_rescale =
		(apply_rescale)
		? wsettings->get_rescale()
		: true;
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("before gen_itk_image()");
#endif
	QString error = CommonUtils::gen_itk_image(ok,
		data,
		pixelformat, pi,
		ivariant,
		direction,
		dimx, dimy, dimz,
		origin_x, origin_y, origin_z,
		spacing_x, spacing_y, spacing_z,
		geometry_from_image,
		allow_geometry_from_image,
		wsettings->get_resize(),
		wsettings->get_size_x(), wsettings->get_size_y(),
		no_warn_rescale,
		(use_icc && icc_ok),
		false);
	for (unsigned int x = 0; x < data.size(); ++x)
	{
		delete [] data[x];
	}
	data.clear();
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("after gen_itk_image()");
#endif
	if (*ok == false)
	{
		ivariant->anatomy.clear();
		ivariant->image_overlays.all_overlays.clear();
		return error;
	}
	if (!ivariant->frame_times.empty() &&
		(static_cast<int>(ivariant->frame_times.size()) !=
			ivariant->di->idimz))
	{
		ivariant->frame_times.clear();
	}
	if (!ivariant->equi)
	{
		if (!ivariant->one_direction)
			ivariant->di->transparency = false;
		ivariant->di->filtering = 0;
	}
	else
	{
		if (ivariant->di->idimz < 7)
			ivariant->di->transparency = false;
	}
	//
	if (!ivariant->image_instance_uids.empty() &&
		ivariant->image_instance_uids.size() != ivariant->di->idimz)
	{
		if (images_ipp.size() == 1 && ivariant->image_instance_uids.size() == 1 &&
			ivariant->image_instance_uids.contains(0))
		{
			const QString iuid = ivariant->image_instance_uids.value(0);
			for (int x = 1; x < ivariant->di->idimz; ++x)
			{
				ivariant->image_instance_uids[x] = iuid;
			}
		}
		else
		{
#ifdef ALIZA_VERBOSE
			std::cout << "Warning: instance UIDs mismatch, cleared UIDs" << std::endl;
#endif
			ivariant->image_instance_uids.clear();
		}
	}
	//
	if (!acqtimes.empty())
	{
		if (images_ipp.size() == acqtimes.size())
		{
			// 0 - acquisition time of the first slice after IPP/IOP sorting
			// 1 - acquisition time of the slice that was acquired first in series (earliest)
			// 2 - acqutsition time of the slice that was acquired last in series
			//
			// Note: there are also attributes 'Series Date' / 'Series Time'.
			//
			constexpr short use_acq_time = 0;
			//
			if (use_acq_time == 0)
			{
				// don't sort
			}
			else
			{
				if (use_acq_time == 1)
				{
					std::sort(acqtimes.begin(), acqtimes.end(), acqtime_less_than);
				}
				else if (use_acq_time == 2)
				{
					std::sort(acqtimes.begin(), acqtimes.end(), acqtime_more_than);
				}
			}
			const QString acquisitiondatetime = acqtimes.at(0);
			if (acquisitiondatetime.size() >= 14)
			{
				ivariant->acquisition_date = acquisitiondatetime.left(8);
				ivariant->acquisition_time = acquisitiondatetime.right(acquisitiondatetime.size() - 8);
			}
#if 0
			std::cout << "acqtimes" << std::endl;
			for (int k = 0; k < acqtimes.size(); ++k)
			{
				std::cout << acqtimes.at(k).toStdString() << std::endl;
			}
			std::cout << "series date/time" << std::endl;
			std::cout << ivariant->series_date.toStdString() << ivariant->series_time.toStdString() << std::endl;
			std::cout << "---------" << std::endl;
#endif
		}
	}
	//
	CommonUtils::reset_bb(ivariant);
	if (ivariant->modality == QString("US") || (ivariant->modality == QString("XA") &&
		ivariant->frame_times.size() > 1))
	{
		ivariant->di->selected_z_slice = 0;
	}
	return QString("");
}

bool DicomUtils::convert_elscint(const QString f, const QString outf)
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
	if (!reader.Read())
	{
		return false;
	}
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	mdcm::File & rfile = reader.GetFile();
	mdcm::DataSet & ds = rfile.GetDataSet();
	mdcm::FileMetaInformation & header = rfile.GetHeader();
	const mdcm::PrivateTag tcompressiontype(0x07a1,0x11,"ELSCINT1");
	const mdcm::DataElement & compressiontype = ds.GetDataElement(tcompressiontype);
	if (compressiontype.IsEmpty())
	{
		return false;
	}
	const mdcm::ByteValue * bv = compressiontype.GetByteValue();
	std::string comprle = "PMSCT_RLE1";
	std::string comprgb = "PMSCT_RGB1";
	bool isrle{};
	bool isrgb{};
	if (strncmp(bv->GetPointer(), comprle.c_str(), comprle.size()) == 0)
	{
		isrle = true;
	}
	else if (strncmp(bv->GetPointer(), comprgb.c_str(), comprgb.size()) == 0)
	{
		isrgb = true;
	}
	if (!isrle && !isrgb)
	{
		return false;
	}
	const mdcm::PrivateTag tprivatepixeldata(0x07a1,0x0a,"ELSCINT1");
	if (ds.FindDataElement(tprivatepixeldata))
	{
		const mdcm::DataElement & compressionpixeldata = ds.GetDataElement(tprivatepixeldata);
		if (compressionpixeldata.IsEmpty()) return false;
		const mdcm::ByteValue * bv2 = compressionpixeldata.GetByteValue();
		mdcm::Tag tpixeldata(0x7fe0, 0x0010);
		mdcm::DataElement pixeldata;
		if (isrle)
		{
			// Create or replace pixel data element 0x7fe0, 0x0010
			if (!ds.FindDataElement(tpixeldata))
			{
				pixeldata = mdcm::DataElement(tpixeldata, 0, mdcm::VR::OW);
			}
			else
			{
				pixeldata = ds.GetDataElement(tpixeldata);
			}
			pixeldata.SetVR(mdcm::VR::OW);
			const size_t bv2l = bv2->GetLength();
			mdcm::Attribute<0x0028,0x0010> at1;
			at1.SetFromDataSet(ds);
			mdcm::Attribute<0x0028,0x0011> at2;
			at2.SetFromDataSet(ds);
			const size_t w = at1.GetValue();
			const size_t h = at2.GetValue();
			const size_t at1l =	w * h * sizeof(unsigned short);
			if (bv2l == at1l)
			{
#ifdef ALIZA_VERBOSE
				std::cout << "Warning: Elscint data seems to be not compressed" << std::endl;
#endif
				pixeldata.SetByteValue(bv2->GetPointer(), bv2->GetLength());
			}
			else
			{
				std::vector<unsigned short> buffer;
				delta_decode(bv2->GetPointer(), bv2->GetLength(), buffer);
				// TODO check that decompress byte buffer match the expected size
				pixeldata.SetByteValue(
					reinterpret_cast<char*>(buffer.data()),
					static_cast<uint32_t>(buffer.size() * sizeof(unsigned short)));
			}
		}
		else if (isrgb)
		{
			// Create or replace pixel data element 0x7fe0, 0x0010
			if (!ds.FindDataElement(tpixeldata))
			{
				pixeldata = mdcm::DataElement(tpixeldata, 0, mdcm::VR::OW);
			}
			else
			{
				pixeldata = ds.GetDataElement(tpixeldata);
			}
			pixeldata.SetVR(mdcm::VR::OW);
			mdcm::Attribute<0x0028,0x0006> at0;
			at0.SetFromDataSet(ds);
			mdcm::Attribute<0x0028,0x0010> at1;
			at1.SetFromDataSet(ds);
			mdcm::Attribute<0x0028,0x0011> at2;
			at2.SetFromDataSet(ds);
			const size_t bv2l = bv2->GetLength();
			const size_t w = at1.GetValue();
			const size_t h = at2.GetValue();
			const size_t outputlen = 3 * h * w;
			if (bv2l == outputlen)
			{
#ifdef ALIZA_VERBOSE
				std::cout << "Warning: Elscint data seems to be not compressed" << std::endl;
#endif
				pixeldata.SetByteValue(bv2->GetPointer(), bv2->GetLength());

			}
			else
			{
				std::vector<unsigned char> buffer;
				delta_decode_rgb(
					reinterpret_cast<const unsigned char*>(bv2->GetPointer()), bv2l, buffer,
					at0.GetValue(), w, h);
				pixeldata.SetByteValue(
					reinterpret_cast<char*>(buffer.data()),
					static_cast<uint32_t>(buffer.size()));
			}
		}
		// Add the pixel data element
		if (ds.FindDataElement(tpixeldata))
		{
			ds.Replace(pixeldata);
		}
		else
		{
			ds.ReplaceEmpty(pixeldata);
		}
		//
		//
		//
		if (ds.FindDataElement(mdcm::Tag(0x07a1,0x0010)))
		{
			ds.Remove(mdcm::Tag(0x07a1,0x0010));
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x100a)))
			{
				ds.Remove(mdcm::Tag(0x07a1,0x100a));
			}
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1010)))
			{
				ds.Remove(mdcm::Tag(0x07a1,0x1010));
			}
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1011)))
			{
				ds.Remove(mdcm::Tag(0x07a1,0x1011));
			}
		}
	}
	else
	{
		const mdcm::Tag tpixeldata(0x7fe0, 0x0010);
		const mdcm::DataElement & epixeldata = ds.GetDataElement(tpixeldata);
		if (epixeldata.IsEmpty()) return false;
		const mdcm::ByteValue * bv2 = epixeldata.GetByteValue();
		mdcm::DataElement pixeldata;
		if (isrle)
		{
			const size_t bv2l = bv2->GetLength();
			mdcm::Attribute<0x0028,0x0010> at1;
			at1.SetFromDataSet(ds);
			mdcm::Attribute<0x0028,0x0011> at2;
			at2.SetFromDataSet(ds);
			const size_t w = at1.GetValue();
			const size_t h = at2.GetValue();
			const size_t at1l = w * h * sizeof(unsigned short);
			if (bv2l == at1l)
			{
#ifdef ALIZA_VERBOSE
				std::cout << "Warning: Elscint data seems to be not compressed" << std::endl;
#endif
			}
			else
			{
				pixeldata = ds.GetDataElement(tpixeldata);
				pixeldata.SetVR(mdcm::VR::OW);
				std::vector<unsigned short> buffer;
				delta_decode(bv2->GetPointer(), bv2->GetLength(), buffer);
				// TODO check that decompress byte buffer match the expected size
				pixeldata.SetByteValue(
					reinterpret_cast<char*>(buffer.data()),
					static_cast<uint32_t>(buffer.size() * sizeof(unsigned short)));
			}
		}
		else if (isrgb)
		{
			mdcm::Attribute<0x0028,0x0006> at0;
			at0.SetFromDataSet(ds);
			mdcm::Attribute<0x0028,0x0010> at1;
			at1.SetFromDataSet(ds);
			mdcm::Attribute<0x0028,0x0011> at2;
			at2.SetFromDataSet(ds);
			const size_t bv2l = bv2->GetLength();
			const size_t w = at1.GetValue();
			const size_t h = at2.GetValue();
			const size_t outputlen = 3 * h * w;
			if (bv2l == outputlen)
			{
#ifdef ALIZA_VERBOSE
				std::cout << "Warning: Elscint data seems to be not compressed" << std::endl;
#endif
			}
			else
			{
				pixeldata = ds.GetDataElement(tpixeldata);
				pixeldata.SetVR(mdcm::VR::OW);
				std::vector<unsigned char> buffer;
				delta_decode_rgb(
					reinterpret_cast<const unsigned char*>(bv2->GetPointer()), bv2->GetLength(), buffer,
					at0.GetValue(), w, h);
				pixeldata.SetByteValue(
					reinterpret_cast<char*>(buffer.data()),
					static_cast<uint32_t>(buffer.size()));
			}
		}
		// Add the pixel data element
		ds.Replace(pixeldata);
		//
		//
		//
		if (ds.FindDataElement(mdcm::Tag(0x07a1,0x0010)))
		{
			ds.Remove(mdcm::Tag(0x07a1,0x0010));
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1010)))
			{
				ds.Remove(mdcm::Tag(0x07a1,0x1010));
			}
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1011)))
			{
				ds.Remove(mdcm::Tag(0x07a1,0x1011));
			}
		}
	}
	if (ds.FindDataElement(mdcm::Tag(0x7fe0,0x0)))
	{
		ds.Remove(mdcm::Tag(0x7fe0,0x0));
	}
	if (ds.FindDataElement(mdcm::Tag(0xfffc,0xfffc)))
	{
		ds.Remove(mdcm::Tag(0xfffc,0xfffc));
	}
	//
	//
	//
	header.SetDataSetTransferSyntax(mdcm::TransferSyntax::ExplicitVRLittleEndian);
	mdcm::Writer writer;
	writer.SetFile(reader.GetFile());
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	writer.SetFileName(QDir::toNativeSeparators(outf).toUtf8().constData());
#else
	writer.SetFileName(QDir::toNativeSeparators(outf).toLocal8Bit().constData());
#endif
#else
	writer.SetFileName(outf.toLocal8Bit().constData());
#endif
	if (!writer.Write())
	{
#ifdef ALIZA_VERBOSE
		std::cout << "Error: can not write Elscint file " << outf.toStdString() << std::endl;
#endif
		return false;
	}
	return true;
}

QString DicomUtils::read_buffer(
	bool * ok, std::vector<char*> & data,
	ImageOverlays & image_overlays,
	const int overlay_idx, // -2 to disable
	AnatomyMap & anatomy, const int anatomy_idx, // unused for enhanced
	const QString & f,
	const bool rescale,
	mdcm::PixelFormat & pixelformat, const bool force_double_pf,
	mdcm::PhotometricInterpretation & pi,
	unsigned int * dimx_, unsigned int * dimy_, unsigned int * dimz_,
	double * origin_x, double * origin_y, double * origin_z,
	double * spacing_x, double * spacing_y, double * spacing_z,
	double * dircos,
	double * shift_tmp, double * scale_tmp,
	const bool clean_unused_bits,
	const bool mosaic,
	const bool uihgrid,
	const bool elscint,
	const bool supp_palette_color,
	const bool pred6_bug,
	const bool cornell_bug,
	const bool fix_jpeg_prec,
	const bool skip_too_large,
	int * red_subscript,
	unsigned long long * buffers_size,
	const bool use_icc, bool * has_icc)
{
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_buffer() begin");
#endif
	*ok = false;
	mdcm::ImageHelper::SetForceRescaleInterceptSlope(true);
	mdcm::ImageHelper::SetWorkaroundPredictorBug(pred6_bug);
	mdcm::ImageHelper::SetWorkaroundCornellBug(cornell_bug);
	mdcm::ImageHelper::SetCleanUnusedBits(clean_unused_bits);
	mdcm::ImageHelper::SetFixJpegBits(fix_jpeg_prec);
	//
	bool rescale_{};
	unsigned long long rescaled_buffer_size{};
	unsigned long long buffer_size{};
	char * rescaled_buffer{};
	char * not_rescaled_buffer{};
	unsigned char * singlebit_buffer{};
	char * buffer{};
	bool singlebit{};
	unsigned int type_size{};
	unsigned int samples_per_pix{};
	double rescale_intercept{};
	double rescale_slope{1.0};
	unsigned long long dimx{};
	unsigned long long dimy{};
	unsigned long long dimz{};
	mdcm::PixelFormat image_pixelformat = mdcm::PixelFormat::UNKNOWN;
	unsigned long long image_buffer_length{};
	QString elscf;
	short icc_for_ybr{};
	char * icc_profile{};
	unsigned int icc_size{};
	//
	const mdcm::Tag tModalityLUTSequence(0x0028, 0x3000);
	bool has_modality_lut{};
	QList<QVariant> lut_descriptor;
	QList<QVariant> lut_data;
	bool mapped_implicit{};
	bool mapped_signed{};
	bool modality_lut_ok{};
	//
	{
		mdcm::ImageReader image_reader;
		if (elscint)
		{
			QFileInfo fi(f);
			elscf = QDir::tempPath() + QString("/") + fi.fileName() + QString("ELSCINT.dcm");
			const bool elsc_ok = convert_elscint(f, elscf);
			if (elsc_ok)
			{
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
				image_reader.SetFileName(QDir::toNativeSeparators(elscf).toUtf8().constData());
#else
				image_reader.SetFileName(QDir::toNativeSeparators(elscf).toLocal8Bit().constData());
#endif
#else
				image_reader.SetFileName(elscf.toLocal8Bit().constData());
#endif
			}
			else
			{
				QFile::remove(elscf);
				return QString("Can not convert ELSCINT file");
			}
		}
		else
		{
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
			image_reader.SetFileName(QDir::toNativeSeparators(f).toUtf8().constData());
#else
			image_reader.SetFileName(QDir::toNativeSeparators(f).toLocal8Bit().constData());
#endif
#else
			image_reader.SetFileName(f.toLocal8Bit().constData());
#endif
			image_reader.SetApplySupplementalLUT(supp_palette_color);
		}
		if (overlay_idx == -2) image_reader.SetProcessOverlays(false);
		const bool i_ok = image_reader.Read();
		if (!i_ok)
		{
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("!image_reader.Read()");
		}
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		mdcm::Image & image = image_reader.GetImage();
		{
			const unsigned long long buffer_size_tmp = image.GetBufferLength();
#ifdef WARN_RAM_SIZE
			const double total_ram = CommonUtils::get_total_memory_saved();
			if (skip_too_large && total_ram > 0.0)
			{
				const double buffer_gb = buffer_size_tmp / 1073741824.0;
				if ((buffer_gb * 3) >= total_ram)
				{
					return QString(
						"The file seems to be too large, "
						"to try to load check \"Ignore RAM shortage\" in Settings");
				}
			}
#endif
			if (buffer_size_tmp > 0xffffffff)
			{
// https://groups.google.com/g/comp.protocols.dicom/c/-tO2v2aH010/m/PNGwaLpBkBsJ
				const mdcm::File & ifile = image_reader.GetFile();
				const mdcm::FileMetaInformation & iheader = ifile.GetHeader();
				const mdcm::TransferSyntax & ts = iheader.GetDataSetTransferSyntax();
				bool skip{true};
				if (ts.IsEncapsulated())
				{
					if (sizeof(void*) >= 8)
					{
						skip = false;
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: GetBufferLength()=" << buffer_size_tmp << std::endl;
#endif
					}
				}
				if (skip)
				{
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return (QString("GetBufferLength() is ") +
						QVariant(buffer_size_tmp).toString());
				}
			}
		}
		//
		if (mosaic)
		{
			mdcm::SplitMosaicFilter mfilter;
			mfilter.SetImage(image);
			mfilter.SetFile(image_reader.GetFile());
			if (mfilter.Split()) image = mfilter.GetImage();
		}
		else if (uihgrid)
		{
			mdcm::SplitUihGridFilter mfilter;
			mfilter.SetImage(image);
			mfilter.SetFile(image_reader.GetFile());
			if (mfilter.Split()) image = mfilter.GetImage();
		}
		*dimx_     = image.GetDimension(0);
		*dimy_     = image.GetDimension(1);
		*dimz_     = image.GetDimension(2);
		dimx = *dimx_;
		dimy = *dimy_;
		dimz = *dimz_;
		*origin_x  = image.GetOrigin(0);
		*origin_y  = image.GetOrigin(1);
		*origin_z  = image.GetOrigin(2);
		*spacing_x = image.GetSpacing(0);
		*spacing_y = image.GetSpacing(1);
		*spacing_z = image.GetSpacing(2);
		const double * dircos_ = image.GetDirectionCosines();
		dircos[0] = dircos_[0];
		dircos[1] = dircos_[1];
		dircos[2] = dircos_[2];
		dircos[3] = dircos_[3];
		dircos[4] = dircos_[4];
		dircos[5] = dircos_[5];
		pi = image.GetPhotometricInterpretation();
		if (overlay_idx != -2)
		{
			const size_t ov_count = image.GetNumberOfOverlays();
			if (ov_count > 0)
			{
				QMultiMap<int, SliceOverlay> slice_overlays;
				for (size_t ov = 0; ov < ov_count; ++ov)
				{
					mdcm::Overlay & o = image.GetOverlay(ov);
					const unsigned int NumberOfFrames = o.GetNumberOfFrames();
					const unsigned int FrameOrigin = o.GetFrameOrigin();
					const size_t o_dimx = o.GetColumns();
					const size_t o_dimy = o.GetRows();
					const int o_x = o.GetOrigin()[0];
					const int o_y = o.GetOrigin()[1];
					if ((NumberOfFrames > 1) && (FrameOrigin > 0) && (overlay_idx < 0))
					{
						const size_t obuffer_size = o.GetUnpackBufferLength();
						if (obuffer_size%NumberOfFrames != 0) continue;
						const size_t fbuffer_size = obuffer_size/NumberOfFrames;
						char * tmp0;
						try
						{
							tmp0 = new char[obuffer_size];
						}
						catch (const std::bad_alloc&)
						{
							continue;
						}
						if (!tmp0)
						{
							continue;
						}
						const bool obuffer_ok = o.GetUnpackBuffer(tmp0, obuffer_size);
						if (!obuffer_ok)
						{
							delete [] tmp0;
							continue;
						}
						int idx = FrameOrigin - 1;
						for (unsigned int y = 0; y < NumberOfFrames; ++y)
						{
							SliceOverlay overlay;
							overlay.dimx = o_dimx;
							overlay.dimy = o_dimy;
							overlay.x    = o_x;
							overlay.y    = o_y;
							const size_t p = idx * o_dimx * o_dimy;
							++idx;
							for (size_t j = 0; j < fbuffer_size; ++j)
							{
								const size_t jj = p + j;
								if (jj < obuffer_size)
								{
									overlay.data.push_back(tmp0[jj]);
								}
								else
								{
#ifdef ALIZA_VERBOSE
									std::cout << "warning: read_buffer() jj="
										<< jj << " obuffer_size"
										<< obuffer_size << std::endl;
#endif
								}
							}
							slice_overlays.insert(idx - 1, overlay);
#if 0
							std::cout << "obuffer_size=" << obuffer_size
								<< " fbuffer_size=" << fbuffer_size
								<< " idx=" << idx-1
								<< " p=" << p << std::endl;
#endif
						}
						delete [] tmp0;
					}
					else
					{
						SliceOverlay overlay;
						overlay.dimx = o_dimx;
						overlay.dimy = o_dimy;
						overlay.x    = o_x;
						overlay.y    = o_y;
						const size_t obuffer_size = o.GetUnpackBufferLength();
						char * tmp0;
						try
						{
							tmp0 = new char[obuffer_size];
						}
						catch (const std::bad_alloc&)
						{
							continue;
						}
						if (!tmp0)
						{
							continue;
						}
						const bool obuffer_ok = o.GetUnpackBuffer(tmp0, obuffer_size);
						if (!obuffer_ok)
						{
							delete [] tmp0;
							continue;
						}
						for (size_t j = 0; j < obuffer_size; ++j)
						{
							overlay.data.push_back(tmp0[j]);
						}
						delete [] tmp0;
						slice_overlays.insert(overlay_idx, overlay);
#if 0
							std::cout
								<< "obuffer_size=" << obuffer_size
								<< " idx=" << overlay_idx << std::endl;
#endif
					}
				}
				const QList<int> keys = slice_overlays.keys();
				for (int x = 0; x < keys.size(); ++x)
				{
					const int idx = keys.at(x);
					SliceOverlays l2 = slice_overlays.values(idx);
					if (!image_overlays.all_overlays.contains(idx))
					{
						image_overlays.all_overlays[idx] = std::move(l2);
#if 0
							std::cout << "image_overlays.all_overlays["
								<< idx << "] = l2"
								<< std::endl;
#endif
					}
					else
					{
						for (int j = 0; j < l2.size(); ++j)
						{
							image_overlays.all_overlays[idx].push_back(l2[j]);
#if 0
							std::cout << "image_overlays.all_overlays["
								<< idx << "].push_back(l2[" << j << "])"
								<< std::endl;
#endif
						}
					}
				}
				slice_overlays.clear();
			}
		}
		//
		{
			// Modality LUT
			const mdcm::File & ifile = image_reader.GetFile();
			const mdcm::DataSet & ds = ifile.GetDataSet();
			has_modality_lut = has_modality_lut_sq(ds);
			if (has_modality_lut)
			{
				modality_lut_ok = DicomUtils::read_gray_lut(
					ds, tModalityLUTSequence,
					lut_descriptor, lut_data,
					&mapped_implicit, &mapped_signed);
			}
		}
		//
		if (anatomy_idx > -1)
		{
			const mdcm::File & ifile = image_reader.GetFile();
			const mdcm::DataSet & ds = ifile.GetDataSet();
			AnatomyDesc a;
			a.laterality = read_image_laterality(ds);
			if (a.laterality.isEmpty())
			{
				a.laterality = read_series_laterality(ds);
			}
			a.body_part = read_anatomic_sq(ds);
			if (a.body_part.isEmpty()) a.body_part = read_body_part(ds);
			anatomy[anatomy_idx] = a;
			if (anatomy_idx == 0 && dimz > 1)
			{
				for (size_t x = 1; x < dimz; ++x) anatomy[x] = a;
			}
		}
		//
		if (use_icc)
		{
			const mdcm::File & ifile = image_reader.GetFile();
			const mdcm::DataSet & ds = ifile.GetDataSet();
			const mdcm::DataElement & icc_e = ds.GetDataElement(mdcm::Tag(0x0028,0x2000));
			if (!icc_e.IsEmpty() && !icc_e.IsUndefinedLength())
			{
				const mdcm::ByteValue * icc_bv = icc_e.GetByteValue();
				if (icc_bv && icc_bv->GetPointer() && (icc_bv->GetLength() > 0))
				{
					icc_size = icc_bv->GetLength();
					icc_profile = new char[icc_size];
					memcpy(icc_profile, icc_bv->GetPointer(), icc_size);
				}
			}
		}
		//
		if (image.GetPlanarConfiguration() == 1)
		{
			mdcm::ImageChangePlanarConfiguration icpc;
			icpc.SetInput(image);
			icpc.SetPlanarConfiguration(0);
			if (icpc.Change())
			{
				image = icpc.GetOutput();
				pixelformat = image.GetPixelFormat();
			}
			else
			{
#ifdef ALIZA_VERBOSE
				std::cout << "Error: failed to change Planar Configuration 1 to 0" << std::endl;
#endif
			}
		}
		//
		if (pi == mdcm::PhotometricInterpretation::PALETTE_COLOR)
		{
			mdcm::ImageApplyLookupTable ialut;
			ialut.SetInput(image);
			ialut.Apply();
			image = ialut.GetOutput();
		}
		else if (supp_palette_color)
		{
			if (!red_subscript)
			{
				delete [] icc_profile;
				if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
				return QString(
					"Error (subscript is null), "
					"can not apply Supplemental LUT");
			}
			mdcm::ApplySupplementalLUT slut;
			slut.SetInput(image);
			slut.Apply();
			image = slut.GetOutput();
			*red_subscript = slut.GetRedSubscript();
		}
		else if (pi == mdcm::PhotometricInterpretation::MONOCHROME1 ||
				 pi == mdcm::PhotometricInterpretation::MONOCHROME2)
		{
			rescale_intercept = image.GetIntercept();
			rescale_slope     = image.GetSlope();
		}
		else if (pi == mdcm::PhotometricInterpretation::YBR_FULL ||
				 pi == mdcm::PhotometricInterpretation::YBR_FULL_422)
		{
			if (use_icc) icc_for_ybr = 1;
		}
		else if (pi == mdcm::PhotometricInterpretation::YBR_PARTIAL_422)
		{
			if (use_icc) icc_for_ybr = 2;
		}
		//
		//
		//
		image_pixelformat = image.GetPixelFormat();
		image_buffer_length = image.GetBufferLength();
		if (image_buffer_length == 0)
		{
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer size is invalid");
		}
		if (buffers_size)
		{
			*buffers_size = image_buffer_length;
		}
		//
		//
		//
		try
		{
			not_rescaled_buffer = new char[image_buffer_length];
		}
		catch (const std::bad_alloc&)
		{
			not_rescaled_buffer = nullptr;
		}
		if (!not_rescaled_buffer)
		{
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer allocation error");
		}
		if (!image.GetBuffer(not_rescaled_buffer))
		{
			delete [] not_rescaled_buffer;
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer is null");
		}
	}
	//
	//
	//
	if (pi == mdcm::PhotometricInterpretation::MONOCHROME1 ||
		pi == mdcm::PhotometricInterpretation::MONOCHROME2)
	{
		if (!supp_palette_color && rescale)
		{
			*shift_tmp = rescale_intercept;
			*scale_tmp = rescale_slope;
			if (force_double_pf || !(
				rescale_intercept >= 0        &&
				rescale_intercept <  0.000001 &&
				rescale_slope     >  0.999999 &&
				rescale_slope     <  1.000001))
			{
				if (pixelformat.GetBitsAllocated() < 8)
				{
					delete [] not_rescaled_buffer;
					delete [] icc_profile;
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return QString(
						"Bits allocated < 8 and rescale, "
						"not supported.");
				}
				if (supp_palette_color)
				{
					delete [] not_rescaled_buffer;
					delete [] icc_profile;
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return QString("Re-scale and Suppl. LUT?");
				}
				mdcm::Rescaler r;
				r.SetIntercept(rescale_intercept);
				r.SetSlope(rescale_slope);
				r.SetPixelFormat(image_pixelformat);
				r.SetUseTargetPixelType(true);
				if (!force_double_pf)
				{
					pixelformat = mdcm::PixelFormat(r.ComputeInterceptSlopePixelType());
				}
				else
				{
					pixelformat = mdcm::PixelFormat::FLOAT64;
				}
				r.SetTargetPixelType(pixelformat);
				{
					unsigned int rescale_type_size{};
					if ((pixelformat.GetBitsAllocated() % 8) != 0)
					{
						if (pixelformat.GetBitsAllocated() < 8)
						{
							rescale_type_size = 8;
						}
						else if (pixelformat.GetBitsAllocated() < 16)
						{
							rescale_type_size = 16;
						}
						else if (pixelformat.GetBitsAllocated() < 32)
						{
							rescale_type_size = 32;
						}
						else if (pixelformat.GetBitsAllocated() < 64)
						{
							rescale_type_size = 64;
						}
						else
						{
							delete [] not_rescaled_buffer;
							delete [] icc_profile;
							if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
							return QString("Internal error (re-scale)");
						}
					}
					else
					{
						rescale_type_size = pixelformat.GetBitsAllocated() / 8;
					}
					rescaled_buffer_size
						= dimx * dimy * dimz * rescale_type_size * pixelformat.GetSamplesPerPixel();
					try
					{
						rescaled_buffer = new char[rescaled_buffer_size];
					}
					catch(const std::bad_alloc&)
					{
						rescaled_buffer = nullptr;
					}
					if (!rescaled_buffer)
					{
						delete [] not_rescaled_buffer;
						delete [] icc_profile;
						if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
						return QString("Buffer is null");
					}
					const bool ok_rescale = r.Rescale(rescaled_buffer, not_rescaled_buffer, image_buffer_length);
					if (ok_rescale)
					{
						rescale_ = true;
						delete [] not_rescaled_buffer;
						not_rescaled_buffer = nullptr;
					}
					else
					{
#ifdef ALIZA_VERBOSE
						std::cout << f.toStdString() << " : rescaling failed" << std::endl;
#endif
						pixelformat = image_pixelformat;
						delete [] rescaled_buffer;
						rescaled_buffer = nullptr;
					}
				}
			}
			else
			{
				// Modality LUT
				if (has_modality_lut)
				{
#if 0
					std::cout << "Modality LUT sequence" << std::endl;
#endif
					const unsigned short pf_ba = image_pixelformat.GetBitsAllocated();
					if (modality_lut_ok && image_pixelformat.GetSamplesPerPixel() == 1 &&
						(pf_ba == 8 || pf_ba == 16))
					{
						const bool pixel_signed = (image_pixelformat.GetPixelRepresentation() == 1);
						const int d0 = lut_descriptor.at(0).toInt();
						const int d1 = lut_descriptor.at(1).toInt();
						const int d2 = lut_descriptor.at(2).toInt();
						(void)d0;
						(void)d2;
						int i1{};
						bool data_possible_negative{};
#if 1
						if (pixel_signed)
						{
							i1 = static_cast<signed short>(d1);
							data_possible_negative = true;
						}
						else
						{
							i1 = static_cast<unsigned short>(d1);
						}
						(void)mapped_implicit;
						(void)mapped_signed;
#else
						if (mapped_implicit)
						{
							if (pixel_signed)
							{
								i1 = static_cast<signed short>(d1);
								data_possible_negative = true;
							}
							else
							{
								i1 = static_cast<unsigned short>(d1);
							}
						}
						else
						{
							if (mapped_signed)
							{
								i1 = static_cast<signed short>(d1);
								data_possible_negative = true;
							}
							else
							{
								i1 = static_cast<unsigned short>(d1);
							}
						}
#endif
						mdcm::PixelFormat mod_pixelformat(mdcm::PixelFormat::FLOAT32);
						rescaled_buffer_size = dimx * dimy * dimz * (mod_pixelformat.GetBitsAllocated() / 8);
						try
						{
							rescaled_buffer = new char[rescaled_buffer_size];
						}
						catch (const std::bad_alloc&)
						{
							rescaled_buffer = nullptr;
						}
						if (!rescaled_buffer)
						{
							delete [] not_rescaled_buffer;
							delete [] icc_profile;
							if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
							return QString("Buffer is null");
						}
						const size_t lut_data_s = lut_data.size();
						if (pf_ba == 8)
						{
							void * vrescaled_buffer = static_cast<void*>(rescaled_buffer);
							float * tmp_data = static_cast<float*>(vrescaled_buffer);
							for (size_t x = 0; x < dimx * dimy * dimz; ++x)
							{
								float out{};
								int idx = (pixel_signed)
									? not_rescaled_buffer[x]
									: static_cast<unsigned char>(not_rescaled_buffer[x]);
								if (i1 < 0)
								{
									idx += (-i1);
								}
								if (idx >= 0)
								{
									if (static_cast<size_t>(idx) < lut_data_s)
									{
										if (data_possible_negative)
										{
											out = static_cast<float>(static_cast<signed char>(
												lut_data.at(idx).toInt()));
										}
										else
										{
											out = static_cast<float>(static_cast<unsigned char>(
												lut_data.at(idx).toInt()));
										}
									}
									else if (static_cast<size_t>(idx) >= lut_data_s)
									{
										if (data_possible_negative)
										{
											out = static_cast<float>(static_cast<signed char>(
												lut_data.at(lut_data_s - 1).toInt()));
										}
										else
										{
											out = static_cast<float>(static_cast<unsigned char>(
												lut_data.at(lut_data_s - 1).toInt()));
										}
									}
								}
								else
								{
									if (data_possible_negative)
									{
										out = static_cast<float>(static_cast<signed char>(
											lut_data.at(0).toInt()));
									}
									else
									{
										out = static_cast<float>(
											static_cast<unsigned char>(lut_data.at(0).toInt()));
									}
								}
								tmp_data[x] = out;
							}
						}
						else if (pf_ba == 16)
						{
							void * vrescaled_buffer = static_cast<void*>(rescaled_buffer);
							float * tmp_data = static_cast<float*>(vrescaled_buffer);
							void * vnot_rescaled_buffer = static_cast<void*>(not_rescaled_buffer);
							unsigned short * tmp_not_rescaled = static_cast<unsigned short*>(vnot_rescaled_buffer);
							for (size_t x = 0; x < dimx * dimy * dimz; ++x)
							{
								float out{};
								int idx = (pixel_signed)
									? static_cast<signed short>(tmp_not_rescaled[x])
									: tmp_not_rescaled[x];
								if (i1 < 0)
								{
									idx += (-i1);
								}
								if (idx >= 0)
								{
									if (static_cast<size_t>(idx) < lut_data_s)
									{
										if (data_possible_negative)
										{
											out = static_cast<float>(
												static_cast<signed short>(lut_data.at(idx).toInt()));
										}
										else
										{
											out = static_cast<float>(
												static_cast<unsigned short>(lut_data.at(idx).toInt()));
										}
									}
									else if (static_cast<size_t>(idx) >= lut_data_s)
									{
										if (data_possible_negative)
										{
											out = static_cast<float>(
												static_cast<signed short>(lut_data.at(lut_data_s - 1).toInt()));
										}
										else
										{
											out = static_cast<float>(
												static_cast<unsigned short>(lut_data.at(lut_data_s - 1).toInt()));
										}
									}
								}
								else
								{
									if (data_possible_negative)
									{
										out = static_cast<float>(
											static_cast<signed short>(lut_data.at(0).toInt()));
									}
									else
									{
										out = static_cast<float>(
											static_cast<unsigned short>(lut_data.at(0).toInt()));
									}
								}
								tmp_data[x] = out;
							}
						}
						pixelformat = mod_pixelformat;
						buffer      = rescaled_buffer;
						buffer_size = rescaled_buffer_size;
						rescale_ = true;
					}
					else
					{
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: failed to apply modality LUT" << std::endl;
#endif
						pixelformat = image_pixelformat;
					}
				}
				else
				{
					pixelformat = image_pixelformat;
				}
			}
		}
		else
		{
#if 1
			*shift_tmp = rescale_intercept;
			*scale_tmp = rescale_slope;
#else
			*shift_tmp = 0.0;
			*scale_tmp = 1.0;
#endif
			pixelformat = image_pixelformat;
		}
	}
	else
	{
		*shift_tmp = 0.0;
		*scale_tmp = 1.0;
		pixelformat = image_pixelformat;
	}
	samples_per_pix = pixelformat.GetSamplesPerPixel();
	if (pixelformat.GetBitsAllocated() < 8)
	{
		if (samples_per_pix > 1)
		{
			const QString tmp_s0 =
				QString("Bits allocated = ") +
				QVariant(static_cast<int>(pixelformat.GetBitsAllocated())).toString() +
				QString(", samples per pixel = ") +
				QVariant(static_cast<int>(samples_per_pix)).toString() +
				QString(", not supported.");
			delete [] not_rescaled_buffer;
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return tmp_s0;
		}
		if (pixelformat.GetBitsAllocated() == 1)
		{
			singlebit = true;
			type_size = 1;
		}
		else
		{
			const QString tmp_s0 =
				QString("Bits allocated = ") +
				QVariant(static_cast<int>(pixelformat.GetBitsAllocated())).toString() +
				QString(", not supported.");
			delete [] not_rescaled_buffer;
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return tmp_s0;
		}
	}
	else
	{
		type_size = ((pixelformat.GetBitsAllocated() % 8) != 0)
					? static_cast<unsigned int>(ceil(pixelformat.GetBitsAllocated() / 8.0))
					: pixelformat.GetBitsAllocated() / 8;
	}
	if (singlebit)
	{
		const unsigned long long singlebit_buffer_size = dimx * dimy * dimz;
		try
		{
			singlebit_buffer = new unsigned char[singlebit_buffer_size];
		}
		catch (const std::bad_alloc&)
		{
			singlebit_buffer = nullptr;
		}
		if (!singlebit_buffer)
		{
			delete [] not_rescaled_buffer;
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer allocation error");
		}
		unsigned long long j{};
		for (unsigned long long x = 0; x < image_buffer_length; ++x)
		{
			const unsigned char c = not_rescaled_buffer[x];
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c &  0x1) ? 255 : 0;
			++j;
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c &  0x2) ? 255 : 0;
			++j;
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c &  0x4) ? 255 : 0;
			++j;
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c &  0x8) ? 255 : 0;
			++j;
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c & 0x10) ? 255 : 0;
			++j;
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c & 0x20) ? 255 : 0;
			++j;
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c & 0x40) ? 255 : 0;
			++j;
			if (j < singlebit_buffer_size) singlebit_buffer[j] = (c & 0x80) ? 255 : 0;
			++j;
		}
		buffer      = reinterpret_cast<char *>(singlebit_buffer);
		buffer_size = singlebit_buffer_size;
	}
	else
	{
		if (rescale_ && rescaled_buffer)
		{
			buffer      = rescaled_buffer;
			buffer_size = rescaled_buffer_size;
		}
		else if (not_rescaled_buffer)
		{
			if (image_buffer_length != dimx * dimy * dimz * type_size * samples_per_pix)
			{
				const QString tmp_s0 =
					QString("Buffer size wrong ") +
					QVariant(image_buffer_length).toString() +
					QString(" but must be ") +
					QVariant(dimx * dimy * dimz * type_size * samples_per_pix).toString();
				delete [] not_rescaled_buffer;
				delete [] rescaled_buffer;
				delete [] icc_profile;
				if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
				return tmp_s0;
			}
			if (icc_size > 0 && icc_profile  && (type_size == 1 || type_size == 2) &&
				samples_per_pix == 3)
			{
#ifdef ALIZA_VERBOSE
				std::cout << "Using ICC profile" << std::endl;
#endif
				char * icc_tmp{};
				char * icc_buffer;
				try
				{
					icc_buffer = new char[image_buffer_length];
				}
				catch (const std::bad_alloc &)
				{
					delete [] not_rescaled_buffer;
					delete [] rescaled_buffer;
					delete [] icc_profile;
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return QString("Memory allocation error");
				}
				if (icc_for_ybr > 0 && type_size == 1)
				{
					try
					{
						icc_tmp = new char[image_buffer_length];
					}
					catch (const std::bad_alloc &)
					{
						delete [] not_rescaled_buffer;
						delete [] rescaled_buffer;
						delete [] icc_buffer;
						delete [] icc_profile;
						if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
						return QString("Memory allocation error");
					}
					for (size_t j = 0; j < image_buffer_length; j+=3)
					{
						// TODO the code is partially duplicated with commonutils.cpp
						int R{};
						int G{};
						int B{};
						double Y        = static_cast<unsigned char>(not_rescaled_buffer[j]);
						const double Cb = static_cast<unsigned char>(not_rescaled_buffer[j + 1]) - 128;
						const double Cr = static_cast<unsigned char>(not_rescaled_buffer[j + 2]) - 128;
						if (icc_for_ybr == 1)
						{
							R = static_cast<int>(Y + (-0.000036820) * Cb +    1.401987577 * Cr);
							G = static_cast<int>(Y + (-0.344113281) * Cb + (-0.714103821) * Cr);
							B = static_cast<int>(Y +    1.771978117 * Cb + (-0.000134583) * Cr);
						}
						else if (icc_for_ybr == 2)
						{
							Y -= 16;
							if (Y < 0) Y = 0; // invalid?
							R = static_cast<int>(1.164415463 * Y + (-0.000095036) * Cb +    1.596001878 * Cr);
							G = static_cast<int>(1.164415463 * Y + (-0.391724564) * Cb + (-0.813013368) * Cr);
							B = static_cast<int>(1.164415463 * Y +    2.017290682 * Cb + (-0.000135273) * Cr);
						}
						if (R > 255) R = 255;
						if (G > 255) G = 255;
						if (B > 255) B = 255;
						icc_tmp[j    ] = static_cast<char>(R < 0 ? 0 : R);
						icc_tmp[j + 1] = static_cast<char>(G < 0 ? 0 : G);
						icc_tmp[j + 2] = static_cast<char>(B < 0 ? 0 : B);
					}
				}
				cms_error = 0;
				cmsSetLogErrorHandler(AlizaLCMS2LogErrorHandler);
				cmsHPROFILE hInProfile = cmsOpenProfileFromMem(icc_profile, icc_size);
				if (cms_error == 0)
				{
					cmsHPROFILE hOutProfile = cmsCreate_sRGBProfile();
					if (hInProfile && hOutProfile)
					{
						cmsHTRANSFORM hTransform =
								cmsCreateTransform(
									hInProfile,
									((type_size == 1) ? TYPE_RGB_8 : TYPE_RGB_16),
									hOutProfile,
									((type_size == 1) ? TYPE_RGB_8 : TYPE_RGB_16),
									INTENT_PERCEPTUAL,
									0);
						if (hTransform)
						{
							if (cms_error == 0)
							{
								cmsDoTransform(
									hTransform,
									((icc_for_ybr > 0 && type_size == 1) ? icc_tmp : not_rescaled_buffer),
									icc_buffer,
									dimx * dimy * dimz);
								if (cms_error == 0)
								{
									buffer = icc_buffer;
									delete [] not_rescaled_buffer;
									not_rescaled_buffer = nullptr;
									*has_icc = true;
								}
								else
								{
									buffer = not_rescaled_buffer;
									delete [] icc_buffer;
								}
							}
							else
							{
								buffer = not_rescaled_buffer;
								delete [] icc_buffer;
							}
							cmsDeleteTransform(hTransform);
						}
						else
						{
							buffer = not_rescaled_buffer;
							delete [] icc_buffer;
						}
						cmsCloseProfile(hInProfile);
						cmsCloseProfile(hOutProfile);
					}
					else
					{
						buffer = not_rescaled_buffer;
						delete [] icc_buffer;
					}
					buffer_size = image_buffer_length;
				}
				else
				{
					buffer      = not_rescaled_buffer;
					buffer_size = image_buffer_length;
				}
				delete [] icc_tmp;
			}
			else
			{
				buffer      = not_rescaled_buffer;
				buffer_size = image_buffer_length;
			}
		}
		else // should never reach
		{
			delete [] rescaled_buffer;
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Internal error");
		}
	}
	//
	const size_t xy = buffer_size / dimz;
	for (unsigned long long j = 0; j < dimz; ++j)
	{
		bool badalloc{};
		char * p__;
		try
		{
			p__ = new char[xy];
		}
		catch(const std::bad_alloc&)
		{
			badalloc = true;
		}
		if (p__ && !badalloc)
		{
			memcpy(p__, &(buffer[j*xy]), xy);
			data.push_back(p__);
		}
		else
		{
			delete [] not_rescaled_buffer;
			delete [] rescaled_buffer;
			delete [] singlebit_buffer;
			delete [] icc_profile;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Memory allocation error");
		}
	}
	delete [] not_rescaled_buffer;
	delete [] rescaled_buffer;
	delete [] singlebit_buffer;
	delete [] icc_profile;
	if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
	*ok = true;
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_buffer() end");
#endif
	return QString("");
}

// rescale stuff is very difficult
// "apply_rescale" is related to Softcopy Presentation state
// and overrides rescale settings
QString DicomUtils::read_enhanced_common(
	bool * ok,
	std::vector<ImageVariant*> & ivariants,
	const QString & sop,
	const QString & efilename,
	const std::vector<char*> & data,
	const ImageOverlays & image_overlays,
	const unsigned int rows_,
	const unsigned int columns_,
	const mdcm::PixelFormat & pixelformat,
	const mdcm::PhotometricInterpretation & pi,
	std::vector <
		std::map <
			unsigned int,
			unsigned int,
			std::less<unsigned int> > > & tmp0,
	const DimIndexValues & idx_values,
	const FrameGroupValues & values,
	const bool ok3d,
	const bool min_load,
	const QWidget * settings,
	double * dircos_read,
	const int red_subscript,
	const double spacing_x_read,
	const double spacing_y_read,
	const double spacing_z_read,
	const double origin_x_read,
	const double origin_y_read,
	const double origin_z_read,
	const double shift_tmp,
	const double scale_tmp,
	const bool apply_rescale,
	const bool use_icc,
	const float tolerance)
{
#if 0
	std::cout << "common: red_subscript = " << red_subscript << std::endl;
#endif
	QString message;
	bool error{};
	const SettingsWidget * wsettings =
		static_cast<const SettingsWidget *>(settings);
	//
	for (unsigned int x = 0; x < tmp0.size(); ++x)
	{
		if (tmp0.at(x).empty())
		{
			*ok = false;
			return QString("read_enhanced_common error (01)");
		}
		for (std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator
			it = tmp0.at(x).cbegin(); it != tmp0.at(x).cend(); ++it)
		{
			const unsigned int idx__ = it->first;
			if (!(
					idx__ < data.size() && data.at(idx__) &&
					idx__ < values.size()))
			{
				*ok = false;
				return QString("read_enhanced_common error (02)");
			}
		}
	}
	//
	// Both variables just to be sure IPP/IOP and IPV/IOV
	// are not mixed (it is impossible).
	bool ipv_iov_found{};
	bool ipp_iop_found{};
	//
	for (unsigned int x = 0; x < tmp0.size(); ++x)
	{
#ifdef ENHANCED_PRINT_INFO
		if (!min_load) std::cout << "Image: " << x << std::endl;
#endif
		QString message_;
		std::vector<char*>   tmp3;
		std::vector<double*> tmp4;
		QStringList tmp5;
		QList< QPair<double, double> > tmp6;
		bool tmp4_ok{true};
		QStringList window_centers_l;
		QStringList window_widths_l;
		QStringList lut_functions_l;
		QStringList lateralities;
		QStringList body_parts;
		QStringList acquisition_datetimes;
		QStringList reference_datetimes;
		ImageOverlays overlays;
		QList<int> ref_segment_nums;
		unsigned int j{};
#ifdef ENHANCED_PRINT_INFO
		if (!min_load) std::cout << " Indices: ";
#endif
		std::map<
			unsigned int,
			unsigned int,
			std::less<unsigned int> > tmp1;
		for (std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator
			it = tmp0.at(x).cbegin(); it != tmp0.at(x).cend(); ++it)
		{
			tmp1[it->second] = it->first;
		}
		if (tmp0.at(x).size() != tmp1.size())
		{
			error = true;
			tmp4_ok = false;
			*ok = false;
			message_ = QString("tmp0.at(x).size() != tmp1.size(), ") +
				QVariant(static_cast<qulonglong>(tmp0.at(x).size())).toString() +
				QString(" != ") +
				QVariant(static_cast<qulonglong>(tmp1.size())).toString();
#ifdef ENHANCED_PRINT_INFO
			if (!min_load)
			{
				std::cout << message_.toStdString();
			}
#endif
			tmp1.clear();
		}
		for (std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator
			it = tmp1.cbegin(); it != tmp1.cend(); ++it)
		{
			const unsigned int idx__ = it->second;
#ifdef ENHANCED_PRINT_INFO
			if (!min_load)
			{
				std::cout
					<< " " <<  it->first
					<< "[" << idx__ << "]";
			}
#endif
			if (idx__ < data.size() &&
				data.at(idx__) &&
				idx__ < values.size())
			{
				tmp3.push_back(data.at(idx__));
				if (values.at(idx__).vol_pos_ok &&
					values.at(idx__).vol_orient_ok &&
					!ipp_iop_found)
				{
					// IPV/IOV
					double * ss = new double[9];
					ss[0] = values.at(idx__).vol_pos[0];
					ss[1] = values.at(idx__).vol_pos[1];
					ss[2] = values.at(idx__).vol_pos[2];
					ss[3] = values.at(idx__).vol_orient[0];
					ss[4] = values.at(idx__).vol_orient[1];
					ss[5] = values.at(idx__).vol_orient[2];
					ss[6] = values.at(idx__).vol_orient[3];
					ss[7] = values.at(idx__).vol_orient[4];
					ss[8] = values.at(idx__).vol_orient[5];
					tmp4.push_back(ss);
					if (!ipv_iov_found) ipv_iov_found = true;
				}
				else if (!values.at(idx__).pat_pos.isEmpty() &&
					!values.at(idx__).pat_orient.isEmpty() &&
					!ipv_iov_found)
				{
					// IPP/IOP
					double pat_pos[3];
					double pat_orient[6];
					const bool ok_p =
						get_patient_position(
							values.at(idx__).pat_pos,
							pat_pos);
					const bool ok_o =
						get_patient_orientation(
							values.at(idx__).pat_orient,
							pat_orient);
					if (!ipp_iop_found) ipp_iop_found = true;
					if (ok_o && ok_p)
					{
						double * ss = new double[9];
						ss[0] = pat_pos[0];
						ss[1] = pat_pos[1];
						ss[2] = pat_pos[2];
						ss[3] = pat_orient[0];
						ss[4] = pat_orient[1];
						ss[5] = pat_orient[2];
						ss[6] = pat_orient[3];
						ss[7] = pat_orient[4];
						ss[8] = pat_orient[5];
						tmp4.push_back(ss);
					}
					else
					{
						tmp4_ok = false;
					}
				}
				// TODO other e.g. "1.2.840.10008.5.1.4.1.1.77.1.6" VL Whole Slide Microscopy
				else
				{
					tmp4_ok = false;
				}
				QPair<double, double> rp;
				rp.first  = values.at(idx__).rescale_intercept;
				rp.second = values.at(idx__).rescale_slope;
				tmp6.push_back(rp);
				if (!values.at(idx__).window_center.isEmpty() &&
					!values.at(idx__).window_width.isEmpty())
				{
					window_centers_l.push_back(values.at(idx__).window_center);
					window_widths_l.push_back(values.at(idx__).window_width);
					lut_functions_l.push_back(values.at(idx__).lut_function);
				}
				if (!values.at(idx__).pix_spacing.isEmpty())
				{
					tmp5.push_back(values.at(idx__).pix_spacing);
				}
				else
				{
					tmp5.push_back(QString(""));
				}
				lateralities.push_back(values.at(idx__).frame_laterality);
				body_parts.push_back(values.at(idx__).frame_body_part);
				acquisition_datetimes.push_back(
					values.at(idx__).frame_acquisition_datetime.trimmed().remove(QChar('\0')));
				reference_datetimes.push_back(
					values.at(idx__).frame_reference_datetime.trimmed().remove(QChar('\0')));
				if (image_overlays.all_overlays.contains(idx__))
				{
					overlays.all_overlays[it->first] =
						image_overlays.all_overlays.value(idx__);
				}
				if (values.at(idx__).ref_segment_num != -1)
				{
					ref_segment_nums.push_back(values.at(idx__).ref_segment_num);
				}
				++j;
			}
			else
			{
				error = true;
				tmp4_ok = false;
				*ok = false;
				message_ = QString("Error (enhanced IOD): index is invalid");
				break;
			}
		}
#ifdef ENHANCED_PRINT_INFO
		if (!min_load) std::cout << std::endl;
#endif
		if (!error)
		{
			bool   geom_ok{};
			bool   equi_{};
			bool   one_direction_{};
			double origin_x_gen, origin_y_gen, origin_z_gen;
			itk::Matrix<itk::SpacePrecisionType, 3, 3> direction;
			double spacing_x, spacing_y, spacing_z;
			double origin_x, origin_y, origin_z;
			double spacing_z_tmp;
			double dircos_gen[6]{};
			float  slices_dir_x, slices_dir_y, slices_dir_z;
			float  up_dir_x, up_dir_y, up_dir_z;
			float  center_x, center_y, center_z;
			std::vector<ImageSlice*> slices;
			const bool enable_gl = min_load ? false : ok3d;
			// Disable texture for Breast Tomosynthesis
			bool skip_texture =
				(min_load || !enable_gl || (sop == QString("1.2.840.10008.5.1.4.1.1.13.1.3")))
				? true : !wsettings->get_3d();
			const int new_id = min_load ? -1 : CommonUtils::get_next_id();
			double window_center{-999999.0};
			double window_width{-999999.0};
			double window_center_tmp{-999999.0};
			double window_width_tmp{-999999.0};
			short lut_function{1};
			short lut_function_tmp{1};
			QString instance_uid;
			int instance_number{-1};
			int ref_segment_num{-1};
			//
			if (!ref_segment_nums.empty())
			{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				const QSet<int> ref_segment_nums_set =
					QSet<int>(ref_segment_nums.begin(), ref_segment_nums.end());
#else
				const QSet<int> ref_segment_nums_set = ref_segment_nums.toSet();
#endif
				if (ref_segment_nums_set.size() == 1)
				{
					ref_segment_num = ref_segment_nums.at(0);
				}
#ifdef ALIZA_VERBOSE
				else
				{
					std::cout << "Multiple \"Segment Number\" values for the image,\n"
						"check Settings for Enhanced Multi-frame IODs." << std::endl;
				}
#endif
			}
			//
			ImageVariant * ivariant = new ImageVariant(
				new_id,
				enable_gl,
				skip_texture,
				nullptr,
				0);
			ivariant->di->filtering = wsettings->get_filtering();
			//
			{
				mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
				reader.SetFileName(QDir::toNativeSeparators(efilename).toUtf8().constData());
#else
				reader.SetFileName(QDir::toNativeSeparators(efilename).toLocal8Bit().constData());
#endif
#else
				reader.SetFileName(efilename.toLocal8Bit().constData());
#endif
				if (reader.ReadUpToTag(mdcm::Tag(0x7fe0,0x0000)))
				{
					const mdcm::File & rfile = reader.GetFile();
					const mdcm::DataSet & ds = rfile.GetDataSet();
					QString charset;
					if (get_string_value(ds, mdcm::Tag(0x0008,0x0005), charset))
					{
						charset = charset.trimmed().remove(QChar('\0'));
					}
					if (sop == QString("1.2.840.10008.5.1.4.1.1.6.2")) // probably should not be present
					{
						read_us_regions(ds, ivariant);
					}
					if (sop == QString("1.2.840.10008.5.1.4.1.1.4.1") ||
						sop == QString("1.2.840.10008.5.1.4.1.1.4.4"))
					{
						ivariant->iinfo = read_enhmr_spectro_info(ds, false);
					}
					else if (sop == QString("1.2.840.10008.5.1.4.1.1.4.2"))
					{
						ivariant->iinfo = read_enhmr_spectro_info(ds, true);
					}
					else if (sop == QString("1.2.840.10008.5.1.4.1.1.2.1") ||
						sop == QString("1.2.840.10008.5.1.4.1.1.2.2"))
					{
						ivariant->iinfo = read_enhct_info(ds);
					}
					else if (sop == QString("1.2.840.10008.5.1.4.1.1.6.3"))
					{
						ivariant->iinfo = read_PhotoacousticImage(ds);
					}
					read_ivariant_info_tags(ds, ivariant);
					read_window(ds, &window_center_tmp, &window_width_tmp, &lut_function_tmp);
					instance_uid = read_instance_uid(ds);
					instance_number = read_instance_number(ds);
					if (ref_segment_num > -1)
					{
						const mdcm::Tag tSegmentSequence(0x0062,0x0002);
						const mdcm::Tag tSegmentNumber(0x0062,0x0004);
						const mdcm::Tag tSegmentLabel(0x0062,0x0005);
						const mdcm::Tag tRecommendedDisplayCIELabValue(0x0062,0x000d);
						const mdcm::DataElement & deSegmentSequence = ds.GetDataElement(tSegmentSequence);
						if (!deSegmentSequence.IsEmpty())
						{
							mdcm::SmartPointer<mdcm::SequenceOfItems> sqSegmentSequence =
								deSegmentSequence.GetValueAsSQ();
							if (sqSegmentSequence && sqSegmentSequence->GetNumberOfItems() > 0)
							{
								for (unsigned int n = 0;
									n < sqSegmentSequence->GetNumberOfItems();
									++n)
								{
									const mdcm::Item & item = sqSegmentSequence->GetItem(n + 1);
									const mdcm::DataSet & nds = item.GetNestedDataSet();
									unsigned short SegmentNumber;
									if (get_us_value(nds, tSegmentNumber, &SegmentNumber))
									{
										if (SegmentNumber == ref_segment_num)
										{
											ivariant->seg_info.ref_segment_num = ref_segment_num;
											{
												const mdcm::DataElement & deSegmentLabel =
													nds.GetDataElement(tSegmentLabel);
												if (!deSegmentLabel.IsEmpty() &&
													!deSegmentLabel.IsUndefinedLength())
												{
													const mdcm::ByteValue * bvSegmentLabel =
														deSegmentLabel.GetByteValue();
													if (bvSegmentLabel)
													{
														const QByteArray baSegmentLabel(
															bvSegmentLabel->GetPointer(),
															bvSegmentLabel->GetLength());
														QString SegmentLabel = CodecUtils::toUTF8(
															&baSegmentLabel,
															charset.toLatin1().constData());
														if (!SegmentLabel.isEmpty())
														{
															ivariant->seg_info.label =
																SegmentLabel.trimmed().remove(QChar('\0'));
														}
													}
												}
											}
											{
												std::vector<unsigned short> cielab;
												if (get_us_values(
														nds,
														tRecommendedDisplayCIELabValue,
														cielab))
												{
													if (cielab.size() >= 3)
													{
														double L_ = (cielab[0] / 65535.0) * 100.0;
														double a_ = ((cielab[1] - 0x8080) / 65535.0) * 255.0;
														double b_ = ((cielab[2] - 0x8080) / 65535.0) * 255.0;
														double R, G, B;
														ColorSpace_::Lab2Rgb(&R, &G, &B, L_, a_, b_);
														if      (R < 0.0) R = 0.0;
														else if (R > 1.0) R = 1.0;
														if      (G < 0.0) G = 0.0;
														else if (G > 1.0) G = 1.0;
														if      (B < 0.0) B = 0.0;
														else if (B > 1.0) B = 1.0;
														ivariant->seg_info.R = R * 255.0;
														ivariant->seg_info.G = G * 255.0;
														ivariant->seg_info.B = B * 255.0;
													}
												}
											}
											break;
										}
									}
								}
							}
						}
					}
				}
			}
			//
#ifndef ALIZA_LOAD_DCM_THREAD
			QApplication::processEvents();
#endif
			//
			for (int i = 0; i < static_cast<int>(tmp1.size()); ++i)
			{
				AnatomyDesc anatomy_desc;
				anatomy_desc.laterality =
					(!lateralities.isEmpty()) ? lateralities.at(i) : QString("");
				anatomy_desc.body_part =
					(!body_parts.isEmpty()) ? body_parts.at(i) : QString("");
				ivariant->anatomy[i] = std::move(anatomy_desc);
			}
			//
			{
				if (overlays.all_overlays.count() > 0)
				{
					ivariant->image_overlays.all_overlays = overlays.all_overlays; // FIXME
				}
			}
			//
			if (!acquisition_datetimes.empty())
			{
				std::sort(acquisition_datetimes.begin(), acquisition_datetimes.end(), acqtime_less_than);
				QString tmp57 = acquisition_datetimes.at(0);
				tmp57 = tmp57.trimmed().remove(QChar('\0'));
				if (tmp57.size() >= 14)
				{
					ivariant->acquisition_date = tmp57.left(8);
					ivariant->acquisition_time = tmp57.right(tmp57.size() - 8);
				}
			}
			//
			bool invalidate{};
			double spacing_tmp0[2]{};
			double spacing_tmp1[2]{};
			bool spacing_ok{};
			for (int i = 0; i < tmp5.size(); ++i)
			{
				spacing_ok = get_pixel_spacing(tmp5.at(i), spacing_tmp0);
				if (!spacing_ok) break;
				if (i > 0)
				{
					if (!(
						(spacing_tmp0[0] + 0.0001 > spacing_tmp1[0]) &&
						(spacing_tmp0[0] - 0.0001 < spacing_tmp1[0]) &&
						(spacing_tmp0[1] + 0.0001 > spacing_tmp1[1]) &&
						(spacing_tmp0[1] - 0.0001 < spacing_tmp1[1])))
					{
						spacing_ok = false;
						break;
					}
				}
				spacing_tmp1[0] = spacing_tmp0[0];
				spacing_tmp1[1] = spacing_tmp0[1];
			}
			if (spacing_ok)
			{
				spacing_x = spacing_tmp0[1];
				spacing_y = spacing_tmp0[0];
			}
			else
			{
				spacing_x = spacing_x_read;
				spacing_y = spacing_y_read;
			}
			if (spacing_x <= 0 || spacing_y <= 0)
			{
				spacing_x = 1.0;
				invalidate = true;
			}
			if (spacing_y <= 0)
			{
				spacing_y = 1.0;
				invalidate = true;
			}
			//
			if (tmp4_ok)
			{
				geom_ok = generate_geometry(
					slices,
					tmp4,
					rows_, columns_,
					spacing_x, spacing_y, &spacing_z_tmp,
					&equi_, &one_direction_,
					&origin_x_gen, &origin_y_gen, &origin_z_gen,
					dircos_gen,
					&slices_dir_x, &slices_dir_y, &slices_dir_z,
					&up_dir_x, &up_dir_y, &up_dir_z,
					&center_x, &center_y, &center_z,
					tolerance);
			}
			//
			ivariant->equi = equi_;
			//
#if 0
			std::cout
				<< "geom_ok = " << geom_ok
				<< " spacing ok = " << spacing_ok
				<< " sx = " << spacing_x
				<< " sy = " << spacing_y
				<< " sz_tmp = " << spacing_z_tmp
				<< " equi = " << equi_ << std::endl;
#endif
			if (geom_ok)
			{
				for (unsigned int k = 0; k < slices.size(); ++k)
				{
					ivariant->di->image_slices.push_back(slices[k]);
				}
				if (spacing_z_tmp < 0 || (spacing_z_tmp <= 0.00001 && one_direction_))
				{
					spacing_z = 0.00001;
					invalidate = true;
				}
				else
				{
					spacing_z = spacing_z_tmp;
				}
				origin_x = origin_x_gen;
				origin_y = origin_y_gen;
				origin_z = origin_z_gen;
				const float row_dircos_x = static_cast<float>(dircos_gen[0]);
				const float row_dircos_y = static_cast<float>(dircos_gen[1]);
				const float row_dircos_z = static_cast<float>(dircos_gen[2]);
				const float col_dircos_x = static_cast<float>(dircos_gen[3]);
				const float col_dircos_y = static_cast<float>(dircos_gen[4]);
				const float col_dircos_z = static_cast<float>(dircos_gen[5]);
				const float nrm_dircos_x = row_dircos_y*col_dircos_z - row_dircos_z*col_dircos_y;
				const float nrm_dircos_y = row_dircos_z*col_dircos_x - row_dircos_x*col_dircos_z;
				const float nrm_dircos_z = row_dircos_x*col_dircos_y - row_dircos_y*col_dircos_x;
				direction[0][0] = row_dircos_x;
				direction[1][0] = row_dircos_y;
				direction[2][0] = row_dircos_z;
				direction[0][1] = col_dircos_x;
				direction[1][1] = col_dircos_y;
				direction[2][1] = col_dircos_z;
				direction[0][2] = nrm_dircos_x;
				direction[1][2] = nrm_dircos_y;
				direction[2][2] = nrm_dircos_z;
				if (invalidate)
				{
					ivariant->equi = false;
				}
				ivariant->di->slices_generated = true;
				ivariant->one_direction = one_direction_;
				ivariant->di->slices_from_dicom = true;
			}
			else
			{
				spacing_x = spacing_x_read > 0 ? spacing_x_read : 1;
				spacing_y = spacing_y_read > 0 ? spacing_y_read : 1;
				spacing_z = spacing_z_read > 0 ? spacing_z_read : 0.00001;
				origin_x  = origin_x_read;
				origin_y  = origin_y_read;
				origin_z  = origin_z_read;
				const float row_dircos_x = static_cast<float>(dircos_read[0]);
				const float row_dircos_y = static_cast<float>(dircos_read[1]);
				const float row_dircos_z = static_cast<float>(dircos_read[2]);
				const float col_dircos_x = static_cast<float>(dircos_read[3]);
				const float col_dircos_y = static_cast<float>(dircos_read[4]);
				const float col_dircos_z = static_cast<float>(dircos_read[5]);
				const float nrm_dircos_x = row_dircos_y*col_dircos_z - row_dircos_z*col_dircos_y;
				const float nrm_dircos_y = row_dircos_z*col_dircos_x - row_dircos_x*col_dircos_z;
				const float nrm_dircos_z = row_dircos_x*col_dircos_y - row_dircos_y*col_dircos_x;
				direction[0][0] = row_dircos_x;
				direction[1][0] = row_dircos_y;
				direction[2][0] = row_dircos_z;
				direction[0][1] = col_dircos_x;
				direction[1][1] = col_dircos_y;
				direction[2][1] = col_dircos_z;
				direction[0][2] = nrm_dircos_x;
				direction[1][2] = nrm_dircos_y;
				direction[2][2] = nrm_dircos_z;
				ivariant->di->skip_texture = true;
				ivariant->di->slices_from_dicom = false;
			}
			for (unsigned int k = 0; k < tmp4.size(); ++k)
			{
				delete [] tmp4[k];
			}
			tmp4.clear();
			//
			const size_t tmp1s = window_centers_l.size();
			const bool tmp1ok =
				(tmp1s > 0) &&
				(static_cast<size_t>(window_widths_l.size()) == tmp1s) &&
				(static_cast<size_t>(lut_functions_l.size()) == tmp1s);
			if (tmp1ok)
			{
				QList<double> tmp1w;
				QList<double> tmp1c;
				QList<short>  tmp1l;
				for (size_t k = 0; k < tmp1s; ++k)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					QStringList w__ = window_widths_l.at(k).split(QString("\\"), Qt::SkipEmptyParts);
					QStringList c__ = window_centers_l.at(k).split(QString("\\"), Qt::SkipEmptyParts);
#else
					QStringList w__ = window_widths_l.at(k).split(QString("\\"), QString::SkipEmptyParts);
					QStringList c__ = window_centers_l.at(k).split(QString("\\"), QString::SkipEmptyParts);
#endif
					if (!w__.empty() && !c__.empty())
					{
						bool ok_c{};
						bool ok_w{};
						const double tmp_w = QVariant(w__.at(0).trimmed().remove(QChar('\0'))).toDouble(&ok_w);
						const double tmp_c = QVariant(c__.at(0).trimmed().remove(QChar('\0'))).toDouble(&ok_c);
						if (ok_c && ok_w)
						{
							double tmp7890 = tmp_w;
							short  tmp7891{1};
							const QString tmp7892 = lut_functions_l.at(k).trimmed().toUpper();
							if (tmp7892 == QString("SIGMOID"))
							{
								tmp7891 = 2;
							}
							else if (tmp7892 == QString("LINEAR_EXACT"))
							{
								tmp7891 = 0;
							}
							tmp1c.push_back(tmp_c);
							tmp1w.push_back(tmp7890);
							tmp1l.push_back(tmp7891);
						}
					}
				}
				//
				const int tmp1c_size = tmp1c.size();
				if (tmp1c_size > 0)
				{
					bool tmp5468ok{true};
					for (int k = 0; k < tmp1c_size; ++k)
					{
						FrameLevel fl;
						fl.lut_function = tmp1l.at(k);
						fl.us_window_center = tmp1c.at(k);
						fl.us_window_width = tmp1w.at(k);
						ivariant->frame_levels[k] = fl;
						//
						if (k > 0)
						{
							if (!(
								MMath::AlmostEqual(tmp1c.at(k), tmp1c.at(k - 1)) &&
								MMath::AlmostEqual(tmp1w.at(k), tmp1w.at(k - 1))))
							{
								tmp5468ok = false;
							}
						}
					}
					if (tmp5468ok)
					{
						window_center = tmp1c.at(0);
						window_width  = tmp1w.at(0);
						lut_function  = tmp1l.at(0);
					}
				}
			}
			else
			{
				window_center = window_center_tmp;
				window_width  = window_width_tmp;
				lut_function  = lut_function_tmp;
			}
			ivariant->di->default_us_window_center =
				ivariant->di->us_window_center = window_center;
			ivariant->di->default_us_window_width =
				ivariant->di->us_window_width  = window_width;
			ivariant->di->default_lut_function =
				ivariant->di->lut_function = lut_function;
			ivariant->di->supp_palette_subsciptor = red_subscript;
			const bool no_warn_rescale =
				(apply_rescale)
				? wsettings->get_rescale()
				: true;
			{
				// MDCM can not read rescale from per frame groups
				const bool rescale_tmp =
					(!apply_rescale || pixelformat.GetSamplesPerPixel() > 1)
					? false : wsettings->get_rescale();
				const double saved_window_center = ivariant->di->default_us_window_center;
				const double saved_window_width = ivariant->di->default_us_window_width;
				const short saved_lut_function = ivariant->di->default_lut_function;
				if (rescale_tmp)
				{
#ifdef ALIZA_LINUX_DEBUG_MEM
					CommonUtils::linux_print_memusage("before gen_itk_image()");
#endif
					message_ = CommonUtils::gen_itk_image(ok,
						tmp3,
						pixelformat,
						pi,
						ivariant,
						direction,
						columns_,
						rows_,
						tmp1.size(),
						origin_x,
						origin_y,
						origin_z,
						spacing_x,
						spacing_y,
						spacing_z,
						!geom_ok,
						false,
						wsettings->get_resize(),
						wsettings->get_size_x(),
						wsettings->get_size_y(),
						no_warn_rescale,
						use_icc,
						false);
#ifdef ALIZA_LINUX_DEBUG_MEM
					CommonUtils::linux_print_memusage("after gen_itk_image()");
#endif
					if (*ok)
					{
						// check if not only 1 and 0
						bool really_rescale{};
						for (int u = 0; u < tmp6.size(); ++u)
						{
							if (!(
								tmp6.at(u).first > -0.000001 &&
								tmp6.at(u).first <  0.000001 &&
								tmp6.at(u).second > 0.999999 &&
								tmp6.at(u).second < 1.000001))
							{
								really_rescale = true;
								break;
							}
						}
						if (really_rescale)
						{
							message = CommonUtils::apply_per_slice_rescale(ivariant, tmp6);
						}
						ivariant->di->default_us_window_center = ivariant->di->us_window_center = saved_window_center;
						ivariant->di->default_us_window_width = ivariant->di->us_window_width = saved_window_width;
						ivariant->di->default_lut_function = ivariant->di->lut_function = saved_lut_function;
						const bool ok_ =
							CommonUtils::reload_monochrome(
								ivariant,
								enable_gl,
								nullptr,
								0,
								wsettings->get_resize(),
								wsettings->get_size_x(),
								wsettings->get_size_y());
						if (!ok_) *ok = false;
					}
				}
				else
				{
#ifdef ALIZA_LINUX_DEBUG_MEM
					CommonUtils::linux_print_memusage("before gen_itk_image()");
#endif
					message_ = CommonUtils::gen_itk_image(ok,
						tmp3,
						pixelformat,
						pi,
						ivariant,
						direction,
						columns_,
						rows_,
						tmp1.size(),
						origin_x,
						origin_y,
						origin_z,
						spacing_x,
						spacing_y,
						spacing_z,
						!geom_ok,
						false,
						wsettings->get_resize(),
						wsettings->get_size_x(),
						wsettings->get_size_y(),
						no_warn_rescale,
						use_icc,
						false);
#ifdef ALIZA_LINUX_DEBUG_MEM
					CommonUtils::linux_print_memusage("after gen_itk_image()");
#endif
				}
			}
			if (*ok)
			{
				if (geom_ok)
				{
					// one inst. UID for all slices
					for (int z = 0; z < ivariant->di->idimz; ++z)
					{
						ivariant->image_instance_uids[z] = instance_uid;
					}
					//
					ivariant->di->slices_direction_x = slices_dir_x;
					ivariant->di->slices_direction_y = slices_dir_y;
					ivariant->di->slices_direction_z = slices_dir_z;
					ivariant->di->up_direction_x = up_dir_x;
					ivariant->di->up_direction_y = up_dir_y;
					ivariant->di->up_direction_z = up_dir_z;
					if (!ivariant->equi)
					{
						float cx{};
						float cy{};
						float cz{};
						CommonUtils::calculate_center_notuniform(ivariant->di->image_slices, &cx, &cy, &cz);
						ivariant->di->default_center_x = ivariant->di->center_x = cx;
						ivariant->di->default_center_y = ivariant->di->center_y = cy;
						ivariant->di->default_center_z = ivariant->di->center_z = cz;
					}
					else
					{
						ivariant->di->default_center_x = ivariant->di->center_x = center_x;
						ivariant->di->default_center_y = ivariant->di->center_y = center_y;
						ivariant->di->default_center_z = ivariant->di->center_z = center_z;
					}
				}
				if (ivariant->equi)
				{
					if (ivariant->di->idimz < 7)
					{
						ivariant->di->transparency = false;
					}
				}
				else
				{
					if (!ivariant->one_direction)
					{
						ivariant->di->transparency = false;
					}
					ivariant->di->filtering = 0;
				}
				if (sop == QString("1.2.840.10008.5.1.4.1.1.4.1") || // Enhanced MR
					sop == QString("1.2.840.10008.5.1.4.1.1.2.1") || // Enhanced CT
					sop == QString("1.2.840.10008.5.1.4.1.1.130") || // Enhanced PET
					sop == QString("1.2.840.10008.5.1.4.1.1.2.2") || // Legacy CT
					sop == QString("1.2.840.10008.5.1.4.1.1.4.4") || // Legacy MR
					sop == QString("1.2.840.10008.5.1.4.1.1.4.3") || // Enhanced MR color
					sop == QString("1.2.840.10008.5.1.4.1.1.128.1")) // Legacy PET
				{
					ivariant->unit_str = QString(" mm");
				}
				ivariant->di->shift_tmp = shift_tmp;
				ivariant->di->scale_tmp = scale_tmp;
				if (geom_ok)
				{
					ivariant->iod_supported = true;
				}
				if (instance_number >= 0)
				{
					ivariant->instance_number = instance_number;
				}
				CommonUtils::reset_bb(ivariant);
				ivariant->filenames = QStringList(efilename);
				ivariants.push_back(ivariant);
			}
			else
			{
				delete ivariant;
			}
		}
		else
		{
			for (unsigned int k = 0; k < tmp4.size(); ++k)
			{
				delete [] tmp4[k];
			}
			tmp4.clear();
		}
		if (!message_.isEmpty()) message.append(QChar('\n') + message_);
#ifdef ENHANCED_PRINT_INFO
		if (!min_load)
		{
			std::cout << "*********" << std::endl;
		}
#endif
	}
#ifdef ENHANCED_PRINT_INFO
	if (ipp_iop_found && ipv_iov_found)
	{
		std::cout << "Error: found both, IPP/IOP and IPV/IOV" << std::endl;
	}
#endif
	return message;
}

void DicomUtils::enhanced_process_indices(
	std::vector< std::map< unsigned int, unsigned int, std::less<unsigned int> > > & tmp0,
	const DimIndexValues & idx_values, const FrameGroupValues & values,
	const int dim8th, const int dim7th, const int dim6th, const int dim5th, const int dim4th, const int dim3rd,
	const short enh_loading_type)
{
	bool error{};
	std::list<unsigned int> tmp1_1;
	std::list<unsigned int> tmp1_2;
	std::list<unsigned int> tmp1_3;
	std::list<unsigned int> tmp1_4;
	std::list<unsigned int> tmp1_5;
	const EnhancedIODLoadingType loading_type = static_cast<EnhancedIODLoadingType>(enh_loading_type);
	const size_t idx_values_size = idx_values.size();
	std::vector< std::map< unsigned int, unsigned int, std::less<unsigned int> > > tmp0single;
	//
	if (dim8th >= 0)
	{
		for (unsigned int x = 0; x < idx_values_size; ++x)
		{
			tmp1_1.push_back(idx_values.at(x).idx.at(dim8th));
		}
		tmp1_1.sort();
		tmp1_1.unique();
	}
	else
	{
		tmp1_1.push_back(1);
	}
	if (dim7th >= 0)
	{
		for (unsigned int x = 0; x < idx_values_size; ++x)
		{
			tmp1_2.push_back(idx_values.at(x).idx.at(dim7th));
		}
		tmp1_2.sort();
		tmp1_2.unique();
	}
	else
	{
		tmp1_2.push_back(1);
	}
	if (dim6th >= 0)
	{
		for (unsigned int x = 0; x < idx_values_size; ++x)
		{
			tmp1_3.push_back(idx_values.at(x).idx.at(dim6th));
		}
		tmp1_3.sort();
		tmp1_3.unique();
	}
	else
	{
		tmp1_3.push_back(1);
	}
	if (dim5th >= 0)
	{
		for (unsigned int x = 0; x < idx_values_size; ++x)
		{
			tmp1_4.push_back(idx_values.at(x).idx.at(dim5th));
		}
		tmp1_4.sort();
		tmp1_4.unique();
	}
	else
	{
		tmp1_4.push_back(1);
	}
	if (dim4th >= 0)
	{
		for (unsigned int x = 0; x < idx_values_size; ++x)
		{
			tmp1_5.push_back(idx_values.at(x).idx.at(dim4th));
		}
		tmp1_5.sort();
		tmp1_5.unique();
	}
	else
	{
		tmp1_5.push_back(1);
	}
	//
	if (idx_values_size < 1 && (
		loading_type == EnhancedIODLoadingType::PreferUniformVolumes ||
		loading_type == EnhancedIODLoadingType::StrictMultipleImages ||
		loading_type == EnhancedIODLoadingType::StrictSingleImage))
	{
		error = true;
#ifdef ALIZA_VERBOSE
		std::cout << "Cannot process Dimension Organization: no indices" << std::endl;
#endif
	}
	if (!error && (
		loading_type == EnhancedIODLoadingType::PreferUniformVolumes ||
		loading_type == EnhancedIODLoadingType::StrictMultipleImages ||
		loading_type == EnhancedIODLoadingType::StrictSingleImage))
	{
#ifdef ENHANCED_PRINT_INFO
		bool warning0 = false;
		bool info0 = false;
#endif
		for (std::list<unsigned int>::const_iterator it1 = tmp1_1.cbegin();
			it1 != tmp1_1.cend();
			++it1)
		{
			for (std::list<unsigned int>::const_iterator it2 = tmp1_2.cbegin();
					it2 != tmp1_2.cend();
					++it2)
			{
				for (std::list<unsigned int>::const_iterator it3 = tmp1_3.cbegin();
						it3 != tmp1_3.cend();
						++it3)
				{
					for (std::list<unsigned int>::const_iterator it4 = tmp1_4.cbegin();
							it4 != tmp1_4.cend();
							++it4)
					{
						for (std::list<unsigned int>::const_iterator it5 = tmp1_5.cbegin();
								it5 != tmp1_5.cend();
								++it5)
						{
							std::map< unsigned int, unsigned int, std::less<unsigned int> > tmp2;
							std::list<unsigned int> tmp2_test;
							for (unsigned int x = 0; x < idx_values_size; ++x)
							{
								const int idx1 =
									(dim8th >= 0 && dim8th < static_cast<int>(idx_values.at(x).idx.size()))
									? static_cast<int>(idx_values.at(x).idx.at(dim8th))
									: -1;
								const int idx2 =
									(dim7th >= 0 && dim7th < static_cast<int>(idx_values.at(x).idx.size()))
									? static_cast<int>(idx_values.at(x).idx.at(dim7th))
									: -1;
								const int idx3 =
									(dim6th >= 0 && dim6th < static_cast<int>(idx_values.at(x).idx.size()))
									? static_cast<int>(idx_values.at(x).idx.at(dim6th))
									: -1;
								const int idx4 =
									(dim5th >= 0 && dim5th < static_cast<int>(idx_values.at(x).idx.size()))
									? static_cast<int>(idx_values.at(x).idx.at(dim5th))
									: -1;
								const int idx5 =
									(dim4th >= 0 && dim4th < static_cast<int>(idx_values.at(x).idx.size()))
									? static_cast<int>(idx_values.at(x).idx.at(dim4th))
									: -1;
								const int idx6 =
									(dim3rd >= 0 && dim3rd < static_cast<int>(idx_values.at(x).idx.size()))
									? static_cast<int>(idx_values.at(x).idx.at(dim3rd))
									: -1;
								if ((idx1 < 0 || idx1 == static_cast<int>(*it1)) &&
									(idx2 < 0 || idx2 == static_cast<int>(*it2)) &&
									(idx3 < 0 || idx3 == static_cast<int>(*it3)) &&
									(idx4 < 0 || idx4 == static_cast<int>(*it4)) &&
									(idx5 < 0 || idx5 == static_cast<int>(*it5)))
								{
									if (idx6 < 0)
									{
										tmp2[idx_values.at(x).id] = idx_values.at(x).id;
										tmp2_test.push_back(idx_values.at(x).id);
									}
									else
									{
										unsigned int tmp2_pos = idx_values.at(x).idx.at(dim3rd);
										// Indices must start with "1", but there are files in the wild
										// with indices starting with "0".
										if (tmp2_pos >= 1)
										{
											tmp2_pos -= 1;
										}
										else
										{
#ifdef ALIZA_VERBOSE
											std::cout <<
												"Error: indices can not start with \"0\","
												"fallback to skip Dimension Organization" << std::endl;
#endif
											error = true;
											break;
										}
										tmp2[idx_values.at(x).id] = tmp2_pos;
										tmp2_test.push_back(tmp2_pos);
									}
								}
							}
							if (!tmp2.empty())
							{
								const size_t tmp2_test_size0 = tmp2_test.size();
								if (tmp2_test_size0 > 0)
								{
									tmp2_test.sort();
									tmp2_test.unique();
									const size_t tmp2_test_size1 = tmp2_test.size();
									if (tmp2_test_size0 != tmp2_test_size1) error = true;
									if (error) break;
								}
								if (loading_type == EnhancedIODLoadingType::PreferUniformVolumes)
								{
									std::map< unsigned int, unsigned int, std::less<unsigned int> > tmp3;
									const bool ok_sort = sort_frames_ippiop(tmp2, tmp3, values);
									if (ok_sort)
									{
										tmp0.push_back(std::move(tmp3));
									}
									else
									{
										tmp0.push_back(std::move(tmp2));
									}
#ifdef ENHANCED_PRINT_INFO
									if (!info0)
									{
										info0 = true;
										std::cout << "Trying to sort by IPP/OIP ...";
										if (ok_sort)
										{
											std::cout << "success";
										}
										else
										{
											std::cout << "failed";
										}
										std::cout << std::endl;
									}
#endif
								}
								else if (loading_type == EnhancedIODLoadingType::StrictMultipleImages)
								{
									tmp0.push_back(std::move(tmp2));
								}
								else if (loading_type == EnhancedIODLoadingType::StrictSingleImage)
								{
									tmp0single.push_back(std::move(tmp2));
								}
							}
#ifdef ENHANCED_PRINT_INFO
							else
							{
								if (!warning0)
								{
									warning0 = true;
									std::cout << "Warning: indices may be not consistent" << std::endl;
								}
							}
#endif
						}
						if (error) break;
					}
					if (error) break;
				}
				if (error) break;
			}
			if (error) break;
		}
		//
		if (!error && loading_type == EnhancedIODLoadingType::StrictSingleImage)
		{
			unsigned int j{};
			std::map< unsigned int, unsigned int, std::less<unsigned int> > tmp;
			for (size_t x = 0; x < tmp0single.size(); ++x)
			{
				std::map< unsigned int, unsigned int, std::less<unsigned int> >::const_iterator it =
					tmp0single.at(x).cbegin();
				while (it != tmp0single.at(x).cend())
				{
					tmp[it->first] = j;
					++j;
					++it;
				}
			}
			tmp0.push_back(std::move(tmp));
		}
	}
	//
	if (error)
	{
		tmp0.clear();
	}
	//
	if (loading_type == EnhancedIODLoadingType::SkipDimensionOrganization ||
		loading_type == EnhancedIODLoadingType::NotDefined ||
		error)
	{
		std::map< unsigned int, unsigned int, std::less<unsigned int> > tmp2;
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			tmp2[x] = x;
		}
		if (!(
			loading_type == EnhancedIODLoadingType::SkipDimensionOrganization ||
			loading_type == EnhancedIODLoadingType::NotDefined))
		{
			std::map< unsigned int, unsigned int, std::less<unsigned int> > tmp3;
			const bool ok_sort = sort_frames_ippiop(tmp2, tmp3, values);
			if (ok_sort)
			{
				tmp0.push_back(std::move(tmp3));
			}
			else
			{
				tmp0.push_back(std::move(tmp2));
			}
		}
		else
		{
			tmp0.push_back(std::move(tmp2));
		}
	}
#ifdef ENHANCED_PRINT_INFO
#if 0
	{
		for (int x = 0; x < tmp0.size(); ++x)
		{
			std::map< unsigned int, unsigned int, std::less<unsigned int> >::const_iterator it =
				tmp0.at(x).cbegin();
			while (it != tmp0.at(x).cend())
			{
				std::cout << "x = " << x << " [" << it->first << "]=" << it->second << '\n';
				++it;
			}
		}
	}
#endif
#endif
}

QString DicomUtils::read_enhanced_3d_8d(
	bool * ok,
	std::vector<ImageVariant*> & ivariants,
	const QString & sop,
	const QString & efilename,
	const std::vector<char*> & data,
	const ImageOverlays & image_overlays,
	const unsigned int rows_, const unsigned int columns_,
	const mdcm::PixelFormat & pixelformat,
	const mdcm::PhotometricInterpretation & pi,
	const int dim8th, const int dim7th, const int dim6th, const int dim5th, const int dim4th, const int dim3rd,
	const DimIndexValues & idx_values, const FrameGroupValues & values,
	const bool ok3d,
	const bool min_load,
	const QWidget * settings,
	double * dircos_read,
	const int red_subscript,
	const double spacing_x_read, const double spacing_y_read, const double spacing_z_read,
	const double origin_x_read, const double origin_y_read, const double origin_z_read,
	const double shift_tmp, const double scale_tmp,
	const bool apply_rescale,
	const bool use_icc,
	const float tolerance,
	const short enh_loading_type)
{
	QString message_ ;
	std::vector< std::map< unsigned int, unsigned int, std::less<unsigned int> > > tmp0;
	enhanced_process_indices(
		tmp0, idx_values, values,
		dim8th, dim7th, dim6th, dim5th, dim4th, dim3rd, enh_loading_type);
	message_ = read_enhanced_common(
		ok,
		ivariants,
		sop,
		efilename,
		data,
		image_overlays,
		rows_, columns_,
		pixelformat, pi,
		tmp0,
		idx_values,  values,
		ok3d,
		min_load,
		settings,
		dircos_read,
		red_subscript,
		spacing_x_read, spacing_y_read, spacing_z_read,
		origin_x_read, origin_y_read, origin_z_read,
		shift_tmp, scale_tmp,
		apply_rescale,
		use_icc,
		tolerance);
	return message_;
}

bool DicomUtils::is_not_interleaved(const QStringList & images)
{
	mdcm::Tag tSlicePosition(0x0020,0x1041);
	std::set<mdcm::Tag> tags;
	tags.insert(tSlicePosition);
	long long tmp0{};
	for (int x = 0; x < images.size(); ++x)
	{
		mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		reader.SetFileName(QDir::toNativeSeparators(images.at(x)).toUtf8().constData());
#else
		reader.SetFileName(QDir::toNativeSeparators(images.at(x)).toLocal8Bit().constData());
#endif
#else
		reader.SetFileName(images.at(x).toLocal8Bit().constData());
#endif
		const bool f_ok = reader.ReadSelectedTags(tags);
		if (!f_ok) return false;
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		const mdcm::File & file = reader.GetFile();
		const mdcm::DataSet & ds = file.GetDataSet();
		if (ds.IsEmpty()) return false;
		const mdcm::DataElement & sp_ = ds.GetDataElement(tSlicePosition);
		if (!sp_.IsEmpty() && !sp_.IsUndefinedLength() && sp_.GetByteValue())
		{
			bool sp_ok{};
			long long tmp1{};
			QString sp = QString::fromLatin1(
				sp_.GetByteValue()->GetPointer(), sp_.GetByteValue()->GetLength());
			if (sp.contains(QString(",")))
			{
				// Workaround invalid VR
				sp.replace(QString(","), QString("."));
			}
			const double spvd = QVariant(sp.trimmed().remove(QChar('\0'))).toDouble(&sp_ok);
			if (sp_ok) tmp1 = 1000 * CommonUtils::set_digits(spvd, 3);
			else return false;
			if (x > 0 && tmp0 > tmp1 - 10 && tmp0 < tmp1 + 10) return false;
			tmp0 = tmp1;
		}
	}
	return true;
}

bool DicomUtils::is_mosaic(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return false;
	const mdcm::Tag tImageType(0x0008,0x0008);
	const mdcm::DataElement & de = ds.GetDataElement(tImageType);
	if (!de.IsEmpty() && !de.IsUndefinedLength() && de.GetByteValue())
	{
		const QString s = QString::fromLatin1(
			de.GetByteValue()->GetPointer(),
			de.GetByteValue()->GetLength()).trimmed().toUpper();
		if (s.contains(QString("MOSAIC"))) return true;
	}
	return false;
}

bool DicomUtils::is_uih_grid(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return false;
	const mdcm::PrivateTag tMRVFrameSequence(0x0065,0x51,"Image Private Header");
	const mdcm::DataElement & e	= ds.GetDataElement(tMRVFrameSequence);
	if (!e.IsEmpty())
	{
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems() > 0) return true;
	}
	else
	{
		const mdcm::DataElement & e1 = ds.GetDataElement(mdcm::Tag(0x0065,0x1051));
		if (!e1.IsEmpty())
		{
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e1.GetValueAsSQ();
			if (sq && sq->GetNumberOfItems() > 0) return true;
		}
	}
	return false;
}

bool DicomUtils::is_elscint(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return false;
	const mdcm::PrivateTag tTamarCompressionType(0x07a1,0x11,"ELSCINT1");
	QString s;
	if (priv_get_string_value(ds, tTamarCompressionType, s))
	{
		if (s.toUpper() == QString("PMSCT_RLE1") || s.toUpper() == QString("PMSCT_RGB1"))
		{
			return true;
		}
		else if (s.toUpper() == QString("LOSSLESS RICE"))
		{
#ifdef ALIZA_VERBOSE
			std::cout << "LOSSLESS RICE is currentry not supported" << std::endl;
#endif
			return false;
		}
	}
	return false;
}

void DicomUtils::write_encapsulated(
	const QString & in_f,
	const QString & out_f)
{
	mdcm::Tag t(0x0042,0x0011);
	std::set<mdcm::Tag> tags;
	tags.insert(t);
	mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	reader.SetFileName(QDir::toNativeSeparators(in_f).toUtf8().constData());
#else
	reader.SetFileName(QDir::toNativeSeparators(in_f).toLocal8Bit().constData());
#endif
#else
	reader.SetFileName(in_f.toLocal8Bit().constData());
#endif
	const bool ok = reader.ReadSelectedTags(tags);
	if (!ok) return;
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty() || e.IsUndefinedLength()) return;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (bv && bv->GetPointer() && (bv->GetLength() > 0))
	{
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		const std::wstring uncpath =
			mdcm::System::ConvertToUtf16((QDir::toNativeSeparators(out_f)).toUtf8().constData());
		std::ofstream o(uncpath.c_str(), std::ios::binary);
#else
		std::ofstream o((QDir::toNativeSeparators(out_f)).toLocal8Bit().constData(), std::ios::binary);
#endif
#else
		std::ofstream o(out_f.toLocal8Bit().constData(), std::ios::binary);
#endif
		o.write(bv->GetPointer(), bv->GetLength());
		o.close();
	}
}

QString DicomUtils::suffix_mpeg(const QString & f)
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
	if (reader.ReadUpToTag(mdcm::Tag(0x0008,0x0016)))
	{
		mdcm::FileMetaInformation & h = reader.GetFile().GetHeader();
		const mdcm::TransferSyntax & ts = h.GetDataSetTransferSyntax();
		if (ts == mdcm::TransferSyntax::MPEG2MainProfileHighLevel ||
			ts == mdcm::TransferSyntax::MPEG2MainProfile)
		{
			return QString(".mpeg");
		}
		else if (
			ts == mdcm::TransferSyntax::MPEG4AVCH264HighProfileLevel4_1 ||
			ts == mdcm::TransferSyntax::MPEG4AVCH264BDcompatibleHighProfileLevel4_1)
		{
			return QString(".mp4");
		}
	}
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	return QString(".bin");
}

void DicomUtils::write_mpeg(
	const QString & in_f,
	const QString & out_f)
{
	mdcm::Tag t(0x7fe0,0x0010);
	std::set<mdcm::Tag> tags;
	tags.insert(t);
	mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	reader.SetFileName(QDir::toNativeSeparators(in_f).toUtf8().constData());
#else
	reader.SetFileName(QDir::toNativeSeparators(in_f).toLocal8Bit().constData());
#endif
#else
	reader.SetFileName(in_f.toLocal8Bit().constData());
#endif
	const bool ok = reader.ReadSelectedTags(tags);
	if (!ok) return;
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty()) return;
	const mdcm::SequenceOfFragments * sf = e.GetSequenceOfFragments();
	if (!sf) return;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	const std::wstring uncpath =
		mdcm::System::ConvertToUtf16((QDir::toNativeSeparators(out_f)).toUtf8().constData());
	std::ofstream o(uncpath.c_str(), std::ios::binary);
#else
	std::ofstream o((QDir::toNativeSeparators(out_f)).toLocal8Bit().constData(), std::ios::binary);
#endif
#else
	std::ofstream o(out_f.toLocal8Bit().constData(), std::ios::binary);
#endif
	sf->WriteBuffer(o);
	o.close();
}

bool DicomUtils::is_dicom_file(const QString & f)
{
	bool dicom{};
	char b[4]{};
	std::ifstream fs;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	const std::wstring uncpath =
		mdcm::System::ConvertToUtf16((QDir::toNativeSeparators(f)).toUtf8().constData());
	fs.open(uncpath.c_str(), std::ios::in | std::ios::binary);
#else
	fs.open((QDir::toNativeSeparators(f)).toLocal8Bit().constData(), std::ios::in | std::ios::binary);
#endif
#else
	fs.open(f.toLocal8Bit().constData(), std::ios::in | std::ios::binary);
#endif
	for (std::streamoff off = 128; off >= 0; off -= 128)
	{
		fs.seekg(off, std::ios_base::beg);
		if (!fs.fail() && !fs.eof())
		{
			fs.read(b, 4);
			if (!fs.fail())
			{
				if (b[0] == 'D' && b[1] == 'I' && b[2] == 'C' && b[3] == 'M')
				{
					dicom = true;
					break;
				}
			}
		}
	}
	if (!dicom)
	{
		unsigned short group_no{};
		fs.seekg(0, std::ios_base::beg);
		if (!fs.fail() && !fs.eof())
		{
			fs.read(reinterpret_cast<char *>(&group_no), sizeof(unsigned short));
			itk::ByteSwapper<unsigned short>::SwapFromSystemToLittleEndian(&group_no);
			// 0x0003 and 0x0005 are illegal, but files exist
			if (group_no == 0x0002 || group_no == 0x0003 || group_no == 0x0005 || group_no == 0x0008)
			{
				dicom = true;
			}
		}
	}
	fs.close();
	return dicom;
}

void DicomUtils::scan_dir_for_rtstruct_image(
	const QString & p, QList<QStringList> & ref_files)
{
	if (p.isEmpty()) return;
	scan_files_for_rtstruct_image(p, ref_files);
	QDirIterator it(p, QDir::Dirs|QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		scan_files_for_rtstruct_image(it.next(), ref_files);
	}
}

void DicomUtils::scan_files_for_rtstruct_image(
	const QString & p, QList<QStringList> & ref_files)
{
	if (p.isEmpty()) return;
	QDir dir(p);
	QStringList flist = dir.entryList(QDir::Files | QDir::Readable, QDir::Name);
	std::vector<std::string> filenames;
	for (int x = 0; x < flist.size(); ++x)
	{
		const QString tmp0 = dir.absolutePath() + QString("/") + flist.at(x);
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		filenames.push_back(std::string((QDir::toNativeSeparators(tmp0)).toUtf8().constData()));
#else
		filenames.push_back(std::string((QDir::toNativeSeparators(tmp0)).toLocal8Bit().constData()));
#endif
#else
		filenames.push_back(std::string(tmp0.toLocal8Bit().constData()));
#endif
	}
	flist.clear();
	//
	const mdcm::Global & g = mdcm::GlobalInstance;
	const mdcm::Dicts & dicts = g.GetDicts();
	const mdcm::Dict & dict = dicts.GetPublicDict();
	std::vector<std::string> t0_files;
	{
		const mdcm::Tag t0(0x0020,0x0052);
		mdcm::Scanner s0;
		s0.AddTag(t0);
		s0.Scan(filenames, dict);
		mdcm::Scanner::ValuesType v0 = s0.GetValues();
		for (mdcm::Scanner::ValuesType::iterator vi0 = v0.begin(); vi0 != v0.end(); ++vi0)
		{
			std::vector<std::string> files__ = s0.GetAllFilenamesFromTagToValue(t0, (*vi0).c_str());
			for (unsigned int j = 0; j < files__.size(); ++j)
			{
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
				t0_files.push_back(std::string(
					QDir::toNativeSeparators(QString::fromUtf8(files__.at(j).c_str())).toUtf8().constData()));
#else
				t0_files.push_back(std::string(
					QDir::toNativeSeparators(QString::fromLocal8Bit(files__.at(j).c_str())).toLocal8Bit().constData()));
#endif
#else
				t0_files.push_back(std::string(QString::fromLocal8Bit(files__.at(j).c_str()).toLocal8Bit().constData()));
#endif
			}
		}
	}
	//
	{
		mdcm::Scanner s1;
		const mdcm::Tag t1(0x0020,0x000e);
		s1.AddTag(t1);
		s1.Scan(t0_files, dict);
		mdcm::Scanner::ValuesType v1 = s1.GetValues();
		for (mdcm::Scanner::ValuesType::iterator vi1 = v1.begin(); vi1 != v1.end(); ++vi1)
		{
			std::vector<std::string> files__ = s1.GetAllFilenamesFromTagToValue(t1, (*vi1).c_str());
			QStringList t1_tmp;
			for (unsigned int j = 0; j < files__.size(); ++j)
			{
				t1_tmp.push_back(
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
					QDir::toNativeSeparators(QString::fromUtf8(files__.at(j).c_str()))
#else
					QDir::toNativeSeparators(QString::fromLocal8Bit(files__.at(j).c_str()))
#endif
#else
					QString::fromLocal8Bit(files__.at(j).c_str())
#endif
				);
			}
			ref_files.push_back(t1_tmp);
		}
	}
}

bool DicomUtils::process_contrours_ref(
	const QString & f,
	const QString & path,
	std::vector<ImageVariant*> & tmp_ivariants,
	bool ok3d,
	short enh_loading_type,
	const QWidget * settings)
{
	unsigned short count_{};
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
	if (!reader.Read()) return false;
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	if (ds.IsEmpty()) return false;
	ImageVariant * tmp_ivariant = new ImageVariant(-1, false, false, nullptr, 0);
	load_contour(ds, tmp_ivariant);
	QSet<QString> ref_frame_of_refs_set;
	for (int z = 0; z < tmp_ivariant->di->rois.size(); ++z)
	{
		ref_frame_of_refs_set << tmp_ivariant->di->rois.at(z).ref_frame_of_ref;
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	QList<QString> ref_frame_of_refs_list =
		(!ref_frame_of_refs_set.empty())
		?
		ref_frame_of_refs_set.values()
		:
		QList<QString>();
#else
	QList<QString> ref_frame_of_refs_list = ref_frame_of_refs_set.toList();
#endif
	QStringList ref_frame_of_refs;
	for (int z = 0; z < ref_frame_of_refs_list.size(); ++z)
	{
		ref_frame_of_refs.push_back(ref_frame_of_refs_list.at(z));
	}
	ref_frame_of_refs_set.clear();
	ref_frame_of_refs_list.clear();
	QList<QStringList> detected_files;
	for (int z = 0; z < ref_frame_of_refs.size(); ++z)
	{
		scan_dir_for_rtstruct_image(path, detected_files);
	}
	for (int z = 0; z < detected_files.size(); ++z)
	{
		bool referenced_slice_found{};
		for (int k = 0; k < detected_files.at(z).size(); ++k)
		{
			QString sop_instance_uid;
			std::set<mdcm::Tag> tags;
			mdcm::Tag tsopinstance(0x0008,0x0018);
			tags.insert(tsopinstance);
			mdcm::Reader reader1;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
			reader1.SetFileName(QDir::toNativeSeparators(detected_files.at(z).at(k)).toUtf8().constData());
#else
			reader1.SetFileName(QDir::toNativeSeparators(detected_files.at(z).at(k)).toLocal8Bit().constData());
#endif
#else
			reader1.SetFileName(detected_files.at(z).at(k).toLocal8Bit().constData());
#endif
			const bool f_ok = reader1.ReadSelectedTags(tags);
			if (!f_ok) continue;
#ifndef ALIZA_LOAD_DCM_THREAD
			QApplication::processEvents();
#endif
			const mdcm::DataSet & ds1 = reader1.GetFile().GetDataSet();
			{
				const mdcm::DataElement & e1 = ds1.GetDataElement(tsopinstance);
				if (!e1.IsEmpty() && !e1.IsUndefinedLength() && e1.GetByteValue())
				{
					sop_instance_uid = QString::fromLatin1(
						e1.GetByteValue()->GetPointer(),
						e1.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
				}
			}
			if (!sop_instance_uid.isEmpty())
			{
				for (int y = 0; y < tmp_ivariant->di->rois.size(); ++y)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
					QMap< int, Contour* >::const_iterator it = tmp_ivariant->di->rois.at(y).contours.cbegin();
					while (it != tmp_ivariant->di->rois.at(y).contours.cend())
#else
					QMap< int, Contour* >::const_iterator it = tmp_ivariant->di->rois.at(y).contours.constBegin();
					while (it != tmp_ivariant->di->rois.at(y).contours.constEnd())
#endif
					{
						const Contour * c = it.value();
						for (int j = 0;
							j < c->ref_sop_instance_uids.size();
							++j)
						{
							if (c->ref_sop_instance_uids.at(j) == sop_instance_uid)
							{
								referenced_slice_found = true;
								break;
							}
						}
						if (referenced_slice_found) break;
						++it;
					}
					if (referenced_slice_found) break;
				}
			}
			if (referenced_slice_found)
			{
				std::vector<ImageVariant*> ivariants;
				QStringList detected_files_tmp = detected_files.at(z);
				detected_files_tmp.sort();
				QStringList dummy;
				const QString message_ =
					read_dicom(
						ivariants,
						dummy,
						dummy,
						dummy,
						dummy,
						dummy,
						QString(""),
						detected_files_tmp,
						ok3d,
						settings,
						2,
						enh_loading_type);
#ifdef ALIZA_VERBOSE
				if (!message_.isEmpty())
				{
					std::cout << message_.toStdString() << std::endl;
				}
#else
				(void)message_;
#endif
#if 0
				if (ivariants.size() > 1)
				{
					std::cout << "process_contrours_ref : ivariants.size() > 1" << std::endl;
				}
#endif
				for (unsigned int j = 0; j < ivariants.size(); ++j)
				{
					for (int i = 0; i < tmp_ivariant->di->rois.size(); ++i)
					{
						ROI roi;
						roi.id = tmp_ivariant->di->rois.at(i).id;
						ContourUtils::copy_roi(roi, tmp_ivariant->di->rois.at(i));
						ivariants[j]->di->rois.push_back(roi);
					}
					tmp_ivariants.push_back(ivariants[j]);
					++count_;
				}
				break;
			}
		}
	}
	delete tmp_ivariant;
	if (count_ > 0) return true;
	return false;
}

QString DicomUtils::find_file_from_uid(
	const QString & p,
	const QString & uid)
{
	QString f;
	if (p.isEmpty()) return f;
	if (uid.isEmpty()) return f;
	bool ok = scan_files_for_instance_uid(p, uid, f);
	if (ok) return f;
	QDirIterator it(p, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		ok = scan_files_for_instance_uid(it.next(), uid, f);
		if (ok) break;
	}
	return f;
}

bool DicomUtils::scan_files_for_instance_uid(
	const QString & p,
	const QString & uid,
	QString & file)
{
	if (p.isEmpty()) return false;
	if (uid.isEmpty()) return false;
	std::set<mdcm::Tag> tags;
	mdcm::Tag tSOPInstanceUID(0x0008,0x0018);
	tags.insert(tSOPInstanceUID);
	QDir dir(p);
	QStringList flist = dir.entryList(QDir::Files|QDir::Readable, QDir::Name);
	for (int x = 0; x < flist.size(); ++x)
	{
		const QString tmp0 = dir.absolutePath() + QString("/") + flist.at(x);
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
		if (!reader.ReadSelectedTags(tags)) continue;
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
		QString uid_;
		const bool ok = get_string_value(ds, tSOPInstanceUID, uid_);
		if (ok && uid.trimmed().remove(QChar('\0')) == uid_.trimmed().remove(QChar('\0')))
		{
			file = tmp0;
			return true;
		}
	}
	return false;
}

void DicomUtils::read_pr_ref(
	const QString & p,
	const QString & f,
	QList<PrRefSeries> & refs)
{
	if (f.isEmpty()) return;
	const mdcm::Tag tReferencedSeriesSequence(0x0008,0x1115);
	const mdcm::Tag tSeriesInstanceUID(0x0020,0x000e);
	const mdcm::Tag tReferencedImageSequence(0x0008,0x1140);
	const mdcm::Tag tReferencedSOPInstanceUID(0x0008,0x1155);
	const mdcm::Tag tReferencedFrameNumber(0x0008,0x1160);
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
	if (!reader.Read()) return;
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	if (ds.IsEmpty()) return;
	const mdcm::DataElement & eReferencedSeriesSequence = ds.GetDataElement(tReferencedSeriesSequence);
	mdcm::SmartPointer<mdcm::SequenceOfItems> sqReferencedSeriesSequence = eReferencedSeriesSequence.GetValueAsSQ();
	if (!(sqReferencedSeriesSequence && sqReferencedSeriesSequence->GetNumberOfItems() > 0))
		return;
	for (unsigned int x = 0; x < sqReferencedSeriesSequence->GetNumberOfItems(); ++x)
	{
		const mdcm::Item & item0 = sqReferencedSeriesSequence->GetItem(x + 1);
		const mdcm::DataSet & nds0 = item0.GetNestedDataSet();
		QString s;
		const bool ok = get_string_value(nds0, tSeriesInstanceUID, s);
		if (!ok) continue;
		const mdcm::DataElement & eReferencedImageSequence =
			nds0.GetDataElement(tReferencedImageSequence);
		if (eReferencedImageSequence.IsEmpty()) continue;
		mdcm::SmartPointer<mdcm::SequenceOfItems>
			sqReferencedImageSequence = eReferencedImageSequence.GetValueAsSQ();
		if (!(sqReferencedImageSequence && sqReferencedImageSequence->GetNumberOfItems() > 0))
			continue;
		//
		PrRefSeries series;
		series.uid = std::move(s);
		//
		for (unsigned int y = 0; y < sqReferencedImageSequence->GetNumberOfItems(); ++y)
		{
			const mdcm::Item & item1 = sqReferencedImageSequence->GetItem(y + 1);
			const mdcm::DataSet & nds1 = item1.GetNestedDataSet();
			QString s0;
			const bool ok0 = get_string_value(nds1, tReferencedSOPInstanceUID, s0);
			if (ok0)
			{
				PrRefImage ref;
				ref.uid = std::move(s0);
				ref.file = find_file_from_uid(p, ref.uid);
				if (ref.file.isEmpty()) continue;
				std::vector<int> frames;
				const bool ok1 = get_is_values(nds1, tReferencedFrameNumber, frames);
				if (ok1)
				{
					for (unsigned int z = 0; z < frames.size(); ++z)
						ref.frames.push_back(frames[z]);
				}
				series.images.push_back(ref);
			}
		}
		//
		PrConfigUtils::read_pr(ds, series);
		//
		refs.push_back(series);
	}
}

QString DicomUtils::read_enhmr_spectro_info(
	const mdcm::DataSet & ds,
	bool spectro)
{
	QString s;
	//
	if (!spectro)
	{
		s += read_CommonCTMRImageDescriptionMacro(ds);
	}
	//
	if (!spectro)
	{
		//
		//    MR Timing and Related Parameters Macro
		//
		//MR Timing and Related Parameters Sequence(0018,9112)
		//  Repetition Time(0018,0080)
		//  Flip Angle (0018,1314)
		//  Echo Train Length (0018,0091)
		//  RF Echo Train Length(0018,9240)
		//  Gradient Echo Train Length (0018,9241)
		//  Specific Absorption Rate Sequence (0018,9239) TODO
		//    Specific Absorption Rate Definition (0018,9179) TODO
		//  Gradient Output Type (0018,9180) [*]
		//  Gradient Output (0018,9182)
		//  Operating Mode Sequence (0018,9176) TODO
		//    Operating Mode Type (0018,9177) TODO
		//    Operating Mode (0018,9178) TODO
		//
		// * Gradient Output Type is
		// DB_DT in T/s
		// ELECTRIC_FIELD in V/m
		// PER_NERVE_STIM percentage of peripheral nerve stimulation
		//
		//
		QString tmp0;
		//
		const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
		const mdcm::DataElement & e = ds.GetDataElement(tSharedFunctionalGroupsSequence);
		if (!e.IsEmpty())
		{
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
			if (sq && sq->GetNumberOfItems() == 1)
			{
				const mdcm::Item & item = sq->GetItem(1);
				const mdcm::DataSet& nds = item.GetNestedDataSet();
				const mdcm::Tag tMRTimingAndRelatedSQ(0x0018,0x9112);
				const mdcm::DataElement & e1 = nds.GetDataElement(tMRTimingAndRelatedSQ);
				if (!e1.IsEmpty())
				{
					mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 = e1.GetValueAsSQ();
					if (sq1 && sq1->GetNumberOfItems() == 1)
					{
						const mdcm::Item & item1 = sq1->GetItem(1);
						const mdcm::DataSet& nds1 =
							item1.GetNestedDataSet();
						//
						const mdcm::Tag tRepetitionTime(0x0018,0x0080);
						tmp0 += generate_string_0(
							nds1,
							tRepetitionTime,
							QString("Repetition Time"),
							QString("ms"));
						//
						const mdcm::Tag tFlipAngle(0x0018,0x1314);
						tmp0 += generate_string_0(
							nds1,
							tFlipAngle,
							QString("Flip Angle"),
							QString(QChar(0x00B0)));
						//
						const mdcm::Tag tEchoTrainLength(0x0018,0x0091);
						tmp0 += generate_string_0(
							nds1,
							tEchoTrainLength,
							QString("Echo Train Length"),
							QString(""));
						//
						const mdcm::Tag tRFEchoTrainLength(0x0018,0x9240);
						unsigned short RFEchoTrainLength;
						if (DicomUtils::get_us_value(
								nds1,
								tRFEchoTrainLength,
								&RFEchoTrainLength))
						{
							QString sRFEchoTrainLength = QVariant(static_cast<int>(RFEchoTrainLength)).toString();
							sRFEchoTrainLength = sRFEchoTrainLength.trimmed();
							if (!sRFEchoTrainLength.isEmpty())
							{
								tmp0 += QString(
									"<span class='y9'>"
									"RF Echo Train Length"
									"</span><br /><span class='y8'>");
								tmp0 += sRFEchoTrainLength;
								tmp0 += QString("</span><br />");
							}
						}
						//
						const mdcm::Tag tGradientEchoTrainLength(0x0018,0x9241);
						unsigned short GradientEchoTrainLength;
						if (DicomUtils::get_us_value(
								nds1,
								tGradientEchoTrainLength,
								&GradientEchoTrainLength))
						{
							QString sGradientEchoTrainLength = QVariant(static_cast<int>(GradientEchoTrainLength)).toString();
							sGradientEchoTrainLength = sGradientEchoTrainLength.trimmed().remove(QChar('\0'));
							if (!sGradientEchoTrainLength.isEmpty())
							{
								tmp0 += QString(
									"<span class='y9'>"
									"Gradient Echo Train Length"
									"</span><br /><span class='y8'>");
								tmp0 += sGradientEchoTrainLength;
								tmp0 += QString("</span><br />");
							}
						}
						//
						const mdcm::Tag tGradientOutput(0x0018,0x9182);
						double GradientOutput;
						if (DicomUtils::get_fd_value(
							ds,
							tGradientOutput,
							&GradientOutput))
						{
							const mdcm::Tag tGradientOutputType(0x0018,0x9180);
							QString GradientOutputType;
							const bool tGradientOutputType_ok =
								get_string_value(
									nds1,
									tGradientOutputType,
									GradientOutputType);
							(void)tGradientOutputType_ok;
							GradientOutputType = GradientOutputType.simplified().remove(QChar('\0'));
							QString sGradientOutputType = GradientOutputType;
							if (GradientOutputType.toUpper() == QString("DB_DT"))
							{
								sGradientOutputType = QString("T/s");
							}
							else if (GradientOutputType.toUpper() == QString("ELECTRIC_FIELD"))
							{
								sGradientOutputType = QString("V/m");
							}
							else if (GradientOutputType.toUpper() == QString("PER_NERVE_STIM"))
							{
								sGradientOutputType = QString("&#37;") + QString(" nerve stim.");
							}
							QString sGradientOutput = QVariant(GradientOutput).toString();
							sGradientOutput = sGradientOutput.trimmed();
							if (!sGradientOutput.isEmpty())
							{
								tmp0 += QString(
									"<span class='y9'>"
									"Gradient Output</span>"
									"<br /><span class='y8'>");
								tmp0 += sGradientOutput;
								tmp0 += QString(" ");
								tmp0 += sGradientOutputType;
								tmp0 += QString("</span><br />");
							}
						}
					}
				}
			}
		}
		if (!tmp0.isEmpty())
		{
			s += QString(
				"<span class='y7'>MR Timing and Related Parameters Macro"
				"</span><br />") + tmp0;
		}
	}
	//
	if (!spectro)
	{
		//
		// MR Pulse Sequence Module
		//
		//Pulse Sequence Name (0018,9005)
		//MR Acquisition Type (0018,0023)
		//Echo Pulse Sequence (0018,9008)
		//Multiple Spin Echo (0018,9011)
		//Multi-planar Excitation (0018,9012)
		//Phase Contrast (0018,9014)
		//Velocity Encoding Acquisition Sequence (0018,9092)
		//  Velocity Encoding Direction (0018,9090)
		//Time of Flight Contrast (0018,9015)
		//Arterial Spin Labeling Contrast (0018,9250)
		//Steady State Pulse Sequence (0018,9017)
		//Echo Planar Pulse Sequence (0018,9018)
		//Saturation Recovery (0018,9024)
		//Spectrally Selected Suppression (0018,9025)
		//Oversampling Phase (0018,9029)
		//Geometry of k-Space Traversal(0018,9032)
		//Rectilinear Phase Encode Reordering(0018,9034)
		//Segmented k-Space Traversal (0018,9033)
		//Coverage of k-Space (0018,9094)
		//Number of k-Space Trajectories (0018,9093)
		//
		QString tmp0;
		//
		const mdcm::Tag tPulseSequenceName(0x0018,0x9005);
		tmp0 += generate_string_0(
			ds,
			tPulseSequenceName,
			QString("Pulse Sequence Name"),
			QString(""));
		//
		const mdcm::Tag tMRAcquisitionType(0x0018,0x0023);
		tmp0 += generate_string_0(
			ds,
			tMRAcquisitionType,
			QString("MR Acquisition Type"),
			QString(""));
		//
		const mdcm::Tag tEchoPulseSequence(0x0018,0x9008);
		tmp0 += generate_string_0(
			ds,
			tEchoPulseSequence,
			QString("Echo Pulse Sequence"),
			QString(""));
		//
		const mdcm::Tag tMultipleSpinEcho(0x0018,0x9011);
		tmp0 += generate_string_0(
			ds,
			tMultipleSpinEcho,
			QString("Multiple Spin Echo"),
			QString(""));
		//
		const mdcm::Tag tMultiplanarExcitation(0x0018,0x9012);
		tmp0 += generate_string_0(
			ds,
			tMultiplanarExcitation,
			QString("Multi-planar Excitation"),
			QString(""));
		//
		const mdcm::Tag tPhaseContrast(0x0018,0x9012);
		tmp0 += generate_string_0(
			ds,
			tPhaseContrast,
			QString("Phase Contrast"),
			QString(""));
		//
		const mdcm::Tag tVelocityEncodingAcquisitionSequence(0x0018,0x9092);
		{
			const mdcm::DataElement & e = ds.GetDataElement(tVelocityEncodingAcquisitionSequence);
			if (!e.IsEmpty())
			{
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq = e.GetValueAsSQ();
				if (sq && sq->GetNumberOfItems() > 1)
				{
					const mdcm::Tag tVelocityEncodingDirection(0x0018,0x9090);
					tmp0 += QString("<span class='y9'>");
					tmp0 += QString("Velocity Encoding Directions");
					tmp0 += QString("</span><br />");
					for (int i = 0; i < static_cast<int>(sq->GetNumberOfItems()); ++i)
					{
						const mdcm::Item & item = sq->GetItem(i + 1);
						const mdcm::DataSet& nds = item.GetNestedDataSet();
						{
							std::vector<double> res;
							if (get_fd_values(nds, tVelocityEncodingDirection, res))
							{
								if (res.size() == 3)
								{
									tmp0 += QString("<span class='y8'>");
									tmp0 += QVariant(res[0]).toString() + QString(" ");
									tmp0 += QVariant(res[1]).toString() + QString(" ");
									tmp0 += QVariant(res[2]).toString();
									tmp0 += QString("</span><br />");
								}
							}
						}
					}
				}
			}
		}
		//
		const mdcm::Tag tTimeofFlightContrast(0x0018,0x9015);
		tmp0 += generate_string_0(
			ds,
			tTimeofFlightContrast,
			QString("Time of Flight Contrast"),
			QString(""));
		//
		const mdcm::Tag tArterialSpinLabelingContrast(0x0018,0x9050);
		tmp0 += generate_string_0(
			ds,
			tArterialSpinLabelingContrast,
			QString("Arterial Spin Labeling Contrast"),
			QString(""));
		//
		const mdcm::Tag tSteadyStatePulseSequence(0x0018,0x9017);
		tmp0 += generate_string_0(
			ds,
			tSteadyStatePulseSequence,
			QString("Steady State Pulse Sequence"),
			QString(""));
		//
		const mdcm::Tag tEchoPlanarPulseSequence(0x0018,0x9018);
		tmp0 += generate_string_0(
			ds,
			tEchoPlanarPulseSequence,
			QString("Echo Planar Pulse Sequence"),
			QString(""));
		//
		const mdcm::Tag tSaturationRecovery(0x0018,0x9024);
		tmp0 += generate_string_0(
			ds,
			tSaturationRecovery,
			QString("Saturation Recovery"),
			QString(""));
		//
		const mdcm::Tag tSpectrallySelectedSuppression(0x0018,0x9025);
		tmp0 += generate_string_0(
			ds,
			tSpectrallySelectedSuppression,
			QString("Spectrally Selected Suppression"),
			QString(""));
		//
		const mdcm::Tag tOversamplingPhase(0x0018,0x9029);
		tmp0 += generate_string_0(
			ds,
			tOversamplingPhase,
			QString("Oversampling Phase"),
			QString(""));
		//
		const mdcm::Tag tGeometryofkSpaceTraversal(0x0018,0x9032);
		tmp0 += generate_string_0(
			ds,
			tGeometryofkSpaceTraversal,
			QString("Geometry of k-Space Traversal"),
			QString(""));
		//
		const mdcm::Tag tRectilinearPhaseEncodeReordering(0x0018,0x9034);
		tmp0 += generate_string_0(
			ds,
			tRectilinearPhaseEncodeReordering,
			QString("Rectilinear Phase Encode Reordering"),
			QString(""));
		//
		const mdcm::Tag tSegmentedkSpaceTraversal(0x0018,0x9033);
		tmp0 += generate_string_0(
			ds,
			tSegmentedkSpaceTraversal,
			QString("Segmented k-Space Traversal"),
			QString(""));
		//
		const mdcm::Tag tCoverageofkSpace(0x0018,0x9094);
		tmp0 += generate_string_0(
			ds,
			tCoverageofkSpace,
			QString("Coverage of k-Space"),
			QString(""));
		//
		const mdcm::Tag tNumberofkSpaceTrajectories(0x0018,0x9093);
		unsigned short NumberofkSpaceTrajectories;
		if (DicomUtils::get_us_value(
				ds,
				tNumberofkSpaceTrajectories,
				&NumberofkSpaceTrajectories))
		{
			QString sNumberofkSpaceTrajectories =
				QVariant(static_cast<int>(NumberofkSpaceTrajectories)).toString();
			sNumberofkSpaceTrajectories =
				sNumberofkSpaceTrajectories.trimmed();
			if (!sNumberofkSpaceTrajectories.isEmpty())
			{
				tmp0 += QString(
					"<span class='y9'>Number of"
					" k-Space Trajectories</span>"
					"<br /><span class='y8'>");
				tmp0 += sNumberofkSpaceTrajectories;
				tmp0 += QString("</span><br />");
			}
		}
		if (!tmp0.isEmpty())
		{
			s += QString(
				"<span class='y7'>MR Pulse Sequence Module"
				"</span><br />") + tmp0;
		}
	}
	//
	{
		//
		//    MR Image and Spectroscopy Instance Macro
		//
		//Content Qualification (0018,9004)
		//Resonant Nucleus (0018,9100)
		//k-space Filtering (0018,9064)
		//Magnetic Field Strength(0018,0087)
		//Applicable Safety Standard Agency (0018,9174)
		//Applicable Safety Standard Description (0018,9175)
		//B1rms(0018,1320)
		//
		QString tmp0;
		//
		const mdcm::Tag tContentQualification(0x0018,0x9104);
		tmp0 += generate_string_0(
			ds,
			tContentQualification,
			QString("Content Qualification"),
			QString(""));
		//
		const mdcm::Tag tResonantNucleus(0x0018,0x9100);
		tmp0 += generate_string_0(
			ds,
			tResonantNucleus,
			QString("Resonant Nucleus"),
			QString(""));
		//
		const mdcm::Tag tkspaceFiltering(0x0018,0x9064);
		tmp0 += generate_string_0(
			ds,
			tkspaceFiltering,
			QString("k-space Filtering"),
			QString(""));
		//
		const mdcm::Tag tApplicableSafetyStandardAgency(0x0018,0x9174);
		tmp0 += generate_string_0(
			ds,
			tApplicableSafetyStandardAgency,
			QString("Applicable Safety Standard Agency"),
			QString(""));
		//
		const mdcm::Tag tApplicableSafetyStandardDescription(0x0018,0x9175);
		tmp0 += generate_string_0(
			ds,
			tApplicableSafetyStandardDescription,
			QString("Applicable Safety Standard Description"),
			QString(""));
		//
		const mdcm::Tag tB1rms(0x0018,0x1320);
		float fB1rms;
		if (DicomUtils::get_fl_value(ds, tB1rms, &fB1rms))
		{
			QString B1rms = QVariant(static_cast<double>(fB1rms)).toString();
			B1rms = B1rms.trimmed();
			if (!B1rms.isEmpty())
			{
				tmp0 += QString(
					"<span class='y9'>B1 + rms</span>"
					"<br /><span class='y8'>");
				tmp0 += B1rms;
				tmp0 += QString(" uT</span><br />");
			}
		}
		//
		if (!tmp0.isEmpty())
		{
			s += QString(
				"<span class='y7'>MR Image and Spectroscopy Instance Macro"
				"</span><br />") + tmp0;
		}
	}
	//
	if (!s.isEmpty()) s += QString("<br />");
	return s;
}

QString DicomUtils::read_enhct_info(
	const mdcm::DataSet & ds)
{
	QString s;
	//
	s += read_CommonCTMRImageDescriptionMacro(ds);
	//
	if (!s.isEmpty()) s += QString("<br />");
	return s;
}

mdcm::VR DicomUtils::get_vr(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	const bool implicit,
	const mdcm::Dicts & dicts)
{
	mdcm::VR vr = mdcm::VR::INVALID;
	bool priv{};
	if (t.IsIllegal())
	{
		return vr;
	}
	else if (t.IsPrivateCreator())
	{
		vr = mdcm::VR::LO;
	}
	else if (t.IsGroupLength())
	{
		vr = mdcm::VR::UL;
	}
	else if (t.IsPrivate())
	{
		priv = true;
	}
	if (!priv)
	{
		if (!implicit)
		{
			const mdcm::DataElement & e = ds.GetDataElement(t);
			if (e != mdcm::DataElement(mdcm::Tag(0xffff,0xffff)))
			{
				vr = e.GetVR();
				// CP-246
				if (vr == mdcm::VR::UN || vr == mdcm::VR::INVALID)
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
			const mdcm::DictEntry & dictentry = dicts.GetDictEntry(t);
			vr = dictentry.GetVR();
		}
	}
	else
	{
		bool ok{};
		if (!implicit)
		{
			vr = ds.GetDataElement(t).GetVR();
			// CP-246
			if (!(vr == mdcm::VR::UN || vr == mdcm::VR::INVALID))
			{
				ok = true;
			}
		}
		if (!ok)
		{
			const mdcm::PrivateDict & pdict = dicts.GetPrivateDict();
			mdcm::Tag private_creator_t = t.GetPrivateCreator();
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
				mdcm::PrivateTag pt(t.GetGroup(), t.GetElement(), private_creator.toLatin1().constData());
				const mdcm::DictEntry & pentry = pdict.GetDictEntry(pt);
				vr = pentry.GetVR();
			}
		}
	}
	return vr;
}

bool DicomUtils::compatible_sq(
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

QString DicomUtils::generate_uid()
{
	mdcm::UIDGenerator g;
	const QString r = QString::fromLatin1(g.Generate());
	return r;
}

bool DicomUtils::read_gray_lut(
	const mdcm::DataSet & ds,
	const mdcm::Tag lut_seq,
	QList<QVariant> & lut_descriptor,
	QList<QVariant> & lut_data,
	bool * mapped_implicit,
	bool * mapped_signed)
{
	const mdcm::Tag tLUTDescriptor(0x0028,0x3002); // US or SS
	const mdcm::Tag tLUTData(0x0028,0x3006); // US or OW
	const mdcm::DataElement & de = ds.GetDataElement(lut_seq);
	if (de.IsEmpty()) return false;
	const mdcm::SmartPointer<mdcm::SequenceOfItems> sq = de.GetValueAsSQ();
	if (!sq)
	{
		return false;
	}
	const unsigned int number_of_items = sq->GetNumberOfItems();
	if (number_of_items != 1)
	{
		return false;
	}
	const mdcm::Item & item = sq->GetItem(1);
	const mdcm::DataSet & nds = item.GetNestedDataSet();
	std::vector<unsigned short> LUTDescriptorUS;
	const mdcm::DataElement & deLUTDescriptor = nds.GetDataElement(tLUTDescriptor);
	const mdcm::DataElement & deLUTData = nds.GetDataElement(tLUTData);
	if (deLUTDescriptor == mdcm::DataElement(mdcm::Tag(0xffff,0xffff)) ||
		deLUTData == mdcm::DataElement(mdcm::Tag(0xffff,0xffff)))
	{
		return false;
	}
	const mdcm::VR vrLUTDescriptor = deLUTDescriptor.GetVR();
#if 0
	std::cout << "vrLUTDescriptor=" << vrLUTDescriptor << std::endl;
#endif
	const bool LUTDescriptor_signed = (vrLUTDescriptor == mdcm::VR::SS);
	if (LUTDescriptor_signed)
	{
		*mapped_signed = true;
	}
	else
	{
		*mapped_signed = false;
	}
	if (vrLUTDescriptor == mdcm::VR::INVALID || vrLUTDescriptor == mdcm::VR::UN)
	{
		*mapped_implicit = true;
	}
	else
	{
		*mapped_implicit = false;
	}
	const mdcm::VR vrLUTData = deLUTData.GetVR();
#if 0
	std::cout << "vrLUTData="<< vrLUTData << std::endl;
#else
	(void)vrLUTData;
#endif
	if (DicomUtils::get_us_values(nds, tLUTDescriptor, LUTDescriptorUS))
	{
		if (LUTDescriptorUS.size() == 3)
		{
			lut_descriptor.push_back(QVariant(static_cast<int>(LUTDescriptorUS[0])));
			lut_descriptor.push_back(QVariant(static_cast<int>(LUTDescriptorUS[1])));
			lut_descriptor.push_back(QVariant(static_cast<int>(LUTDescriptorUS[2])));
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	const unsigned int lut_data_bits  = lut_descriptor.at(2).toInt();
	const unsigned int lut_data_size0 = lut_descriptor.at(0).toInt();
	const mdcm::DataElement & e = nds.GetDataElement(tLUTData);
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv)
	{
		return false;
	}
	const char * p = bv->GetPointer();
	const size_t lut_data_size1 = bv->GetLength();
	if (!p)
	{
		return false;
	}
#if 0
	std::cout << "lut_data_bits = "  << lut_data_bits  << std::endl;
	std::cout << "lut_data_size0 = " << lut_data_size0 << std::endl;
	std::cout << "lut_data_size1 = " << lut_data_size1 << std::endl;
#endif
	if (lut_data_bits == 8 && lut_data_size0 == lut_data_size1)
	{
		const void * void_p = static_cast<const void*>(p);
		const unsigned char * lut_data8 = static_cast<const unsigned char *>(void_p);
		for (size_t x = 0; x < lut_data_size0; ++x)
		{
			lut_data.push_back(QVariant(static_cast<int>(lut_data8[x])));
		}
	}
	else if (lut_data_size0 * 2 == lut_data_size1)
	{
		const void * void_p = static_cast<const void*>(p);
		const unsigned short * lut_data16 = static_cast<const unsigned short *>(void_p);
		for (size_t x = 0; x < lut_data_size0; ++x)
		{
			lut_data.push_back(QVariant(static_cast<int>(lut_data16[x])));
		}
	}
	else
	{
		return false;
	}
	return true;
}

// load_type
// 0 - default
// 1 - PR reference
// 2 - RT reference
// 3 - SR reference
QString DicomUtils::read_dicom(
	std::vector<ImageVariant*> & ivariants,
	QStringList & pdf_files,
	QStringList & stl_files,
	QStringList & video_files,
	QStringList & spectroscopy_files,
	QStringList & sr_files,
	const QString & root,
	const QStringList & filenames,
	bool ok3d,
	const QWidget * settings,
	short load_type,
	short enh_loading_type)
{
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_dicom() begin");
#endif
	bool ok{};
	QString message_;
	const mdcm::Tag tSOPClassUID(0x0008,0x0016);
	const mdcm::Tag tSlicePosition(0x0020,0x1041);
	const mdcm::Tag tPhotometricInterpretation(0x0028,0x0004);
	const mdcm::Tag tPixelSpacing(0x0028,0x0030);
	QStringList images;
	QStringList rtstruct_ref_search;
	QString     rtstruct_ref_search_path;
	QStringList grey_softcopy_pr_files;
	QStringList color_softcopy_pr_files;
	QStringList pseudo_color_softcopy_pr_files;
	QStringList blending_softcopy_pr_files;
	QStringList xaxrf_softcopy_pr_files;
	QStringList advanced_blending_softcopy_pr_files;
	QVector<QStringList> extracted_images;
	bool enhanced{};
	bool supp_palette{};
	bool multiframe{};
	bool mosaic{};
	bool elscint{};
	bool uihgrid{};
	bool mixed{};
	bool skip_volume{};
	bool multiseries{};
	bool ultrasound{};
	bool nuclear{};
	unsigned short rows_tmp0{};
	unsigned short rows_tmp1{};
	unsigned short columns_tmp0{};
	unsigned short columns_tmp1{};
	unsigned short ba_tmp0{};
	unsigned short ba_tmp1{};
	bool localizer_tmp0{};
	bool localizer_tmp1{};
	QString sop_tmp0, sop_tmp1;
	QString photometric_tmp0, photometric_tmp1;
	QString pspacing_tmp0, pspacing_tmp1;
	bool icc_found_tmp0{};
	bool icc_found_tmp1{};
	const SettingsWidget * const wsettings =
		static_cast<const SettingsWidget * const>(settings);
	std::map<unsigned int, SliceInstance> slice_pos_map;
	std::list<long long> slice_pos_list;
	const float tolerance{0.01f};
	int count_images{};
	int count_uid_errors{};
	//
	//
	//
	const int filenames_size = filenames.size();
	const QString filenames_num = QString(" / ") + QString::number(filenames_size);
	for (int x = 0; x < filenames_size; ++x)
	{
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		QString sop;
		QString photometric;
		QString pspacing;
		bool icc_found{};
		unsigned short columns_{};
		unsigned short rows_{};
		unsigned short ba_{};
		unsigned short bs_{};
		unsigned short hb_{};
		short pr_{-1};
		bool localizer_{};
		QFileInfo fi(filenames.at(x));
		mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
		reader.SetFileName(QDir::toNativeSeparators(filenames.at(x)).toUtf8().constData());
#else
		reader.SetFileName(QDir::toNativeSeparators(filenames.at(x)).toLocal8Bit().constData());
#endif
#else
		reader.SetFileName(filenames.at(x).toLocal8Bit().constData());
#endif
		if (!reader.Read()) continue;
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		const mdcm::File & file = reader.GetFile();
		const mdcm::FileMetaInformation & header = file.GetHeader();
		const mdcm::TransferSyntax & ts = header.GetDataSetTransferSyntax();
		const mdcm::DataSet & ds = file.GetDataSet();
		if (ds.IsEmpty())
		{
			continue;
		}
#ifndef ALIZA_LOAD_DCM_THREAD
		QApplication::processEvents();
#endif
		{
			const mdcm::DataElement & sop_ = ds.GetDataElement(tSOPClassUID);
			if (!sop_.IsEmpty() && !sop_.IsUndefinedLength() && sop_.GetByteValue())
			{
				sop = QString::fromLatin1(
					sop_.GetByteValue()->GetPointer(),
					sop_.GetByteValue()->GetLength()).trimmed().remove(QChar('\0'));
			}
		}
		const bool tPhotometricInterpretation_ok =
			DicomUtils::get_string_value(ds, tPhotometricInterpretation, photometric);
		(void)tPhotometricInterpretation_ok;
		const bool tPixelSpacing_ok = DicomUtils::get_string_value(
			ds, tPixelSpacing, pspacing);
		(void)tPixelSpacing_ok;
		if (wsettings->get_apply_icc() && ds.FindDataElement(mdcm::Tag(0x0028,0x2000)))
		{
			icc_found = true;
		}
#if 1
		if (sop == QString("1.2.840.10008.5.1.4.1.1.77.1.6")) // TODO
#else
		if (false)
#endif
		{
			// VL Whole Slide Microscopy Image Storage
			if (load_type == 0)
			{
				if (!message_.isEmpty()) message_ += QChar('\n');
				message_ += QString("VL Whole Slide Microscopy Image Storage");
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.481.3"))
		{
			// RTSTRUCT
			if (load_type == 0)
			{
				QFileInfo reffi(filenames.at(x));
				rtstruct_ref_search_path = reffi.absolutePath();
				rtstruct_ref_search.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.11.1"))
		{
			// Grayscale Softcopy presentation
			if (load_type == 0)
			{
				grey_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.11.2"))
		{
			// Color Softcopy presentation
			if (load_type == 0)
			{
				color_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.11.3"))
		{
			// Pseudo-color Softcopy presentation
			if (load_type == 0)
			{
				pseudo_color_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.11.4"))
		{
			// Blending Softcopy presentation
			if (load_type == 0)
			{
				blending_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.11.5"))
		{
			// XA/XRF Softcopy presentation
			if (load_type == 0)
			{
				xaxrf_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.11.8"))
		{
			// Advanced Blending Softcopy presentation
			if (load_type == 0)
			{
				advanced_blending_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.4.2"))
		{
			// Spectroscopy
			if (load_type == 0)
			{
				spectroscopy_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (
			sop == QString("1.2.840.10008.5.1.4.1.1.66.5") ||
			sop == QString("1.2.840.10008.5.1.4.1.1.68.1"))
		{
			// Meshes
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.104.1"))
		{
			// PDF
			if (load_type == 0)
			{
				if (check_encapsulated(ds))
					pdf_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop == QString("1.2.840.10008.5.1.4.1.1.104.3"))
		{
			// STL
			if (load_type == 0)
			{
				if (check_encapsulated(ds))
					stl_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (
			sop == QString("1.2.840.10008.5.1.4.1.1.77.1.1.1") ||
			sop == QString("1.2.840.10008.5.1.4.1.1.77.1.4.1"))
		{
			// Video Endoscopic Image Storage
			// Video Photographic Image Storage
			if (load_type == 0)
			{
				video_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (
			   sop == QString("1.2.840.10008.5.1.4.1.1.88.11") // Basic Text SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.22") // Enhanced SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.33") // Comprehensive SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.34") // Comprehensive 3D SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.35") // Extensible SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.40") // Procedure Log Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.50") // Mammography CAD SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.59") // Key Object Selection Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.65") // Chest CAD SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.67") // X-Ray Radiation Dose SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.68") // Radiopharmaceutical Radiation Dose SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.69") // Colon CAD SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.70") // Implantation Plan SR Document Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.71") // Acquisition Context SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.72") // Simplified Adult Echo SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.73") // Patient Radiation Dose SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.74") // Planned Imaging Agent Administration SR Storage
			|| sop == QString("1.2.840.10008.5.1.4.1.1.88.75") // Performed Imaging Agent Administration SR Storage
			)
		{
			if (load_type == 0)
			{
				sr_files.push_back(filenames.at(x));
			}
			continue;
		}
		//
		if (is_image(
				ds,
				&rows_, &columns_,
				&ba_, &bs_, &hb_,
				&pr_,
				&localizer_))
		{
			sop_tmp0 = sop;
			rows_tmp0 = rows_;
			columns_tmp0 = columns_;
			ba_tmp0 = ba_;
			localizer_tmp0 = localizer_;
			photometric_tmp0 = std::move(photometric);
			pspacing_tmp0 = std::move(pspacing);
			icc_found_tmp0 = icc_found;
			if (sop == QString("1.2.840.10008.5.1.4.1.1.6.1") ||
				sop == QString("1.2.840.10008.5.1.4.1.1.6")   ||
				sop == QString("1.2.840.10008.5.1.4.1.1.3.1") ||
				sop == QString("1.2.840.10008.5.1.4.1.1.3"))
			{
				ultrasound = true;
			}
			else if (sop == QString("1.2.840.10008.5.1.4.1.1.20"))
			{
				nuclear = true;
			}
			else
			{
				// for possible multiseries
				{
					const mdcm::DataElement & sp_ = ds.GetDataElement(tSlicePosition);
					if (!sp_.IsEmpty() && !sp_.IsUndefinedLength() && sp_.GetByteValue())
					{
						bool sp_ok{};
						QString sp = QString::fromLatin1(
							sp_.GetByteValue()->GetPointer(),
							sp_.GetByteValue()->GetLength());
						if (sp.contains(QString(",")))
						{
							// Workaround invalid VR
							sp.replace(QString(","), QString("."));
						}
						const double spvd =
							QVariant(sp.trimmed().remove(QChar('\0'))).toDouble(&sp_ok);
						if (sp_ok)
						{
							const long long spvl = 1000 * CommonUtils::set_digits(spvd, 3);
							SliceInstance si;
							si.id = x;
							si.slice_position = spvl;
							si.instance_number = read_instance_number(ds);
							slice_pos_map[x] = si;
							slice_pos_list.push_back(spvl);
						}
					}
				}
				if (has_functional_groups(ds))
				{
					// "1.2.840.10008.5.1.4.1.1.4.1"      Enhanced MR
					// "1.2.840.10008.5.1.4.1.1.2.1"      Enhanced CT
					// "1.2.840.10008.5.1.4.1.1.130"      Enhanced PET
					// "1.2.840.10008.5.1.4.1.1.6.2"      Enhanced US Volume

					// "1.2.840.10008.5.1.4.1.1.13.1.1"   Enhanced X Ray 3D Angiographic
					// "1.2.840.10008.5.1.4.1.1.13.1.2"   Enhanced X-Ray 3D Craniofacial
					// "1.2.840.10008.5.1.4.1.1.13.1.3"   Breast Tomosynthesis
					// "1.2.840.10008.5.1.4.1.1.2.2"      Legacy Converted Enhanced CT
					// "1.2.840.10008.5.1.4.1.1.4.4"      Legacy Converted Enhanced MR
					// "1.2.840.10008.5.1.4.1.1.128.1"    Legacy Converted Enhanced PET
					// "1.2.840.10008.5.1.4.1.1.30"       Parametric Map
					// "1.2.840.10008.5.1.4.1.1.66.4"     Segmentation
					// "1.2.840.10008.5.1.4.1.1.4.3"      Enhanced MR Color
					// "1.2.840.10008.5.1.4.1.1.77.1.5.4" Ophthalmic Tomography
					// "1.2.840.10008.5.1.4.1.1.14.1"     Intravascular Optical Coherence Tomography Image Storage - For Presentation
					// "1.2.840.10008.5.1.4.1.1.14.2"     Intravascular Optical Coherence Tomography Image Storage - For Processing
					// "1.2.840.10008.5.1.4.1.1.13.1.4"   Breast Projection X-Ray Image Storage - For Presentation
					// "1.2.840.10008.5.1.4.1.1.13.1.5"   Breast Projection X-Ray Image Storage - For Processing
					// "1.2.840.10008.5.1.4.1.1.77.1.5.5" Wide Field Ophthalmic Photography Stereographic Projection
					// "1.2.840.10008.5.1.4.1.1.77.1.5.6" Wide Field Ophthalmic Photography 3D Coordinates
					// "1.2.840.10008.5.1.4.1.1.12.1.1"   Enhanced X-Ray Angiographic
					// "1.2.840.10008.5.1.4.1.1.12.2.1"   Enhanced X-Ray RF
					//
					// "1.2.840.10008.5.1.4.1.1.7.3" Multi-frame Grayscale Word SC
					// "1.2.840.10008.5.1.4.1.1.7.2" Multi-frame Grayscale Byte SC
					// "1.2.840.10008.5.1.4.1.1.7.4" Multi-frame True Color SC
					//
					// and unknown IODs
					//
					enhanced = true;
				}
				if (enhanced)
				{
					if (force_suppllut == 0)
					{
						if ((load_type == 0||load_type == 2) && has_supp_palette(ds))
						{
							supp_palette = wsettings->get_apply_supplemental_lut();
						}
					}
					else if (force_suppllut == 1)
					{
						supp_palette = true;
					}
					else if (force_suppllut == 2)
					{
						supp_palette = false;
					}
				}
				if (wsettings->get_mosaic())
				{
					if (is_mosaic(ds))
					{
						mosaic  = true;
					}
					else if (is_uih_grid(ds))
					{
						uihgrid = true;
					}
				}
				if (is_elscint(ds))
				{
					elscint = true;
				}
				else if (ts == mdcm::TransferSyntax::CT_private_ELE)
				{
#ifdef ALIZA_VERBOSE
					std::cout << "Warning: transfer syntax CT-private-ELE" << std::endl;
#else
					(void)ts;
#endif
				}
				if (is_multiframe(ds))
				{
					multiframe = true;
				}
				if ((count_images > 0) && (
					rows_tmp1        != rows_tmp0        ||
					columns_tmp1     != columns_tmp0     ||
					ba_tmp1          != ba_tmp0          ||
					sop_tmp1         != sop_tmp0         ||
					photometric_tmp1 != photometric_tmp0 ||
					pspacing_tmp1    != pspacing_tmp0    ||
					icc_found_tmp1   != icc_found_tmp0   ||
					localizer_tmp1   != localizer_tmp0))
				{
					mixed = true;
#if 0
					std::cout << "Mixed series" << std::endl;
#endif
				}
				if (
#if 0
				   sop == QString("1.2.840.10008.5.1.4.1.1.7")       || // SC
#endif
				   sop == QString("1.2.840.10008.5.1.4.1.1.77.1.5.1")|| // Ophthalmic Photography  8 Bit
				   sop == QString("1.2.840.10008.5.1.4.1.1.77.1.5.2")|| // Ophthalmic Photography 16 Bit
				   sop == QString("1.2.840.10008.5.1.4.1.1.1")       || // Computed Radiography
				   sop == QString("1.2.840.10008.5.1.4.1.1.1.1")     || // Digital X-Ray - For Presentation
				   sop == QString("1.2.840.10008.5.1.4.1.1.1.1.1")   || // Digital X-Ray - For Processing
				   sop == QString("1.2.840.10008.5.1.4.1.1.1.2")     || // Digital Mammography X-Ray - For Presentation
				   sop == QString("1.2.840.10008.5.1.4.1.1.1.2.1")   || // Digital Mammography X-Ray - For Processing
				   sop == QString("1.2.840.10008.5.1.4.1.1.1.3")     || // Digital Intra-Oral X-Ray - For Presentation
				   sop == QString("1.2.840.10008.5.1.4.1.1.1.3.1")   || // Digital Intra-Oral X-Ray - For Processing
				   sop == QString("1.2.840.10008.5.1.4.1.1.12.1")    || // X-Ray Angiographic
				   sop == QString("1.2.840.10008.5.1.4.1.1.12.2")       // X-Ray RF
				)
				{
					skip_volume = true;
				}
			}
			images.push_back(filenames.at(x));
			++count_images;
			sop_tmp1 = sop_tmp0;
			rows_tmp1 = rows_tmp0;
			columns_tmp1 = columns_tmp0;
			ba_tmp1 = ba_tmp0;
			photometric_tmp1 = photometric_tmp0;
			pspacing_tmp1 = pspacing_tmp0;
			icc_found_tmp1 = icc_found_tmp0;
			localizer_tmp1 = localizer_tmp0;
		}
	}
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_dicom() 1");
#endif
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	//
	//
	//
	// is multiseries?
	if (!ultrasound &&
		!nuclear &&
		!multiframe &&
		!enhanced &&
		!mixed &&
		!skip_volume &&
		!mosaic &&
		!uihgrid)
	{
		const size_t size0 = slice_pos_list.size();
		slice_pos_list.sort();
		slice_pos_list.unique();
		const size_t size1 = slice_pos_list.size();
		if (size0 != size1 &&
			size0 == static_cast<size_t>(images.size()) &&
			images.size()%size1 == 0)
		{
			multiseries = true;
		}
		slice_pos_list.clear();
		if (multiseries)
		{
			// does every image have unique instance id?
			bool unique_instance_nums{};
			std::list<int> slices_instance_nums;
			{
				for (std::map<unsigned int, SliceInstance>::const_iterator it =
						slice_pos_map.cbegin();
					it != slice_pos_map.cend();
					++it)
				{
					const SliceInstance & si = it->second;
					if (si.instance_number > 0)
					{
						slices_instance_nums.push_back(si.instance_number);
					}
				}
				const size_t inst_size0 = slices_instance_nums.size();
				slices_instance_nums.sort();
				slices_instance_nums.unique();
				const size_t inst_size1 = slices_instance_nums.size();
				if (inst_size0 == inst_size1 &&
					inst_size0 == static_cast<size_t>(images.size()))
				{
					unique_instance_nums = true;
				}
				slices_instance_nums.clear();
			}
			std::map<unsigned int, unsigned int> slices_instance_map;
			if (unique_instance_nums)
			{
				for (std::map<unsigned int, SliceInstance>::const_iterator it =
						slice_pos_map.cbegin();
					it != slice_pos_map.cend();
					++it)
				{
					slices_instance_map[it->second.instance_number] = it->second.id;
				}
				std::vector<unsigned int> image_ids;
				for (std::map<unsigned int, unsigned int>::const_iterator it =
						slices_instance_map.cbegin();
					it != slices_instance_map.cend();
					++it)
				{
					image_ids.push_back(it->second);
				}
				// is sequential or interleaved?
				bool interleaved{};
				for (unsigned int k = 0; k < image_ids.size(); k += size1)
				{
					QStringList images_tmp;
					for (unsigned int j = 0; j < size1; ++j)
					{
						const unsigned int id__ = image_ids.at(k+j);
						if (id__<static_cast<unsigned int>(images.size()))
						{
							images_tmp.push_back(images.at(id__));
						}
					}
					if (!(is_not_interleaved(images_tmp)))
					{
						interleaved = true;
						break;
					}
					extracted_images.push_back(images_tmp);
				}
				image_ids.clear();
				if (interleaved)
				{
					for (int k = 0; k < extracted_images.size(); ++k)
					{
						extracted_images[k].clear();
					}
					extracted_images.clear();
					// map: key - unique instance number, value - slice position
					std::map<unsigned int, long long> slices_pos_map2;
					// list of all slice positions
					std::list<long long> slices_pos_list2;
					for (std::map<unsigned int, SliceInstance>::const_iterator it =
							slice_pos_map.cbegin();
						it != slice_pos_map.cend();
						++it)
					{
						slices_pos_map2[it->second.instance_number] =
							it->second.slice_position;
						slices_pos_list2.push_back(it->second.slice_position);
					}
					// find unique slice positions
					slices_pos_list2.sort();
					slices_pos_list2.unique();
					const size_t unique_slice_pos_size =
						slices_pos_list2.size();
					unsigned int g{};
					// assign id for every unique slice postion
					std::map<unsigned int, long long> slices_pos_ids;
					for (std::list<long long>::const_iterator it =
							slices_pos_list2.cbegin();
						it != slices_pos_list2.cend(); ++it)
					{
						slices_pos_ids[g]=*it;
						++g;
					}
					// find instance numbers for every unique slice postion
					std::vector< std::vector<unsigned int> > slices_;
					for (std::map<unsigned int, long long>::const_iterator it =
						slices_pos_ids.cbegin();
						it != slices_pos_ids.cend();
						++it)
					{
						std::vector<unsigned int> tmp_list;
						for (std::map<unsigned int, long long>::const_iterator it2 =
								slices_pos_map2.cbegin();
							it2 != slices_pos_map2.cend();
							++it2)
						{
							if (it->second == it2->second)
							{
								tmp_list.push_back(it2->first);
							}
						}
						slices_.push_back(std::move(tmp_list));
					}
					if (images.size() % unique_slice_pos_size != 0)
					{
						multiseries = false;
					}
					else
					{
						for (unsigned int j = 0; j < images.size() / unique_slice_pos_size; ++j)
						{
							QStringList images_tmp;
							for (unsigned int k = 0; k < unique_slice_pos_size; ++k)
							{
								if (k < slices_.size() && j < slices_.at(k).size())
								{
									const unsigned int id_0_ = slices_.at(k).at(j);
									if (slices_instance_map.count(id_0_) > 0)
									{
										const unsigned int id_1_ =
											slices_instance_map[slices_.at(k).at(j)];
										if (id_1_<static_cast<unsigned int>(images.size()))
										{
											images_tmp.push_back(images.at(id_1_));
										}
										else
										{
											multiseries = false;
											break;
										}
									}
									else
									{
										multiseries = false;
										break;
									}
								}
								else
								{
									multiseries = false;
									break;
								}
							}
							extracted_images.push_back(images_tmp);
						}
					}
					slices_pos_map2.clear();
					slices_pos_list2.clear();
					for (unsigned int j = 0; j < slices_.size(); ++j)
					{
						slices_[j].clear();
					}
					slices_.clear();
				}
				slices_instance_nums.clear();
				slices_instance_map.clear();
			}
			else
			{
				// can not process without instance ids
				multiseries = false;
			}
		}
	}
	//
#ifndef ALIZA_LOAD_DCM_THREAD
	QApplication::processEvents();
#endif
	//
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_dicom() 2");
#endif
	if (ultrasound)
	{
		// TODO check PR
		for (int x = 0; x < images.size(); ++x)
		{
			QStringList images_tmp;
			images_tmp << images.at(x);
			ImageVariant * ivariant = new ImageVariant(
				CommonUtils::get_next_id(),
				false,
				true,
				nullptr,
				0);
			message_ = read_ultrasound(
				&ok, load_type,
				ivariant,
				images_tmp,
				settings);
			if (ok)
			{
				ivariant->filenames = std::move(images_tmp);
				ivariants.push_back(ivariant);
			}
			else
			{
				delete ivariant;
			}
		}
	}
	else if (nuclear)
	{
		// TODO check PR
		for (int x = 0; x < images.size(); ++x)
		{
			QStringList images_tmp;
			images_tmp << images.at(x);
			ImageVariant * ivariant = new ImageVariant(
				CommonUtils::get_next_id(),
				false,
				true,
				nullptr,
				0);
			message_ = read_nuclear(
				&ok, load_type,
				ivariant,
				images_tmp,
				false,
				settings);
			if (ok)
			{
				ivariant->filenames = std::move(images_tmp);
				ivariants.push_back(ivariant);
			}
			else
			{
				delete ivariant;
			}
		}
	}
	else if (skip_volume)
	{
		for (int x = 0; x < images.size(); ++x)
		{
			QStringList images_tmp;
			images_tmp << images.at(x);
			{
				// don't load OpenGL
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					false,
					true,
					nullptr,
					0);
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					false,
					ivariant,
					images_tmp,
					false,
					settings,
					tolerance,
					(load_type == 1) ? false : true);
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
		}
	}
	else if (multiseries && (load_type == 0))
	{
		for (int k = 0; k < extracted_images.size(); ++k)
		{
			std::vector<QString> images__;
			std::vector<QString> images_ipp;
			for (int j = 0; j < extracted_images.at(k).size(); ++j)
			{
				images__.push_back(extracted_images.at(k).at(j));
			}
			if (images__.size() > 1)
			{
				sort_dicom_files_ippiop(images__, images_ipp);
			}
			else if (images__.size() == 1)
			{
				images_ipp.push_back(images__.at(0));
			}
			QStringList images_tmp;
			for (size_t j = 0; j < images_ipp.size(); ++j)
			{
				images_tmp << images_ipp.at(j);
			}
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					nullptr,
					0);
				ivariant->di->filtering = wsettings->get_filtering();
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					elscint,
					ivariant,
					images_tmp,
					ok3d,
					settings,
					tolerance,
					true);
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
		}
	}
	else if (enhanced)
	{
		for (int x = 0; x < images.size(); ++x)
		{
			if (load_type == 0||load_type == 2)
			{
				bool supp_palette_failed = false;
				if (supp_palette)
				{
					std::vector<ImageVariant*> supp_color_images;
					message_ = read_enhanced_supp_palette(
						&ok,
						images.at(x),
						supp_color_images,
						false,
						false,
						enh_loading_type,
						settings,
						tolerance);
					if (!message_.isEmpty() || !ok)
					{
						for (unsigned int jjj = 0; jjj < supp_color_images.size(); ++jjj)
						{
							delete supp_color_images[jjj];
						}
						supp_color_images.clear();
						supp_palette_failed = true;
					}
					else
					{
						std::vector<ImageVariant*> supp_grey_images;
						message_ = read_enhanced(
							&ok,
							images.at(x),
							supp_grey_images,
							ok3d,
							false,
							enh_loading_type,
							settings,
							tolerance,
							true);
						if (!message_.isEmpty() ||
							!ok ||
							(supp_color_images.size() != supp_grey_images.size()))
						{
							for (unsigned int jjj = 0; jjj < supp_color_images.size(); ++jjj)
							{
								delete supp_color_images[jjj];
							}
							supp_color_images.clear();
							for (unsigned int jjj = 0; jjj < supp_grey_images.size(); ++jjj)
							{
								delete supp_grey_images[jjj];
							}
							supp_grey_images.clear();
							supp_palette_failed = true;
						}
						if (!supp_palette_failed)
						{
							// load 2 separate images for debug
#if 0
							for (unsigned int jjj = 0; jjj < supp_color_images.size(); ++jjj)
							{
								ivariants.push_back(supp_color_images[jjj]);
							}
							for (unsigned int jjj = 0; jjj < supp_grey_images.size(); ++jjj)
							{
								ivariants.push_back(supp_grey_images[jjj]);
							}
#else
							for (unsigned int jjj = 0; jjj < supp_color_images.size(); ++jjj)
							{
								if (!(
									(supp_grey_images.at(jjj)->di->idimx == supp_color_images.at(jjj)->di->idimx) &&
									(supp_grey_images.at(jjj)->di->idimy == supp_color_images.at(jjj)->di->idimy) &&
									(supp_grey_images.at(jjj)->di->idimz == supp_color_images.at(jjj)->di->idimz)))
								{
									for (unsigned int zzz = 0; zzz < supp_color_images.size(); ++zzz)
									{
										delete supp_color_images[zzz];
									}
									for (unsigned int zzz = 0; zzz < supp_grey_images.size(); ++zzz)
									{
										delete supp_grey_images[zzz];
									}
									return QString("Supplemental LUT failed,\ninternal error");
								}
								CommonUtils::calculate_minmax_scalar(supp_grey_images[jjj]);
								QString supp_palette_error;
								ImageVariant * v = new ImageVariant(
									CommonUtils::get_next_id(), ok3d, true, nullptr, 0);
								if (supp_color_images.at(jjj)->image_type == 11)
								{
									v->pUS_rgb = RGBImageTypeUS::New();
									v->image_type = 11;
									supp_palette_error = supp_palette_grey_to_rgbUS(
										v->pUS_rgb,
										supp_color_images[jjj]->pUS_rgb,
										supp_color_images.at(jjj)->di->supp_palette_subsciptor,
										supp_grey_images[jjj]);
								}
								else if (supp_color_images.at(jjj)->image_type == 14)
								{
									v->pUC_rgb = RGBImageTypeUC::New();
									v->image_type = 14;
									supp_palette_error = supp_palette_grey_to_rgbUC(
										v->pUC_rgb,
										supp_color_images[jjj]->pUC_rgb,
										supp_color_images.at(jjj)->di->supp_palette_subsciptor,
										supp_grey_images[jjj]);
								}
								else
								{
									for (unsigned int zzz = 0; zzz < supp_color_images.size(); ++zzz)
									{
										delete supp_color_images[zzz];
									}
									for (unsigned int zzz = 0; zzz < supp_grey_images.size(); ++zzz)
									{
										delete supp_grey_images[zzz];
									}
									return QString("Supplemental LUT failed, wrong image type");
								}
								if (!supp_palette_error.isEmpty())
								{
									for (unsigned int zzz = 0; zzz < supp_color_images.size(); ++zzz)
									{
										delete supp_color_images[zzz];
									}
									for (unsigned int zzz = 0;
										zzz < supp_grey_images.size();
										++zzz)
									{
										delete supp_grey_images[zzz];
									}
									delete v;
									return QString("Internal error, ") + supp_palette_error;
								}
								CommonUtils::copy_slices(v, supp_grey_images.at(jjj));
								CommonUtils::reload_rgb_rgba(v);
								if (v->equi)
								{
									if (v->di->idimz < 7) v->di->transparency = false;
								}
								else
								{
									if (!v->one_direction) v->di->transparency = false;
									v->di->filtering = 0;
								}
								CommonUtils::reset_bb(v);
								v->filenames = supp_color_images.at(jjj)->filenames;
								ivariants.push_back(v);
								delete supp_grey_images[jjj];
								supp_grey_images[jjj] = nullptr;
								delete supp_color_images[jjj];
								supp_color_images[jjj] = nullptr;
							}
						}
					}
#endif
				}
				if (!supp_palette || supp_palette_failed)
				{
					message_ = read_enhanced(
						&ok,
						images.at(x),
						ivariants,
						ok3d,
						false,
						enh_loading_type,
						settings,
						tolerance,
						true);
				}
			}
			else
			{
				message_ = read_enhanced(
					&ok,
					images.at(x),
					ivariants,
					false,
					false,
					enh_loading_type,
					settings,
					tolerance,
					(load_type == 1) ? false : true);
			}
		}
	}
	else if (mosaic && (load_type == 0))
	{
		// TODO
		for (int x = 0; x < images.size(); ++x)
		{
			QStringList images_tmp;
			images_tmp << images.at(x);
			ImageVariant * ivariant = new ImageVariant(
				CommonUtils::get_next_id(),
				ok3d,
				!wsettings->get_3d(),
				nullptr,
				0);
			ivariant->di->filtering = wsettings->get_filtering();
			message_ = read_series(
				&ok,
				false,
				true,
				false,
				false,
				ivariant,
				images_tmp,
				ok3d,
				settings,
				tolerance,
				true);
			if (ok)
			{
				ivariant->filenames = std::move(images_tmp);
				ivariants.push_back(ivariant);
			}
			else
			{
				delete ivariant;
			}
		}
	}
	else if (uihgrid && (load_type == 0))
	{
		// TODO
		for (int x = 0; x < images.size(); ++x)
		{
			QStringList images_tmp;
			images_tmp << images.at(x);
			ImageVariant * ivariant = new ImageVariant(
				CommonUtils::get_next_id(),
				ok3d,
				!wsettings->get_3d(),
				nullptr,
				0);
			ivariant->di->filtering = wsettings->get_filtering();
			message_ = read_series(
				&ok,
				false,
				false,
				true,
				false,
				ivariant,
				images_tmp,
				ok3d,
				settings,
				tolerance,
				true);
			if (ok)
			{
				ivariant->filenames = std::move(images_tmp);
				ivariants.push_back(ivariant);
			}
			else
			{
				delete ivariant;
			}
		}
	}
	else if (multiframe)
	{
		for (int x = 0; x < images.size(); ++x)
		{
			QStringList images_tmp;
			images_tmp << images.at(x);
			if (load_type == 0||load_type == 2)
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					nullptr,
					0);
				ivariant->di->filtering = wsettings->get_filtering();
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					false,
					ivariant,
					images_tmp,
					ok3d,
					settings,
					tolerance,
					true);
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
			else
			{
				// don't load OpenGL for intermediate image
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					false,
					true,
					nullptr,
					0);
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					false,
					ivariant,
					images_tmp,
					false,
					settings,
					tolerance,
					(load_type == 3));
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
		}
	}
	else if (mixed)
	{
		const mdcm::Tag tt(0x0008,0x0008);
		const mdcm::Tag sc(0x0008,0x0016);
		const mdcm::Tag ph(0x0028,0x0004);
		const mdcm::Tag tr(0x0028,0x0010);
		const mdcm::Tag tc(0x0028,0x0011);
		const mdcm::Tag ps(0x0028,0x0030);
		const mdcm::Tag ta(0x0028,0x0100);
		const mdcm::Tag cc(0x0028,0x2000);
		std::vector<MixedDicomSeriesInfo> msi;
		for (int x = 0; x < images.size(); ++x)
		{
			MixedDicomSeriesInfo si;
			si.file = QString(images.at(x));
			mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
			reader.SetFileName(QDir::toNativeSeparators(images.at(x)).toUtf8().constData());
#else
			reader.SetFileName(QDir::toNativeSeparators(images.at(x)).toLocal8Bit().constData());
#endif
#else
			reader.SetFileName(images.at(x).toLocal8Bit().constData());
#endif
			if (!reader.ReadUpToTag(mdcm::Tag(0x0028,0x2000))) continue;
#ifndef ALIZA_LOAD_DCM_THREAD
			QApplication::processEvents();
#endif
			const mdcm::File    & file = reader.GetFile();
			const mdcm::DataSet & ds   = file.GetDataSet();
			if (ds.IsEmpty()) continue;
			unsigned short r{};
			unsigned short c{};
			unsigned short a{};
			if (get_us_value(ds, tr, &r))
			{
				si.rows = r;
			}
			if (get_us_value(ds, tc, &c))
			{
				si.columns = c;
			}
			if (get_us_value(ds, ta, &a))
			{
				si.allocated = a;
			}
			{
				QString s;
				if (get_string_value(ds, tt, s))
				{
					if (s.toUpper().contains(QString("LOCALIZER")))
						si.localizer = true;
				}
			}
			{
				QString s1;
				get_string_value(ds, ph, s1);
				si.photometric = s1.trimmed().remove(QChar('\0'));
			}
			{
				QString s2;
				get_string_value(ds, sc, s2);
				si.sop = s2.trimmed().remove(QChar('\0'));
			}
			{
				QString s3;
				std::vector<double> v3;
				if (get_ds_values(ds, ps, v3))
				{
					if (v3.size() == 2)
					{
						s3 = QString::asprintf("%.4f-%.4f", v3.at(0), v3.at(1));
					}
				}
				si.spacing = s3;
			}
			if (ds.FindDataElement(cc))
			{
				si.icc = true;
			}
			msi.push_back(si);
		}
		QMultiMap<QString, QString> l0;
		for (size_t x = 0; x < msi.size(); ++x)
		{
			const MixedDicomSeriesInfo & i = msi.at(x);
			if (i.rows != -1 && i.columns != -1 && i.allocated > 0)
			{
				const QString k1 =
					QString::number(i.rows) +
					QString("x") +
					QString::number(i.columns) +
					QString("x") +
					QString::number(static_cast<int>(i.allocated)) +
					(i.localizer ? QString("L") : QString("")) +
					QString("-") + i.sop +
					QString("-") + i.photometric + QString("-") +
					QString("-") + i.spacing + QString("-") +
					(i.icc ? QString("icc") : QString(""));
				l0.insert(k1, i.file);
			}
		}
		QList<QStringList> fff;
		const QList<QString> & l1 = l0.keys();
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		const QSet<QString> s1 =
			(l1.empty())
			?
			QSet<QString>()
			:
			QSet<QString>(l1.begin(), l1.end());
#else
		const QSet<QString> s1 = l1.toSet();
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		QSet<QString>::const_iterator it1 = s1.cbegin();
		while (it1 != s1.cend())
#else
		QSet<QString>::const_iterator it1 = s1.constBegin();
		while (it1 != s1.constEnd())
#endif
		{
#if 0
			std::cout << (*it1).toStdString() << std::endl;
#endif
			QStringList ff;
			const QList<QString> & q = l0.values(*it1);
			for (int y = 0; y < q.size(); ++y)
			{
				ff.push_back(q.at(y));
#if 0
				std::cout << q.at(y).toStdString() << std::endl;
#endif
			}
			fff.push_back(ff);
			++it1;
		}
		for (int x = 0; x < fff.size(); ++x)
		{
			std::vector<QString> images__;
			std::vector<QString> images_ipp;
			for (int k = 0; k < fff.at(x).size(); ++k)
			{
				images__.push_back(fff.at(x).at(k));
			}
			if (images__.size() > 1)
			{
				sort_dicom_files_ippiop(images__, images_ipp);
			}
			else if (images__.size() == 1)
			{
				images_ipp.push_back(images__.at(0));
			}
			QStringList images_tmp;
			for (size_t j = 0; j < images_ipp.size(); ++j)
			{
				images_tmp << images_ipp.at(j);
			}
			if (load_type == 0 || load_type == 2)
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					nullptr,
					0);
				ivariant->di->filtering = wsettings->get_filtering();
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					elscint,
					ivariant,
					images_tmp,
					ok3d,
					settings,
					tolerance,
					true);
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
			else
			{
				// don't load OpenGL for intermediate image
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					false,
					true,
					nullptr,
					0);
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					elscint,
					ivariant,
					images_tmp,
					false,
					settings,
					tolerance,
					(load_type == 3));
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
		}
	}
	else // usual series
	{
		if (!images.empty())
		{
			std::vector<QString> images__;
			std::vector<QString> images_ipp;
			for (int k = 0; k < images.size(); ++k)
			{
				images__.push_back(images.at(k));
			}
			if (images__.size() > 1)
			{
				sort_dicom_files_ippiop(images__, images_ipp);
			}
			else if (images__.size() == 1)
			{
				images_ipp.push_back(images__.at(0));
			}
			QStringList images_tmp;
			for (size_t j = 0; j < images_ipp.size(); ++j)
			{
				images_tmp << images_ipp.at(j);
			}
			if (load_type == 0 || load_type == 2)
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					nullptr,
					0);
				ivariant->di->filtering = wsettings->get_filtering();
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					elscint,
					ivariant,
					images_tmp,
					ok3d,
					settings,
					tolerance,
					true);
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					{
						QList<QString> l_uids =
							ivariant->image_instance_uids.values();
						if (!l_uids.empty())
						{
							const size_t l_size = l_uids.size();
							if (l_size > 1)
							{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
								QSet<QString> s_uids =
									QSet<QString>(l_uids.begin(), l_uids.end());
#else
								QSet<QString> s_uids = l_uids.toSet();
#endif
								const size_t s_size = s_uids.size();
								if (l_size > s_size) ++count_uid_errors;
							}
						}
					}
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
			else
			{
				// don't load OpenGL for intermediate image
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					false,
					true,
					nullptr,
					0);
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					elscint,
					ivariant,
					images_tmp,
					false,
					settings,
					tolerance,
					(load_type == 3));
				if (ok)
				{
					ivariant->filenames = std::move(images_tmp);
					//
					{
						QList<QString> l_uids =
							ivariant->image_instance_uids.values();
						if (!l_uids.empty())
						{
							const size_t l_size = l_uids.size();
							if (l_size > 1)
							{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
								QSet<QString> s_uids =
									QSet<QString>(l_uids.begin(), l_uids.end());
#else
								QSet<QString> s_uids = l_uids.toSet();
#endif
								const size_t s_size = s_uids.size();
								if (l_size > s_size) ++count_uid_errors;
							}
						}
					}
					//
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
		}
	}
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_dicom() 3");
#endif
	if (count_uid_errors > 0)
	{
		if (!message_.isEmpty()) message_.append(QChar('\n'));
		message_.append(
			QString("Warning: UIDs are not unique (") +
			QVariant(count_uid_errors).toString() +
			QString(")"));
	}
	//
	if (load_type != 0) return message_;
	//
	if (!rtstruct_ref_search.empty())
	{
		for (long x = 0; x < rtstruct_ref_search.size(); ++x)
		{
			std::vector<ImageVariant*> tmp_ivariants_rtstruct;
			const bool ref_ok =
				process_contrours_ref(
					rtstruct_ref_search.at(x),
					rtstruct_ref_search_path,
					tmp_ivariants_rtstruct,
					ok3d,
					1, // force sorted uniform
					settings);
			bool ref2_ok{};
			if (ref_ok)
			{
				for (unsigned int y = 0;
					y < tmp_ivariants_rtstruct.size();
					++y)
				{
					tmp_ivariants_rtstruct[y]->filenames =
						QStringList(rtstruct_ref_search.at(x));
					ivariants.push_back(tmp_ivariants_rtstruct[y]);
				}
			}
			else
			{
				if (!root.isEmpty())
				{
					ref2_ok = process_contrours_ref(
						rtstruct_ref_search.at(x),
						root,
						tmp_ivariants_rtstruct,
						ok3d,
						enh_loading_type,
						settings);
				}
				else
				{
					// Try to go one directory up
					QFileInfo fi5(rtstruct_ref_search_path + QString("/.."));
					if (fi5.exists())
					{
						ref2_ok = process_contrours_ref(
							rtstruct_ref_search.at(x),
							QDir::toNativeSeparators(fi5.absoluteFilePath()),
							tmp_ivariants_rtstruct,
							ok3d,
							enh_loading_type,
							settings);
					}
				}
				if (ref2_ok)
				{
					for (unsigned int y = 0;
						y < tmp_ivariants_rtstruct.size();
						++y)
					{
						tmp_ivariants_rtstruct[y]->filenames =
							QStringList(rtstruct_ref_search.at(x));
						ivariants.push_back(tmp_ivariants_rtstruct[y]);
					}
				}
			}
			if (!(ref_ok || ref2_ok))
			{
				if (!message_.isEmpty()) message_.append(QChar('\n'));
				message_.append(QString(
					"The series referenced in the RTSTRUCT "
					"could not be found. Try using the DICOM scanner from a "
					"folder containing both the RTSTRUCT and the referenced series."));
				mdcm::Reader reader;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
				reader.SetFileName(QDir::toNativeSeparators(rtstruct_ref_search.at(x)).toUtf8().constData());
#else
				reader.SetFileName(QDir::toNativeSeparators(rtstruct_ref_search.at(x)).toLocal8Bit().constData());
#endif
#else
				reader.SetFileName(rtstruct_ref_search.at(x).toLocal8Bit().constData());
#endif
				const bool str_f_ok = reader.Read();
#ifndef ALIZA_LOAD_DCM_THREAD
				QApplication::processEvents();
#endif
				if (str_f_ok)
				{
					ImageVariant * ivariant = new ImageVariant(
						CommonUtils::get_next_id(),
						ok3d, !wsettings->get_3d(), nullptr, 0);
					ivariant->filenames = QStringList(rtstruct_ref_search.at(x));
					const mdcm::File & file = reader.GetFile();
					const mdcm::DataSet & ds = file.GetDataSet();
					load_contour(ds, ivariant);
					ContourUtils::calculate_rois_center(ivariant);
					ivariants.push_back(ivariant);
				}
			}
		}
	}
	//
	if (!grey_softcopy_pr_files.empty())
	{
		const QString file0 = grey_softcopy_pr_files.at(0);
		QFileInfo p0(file0);
		QString message_pr;
		QString root_tmp;
		if (root.isEmpty())
		{
			root_tmp = p0.absolutePath();
		}
		else
		{
			root_tmp = root;
		}
		unsigned int count = process_gsps(
			grey_softcopy_pr_files,
			root_tmp,
			wsettings,
			ok3d,
			ivariants,
			message_pr);
		if (root.isEmpty() && count < 1 && message_pr.isEmpty())
		{
			// Try to go one directory up
			QFileInfo fi5(root_tmp + QString("/.."));
			if (fi5.exists())
			{
				count = process_gsps(
					grey_softcopy_pr_files,
					QDir::toNativeSeparators(fi5.absoluteFilePath()),
					wsettings,
					ok3d,
					ivariants,
					message_pr);
			}
		}
		if (!message_pr.isEmpty())
		{
			if (!message_.isEmpty()) message_.append(QChar('\n'));
			message_.append(message_pr);
		}
		if (count < 1)
		{
			if (!message_.isEmpty()) message_.append(QChar('\n'));
			message_.append(QString(
				"The series referenced in the Grayscale Soft Copy presentation "
				"could not be found or opened. Try using the DICOM scanner from a "
				"folder containing both the GSPS series and the referenced series."));
		}
	}
	if (!color_softcopy_pr_files.empty()        ||
		!pseudo_color_softcopy_pr_files.empty() ||
		!blending_softcopy_pr_files.empty()     ||
		!xaxrf_softcopy_pr_files.empty()        ||
		!advanced_blending_softcopy_pr_files.empty())
	{
		QString pr_warn;
		if (!color_softcopy_pr_files.empty())
			pr_warn += QString("Color Softcopy Presentation ");
		if (!pseudo_color_softcopy_pr_files.empty())
			pr_warn += QString("Pseudo-color Softcopy Presentation ");
		if (!blending_softcopy_pr_files.empty())
			pr_warn += QString("Blending Softcopy Presentation ");
		if (!xaxrf_softcopy_pr_files.empty())
			pr_warn += QString("XA/XRF Grayscale Softcopy Presentation ");
		if (!advanced_blending_softcopy_pr_files.empty())
			pr_warn += QString("Advanced Blending Softcopy Presentation ");
		pr_warn += QString("is currently not supported");
		if (!pr_warn.isEmpty())
		{
			if (message_.isEmpty()) message_.append(QChar('\n'));
			message_.append(pr_warn);
		}
	}
	//
#if 0
	for (size_t x = 0; x < ivariants.size(); ++x)
	{
		CommonUtils::get_reference_count(ivariants.at(x));
	}
#endif
#ifdef ALIZA_LINUX_DEBUG_MEM
	CommonUtils::linux_print_memusage("read_dicom() end");
#endif
	return message_;
}

#ifdef ENHANCED_PRINT_INFO
#undef ENHANCED_PRINT_INFO
#endif

#ifdef WARN_RAM_SIZE
#undef WARN_RAM_SIZE
#endif

