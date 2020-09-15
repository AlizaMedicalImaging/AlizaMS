//#define ENHANCED_PRINT_INFO

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include "CG/glwidget-qt5.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#include "structures.h"
#include "dicomutils.h"
#include "commonutils.h"
#include "codecutils.h"
#include "contourutils.h"
#include "prconfigutils.h"
#include "srutils.h"
#include "ultrasoundregiondata.h"
#include "ultrasoundregionutils.h"
#include "spectroscopydata.h"
#include "spectroscopyutils.h"
#include <itkImageSliceIteratorWithIndex.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include <itkByteSwapper.h>
#include <itkMath.h>
#include "mdcmReader.h"
#include "mdcmFile.h"
#include "mdcmAttribute.h"
#include "mdcmVersion.h"
#include "mdcmGlobal.h"
#include "mdcmDicts.h"
#include "mdcmDict.h"
#include "mdcmUIDGenerator.h"
#include "mdcmImageWriter.h"
#include "mdcmImageReader.h"
#include "mdcmImage.h"
#include "mdcmElement.h"
#include "mdcmRescaler.h"
#include "mdcmImageChangePlanarConfiguration.h"
#include "mdcmImageApplyLookupTable.h"
#include "mdcmApplySupplementalLUT.h"
#include "mdcmImageHelper.h"
#include "mdcmOverlay.h"
#include "mdcmSplitMosaicFilter.h"
#include "mdcmDataElement.h"
#include "mdcmScanner.h"
#include "mdcmCSAHeader.h"
#include "mdcmVM.h"
#include "mdcmVR.h"
#include "mdcmUIDs.h"
#include "splituihgridfilter.h"
#include <QSet>
#include <QTextCodec>
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include "settingswidget.h"
#include "iconutils.h"
#include "updateqtcommand.h"
#include "findrefdialog.h"
#include "srwidget.h"
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <set>
#include <algorithm>

#include "vectormath/scalar/vectormath.h"
typedef Vectormath::Scalar::Vector3 sVector3;
typedef Vectormath::Scalar::Vector4 sVector4;
typedef Vectormath::Scalar::Matrix4 sMatrix4;

namespace mdcm
{
class Sorter2
{
public:
	Sorter2();
	virtual ~Sorter2();
	const std::vector<QString> & GetFilenames() const;
	typedef bool (*SortFunction)(DataSet const &, DataSet const &);
	void SetSortFunction(SortFunction);
	virtual bool StableSort(std::vector<QString> const &);
protected:
	std::vector<QString> Filenames;
	SortFunction SortFunc;
};

namespace {
class SortFunctor2
{
public:
	bool operator() (File const *file1, File const *file2)
	{
		return (SortFunc)(file1->GetDataSet(), file2->GetDataSet());
	}
	Sorter2::SortFunction SortFunc;
	SortFunctor2()
	{
		SortFunc = 0;
	}
	SortFunctor2(SortFunctor2 const & sf)
	{
		SortFunc = sf.SortFunc;
	}
	void operator=(Sorter2::SortFunction sf)
	{
		SortFunc = sf;
	}
};
}

class FileWithQString : public File
{
public:
	FileWithQString(File & f) : File(f), filename(QString("")) {}
	QString filename;
};

Sorter2::Sorter2() { SortFunc = 0; }

Sorter2::~Sorter2() {}

const std::vector<QString> & Sorter2::GetFilenames() const
{
	return Filenames;
}

void Sorter2::SetSortFunction(SortFunction f)
{
	SortFunc = f;
}

bool Sorter2::StableSort(
	std::vector<QString> const & filenames)
{
	if(filenames.empty() || !SortFunc)
	{
		Filenames.clear();
		return true;
	}
	std::set<mdcm::Tag> tags;
	tags.insert(mdcm::Tag(0x0020,0x0032));
	tags.insert(mdcm::Tag(0x0020,0x0037));
	std::vector< SmartPointer<FileWithQString> > filelist;
	filelist.resize(filenames.size());
	std::vector< SmartPointer<FileWithQString> >::iterator it2 =
		filelist.begin();
	for(
		std::vector<QString>::const_iterator it =
			filenames.begin();
		it != filenames.end() && it2 != filelist.end();
		++it, ++it2)
	{
		Reader reader;
		reader.SetFileName(it->toLocal8Bit().constData());
		SmartPointer<FileWithQString> & f = *it2;
		if (reader.ReadSelectedTags(tags))
		{
			f = new FileWithQString(reader.GetFile());
			f->filename = *it;
		}
		else { return false; }
	}
	SortFunctor2 sf;
	sf = Sorter2::SortFunc;
	std::stable_sort(filelist.begin(), filelist.end(), sf);
	Filenames.clear();
	for(it2 = filelist.begin(); it2 != filelist.end(); ++it2 )
	{
		SmartPointer<FileWithQString> const & f = *it2;
		Filenames.push_back(f->filename);
	}
	return true;
}

} // mdcm

static bool sort0_(
	mdcm::DataSet const & ds1,
	mdcm::DataSet const & ds2)
{
	// IPP sorting
	const mdcm::Tag t1(0x0020,0x0032);
	if(!ds1.FindDataElement(t1)||!ds2.FindDataElement(t1))
		return false;
	const mdcm::Tag t2(0x0020,0x0037);
	if(!ds1.FindDataElement(t2)||!ds2.FindDataElement(t2))
		return false;
	mdcm::Attribute<0x0020,0x0032> ipp1; ipp1.Set(ds1);
	mdcm::Attribute<0x0020,0x0037> iop1; iop1.Set(ds1);
	mdcm::Attribute<0x0020,0x0032> ipp2; ipp2.Set(ds2);
	mdcm::Attribute<0x0020,0x0037> iop2; iop2.Set(ds2);
	if (ipp1.GetNumberOfValues()<3||ipp2.GetNumberOfValues()<3)
		return false;
	if (iop1.GetNumberOfValues()<6||iop2.GetNumberOfValues()<6)
		return false;
	const double t = 0.001;
	if (!(
		((iop1[0] + t) > iop2[0]) && ((iop1[0] - t) < iop2[0]) &&
		((iop1[1] + t) > iop2[1]) && ((iop1[1] - t) < iop2[1]) &&
		((iop1[2] + t) > iop2[2]) && ((iop1[2] - t) < iop2[2]) &&
		((iop1[3] + t) > iop2[3]) && ((iop1[3] - t) < iop2[3]) &&
		((iop1[4] + t) > iop2[4]) && ((iop1[4] - t) < iop2[4]) &&
		((iop1[5] + t) > iop2[5]) && ((iop1[5] - t) < iop2[5])))
		return false;
	double normal[3];
	normal[0] = iop1[1]*iop1[5] - iop1[2]*iop1[4];
	normal[1] = iop1[2]*iop1[3] - iop1[0]*iop1[5];
	normal[2] = iop1[0]*iop1[4] - iop1[1]*iop1[3];
	double dist1 = 0, dist2 = 0;
	for (int i = 0; i < 3; ++i) dist1 += normal[i]*ipp1[i];
	for (int i = 0; i < 3; ++i) dist2 += normal[i]*ipp2[i];
	return (dist1 < dist2);
}

static QString generate_string_0(
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

static QString read_MRImageModule(const mdcm::DataSet & ds)
{
	QString s("");
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
			const QStringList tmp0 =
				tmp2.split(
					QString("\\"), QString::SkipEmptyParts);
			const int tmp0_size = tmp0.size();
			s += QString("<span class='y8'>");
			for (int x = 0; x < tmp0_size; x++)
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
				const QString tmp2 = SequenceVariant.trimmed();
				if (!tmp2.isEmpty())
				{
					s += QString("<span class='y9'>Variant</span><br />");
					{
						const QStringList tmp0 =
							tmp2.split(
								QString("\\"), QString::SkipEmptyParts);
						const int tmp0_size = tmp0.size();
						s += QString("<span class='y8'>");
						for (int x = 0; x < tmp0_size; x++)
						{
							const QString tmp1 = tmp0.at(x).trimmed();
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
							if (x != tmp0_size - 1) s += QString("<br />");
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
				const QString tmp2 = ScanOptions.trimmed();
				if (!tmp2.isEmpty())
				{
					s += QString("<span class='y9'>Options</span><br />");
					{
						const QStringList tmp0 =
							tmp2.split(
								QString("\\"), QString::SkipEmptyParts);
						const int tmp0_size = tmp0.size();
						s += QString("<span class='y8'>");
						for (int x = 0; x < tmp0_size; x++)
						{
							const QString tmp1 = tmp0.at(x).trimmed();
							if (tmp1 == QString("PER"))
								s += QString("Phase Encode Reordering");
							else if (tmp1 == QString("RG"))
								s += QString("Respiratory Gating");
							else if (tmp1 == QString("CG"))
								s += QString("Cardiac Gating");
							else if (tmp1 == QString("PPG"))
								s += QString("Peripheral 1.2.826.0.1Pulse Gating");
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
							if (x != tmp0_size - 1) s += QString("<br />");
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
#if 0
	const mdcm::Tag tImagingFrequency(0x0018,0x0084);
	s += generate_string_0(
		ds,
		tImagingFrequency,
		QString("Imaging Frequency"),
		QString("MHz"));
#endif
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
			QVariant((double)fB1rms).toString();
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

static QString read_CTImageModule(const mdcm::DataSet & ds)
{
	QString s("");

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
		for (size_t x = 0; x < DataCollectionCenterPatient.size(); x++)
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
		for (size_t x = 0; x < ReconstructionTargetCenterPatient.size(); x++)
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
			for (size_t x = 0; x < FocalSpots.size(); x++)
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
			for (size_t x = 0; x < IsocenterPosition.size(); x++)
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

static QString read_CommonCTMRImageDescriptionMacro(const mdcm::DataSet & ds)
{
	QString s("");
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

template <typename T, long long TVR>
bool get_vm1_bin_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	T * result)
{
	if (!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() ||
		v.IsUndefinedLength() ||
		!v.GetByteValue()) return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (
		tvr_ != static_cast<long long>(vr) &&
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
	if (!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() ||
		v.IsUndefinedLength() ||
		!v.GetByteValue()) return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (
		tvr_ != static_cast<long long>(vr) &&
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
	if (!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() ||
		v.IsUndefinedLength() ||
		!v.GetByteValue()) return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (
		tvr_ != static_cast<long long>(vr) &&
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
	for (unsigned int x = 0; x < l; x++)
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
	if (!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & v = ds.GetDataElement(t);
	if (v.IsEmpty() ||
		v.IsUndefinedLength() ||
		!v.GetByteValue()) return false;
#if 0
	const mdcm::VR vr = v.GetVR();
	const long long tvr_ = TVR;
	if (
		tvr_ != static_cast<long long>(vr) &&
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
	for (unsigned int x = 0; x < l; x++)
	{
		result.push_back(static_cast<T>(e.GetValue(x)));
	}
	return true;
}

QString DicomUtils::convert_pn_value(const QString & n)
{
	//family name, given name, middle name, name prefix, name suffix
	QString s("");
	const QStringList tmp1 = n.split(
		QString("="),
		QString::KeepEmptyParts);
	for (int x = 0; x < tmp1.size(); x++)
	{
		if (!s.isEmpty()) s += QString(" ");
		const QStringList tmp2 = tmp1.at(x).split(
			QString("^"),
			QString::KeepEmptyParts);
		const int tmp2_size = tmp2.size();
		if (tmp2_size == 1)
		{
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 2)
		{
			const QString s1 =
				tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" ");
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 3)
		{
			const QString s1 =
				tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" "); 
			const QString s2 =
				tmp2.at(2).trimmed().remove(QChar('\0'));
			s += s2;
			if (!s2.isEmpty()) s += QString(" ");
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 4)
		{
			const QString s3 =
				tmp2.at(3).trimmed().remove(QChar('\0'));
			s += s3;
			if (!s3.isEmpty()) s += QString(" ");
			const QString s1 =
				tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" ");
			const QString s2 =
				tmp2.at(2).trimmed().remove(QChar('\0'));
			s += s2;
			if (!s2.isEmpty()) s += QString(" ");
			s += tmp2.at(0).trimmed().remove(QChar('\0'));
		}
		else if (tmp2_size == 5)
		{
			const QString s3 =
				tmp2.at(3).trimmed().remove(QChar('\0'));
			s += s3;
			if (!s3.isEmpty()) s += QString(" ");
			const QString s1 =
				tmp2.at(1).trimmed().remove(QChar('\0'));
			s += s1;
			if (!s1.isEmpty()) s += QString(" ");
			const QString s2 =
				tmp2.at(2).trimmed().remove(QChar('\0'));
			s += s2;
			if (!s2.isEmpty()) s += QString(" ");
			const QString s0 =
				tmp2.at(0).trimmed().remove(QChar('\0'));
			s += s0;
			const QString s4 =
				tmp2.at(4).trimmed().remove(QChar('\0'));
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
	if (e.IsEmpty()||e.IsUndefinedLength()) return QString("");
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return QString("");
	const QByteArray ba(bv->GetPointer(), bv->GetLength());
	const QString tmp0 = CodecUtils::toUTF8(
		&ba,
		charset);
	return convert_pn_value(tmp0);
}

bool DicomUtils::get_us_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	unsigned short * result)
{
	const bool ok =
		get_vm1_bin_value<unsigned short, mdcm::VR::US>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_sl_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	int * result)
{
	const bool ok =
		get_vm1_bin_value<int, mdcm::VR::SL>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_ul_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	unsigned int * result)
{
	const bool ok =
		get_vm1_bin_value<unsigned int, mdcm::VR::UL>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_fd_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	double * result)
{
	const bool ok =
		get_vm1_bin_value<double, mdcm::VR::FD>(
			ds, t, result);
	return ok;
}

bool DicomUtils::priv_get_fd_value(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	double * result)
{
	const bool ok =
		get_priv_vm1_bin_value<double, mdcm::VR::FD>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_fl_value(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	float * result)
{
	const bool ok =
		get_vm1_bin_value<float, mdcm::VR::FL>(
			ds, t, result);
	return ok;
}

bool DicomUtils::priv_get_fl_value(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	float * result)
{
	const bool ok =
		get_priv_vm1_bin_value<float, mdcm::VR::FL>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_us_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<unsigned short> & result)
{
	const bool ok =
		get_vm1_n_bin_values<unsigned short, mdcm::VR::US>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_sl_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<int> & result)
{
	const bool ok =
		get_vm1_n_bin_values<int, mdcm::VR::SL>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_ul_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<unsigned int> & result)
{
	const bool ok =
		get_vm1_n_bin_values<unsigned int, mdcm::VR::UL>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_fd_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<double> & result)
{
	const bool ok =
		get_vm1_n_bin_values<double, mdcm::VR::FD>(
			ds, t, result);
	return ok;
}

bool DicomUtils::priv_get_fd_values(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	std::vector<double> & result)
{
	const bool ok =
		get_priv_vm1_n_bin_values<double, mdcm::VR::FD>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_fl_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<float> & result)
{
	const bool ok =
		get_vm1_n_bin_values<float, mdcm::VR::FL>(
			ds, t, result);
	return ok;
}

bool DicomUtils::get_ds_values(
	const mdcm::DataSet & ds,
	const mdcm::Tag & t,
	std::vector<double> & result)
{
	if(!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & e =
		ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	const QString tmp0 = QString::fromLatin1(
		bv->GetPointer(),
		bv->GetLength());
	const QStringList tmp1 = tmp0.split(
		QString("\\"),
		QString::SkipEmptyParts);
	if (tmp1.empty()) return false;
	for (int x = 0; x < tmp1.size(); x++)
	{
		bool ok = false;
		const double tmp3 =
			QVariant(
				tmp1.at(x).trimmed().
					remove(QChar('\0'))).
						toDouble(&ok);
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
	if(!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & e =
		ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	const QString tmp0 = QString::fromLatin1(
		bv->GetPointer(),
		bv->GetLength());
	const QStringList tmp1 = tmp0.split(
		QString("\\"),
		QString::SkipEmptyParts);
	if (tmp1.empty()) return false;
	for (int x = 0; x < tmp1.size(); x++)
	{
		bool ok = false;
		const double tmp3 =
			QVariant(
				tmp1.at(x).trimmed().
					remove(QChar('\0'))).
						toDouble(&ok);
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
	if (!ds.FindDataElement(t))
		return false;
	const mdcm::DataElement & e =
		ds.GetDataElement(t);
	if (e.IsEmpty() ||
		e.IsUndefinedLength() ||
		!e.GetByteValue())
		return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (bv)
	{
		const QString tmp0 = QString::fromLatin1(
			bv->GetPointer(),
			bv->GetLength()).trimmed().
				remove(QChar('\0'));
		bool ok = false;
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
	if(!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & e =
		ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	const QString tmp0 = QString::fromLatin1(
		bv->GetPointer(),
		bv->GetLength());
	const QStringList tmp1 = tmp0.split(
		QString("\\"),
		QString::SkipEmptyParts);
	if (tmp1.empty()) return false;
	for (int x = 0; x < tmp1.size(); x++)
	{
		bool ok = false;
		const int tmp3 = QVariant(
			tmp1.at(x).
				trimmed().
				remove(QChar('\0'))).toInt(&ok);
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
	if(!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & e =
		ds.GetDataElement(t);
	if (e.IsEmpty()) return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	if (4==bv->GetLength())
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
	if(!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty() || e.IsUndefinedLength())
		return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	s = QString::fromLatin1(
		bv->GetPointer(),
		bv->GetLength()).trimmed();
	return true;
}

bool DicomUtils::priv_get_string_value(
	const mdcm::DataSet & ds,
	const mdcm::PrivateTag & t,
	QString & s)
{
	if(!ds.FindDataElement(t)) return false;
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty() || e.IsUndefinedLength())
		return false;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (!bv) return false;
	s = QString::fromLatin1(
		bv->GetPointer(),
		bv->GetLength()).trimmed();
	return true;
}

QString DicomUtils::generate_id()
{
	char c[] = "\0\0\0\0\0\0\0\0\0\0\0\0";
    const char s[] =
    	"0123456789ABCDEFGHIJKLMNOPQRSTU"
		"VWXYZabcdefghijklmnopqrstuvwxyz";
	const unsigned int ss = sizeof(s);
	for (unsigned int i = 0; i < 11; i++)
	{
		c[i] = s[rand() % (ss - 1)];
	}
	const QString r = QString(c).trimmed();
 	return r;
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
	QString s("");
	ok = get_string_value(ds, mdcm::Tag(0x0008,0x0008), s);
	if (ok && s.contains(QString("LOCALIZER"))) *l = true;
	else *l = false;
	return true;
}

bool DicomUtils::has_functional_groups(const mdcm::DataSet & ds)
{
	const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
	const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
	if (ds.FindDataElement(tSharedFunctionalGroupsSequence))
	{
		const mdcm::DataElement & e  =
			ds.GetDataElement(tSharedFunctionalGroupsSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems()>0) return true;
	}
	if (ds.FindDataElement(tPerFrameFunctionalGroupsSequence))
	{
		const mdcm::DataElement & e  =
			ds.GetDataElement(tPerFrameFunctionalGroupsSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems()>0) return true;
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
	if (get_string_value(ds,tPixelPresentation,PixelPresentation))
	{
		if (PixelPresentation.toUpper().trimmed() ==
				QString("COLOR"))
		{
			QString PhotometricInterpretation;
			if (get_string_value(
					ds,
					tPhotometricInterpretation,
					PhotometricInterpretation))
			{
				if (PhotometricInterpretation.toUpper().trimmed() ==
						QString("MONOCHROME2") &&
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
	if(ds.FindDataElement(mdcm::Tag(0x0028,0x3000)))
	{
		const mdcm::DataElement& e =
			ds.GetDataElement(mdcm::Tag(0x0028,0x3000));
		const mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems()==1)
		{
			const mdcm::Item & item = sq->GetItem(1);
			const mdcm::DataSet & nds = item.GetNestedDataSet();
			if(nds.FindDataElement(mdcm::Tag(0x0028,0x3006)) &&
				nds.FindDataElement(mdcm::Tag(0x0028,0x3002)))
			{
				const mdcm::DataElement & e1  =
					nds.GetDataElement(mdcm::Tag(0x0028,0x3006));
				const mdcm::DataElement & e2  =
					nds.GetDataElement(mdcm::Tag(0x0028,0x3002));
				if (!e1.IsEmpty() && !e2.IsEmpty()) return true;
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
	if (get_string_value(ds,tnumframes,numframes))
	{
		const QVariant v(numframes.remove(QChar('\0')));
		bool ok = false;
		const int k = v.toInt(&ok);
		if (ok && k>1) return true;
	}
	return false;
}

void DicomUtils::read_sop_instance_uid(
	const QString & f,
	QString & sop_instance_uid)
{
	std::set<mdcm::Tag> tags;
	mdcm::Tag tsopinstance(0x0008,0x0018);
	tags.insert(tsopinstance);
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
	const bool f_ok = reader.ReadSelectedTags(tags);
	if (!f_ok) return;
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	if (ds.IsEmpty()) return;
	QString sop_instance_uid_;
	if (get_string_value(ds,tsopinstance,sop_instance_uid_))
		sop_instance_uid = sop_instance_uid_.remove(QChar('\0'));
}

void DicomUtils::read_image_info(
	const QString & f,
	unsigned short * rows_,
	unsigned short * columns_,
	QString        & position,
	QString        & orientation,
	QString        & spacing,
	QString        & sop_instance_uid)
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
	// Pixel Aspect Ratio
	mdcm::Tag tspacing3(0x0028,0x0034);    tags.insert(tspacing3);
	//
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
	const bool f_ok = reader.ReadSelectedTags(tags);
	if (!f_ok) return;
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	if (ds.IsEmpty()) return;
	//
	const bool b0 = get_us_value(ds,trows,rows_);
	const bool b1 = get_us_value(ds,tcolumns,columns_);
	if (!(b0 && b1))
	{
		;;
	}
	//
	{
		QString sop_instance_uid_;
		if (get_string_value(ds,tsopinstance,sop_instance_uid_))
			sop_instance_uid = sop_instance_uid_.remove(QChar('\0'));
	}
	//
	{
		QString position_;
		if (ds.FindDataElement(tpos))
		{
			if (get_string_value(ds,tpos,position_))
				position = position_.remove(QChar('\0'));
		}
		else
		{
			if (get_string_value(ds,tpos_old,position_))
				position = position_.remove(QChar('\0'));
		}
	}
	//
	{
		QString orientation_;
		if (ds.FindDataElement(torie))
		{
			if (get_string_value(ds,torie,orientation_))
				orientation = orientation_.remove(QChar('\0'));
		}
		else
		{
			if (get_string_value(ds,torie_old,orientation_))
				orientation = orientation_.remove(QChar('\0'));
		}
	}
	//
	{
		QString spacing_;
		if (ds.FindDataElement(tspacing0))
		{
			if (get_string_value(ds,tspacing0,spacing_))
				spacing = spacing_.remove(QChar('\0'));
		}
		else if (ds.FindDataElement(tspacing1))
		{
			if (get_string_value(ds,tspacing1,spacing_))
				spacing = spacing_.remove(QChar('\0'));
		}
		else if (ds.FindDataElement(tspacing2))
		{
			if (get_string_value(ds,tspacing2,spacing_))
				spacing = spacing_.remove(QChar('\0'));
		}
		else
		{
			if (get_string_value(ds,tspacing3,spacing_))
				spacing = spacing_.remove(QChar('\0'));
		}
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
	//
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
	const bool f_ok = reader.ReadSelectedTags(tags);
	if (!f_ok) return;
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	if (ds.IsEmpty()) return;
	//
	const bool b0 = get_us_value(ds,trows,rows_);
	const bool b1 = get_us_value(ds,tcolumns,columns_);
	const bool b2 = get_ds_values(ds,tframeoffset,z_offsets);
	if (!(b0 && b1 && b2))
	{
		std::cout << "read_image_info_rtdose warning (1)" << std::endl;
	}
	//
	{
		QString numframes;
		if (get_string_value(ds,tframes,numframes))
		{
			const QVariant v(numframes.remove(QChar('\0')));
			bool ok = false;
			const int k = v.toInt(&ok);
			if (ok && k>0) *num_frames_ = (unsigned short)k;
		}
	}
	//
	{
		QString position_;
		if (get_string_value(ds,tpos,position_))
			position = position_.remove(QChar('\0'));
	}
	//
	{
		QString orientation_;
		if (get_string_value(ds,torie,orientation_))
			orientation = orientation_.remove(QChar('\0'));
	}
	//
	{
		QString spacing_;
		if (get_string_value(ds,tspacing,spacing_))
			spacing = spacing_.remove(QChar('\0'));
	}
}

void DicomUtils::load_contour(
	const mdcm::DataSet & ds,
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	QString charset("");
	const mdcm::Tag tcharset(0x0008,0x0005);
	{
		QString charset_;
		if (get_string_value(ds,tcharset,charset_))
			charset = charset_.remove(QChar('\0'));
	}
	// StructureSetROISequence
	const mdcm::Tag tssroisq(0x3006,0x0020);
	if(!ds.FindDataElement(tssroisq)) return;
	const mdcm::DataElement & ssroisq =
		ds.GetDataElement(tssroisq);
	mdcm::SmartPointer<mdcm::SequenceOfItems> ssqi =
		ssroisq.GetValueAsSQ();
	if(!(ssqi && ssqi->GetNumberOfItems()>0)) return;
	// ROIContourSequence
	const mdcm::Tag troicsq(0x3006,0x0039);
	if(!ds.FindDataElement(troicsq)) return;
	const mdcm::DataElement & roicsq =
		ds.GetDataElement(troicsq);
	mdcm::SmartPointer<mdcm::SequenceOfItems> sqi =
		roicsq.GetValueAsSQ();
	//
	if(!(sqi && sqi->GetNumberOfItems()>0)) return;
	if(!(ssqi && ssqi->GetNumberOfItems()>0)) return;
	// RTROIObservationsSequence
	mdcm::SmartPointer<mdcm::SequenceOfItems> obssq = NULL;
	const mdcm::Tag tobservationssq(0x3006,0x0080);
	if(ds.FindDataElement(tobservationssq))
	{
		const mdcm::DataElement & eobservation =
			ds.GetDataElement(tobservationssq);
		obssq = eobservation.GetValueAsSQ();
	}
	//
	for (unsigned int pd = 0; pd < sqi->GetNumberOfItems(); ++pd)
	{
		ROI roi;
		//
		const mdcm::Item & item = sqi->GetItem(pd+1);
		const mdcm::DataSet & nestedds =
			item.GetNestedDataSet();
		// Referenced ROI Number
		mdcm::Attribute<0x3006,0x0084> roinumber;
		roinumber.SetFromDataElement(
			nestedds.GetDataElement(roinumber.GetTag()));
		// find structure_set_roi_sequence corresponding
		// to roi_contour_sequence (by comparing id numbers)
		unsigned int spd = 0;
		mdcm::Item    sitem;
		mdcm::DataSet snestedds;
		mdcm::Attribute<0x3006,0x0022> sroinumber; // ROI Number
		int roi_number = -1;
		do
		{
			sitem = ssqi->GetItem(spd+1);
			snestedds = sitem.GetNestedDataSet();
			if (snestedds.FindDataElement(mdcm::Tag(0x3006,0x0022)))
			{
				sroinumber.SetFromDataElement(
					snestedds.GetDataElement(sroinumber.GetTag()));
				roi_number = sroinumber.GetValue();
			}
			spd++;
			if (spd >= ssqi->GetNumberOfItems()) break;
		} while (sroinumber.GetValue() != roinumber.GetValue());
		if (roi_number < 0) return;
		roi.id = roi_number;
		//
		if (obssq && obssq->GetNumberOfItems()>0)
		{
			const mdcm::Tag tinterpretedtype(0x3006,0x00a4);
			for (unsigned int obsidx = 0;
				obsidx < obssq->GetNumberOfItems();
				++obsidx)
			{
				const mdcm::Item & obsitem = obssq->GetItem(obsidx+1);
				const mdcm::DataSet & obsnestedds = obsitem.GetNestedDataSet();
				if (obsnestedds.FindDataElement(mdcm::Tag(0x3006,0x0084)))
				{
					roinumber.SetFromDataElement(
						obsnestedds.GetDataElement(roinumber.GetTag()));
					if ((int)roinumber.GetValue() == roi_number)
					{
						if (obsnestedds.FindDataElement(tinterpretedtype))
						{
							const mdcm::DataElement & einterpretedtype =
								obsnestedds.GetDataElement(tinterpretedtype);
							if (!einterpretedtype.IsEmpty() &&
								!einterpretedtype.IsUndefinedLength() &&
								einterpretedtype.GetByteValue())
							roi.interpreted_type = QString::fromLatin1(
								einterpretedtype.GetByteValue()->GetPointer(),
								einterpretedtype.GetByteValue()->GetLength()).
									trimmed();
						}
						break;
					}
				}
			}
		}
		// Referenced Frame of Reference UID
		const mdcm::Tag trefframeofref(0x3006,0x0024);
		if(snestedds.FindDataElement(trefframeofref))
		{
			const mdcm::DataElement & erefframeofref =
				snestedds.GetDataElement(trefframeofref);
			if (!erefframeofref.IsEmpty() &&
				!erefframeofref.IsUndefinedLength() &&
				erefframeofref.GetByteValue())
				roi.ref_frame_of_ref =
					QString::fromLatin1(
						erefframeofref.GetByteValue()->GetPointer(),
						erefframeofref.GetByteValue()->GetLength()).
							trimmed().remove(QChar('\0'));
		}
		// ROIName
		const mdcm::Tag stcsq(0x3006,0x0026);
		if(snestedds.FindDataElement(stcsq))
		{
			const mdcm::DataElement & sde = snestedds.GetDataElement(stcsq);
			if (!sde.IsEmpty() && !sde.IsUndefinedLength() && sde.GetByteValue())
			{
				QByteArray ba(
					sde.GetByteValue()->GetPointer(),
					sde.GetByteValue()->GetLength());
				const QString tmp0 =
					CodecUtils::toUTF8(
						&ba, charset.toLatin1().constData());
				roi.name =tmp0.trimmed().remove(QChar('\0'));
			}
		}
		int color_r = 255;
		int color_g = 255;
		int color_b = 255;
		// ROI Display Color
		const mdcm::Tag troidc(0x3006, 0x002a);
		mdcm::Attribute<0x3006, 0x002a> color = {};
		if(nestedds.FindDataElement(troidc))
		{
			const mdcm::DataElement &decolor =
				nestedds.GetDataElement(troidc);
			color.SetFromDataElement(decolor);
			const int * color_p = color.GetValues();
			if (color.GetNumberOfValues()==3)
			{
				color_r = color_p[0];
				color_g = color_p[1];
				color_b = color_p[2];
			}
		}
		roi.color.r=(float)color_r/255.0f;
		roi.color.g=(float)color_g/255.0f;
		roi.color.b=(float)color_b/255.0f;
		// ContourSequence
		const mdcm::Tag tcsq(0x3006, 0x0040);
		if(!nestedds.FindDataElement(tcsq)) continue;
		const mdcm::DataElement & csq =
			nestedds.GetDataElement(tcsq);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sqi2 =
			csq.GetValueAsSQ();
		if(!(sqi2 && sqi2->GetNumberOfItems()>0)) continue;
		unsigned int nitems = sqi2->GetNumberOfItems();
		//
		for(unsigned int i = 0; i < nitems; ++i)
		{
			const mdcm::Item & item2 = sqi2->GetItem(i+1);
			const mdcm::DataSet& nestedds2 =
				item2.GetNestedDataSet();
			// ContourGeometricType
			const mdcm::Tag tcontour_geometric_type(0x3006, 0x0042);
			const mdcm::DataElement & contour_geometric_type =
				nestedds2.GetDataElement(tcontour_geometric_type);
			QString qtr_contour_geometric_type;
			if (!contour_geometric_type.IsEmpty() &&
				!contour_geometric_type.IsUndefinedLength() &&
				contour_geometric_type.GetByteValue())
				qtr_contour_geometric_type = QString::fromLatin1(
					contour_geometric_type.GetByteValue()->GetPointer(),
					contour_geometric_type.GetByteValue()->GetLength()).
						trimmed().remove(QChar('\0')).toUpper();
			// ContourData
			const mdcm::Tag tcontourdata(0x3006, 0x0050);
			const mdcm::DataElement & contourdata =
				nestedds2.GetDataElement(tcontourdata);
			mdcm::Attribute<0x3006,0x0050> at1;
			at1.SetFromDataElement(contourdata);
			const double * varray_p = at1.GetValues();
			unsigned int vertices   = at1.GetNumberOfValues()/3;
			Contour * contour = new Contour();
			contour->id = i;
			contour->roiid = roi.id;
			if (qtr_contour_geometric_type ==
				QString("CLOSED_PLANAR")) contour->type = 1;
			else if (qtr_contour_geometric_type ==
				QString("OPEN_PLANAR")) contour->type = 2;
			else if (qtr_contour_geometric_type ==
				QString("OPEN_NONPLANAR")) contour->type = 3;
			else if (qtr_contour_geometric_type ==
				QString("POINT")) contour->type = 4;
			else contour->type = 0;
			for(unsigned int j = 0; j < vertices * 3; j+=3)
			{
				DPoint point; point.x =
					static_cast<float>(varray_p[j+0]);
				point.y = static_cast<float>(varray_p[j+1]);
				point.z = static_cast<float>(varray_p[j+2]);
				contour->dpoints.push_back(point);
			}
			// Contour Image Sequence
			const mdcm::Tag timageseq(0x3006,0x0016);
			if(nestedds2.FindDataElement(timageseq))
			{
				const mdcm::DataElement & imageseq =
					nestedds2.GetDataElement(timageseq);
				mdcm::SmartPointer<mdcm::SequenceOfItems> sqimageseq =
					imageseq.GetValueAsSQ();
				if(sqimageseq && sqimageseq->GetNumberOfItems()>0)
				{
					for (unsigned int n_imageseq = 0;
						n_imageseq < sqimageseq->GetNumberOfItems();
						n_imageseq++)
					{
						const mdcm::Item & imageseqitem =
							sqimageseq->GetItem(n_imageseq+1);
						const mdcm::DataSet & imageseqds =
							imageseqitem.GetNestedDataSet();
						// Referenced SOP Instance UID
						const mdcm::Tag trefsopinstuid(0x0008, 0x1155);
						if(imageseqds.FindDataElement(trefsopinstuid))
						{
							const mdcm::DataElement & erefsopinstuid =
								imageseqds.GetDataElement(trefsopinstuid);
							if (!erefsopinstuid.IsEmpty() &&
								!erefsopinstuid.IsUndefinedLength() &&
								erefsopinstuid.GetByteValue())
							contour->ref_sop_instance_uids <<
								QString::fromLatin1(
									erefsopinstuid.GetByteValue()->GetPointer(),
									erefsopinstuid.GetByteValue()->GetLength()).
										trimmed().remove(QChar('\0'));
						}
					}
				}
			}
			//
			// may be TODO : Contour Slab Thickness, Contour Offset Vector
			//
			contour->vao_initialized = false;
			roi.contours[contour->id] = contour;
		}
		if (ivariant) ivariant->di->rois.push_back(roi);
	}
	ivariant->image_type = 100;
	read_ivariant_info_tags(ds, ivariant);
}

int DicomUtils::read_instance_number(
	const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return -1;
	const mdcm::Tag tInstanceNumber(0x0020, 0x0013);
	if (ds.FindDataElement(tInstanceNumber))
	{
		const mdcm::DataElement & e =
			ds.GetDataElement(tInstanceNumber);
		if (!e.IsEmpty() &&
			!e.IsUndefinedLength() &&
			e.GetByteValue())
		{
			const QString s = QString::fromLatin1(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			bool ok;
			const int tmp0 =
				QVariant(s.trimmed().remove(QChar('\0'))).toInt(&ok);
			if (ok) return tmp0;
		}
	}
	return -1;
}

QString DicomUtils::read_instance_uid(
	const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return QString("");
	const mdcm::Tag tInstanceUID(0x0008, 0x0018);
	if (ds.FindDataElement(tInstanceUID))
	{
		const mdcm::DataElement & e =
			ds.GetDataElement(tInstanceUID);
		if (!e.IsEmpty() &&
			!e.IsUndefinedLength() &&
			e.GetByteValue())
		{
			const QString s = QString::fromLatin1(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			return s;
		}
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
	short lut_function_ = 0;
	QString s;
	if (get_string_value(ds, mdcm::Tag(0x0028,0x1056), s))
	{
		s = s.trimmed().toUpper().remove(QChar('\0'));
		if (s == QString("LINEAR_EXACT")) lut_function_ = 1;
		else if (s == QString("SIGMOID")) lut_function_ = 2;
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
			const double k = w.at(0);
			if      (lut_function_ == 0 && k >= 2) { *width_ = k - 1; }
			else if (lut_function_ == 1 && k >= 2) { *width_ = k;     }
			else if (lut_function_ == 2 && k >= 2) { *width_ = k;     }
			else                                   { *width_ = k;     }
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
	const bool ok3d, const bool skip_texture, GLWidget * gl,
	QProgressDialog * pb,
	float tolerance)
{
	if (!ivariant) return false;
	bool ok = false;
	const int unsigned size_z = filenames_.size();
	std::vector<double*> values;
	unsigned short rows    = 0;
	unsigned short columns = 0;
	double spacing_x = 0, spacing_y = 0, spacing_z = 0;
	double origin_x  = 0, origin_y  = 0, origin_z  = 0;
	float
		slices_dir_x,
		slices_dir_y,
		slices_dir_z,
		up_dir_x,
		up_dir_y,
		up_dir_z;
	float center_x, center_y, center_z;
	double dircos[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	if (pb)
	{
		const QString info_ = QString(
			"Loading geometry from ") +
			QVariant(size_z).toString()+
			QString(" files");
		pb->setLabelText(info_);
		pb->setValue(-1);
	}
	QApplication::processEvents();
	for (unsigned int i = 0; i < size_z; i++)
	{
		QString
			sop_instance_uid(""),
			pat_pos_s(""),
			pat_orient_s(""),
			pix_spacing_s("");
		unsigned short rows_ = 0, columns_ = 0;
		read_image_info(
			filenames_.at(i),
			&rows_, &columns_,
			pat_pos_s,
			pat_orient_s,
			pix_spacing_s,
			sop_instance_uid);
		double pat_pos[3];
		double pat_orient[6];
		double pix_spacing[2];
		const bool ok_pos =
			get_patient_position(pat_pos_s,pat_pos);
		const bool ok_orient =
			get_patient_orientation(pat_orient_s,pat_orient);
		const bool pix_spacing_ok =
			get_pixel_spacing(pix_spacing_s,pix_spacing);
		if (!ok_pos||!ok_orient||!pix_spacing_ok) goto quit_;
		if (i==0)
		{
			rows = rows_; columns = columns_;
			spacing_x = pix_spacing[1];
			spacing_y = pix_spacing[0];
		}
		else
		{
			if (rows!=rows_||columns!=columns_) goto quit_;
			if (spacing_x > pix_spacing[1]+0.01 ||
				spacing_x < pix_spacing[1]-0.01 ||
				spacing_y > pix_spacing[0]+0.01 ||
				spacing_y < pix_spacing[0]-0.01) goto quit_;
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
		ivariant->image_instance_uids[i] = sop_instance_uid;
#if 0
		if (i == 0) std::cout << std::endl;
		std::cout << "read_slices() "
			<< i
			<< " " <<
			sop_instance_uid.toStdString() << " "
			<< filenames_.at(i).toStdString()
			<< std::endl;
		if (i == size_z - 1) std::cout << std::endl;
#endif
	}
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	ok = generate_geometry(
			ivariant->di->image_slices,
			ivariant->di->spectroscopy_slices,
			values,
			rows, columns,
			spacing_x, spacing_y, &spacing_z,
			ok3d, ivariant->di->skip_texture, gl,
			&ivariant->equi, &ivariant->one_direction,
			&origin_x, &origin_y, &origin_z,
			dircos,
			&slices_dir_x, &slices_dir_y, &slices_dir_z,
			&up_dir_x, &up_dir_y, &up_dir_z,
			&center_x, &center_y, &center_z,
			tolerance,
			false);
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
		ivariant->di->iz_spacing = size_z==1 ? 1 : spacing_z;
		for (int x = 0; x < 6; x++)
			ivariant->di->dircos[x] = (float)dircos[x]; ///////
		//
		if (ivariant->equi == false)
		{
			float cx = 0.0f, cy = 0.0f, cz = 0.0f;
			CommonUtils::calculate_center_notuniform(
				ivariant->di->image_slices,&cx,&cy,&cz);
			ivariant->di->default_center_x =
				ivariant->di->center_x = cx;
			ivariant->di->default_center_y =
				ivariant->di->center_y = cy;
			ivariant->di->default_center_z =
				ivariant->di->center_z = cz;
		}
		else
		{
			ivariant->di->default_center_x =
				ivariant->di->center_x = center_x;
			ivariant->di->default_center_y =
				ivariant->di->center_y = center_y;
			ivariant->di->default_center_z =
				ivariant->di->center_z = center_z;
		}
		ivariant->di->slices_generated = true;
		ivariant->di->slices_from_dicom = true;
	}
quit_:
	for (unsigned int x = 0; x < values.size(); x++)
	{
		if (values.at(x)) delete [] values[x];
	}
	return ok;
}

bool DicomUtils::read_slices_uihgrid(
	const mdcm::DataSet & ds, ImageVariant * ivariant,
	const bool ok3d, const bool skip_texture, GLWidget * gl,
	QProgressDialog * pb,
	float tolerance)
{
	if (!ivariant) return false;
	//
	const mdcm::PrivateTag tMRNumberOfSliceInVolume(
		0x0065, 0x50, "Image Private Header");
	int num_slices = 0;
	if (ds.FindDataElement(tMRNumberOfSliceInVolume))
	{
		std::vector<double> result;
		if (DicomUtils::priv_get_ds_values(
			ds,tMRNumberOfSliceInVolume,result))
			num_slices = (int)result[0];
	}
	else if (ds.FindDataElement(mdcm::Tag(0x0065,0x1050)))
	{
		std::vector<double> result;
		if (DicomUtils::get_ds_values(
			ds,mdcm::Tag(0x0065,0x1050),result))
			num_slices = (int)result[0];
	}
	if (num_slices < 1) return false;
	//
	const mdcm::Tag tImageOrientationPatient(0x0020,0x0037);
	std::vector<double> pat_orient;
	if (get_ds_values(
			ds,tImageOrientationPatient,pat_orient))
	{
		if (pat_orient.size()!=6) return false;
	}
	//
	double spacing_x = 0, spacing_y = 0, spacing_z = 0;
	bool spacing_ok = false;
	{
		const mdcm::Tag tspacing0(0x0028,0x0030);
		const mdcm::Tag tspacing1(0x0018,0x1164);
		const mdcm::Tag tspacing2(0x0018,0x2010);
		const mdcm::Tag tspacing3(0x0028,0x0034);
		QString spacing_s;
		if (ds.FindDataElement(tspacing0))
		{
			QString spacing_;
			if (get_string_value(ds,tspacing0,spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
		}
		else if (ds.FindDataElement(tspacing1))
		{
			QString spacing_;
			if (get_string_value(ds,tspacing1,spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
		}
		else if (ds.FindDataElement(tspacing2))
		{
			QString spacing_;
			if (get_string_value(ds,tspacing2,spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
		}
		else
		{
			QString spacing_;
			if (get_string_value(ds,tspacing3,spacing_))
				spacing_s = spacing_.remove(QChar('\0'));
		}
		double pix_spacing[2];
		if (get_pixel_spacing(spacing_s,pix_spacing))
		{
			spacing_x = pix_spacing[1];
			spacing_y = pix_spacing[0];
			spacing_ok = true;
		}
	}
	if (!spacing_ok) return false;
	//
	const mdcm::PrivateTag tMRVFrameSequence(
		0x0065,0x51,"Image Private Header");
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq;
	if (ds.FindDataElement(tMRVFrameSequence))
	{
		const mdcm::DataElement & e  =
			ds.GetDataElement(tMRVFrameSequence);
		sq = e.GetValueAsSQ();
		if (!(sq && sq->GetNumberOfItems()>0))
			return false;
	}
	else if (ds.FindDataElement(mdcm::Tag(0x0065,0x1051)))
	{
		const mdcm::DataElement & e  =
			ds.GetDataElement(mdcm::Tag(0x0065,0x1051));
		sq = e.GetValueAsSQ();
		if (!(sq && sq->GetNumberOfItems()>0))
			return false;
	}
	else
	{
		return false;
	}
	const unsigned int num_items = sq->GetNumberOfItems();
	if ((unsigned int)num_slices != num_items) return false;
	//
	//
	// FIXME
	unsigned short image_rows;
	const mdcm::Tag tRows(0x0028,0x0010);
	if (!get_us_value(ds,tRows,&image_rows))
		return false;
	const mdcm::Tag tColumns(0x0028,0x0011);
	unsigned short image_columns;
	if (!get_us_value(ds,tColumns,&image_columns))
		return false;
	const unsigned int xx =
		(image_columns >= image_rows)
		?
		image_columns/ceil(sqrt(num_slices))
		:
		image_rows/ceil(sqrt(num_slices));
	unsigned short rows = xx;
	unsigned short columns = xx;
	//
	//
	//
	QMap<qlonglong, QString> acqtimes;
	std::vector<double*> values;
	for (unsigned int x = 0; x < num_items; x++)
	{
		const mdcm::Item & item = sq->GetItem(x+1);
		const mdcm::DataSet & nds = item.GetNestedDataSet();
		const mdcm::Tag tImagePositionPatient(0x0020,0x0032);
		std::vector<double> pat_pos;
		if (DicomUtils::get_ds_values(
				nds,tImagePositionPatient,pat_pos))
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
		QString AcquisitionDateTime("");
		if (DicomUtils::get_string_value(
				nds,
				tAcquisitionDateTime,
				AcquisitionDateTime))
		{
			;;
		}
		else if (
			nds.FindDataElement(tAcquisitionDate) &&
			nds.FindDataElement(tAcquisitionTime))
		{
			QString AcquisitionDate("");
			QString AcquisitionTime("");
			if (
				DicomUtils::get_string_value(
					nds,
					tAcquisitionDate,
					AcquisitionDate) &&
				DicomUtils::get_string_value(
					nds,
					tAcquisitionTime,
					AcquisitionTime))
			{
				if (
					!AcquisitionDate.isEmpty() &&
					!AcquisitionTime.isEmpty())
					AcquisitionDateTime =
						AcquisitionDate
							.trimmed()
							.remove(QChar('\0')) +
						AcquisitionTime.trimmed();
			}
		}
		if (!AcquisitionDateTime.isEmpty())
		{
			bool tmp56_ok = false;
			const double tmp56 = QVariant(
				AcquisitionDateTime.trimmed())
					.toDouble(&tmp56_ok);
			if (tmp56_ok)
				acqtimes[(qlonglong)(tmp56*1000.0)] =
					AcquisitionDateTime;
		}
	}
	QMap<qlonglong,QString>::const_iterator acqit =
		acqtimes.constBegin();
	if (acqit != acqtimes.constEnd())
	{
		const QString tmp57 = acqit.value();
		if (tmp57.size() >= 14)
		{
			ivariant->acquisition_date =
				tmp57.left(8);
			ivariant->acquisition_time =
				tmp57.right(tmp57.size() - 8);
		}
	}
	//
	double origin_x  = 0, origin_y  = 0, origin_z  = 0;
	float
		slices_dir_x,
		slices_dir_y,
		slices_dir_z,
		up_dir_x,
		up_dir_y,
		up_dir_z;
	float center_x, center_y, center_z;
	double dircos[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	const bool ok = generate_geometry(
			ivariant->di->image_slices,
			ivariant->di->spectroscopy_slices,
			values,
			rows, columns,
			spacing_x, spacing_y, &spacing_z,
			ok3d, ivariant->di->skip_texture, gl,
			&ivariant->equi, &ivariant->one_direction,
			&origin_x, &origin_y, &origin_z,
			dircos,
			&slices_dir_x, &slices_dir_y, &slices_dir_z,
			&up_dir_x, &up_dir_y, &up_dir_z,
			&center_x, &center_y, &center_z,
			tolerance,
			false);
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
		for (int x = 0; x < 6; x++)
			ivariant->di->dircos[x] = (float)dircos[x]; ///////
		//
		if (ivariant->equi == false)
		{
			float cx = 0.0f, cy = 0.0f, cz = 0.0f;
			CommonUtils::calculate_center_notuniform(
				ivariant->di->image_slices,&cx,&cy,&cz);
			ivariant->di->default_center_x =
				ivariant->di->center_x = cx;
			ivariant->di->default_center_y =
				ivariant->di->center_y = cy;
			ivariant->di->default_center_z =
				ivariant->di->center_z = cz;
		}
		else
		{
			ivariant->di->default_center_x =
				ivariant->di->center_x = center_x;
			ivariant->di->default_center_y =
				ivariant->di->center_y = center_y;
			ivariant->di->default_center_z =
				ivariant->di->center_z = center_z;
		}
		ivariant->di->slices_generated = true;
		ivariant->di->slices_from_dicom = true;
	}
	for (unsigned int x = 0; x < values.size(); x++)
	{
		if (values.at(x)) delete [] values[x];
	}
	return ok;
}

bool DicomUtils::read_slices_rtdose(
	const QString & filename_, ImageVariant * ivariant,
	const bool ok3d, const bool skip_texture, GLWidget * gl,
	QProgressDialog * pb,
	float tolerance)
{
	if (!ivariant) return false;
	//
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	//
	bool ok = false;
	std::vector<double*> values;
	unsigned short numframes = 0;
	unsigned short rows      = 0;
	unsigned short columns   = 0;
	std::vector<double> z_offsets;
	double pat_pos[3];
	double pat_orient[6];
	double pix_spacing[2];
	double spacing_x, spacing_y, spacing_z, origin_x, origin_y, origin_z;
	float slices_dir_x, slices_dir_y, slices_dir_z, up_dir_x, up_dir_y, up_dir_z;
	float center_x, center_y, center_z;
	double dircos[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	QString pat_pos_s(""), pat_orient_s(""), pix_spacing_s("");
	read_image_info_rtdose(filename_,
		&numframes, &rows, &columns,
		pat_pos_s, pat_orient_s, pix_spacing_s, z_offsets);
	const bool ok_pos         = get_patient_position(pat_pos_s,pat_pos);
	const bool ok_orient      = get_patient_orientation(pat_orient_s,pat_orient);
	const bool pix_spacing_ok = get_pixel_spacing(pix_spacing_s,pix_spacing);
	if (numframes==0||rows==0||columns==0) goto quit_;
	if (!ok_pos||!ok_orient||!pix_spacing_ok) goto quit_;
	if (z_offsets.empty()) goto quit_;
	if (numframes!=z_offsets.size()) goto quit_;
	spacing_x = pix_spacing[0];
	spacing_y = pix_spacing[1];
	for (unsigned int i = 0; i < numframes; i++)
	{
		double * p = new double[9];
		p[0] = pat_pos[0];
		p[1] = pat_pos[1];
		p[2] = pat_pos[2] + (float)z_offsets.at(i);
		p[3] = pat_orient[0];
		p[4] = pat_orient[1];
		p[5] = pat_orient[2];
		p[6] = pat_orient[3];
		p[7] = pat_orient[4];
		p[8] = pat_orient[5];
		values.push_back(p);
	}
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	ok = generate_geometry(
			ivariant->di->image_slices,
			ivariant->di->spectroscopy_slices,
			values,
			rows, columns,
			spacing_x, spacing_y, &spacing_z,
			ok3d, ivariant->di->skip_texture, gl,
			&ivariant->equi,
			&ivariant->one_direction,
			&origin_x, &origin_y, &origin_z,
			dircos,
			&slices_dir_x, &slices_dir_y, &slices_dir_z,
			&up_dir_x, &up_dir_y, &up_dir_z,
			&center_x, &center_y, &center_z,
			tolerance,
			false);
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
		for (int x = 0; x < 6; x++) ivariant->di->dircos[x] = (float)dircos[x]; ///////
		//
		if (ivariant->equi == false)
		{
			float cx = 0.0f, cy = 0.0f, cz = 0.0f;
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
	for (unsigned int x = 0; x < values.size(); x++)
	{
		if (values.at(x)) delete [] values[x];
	}
	return ok;
}

static short force_suppllut = 0;
void DicomUtils::global_force_suppllut(short x)
{
	// 1 force apply
	// 2 force not apply
	force_suppllut = x;
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

	if (ds.FindDataElement(tDimensionIndexSequence))
	{
		const mdcm::DataElement &
			deDimensionIndexSequence =
				ds.GetDataElement(tDimensionIndexSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems>
			sqDimensionIndexSequence =
				deDimensionIndexSequence.GetValueAsSQ();
		if (!sqDimensionIndexSequence) return;
		const unsigned int number_of_items =
			sqDimensionIndexSequence->GetNumberOfItems();
		for (unsigned int x = 0; x < number_of_items; x++)
		{
			const mdcm::Item & item =
				sqDimensionIndexSequence->GetItem(x+1);
			const mdcm::DataSet & nestedds =
				item.GetNestedDataSet();
			unsigned short
				group0 = 0,
				element0 = 0,
				group1 = 0,
				element1 = 0;
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
			if (ok0 && ok1)
			{
				DimIndex idx;
				idx.uid = "";
				idx.index_pointer = mdcm::Tag(group0,element0);
				idx.group_pointer = mdcm::Tag(group1,element1);
				sq.push_back(idx);
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
	if (ds.FindDataElement(tPerFrameFunctionalGroupsSequence))
	{
		const mdcm::DataElement & dePerFrameFunctionalGroupsSequence =
			ds.GetDataElement(tPerFrameFunctionalGroupsSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems>
			sqPerFrameFunctionalGroupsSequence =
				dePerFrameFunctionalGroupsSequence.GetValueAsSQ();
		if (sqPerFrameFunctionalGroupsSequence &&
			sqPerFrameFunctionalGroupsSequence->GetNumberOfItems()>0)
		{
			for (unsigned int x = 0;
				x < sqPerFrameFunctionalGroupsSequence->
					GetNumberOfItems();
				x++)
			{
				const mdcm::Item & item =
					sqPerFrameFunctionalGroupsSequence->GetItem(x+1);
				const mdcm::DataSet & nestedds = item.GetNestedDataSet();
				if (nestedds.FindDataElement(tFrameContentSequence))
				{
					const mdcm::DataElement & deFrameContentSequence =
						nestedds.GetDataElement(tFrameContentSequence);
					mdcm::SmartPointer<mdcm::SequenceOfItems>
						sqFrameContentSequence =
							deFrameContentSequence.GetValueAsSQ();
					if (sqFrameContentSequence &&
						sqFrameContentSequence->GetNumberOfItems()==1)
					{
						const mdcm::Item & item1 =
							sqFrameContentSequence->GetItem(1);
						const mdcm::DataSet & nestedds1 =
							item1.GetNestedDataSet();
						if (nestedds1.FindDataElement(
								tDimensionIndexValues))
						{
							DimIndexValue index_value;
							index_value.id = x;
							const bool ok = get_ul_values(
								nestedds1,
								tDimensionIndexValues,
								index_value.idx);
							if (ok && index_value.idx.size() ==
								idx_sq_size)
								values.push_back(index_value);
							else return false;
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
	if (!ds.FindDataElement(t)) return false;
	QString charset("");
	if(ds.FindDataElement(tSpecificCharacterSet))
	{
		const mdcm::DataElement & eSpecificCharacterSet =
			ds.GetDataElement(tSpecificCharacterSet);
		if (!eSpecificCharacterSet.IsEmpty() &&
			!eSpecificCharacterSet.IsUndefinedLength() &&
			eSpecificCharacterSet.GetByteValue())
			charset = QString::fromLatin1(
				eSpecificCharacterSet.GetByteValue()->GetPointer(),
				eSpecificCharacterSet.GetByteValue()->GetLength()).
					trimmed();
	}
	//
	const mdcm::DataElement & deGroup = ds.GetDataElement(t);
	mdcm::SmartPointer<mdcm::SequenceOfItems> sqGroup =
		deGroup.GetValueAsSQ();
	if (!(sqGroup && sqGroup->GetNumberOfItems()>0))
		return false;
	for (unsigned int x = 0; x < sqGroup->GetNumberOfItems(); x++)
	{
		FrameGroup fg;
		fg.id = x;
		fg.stack_id_ok = false;
		fg.in_stack_pos_num_ok = false;
		fg.temp_pos_idx_ok = false;
		fg.vol_orient_ok = false;
		fg.vol_pos_ok = false;
		fg.temp_pos_off_ok = false;
		fg.us_temp_pos_unknown_ok = false;
		const mdcm::Item & item = sqGroup->GetItem(x+1);
		const mdcm::DataSet & nestedds = item.GetNestedDataSet();
		if (nestedds.FindDataElement(tFrameContentSequence))
		{
			const mdcm::DataElement & deFrameContentSequence =
				nestedds.GetDataElement(tFrameContentSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems>
				sqFrameContentSequence =
					deFrameContentSequence.GetValueAsSQ();
			if (sqFrameContentSequence &&
				sqFrameContentSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 =
					sqFrameContentSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 =
					item1.GetNestedDataSet();
				if (nestedds1.FindDataElement(tDimensionIndexValues))
				{
					DimIndexValue index_value;
					index_value.id = x;
					const bool ok = get_ul_values(
						nestedds1,
						tDimensionIndexValues,
						index_value.idx);
					if (ok && index_value.idx.size()==sq.size())
						dim_idx_values.push_back(index_value);
				}
				if (nestedds1.FindDataElement(tStackID))
				{
					const mdcm::DataElement & deStackID =
						nestedds1.GetDataElement(tStackID);
					if (!deStackID.IsEmpty() &&
						!deStackID.IsUndefinedLength() &&
						deStackID.GetByteValue())
						fg.stack_id=
							QVariant(QString::fromLatin1(
								deStackID.GetByteValue()->GetPointer(),
								deStackID.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'))
								).toInt(&fg.stack_id_ok);
				}
				if (nestedds1.FindDataElement(tInStackPositionNumber))
				{
					const mdcm::DataElement & deInStackPositionNumber
						= nestedds1.GetDataElement(tInStackPositionNumber);
					if (!deInStackPositionNumber.IsEmpty() &&
						!deInStackPositionNumber.IsUndefinedLength() &&
						deInStackPositionNumber.GetByteValue())
					{
						unsigned int tmp678;
						fg.in_stack_pos_num_ok = get_ul_value(
							nestedds1,
							tInStackPositionNumber,
							&tmp678);
						fg.in_stack_pos_num = (int)tmp678;
					}
				}
				QString FrameAcquisitionDateTime;
				if (
					get_string_value(
						nestedds1,
						tFrameAcquisitionDateTime,
						FrameAcquisitionDateTime))
				{
					fg.frame_acquisition_datetime =
						FrameAcquisitionDateTime;
				}
				QString FrameReferenceDateTime;
				if (
					get_string_value(
						nestedds1,
						tFrameReferenceDateTime,
						FrameReferenceDateTime))
				{
					fg.frame_reference_datetime =
						FrameReferenceDateTime;
				}
			}
		}
		if (nestedds.FindDataElement(tPlanePositionSequence))
		{
			const mdcm::DataElement & dePlanePositionSequence
				= nestedds.GetDataElement(tPlanePositionSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPlanePositionSequence
				= dePlanePositionSequence.GetValueAsSQ();
			if (sqPlanePositionSequence && sqPlanePositionSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 = sqPlanePositionSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				if (nestedds1.FindDataElement(tImagePositionPatient))
				{
					const mdcm::DataElement & deImagePositionPatient
						= nestedds1.GetDataElement(tImagePositionPatient);
					if (!deImagePositionPatient.IsEmpty() &&
						!deImagePositionPatient.IsUndefinedLength() &&
						deImagePositionPatient.GetByteValue())
						fg.pat_pos =
							QString::fromLatin1(
								deImagePositionPatient.GetByteValue()->GetPointer(),
								deImagePositionPatient.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
			}
		}
		if (nestedds.FindDataElement(tPlaneOrientationSequence))
		{
			const mdcm::DataElement & dePlaneOrientationSequence
				= nestedds.GetDataElement(tPlaneOrientationSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPlaneOrientationSequence
				= dePlaneOrientationSequence.GetValueAsSQ();
			if (sqPlaneOrientationSequence &&
				sqPlaneOrientationSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 = sqPlaneOrientationSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				if (nestedds1.FindDataElement(tImageOrientationPatient))
				{
					const mdcm::DataElement & deImageOrientationPatient
						= nestedds1.GetDataElement(tImageOrientationPatient);
					if (!deImageOrientationPatient.IsEmpty() &&
						!deImageOrientationPatient.IsUndefinedLength() &&
						deImageOrientationPatient.GetByteValue())
						fg.pat_orient =
							QString::fromLatin1(
								deImageOrientationPatient.GetByteValue()->GetPointer(),
								deImageOrientationPatient.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
			}
		}
		if (nestedds.FindDataElement(tPixelMeasuresSequence))
		{
			const mdcm::DataElement & dePixelMeasuresSequence
				= nestedds.GetDataElement(tPixelMeasuresSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPixelMeasuresSequence
				= dePixelMeasuresSequence.GetValueAsSQ();
			if (sqPixelMeasuresSequence && sqPixelMeasuresSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 = sqPixelMeasuresSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				if (nestedds1.FindDataElement(tPixelSpacing))
				{
					const mdcm::DataElement & dePixelSpacing
						= nestedds1.GetDataElement(tPixelSpacing);
					if (!dePixelSpacing.IsEmpty() &&
						!dePixelSpacing.IsUndefinedLength() &&
						dePixelSpacing.GetByteValue())
						fg.pix_spacing =
							QString::fromLatin1(
								dePixelSpacing.GetByteValue()->GetPointer(),
								dePixelSpacing.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
				else if (nestedds1.FindDataElement(tImagerPixelSpacing))
				{
					const mdcm::DataElement & deImagerPixelSpacing
						= nestedds1.GetDataElement(tImagerPixelSpacing);
					if (!deImagerPixelSpacing.IsEmpty() &&
						!deImagerPixelSpacing.IsUndefinedLength() &&
						deImagerPixelSpacing.GetByteValue())
						fg.pix_spacing =
							QString::fromLatin1(
								deImagerPixelSpacing.GetByteValue()->GetPointer(),
								deImagerPixelSpacing.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
				else if (nestedds1.FindDataElement(tNominalScannedPixelSpacing))
				{
					const mdcm::DataElement & deNominalScannedPixelSpacing
						= nestedds1.GetDataElement(tNominalScannedPixelSpacing);
					if (!deNominalScannedPixelSpacing.IsEmpty() &&
						!deNominalScannedPixelSpacing.IsUndefinedLength() &&
						deNominalScannedPixelSpacing.GetByteValue())
						fg.pix_spacing =
							QString::fromLatin1(
								deNominalScannedPixelSpacing.GetByteValue()->GetPointer(),
								deNominalScannedPixelSpacing.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
				else if (nestedds1.FindDataElement(tPixelAspectRatio))
				{
					const mdcm::DataElement & dePixelAspectRatio
						= nestedds1.GetDataElement(tPixelAspectRatio);
					if (!dePixelAspectRatio.IsEmpty() &&
						!dePixelAspectRatio.IsUndefinedLength() &&
						dePixelAspectRatio.GetByteValue())
						fg.pix_spacing =
							QString::fromLatin1(
								dePixelAspectRatio.GetByteValue()->GetPointer(),
								dePixelAspectRatio.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
				else { ;; }
				if (nestedds1.FindDataElement(tSliceThickness))
				{
					const mdcm::DataElement & deSliceThickness
						= nestedds1.GetDataElement(tSliceThickness);
					if (!deSliceThickness.IsEmpty() &&
						!deSliceThickness.IsUndefinedLength() &&
						deSliceThickness.GetByteValue())
						fg.slice_thick =
							QString::fromLatin1(
								deSliceThickness.GetByteValue()->GetPointer(),
								deSliceThickness.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
			}
		}
		if (nestedds.FindDataElement(tFrameAnatomySequence))
		{
			const mdcm::DataElement & deFrameAnatomySequence
				= nestedds.GetDataElement(tFrameAnatomySequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqFrameAnatomySequence
				= deFrameAnatomySequence.GetValueAsSQ();
			if (sqFrameAnatomySequence &&
				sqFrameAnatomySequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 = sqFrameAnatomySequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				if (nestedds1.FindDataElement(tFrameLaterality))
				{
					const mdcm::DataElement & deFrameLaterality
						= nestedds1.GetDataElement(tFrameLaterality);
					if (!deFrameLaterality.IsEmpty() &&
						!deFrameLaterality.IsUndefinedLength() &&
						deFrameLaterality.GetByteValue())
						fg.frame_laterality =
							QString::fromLatin1(
								deFrameLaterality.GetByteValue()->GetPointer(),
								deFrameLaterality.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
				if (nestedds1.FindDataElement(tAnatomicRegionSequence))
				{
					const mdcm::DataElement & deAnatomicRegionSequence
						= nestedds1.GetDataElement(tAnatomicRegionSequence);
					mdcm::SmartPointer<mdcm::SequenceOfItems> sqAnatomicRegionSequence
						= deAnatomicRegionSequence.GetValueAsSQ();
					if (sqAnatomicRegionSequence &&
						sqAnatomicRegionSequence->GetNumberOfItems()==1)
					{
						const mdcm::Item & item2 = sqAnatomicRegionSequence->GetItem(1);
						const mdcm::DataSet & nestedds2 = item2.GetNestedDataSet();
						if (nestedds2.FindDataElement(tCodeMeaning))
						{
							const mdcm::DataElement & deCodeMeaning
								= nestedds2.GetDataElement(tCodeMeaning);
							if (!deCodeMeaning.IsEmpty() &&
								!deCodeMeaning.IsUndefinedLength() &&
								deCodeMeaning.GetByteValue())
							{
								QByteArray baCodeMeaning(
									deCodeMeaning.GetByteValue()->GetPointer(),
									deCodeMeaning.GetByteValue()->GetLength());
								const QString frame_body_part =
									CodecUtils::toUTF8(
										&baCodeMeaning,
										charset.toLatin1().constData());
								if (!frame_body_part.isEmpty())
									fg.frame_body_part =
										frame_body_part.
											trimmed().
												remove(QChar('\0'));
							}
						}
					}
				}
			}
		}
		if (nestedds.FindDataElement(tFrameVOILUTSequence))
		{
			const mdcm::DataElement & deFrameVOILUTSequence
				= nestedds.GetDataElement(tFrameVOILUTSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqFrameVOILUTSequence
				= deFrameVOILUTSequence.GetValueAsSQ();
			if (sqFrameVOILUTSequence &&
				sqFrameVOILUTSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 = sqFrameVOILUTSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				if (nestedds1.FindDataElement(tWindowCenter))
				{
					const mdcm::DataElement & deWindowCenter
						= nestedds1.GetDataElement(tWindowCenter);
					if (!deWindowCenter.IsEmpty() &&
						!deWindowCenter.IsUndefinedLength() &&
						deWindowCenter.GetByteValue())
						fg.window_center =
							QString::fromLatin1(
								deWindowCenter.GetByteValue()->GetPointer(),
								deWindowCenter.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
				if (nestedds1.FindDataElement(tWindowWidth))
				{
					const mdcm::DataElement & deWindowWidth
						= nestedds1.GetDataElement(tWindowWidth);
					if (!deWindowWidth.IsEmpty() &&
						!deWindowWidth.IsUndefinedLength() &&
						deWindowWidth.GetByteValue())
						fg.window_width =
							QString::fromLatin1(
								deWindowWidth.GetByteValue()->GetPointer(),
								deWindowWidth.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
				if (nestedds1.FindDataElement(tLUTFunction))
				{
					const mdcm::DataElement & deLUTFunction
						= nestedds1.GetDataElement(tLUTFunction);
					if (!deLUTFunction.IsEmpty() &&
						!deLUTFunction.IsUndefinedLength() &&
						deLUTFunction.GetByteValue())
						fg.lut_function =
							QString::fromLatin1(
								deLUTFunction.GetByteValue()->GetPointer(),
								deLUTFunction.GetByteValue()->GetLength()).
									trimmed().remove(QChar('\0'));
				}
			}
		}
		if (nestedds.FindDataElement(tTemporalPositionSequence))
		{
			const mdcm::DataElement &
				deTemporalPositionSequence
					= nestedds.GetDataElement(tTemporalPositionSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems>
				sqTemporalPositionSequence =
					deTemporalPositionSequence.GetValueAsSQ();
			if (sqTemporalPositionSequence &&
				sqTemporalPositionSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 =
					sqTemporalPositionSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				if (nestedds1.FindDataElement(tTemporalPositionTimeOffset))
					fg.temp_pos_off_ok =
						get_fd_value(
							nestedds1,
							tTemporalPositionTimeOffset,
							&fg.temp_pos_off);
			}
		}
		if (nestedds.FindDataElement(tPlanePositionVolumeSequence))
		{
			const mdcm::DataElement & dePlanePositionVolumeSequence
				= nestedds.GetDataElement(tPlanePositionVolumeSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems>
				sqPlanePositionVolumeSequence =
				dePlanePositionVolumeSequence.GetValueAsSQ();
			if (sqPlanePositionVolumeSequence &&
				sqPlanePositionVolumeSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 =
					sqPlanePositionVolumeSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 =
					item1.GetNestedDataSet();
				std::vector<double> tmp1;
				if (get_fd_values(
						nestedds1,
						tImagePositionVolume,
						tmp1))
				{
					if (tmp1.size()==3)
					{
						fg.vol_pos_ok = true;
						fg.vol_pos[0]=tmp1.at(0);
						fg.vol_pos[1]=tmp1.at(1);
						fg.vol_pos[2]=tmp1.at(2);
					}
				}
			}
		}
		if (nestedds.FindDataElement(tPlaneOrientationVolumeSequence))
		{
			const mdcm::DataElement & dePlaneOrientationVolumeSequence
				= nestedds.GetDataElement(tPlaneOrientationVolumeSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPlaneOrientationVolumeSequence
				= dePlaneOrientationVolumeSequence.GetValueAsSQ();
			if (sqPlaneOrientationVolumeSequence &&
				sqPlaneOrientationVolumeSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 = sqPlaneOrientationVolumeSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				std::vector<double> tmp1;
				if (get_fd_values(nestedds1, tImageOrientationVolume, tmp1))
				{
					if (tmp1.size()==6)
					{
						fg.vol_orient_ok = true;
						fg.vol_orient[0]=tmp1.at(0);
						fg.vol_orient[1]=tmp1.at(1);
						fg.vol_orient[2]=tmp1.at(2);
						fg.vol_orient[3]=tmp1.at(3);
						fg.vol_orient[4]=tmp1.at(4);
						fg.vol_orient[5]=tmp1.at(5);
					}
				}
			}
		}
		if (nestedds.FindDataElement(tPixelValueTransformationSequence))
		{
			const mdcm::DataElement & dePixelValueTransformationSequence
				= nestedds.GetDataElement(tPixelValueTransformationSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sqPixelValueTransformationSequence
				= dePixelValueTransformationSequence.GetValueAsSQ();
			if (sqPixelValueTransformationSequence &&
				sqPixelValueTransformationSequence->GetNumberOfItems()==1)
			{
				const mdcm::Item & item1 = sqPixelValueTransformationSequence->GetItem(1);
				const mdcm::DataSet & nestedds1 = item1.GetNestedDataSet();
				std::vector<double> tmp1;
				std::vector<double> tmp2;
				if (
					get_ds_values(nestedds1, tRescaleIntercept, tmp1) &&
					get_ds_values(nestedds1, tRescaleSlope, tmp2))
				{
					fg.rescale_ok        = true;
					fg.rescale_intercept = tmp1.at(0);
					fg.rescale_slope     = tmp2.at(0);
				}
				QString tmp3;
				if (get_string_value(nestedds1, tRescaleType, tmp3))
					fg.rescale_type = tmp3;
			}
		}
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
	double frametime = 100.0;
	//
	if (ds.FindDataElement(tframeincrementpointer))
	{
		unsigned short group, element;
		char group_[2]; char element_[2];
		const mdcm::DataElement & e = ds.GetDataElement(tframeincrementpointer);
		const mdcm::ByteValue * bv = e.GetByteValue();
		if (bv)
		{
			char * buffer = new char[4];
			const qlonglong length = static_cast<qlonglong>(bv->GetLength());
			if (length==4)
			{
				const bool ok0 = bv->GetBuffer(buffer,4);
				if (ok0)
				{
					group_[0]   = buffer[0]; group_[1]   = buffer[1];
					memcpy(&group,group_,2);
					element_[0] = buffer[2]; element_[1] = buffer[3];
					memcpy(&element,element_,2);
					if (group==0x0018 && element==0x1063)
					{
						if (ds.FindDataElement(tframetime))
						{
							const mdcm::DataElement & e = ds.GetDataElement(tframetime);
							if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
							{
								const QString s = QString::fromLatin1(e.GetByteValue()->GetPointer(),e.GetByteValue()->GetLength());
								bool ok;
								double tmp0 = QVariant(s.trimmed().remove(QChar('\0'))).toDouble(&ok);
								if (ok) frametime = tmp0;
							}
						}
						for (int x = 0; x < dimz; x++) ivariant->frame_times.push_back(frametime);
					}
					else if (group==0x0018 && element==0x1065)
					{
						if (ds.FindDataElement(tframetimes))
						{
							const mdcm::DataElement & e = ds.GetDataElement(tframetimes);
							if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
							{
								const QString s = QString::fromLatin1(e.GetByteValue()->GetPointer(),e.GetByteValue()->GetLength());
								bool ok;
								QStringList l = s.trimmed().remove(QChar('\0')).split(QString("\\"));
								for (int x = 0; x < l.size(); x++)
								{
									double tmp0 = QVariant(l.at(x).trimmed().remove(QChar('\0'))).toDouble(&ok);
									if (ok) ivariant->frame_times.push_back(tmp0);
								}
							}
						}
					}
				}
			}
			if (buffer) delete [] buffer;
		}
	}
	else
	{
		if (ds.FindDataElement(tframetime))
		{
			const mdcm::DataElement & e = ds.GetDataElement(tframetime);
			if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
			{
				const QString s = QString::fromLatin1(e.GetByteValue()->GetPointer(),e.GetByteValue()->GetLength());
				bool ok;
				double tmp0 = QVariant(s.trimmed().remove(QChar('\0'))).toDouble(&ok);
				if (ok) frametime = tmp0;
			}
			for (int x = 0; x < dimz; x++) ivariant->frame_times.push_back(frametime);
		}
	}
}

QString DicomUtils::read_anatomic_sq(const mdcm::DataSet & ds)
{
	const mdcm::Tag tAnatomicRegionSequence(0x0008,0x2218);
	const mdcm::Tag tCodeMeaning(0x0008,0x0104);
	if (ds.FindDataElement(tAnatomicRegionSequence))
	{
		const mdcm::Tag tSpecificCharacterSet(0x0008,0x0005);
		QString charset("");
		if(ds.FindDataElement(tSpecificCharacterSet))
		{
			const mdcm::DataElement & eSpecificCharacterSet =
				ds.GetDataElement(tSpecificCharacterSet);
			if (!eSpecificCharacterSet.IsEmpty() &&
				!eSpecificCharacterSet.IsUndefinedLength() &&
				eSpecificCharacterSet.GetByteValue())
				charset = QString::fromLatin1(
					eSpecificCharacterSet.GetByteValue()->GetPointer(),
					eSpecificCharacterSet.GetByteValue()->GetLength()).trimmed();
		}
		const mdcm::DataElement & e =
			ds.GetDataElement(tAnatomicRegionSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems()==1)
		{
			const mdcm::Item & i = sq->GetItem(1);
			const mdcm::DataSet & nds = i.GetNestedDataSet();
			if (nds.FindDataElement(tCodeMeaning))
			{
				const mdcm::DataElement & e2 =
					nds.GetDataElement(tCodeMeaning);
				if (!e2.IsEmpty() &&
					!e2.IsUndefinedLength() &&
					e2.GetByteValue())
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
	}
	return QString("");
}

QString DicomUtils::read_series_laterality(const mdcm::DataSet & ds)
{
	const mdcm::Tag tlaterality(0x0020,0x0060);
	if(ds.FindDataElement(tlaterality))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tlaterality);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			const QString s = QString::fromLatin1(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			if (!s.isEmpty()) return s.trimmed().remove(QChar('\0'));
		}
	}
	return QString("");
}

QString DicomUtils::read_image_laterality(const mdcm::DataSet & ds)
{
	const mdcm::Tag tilaterality(0x0020,0x0062);
	if(ds.FindDataElement(tilaterality))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tilaterality);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			const QString s = QString::fromLatin1(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			if (!s.isEmpty()) return s.trimmed().remove(QChar('\0'));
		}
	}
	return QString("");
}

QString DicomUtils::read_body_part(const mdcm::DataSet & ds)
{
	const mdcm::Tag tbodypart(0x0018,0x0015);
	if(ds.FindDataElement(tbodypart))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tbodypart);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			const QString s = QString::fromLatin1(
				e.GetByteValue()->GetPointer(),
				e.GetByteValue()->GetLength());
			if (!s.isEmpty()) return s.trimmed().toLower().remove(QChar('\0'));
		}
	}
	return QString("");
}

void DicomUtils::read_ivariant_info_tags(const mdcm::DataSet & ds, ImageVariant * ivariant)
{
	if (ds.IsEmpty()) return;
	if (!ivariant)    return;
	QString charset("");
	const mdcm::Tag tcharset(0x0008,0x0005);
	const mdcm::Tag timagetype(0x0008,0x0008);
	const mdcm::Tag tsop(0x0008,0x0016);
	const mdcm::Tag tstudydate(0x0008,0x0020);
	const mdcm::Tag tstudytime(0x0008,0x0030);
	const mdcm::Tag tseriesdate(0x0008,0x0021);
	const mdcm::Tag tseriestime(0x0008,0x0031);
	const mdcm::Tag tacquisitiondate(0x0008,0x0022);
	const mdcm::Tag tacquisitiontime(0x0008,0x0032);
	const mdcm::Tag tacquisitiondatetime(0x0008,0x002a);
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
	const mdcm::PrivateTag tprivcomment(0x0067,0x01,"ALIZA 001");
	QString charset_tmp;
	if (get_string_value(ds, tcharset, charset_tmp))
		charset = charset_tmp;
	QString imagetype;
	if (get_string_value(ds, timagetype, imagetype))
	{
		imagetype = imagetype.trimmed();
		if (!imagetype.isEmpty())
		{
			QString imagetype_("");
			const QStringList l = imagetype.toLower().split(
				QString("\\"),
				QString::SkipEmptyParts);
			imagetype_.append(QString(
				"<br /><span class='y7'>DICOM image type</span>"
				"<br /><span class='y6'>"));
			for (int x = 0; x < l.size(); x++)
				imagetype_.append(l.at(x) + QString("<br />"));
			if (!l.empty())
				imagetype_.append(QString("</span><br />"));
			ivariant->imagetype = imagetype_;
		}
	}
	QString sop;
	if (get_string_value(ds, tsop, sop))
	{
		ivariant->sop = sop.remove(QChar('\0'));
		mdcm::UIDs uid;
		uid.SetFromUID(ivariant->sop.toLatin1().constData());
		ivariant->iod = QString::fromLatin1(uid.GetName());
	}
	QString studydate;
	if(get_string_value(ds, tstudydate, studydate))
		ivariant->study_date = studydate;
	QString seriesdate;
	if(get_string_value(ds,tseriesdate,seriesdate))
		ivariant->series_date = seriesdate;
	QString studytime;
	if(get_string_value(ds,tstudytime,studytime))
		ivariant->study_time = studytime;
	QString seriestime;
	if(get_string_value(ds,tseriestime,seriestime))
		ivariant->series_time = seriestime;
	//
	// don't override for enhanced
	if (
		ivariant->acquisition_date.isEmpty() ||
		ivariant->acquisition_time.isEmpty())
	{
		bool acqdatetime_ok = false;
		QString acquisitiondatetime;
		{
			if(get_string_value(
					ds,
					tacquisitiondatetime,
					acquisitiondatetime))
			{
				acquisitiondatetime = acquisitiondatetime.trimmed();
				if (acquisitiondatetime.size() >= 14)
				{
					ivariant->acquisition_date =
						acquisitiondatetime.left(8);
					ivariant->acquisition_time =
						acquisitiondatetime.right(
							acquisitiondatetime.size() - 8);
					acqdatetime_ok = true;
				}
			}
		}
		if (!acqdatetime_ok)
		{
			QString acquisitiondate;
			QString acquisitiontime;
			if(
				get_string_value(
					ds,
					tacquisitiondate,
					acquisitiondate) &&
				get_string_value(
					ds,
					tacquisitiontime,
					acquisitiontime))
			{
				ivariant->acquisition_date = acquisitiondate.trimmed();
				ivariant->acquisition_time = acquisitiontime.trimmed();
			}
		}
	}
	//
	QString modality;
	if (get_string_value(ds,tmodality,modality))
		ivariant->modality = modality.remove(QChar('\0'));
	if(ds.FindDataElement(tmanufacturer))
	{
		QString manufacturer_s;
		QString model_s;
		const mdcm::DataElement & e =
			ds.GetDataElement(tmanufacturer);
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
			if(ds.FindDataElement(tmodel))
			{
				const mdcm::DataElement & e1 =
					ds.GetDataElement(tmodel);
				if (!e1.IsEmpty() &&
					!e1.IsUndefinedLength() &&
					e1.GetByteValue())
				{
					QByteArray ba1(
						e1.GetByteValue()->GetPointer(),
						e1.GetByteValue()->GetLength());
					const QString tmp1 =
						CodecUtils::toUTF8(
							&ba1, charset.toLatin1().constData());
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
			ivariant->hardware = manufacturer_s;
	}
	if(ds.FindDataElement(tinstituion))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tinstituion);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			ivariant->institution = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	if(ds.FindDataElement(tstudydesc))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tstudydesc);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			ivariant->study_description = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	if(ds.FindDataElement(tseriesdesc))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tseriesdesc);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			ivariant->series_description = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	if(ds.FindDataElement(tpatientname))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tpatientname);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty()) ivariant->pat_name = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	if(ds.FindDataElement(tpatientid))
	{
		const mdcm::DataElement & e = ds.GetDataElement(tpatientid);
		if (!e.IsEmpty() && !e.IsUndefinedLength() && e.GetByteValue())
		{
			QByteArray ba(e.GetByteValue()->GetPointer(), e.GetByteValue()->GetLength());
			const QString tmp0 = CodecUtils::toUTF8(&ba, charset.toLatin1().constData());
			if (!tmp0.isEmpty()) ivariant->pat_id = tmp0.trimmed().remove(QChar('\0'));
		}
	}
	QString birthdate;
	if (get_string_value(ds,tbirthdate,birthdate))
		ivariant->pat_birthdate =
			birthdate.remove(QChar('\0'));
	QString patweight;
	if (get_string_value(ds,tpatweight,patweight))
		ivariant->pat_weight = patweight;
	QString sex;
	if (get_string_value(ds,tsex,sex))
		ivariant->pat_sex = sex.remove(QChar('\0'));
	QString fieldstrength;
	if (get_string_value(ds,tfieldstrength,fieldstrength))
	{
		fieldstrength = fieldstrength.trimmed();
		bool conv_ok = false;
		const double fieldstrength_ =
			QVariant(fieldstrength).toDouble(&conv_ok);
		if (conv_ok)
		{
			fieldstrength.prepend(
				QString("Magnet field strength "));
			if (fieldstrength_ < 50.0)
				fieldstrength.append(QString(" T")); // FIXME
			ivariant->hardware_info = fieldstrength;
		}
	}
	QString studyuid;
	if (get_string_value(ds,tstudyuid,studyuid))
		ivariant->study_uid = studyuid.remove(QChar('\0'));
	QString seriesuid;
	if (get_string_value(ds,tseriesuid,seriesuid))
		ivariant->series_uid = seriesuid.remove(QChar('\0'));
	QString studyid;
	if (get_string_value(ds,tstudyid,studyid))
		ivariant->study_id = studyid.remove(QChar('\0'));
	QString frame_of_refuid;
	if (get_string_value(ds,tframe_of_refuid,frame_of_refuid))
		ivariant->frame_of_ref_uid =
			frame_of_refuid.remove(QChar('\0'));
	if (ds.FindDataElement(tcomment))
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
				ivariant->comment = tmp0.trimmed().
					remove(QChar('\0'));
		}
	}
	QString interpretation;
	if (get_string_value(ds,tinterpretation,interpretation))
		ivariant->interpretation =
			interpretation.remove(QChar('\0'));
	if(ds.FindDataElement(tprivcomment))
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
}

bool DicomUtils::get_patient_position(
	const QString & p,
	double * pp) 
{
	if (pp==NULL || p.isEmpty()) return false;
	const QString tmp0 = p.trimmed().
		remove(QChar('\0'));
	const QStringList list =
		tmp0.split(QString("\\"), QString::SkipEmptyParts);
	if (list.size() == 3)
	{
		bool ok = false;
		pp[0] = list.at(0).toDouble(&ok);
		if (!ok) pp[0] = (double)list.at(0).toInt(&ok);
		if (!ok) return false;
		pp[1] = list.at(1).toDouble(&ok);
		if (!ok) pp[1] = (double)list.at(1).toInt(&ok);
		if (!ok) return false;
		pp[2] = list.at(2).toDouble(&ok);
		if (!ok) pp[2] = (double)list.at(2).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	return false;
}

bool DicomUtils::get_patient_orientation(
	const QString & o,
	double * po) 
{
	if (po==NULL || o.isEmpty()) return false;
	const QString tmp0 = QString(o.trimmed()).
		remove(QChar('\0'));
	const QStringList list =
		tmp0.split(QString("\\"), QString::SkipEmptyParts);
	if (list.size() == 6)
	{
		bool ok = false;
		po[0] = list.at(0).toDouble(&ok);
		if (!ok) po[0] = (double)list.at(0).toInt(&ok);
		if (!ok) return false;
		po[1] = list.at(1).toDouble(&ok);
		if (!ok) po[1] = (double)list.at(1).toInt(&ok);
		if (!ok) return false;
		po[2] = list.at(2).toDouble(&ok);
		if (!ok) po[2] = (double)list.at(2).toInt(&ok);
		if (!ok) return false;
		po[3] = list.at(3).toDouble(&ok);
		if (!ok) po[3] = (double)list.at(3).toInt(&ok);
		if (!ok) return false;
		po[4] = list.at(4).toDouble(&ok);
		if (!ok) po[4] = (double)list.at(4).toInt(&ok);
		if (!ok) return false;
		po[5] = list.at(5).toDouble(&ok);
		if (!ok) po[5] = (double)list.at(5).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	return false;
}

bool DicomUtils::get_pixel_spacing(
	const QString & s,
	double * ps)
{
	const QString tmp0 =
		QString(s.trimmed()).remove(QChar('\0'));
	const QStringList list =
		tmp0.split(QString("\\"));
	bool ok = false;
	if (list.size()==1)
	{
		ps[0] = ps[1] = list.at(0).toDouble(&ok);
		if (!ok) ps[0] = ps[1] =
			(double)list.at(0).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	else if (list.size()==2)
	{
		ps[0] = list.at(0).toDouble(&ok);
		if (!ok) ps[0] = (double)list.at(0).toInt(&ok);
		if (!ok) return false;
		ps[1] = list.at(1).toDouble(&ok);
		if (!ok) ps[1] = (double)list.at(1).toInt(&ok);
		if (!ok) return false;
		return true;
	}
	return false;
}

bool DicomUtils::generate_geometry(
		std::vector<ImageSlice*> & cubeslices,
		std::vector<SpectroscopySlice*> & spectorscopyslices,
		const std::vector<double*> & values,
		const unsigned int rows_, const unsigned int columns_,
		const double spacing_x, const double spacing_y, double * spacing_z,
		const bool ok3d, const bool skip_texture, GLWidget * gl,
		bool * equi_,
		bool * one_direction_,
		double * origin_x,  double * origin_y,  double * origin_z,
		double * dircos,
		float * slices_dir_x,float * slices_dir_y,float * slices_dir_z,
		float * up_dir_x,float * up_dir_y,float * up_dir_z,
		float * center_x,float * center_y,float * center_z,
		float tolerance,
		const bool spectroscopy)
{
	const unsigned int size_ = values.size();
	if (size_ < 1) return false;
	sVector3 first = sVector3(0.0f,0.0f,0.0f);
	sVector3 last  = sVector3(0.0f,0.0f,0.0f);
	sVector3 v0 = sVector3(0.0f,0.0f,0.0f);
	sVector3 v1 = sVector3(0.0f,0.0f,0.0f);
	sVector3 up = sVector3(0.0f,0.0f,0.0f);
	QString tmp0("");
	bool tmp1 = true, tmp2 = true;
	sVector3 tmp_p0 = sVector3(0.0f,0.0f,0.0f);
	sVector3 tmp_p1 = sVector3(0.0f,0.0f,0.0f);
	sVector3 tmp_p2 = sVector3(0.0f,0.0f,0.0f);
	sVector3 tmp_p3 = sVector3(0.0f,0.0f,0.0f);
	float tmp_length0=0.0f, tmp_length1=0.0f, tmp_length2=0.0f, tmp_length3=0.0f;
	double spacing_z_ = 0;
	bool invalidate_volume = false;
	for (unsigned int i = 0; i < size_; i++)
	{
		const double * ipp_iop = values.at(i);
		const sVector4 c0 = sVector4(
					(float)ipp_iop[3]*spacing_x,
					(float)ipp_iop[4]*spacing_x,
					(float)ipp_iop[5]*spacing_x,
					0.0f);
		const sVector4 c1 = sVector4(
					(float)ipp_iop[6]*spacing_y,
					(float)ipp_iop[7]*spacing_y,
					(float)ipp_iop[8]*spacing_y,
					0.0f);
		const sVector4 c2 = sVector4(0.0f,0.0f,0.0f,0.0f);
		const sVector4 c3 = sVector4(
					(float)ipp_iop[0],
					(float)ipp_iop[1],
					(float)ipp_iop[2],
					1.0f);
		sMatrix4 m0 = sMatrix4::identity();
		m0.setCol0(c0);
		m0.setCol1(c1);
		m0.setCol2(c2);
		m0.setCol3(c3);
		const sVector4 ind0 = sVector4(0.0f,(float)(rows_-1),0.0f,1.0f);
		const sVector4 ind1 = sVector4(0.0f,0.0f,0.0f,1.0f);
		const sVector4 ind2 = sVector4((float)(columns_-1),(float)(rows_-1),0.0f,1.0f);
		const sVector4 ind3 = sVector4((float)(columns_-1),0.0f,0.0f,1.0f);
		const sVector4 p0 = sVector4(m0*ind0);
		const sVector4 p1 = sVector4(m0*ind1);
		const sVector4 p2 = sVector4(m0*ind2);
		const sVector4 p3 = sVector4(m0*ind3);
		const float x0 = p0.getX(), y0 = p0.getY(), z0 = p0.getZ();
		const float x1 = p1.getX(), y1 = p1.getY(), z1 = p1.getZ();
		const float x2 = p2.getX(), y2 = p2.getY(), z2 = p2.getZ();
		const float x3 = p3.getX(), y3 = p3.getY(), z3 = p3.getZ();
		const QString orientation_string = CommonUtils::get_orientation2(&ipp_iop[3]);
		if (i==0)
		{
			first = sVector3(x0,y0,z0);
			v0 = (p0.getXYZ()+p3.getXYZ())*0.5f;
			sVector3 tmp_up0 = sVector3(x1,y1,z1);
			sVector3 tmp_up1 = sVector3(x0,y0,z0);
			up = sVector3(normalize(tmp_up1-tmp_up0));
			*origin_x = ipp_iop[0];
			*origin_y = ipp_iop[1];
			*origin_z = ipp_iop[2];
			for (int j = 0; j < 6; j++) dircos[j] = ipp_iop[3+j];
		}
		if (i==size_-1)
		{
			last = sVector3(x0,y0,z0);
			v1 = (p0.getXYZ()+p3.getXYZ())*0.5f;
		}
		if (spectroscopy)
		{
			CommonUtils::generate_spectroscopyslice(
				spectorscopyslices,
				orientation_string,
				ok3d, gl,
				x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3,
				columns_, rows_);
		}
		else
		{
			CommonUtils::generate_cubeslice(
				cubeslices,
				orientation_string,
				size_, i,
				x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3,
				ipp_iop);
		}
		const float length0 = length(p0.getXYZ()-tmp_p0);
		const float length1 = length(p1.getXYZ()-tmp_p1);
		const float length2 = length(p2.getXYZ()-tmp_p2);
		const float length3 = length(p3.getXYZ()-tmp_p3);
		// check orientation
		if (i!=0)
		{
			if (tmp1)
			{
				if (tmp0!=orientation_string) tmp1 = false;
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
		if (i>=2)
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
		if (i==size_-1) spacing_z_ = static_cast<double>(length0);
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
	const float row_dircos_x = (float)dircos[0];
	const float row_dircos_y = (float)dircos[1];
	const float row_dircos_z = (float)dircos[2];
	const float col_dircos_x = (float)dircos[3];
	const float col_dircos_y = (float)dircos[4];
	const float col_dircos_z = (float)dircos[5];
	const float nrm_dircos_x = row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y; 
	const float nrm_dircos_y = row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const float nrm_dircos_z = row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	const sVector3 direction1 = normalize(sVector3(nrm_dircos_x,nrm_dircos_y,nrm_dircos_z));
	if (size_ > 1)
	{
		const sVector3 direction0_tmp = sVector3(last - first);
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
			if (
				tmp1 &&
				((tmp2 && (size_ > 2))||size_ == 2) &&
				!(
				(direction0.getX()<direction1.getX()+0.001f && direction0.getX()>direction1.getX()-0.001f) &&
				(direction0.getY()<direction1.getY()+0.001f && direction0.getY()>direction1.getY()-0.001f) &&
				(direction0.getZ()<direction1.getZ()+0.001f && direction0.getZ()>direction1.getZ()-0.001f)
				))
			{
				invalidate_volume = true;
				std::cout
					<< "Warning: orientation is not reliable -\n"
					<< " cosines defined in DICOM: "
					<< row_dircos_x << "\\" << row_dircos_y << "\\" << row_dircos_z << "\\"
					<< col_dircos_x << "\\" << col_dircos_y << "\\" << col_dircos_z << "\n"
					<< " z direction calculated from defined cosines: "
					<< direction1.getX() << "," << direction1.getY() << "," << direction1.getZ() << "\n"
					<< " z direction from geometry (real): "
					<< direction0.getX() << "," << direction0.getY() << "," << direction0.getZ() << "\n"
					<< " ... using image as non-uniform.\n"
					<< std::endl;
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
		if      (size_ >  2) { *equi_ = (tmp1 && tmp2); }
		else if (size_ == 2) { *equi_ = tmp1; }
	}
	//
	*up_dir_x = up.getX();
	*up_dir_y = up.getY();
	*up_dir_z = up.getZ();
	if (*equi_==true)
	{
		const sVector3 cube_center = sVector3((v0+v1)*0.5f);
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
	*spacing_z = (size_>1) ? spacing_z_ : 1;
	return true;
}

void DicomUtils::enhanced_get_indices(
	const DimIndexSq & sq,
	int * dim6th,
	int * dim5th,
	int * dim4th,
	int * dim3rd,
	int * enh_id)
{
	int stack_id_idx      = -1;
	int in_stack_pos_idx  = -1;
	int temporal_pos_idx  = -1;
	int contrast_idx      = -1;
	int b_value_idx       = -1;
	int gradients_idx     = -1;
	int lut_label_idx     = -1;
	int temporal_idx      = -1;
	int plane_pos_idx     = -1;
	int datatype_idx      = -1;
	int mr_frame_type_idx = -1;
	int mr_eff_echo_idx   = -1;
	const unsigned int sq_size = sq.size();
	for (unsigned int x = 0; x < sq_size; x++)
	{
		if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9056))
		{
			stack_id_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9057))
		{
			in_stack_pos_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9128))
		{
			temporal_pos_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9117) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9087))
		{
			b_value_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9117) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9089))
		{
			gradients_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9341)) // TODO check
		{
			contrast_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0040,0x9096) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0040,0x9210))
		{
			lut_label_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9310) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x930d))
		{
			temporal_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x930e) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9301))
		{
			plane_pos_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9807) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9808))
		{
			datatype_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9226) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0008,0x9007))
		{
			mr_frame_type_idx = (int)x;
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9114) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9082))
		{
			mr_eff_echo_idx = (int)x;
		}
	}
	//
	//
	// workaround to guess temporar tag, e.g. ultrasound
	if (sq_size==3 &&
		temporal_idx<0 &&
		plane_pos_idx==1 &&
		datatype_idx==2)
	{
		temporal_idx = 0;
	}
	//
	//
	//
	// well-known combinations
	if (sq_size==3 &&
		temporal_idx>=0 &&
		plane_pos_idx>=0 &&
		datatype_idx>=0)
	{
		*dim5th = datatype_idx;
		*dim4th = temporal_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 101;
	}
	else if (sq_size==2 &&
		plane_pos_idx>=0 &&
		datatype_idx>=0)
	{
		*dim4th = datatype_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 103;
	}
	else if (sq_size==2 &&
		plane_pos_idx>=0 &&
		temporal_idx>=0)
	{
		*dim4th = temporal_idx;
		*dim3rd = plane_pos_idx;
		*enh_id = 104;
	}
	else if (sq_size==1 &&
		temporal_idx==0)
	{
		*dim3rd = temporal_idx;
		*enh_id = 105;
	}
	else if (sq_size==1 &&
		plane_pos_idx==0)
	{
		*dim3rd = plane_pos_idx;
		*enh_id = 106;
	}
	// generic
	else if (sq_size==1 &&
		in_stack_pos_idx==0)
	{
		*enh_id = 1;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==2 &&
		in_stack_pos_idx>=0 &&
		stack_id_idx>=0)
	{
		*enh_id = 2;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	// temporal
	else if (sq_size==3 &&
		stack_id_idx>=0 &&
		in_stack_pos_idx>=0 &&
		temporal_pos_idx>=0)
	{
		*enh_id = 3;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==2 &&
		temporal_pos_idx>=0 &&
		stack_id_idx>=0)
	{
		*enh_id = 4;
		*dim4th = stack_id_idx;
		*dim3rd = temporal_pos_idx;
	}
	else if (sq_size==1 &&
		temporal_pos_idx==0)
	{
		*enh_id = 5;
		*dim3rd = temporal_pos_idx;
	}
	// contrast
	else if (sq_size==4 &&
		temporal_pos_idx>=0 &&
		in_stack_pos_idx>=0 &&
		stack_id_idx>=0 &&
		contrast_idx>=0)
	{
		*enh_id = 6;
		*dim6th = contrast_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==3 &&
		in_stack_pos_idx>=0 &&
		stack_id_idx>=0 &&
		contrast_idx>=0)
	{
		*enh_id = 7;
		*dim5th = contrast_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==2 &&
		in_stack_pos_idx>=0 &&
		contrast_idx>=0)
	{
		*enh_id = 8;
		*dim4th = contrast_idx;
		*dim3rd = in_stack_pos_idx;
	}
	// mr frame type
	else if (sq_size==4 &&
		stack_id_idx>=0 &&
		in_stack_pos_idx>=0 &&
		temporal_pos_idx>=0 &&
		mr_frame_type_idx>=0)
	{
		*enh_id = 9;
		*dim6th = mr_frame_type_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==3 &&
		stack_id_idx>=0 &&
		in_stack_pos_idx>=0 &&
		mr_frame_type_idx>=0)
	{
		*enh_id = 10;
		*dim5th = mr_frame_type_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	// mr eff. echo
	else if (sq_size==4 &&
		stack_id_idx>=0 &&
		in_stack_pos_idx>=0 &&
		temporal_pos_idx>=0 &&
		mr_eff_echo_idx>=0)
	{
		*enh_id = 11;
		*dim6th = mr_eff_echo_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==3 &&
		stack_id_idx>=0 &&
		in_stack_pos_idx>=0 &&
		mr_eff_echo_idx>=0)
	{
		*enh_id = 12;
		*dim5th = mr_eff_echo_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	// diffusion
	else if (sq_size==4 &&
		stack_id_idx>=0 && 
		in_stack_pos_idx>=0 &&
		b_value_idx>=0 &&
		gradients_idx>=0)
	{
		*enh_id = 13;
		*dim6th = b_value_idx;
		*dim5th = gradients_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==3 &&
		in_stack_pos_idx>=0 &&
		b_value_idx>=0 &&
		gradients_idx>=0)
	{
		*enh_id = 14;
		*dim5th = b_value_idx;
		*dim4th = gradients_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==3 &&
		stack_id_idx>=0 &&
		in_stack_pos_idx>=0 &&
		gradients_idx>=0)
	{
		*enh_id = 15;
		*dim5th = gradients_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==3 &&
		b_value_idx>=0 &&
		in_stack_pos_idx>=0 &&
		stack_id_idx>=0)
	{
		*enh_id = 16;
		*dim5th = b_value_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	// LUT
	else if (sq_size==4 &&
		stack_id_idx>=0 &&
		in_stack_pos_idx>=0 &&
		lut_label_idx>=0 &&
		temporal_pos_idx>=0)
	{
		*enh_id = 17;
		*dim6th = lut_label_idx;
		*dim5th = temporal_pos_idx;
		*dim4th = stack_id_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else if (sq_size==3 &&
		in_stack_pos_idx>=0 &&
		lut_label_idx>=0 &&
		temporal_pos_idx>=0)
	{
		*enh_id = 18;
		*dim5th = lut_label_idx;
		*dim4th = temporal_pos_idx;
		*dim3rd = in_stack_pos_idx;
	}
	else
	{
		;;
	}
	// not recognized, try generic approach
	if (*enh_id<0)
	{
		if (sq_size==3 &&
			stack_id_idx>=0 &&
			in_stack_pos_idx>=0)
		{
			int dim5th_tmp = -1;
			for (unsigned int x = 0; x < 3; x++)
			{
				if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
					sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9056))
				{
					;;
				}
				else if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
					sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9057))
				{
					;;
				}
				else
				{
					dim5th_tmp = (int)x;
					break;
				}
			}
			*enh_id = 999;
			*dim5th = dim5th_tmp;
			*dim4th = stack_id_idx;
			*dim3rd = in_stack_pos_idx;
		}
		else if (sq_size==4 &&
			stack_id_idx>=0 &&
			in_stack_pos_idx>=0 &&
			temporal_pos_idx>=0)
		{
			int dim6th_tmp = -1;
			for (unsigned int x = 0; x < 4; x++)
			{
				if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
					sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9056))
				{
					;;
				}
				else if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
					sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9057))
				{
					;;
				}
				else if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
					sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9128))
				{
					;;
				}
				else
				{
					dim6th_tmp = (int)x;
					break;
				}
			}
			*enh_id = 998;
			*dim6th = dim6th_tmp;
			*dim5th = temporal_pos_idx;
			*dim4th = stack_id_idx;
			*dim3rd = in_stack_pos_idx;
		}
		else if (sq_size==1 && sq.at(0).size > 1)
		{
			*enh_id = 997;
			*dim3rd = 0;
		}
	}
}

void DicomUtils::enhanced_process_values(
	FrameGroupValues & values,
	const FrameGroupValues & shared_values)
{
	bool vol_pos_miss     = false;
	bool vol_orient_miss  = false;
	bool pat_pos_miss     = false;
	bool pat_orient_miss  = false;
	bool pix_spacing_miss = false;
	bool window_miss      = false;
	bool laterality_miss  = false;
	bool body_part_miss   = false;
	bool rescale_miss     = false;
	for (unsigned int x = 0; x < values.size(); x++)
	{
		if (!values.at(x).vol_pos_ok) vol_pos_miss = true;
		if (!values.at(x).vol_orient_ok) vol_orient_miss = true;
		if (values.at(x).pat_pos.isEmpty()) pat_pos_miss = true;
		if (values.at(x).pat_orient.isEmpty()) pat_orient_miss = true;
		if (values.at(x).pix_spacing.isEmpty()) pix_spacing_miss = true;
		if (
			values.at(x).window_center.isEmpty() ||
			values.at(x).window_width.isEmpty()) window_miss = true;
		if (values.at(x).frame_laterality.isEmpty()) laterality_miss = true;
		if (values.at(x).frame_body_part.isEmpty()) body_part_miss = true;
		if (!values.at(x).rescale_ok) rescale_miss = true;
	}
	if (vol_pos_miss &&
		shared_values.size()==1 &&
		shared_values.at(0).vol_pos_ok)
	{
		for (unsigned int x = 0; x < values.size(); x++)
		{
			values[x].vol_pos[0] = shared_values.at(0).vol_pos[0];
			values[x].vol_pos[1] = shared_values.at(0).vol_pos[1];
			values[x].vol_pos[2] = shared_values.at(0).vol_pos[2];
			values[x].vol_pos_ok = true;
		}
	}
	if (
		vol_orient_miss &&
		shared_values.size()==1 &&
		shared_values.at(0).vol_orient_ok)
	{
		for (unsigned int x = 0; x < values.size(); x++)
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
	if (
		pat_pos_miss &&
		shared_values.size()==1 &&
		!shared_values.at(0).pat_pos.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); x++)
			values[x].pat_pos = shared_values.at(0).pat_pos;
	}
	if (
		pat_orient_miss &&
		shared_values.size()==1 &&
		!shared_values.at(0).pat_orient.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); x++)
			values[x].pat_orient = shared_values.at(0).pat_orient;
	}
	if (
		pix_spacing_miss &&
		shared_values.size()==1 &&
		!shared_values.at(0).pix_spacing.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); x++)
			values[x].pix_spacing = shared_values.at(0).pix_spacing;
	}
	if (
		window_miss && shared_values.size()==1 &&
		!shared_values.at(0).window_center.isEmpty() &&
		!shared_values.at(0).window_width.isEmpty())
	{
		QString lut_function = QString("LINEAR");
		if (!shared_values.at(0).lut_function.isEmpty())
			lut_function = shared_values.at(0).lut_function;
		for (unsigned int x = 0; x < values.size(); x++)
		{
			values[x].window_center = shared_values.at(0).window_center;
			values[x].window_width  = shared_values.at(0).window_width;
			values[x].lut_function  = lut_function;
		}
	}
	if (
		laterality_miss &&
		shared_values.size()==1 &&
		!shared_values.at(0).frame_laterality.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); x++)
			values[x].frame_laterality =
				shared_values.at(0).frame_laterality;
	}
	if (
		body_part_miss &&
		shared_values.size()==1 &&
		!shared_values.at(0).frame_body_part.isEmpty())
	{
		for (unsigned int x = 0; x < values.size(); x++)
			values[x].frame_body_part =
				shared_values.at(0).frame_body_part;
	}
	if (rescale_miss)
	{
		double  rescale_intercept = 0.0;
		double  rescale_slope     = 1.0;
		QString rescale_type("");
		if (
			shared_values.size()==1 &&
			shared_values.at(0).rescale_ok)
		{
			rescale_intercept =
				shared_values.at(0).rescale_intercept;
			rescale_slope =
				shared_values.at(0).rescale_slope;
			rescale_type =
				shared_values.at(0).rescale_type;
		}
		for (unsigned int x = 0; x < values.size(); x++)
		{
			values[x].rescale_intercept =
				rescale_intercept;
			values[x].rescale_slope =
				rescale_slope;
			values[x].rescale_type =
				rescale_type;
		}
	}
}

void DicomUtils::enhanced_check_rescale(
	const mdcm::DataSet & ds,
	FrameGroupValues & v)
{
	bool rescale_miss = false;
	for (unsigned int x = 0; x < v.size(); x++)
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
	const bool ok0 = DicomUtils::get_ds_values(
		ds, tRescaleIntercept, tmp0);
	const bool ok1 = DicomUtils::get_ds_values(
		ds, tRescaleSlope, tmp1);
	if (
		ok0 &&
		ok1 &&
		!tmp0.empty() &&
		!tmp1.empty())
	{
		for (unsigned int x = 0; x < v.size(); x++)
		{
			v.at(x).rescale_intercept = tmp0.at(0);
			v.at(x).rescale_slope     = tmp1.at(0);
			v.at(x).rescale_ok = true;
		}
	}
}

void DicomUtils::print_sq(const DimIndexSq & sq)
{
	if (!sq.empty())
	{
		std::cout << "DimIndexSq:" << std::endl;
	}
	else
	{
		std::cout << "DimIndexSq is empty" << std::endl;
	}
	for (size_t x = 0; x < sq.size(); x++)
	{
		std::cout
			<< " " << x << " "
			<< sq.at(x).group_pointer << " "
			<< sq.at(x).index_pointer << " ";
		if (sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9056))
		{
			std::cout << "stack_id_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9057))
		{
			std::cout << "in_stack_pos_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9111) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9128))
		{
			std::cout << "temporal_pos_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9117) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9087))
		{
			std::cout << "b_value_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9117) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9089))
		{
			std::cout << "gradients_idx";
		}
		else if (sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9341))
		{
			std::cout << "contrast_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0040,0x9096) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0040,0x9210))
		{
			std::cout << "lut_label_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0020,0x9310) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x930d))
		{
			std::cout << "temporal_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0020,0x930e) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0020,0x9301))
		{
			std::cout << "plane_pos_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9807) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9808))
		{
			std::cout << "datatype_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9226) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0008,0x9007))
		{
			std::cout << "mr_frame_type_idx";
		}
		else if (
			sq.at(x).group_pointer==mdcm::Tag(0x0018,0x9114) &&
			sq.at(x).index_pointer==mdcm::Tag(0x0018,0x9082))
		{
			std::cout << "mr_eff_echo_idx";
		}
		std::cout << " " << sq.at(x).size << std::endl;
	}
}

void DicomUtils::print_func_group(const FrameGroupValues & values)
{
	for (unsigned int x = 0; x < values.size(); x++)
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
		if (
			get_is_value(
				ds,
				tShutterLeftVerticalEdge,
				&ShutterLeftVerticalEdge) &&
			get_is_value(
				ds,
				tShutterRightVerticalEdge,
				&ShutterRightVerticalEdge) &&
			get_is_value(
				ds,
				tShutterUpperHorizontalEdge,
				&ShutterUpperHorizontalEdge) &&
			get_is_value(
				ds,
				tShutterLowerHorizontalEdge,
				&ShutterLowerHorizontalEdge))
		{
			a.ShutterLeftVerticalEdge    = ShutterLeftVerticalEdge;
			a.ShutterRightVerticalEdge   = ShutterRightVerticalEdge;
			a.ShutterUpperHorizontalEdge = ShutterUpperHorizontalEdge;
			a.ShutterLowerHorizontalEdge = ShutterLowerHorizontalEdge;
		}
		std::vector<int> CenterofCircularShutter;
		int RadiusofCircularShutter;
		if (
			get_is_values(
				ds,
				tCenterofCircularShutter,
				CenterofCircularShutter) &&
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
		if (get_is_values(
				ds,
				tVerticesofthePolygonalShutter,
				VerticesofthePolygonalShutter) &&
			(VerticesofthePolygonalShutter.size() > 1) &&
			(VerticesofthePolygonalShutter.size()%2 == 0))
		{
			for (unsigned int x15 = 0;
				x15 < VerticesofthePolygonalShutter.size();
				x15++)
			{
				a.VerticesofthePolygonalShutter.push_back(
					VerticesofthePolygonalShutter.at(x15));
			}
		}
		unsigned short ShutterPresentationValue;
		if (get_us_value(
				ds,
				tShutterPresentationValue,
				&ShutterPresentationValue))
		{
			a.ShutterPresentationValue = (int)ShutterPresentationValue;
		}
		std::vector<unsigned short> ShutterPresentationColorCIELabValue;
		if (get_us_values(
				ds,
				tShutterPresentationColorCIELabValue,
				ShutterPresentationColorCIELabValue) &&
			(ShutterPresentationColorCIELabValue.size()==3))
		{
			a.ShutterPresentationColorCIELabValue_L =
				(int)ShutterPresentationColorCIELabValue.at(0);
			a.ShutterPresentationColorCIELabValue_a =
				(int)ShutterPresentationColorCIELabValue.at(1);
			a.ShutterPresentationColorCIELabValue_b =
				(int)ShutterPresentationColorCIELabValue.at(2);
		}
		return true;
	}
	return false;
}

QString DicomUtils::read_enhanced(
	bool * ok,
	const QString & f,
	std::vector<ImageVariant*> & ivariants,
	int max_3d_tex_size, GLWidget * gl, bool ok3d,
	bool min_load,
	bool enh_original_frames,
	const QWidget * settings, QProgressDialog * pb,
	float tolerance,
	bool apply_rescale)
{
	*ok = false;
	QString message_;
	if (!settings) return QString("settings==NULL");
	const SettingsWidget * wsettings =
		static_cast<const SettingsWidget*>(settings);
	std::vector<char*> data;
	DimIndexSq sq;
	DimIndexValues idx_values;
	FrameGroupValues values;
	FrameGroupValues shared_values;
	ImageOverlays image_overlays;
	itk::Matrix<itk::SpacePrecisionType,3,3> direction;
	mdcm::PhotometricInterpretation pi;
	mdcm::PixelFormat pixelformat;
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
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
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	unsigned short rows_ = 0, columns_ = 0;
	const mdcm::Tag tRows(0x0028,0x0010);
	const bool rows_ok = get_us_value(ds,tRows,&rows_);
	const mdcm::Tag tColumns(0x0028,0x0011);
	const bool cols_ok = get_us_value(ds,tColumns,&columns_);
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	QString sop("");
	QString sop_tmp("");
	QString iod("");
	const mdcm::Tag tSOPClassUID(0x0008,0x0016);
	const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
	const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
	if (get_string_value(ds,tSOPClassUID,sop_tmp))
	{
		sop = sop_tmp.remove(QChar('\0'));
		mdcm::UIDs uid;
		uid.SetFromUID(sop.toLatin1().constData());
		iod = QString::fromLatin1(uid.GetName());
	}
	read_dimension_index_sq(ds, sq);
	//
	const size_t sq_size = sq.size();
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
			"Functional groups not found,\n"
			"probably broken DICOM file.");
	}
	if (ok_f && idx_values.size()==values.size())
	{
		for (size_t x = 0; x < sq_size; x++)
		{
			std::list<unsigned int> tmpl;
			for (size_t j = 0; j < idx_values.size(); j++)
				tmpl.push_back(idx_values.at(j).idx.at(x));
			tmpl.sort();
			tmpl.unique();
			sq[x].size = tmpl.size();
		}
	}
#ifdef ENHANCED_PRINT_INFO
	if (!min_load)
	{
		print_sq(sq);
		std::cout << std::endl;
		std::cout << "Group values:" << std::endl;
		print_func_group(values);
		std::cout << "Shared group values:" << std::endl;
		print_func_group(shared_values);
	}
#endif
	enhanced_process_values(values, shared_values);
	enhanced_check_rescale(ds, values);
	//
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	double dircos_read[] = {0.0,0.0,0.0,0.0,0.0,0.0};
	unsigned int dimx_read, dimy_read, dimz_read;
	double origin_x_read,  origin_y_read,  origin_z_read;
	double spacing_x_read, spacing_y_read, spacing_z_read;
	double unsused0 = 0.0, unsused1 = 1.0;
	AnatomyMap empty_;
	// do not use GDCM's rescale for enhanced, except for US
	const bool rescale_tmp =
		(sop==QString("1.2.840.10008.5.1.4.1.1.6.2"))
		? wsettings->get_rescale()
		: false;
	message_ =
		read_buffer(
			ok, data,
			image_overlays, -1,
			empty_, -1, // unused in enhanced
			f,
			rescale_tmp,
			pixelformat, pi,
			&dimx_read, &dimy_read, &dimz_read,
			&origin_x_read,  &origin_y_read,  &origin_z_read,
			&spacing_x_read, &spacing_y_read, &spacing_z_read,
			dircos_read,
			&unsused0,
			&unsused1, // GDCM's rescale, unused
			clean_unused_bits,
			false, false, false,
			false, NULL);
	if (*ok==false) return message_;
	if (
		rows_ok && cols_ok &&
		(dimx_read!=columns_||dimy_read!=rows_))
	{
		*ok=false; 
		for (unsigned int x=0; x < data.size(); x++)
		{
			if (data.at(x)) delete [] data[x];
		}
		data.clear();
		return QString(
			"dimx_read!=columns_||dimy_read!=rows_");
	}
	if (dimz_read!=data.size())
	{
		*ok=false;
		for (unsigned int x=0; x < data.size(); x++)
		{
			if (data.at(x)) delete [] data[x];
		}
		data.clear();
		return QString("dimz_read!=data.size()");
	}
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	//
	//
	//
	bool tmp17 = false;
	int dim6th = -1;
	int dim5th = -1;
	int dim4th = -1;
	int dim3rd = -1;
	int enh_id = -1;
	if (!idx_values.empty() && !values.empty())
	{
		enhanced_get_indices(
			sq,
			&dim6th, &dim5th, &dim4th, &dim3rd,
			&enh_id);
	}
	else if (idx_values.empty() && !values.empty())
	{
		// stack id/position number without dimension organisation?
		// try to re-build
		const bool is_legacy = (
			sop == QString("1.2.840.10008.5.1.4.1.1.2.2") ||
			sop == QString("1.2.840.10008.5.1.4.1.1.4.4") ||
			sop == QString("1.2.840.10008.5.1.4.1.1.128.1"))
			?
			true : false;
		if (!is_legacy)
		{
			// don't rebuld for Legacy IODs, many files are wrong,
			// try frames one by one
			bool idx_values_rebuild = false;
			bool tmp12 = false;
			DimIndexValues idx_values_tmp;
			for (unsigned int x = 0; x < values.size(); x++)
			{
				if (!(
					values.at(x).stack_id_ok &&
					values.at(x).in_stack_pos_num_ok))
				{
					tmp12 = true;
					break;
				}
			}
			if (!tmp12)
			{
				for (unsigned int x = 0; x < values.size(); x++)
				{
					DimIndexValue tmp13;
					tmp13.id = values.at(x).id;
					tmp13.idx.push_back(values.at(x).stack_id);
					tmp13.idx.push_back(values.at(x).in_stack_pos_num);
					idx_values_tmp.push_back(tmp13);
				}
				for (unsigned int x = 0; x < idx_values_tmp.size(); x++)
				{
					idx_values.push_back(idx_values_tmp[x]);
				}
				idx_values_tmp.clear();
				idx_values_rebuild = true;
#ifdef ENHANCED_PRINT_INFO
				if (!min_load)
					std::cout
						<< "stack id and position without dim. org."
						<< std::endl;
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
			<< " dim6th=" << dim6th
			<< " dim5th=" << dim5th
			<< " dim4th=" << dim4th
			<< " dim3rd=" << dim3rd
			<< std::endl;
	}
#endif
	message_ = read_enhanced_3d_6d(
		&tmp17, ivariants, sop, f,
		data,
		image_overlays,
		rows_, columns_, pixelformat, pi,
		dim6th, dim5th, dim4th, dim3rd,
		idx_values, values,
		ok3d, max_3d_tex_size, gl,
		min_load,
		settings,
		dircos_read,
		INT_MIN,
		spacing_x_read, spacing_y_read, spacing_z_read,
		origin_x_read, origin_y_read, origin_z_read,
		unsused0, unsused1,
		apply_rescale,
		ds,
		pb,
		tolerance);
#ifdef ENHANCED_PRINT_INFO
	if (!min_load && !message_.isEmpty())
		std::cout << message_.toStdString() << std::endl;
#endif
	if (!tmp17 &&
		!(
			dim6th == -1 &&
			dim5th == -1 &&
			dim4th == -1 &&
			dim3rd == -1))
	{
#ifdef ENHANCED_PRINT_INFO
		if (!min_load)
			std::cout << "  Fallback" << std::endl;
#endif
		message_ = read_enhanced_3d_6d(
			&tmp17, ivariants, sop, f,
			data,
			image_overlays,
			rows_, columns_, pixelformat, pi,
			-1, -1, -1, -1,
			idx_values, values,
			ok3d, max_3d_tex_size, gl,
			min_load,
			settings,
			dircos_read,
			INT_MIN,
			spacing_x_read, spacing_y_read, spacing_z_read,
			origin_x_read,  origin_y_read,  origin_z_read,
			unsused0, unsused1,
			apply_rescale,
			ds,
			pb,
			tolerance);
	}
	*ok = tmp17;
	for (unsigned int x=0; x < data.size(); x++)
	{
		if (data.at(x))
		{
			delete [] data[x];
			data[x] = NULL;
		}
	}
	data.clear();
	return message_;
}

QString DicomUtils::read_enhanced_supp_palette(
	bool * ok,
	const QString & f,
	std::vector<ImageVariant*> & ivariants,
	int max_3d_tex_size, GLWidget * gl, bool ok3d,
	bool min_load,
	bool enh_original_frames,
	const QWidget * settings, QProgressDialog * pb,
	float tolerance)
{
	*ok = false;
	QString message_;
	if (!settings) return QString("settings==NULL");
	const SettingsWidget * wsettings =
		static_cast<const SettingsWidget*>(settings);
	std::vector<char*> data;
	DimIndexSq sq;
	DimIndexValues idx_values;
	FrameGroupValues values;
	FrameGroupValues shared_values;
	ImageOverlays image_overlays; // unused
	itk::Matrix<itk::SpacePrecisionType,3,3> direction;
	mdcm::PhotometricInterpretation pi;
	mdcm::PixelFormat pixelformat;
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
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
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	unsigned short rows_ = 0, columns_ = 0;
	const mdcm::Tag tRows(0x0028,0x0010);
	const bool rows_ok = get_us_value(ds,tRows,&rows_);
	const mdcm::Tag tColumns(0x0028,0x0011);
	const bool cols_ok = get_us_value(ds,tColumns,&columns_);
	QString sop("");
	QString sop_tmp("");
	QString iod("");
	const mdcm::Tag tSOPClassUID(0x0008,0x0016);
	const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
	const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
	if (get_string_value(ds,tSOPClassUID,sop_tmp))
	{
		sop = sop_tmp.remove(QChar('\0'));
		mdcm::UIDs uid;
		uid.SetFromUID(sop.toLatin1().constData());
		iod = QString::fromLatin1(uid.GetName());
	}
	read_dimension_index_sq(ds, sq);
	//
	const size_t sq_size = sq.size();
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
			"Functional groups not found,\n"
			"probably broken DICOM file.");
	}
	if (ok_f && idx_values.size()==values.size())
	{
		for (size_t x = 0; x < sq_size; x++)
		{
			std::list<unsigned int> tmpl;
			for (size_t j = 0; j < idx_values.size(); j++)
				tmpl.push_back(idx_values.at(j).idx.at(x));
			tmpl.sort();
			tmpl.unique();
			sq[x].size = tmpl.size();
		}
	}
#ifdef ENHANCED_PRINT_INFO
	if (!min_load)
	{
		print_sq(sq);
		std::cout << std::endl;
		std::cout << "Group values:" << std::endl;
		print_func_group(values);
		std::cout << "Shared group values:" << std::endl;
		print_func_group(shared_values);
	}
#endif
	enhanced_process_values(values, shared_values);
	//
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	double dircos_read[] = {0.0,0.0,0.0,0.0,0.0,0.0};
	unsigned int dimx_read = 0, dimy_read = 0, dimz_read = 0;
	double origin_x_read = 0, origin_y_read = 0, origin_z_read = 0;
	double spacing_x_read = 0, spacing_y_read = 0, spacing_z_read = 0;
	double unsused0 = 0.0, unsused1 = 1.0;
	int red_subscript = INT_MIN;
	AnatomyMap empty_;
	// do not use GDCM's rescale for enhanced, except for US
	message_ =
		read_buffer(
			ok, data,
			image_overlays, -2, // unused
			empty_, -1, // unused in enhanced
			f,
			false,
			pixelformat, pi,
			&dimx_read, &dimy_read, &dimz_read,
			&origin_x_read,  &origin_y_read,  &origin_z_read,
			&spacing_x_read, &spacing_y_read, &spacing_z_read,
			dircos_read,
			&unsused0,
			&unsused1, // GDCM's rescale, unused
			clean_unused_bits,
			false, false, false,
			true, &red_subscript);
#if 0
	std::cout << "subscript = " << red_subscript << std::endl;
#endif
	if (*ok==false) return message_;
	if (
		rows_ok && cols_ok &&
		(dimx_read!=columns_||dimy_read!=rows_))
	{
		*ok=false; 
		for (unsigned int x=0; x < data.size(); x++)
		{
			if (data.at(x)) delete [] data[x];
		}
		data.clear();
		return QString(
			"dimx_read!=columns_||dimy_read!=rows_");
	}
	if (dimz_read!=data.size())
	{
		*ok=false;
		for (unsigned int x=0; x < data.size(); x++)
		{
			if (data.at(x)) delete [] data[x];
		}
		data.clear();
		return QString("dimz_read!=data.size()");
	}
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	//
	//
	//
	bool tmp17 = false;
	int dim6th = -1;
	int dim5th = -1;
	int dim4th = -1;
	int dim3rd = -1;
	int enh_id = -1;
	if (!idx_values.empty() && !values.empty())
	{
		enhanced_get_indices(
			sq,
			&dim6th, &dim5th, &dim4th, &dim3rd,
			&enh_id);
	}
	else if (idx_values.empty() && !values.empty())
	{
		// stack id/position number without dimension organisation?
		// try to re-build
		const bool is_legacy = (
			sop == QString("1.2.840.10008.5.1.4.1.1.2.2") ||
			sop == QString("1.2.840.10008.5.1.4.1.1.4.4") ||
			sop == QString("1.2.840.10008.5.1.4.1.1.128.1"))
			?
			true : false;
		if (!is_legacy)
		{
			// don't rebuld for Legacy IODs, many files are wrong,
			// try frames one by one
			bool idx_values_rebuild = false;
			bool tmp12 = false;
			DimIndexValues idx_values_tmp;
			for (unsigned int x = 0; x < values.size(); x++)
			{
				if (!(
					values.at(x).stack_id_ok &&
					values.at(x).in_stack_pos_num_ok))
				{
					tmp12 = true;
					break;
				}
			}
			if (!tmp12)
			{
				for (unsigned int x = 0; x < values.size(); x++)
				{
					DimIndexValue tmp13;
					tmp13.id = values.at(x).id;
					tmp13.idx.push_back(values.at(x).stack_id);
					tmp13.idx.push_back(values.at(x).in_stack_pos_num);
					idx_values_tmp.push_back(tmp13);
				}
				for (unsigned int x = 0; x < idx_values_tmp.size(); x++)
				{
					idx_values.push_back(idx_values_tmp[x]);
				}
				idx_values_tmp.clear();
				idx_values_rebuild = true;
#ifdef ENHANCED_PRINT_INFO
				if (!min_load)
					std::cout
						<< "stack id and position without dim. org."
						<< std::endl;
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
			<< " dim6th=" << dim6th
			<< " dim5th=" << dim5th
			<< " dim4th=" << dim4th
			<< " dim3rd=" << dim3rd
			<< std::endl;
	}
#endif
	message_ = read_enhanced_3d_6d(
		&tmp17, ivariants, sop, f,
		data, 
		image_overlays,
		rows_, columns_, pixelformat, pi,
		dim6th, dim5th, dim4th, dim3rd,
		idx_values, values,
		ok3d, max_3d_tex_size, gl,
		min_load,
		settings,
		dircos_read,
		red_subscript,
		spacing_x_read, spacing_y_read, spacing_z_read,
		origin_x_read, origin_y_read, origin_z_read,
		unsused0, unsused1,
		false,
		ds,
		pb,
		tolerance);
#ifdef ENHANCED_PRINT_INFO
	if (!min_load && !message_.isEmpty())
		std::cout << message_.toStdString() << std::endl;
#endif
	if (!tmp17 &&
		!(
			dim6th == -1 &&
			dim5th == -1 &&
			dim4th == -1 &&
			dim3rd == -1))
	{
#ifdef ENHANCED_PRINT_INFO
		if (!min_load)
			std::cout << "  Fallback" << std::endl;
#endif
		message_ = read_enhanced_3d_6d(
			&tmp17, ivariants, sop, f,
			data,
			image_overlays,
			rows_, columns_, pixelformat, pi,
			-1, -1, -1, -1,
			idx_values, values,
			ok3d, max_3d_tex_size, gl,
			min_load,
			settings,
			dircos_read,
			red_subscript,
			spacing_x_read, spacing_y_read, spacing_z_read,
			origin_x_read,  origin_y_read,  origin_z_read,
			unsused0, unsused1,
			false,
			ds,
			pb,
			tolerance);
	}
	*ok = tmp17;
	for (unsigned int x=0; x < data.size(); x++)
	{
		if (data.at(x))
		{
			delete [] data[x];
			data[x] = NULL;
		}
	}
	data.clear();
	return message_;
}

QString DicomUtils::read_ultrasound(
	bool * ok, ImageVariant * ivariant,
	const QStringList & images_ipp,
	const QWidget * settings, QProgressDialog * pb)
{
	//
	const bool overwrite_mdcm_spacing = true;
	//
	*ok = false;
	if (!ivariant) return QString("ivariant==NULL");
	if (!settings) return QString("settings==NULL");
	if (images_ipp.size() != 1)
		return QString("read_ultrasound reads 1 image");
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	const SettingsWidget * wsettings =
		static_cast<const SettingsWidget *>(settings);
	unsigned int dimx = 0, dimy = 0, dimz = 0;
	double origin_x  = 0.0, origin_y  = 0.0, origin_z  = 0.0;
	double spacing_x = 0.0, spacing_y = 0.0, spacing_z = 0.0;
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	std::vector<char*> data;
	itk::Matrix<itk::SpacePrecisionType,3,3> direction;
	mdcm::PixelFormat pixelformat;
	mdcm::PhotometricInterpretation pi;
	const mdcm::Tag tnumframes(0x0028,0x0008);
	const mdcm::Tag tPixelAspectRatio(0x0028,0x0034);
	const bool overlays_enabled = wsettings->get_overlays();
	const int overlays_idx = overlays_enabled ? 0 : -2;
	int number_of_frames = 0;
	//
	mdcm::Reader reader;
	reader.SetFileName(images_ipp.at(0).toLocal8Bit().constData());
	*ok = reader.Read();
	if (*ok==false)
	{
		return (QString(
			"can not read file ") + images_ipp.at(0));
	}
	const mdcm::File    & file = reader.GetFile();
	const mdcm::DataSet & ds   = file.GetDataSet();
	if (ds.FindDataElement(tnumframes))
	{
		const mdcm::DataElement & e =
			ds.GetDataElement(tnumframes);
		if (
			!e.IsEmpty() &&
			!e.IsUndefinedLength() &&
			e.GetByteValue())
		{
			QString numframes("");
			numframes =
				QString::fromLatin1(
					e.GetByteValue()->GetPointer(),
					e.GetByteValue()->GetLength());
			const QVariant v(
				numframes.trimmed().remove(QChar('\0')));
			bool c_ok = false;
			const int k = v.toInt(&c_ok);
			if (c_ok) number_of_frames = k;
		}
	}
	//
	read_ivariant_info_tags(ds, ivariant);
	//
	if (number_of_frames > 1)
		read_frame_times(ds, ivariant, number_of_frames);
	//
	read_us_regions(ds, ivariant);
	//
	double tmp_c = -999999.0, tmp_w = -999999.0;
	short tmp_lut_function = 0;
	read_window(ds, &tmp_c, &tmp_w, &tmp_lut_function);
	ivariant->di->default_us_window_center =
		ivariant->di->us_window_center = tmp_c;
	ivariant->di->default_us_window_width =
		ivariant->di->us_window_width  = tmp_w;
	ivariant->di->lut_function = tmp_lut_function;
	//
	double dircos_[] = {0.0,0.0,0.0,0.0,0.0,0.0};
	unsigned int dimx_, dimy_, dimz_;
	double origin_x_, origin_y_, origin_z_;
	double spacing_x_, spacing_y_, spacing_z_;
	double shift_tmp = 0.0, scale_tmp = 1.0;
	QString buff_error;
	buff_error = read_buffer(
		ok,
		data,
		ivariant->image_overlays, overlays_idx,
		ivariant->anatomy, 0, // FIXME
		images_ipp.at(0),
		wsettings->get_rescale(),
		pixelformat, pi,
		&dimx_, &dimy_, &dimz_,
		&origin_x_, &origin_y_, &origin_z_,
		&spacing_x_, &spacing_y_, &spacing_z_,
		dircos_,
		&shift_tmp, &scale_tmp,
		clean_unused_bits,
		false, false, false,
		false, NULL);
	if (*ok==false) return buff_error;
	//
	if (overwrite_mdcm_spacing)
	{
		if (ds.FindDataElement(tPixelAspectRatio))
		{
			std::vector<int> tmp0;
			const bool tmp0_ok =
				get_is_values(ds,tPixelAspectRatio,tmp0);
			if (tmp0_ok && tmp0.size()==2)
			{
				spacing_x = tmp0[1];
				spacing_y = tmp0[0];
			}
			else
			{
				spacing_x = 1.0;
				spacing_y = 1.0;
			}
		}
		else
		{
			spacing_x = 1.0;
			spacing_y = 1.0;
		}
		spacing_z = 1.0;
	}
	else
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
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	//
	QString error = CommonUtils::gen_itk_image(ok,
		data, true,
		pixelformat, pi,
		ivariant, 
		direction,
		dimx, dimy, dimz,
		origin_x, origin_y, origin_z,
		spacing_x, spacing_y, spacing_z,
		true, false,
		wsettings->get_resize(),
		wsettings->get_size_x(), wsettings->get_size_y(),
		wsettings->get_rescale(),
		0, NULL, pb,
		false);
	for (unsigned int x = 0; x < data.size(); x++)
	{
		if (data.at(x))
		{
			delete [] data[x];
			data[x] = NULL;
		}
	}
	data.clear();
	if (*ok==true) IconUtils::icon(ivariant);
	if (*ok==false) return error;
	if (!ivariant->frame_times.empty() &&
		((int)ivariant->frame_times.size()!=ivariant->di->idimz))
	{
		ivariant->frame_times.clear();
	}
	//
	CommonUtils::reset_bb(ivariant);
	ivariant->di->selected_z_slice = 0;
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
	int max_3d_tex_size, GLWidget * gl, bool ok3d,
	const QWidget * settings, QProgressDialog * pb,
	float tolerance,
	bool apply_rescale)
{
	*ok = false;
	if (!ivariant) return QString("ivariant==NULL");
	if (!settings) return QString("settings==NULL");
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	const SettingsWidget * wsettings =
		static_cast<const SettingsWidget *>(settings);
	unsigned int dimx = 0, dimy = 0, dimz = 0;
	double origin_x  = 0.0, origin_y  = 0.0, origin_z  = 0.0;
	double spacing_x = 0.0, spacing_y = 0.0, spacing_z = 0.0;
	const bool clean_unused_bits = wsettings->get_clean_unused_bits();
	std::vector<char*> data;
	itk::Matrix<itk::SpacePrecisionType,3,3> direction;
	mdcm::PixelFormat pixelformat;
	mdcm::PixelFormat previous_pixelformat;
	mdcm::PhotometricInterpretation pi;
	bool geometry_from_image = min_load ? true : false;
	bool slices_ok = false;
	const mdcm::Tag tnumframes(0x0028,0x0008);
	const bool overlays_enabled = wsettings->get_overlays();
	//
	std::vector<double> levels_;
	std::vector<double> windows_;
	std::vector<short>  luts_;
	//
	for (int j = 0; j < images_ipp.size(); j++)
	{
		int number_of_frames = 0;
		mdcm::Reader reader;
		reader.SetFileName(
			images_ipp.at(j).toLocal8Bit().constData());
		*ok = reader.Read();
		if (*ok==false)
			return (QString("can not read file ")+images_ipp.at(j));
		const mdcm::File & file = reader.GetFile();
		const mdcm::DataSet & ds = file.GetDataSet();
		if (j==0)
		{
			if (ds.FindDataElement(tnumframes))
			{
				const mdcm::DataElement & e =
					ds.GetDataElement(tnumframes);
				if (
					!e.IsEmpty() &&
					!e.IsUndefinedLength() &&
					e.GetByteValue())
				{
					QString numframes("");
					numframes =
						QString::fromLatin1(
							e.GetByteValue()->GetPointer(),
							e.GetByteValue()->GetLength());
					const QVariant v(
						numframes.trimmed().remove(QChar('\0')));
					bool c_ok = false; const int k = v.toInt(&c_ok);
					if (c_ok) number_of_frames = k;
				}
			}
			if (!min_load)
			{
				read_ivariant_info_tags(ds, ivariant);
				if (ivariant->sop ==
					QString("1.2.840.10008.5.1.4.1.1.4"))
					ivariant->iinfo = read_MRImageModule(ds);
				else if (ivariant->sop ==
					QString("1.2.840.10008.5.1.4.1.1.2"))
					ivariant->iinfo = read_CTImageModule(ds);
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
						ok3d,
						ivariant->di->skip_texture,
						gl,
						pb,
						tolerance);
					if (slices_ok) ivariant->iod_supported = true;
					else geometry_from_image = true;
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
							ok3d,
							ivariant->di->skip_texture,
							gl,
							pb,
							tolerance);
						if (slices_ok) ivariant->iod_supported = true;
						else geometry_from_image = true;
						ivariant->unit_str = QString(" mm");
					}
				}
				else if (ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.481.2")) // RTDOSE
				{
					if (!min_load)
					{
						slices_ok =	read_slices_rtdose(
							images_ipp.at(j),
							ivariant,
							ok3d,
							ivariant->di->skip_texture,
							gl,
							pb,
							0.01f);
						if (slices_ok) ivariant->iod_supported = true;
						else geometry_from_image = true;
						if (slices_ok) load_contour(ds,ivariant);
					}
				}
				else if (
						ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.7")       || // SC
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.77.1.5.1")|| // Ophthalmic Photography  8 Bit
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.77.1.5.2")|| // Ophthalmic Photography 16 Bit
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.1")       || // Computed Radiography 
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.1.1")     || // Digital X-Ray - For Presentation
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.1.1.1")   || // Digital X-Ray - For Processing
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.1.2")     || // Digital Mammography X-Ray - For Presentation
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.1.2.1")   || // Digital Mammography X-Ray - For Processing
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.1.3")     || // Digital Intra-Oral X-Ray - For Presentation
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.1.3.1")   || // Digital Intra-Oral X-Ray - For Processing
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.20")      || // Nuclear Medicine
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.12.1")    || // X-Ray Angiographic
				        ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.12.2")       // X-Ray RF
						)
				{
					if (!min_load) ivariant->di->skip_texture = true;
				}
				else
				{
					if (!min_load)
					{
						slices_ok =	read_slices(
							images_ipp,
							ivariant,
							ok3d,
							ivariant->di->skip_texture,
							gl,
							pb,
							tolerance);
						if (!slices_ok) ivariant->di->skip_texture = true;
					}
				}
				if (!min_load && ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.128")) // PET
				{
					;;
				}
			}
			//
			if (!min_load) read_frame_times(ds, ivariant, number_of_frames); //////////
			//
			if (!min_load && images_ipp.size()==1)
			{
				const int inst_num = read_instance_number(ds);
				if (inst_num>=0) ivariant->instance_number = inst_num;
			}
			//
		}
		//
		if (!min_load)
		{
			double tmp_c = -999999.0, tmp_w = -999999.0;
			short lut_function = 0;
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
		double dircos_[] = {0.0,0.0,0.0,0.0,0.0,0.0};
		unsigned int dimx_ = 0, dimy_ = 0, dimz_ = 0;
		double origin_x_ = 0.0, origin_y_ = 0.0, origin_z_ = 0.0;
		double spacing_x_ = 0.0, spacing_y_ = 0.0, spacing_z_ = 0.0;
		double shift_tmp = 0.0, scale_tmp = 1.0;
		QString buff_error;
		const int overlays_idx = overlays_enabled ? j : -2;
		const bool rescale = (!apply_rescale) ? false : wsettings->get_rescale();
		if (images_ipp.size()>1)
		{
			std::vector<char*> data_;
			buff_error = read_buffer(
				ok,
				data_,
				ivariant->image_overlays,
				overlays_idx,
				ivariant->anatomy,
				j,
				images_ipp.at(j),
				rescale,
				pixelformat, pi,
				&dimx_, &dimy_, &dimz_,
				&origin_x_, &origin_y_, &origin_z_,
				&spacing_x_, &spacing_y_, &spacing_z_,
				dircos_,
				&shift_tmp, &scale_tmp,
				clean_unused_bits,
				false, false, elscint,
				false, NULL);
			if (dimz_>1)
			{
				*ok = false;
				ivariant->anatomy.clear();
				ivariant->image_overlays.all_overlays.clear();
				return QString("Can not read particular series (1)");
			}
			if (!data_.empty() && data_.at(0))
			{
				data.push_back(&data_[0][0]);
			}
			else
			{
				*ok = false;
				ivariant->anatomy.clear();
				ivariant->image_overlays.all_overlays.clear();
				return QString(
					"Can not read particular series (2)");
			}
		}
		else if (images_ipp.size()==1)
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
				pixelformat, pi,
				&dimx_, &dimy_, &dimz_,
				&origin_x_, &origin_y_, &origin_z_,
				&spacing_x_, &spacing_y_, &spacing_z_,
				dircos_,
				&shift_tmp, &scale_tmp,
				clean_unused_bits,
				mosaic, uihgrid, elscint,
				false, NULL);
		}
		if (*ok == false)
		{
			ivariant->anatomy.clear();
			ivariant->image_overlays.all_overlays.clear();
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
			if (images_ipp.size()==1) dimz = dimz_;
			else dimz = images_ipp.size();
			if (slices_ok)
			{
				bool invalidate = false;
				if (ivariant->equi)
				{
					const float tmp0_spacing_x = (float)
						spacing_x_;
					const float tmp0_spacing_y = (float)
						spacing_y_;
					const float tmp1_spacing_x = (float)
						ivariant->di->ix_spacing;
					const float tmp1_spacing_y = (float)
						ivariant->di->iy_spacing;
					const float tmp1_spacing_z = (float)
						ivariant->di->iz_spacing;
					if (tmp1_spacing_x <= 0)
					{
						std::cout <<
							"ivariant->di->ix_spacing <= 0 "
							<< std::endl;
						invalidate=true;
						ivariant->di->ix_spacing = 1.0;
					}
					if (tmp1_spacing_y <= 0)
					{
						std::cout <<
							"ivariant->di->iy_spacing <= 0 "
							<< std::endl;
						invalidate=true;
						ivariant->di->iy_spacing = 1.0;
					}
					if (tmp1_spacing_z <= 0)
					{
						std::cout <<
							"ivariant->di->iz_spacing <= 0 "
							<< std::endl;
						invalidate=true;
						ivariant->di->iz_spacing = 0.00001;
					}
					if (tmp0_spacing_x+0.001f<tmp1_spacing_x ||
						tmp0_spacing_x-0.001f>tmp1_spacing_x)
					{
						std::cout
							<< "tmp0_spacing_x!=tmp1_spacing_x "
							<< tmp0_spacing_x << " "
							<< tmp1_spacing_x << std::endl;
						invalidate=true;
					}
					if (tmp0_spacing_y+0.001f<tmp1_spacing_y ||
						tmp0_spacing_y-0.001f>tmp1_spacing_y)
					{
						std::cout
							<< "tmp0_spacing_y!=tmp1_spacing_y "
							<< tmp0_spacing_y << " "
							<< tmp1_spacing_y << std::endl;
						invalidate=true;
					}
					const float tmp0_origin_x = (float)
						origin_x_;
					const float tmp0_origin_y = (float)
						origin_y_;
					const float tmp0_origin_z = (float)
						origin_z_;
					const float tmp1_origin_x = (float)
						ivariant->di->ix_origin;
					const float tmp1_origin_y = (float)
						ivariant->di->iy_origin;
					const float tmp1_origin_z = (float)
						ivariant->di->iz_origin;
					if (tmp0_origin_x+0.001f < tmp1_origin_x ||
						tmp0_origin_x-0.001f > tmp1_origin_x)
					{
						std::cout
							<< "tmp0_origin_x!=tmp1_origin_x "
							<< tmp0_origin_x << " "
							<< tmp1_origin_x << std::endl;
						invalidate=true;
					}
					if (tmp0_origin_y+0.001f<tmp1_origin_y ||
						tmp0_origin_y-0.001f>tmp1_origin_y)
					{
						std::cout
							<< "tmp0_origin_y!=tmp1_origin_y "
							<< tmp0_origin_y << " "
							<< tmp1_origin_y << std::endl;
						invalidate=true;
					}
					if (tmp0_origin_z+0.001f<tmp1_origin_z ||
						tmp0_origin_z-0.001f>tmp1_origin_z)
					{
						std::cout
							<< "tmp0_origin_z!=tmp1_origin_z "
							<< tmp0_origin_z << " "
							<< tmp1_origin_z << std::endl;
						invalidate=true;
					}
					const float tmp0_dircos_0 = (float)
						dircos_[0];
					const float tmp0_dircos_1 = (float)
						dircos_[1];
					const float tmp0_dircos_2 = (float)
						dircos_[2];
					const float tmp0_dircos_3 = (float)
						dircos_[3];
					const float tmp0_dircos_4 = (float)
						dircos_[4];
					const float tmp0_dircos_5 = (float)
						dircos_[5];
					const float tmp1_dircos_0 = (float)
						ivariant->di->dircos[0];
					const float tmp1_dircos_1 = (float)
						ivariant->di->dircos[1];
					const float tmp1_dircos_2 = (float)
						ivariant->di->dircos[2];
					const float tmp1_dircos_3 = (float)
						ivariant->di->dircos[3];
					const float tmp1_dircos_4 = (float)
						ivariant->di->dircos[4];
					const float tmp1_dircos_5 = (float)
						ivariant->di->dircos[5];
					if (tmp0_dircos_0+0.001f<tmp1_dircos_0 ||
						tmp0_dircos_0-0.01f>tmp1_dircos_0)
					{
						std::cout
							<< "tmp0_dircos_0!=tmp1_dircos_0 "
							<< tmp0_dircos_0 << " "
							<< tmp1_dircos_0 << std::endl;
						invalidate=true;
					}
					if (tmp0_dircos_1+0.001f<tmp1_dircos_1 ||
						tmp0_dircos_1-0.001f>tmp1_dircos_1)
					{
						std::cout
							<< "tmp0_dircos_1!=tmp1_dircos_1 "
							<< tmp0_dircos_1 << " "
							<< tmp1_dircos_1 << std::endl;
						invalidate=true;
					}
					if (tmp0_dircos_2+0.001f<tmp1_dircos_2 ||
						tmp0_dircos_2-0.001f>tmp1_dircos_2)
					{
						std::cout
							<< "tmp0_dircos_2!=tmp1_dircos_2 "
							<< tmp0_dircos_2 << " "
							<< tmp1_dircos_2 << std::endl;
						invalidate=true;
					}
					if (tmp0_dircos_3+0.001f<tmp1_dircos_3 ||
						tmp0_dircos_3-0.001f>tmp1_dircos_3)
					{
						std::cout
							<< "tmp0_dircos_3!=tmp1_dircos_3 "
							<< tmp0_dircos_3 << " "
							<< tmp1_dircos_3 << std::endl;
						invalidate=true;
					}
					if (tmp0_dircos_4+0.001f<tmp1_dircos_4 ||
						tmp0_dircos_4-0.001f>tmp1_dircos_4)
					{
						std::cout
							<< "tmp0_dircos_4!=tmp1_dircos_4 "
							<< tmp0_dircos_4 << " "
							<< tmp1_dircos_4 << std::endl;
						invalidate=true;
					}
					if (tmp0_dircos_5+0.001f<tmp1_dircos_5 ||
						tmp0_dircos_5-0.001f>tmp1_dircos_5)
					{
						std::cout
							<< "tmp0_dircos_5!=tmp1_dircos_5 "
							<< tmp0_dircos_5 << " "
							<< tmp1_dircos_5 << std::endl;
						invalidate=true;
					}
				}
				if (invalidate)
				{
					ivariant->equi = false;
					ivariant->orientation = 0;
					ivariant->orientation_string = QString("");
					std::cout <<
							"warning: could not validate "
							"image, using as non-uniform"
						<< std::endl;
				}
				spacing_x = ivariant->di->ix_spacing > 0 ?
					ivariant->di->ix_spacing : 1;
				spacing_y = ivariant->di->iy_spacing > 0 ?
					ivariant->di->iy_spacing : 1;
				spacing_z = ivariant->di->iz_spacing > 0.00001 ?
					ivariant->di->iz_spacing : 0.00001;
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
					row_dircos_y * col_dircos_z -
						row_dircos_z * col_dircos_y; 
				const float nrm_dircos_y =
					row_dircos_z * col_dircos_x -
						row_dircos_x * col_dircos_z;
				const float nrm_dircos_z =
					row_dircos_x * col_dircos_y -
						row_dircos_y * col_dircos_x;
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
				spacing_z =
					spacing_z_ > 0.00001 ? spacing_z_ : 0.00001;
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
					row_dircos_y * col_dircos_z -
						row_dircos_z * col_dircos_y; 
				const float nrm_dircos_y =
					row_dircos_z * col_dircos_x -
						row_dircos_x * col_dircos_z;
				const float nrm_dircos_z =
					row_dircos_x * col_dircos_y -
						row_dircos_y * col_dircos_x;
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
			if (ivariant->di->iz_spacing <= 0.00001 &&
				ivariant->one_direction)
			{
				ivariant->di->iz_spacing = 0.00001;
				ivariant->di->skip_texture = true;
				ivariant->equi = false;
			}
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
		if (j > 0)
		{
			if (
				(previous_pixelformat.GetBitsAllocated()
					!= pixelformat.GetBitsAllocated()) ||
				(previous_pixelformat.GetScalarType()
					!= pixelformat.GetScalarType()) ||
				(previous_pixelformat.GetSamplesPerPixel()
					!= pixelformat.GetSamplesPerPixel()))
			{
				*ok = false;
				for (unsigned int x = 0; x < data.size(); x++)
				{
					if (data.at(x)) delete [] data[x];
				}
				data.clear();
				ivariant->anatomy.clear();
				ivariant->image_overlays.all_overlays.clear();
				return QString(
					"previous_pixelformat!=pixelformat");
			}
			if (!min_load && !mosaic && !uihgrid)
			{
				PRDisplayShutter a;
				if (read_shutter(ds, a))
				{
					ivariant->pr_display_shutters.insert(j, a);
				}
			}
		}
		previous_pixelformat = pixelformat;
	}
	//
	if ((images_ipp.size()>1) && data.size()!=dimz)
	{
		*ok = false;
		for (unsigned int x = 0; x < data.size(); x++)
		{
			if (data.at(x)) delete [] data[x];
		}
		data.clear();
		ivariant->anatomy.clear();
		ivariant->image_overlays.all_overlays.clear();
		return QString("data.size()!=dimz");
	}
	if (pb) pb->setValue(-1);
	QApplication::processEvents();
	//
	{
		bool one_level = true;
		bool one_lut   = true;
		const size_t levels_size = levels_.size();
		if (levels_size == 1)
		{
			ivariant->di->default_us_window_center =
				ivariant->di->us_window_center = levels_.at(0);
			ivariant->di->default_us_window_width  =
				ivariant->di->us_window_width = windows_.at(0);
			ivariant->di->lut_function = luts_.at(0);
		}
		else if (levels_size > 1)
		{
			for (size_t x = 1; x < levels_size; x++)
			{
				const bool b3 = luts_.at(x) == luts_.at(x-1);
				if (!b3) one_lut = false;
					const bool b1 =
				itk::Math::FloatAlmostEqual(levels_.at(x), levels_.at(x-1));
				const bool b2 =
					itk::Math::FloatAlmostEqual(windows_.at(x), windows_.at(x-1));
				if (!b1||!b2)
				{
					one_level = false;
					break;
				}
			}
			if (one_level)
			{
				ivariant->di->default_us_window_center =
					ivariant->di->us_window_center = levels_.at(0);
				ivariant->di->default_us_window_width  =
					ivariant->di->us_window_width = windows_.at(0);
			}
			if (one_lut) ivariant->di->lut_function = luts_.at(0);
		}
	}
	//
	const bool allow_geometry_from_image =
		(mosaic||uihgrid) ? true : false;
	const bool no_warn_rescale =
		(apply_rescale)
		? wsettings->get_rescale()
		: true;
	QString error = CommonUtils::gen_itk_image(ok,
		data, true,
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
		max_3d_tex_size, gl, pb,
		false);
	for (unsigned int x = 0; x < data.size(); x++)
	{
		if (data.at(x))
		{
			delete [] data[x];
			data[x] = NULL;
		}
	}
	data.clear();
	if (*ok==true) IconUtils::icon(ivariant);
	if (*ok==false)
	{
		ivariant->anatomy.clear();
		ivariant->image_overlays.all_overlays.clear();
		return error;
	}
	if (!ivariant->frame_times.empty() &&
		((int)ivariant->frame_times.size() !=
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
		ivariant->image_instance_uids.size()!=ivariant->di->idimz)
	{
		ivariant->image_instance_uids.clear();
	}
	//
	CommonUtils::reset_bb(ivariant);
	if (ivariant->modality==QString("US") ||
			(ivariant->modality==QString("XA") &&
			ivariant->frame_times.size()>1))
		ivariant->di->selected_z_slice = 0;
	return QString("");
}

#if 1 // no example file
static void delta_decode_rgb(
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
	if(data_size == outputlen) return;
	unsigned char * src  = (unsigned char*)data_in;
	unsigned char * dest = (unsigned char*)&new_stream[0];
	union
	{
		unsigned char gray;
	unsigned char rgb[3];
	} pixel;
	pixel.rgb[0] = pixel.rgb[1] = pixel.rgb[2] = 0;
	// Start in grayscale mode
	bool graymode = true;
	size_t dx = 1;
	size_t dy = 3;
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
	// The following is highly unoptimized as we have nested
	// if statement in a while loop we need to switch from one
	// algorithm to ther other (RGB <-> GRAY).
	while (ps)
	{
		// Next byte
		unsigned char b = *src++;
		assert( src < data_in + data_size );
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
			ps--;
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
			ps--;
			break;
		}
	}
}
#endif

static void delta_decode(
	const char * inbuffer,
	size_t length,
	std::vector<unsigned short> & output)
{
	// RLE pass
	std::vector<char> temp;
	for(size_t i = 0; i < length; i++)
	{
		if(inbuffer[i] == (char)0xa5)
		{
			int repeat = (int)((unsigned char)inbuffer[i+1] + 1);
			const char value = inbuffer[i+2];
			while(repeat > 0)
			{
				temp.push_back(value);
				--repeat;
			}
			i += 2;
		}
		else
		{
			temp.push_back(inbuffer[i]);
		}
	}
	// Delta encoding pass
	unsigned short delta = 0;
	for(size_t i = 0; i < temp.size(); ++i)
	{
		if(temp[i] == (char)0x5a)
		{
			const unsigned char v1 = (unsigned char)temp[i+1];
			const unsigned char v2 = (unsigned char)temp[i+2];
			const unsigned short value = (unsigned short)(v2 * 256 + v1);
			output.push_back(value);
			delta = value;
			i += 2;
		}
		else
		{
			const unsigned short value = (unsigned short)(temp[i] + delta);
			output.push_back(value);
			delta = value;
		}
	}
	if (output.size() % 2)
	{
		output.resize(output.size() - 1);
	}
}

bool DicomUtils::convert_elscint(const QString f, const QString outf)
{
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
	if(!reader.Read())
	{
		return false;
	}
	const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
	const mdcm::PrivateTag tcompressiontype(0x07a1,0x11,"ELSCINT1");
	if(!ds.FindDataElement(tcompressiontype))
	{
		return false;
	}
	const mdcm::DataElement& compressiontype =
		ds.GetDataElement(tcompressiontype);
	if (compressiontype.IsEmpty())
	{
		return false;
	}
	const mdcm::ByteValue * bv = compressiontype.GetByteValue();
	std::string comprle = "PMSCT_RLE1";
	std::string comprgb = "PMSCT_RGB1";
	bool isrle = false;
	bool isrgb = false;
	if(strncmp(
		bv->GetPointer(),
		comprle.c_str(),
		comprle.size()) == 0)
	{
		isrle = true;
	}
	else if(strncmp(
		bv->GetPointer(),
		comprgb.c_str(),
		comprgb.size()) == 0)
	{
		isrgb = true;
	}
	if(!isrle && !isrgb)
	{
		return false;
	}
	const mdcm::PrivateTag tprivatepixeldata(0x07a1,0x0a,"ELSCINT1");
	if(ds.FindDataElement(tprivatepixeldata))
	{
		const mdcm::DataElement & compressionpixeldata =
			ds.GetDataElement(tprivatepixeldata);
		if (compressionpixeldata.IsEmpty()) return false;
		const mdcm::ByteValue * bv2 = compressionpixeldata.GetByteValue();
		mdcm::Tag tpixeldata(0x7fe0, 0x0010);
		mdcm::DataElement pixeldata;
		if (isrle)
		{
			// Create or replace pixel data element 0x7fe0, 0x0010
			if(!reader.GetFile().GetDataSet().FindDataElement(tpixeldata))
			{
				pixeldata = mdcm::DataElement(tpixeldata, 0, mdcm::VR::OW);
			}
			else
			{
				pixeldata = reader.GetFile().GetDataSet().GetDataElement(tpixeldata);
			}
			pixeldata.SetVR(mdcm::VR::OW);
			const size_t bv2l = bv2->GetLength();
			mdcm::Attribute<0x0028,0x0010> at1;
			at1.SetFromDataSet(ds);
			mdcm::Attribute<0x0028,0x0011> at2;
			at2.SetFromDataSet(ds);
			const size_t w = at1.GetValue();
			const size_t h = at2.GetValue();
			const size_t at1l = w*h*sizeof(unsigned short);
			if(bv2l == at1l)
			{
				std::cout << "Warning: Elscint data seems to be not compressed"
					<< std::endl;
				pixeldata.SetByteValue(bv2->GetPointer(), bv2->GetLength());
			}
			else
			{
				std::vector<unsigned short> buffer;
				delta_decode(bv2->GetPointer(), bv2->GetLength(), buffer);
				// TODO check that decompress byte buffer match the expected size
				pixeldata.SetByteValue(
					(char*)&buffer[0],
					(uint32_t)(buffer.size() * sizeof(unsigned short)));
			}
		}
		else if (isrgb)
		{
			// Create or replace pixel data element 0x7fe0, 0x0010
			if(!reader.GetFile().GetDataSet().FindDataElement(tpixeldata))
			{
				pixeldata = mdcm::DataElement(tpixeldata, 0, mdcm::VR::OW);
			}
			else
			{
				pixeldata = reader.GetFile().GetDataSet().GetDataElement(tpixeldata);
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
			const size_t outputlen = 3*h*w;
			if(bv2l == outputlen)
			{
				std::cout
					<< "Warning: Elscint data seems to be not compressed"
					<< std::endl;
				pixeldata.SetByteValue(bv2->GetPointer(), bv2->GetLength());

			}
			else
			{
				std::vector<unsigned char> buffer;
				delta_decode_rgb(
					(const unsigned char*)bv2->GetPointer(), bv2l, buffer,
					at0.GetValue(), w, h);
				pixeldata.SetByteValue((char*)&buffer[0], (uint32_t)buffer.size());
			}
		}
		// Add the pixel data element
		if(reader.GetFile().GetDataSet().FindDataElement(tpixeldata))
		{
			reader.GetFile().GetDataSet().Replace(pixeldata);
		}
		else
		{
			reader.GetFile().GetDataSet().ReplaceEmpty(pixeldata);
		}
		//
		//
		//
		if (ds.FindDataElement(mdcm::Tag(0x07a1,0x0010)))
		{
			reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x07a1,0x0010));
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x100a)))
			{
				reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x07a1,0x100a));
			}
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1010)))
			{
				reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x07a1,0x1010));
			}
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1011)))
			{
				reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x07a1,0x1011));
			}
		}
	}
	else
	{
		const mdcm::Tag tpixeldata(0x7fe0, 0x0010);
		if (!ds.FindDataElement(tpixeldata)) return false;
		const mdcm::DataElement & epixeldata =
			ds.GetDataElement(tpixeldata);
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
			const size_t at1l =	w*h*sizeof(unsigned short);
			if(bv2l == at1l)
			{
				std::cout
					<< "Warning: Elscint data seems to be not compressed"
					<< std::endl;
			}
			else
			{
				pixeldata = reader.GetFile().GetDataSet().GetDataElement(tpixeldata);
				pixeldata.SetVR(mdcm::VR::OW);
				std::vector<unsigned short> buffer;
				delta_decode(bv2->GetPointer(), bv2->GetLength(), buffer);
				// TODO check that decompress byte buffer match the expected size
				pixeldata.SetByteValue(
					(char*)&buffer[0],
					(uint32_t)(buffer.size() * sizeof(unsigned short)));
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
			const size_t outputlen = 3*h*w;
			if(bv2l == outputlen)
			{
				std::cout
					<< "Warning: Elscint data seems to be not compressed"
					<< std::endl;
			}
			else
			{
				pixeldata = reader.GetFile().GetDataSet().GetDataElement(tpixeldata);
				pixeldata.SetVR(mdcm::VR::OW);
				std::vector<unsigned char> buffer;
				delta_decode_rgb(
					(const unsigned char*)bv2->GetPointer(), bv2->GetLength(), buffer,
					at0.GetValue(), w, h);
				pixeldata.SetByteValue((char*)&buffer[0], (uint32_t)buffer.size());
			}
		}
		// Add the pixel data element
		reader.GetFile().GetDataSet().Replace(pixeldata);
		//
		//
		//
		if (ds.FindDataElement(mdcm::Tag(0x07a1,0x0010)))
		{
			reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x07a1,0x0010));
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1010)))
			{
				reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x07a1,0x1010));
			}
			if (ds.FindDataElement(mdcm::Tag(0x07a1,0x1011)))
			{
				reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x07a1,0x1011));
			}
		}
	}
	if (ds.FindDataElement(mdcm::Tag(0x7fe0,0x0)))
	{
		reader.GetFile().GetDataSet().Remove(mdcm::Tag(0x7fe0,0x0));
	}
	if (ds.FindDataElement(mdcm::Tag(0xfffc,0xfffc)))
	{
		reader.GetFile().GetDataSet().Remove(mdcm::Tag(0xfffc,0xfffc));
	}
	//
	//
	//
	reader.GetFile().GetHeader().SetDataSetTransferSyntax(
		mdcm::TransferSyntax::ExplicitVRLittleEndian);
	mdcm::Writer writer;
	writer.SetFile(reader.GetFile());
	writer.SetFileName(outf.toLocal8Bit().constData());
	if(!writer.Write())
	{
		std::cout << "Error: can not write tmp Elscint file "
		<< outf.toStdString()
		<< std::endl;
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
	mdcm::PixelFormat & pixelformat, mdcm::PhotometricInterpretation & pi,
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
	int * red_subscript)
{
	*ok = false;
	if (rescale)
	{
		mdcm::ImageHelper::SetForceRescaleInterceptSlope(true);
	}
	else
	{
		mdcm::ImageHelper::SetForceRescaleInterceptSlope(false);
	}
	mdcm::ImageHelper::SetCleanUnusedBits(clean_unused_bits);
	mdcm::ImageReader image_reader;
	QString elscf("");
	if (elscint)
	{
		QFileInfo fi(f);
		elscf = QDir::toNativeSeparators(
			QDir::tempPath() +
			QDir::separator() +
			fi.fileName() + QString("ELSCINT.dcm"));
		const bool elsc_ok = convert_elscint(f, elscf);
		if (elsc_ok)
		{
			image_reader.SetFileName(elscf.toLocal8Bit().constData());
		}
		else
		{
			QFile::remove(elscf);
			return QString("Can not convert ELSCINT file");
		}
	}
	else
	{
		image_reader.SetFileName(f.toLocal8Bit().constData());
		image_reader.SetApplySupplementalLUT(supp_palette_color);
	}
	if (overlay_idx == -2) image_reader.SetProcessOverlays(false);
	const bool i_ok = image_reader.Read();
	if (!i_ok)
	{
		if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
		return QString("!image_reader.Read()");
	}
	mdcm::Image & image = image_reader.GetImage();
#if 0
	const mdcm::TransferSyntax & ts =
		image_reader.GetFile().GetHeader().GetDataSetTransferSyntax();
#endif
	//
	if (mosaic)
	{
		mdcm::SplitMosaicFilter mfilter;
		mfilter.SetImage(image);
		mfilter.SetFile(image_reader.GetFile());
		if (mfilter.Split())
			image = mfilter.GetImage();
	}
	else if (uihgrid)
	{
		mdcm::SplitUihGridFilter mfilter;
		mfilter.SetImage(image);
		mfilter.SetFile(image_reader.GetFile());
		if (mfilter.Split())
			image = mfilter.GetImage();
	}
	*dimx_     = image.GetDimension(0);
	*dimy_     = image.GetDimension(1);
	*dimz_     = image.GetDimension(2);
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
	const unsigned int dimx = *dimx_;
	const unsigned int dimy = *dimy_;
	const unsigned int dimz = *dimz_;
	pi = image.GetPhotometricInterpretation();
	if (overlay_idx!=-2)
	{
		const size_t ov_count = image.GetNumberOfOverlays();
		if (ov_count>0)
		{
			QMultiMap<int, SliceOverlay> slice_overlays;
			for (size_t ov = 0; ov < ov_count; ov++)
			{
				mdcm::Overlay & o = image.GetOverlay(ov);
				const unsigned int NumberOfFrames =
					(unsigned int)o.GetNumberOfFrames();
				const unsigned int FrameOrigin =
					(unsigned int)o.GetFrameOrigin();
				const int o_dimx = (int)o.GetColumns();
				const int o_dimy = (int)o.GetRows();
				const int o_x    = (int)o.GetOrigin()[0];
				const int o_y    = (int)o.GetOrigin()[1];
				if ((NumberOfFrames > 1) && (FrameOrigin > 0) &&
						(overlay_idx < 0))
				{
					const size_t obuffer_size = o.GetUnpackBufferLength();
					if (obuffer_size%NumberOfFrames != 0) continue;
					const size_t fbuffer_size =
						obuffer_size/NumberOfFrames;
					char * tmp0;
					try { tmp0 = new char[obuffer_size]; }
					catch (std::bad_alloc&) { continue; }
					if (!tmp0) continue;
					const bool obuffer_ok = o.GetUnpackBuffer(
						tmp0, obuffer_size);
					if (!obuffer_ok)
					{
						delete [] tmp0;
						continue;
					}
					int idx = FrameOrigin - 1;
					for (unsigned int y = 0; y < NumberOfFrames; y++)
					{
						SliceOverlay overlay;
						overlay.dimx = o_dimx;
						overlay.dimy = o_dimy;
						overlay.x    = o_x;
						overlay.y    = o_y;
						const size_t p = idx*o_dimx*o_dimy;
						idx++;
						for (size_t j = 0; j < fbuffer_size; j++)
						{
							const size_t jj = p + j;
							if (jj < obuffer_size)
							{
								overlay.data.push_back(tmp0[jj]);
							}
							else
							{
								std::cout
									<< "warning: read_buffer() jj="
									<< jj << " obuffer_size"
									<< obuffer_size << std::endl;
							}
						}
						slice_overlays.insert(idx-1, overlay);
#if 0
						std::cout
							<< "obuffer_size=" << obuffer_size
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
					const size_t obuffer_size =
						o.GetUnpackBufferLength();
					char * tmp0;
					try { tmp0 = new char[obuffer_size]; }
					catch (std::bad_alloc&) { continue; }
					if (!tmp0) continue;
					const bool obuffer_ok = o.GetUnpackBuffer(
						tmp0, obuffer_size);
					if (!obuffer_ok)
					{
						delete [] tmp0;
						continue;
					}
					for (size_t j = 0; j < obuffer_size; j++)
					{
						overlay.data.push_back(tmp0[j]);
					}
					delete [] tmp0;
					slice_overlays.insert(
						overlay_idx,
						overlay);
#if 0
						std::cout
							<< "obuffer_size=" << obuffer_size
							<< " idx=" << overlay_idx << std::endl;
#endif
				}
			}
			const QList<int> keys = slice_overlays.keys();
			for (int x = 0; x < keys.size(); x++)
			{
				const int idx = keys.at(x);
				SliceOverlays l2 = slice_overlays.values(idx);
				if (!image_overlays.all_overlays.contains(idx))
				{
					image_overlays.all_overlays[idx] = l2;
#if 0
						std::cout
							<< "image_overlays.all_overlays["
							<< idx << "] = l2"
							<< std::endl;
#endif
				}
				else
				{
					for (int j = 0; j < l2.size(); j++)
					{
						image_overlays.all_overlays[idx]
							.push_back(l2[j]);
#if 0
						std::cout
							<< "image_overlays.all_overlays["
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
	if (anatomy_idx > -1)
	{
		const mdcm::DataSet & ds = image_reader.GetFile().GetDataSet();
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
			for (int x = 1; x < (int)dimz; x++) anatomy[x] = a;
		}
	}
	//
	bool rescale_ = false;
	size_t rescaled_buffer_size = 0;
	size_t supp_rescaled_buffer_size = 0;
	size_t not_rescaled_buffer_size = 0;
	size_t buffer_size = 0;
	char * rescaled_buffer = NULL;
	char * supp_rescaled_buffer = NULL;
	char * not_rescaled_buffer = NULL;
	char * buffer = NULL;
	bool singlebit = false;
	unsigned int type_size = 0;
	unsigned int samples_per_pix = 0;
	//
	if (image.GetPlanarConfiguration()==1)
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
			std::cout << "Error: failed to change Planar Configuration 1 to 0"
				<< std::endl;
		}
	}
	//
	if (pi == mdcm::PhotometricInterpretation::PALETTE_COLOR)
	{
		mdcm::ImageApplyLookupTable ialut;
		ialut.SetInput(image);
		ialut.Apply();
		image = ialut.GetOutput();
		pixelformat = image.GetPixelFormat();
	}
	else if (pi==mdcm::PhotometricInterpretation::MONOCHROME1 ||
			 pi==mdcm::PhotometricInterpretation::MONOCHROME2)
	{
		const double rescale_intercept = image.GetIntercept();
		const double rescale_slope     = image.GetSlope();
		*shift_tmp = rescale_intercept;
		*scale_tmp = rescale_slope;
		if (rescale)
		{
			if (!(
				rescale_intercept >= 0        &&
				rescale_intercept <  0.000001 &&
				rescale_slope     >  0.999999 &&
				rescale_slope     <  1.000001))
			{
				if (pixelformat.GetBitsAllocated() < 8)
				{
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return QString(
						"Bits allocated < 8 and rescale,\n"
						"not supported.");
				}
				if (supp_palette_color)
				{
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return QString(
						"Re-scale and Suppl. LUT?");
				}
				mdcm::Rescaler r;
				r.SetIntercept(rescale_intercept);
				r.SetSlope(rescale_slope);
				r.SetPixelFormat(image.GetPixelFormat());
				pixelformat = mdcm::PixelFormat(r.ComputeInterceptSlopePixelType());
				r.SetUseTargetPixelType(true);
				r.SetTargetPixelType(pixelformat);
				{
					unsigned int rescale_type_size = 0;
					if (pixelformat.GetBitsAllocated()%8!=0)
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
							if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
							return QString("Internal error (re-scale)");
						}
					}
					else
					{
						rescale_type_size = pixelformat.GetBitsAllocated()/8;
					}
					char * in_buffer;
					try
					{
						in_buffer = new char[image.GetBufferLength()];
					}
					catch(std::bad_alloc&)
					{
						if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
						return QString("Buffer allocation error");
					}
					if (!in_buffer)
					{
						if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
						return QString("Buffer allocation error");
					}
					if (!image.GetBuffer(in_buffer))
					{
						delete [] in_buffer;
						if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
						return QString("Buffer is NULL");
					}
					rescaled_buffer_size
						= dimx*dimy*dimz*rescale_type_size*pixelformat.GetSamplesPerPixel();
					try { rescaled_buffer = new char[rescaled_buffer_size]; }
					catch(std::bad_alloc&) { return QString("Buffer allocation error"); }
					if (!rescaled_buffer)
					{
						if (in_buffer) delete [] in_buffer;
						if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
						return QString("Buffer is NULL");
					}
					const bool ok_rescale = r.Rescale(rescaled_buffer, in_buffer, image.GetBufferLength());
					delete [] in_buffer;
					if (ok_rescale)
					{
						rescale_ = true;
					}
					else
					{
						std::cout << f.toStdString() << " : rescaling failed" << std::endl;
						pixelformat = image.GetPixelFormat();
					}
				}
			}
			else
			{
// FIXME Modality LUT
//				const mdcm::DataSet & ds = image_reader.GetFile().GetDataSet();
//				if (has_modality_lut_sq(ds))
//				{
//					;;
//				}
//				else
//				{
					pixelformat = image.GetPixelFormat();
//				}
			}
		}
		else
		{
			pixelformat = image.GetPixelFormat();
		}
	}
	else
	{
		pixelformat = image.GetPixelFormat();
	}
	samples_per_pix = pixelformat.GetSamplesPerPixel();
	if (pixelformat.GetBitsAllocated()<8)
	{
		if (samples_per_pix>1)
		{
			const QString tmp_s0 =
				QString("Bits allocated = ") +
				QVariant((int)pixelformat.GetBitsAllocated()).toString() +
				QString(",\n samples per pixel = ") +
				QVariant((int)samples_per_pix).toString() +
				QString(",\nnot supported.");
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return tmp_s0;
		}
		if (pixelformat.GetBitsAllocated()==1)
		{
			singlebit = true;
		}
		else
		{
			const QString tmp_s0 =
				QString("Bits allocated = ") +
				QVariant((int)pixelformat.GetBitsAllocated()).toString() +
				QString(", not supported.");
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return tmp_s0;
		}
	}
	else
	{
		type_size = (pixelformat.GetBitsAllocated()%8!=0)
					? static_cast<unsigned int>(ceil((double)pixelformat.GetBitsAllocated()/8.0))
					: pixelformat.GetBitsAllocated()/8;
	}
	if (singlebit)
	{
		const size_t singlebit_buffer_size = image.GetBufferLength();
		unsigned char * singlebit_buffer;
		try
		{
			singlebit_buffer = new unsigned char[singlebit_buffer_size];
		}
		catch(std::bad_alloc&)
		{
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer allocation error");
		}
		if (!singlebit_buffer)
		{
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer allocation error");
		}
		if (!image.GetBuffer((char*)singlebit_buffer))
		{
			delete [] singlebit_buffer;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer is NULL");
		}
		not_rescaled_buffer_size=dimx*dimy*dimz;
		if (not_rescaled_buffer_size != singlebit_buffer_size*8)
		{
			delete [] singlebit_buffer;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Wrong buffer size");
		}
		try
		{
			not_rescaled_buffer = new char[not_rescaled_buffer_size];
		}
		catch(std::bad_alloc&)
		{
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer allocation error");
		}
		if (!not_rescaled_buffer)
		{
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString("Buffer allocation error");
		}
		size_t j = 0;
		for (size_t x = 0; x < singlebit_buffer_size; x++)
		{
			const unsigned char c = singlebit_buffer[x];
			not_rescaled_buffer[j  ] = (c &  0x1) ? 255 : 0;
			not_rescaled_buffer[j+1] = (c &  0x2) ? 255 : 0;
			not_rescaled_buffer[j+2] = (c &  0x4) ? 255 : 0;
			not_rescaled_buffer[j+3] = (c &  0x8) ? 255 : 0;
			not_rescaled_buffer[j+4] = (c & 0x10) ? 255 : 0;
			not_rescaled_buffer[j+5] = (c & 0x20) ? 255 : 0;
			not_rescaled_buffer[j+6] = (c & 0x40) ? 255 : 0;
			not_rescaled_buffer[j+7] = (c & 0x80) ? 255 : 0;
			j += 8;
		}
		delete [] singlebit_buffer;
		buffer      = not_rescaled_buffer;
		buffer_size = not_rescaled_buffer_size;
	}
	else
	{
		if (rescale_ && rescaled_buffer)
		{
			buffer      = rescaled_buffer;
			buffer_size = rescaled_buffer_size;
		}
		else
		{
			not_rescaled_buffer_size = image.GetBufferLength();
			if (image.GetPhotometricInterpretation()==
					mdcm::PhotometricInterpretation::PALETTE_COLOR)
			{
				not_rescaled_buffer_size*=3;
				if (not_rescaled_buffer_size!=3*dimx*dimy*dimz*type_size*samples_per_pix)
				{
					const QString tmp_s0 =
						QString("Buffer size wrong\n") +
						QVariant((int)not_rescaled_buffer_size).toString() +
						QString("\nbut must be\n") +
						QVariant((int)(3*dimx*dimy*dimz*type_size*samples_per_pix)).toString();
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return tmp_s0;
				}
			}
			else
			{
				if (not_rescaled_buffer_size!=dimx*dimy*dimz*type_size*samples_per_pix)
				{
					const QString tmp_s0 =
						QString("Buffer size wrong\n") +
						QVariant(static_cast<int>(not_rescaled_buffer_size)).toString() +
						QString("\nbut must be\n") +
						QVariant(static_cast<int>(dimx*dimy*dimz*type_size*samples_per_pix)).toString();
					if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
					return tmp_s0;
				}
			}
			try { not_rescaled_buffer = new char[not_rescaled_buffer_size]; }
			catch(std::bad_alloc&) { return QString("Buffer allocation error"); }
			if (!not_rescaled_buffer) return QString("Buffer allocation error");
			if (!image.GetBuffer(not_rescaled_buffer))
			{
				delete [] not_rescaled_buffer;
				if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
				return QString("Buffer is NULL");
			}
			buffer      = not_rescaled_buffer;
			buffer_size = not_rescaled_buffer_size;
		}
	}
	if (supp_palette_color)
	{
		if (!red_subscript)
		{
			if (not_rescaled_buffer)
			{
				delete [] not_rescaled_buffer;
			}
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString(
				"Error (subscript is NULL),\n"
				"can not apply Supplemental LUT");
		}
		*shift_tmp = 0.0;
		*scale_tmp = 1.0;
		mdcm::ApplySupplementalLUT slut;
		slut.SetInput(image);
		slut.Apply();
		image = slut.GetOutput();
		*red_subscript = slut.GetRedSubscript();
		pixelformat = image.GetPixelFormat();
		if (!rescale_)
		{
			supp_rescaled_buffer_size = image.GetBufferLength();
			try
			{
				supp_rescaled_buffer =
					new char[supp_rescaled_buffer_size];
			}
			catch(std::bad_alloc&)
			{
				if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
				return QString("Buffer allocation error");
			}
			if (!supp_rescaled_buffer)
			return QString("Buffer allocation error");
			if (!image.GetBuffer(supp_rescaled_buffer))
			{
				delete [] supp_rescaled_buffer;
				if (not_rescaled_buffer)
				{
					delete [] not_rescaled_buffer;
				}
				if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
				return QString("Buffer is NULL");
			}
			buffer = supp_rescaled_buffer;
			buffer_size = supp_rescaled_buffer_size;
		}
		else // should not happen
		{
			(void)supp_rescaled_buffer_size;
			if (rescaled_buffer) delete [] rescaled_buffer;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			return QString(
				"Error (buffer rescaled),\n"
				"can not apply Supplemental LUT");
		}
	}
	const size_t xy = buffer_size/dimz;
	for (unsigned int j = 0; j < dimz; j++)
	{
		char * p__ = NULL;
		bool badalloc = false;
		try { p__ = new char[xy]; }
		catch(std::bad_alloc&) { badalloc = true; }
		if (p__ && !badalloc)
		{
			memcpy(p__,&(buffer[j*xy]),xy);
			data.push_back(p__);
		}
		else
		{
			if (not_rescaled_buffer)  delete [] not_rescaled_buffer;
			if (rescaled_buffer)      delete [] rescaled_buffer;
			if (supp_rescaled_buffer) delete [] supp_rescaled_buffer;
			if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
			*ok = false;
			return QString("Memory allocation error");
		}
	}
	if (not_rescaled_buffer)  delete [] not_rescaled_buffer;
	if (rescaled_buffer)      delete [] rescaled_buffer;
	if (supp_rescaled_buffer) delete [] supp_rescaled_buffer;
	if (elscint && !elscf.isEmpty()) QFile::remove(elscf);
	*ok = true;
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
	const int max_3d_tex_size,
	GLWidget * gl,
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
	const mdcm::DataSet & ds, QProgressDialog * pb,
	float tolerance)
{
#if 0
	std::cout << "common: red_subscript = " << red_subscript << std::endl;
#endif
	QString message("");
	bool error = false;
	const SettingsWidget * wsettings =
		static_cast<const SettingsWidget *>(settings);
	//
	for (unsigned int x = 0; x < tmp0.size(); x++)
	{
		if (tmp0.at(x).empty())
		{
			*ok = false;
			return QString("read_enhanced_common error (01)");
		}
		for (
			std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator
			it = tmp0.at(x).begin(); it != tmp0.at(x).end(); ++it)
		{
			const unsigned int idx__ = it->first;
			if (!(
					idx__<data.size() && data.at(idx__) &&
					idx__<values.size()))
			{
				*ok = false;
				return QString("read_enhanced_common error (02)");
			}
		}
	}
	//
	for (unsigned int x = 0; x < tmp0.size(); x++)
	{
#ifdef ENHANCED_PRINT_INFO
		if (!min_load) std::cout << "Image: " << x << std::endl;
#endif
		QString message_("");
		std::vector<char*>   tmp3;
		std::vector<double*> tmp4;
		QStringList tmp5;
		QList< QPair< double, double> > tmp6;
		bool tmp4_ok = true;
		QStringList window_centers_l;
		QStringList window_widths_l;
		QStringList lut_functions_l;
		QStringList lateralities;
		QStringList body_parts;
		QStringList acquisition_datetimes;
		QStringList reference_datetimes;
		ImageOverlays overlays;
		unsigned int j = 0;
#ifdef ENHANCED_PRINT_INFO
		if (!min_load) std::cout << " Indices:";
#endif
		std::map<
			unsigned int,
			unsigned int,
			std::less<unsigned int> > tmp1;
		for (
			std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator
			it = tmp0.at(x).begin(); it != tmp0.at(x).end(); ++it)
		{
			tmp1[it->second] = it->first;
		}
		if (tmp0.at(x).size()!=tmp1.size())
		{
			error = true;
			tmp4_ok = false;
			*ok = false;
			message_ = QString("tmp0.at(x).size()!=tmp1.size()");
#ifdef ENHANCED_PRINT_INFO
			if (!min_load)
			{
				std::cout << " tmp0.at(x).size()!=tmp1.size()";
			}
#endif
			tmp1.clear();
		}
		for (
			std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator
			it = tmp1.begin(); it != tmp1.end(); ++it)
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
			if (
				idx__<data.size() &&
				data.at(idx__) &&
				idx__<values.size())
			{
				tmp3.push_back(data.at(idx__));
				double * ss = new double[9];
				if (sop==QString("1.2.840.10008.5.1.4.1.1.6.2")) // US
				{
					if (
						values.at(idx__).vol_pos_ok &&
						values.at(idx__).vol_orient_ok)
					{
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
					}
					else { tmp4_ok = false; delete [] ss; }
				}
/*
				else if (sop==QString("1.2.840.10008.5.1.4.1.1.77.1.6")) // VL Whole Slide Microscopy
				{
					if (
						values.at(idx__).vol_pos_ok &&
						values.at(idx__).vol_orient_ok)
					{
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
					}
					else { tmp4_ok = false; delete [] ss; }

				}
*/
				// with iop/ipp
				else
				{
					if (!values.at(idx__).pat_pos.isEmpty() &&
						!values.at(idx__).pat_orient.isEmpty())
					{
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
						if (ok_o && ok_p)
						{
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
						else { tmp4_ok = false; delete [] ss; }
					}
					else { tmp4_ok = false; delete [] ss; }
				}
				QPair< double, double> rp;
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
					values.at(idx__).frame_acquisition_datetime.trimmed());
				reference_datetimes.push_back(
					values.at(idx__).frame_reference_datetime.trimmed());
				if (image_overlays.all_overlays.contains(idx__))
				{
					overlays.all_overlays[it->first] =
						image_overlays.all_overlays.value(idx__);
				}
				//
				j++;
			}
			else 
			{
				error = true;
				tmp4_ok = false;
				*ok = false;
				message_ = QString(
					"!(idx__<data.size() && data.at(idx__) && idx__<values.size())");
				break;
			}
		}
#ifdef ENHANCED_PRINT_INFO
		if (!min_load) std::cout << std::endl;
#endif
		//
		if (!error)
		{
			bool   geom_ok = false;
			bool   equi_ = false;
			bool   one_direction_ = false;
			double origin_x_gen, origin_y_gen, origin_z_gen;
			itk::Matrix<itk::SpacePrecisionType,3,3> direction;
			double spacing_x, spacing_y, spacing_z;
			double origin_x, origin_y, origin_z;
			double spacing_z_tmp;
			double dircos_gen[] = {0.0,0.0,0.0,0.0,0.0,0.0};
			float  slices_dir_x, slices_dir_y, slices_dir_z;
			float  up_dir_x, up_dir_y, up_dir_z;
			float  center_x, center_y, center_z;
			std::vector<ImageSlice*> slices;
			const bool enable_gl = min_load ? false : ok3d;
			bool skip_texture = min_load ?  true : !wsettings->get_3d();
			const int new_id = min_load ? -1 : CommonUtils::get_next_id();
			const int lut1   = 0;
			ImageVariant * ivariant = new ImageVariant(
				new_id,
				enable_gl,
				skip_texture,
				gl,
				lut1);
			//
			for (int i = 0; i < (int)tmp1.size(); i++)
			{
				AnatomyDesc anatomy_desc;
				anatomy_desc.laterality =
					(!lateralities.isEmpty()) ? lateralities.at(i) : QString("");
				anatomy_desc.body_part =
					(!body_parts.isEmpty()) ? body_parts.at(i) : QString("");
				ivariant->anatomy[i] = anatomy_desc;
			}
			//
			{
				if (overlays.all_overlays.count() > 0)
				{
					ivariant->image_overlays.all_overlays = overlays.all_overlays; // FIXME
				}
			}
			//
			{
				QMap<qlonglong, QString> acq_times_tmp;
				for (int i = 0; i < acquisition_datetimes.size(); i++)
				{
					if (!acquisition_datetimes.at(i).isEmpty())
					{
						bool tmp2_ok = false;
						const double dacq = QVariant(
							acquisition_datetimes.at(i))
								.toDouble(&tmp2_ok);
						if (tmp2_ok && dacq > 0)
						{
							acq_times_tmp[(qlonglong)(dacq*1000.0 + 0.5)] =
								acquisition_datetimes.at(i);
						}
					}
				}
				QMap<qlonglong,QString>::const_iterator acqit =
					acq_times_tmp.constBegin();
				if (acqit != acq_times_tmp.constEnd())
				{
					const QString tmp57 = acqit.value();
					if (tmp57.size() >= 14)
					{
						ivariant->acquisition_date =
							tmp57.left(8);
						ivariant->acquisition_time =
							tmp57.right(tmp57.size() - 8);
					}
				}
			}
			//
			bool invalidate = false;
			double spacing_tmp0[2] = {0.0, 0.0 };
			double spacing_tmp1[2] = {0.0, 0.0 };
			bool spacing_ok = false;
			for (int i = 0; i < tmp5.size(); i++)
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
				std::vector<SpectroscopySlice*> empty__;
				geom_ok = generate_geometry(
					slices, empty__,
					tmp4,
					rows_, columns_,
					spacing_x, spacing_y, &spacing_z_tmp,
					enable_gl, skip_texture, gl,
					&equi_, &one_direction_,
					&origin_x_gen, &origin_y_gen, &origin_z_gen,
					dircos_gen,
					&slices_dir_x, &slices_dir_y, &slices_dir_z,
					&up_dir_x, &up_dir_y, &up_dir_z,
					&center_x, &center_y, &center_z,
					tolerance,
					false);
			}
			//
			ivariant->equi = equi_;
			//
			if (geom_ok)
			{
				for (unsigned int k = 0; k < slices.size(); k++)
					ivariant->di->image_slices.push_back(slices[k]);
				ivariant->di->slices_generated = true;
				if (spacing_z_tmp < 0)
				{
					spacing_z = 0.00001;
					invalidate = true;
				}
				else if (spacing_z_tmp <= 0.00001 && one_direction_)
				{
					spacing_z = 0.00001;
					ivariant->di->skip_texture = true;
				}
				else
				{
					spacing_z = spacing_z_tmp;
				}
				origin_x = origin_x_gen;
				origin_y = origin_y_gen;
				origin_z = origin_z_gen;
				const float row_dircos_x = (float)dircos_gen[0];
				const float row_dircos_y = (float)dircos_gen[1];
				const float row_dircos_z = (float)dircos_gen[2];
				const float col_dircos_x = (float)dircos_gen[3];
				const float col_dircos_y = (float)dircos_gen[4];
				const float col_dircos_z = (float)dircos_gen[5];
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
					ivariant->di->skip_texture = true;
					ivariant->di->slices_from_dicom = false;
				}
				else
				{
					ivariant->one_direction = one_direction_;
					ivariant->di->slices_from_dicom = true;
				}
			}
			else
			{
				spacing_x = spacing_x_read > 0 ? spacing_x_read : 1;
				spacing_y = spacing_y_read > 0 ? spacing_y_read : 1;
				spacing_z = spacing_z_read > 0 ? spacing_z_read : 0.00001;
				origin_x  = origin_x_read;
				origin_y  = origin_y_read;
				origin_z  = origin_z_read;
				const float row_dircos_x = (float)dircos_read[0];
				const float row_dircos_y = (float)dircos_read[1];
				const float row_dircos_z = (float)dircos_read[2];
				const float col_dircos_x = (float)dircos_read[3];
				const float col_dircos_y = (float)dircos_read[4];
				const float col_dircos_z = (float)dircos_read[5];
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
			}
			for (unsigned int k = 0; k < tmp4.size(); k++)
			{
				if (tmp4.at(k)) delete [] tmp4[k];
			}
			tmp4.clear();
			//
			if (sop==QString("1.2.840.10008.5.1.4.1.1.6.2"))
				read_us_regions(ds, ivariant);
			//
			double window_center = -999999, window_width = -999999;
			short lut_function = -1;
			const size_t tmp1s = window_centers_l.size();
			const bool tmp1ok =
				(tmp1s > 0) &&
				((size_t)window_widths_l.size() == tmp1s) &&
				((size_t)lut_functions_l.size() == tmp1s);
			if (tmp1ok)
			{
				QList<double> tmp1w;
				QList<double> tmp1c;
				QList<short>  tmp1l;
				for (size_t k = 0; k < tmp1s; k++)
				{
					QStringList w__ =
						window_widths_l.at(k).split(QString("\\"),
						QString::SkipEmptyParts);
					QStringList c__ =
						window_centers_l.at(k).split(QString("\\"),
						QString::SkipEmptyParts);
					if (!w__.empty() && !c__.empty())
					{
						bool ok_c = false, ok_w = false;
						const double tmp_w =
							QVariant(w__.at(0).trimmed().remove(QChar('\0')))
								.toDouble(&ok_w);
						const double tmp_c =
							QVariant(c__.at(0).trimmed().remove(QChar('\0')))
								.toDouble(&ok_c);
						if (ok_c && ok_w)
						{
							double tmp7890 = tmp_w;
							short  tmp7891 = -1;
							const QString tmp7892 =
								lut_functions_l.at(k).trimmed().toUpper();
							if (tmp7892 == QString("SIGMOID"))
							{
								tmp7891 = 2;
							}
							else if (tmp7892 == QString("LINEAR_EXACT"))
							{
								tmp7891 = 1;
							}
							else
							{
								tmp7891 = 0;
								if (tmp_w >= 2) tmp7890 -= 1;
							}
							tmp1c.push_back(tmp_c);
							tmp1w.push_back(tmp7890);
							tmp1l.push_back(tmp7891);
						}
					}
				}
				/////////////////////
				const int tmp1c_size = tmp1c.size();
				if (tmp1c_size == 1)
				{
					window_center = tmp1c.at(0);
					window_width  = tmp1w.at(0);
					lut_function  = tmp1l.at(0);
				}
				else if (tmp1c_size > 1)
				{
					bool tmp5468ok = true;
					for (int k = 1; k < tmp1c_size; k++)
					{
						if (!(
							itk::Math::FloatAlmostEqual<double>(tmp1c.at(k), tmp1c.at(k - 1)) &&
							itk::Math::FloatAlmostEqual<double>(tmp1w.at(k), tmp1w.at(k - 1))))
						{
							tmp5468ok = false;
							break;
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
				read_window(ds, &window_center, &window_width, &lut_function);
			}
			ivariant->di->default_us_window_center = ivariant->di->us_window_center = window_center;
			ivariant->di->default_us_window_width  = ivariant->di->us_window_width  = window_width;
			ivariant->di->lut_function = lut_function;
			ivariant->di->supp_palette_subsciptor = red_subscript;
			const bool no_warn_rescale =
				(apply_rescale)
				? wsettings->get_rescale()
				: true;
			if (sop==QString("1.2.840.10008.5.1.4.1.1.6.2")) // US
			{
				message_ = CommonUtils::gen_itk_image(ok,
					tmp3,
					false,
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
					max_3d_tex_size,
					gl,
					pb,
					false);
			}
			else
			{
				// GDCM can not read rescale from per frame groups
				const bool rescale_tmp =
					(!apply_rescale ||
						pixelformat.GetSamplesPerPixel() > 1)
					? false : wsettings->get_rescale();
				const double saved_window_center = ivariant->di->default_us_window_center;
				const double saved_window_width = ivariant->di->default_us_window_width;
				if (rescale_tmp)
				{
					message_ = CommonUtils::gen_itk_image(ok,
						tmp3,
						false,
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
						0,
						NULL,
						pb,
						false);
					if (*ok)
					{
						// check if not only 1 and 0
						bool really_rescale = false;
						for (int u = 0; u < tmp6.size(); u++)
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
							message = CommonUtils::apply_per_slice_rescale(
								ivariant, tmp6);
						}
						if (true)
						{
							ivariant->di->default_us_window_center = saved_window_center;
							ivariant->di->us_window_center = saved_window_center;
							ivariant->di->default_us_window_width = saved_window_width;
							ivariant->di->us_window_width = saved_window_width;
						}
						const bool ok_ =
							CommonUtils::reload_monochrome(
								ivariant,
								enable_gl,
								gl,
								max_3d_tex_size,
								wsettings->get_resize(),
								wsettings->get_size_x(),
								wsettings->get_size_y());
						if (!ok_) *ok = false;
					}
				}
				else
				{
					message_ = CommonUtils::gen_itk_image(ok,
						tmp3,
						false,
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
						max_3d_tex_size,
						gl,
						pb,
						false);
				}
			}
			if (*ok)
			{
				if (geom_ok)
				{
					// one inst. UID for all slices
					const QString instance_uid = read_instance_uid(ds);
					for (int z = 0; z < ivariant->di->idimz; z++)
						ivariant->image_instance_uids[z] = instance_uid;
					//
					ivariant->di->slices_direction_x = slices_dir_x;
					ivariant->di->slices_direction_y = slices_dir_y;
					ivariant->di->slices_direction_z = slices_dir_z;
					ivariant->di->up_direction_x = up_dir_x;
					ivariant->di->up_direction_y = up_dir_y;
					ivariant->di->up_direction_z = up_dir_z;
					ivariant->di->default_center_x = ivariant->di->center_x = center_x;
					ivariant->di->default_center_y = ivariant->di->center_y = center_y;
					ivariant->di->default_center_z = ivariant->di->center_z = center_z;
					if (!ivariant->equi)
					{
						float cx = 0.0f, cy = 0.0f, cz = 0.0f;
						CommonUtils::calculate_center_notuniform(
							ivariant->di->image_slices,&cx,&cy,&cz);
						ivariant->di->default_center_x = ivariant->di->center_x = cx;
						ivariant->di->default_center_y = ivariant->di->center_y = cy;
						ivariant->di->default_center_z = ivariant->di->center_z = cz;
					}
				}
				if (ivariant->equi)
				{
					if (ivariant->di->idimz < 7)
						ivariant->di->transparency = false;
				}
				else
				{
					if (!ivariant->one_direction)
						ivariant->di->transparency = false;
					ivariant->di->filtering = 0;
				}
				if (
					sop==QString("1.2.840.10008.5.1.4.1.1.4.1") || // Enhanced MR
					sop==QString("1.2.840.10008.5.1.4.1.1.2.1") || // Enhanced CT
					sop==QString("1.2.840.10008.5.1.4.1.1.130") || // Enhanced PET
					sop==QString("1.2.840.10008.5.1.4.1.1.2.2") || // Legacy CT
					sop==QString("1.2.840.10008.5.1.4.1.1.4.4") || // Legacy MR
					sop==QString("1.2.840.10008.5.1.4.1.1.4.3") || // Enhanced MR color
					sop==QString("1.2.840.10008.5.1.4.1.1.128.1")) // Legacy PET
					ivariant->unit_str = QString(" mm");
				ivariant->di->shift_tmp = shift_tmp;
				ivariant->di->scale_tmp = scale_tmp;
				if (geom_ok) ivariant->iod_supported = true;
				const int instance_number = read_instance_number(ds);
				if (instance_number>=0)
				{
					ivariant->instance_number =
						read_instance_number(ds);
				}
				read_ivariant_info_tags(ds, ivariant);
				if (sop==QString("1.2.840.10008.5.1.4.1.1.130") ||
					sop==QString("1.2.840.10008.5.1.4.1.1.128.1"))
				{
					;;
				}
				else if (
					sop==QString("1.2.840.10008.5.1.4.1.1.4.1") ||
					sop==QString("1.2.840.10008.5.1.4.1.1.4.4"))
				{
					ivariant->iinfo = read_enhmr_spectro_info(ds, false);
				}
				else if (sop==QString("1.2.840.10008.5.1.4.1.1.4.2"))
				{
					ivariant->iinfo = read_enhmr_spectro_info(ds, true);
				}
				else if (
					sop==QString("1.2.840.10008.5.1.4.1.1.2.1") ||
					sop==QString("1.2.840.10008.5.1.4.1.1.2.2"))
				{
					ivariant->iinfo = read_enhct_info(ds);
				}
				CommonUtils::reset_bb(ivariant);
				IconUtils::icon(ivariant);
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
			for (unsigned int k = 0; k < tmp4.size(); k++)
			{
				if (tmp4.at(k)) delete [] tmp4[k];
			}
			tmp4.clear();
		}
		if (!message_.isEmpty()) message.append(QString("\n")+message_);
#ifdef ENHANCED_PRINT_INFO
		if (!min_load) std::cout << "*********" << std::endl;
#endif
	}
	return message;
}

bool DicomUtils::enhanced_process_indices(
	std::vector< std::map< unsigned int,unsigned int,std::less<unsigned int> > > & tmp0,
	const DimIndexValues & idx_values, const FrameGroupValues & values,
	const int dim6th, const int dim5th, const int dim4th, const int dim3rd)
{
	bool error = false;
	std::list<unsigned int> tmp1_1;
	std::list<unsigned int> tmp1_2;
	std::list<unsigned int> tmp1_3;
	if (dim6th>=0)
	{
		for (unsigned int x = 0; x < idx_values.size(); x++)
			tmp1_1.push_back(idx_values.at(x).idx.at(dim6th));
		tmp1_1.sort();
		tmp1_1.unique();
	}
	else
	{
		tmp1_1.push_back(1);
	}
	if (dim5th>=0)
	{
		for (unsigned int x = 0; x < idx_values.size(); x++)
			tmp1_2.push_back(idx_values.at(x).idx.at(dim5th));
		tmp1_2.sort();
		tmp1_2.unique();
	}
	else
	{
		tmp1_2.push_back(1);
	}
	if (dim4th>=0)
	{
		for (unsigned int x = 0; x < idx_values.size(); x++)
			tmp1_3.push_back(idx_values.at(x).idx.at(dim4th));
		tmp1_3.sort();
		tmp1_3.unique();
	}
	else
	{
		tmp1_3.push_back(1);
	}
	if (!idx_values.empty())
	{
		for (
			std::list<unsigned int>::const_iterator it1 = tmp1_1.begin();
			it1 != tmp1_1.end();
			it1++)
		{
			for (
				std::list<unsigned int>::const_iterator it2 = tmp1_2.begin();
				it2 != tmp1_2.end();
				it2++)
			{
				for (
					std::list<unsigned int>::const_iterator it3 = tmp1_3.begin();
					it3 != tmp1_3.end();
					it3++)
				{
					std::map< unsigned int,unsigned int,std::less<unsigned int> > tmp2;
					std::list<unsigned int> tmp2_test;
					for (unsigned int x = 0; x < idx_values.size(); x++)
					{
						const int idx1 =
							dim6th >= 0 && dim6th < (int)idx_values.at(x).idx.size()
							? idx_values.at(x).idx.at(dim6th)
							: -1;
						const int idx2 =
							dim5th >= 0 && dim5th < (int)idx_values.at(x).idx.size()
							? idx_values.at(x).idx.at(dim5th)
							: -1;
						const int idx3 =
							dim4th >= 0 && dim4th < (int)idx_values.at(x).idx.size()
							? idx_values.at(x).idx.at(dim4th)
							: -1;
						const int idx4 =
							dim3rd >= 0 && dim3rd < (int)idx_values.at(x).idx.size()
							? idx_values.at(x).idx.at(dim3rd)
							: -1;
						if (
							(idx1<0 || idx1==(int)*it1) &&
							(idx2<0 || idx2==(int)*it2) &&
							(idx3<0 || idx3==(int)*it3))
						{
							if (idx4<0)
							{
								tmp2[idx_values.at(x).id] = idx_values.at(x).id;
								tmp2_test.push_back(idx_values.at(x).id);
							}
							else
							{
								const unsigned int tmp2_pos =
									idx_values.at(x).idx.at(dim3rd);
								tmp2[idx_values.at(x).id] = tmp2_pos;
								tmp2_test.push_back(tmp2_pos);
							}
						}
					}
					const size_t tmp2_test_size0 = tmp2_test.size();
					if (tmp2_test_size0 > 0)
					{
						tmp2_test.sort();
						tmp2_test.unique();
						const size_t tmp2_test_size1 = tmp2_test.size();
						if (tmp2_test_size0!=tmp2_test_size1)
						{
							error = true;
						}
						if (!error) tmp0.push_back(tmp2);
						else break;
					}
				}
				if (error) break;
			}
			if (error) break;
		}
	}
	else
	{
		std::map< unsigned int,unsigned int,std::less<unsigned int> > tmp2;
		for (unsigned int x = 0; x < values.size(); x++)
			tmp2[x] = x;
		tmp0.push_back(tmp2);
	}
	return !error;
}

QString DicomUtils::read_enhanced_3d_6d(
	bool * ok, std::vector<ImageVariant*> & ivariants,
	const QString & sop,
	const QString & efilename,
	const std::vector<char*> & data,
	const ImageOverlays & image_overlays,
	const unsigned int rows_, const unsigned int columns_,
	const mdcm::PixelFormat & pixelformat,
	const mdcm::PhotometricInterpretation & pi,
	const int dim6th, const int dim5th, const int dim4th, const int dim3rd,
	const DimIndexValues & idx_values, const FrameGroupValues & values,
	const bool ok3d, const int max_3d_tex_size, GLWidget * gl,
	const bool min_load, const QWidget * settings,
	double * dircos_read,
	const int red_subscript,
	const double spacing_x_read, const double spacing_y_read, const double spacing_z_read,
	const double origin_x_read, const double origin_y_read, const double origin_z_read,
	const double shift_tmp, const double scale_tmp,
	const bool apply_rescale,
	const mdcm::DataSet & ds,
	QProgressDialog * pb,
	float tolerance)
{
	QString message_ ;
	std::vector< std::map< unsigned int,unsigned int,std::less<unsigned int> > > tmp0;
	*ok = enhanced_process_indices(
		tmp0, idx_values, values,
		dim6th, dim5th, dim4th, dim3rd);
	if (*ok)
		message_ = read_enhanced_common(
			ok, ivariants,
			sop,
			efilename,
			data,
			image_overlays,
			rows_, columns_,
			pixelformat, pi,
			tmp0,
			idx_values,  values,
			ok3d, max_3d_tex_size, gl, min_load,
			settings,
			dircos_read,
			red_subscript,
			spacing_x_read, spacing_y_read, spacing_z_read,
			origin_x_read, origin_y_read, origin_z_read,
			shift_tmp, scale_tmp,
			apply_rescale,
			ds,
			pb,
			tolerance);
	else
		message_ = QString("Can not build indices");
	return message_;
}

bool DicomUtils::is_not_interleaved(const QStringList & images)
{
	const mdcm::Tag tSlicePosition(0x0020,0x1041);
	std::set<mdcm::Tag> tags;
	tags.insert(tSlicePosition);
	long tmp0 = 0;
	for (int x = 0; x < images.size(); x++)
	{
		mdcm::Reader reader;
		reader.SetFileName(images.at(x).toLocal8Bit().constData());
		const bool f_ok = reader.ReadSelectedTags(tags);
		if (!f_ok) return false;
		const mdcm::File & file = reader.GetFile();
		const mdcm::DataSet & ds = file.GetDataSet();
		if (ds.IsEmpty()) return false;
		if (ds.FindDataElement(tSlicePosition))
		{
			const mdcm::DataElement & sp_ = ds.GetDataElement(tSlicePosition);
			if (!sp_.IsEmpty() && !sp_.IsUndefinedLength() && sp_.GetByteValue())
			{
				bool sp_ok = false;
				long tmp1 = -9999999999L;
				const QString sp = QString::fromLatin1(
					sp_.GetByteValue()->GetPointer(),sp_.GetByteValue()->GetLength());
				const double spvd = QVariant(sp.trimmed().remove(QChar('\0'))).toDouble(&sp_ok);
				if (sp_ok) tmp1 = 1000 * CommonUtils::set_digits(spvd,3);
				else return false;
				if (x>0 && tmp0>tmp1-10 && tmp0<tmp1+10) return false;
				tmp0 = tmp1;
			}
		}
	}
	return true;
}

bool DicomUtils::is_mosaic(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return false;
	const mdcm::Tag tImageType(0x0008,0x0008);
	if (ds.FindDataElement(tImageType))
	{
		const mdcm::DataElement & de = ds.GetDataElement(tImageType);
		if (!de.IsEmpty() && !de.IsUndefinedLength() && de.GetByteValue())
		{
			const QString s =
				QString::fromLatin1(de.GetByteValue()->GetPointer(),
									de.GetByteValue()->GetLength()).trimmed().toUpper();
			if (s.contains(QString("MOSAIC"))) return true;
		}
	}
	return false;
}

bool DicomUtils::is_uih_grid(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return false;
	const  mdcm::PrivateTag tMRVFrameSequence(
		0x0065,0x51,"Image Private Header");
	if (ds.FindDataElement(tMRVFrameSequence))
	{
		const mdcm::DataElement & e	=
			ds.GetDataElement(tMRVFrameSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems()>0) return true;
	}
	else if (ds.FindDataElement(mdcm::Tag(0x0065,0x1051)))
	{
		const mdcm::DataElement & e	=
			ds.GetDataElement(mdcm::Tag(0x0065,0x1051));
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			e.GetValueAsSQ();
		if (sq && sq->GetNumberOfItems()>0) return true;
	}
	return false;
}

bool DicomUtils::is_elscint(const mdcm::DataSet & ds)
{
	if (ds.IsEmpty()) return false;
	const mdcm::PrivateTag tTamarCompressionType(
		0x07a1,0x11,"ELSCINT1");
	if (ds.FindDataElement(tTamarCompressionType))
	{
		QString s;
		if (priv_get_string_value(ds, tTamarCompressionType, s))
		{
			if (s.toUpper() ==
					QString("PMSCT_RLE1") ||
				s.toUpper() ==
					QString("PMSCT_RGB1"))
			{
				return true;
			}
			else if (s.toUpper() ==
					QString("LOSSLESS RICE"))
			{
#if 0
				std::cout << "LOSSLESS RICE is currentry not supported"
					<< std::endl;
#endif
				return false;
			}
		}
	}
	return false;
}

void DicomUtils::write_encapsulated(
	const QString & in_f,
	const QString & out_f)
{
	const mdcm::Tag t(0x0042,0x0011);
	std::set<mdcm::Tag> tags;
	tags.insert(t);
	mdcm::Reader reader;
	reader.SetFileName(in_f.toLocal8Bit().constData());
	const bool ok = reader.ReadSelectedTags(tags);
	if (!ok) return;
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	if(!ds.FindDataElement(t)) return;
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty()||e.IsUndefinedLength()) return;
	const mdcm::ByteValue * bv = e.GetByteValue();
	if (bv && bv->GetPointer() &&bv->GetLength()>0)
	{
		std::ofstream o(
			out_f.toLocal8Bit().constData(),
			std::ios::binary);
		o.write(bv->GetPointer(), bv->GetLength());
		o.close();
	}
}

QString DicomUtils::suffix_mpeg(const QString & f)
{
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
	if (reader.ReadUpToTag(mdcm::Tag(0x0008,0x0016)))
	{
		mdcm::FileMetaInformation & h = reader.GetFile().GetHeader();
		const mdcm::TransferSyntax & ts = h.GetDataSetTransferSyntax();
		if (
			ts == mdcm::TransferSyntax::MPEG2MainProfileHighLevel ||
			ts == mdcm::TransferSyntax::MPEG2MainProfile)
			return QString(".mpeg");
		else if (
			ts == mdcm::TransferSyntax::MPEG4AVCH264HighProfileLevel4_1 ||
			ts == mdcm::TransferSyntax::MPEG4AVCH264BDcompatibleHighProfileLevel4_1)
			return QString(".mp4");
	}
	return QString(".bin");
}

void DicomUtils::write_mpeg(
	const QString & in_f,
	const QString & out_f)
{
	const mdcm::Tag t(0x7fe0,0x0010);
	std::set<mdcm::Tag> tags;
	tags.insert(t);
	mdcm::Reader reader;
	reader.SetFileName(in_f.toLocal8Bit().constData());
	const bool ok = reader.ReadSelectedTags(tags);
	if (!ok) return;
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	if(!ds.FindDataElement(t)) return;
	const mdcm::DataElement & e = ds.GetDataElement(t);
	if (e.IsEmpty()) return;
	const mdcm::SequenceOfFragments * sf =
		e.GetSequenceOfFragments();
	if(!sf) return;
	std::ofstream output(
		out_f.toLocal8Bit().constData(), std::ios::binary);
	sf->WriteBuffer(output);
	output.close();
}

bool DicomUtils::is_dicom_file(const QString & f)
{
	bool dicom = false;
	char b[4];
	std::ifstream fs;
	fs.open(
		f.toLocal8Bit().constData(),
		std::ios::in|std::ios::binary);
	for (long off = 128; off >= 0; off -= 128)
	{
		fs.seekg(off, std::ios_base::beg);
		if(!fs.fail() && !fs.eof())
		{
			fs.read(b, 4);
			if(!fs.fail())
			{
				if (
					b[0] == (char)'D' &&
					b[1] == (char)'I' &&
					b[2] == (char)'C' &&
					b[3] == (char)'M') dicom = true;
			}
		}
	}
	if(!dicom)
	{
		unsigned short group_no;
		fs.seekg(0, std::ios_base::beg);
		if (!fs.fail() && !fs.eof())
		{
			fs.read(
				reinterpret_cast<char *>(&group_no),
				sizeof(unsigned short));
			itk::ByteSwapper<unsigned short>::SwapFromSystemToLittleEndian(
				&group_no);
			// 0x0003 and 0x0005 are illegal, but files exist
			if(
				group_no==0x0002 ||
				group_no==0x0003 ||
				group_no==0x0005 ||
				group_no==0x0008) dicom = true;
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
	while(it.hasNext())
	{
		scan_files_for_rtstruct_image(it.next(), ref_files);
	}
}

void DicomUtils::scan_files_for_rtstruct_image(
	const QString & p, QList<QStringList> & ref_files)
{
	if (p.isEmpty()) return;
	QDir dir(p);
	QStringList flist =
		dir.entryList(QDir::Files|QDir::Readable,QDir::Name);
	std::vector<std::string> filenames;
	for (int x = 0; x < flist.size(); x++)
	{
		QApplication::processEvents();
		const QString tmp0 =
			QDir::toNativeSeparators(
				dir.absolutePath() + QDir::separator() + flist.at(x));
		if (is_dicom_file(tmp0))
			filenames.push_back(
				std::string(tmp0.toLocal8Bit().constData()));
	}
	flist.clear();
	//
	std::vector<std::string> t0_files;
	{
		mdcm::Scanner s0;
		const mdcm::Tag t0(0x0020,0x0052);
		s0.AddTag(t0);
		const bool b0 = s0.Scan(filenames);
		if(!b0) return;
		mdcm::Scanner::ValuesType v0 = s0.GetValues();
		mdcm::Scanner::ValuesType::iterator vi0 = v0.begin();
		for (;vi0!=v0.end();++vi0)
		{
			QApplication::processEvents();
			std::vector<std::string> files__ =
				s0.GetAllFilenamesFromTagToValue(t0, (*vi0).c_str());
			for (unsigned int j = 0; j < files__.size(); j++)
			{
				t0_files.push_back(files__[j]);
			}
		}
	}
	//
	{
		mdcm::Scanner s1;
		const mdcm::Tag t1(0x0020,0x000e);
		s1.AddTag(t1);
		const bool b1 = s1.Scan(t0_files);
		if(!b1) return;
		mdcm::Scanner::ValuesType v1 = s1.GetValues();
		mdcm::Scanner::ValuesType::iterator vi1 = v1.begin();
		for (;vi1!=v1.end();++vi1)
		{
			QApplication::processEvents();
			std::vector<std::string> files__ =
				s1.GetAllFilenamesFromTagToValue(t1, (*vi1).c_str());
			QStringList t1_tmp;
			for (unsigned int j = 0; j < files__.size(); j++)
				t1_tmp.push_back(QString::fromLocal8Bit(files__[j].c_str()));
			ref_files.push_back(t1_tmp);
		}
	}
	//
	QApplication::processEvents();
}

bool DicomUtils::process_contrours_ref(
	const QString & f,
	const QString & path,
	std::vector<ImageVariant*> & tmp_ivariants,
	int max_3d_tex_size, GLWidget * gl, bool ok3d,
	const QWidget * settings,
	QProgressDialog * pb)
{
	unsigned short count_ = 0;
	if (pb)
	{
		pb->setLabelText(QString("Searching ."));
		pb->setValue(-1);
	}
	QApplication::processEvents();
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
	if (!reader.Read()) return false;
	QApplication::processEvents();
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	if (ds.IsEmpty()) return false;
	ImageVariant * tmp_ivariant = new ImageVariant(-1, false, false, NULL, 0);
	load_contour(ds,tmp_ivariant);
	QSet<QString> ref_frame_of_refs_set;
	for (int z = 0; z < tmp_ivariant->di->rois.size(); z++)
	{
		ref_frame_of_refs_set << tmp_ivariant->di->rois.at(z).ref_frame_of_ref;
	}
	QList<QString> ref_frame_of_refs_list = ref_frame_of_refs_set.toList();
	QStringList ref_frame_of_refs;
	for (int z = 0; z < ref_frame_of_refs_list.size(); z++)
	{
		ref_frame_of_refs.push_back(ref_frame_of_refs_list.at(z));
	}
	ref_frame_of_refs_set.clear();
	ref_frame_of_refs_list.clear();
	QList<QStringList> detected_files;
	for (int z = 0; z < ref_frame_of_refs.size(); z++)
	{
		scan_dir_for_rtstruct_image(path, detected_files);
	}
	for (int z = 0; z < detected_files.size(); z++)
	{
		bool referenced_slice_found = false;
		for (int k = 0; k < detected_files.at(z).size(); k++)
		{
			if (pb)
			{
				pb->setLabelText(QString("Searching .."));
				pb->setValue(-1);
			}
			QApplication::processEvents();
			QString sop_instance_uid("");
			std::set<mdcm::Tag> tags;
			mdcm::Tag tsopinstance(0x0008,0x0018);
			tags.insert(tsopinstance);
			mdcm::Reader reader;
			reader.SetFileName(
				detected_files.at(z).at(k).toLocal8Bit().constData());
			const bool f_ok = reader.ReadSelectedTags(tags);
			if (!f_ok) continue;
			const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
			if (ds.FindDataElement(tsopinstance))
			{
				const mdcm::DataElement & e =
					ds.GetDataElement(tsopinstance);
				if (!e.IsEmpty() &&
					!e.IsUndefinedLength() &&
					e.GetByteValue())
				{
					sop_instance_uid =
						QString::fromLatin1(
							e.GetByteValue()->GetPointer(),
							e.GetByteValue()->GetLength()).
								trimmed().remove(QChar('\0'));
				}
			}
			if (!sop_instance_uid.isEmpty())
			{
				for (int y = 0; y < tmp_ivariant->di->rois.size(); y++)
				{
					QMap< int, Contour* >::const_iterator it =
						tmp_ivariant->di->rois.at(y).contours.constBegin();
					while (it != tmp_ivariant->di->rois.at(y).contours.constEnd())
					{
						if (pb)
						{
							pb->setLabelText(QString("Searching ..."));
							pb->setValue(-1);
							QApplication::processEvents();
						}
						const Contour * c = it.value();
						for (int j = 0;
							j < c->ref_sop_instance_uids.size();
							j++)
						{
							if (c->ref_sop_instance_uids.at(j) ==
								sop_instance_uid)
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
				if (pb)
				{
					pb->setLabelText(QString("Searching ...."));
					pb->setValue(-1);
					QApplication::processEvents();
				}
				std::vector<ImageVariant*> ivariants;
				QStringList detected_files_tmp = detected_files.at(z);
				detected_files_tmp.sort();
				const QString message_ =
					read_dicom(
						ivariants, detected_files_tmp,
						max_3d_tex_size, gl, NULL, ok3d,
						settings,
						pb,
						2);
				if (!message_.isEmpty())
				{
					std::cout << message_.toStdString() << std::endl;
				}
#if 0
				if (ivariants.size() > 1)
				{
					std::cout <<
						"process_contrours_ref : ivariants.size() > 1"
						<< std::endl;
				}
#endif
				for (unsigned int j = 0; j < ivariants.size(); j++)
				{
					for (int i = 0; i < tmp_ivariant->di->rois.size(); i++)
					{
						if (pb)
						{
							pb->setLabelText(QString("Searching ...."));
							pb->setValue(-1);
							QApplication::processEvents();
						}
						ROI roi;
						roi.id = tmp_ivariant->di->rois.at(i).id;
						ContourUtils::copy_roi(roi, tmp_ivariant->di->rois.at(i));
						ivariants[j]->di->rois.push_back(roi);
					}
					tmp_ivariants.push_back(ivariants[j]);
					count_ += 1;
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
	const QString & uid,
	QProgressDialog * pb)
{
	QString f("");
	if (p.isEmpty())   return f;
	if (uid.isEmpty()) return f;
	bool ok = scan_files_for_pr_image(p, uid, f, pb);
	if (ok) return f;
	QDirIterator it(p, QDir::Dirs|QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		ok = scan_files_for_pr_image(it.next(), uid, f, pb);
		if (ok) break;
	}
	QApplication::processEvents();
	return f;
}

bool DicomUtils::scan_files_for_pr_image(
	const QString & p,
	const QString & uid,
	QString & file,
	QProgressDialog * pb)
{
	if (p.isEmpty())   return false;
	if (uid.isEmpty()) return false;
	std::set<mdcm::Tag> tags;
	const mdcm::Tag tSOPInstanceUID(0x0008,0x0018);
	tags.insert(tSOPInstanceUID);
	QDir dir(p);
	QStringList flist =
		dir.entryList(QDir::Files|QDir::Readable,QDir::Name);
	QApplication::processEvents();
	std::vector<std::string> filenames;
	for (int x = 0; x < flist.size(); x++)
	{
		if (pb) pb->setValue(-1);
		QApplication::processEvents();
		const QString tmp0 =
			QDir::toNativeSeparators(
				dir.absolutePath() + QDir::separator() + flist.at(x));
		if (is_dicom_file(tmp0))
		{
			mdcm::Reader reader;
			reader.SetFileName(
				tmp0.toLocal8Bit().constData());
			if (!reader.ReadSelectedTags(tags))
				continue;
			const mdcm::DataSet & ds = reader.GetFile().GetDataSet();
			QString uid_("");
			const bool ok = get_string_value(ds, tSOPInstanceUID, uid_);
			if (ok && uid == uid_)
			{
				file = tmp0;
				return true;
			}
		}
	}
	return false;
}

void DicomUtils::read_pr_ref(
	const QString & p,
	const QString & f,
	QList<PrRefSeries> & refs,
	QProgressDialog * pb)
{
	if (f.isEmpty()) return;
	const mdcm::Tag tReferencedSeriesSequence(0x0008,0x1115);
	const mdcm::Tag tSeriesInstanceUID(0x0020,0x000e);
	const mdcm::Tag tReferencedImageSequence(0x0008,0x1140);
	const mdcm::Tag tReferencedSOPInstanceUID(0x0008,0x1155);
	const mdcm::Tag tReferencedFrameNumber(0x0008,0x1160);
	mdcm::Reader reader;
	reader.SetFileName(f.toLocal8Bit().constData());
	if (!reader.Read()) return;
	const mdcm::File & file = reader.GetFile();
	const mdcm::DataSet & ds = file.GetDataSet();
	if (ds.IsEmpty()) return;
	if (!ds.FindDataElement(tReferencedSeriesSequence)) return;
	const mdcm::DataElement & eReferencedSeriesSequence =
		ds.GetDataElement(tReferencedSeriesSequence);
	mdcm::SmartPointer<mdcm::SequenceOfItems>
		sqReferencedSeriesSequence =
			eReferencedSeriesSequence.GetValueAsSQ();
	if(!(sqReferencedSeriesSequence &&
			sqReferencedSeriesSequence->GetNumberOfItems()>0))
		return;
	for (
		unsigned int x = 0;
		x < sqReferencedSeriesSequence->GetNumberOfItems();
		++x)
	{
		QApplication::processEvents();
		const mdcm::Item & item0 =
			sqReferencedSeriesSequence->GetItem(x+1);
		const mdcm::DataSet & nds0 =
			item0.GetNestedDataSet();
		QString s;
		const bool ok = get_string_value(
			nds0, tSeriesInstanceUID, s);
		if (!ok) continue;
		if (!nds0.FindDataElement(tReferencedImageSequence))
			continue;
		const mdcm::DataElement & eReferencedImageSequence =
			nds0.GetDataElement(tReferencedImageSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems>
			sqReferencedImageSequence =
				eReferencedImageSequence.GetValueAsSQ();
		if(!(sqReferencedImageSequence &&
				sqReferencedImageSequence->GetNumberOfItems()>0))
			continue;
		//
		PrRefSeries series;
		series.uid = s;
		//
		for (
			unsigned int y = 0;
			y < sqReferencedImageSequence->GetNumberOfItems();
			++y)
		{
			QApplication::processEvents();
			const mdcm::Item & item1 =
				sqReferencedImageSequence->GetItem(y+1);
			const mdcm::DataSet & nds1 =
				item1.GetNestedDataSet();
			QString s0;
			const bool ok0 = get_string_value(
				nds1, tReferencedSOPInstanceUID, s0);
			if (ok0)
			{
				PrRefImage ref;
				ref.uid = s0;
				ref.file = find_file_from_uid(p, ref.uid, pb);
				if (ref.file.isEmpty()) continue;
				std::vector<int> frames;
				const bool ok1 = get_is_values(
					nds1, tReferencedFrameNumber, frames);
				if (ok1)
				{
					for (unsigned int z = 0; z < frames.size(); z++)
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

template <typename T> QString supp_palette_grey_to_rgbUS_(
	RGBImageTypeUS::Pointer & out_image,
	const typename T::Pointer & image,
	const RGBImageTypeUS::Pointer & color_image,
	const int red_subscript,
	const ImageVariant * v)
{
	if (!v) return QString("Image is NULL");
	if (image.IsNull()) return QString("Image is NULL");
	if (out_image.IsNull()) return QString("Out image is NULL");
	if (!(red_subscript > INT_MIN))
		return QString("Internal error,\nSubscript <= INT_MIN");
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
	catch (itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	//
	//
	const double wmin  = v->di->us_window_center - v->di->us_window_width*0.5;
	const double wmax  = v->di->us_window_center + v->di->us_window_width*0.5;
	const double diff_ = (wmax-wmin);
	const double div_  = (diff_!=0.0) ? diff_ : 1.0;
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
				unsigned short c = 0;
				if (k < (double)red_subscript)
				{
					const double r = (k+(-wmin))/div_;
					if ((k>=wmin) && (k<=wmax))
					{
						c = static_cast<unsigned short>(USHRT_MAX*r);
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
	catch(itk::ExceptionObject & ex)
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
	if (!v) return QString("Image is NULL");
	if (image.IsNull()) return QString("Image is NULL");
	if (out_image.IsNull()) return QString("Out image is NULL");
	if (!(red_subscript > INT_MIN))
		return QString("Internal error,\nSubscript <= INT_MIN");
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
	catch (itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	//
	QApplication::processEvents();
	//
	unsigned long long tmp37 = 0;
	const double wmin  = v->di->us_window_center - v->di->us_window_width*0.5;
	const double wmax  = v->di->us_window_center + v->di->us_window_width*0.5;
	const double diff_ = (wmax-wmin);
	const double div_  = (diff_!=0.0) ? diff_ : 1.0;
	try
	{
		itk::ImageRegionConstIterator<T> it1(image, image->GetLargestPossibleRegion());
		it1.GoToBegin();
		itk::ImageRegionIterator<RGBImageTypeUC> it0(out_image, out_image->GetLargestPossibleRegion());
		it0.GoToBegin();
		itk::ImageRegionConstIterator<RGBImageTypeUC> it2(color_image, color_image->GetLargestPossibleRegion());
		it2.GoToBegin();
		while (!(it1.IsAtEnd()||it0.IsAtEnd()||it2.IsAtEnd()))
		{
			//
			if (tmp37%9999 == 0) QApplication::processEvents();
			//
			const RGBPixelUC & pixel = it2.Get();
			if (pixel.GetRed() > 0 || pixel.GetGreen() > 0 || pixel.GetBlue() > 0)
			{
				it0.Set(pixel);
			}
			else
			{
				const double k = static_cast<double>(it1.Get());
				unsigned char c = 0;
				if (k < (double)red_subscript)
				{
					const double r = (k+(-wmin))/div_;
					if ((k>=wmin) && (k<=wmax))
					{
						c = static_cast<unsigned char>(UCHAR_MAX*r);
					}
					else if (k>wmax)
					{
						c = static_cast<unsigned char>(UCHAR_MAX);
					}
				}
				RGBPixelUC pixel0;
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
	catch(itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	return QString("");
}

static QString supp_palette_grey_to_rgbUS(
	RGBImageTypeUS::Pointer & out_image,
	const RGBImageTypeUS::Pointer & color_image,
	const int red_subscript,
	const ImageVariant * v)
{
	QString result("");
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

static QString supp_palette_grey_to_rgbUC(
	RGBImageTypeUC::Pointer & out_image,
	const RGBImageTypeUC::Pointer & color_image,
	const int red_subscript,
	const ImageVariant * v)
{
	QString result("");
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

QString DicomUtils::read_enhmr_spectro_info(
	const mdcm::DataSet & ds,
	bool spectro)
{
	QString s("");
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
		QString tmp0("");
		//
		const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
		if (ds.FindDataElement(tSharedFunctionalGroupsSequence))
		{
			const mdcm::DataElement & e  =
				ds.GetDataElement(tSharedFunctionalGroupsSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
				e.GetValueAsSQ();
			if (sq && sq->GetNumberOfItems()==1)
			{
				const mdcm::Item & item = sq->GetItem(1);
				const mdcm::DataSet& nds = item.GetNestedDataSet();
				const mdcm::Tag tMRTimingAndRelatedSQ(0x0018,0x9112);
				if (nds.FindDataElement(tMRTimingAndRelatedSQ))
				{
					const mdcm::DataElement & e1  =
						nds.GetDataElement(tMRTimingAndRelatedSQ);
					mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 =
						e1.GetValueAsSQ();
					if (sq1 && sq1->GetNumberOfItems()==1)
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
						const mdcm::Tag tRFEchoTrainLength(
							0x0018,0x9240);
						unsigned short RFEchoTrainLength;
						if (DicomUtils::get_us_value(
								nds1,
								tRFEchoTrainLength,
								&RFEchoTrainLength))
						{
							QString sRFEchoTrainLength =
								QVariant((int)RFEchoTrainLength)
									.toString();
							sRFEchoTrainLength =
								sRFEchoTrainLength.trimmed();
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
						const mdcm::Tag tGradientEchoTrainLength(
							0x0018,0x9241);
						unsigned short GradientEchoTrainLength;
						if (DicomUtils::get_us_value(
								nds1,
								tGradientEchoTrainLength,
								&GradientEchoTrainLength))
						{
							QString sGradientEchoTrainLength =
								QVariant((int)GradientEchoTrainLength)
									.toString();
							sGradientEchoTrainLength =
								sGradientEchoTrainLength
									.trimmed().remove(QChar('\0'));
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
							const mdcm::Tag tGradientOutputType(
								0x0018,0x9180);
							QString GradientOutputType("");
							get_string_value(
								nds1,
								tGradientOutputType,
								GradientOutputType);
							GradientOutputType = GradientOutputType
								.simplified().remove(QChar('\0'));
							QString sGradientOutputType =
								GradientOutputType;
							if (GradientOutputType.toUpper() ==
								QString("DB_DT"))
							{
								sGradientOutputType = QString("T/s");
							}
							else if (GradientOutputType.toUpper() ==
								QString("ELECTRIC_FIELD"))
							{
								sGradientOutputType = QString("V/m");
							}
							else if (GradientOutputType.toUpper() ==
								QString("PER_NERVE_STIM"))
							{
								sGradientOutputType = QString("&#37;") +
									QString(" nerve stim.");
							}
							QString sGradientOutput =
								QVariant((double)GradientOutput)
									.toString();
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
			"</span><br />");
			s += tmp0;
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
		QString tmp0("");
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
		if (ds.FindDataElement(tVelocityEncodingAcquisitionSequence))
		{
			const mdcm::DataElement & e  =
				ds.GetDataElement(tVelocityEncodingAcquisitionSequence);
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
				e.GetValueAsSQ();
			if (sq && sq->GetNumberOfItems()>1)
			{
				const mdcm::Tag tVelocityEncodingDirection(0x0018,0x9090);
				tmp0 += QString("<span class='y9'>");
				tmp0 += QString("Velocity Encoding Directions");
				tmp0 += QString("</span><br />");
				for (int i = 0; i < (int)sq->GetNumberOfItems(); i++)
				{
					const mdcm::Item & item = sq->GetItem(i+1);
					const mdcm::DataSet& nds = item.GetNestedDataSet();
					if (nds.FindDataElement(tVelocityEncodingDirection))
					{
						std::vector<double> res;
						if (get_fd_values(nds,tVelocityEncodingDirection,res))
						{
							if (res.size()==3)
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
				QVariant((int)NumberofkSpaceTrajectories).toString();
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
		if (!tmp0.isEmpty()) s += QString(
			"<span class='y7'>MR Pulse Sequence Module"
			"</span><br />") + tmp0;
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
		QString tmp0("");
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
			QString B1rms =
				QVariant((double)fB1rms).toString();
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
		if (!tmp0.isEmpty()) s += QString(
			"<span class='y7'>MR Image and Spectroscopy Instance Macro"
			"</span><br />") + tmp0;

	}
	//
	if (!s.isEmpty()) s += QString("<br />");
	return s;
}

QString DicomUtils::read_enhct_info(
	const mdcm::DataSet & ds)
{
	QString s("");
	//
	s += read_CommonCTMRImageDescriptionMacro(ds);
	//
	if (!s.isEmpty()) s += QString("<br />");
	return s;
}

typedef struct 
{
	int rows;
	int columns;
	short allocated;
	bool localizer;
	QString file;
} MixedDicomSeriesInfo;

QString DicomUtils::read_dicom(
	std::vector<ImageVariant*> & ivariants,
	const QStringList & filenames,
	int max_3d_tex_size,
	GLWidget * gl,
	ShaderObj * mesh_shader,
	bool ok3d,
	const QWidget * settings,
	QProgressDialog * pb,
	short load_type,
	bool enh_original_frames)
{
	bool ok = false;
	QString message_;
	const mdcm::Tag tSOPClassUID(0x0008,0x0016);
	const mdcm::Tag tSlicePosition(0x0020,0x1041);
	const mdcm::Tag tPhotometricInterpretation(0x0028,0x0004);
	QStringList images;
	QStringList pdf_files;
	QStringList stl_files;
	QStringList video_files;
	QStringList rtstruct_ref_search;
	QString     rtstruct_ref_search_path;
	QStringList grey_softcopy_pr_files;
	QStringList color_softcopy_pr_files;
	QStringList pseudo_color_softcopy_pr_files;
	QStringList blending_softcopy_pr_files;
	QStringList xaxrf_softcopy_pr_files;
	QStringList advanced_blending_softcopy_pr_files;
	std::vector<ImageVariant*> rtstructs;
	std::vector<ImageVariant*> meshes;
	std::vector<ImageVariant*> spectroscopy_images;
	QVector<QStringList> extracted_images;
	bool enhanced     = false;
	bool supp_palette = false;
	bool multiframe   = false;
	bool mosaic       = false;
	bool elscint      = false;
	bool uihgrid      = false;
	bool mixed        = false;
	bool multiseries  = false;
	bool ultrasound   = false;
	unsigned short rows_tmp0 = 0, rows_tmp1 = 0;
	unsigned short columns_tmp0 = 0, columns_tmp1 = 0;
	unsigned short ba_tmp0 = 0, ba_tmp1 = 0;
	unsigned short bs_tmp0 = 0, bs_tmp1 = 0;
	unsigned short hb_tmp0 = 0, hb_tmp1 = 0;
	short pr_tmp0 = -1, pr_tmp1 = -1;
	bool  localizer_tmp0 = false, localizer_tmp1 = false;
	QString sop_tmp0, sop_tmp1;
	QString photometric_tmp0, photometric_tmp1;
	const SettingsWidget * wsettings =
		static_cast<const SettingsWidget*>(settings);
	std::map<unsigned int,SliceInstance> slice_pos_map;
	std::list<long> slice_pos_list;
	bool asked_about_supp_palette = false;
	bool asked_about_modality_lut = false;
	bool load_image_ref_contour = false;
	const float tolerance = 0.01f;
	int count_images = 0;
	int count_uid_errors = 0;
	//
	//
	//
	const int filenames_size = filenames.size();
	const QString filenames_num = QString(" / ") +
		QString::number(filenames_size);
	for (int x = 0; x < filenames_size; x++)
	{
		if (pb)
		{
			pb->setLabelText(QString("Loading ... ") +
				QString::number(x) + filenames_num);
			pb->show();
			pb->setValue(-1);
		}
		QApplication::processEvents();
		QString sop;
		QString photometric;
		unsigned short columns_ = 0, rows_ = 0;
		unsigned short ba_ = 0, bs_ = 0, hb_ = 0;
		short pr_ = -1;
		bool localizer_ = false;
		QFileInfo fi(filenames.at(x));
		mdcm::Reader reader;
		reader.SetFileName(
			filenames.at(x).toLocal8Bit().constData());
		if (!reader.Read()) continue;
		const mdcm::File & file = reader.GetFile();
		const mdcm::FileMetaInformation & header = file.GetHeader();
		const mdcm::TransferSyntax & ts =
			header.GetDataSetTransferSyntax();
		const mdcm::DataSet & ds = file.GetDataSet();
		if (ds.IsEmpty())
		{
			continue;
		}
		if (ds.FindDataElement(tSOPClassUID))
		{
			const mdcm::DataElement & sop_ =
				ds.GetDataElement(tSOPClassUID);
			if (!sop_.IsEmpty() &&
				!sop_.IsUndefinedLength() &&
				sop_.GetByteValue())
			{
				sop = QString::fromLatin1(
					sop_.GetByteValue()->GetPointer(),
					sop_.GetByteValue()->GetLength()).
						trimmed().remove(QChar('\0'));
			}
		}
		DicomUtils::get_string_value(
			ds, tPhotometricInterpretation, photometric);
#if 1
		if (sop==QString("1.2.840.10008.5.1.4.1.1.77.1.6")) // TODO
#else
		if (false)
#endif
		{
			// VL Whole Slide Microscopy Image Storage
			if (load_type == 0)
			{
				if (pb) pb->hide();
				QMessageBox mbox;
				mbox.setWindowModality(Qt::ApplicationModal);
				mbox.addButton(QMessageBox::Close);
				mbox.setIcon(QMessageBox::Warning);
				mbox.setText(QString(
					"VL Whole Slide Microscopy Image Storage\n"
					"is currently not supported"));
				mbox.exec();
				if (pb) pb->show();
				QApplication::processEvents();
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.481.3"))
		{
			// RTSTRUCT
			if (load_type == 0)
			{
				QFileInfo reffi(filenames.at(x));
				rtstruct_ref_search_path =
					QDir::toNativeSeparators(reffi.absolutePath());
				load_image_ref_contour = true;
				if (!load_image_ref_contour)
				{
					if (pb) pb->setValue(-1);
					QApplication::processEvents();
					ImageVariant * ivariant =
						new ImageVariant(
							CommonUtils::get_next_id(),
							ok3d,
							!wsettings->get_3d(),
							gl,
							0);
					ivariant->filenames =
						QStringList(
							QDir::toNativeSeparators(filenames.at(x)));
					load_contour(ds,ivariant);
					ContourUtils::calculate_rois_center(ivariant);
					rtstructs.push_back(ivariant);
				}
				else
				{
					rtstruct_ref_search.push_back(filenames.at(x));
				}
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.11.1"))
		{
			// Grayscale Softcopy presentation
			if (load_type == 0)
			{
				grey_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.11.2"))
		{
			// Color Softcopy presentation
			if (load_type == 0)
			{
				color_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.11.3"))
		{
			// Pseudo-color Softcopy presentation
			if (load_type == 0)
			{
				pseudo_color_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.11.4"))
		{
			// Blending Softcopy presentation
			if (load_type == 0)
			{
				blending_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.11.5"))
		{
			// XA/XRF Softcopy presentation
			if (load_type == 0)
			{
				xaxrf_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.11.8"))
		{
			// Advanced Blending Softcopy presentation
			if (load_type == 0)
			{
				advanced_blending_softcopy_pr_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.4.2"))
		{
			// Spectroscopy
			if (load_type == 0)
			{
				const QString spect_message =
					SpectroscopyUtils::ProcessData(
						ds, spectroscopy_images,
						max_3d_tex_size, gl, ok3d, pb, tolerance);
				if (!spect_message.isEmpty())
				{
					if (pb) pb->hide();
					QMessageBox mbox;
					mbox.setWindowModality(Qt::ApplicationModal);
					mbox.addButton(QMessageBox::Close);
					mbox.setIcon(QMessageBox::Warning);
					mbox.setText(spect_message);
					mbox.exec();
					if (pb) pb->show();
				}
				QApplication::processEvents();
			}
			continue;
		}
		else if (
			sop==QString("1.2.840.10008.5.1.4.1.1.66.5") ||
			sop==QString("1.2.840.10008.5.1.4.1.1.68.1"))
		{
			// Meshes
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.104.1"))
		{
			// PDF
			if (load_type == 0)
			{
				if (check_encapsulated(ds))
					pdf_files.push_back(filenames.at(x));
			}
			continue;
		}
		else if (sop==QString("1.2.840.10008.5.1.4.1.1.104.3"))
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
			sop==QString("1.2.840.10008.5.1.4.1.1.77.1.1.1") ||
			sop==QString("1.2.840.10008.5.1.4.1.1.77.1.4.1"))
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
			   sop==QString("1.2.840.10008.5.1.4.1.1.88.11") // Basic Text SR Storage
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
		{
			if (load_type == 0)
			{
				const bool srinfo = wsettings->get_sr_info();
				QString t00080005;
				get_string_value(
					ds, mdcm::Tag(0x0008,0x0005), t00080005);
				const QString s0 =
					SRUtils::read_sr_title1(ds, t00080005);
				if (pb) pb->hide();
				SRUtils::set_asked_for_path_once(false);
				SRWidget * sr =
					new SRWidget(wsettings->get_scale_icons());
				sr->setAttribute(Qt::WA_DeleteOnClose);
				sr->setWindowTitle(s0);
				const QString s1 =
					SRUtils::read_sr_content_sq(
					ds,
					t00080005,
					fi.absolutePath(),
					settings,
					sr->textBrowser,
					pb,
					sr->tmpfiles,
					sr->srimages,
					0,
					srinfo,
					QString("1"),
					true);
				sr->initSR(s1);
				sr->show();
				sr->raise();
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
			bs_tmp0 = bs_;
			hb_tmp0 = hb_;
			pr_tmp0 = pr_;
			localizer_tmp0 = localizer_;
			photometric_tmp0 = photometric;
			if (
				sop==QString("1.2.840.10008.5.1.4.1.1.6.1") ||
				sop==QString("1.2.840.10008.5.1.4.1.1.6")   ||
				sop==QString("1.2.840.10008.5.1.4.1.1.3.1") ||
				sop==QString("1.2.840.10008.5.1.4.1.1.3"))
			{
				ultrasound = true;
			}
			else
			{
				if (ds.FindDataElement(tSlicePosition)) // for possible multiseries
				{
					const mdcm::DataElement & sp_ = ds.GetDataElement(tSlicePosition);
					if (!sp_.IsEmpty() && !sp_.IsUndefinedLength() && sp_.GetByteValue())
					{
						bool sp_ok = false;
						const QString sp = QString::fromLatin1(
							sp_.GetByteValue()->GetPointer(),
							sp_.GetByteValue()->GetLength());
						const double spvd =
							QVariant(sp.trimmed().remove(QChar('\0'))).toDouble(&sp_ok);
						if (sp_ok)
						{
							const long spvl = 1000 * CommonUtils::set_digits(spvd,3);
							SliceInstance si;
							si.id = x;
							si.slice_position = spvl;
							si.instance_number = read_instance_number(ds);
							slice_pos_map[x] = si;
							slice_pos_list.push_back(spvl);
						}
					}
				}
				if (
					sop==QString("1.2.840.10008.5.1.4.1.1.4.1")     || // Enhanced MR
					sop==QString("1.2.840.10008.5.1.4.1.1.2.1")     || // Enhanced CT
					sop==QString("1.2.840.10008.5.1.4.1.1.130")     || // Enhanced PET
					sop==QString("1.2.840.10008.5.1.4.1.1.6.2")     || // Enhanced US Volume
					sop==QString("1.2.840.10008.5.1.4.1.1.13.1.1")  || // Enhanced X Ray 3D Angiographic
					sop==QString("1.2.840.10008.5.1.4.1.1.13.1.2")  || // Enhanced X-Ray 3D Craniofacial
					sop==QString("1.2.840.10008.5.1.4.1.1.13.1.3")  || // Breast Tomosynthesis
					sop==QString("1.2.840.10008.5.1.4.1.1.2.2")     || // Legacy Converted Enhanced CT
					sop==QString("1.2.840.10008.5.1.4.1.1.4.4")     || // Legacy Converted Enhanced MR
					sop==QString("1.2.840.10008.5.1.4.1.1.128.1")   || // Legacy Converted Enhanced PET
					sop==QString("1.2.840.10008.5.1.4.1.1.30")      || // Parametric Map
					sop==QString("1.2.840.10008.5.1.4.1.1.66.4")    || // Segmentation
					sop==QString("1.2.840.10008.5.1.4.1.1.4.3")     || // Enhanced MR Color
				    sop==QString("1.2.840.10008.5.1.4.1.1.77.1.5.4")|| // Ophthalmic Tomography
					sop==QString("1.2.840.10008.5.1.4.1.1.14.1")    || // Intravascular Optical Coherence Tomography Image Storage - For Presentation
					sop==QString("1.2.840.10008.5.1.4.1.1.14.2")    || // Intravascular Optical Coherence Tomography Image Storage - For Processing
					// ipp-iop ??
					sop==QString("1.2.840.10008.5.1.4.1.1.13.1.4")  || // Breast Projection X-Ray Image Storage - For Presentation
					sop==QString("1.2.840.10008.5.1.4.1.1.13.1.5")  || // Breast Projection X-Ray Image Storage - For Processing
				    sop==QString("1.2.840.10008.5.1.4.1.1.77.1.5.5")|| // Wide Field Ophthalmic Photography Stereographic Projection
				    sop==QString("1.2.840.10008.5.1.4.1.1.77.1.5.6")|| // Wide Field Ophthalmic Photography 3D Coordinates
				    sop==QString("1.2.840.10008.5.1.4.1.1.12.1.1")  || // Enhanced X-Ray Angiographic
				    sop==QString("1.2.840.10008.5.1.4.1.1.12.2.1")     // Enhanced X-Ray RF
					//sop==QString("1.2.840.10008.5.1.4.1.1.77.1.6")     // VL Whole Slide Microscopy
					)
				{
					enhanced = true;
				}
				if (
					sop==QString("1.2.840.10008.5.1.4.1.1.7.3") || // Multi-frame Grayscale Word SC
					sop==QString("1.2.840.10008.5.1.4.1.1.7.2") || // Multi-frame Grayscale Byte SC
					sop==QString("1.2.840.10008.5.1.4.1.1.7.4")    // Multi-frame True Color SC
					)
				{
					if (has_functional_groups(ds)) enhanced = true;
				}
				if (enhanced)
				{
					if (force_suppllut == 0)
					{
						if ((load_type == 0||load_type == 2) && has_supp_palette(ds))
						{
							if (!asked_about_supp_palette)
							{
								if (pb) pb->hide();
								QMessageBox mbox;
								mbox.setWindowModality(Qt::ApplicationModal);
								mbox.addButton(QMessageBox::Yes);
								mbox.addButton(QMessageBox::No);
								mbox.setDefaultButton(QMessageBox::Yes);
								mbox.setIcon(QMessageBox::Question);
								mbox.setText(QString(
									"Apply Supplemental Palette?"));
								qApp->processEvents();
								if (mbox.exec() == QMessageBox::Yes)
								{
									supp_palette = true;
								}
								if (pb) pb->show();
								qApp->processEvents();
								asked_about_supp_palette = true;
							}
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
					std::cout << "Warning: transfer syntax CT-private-ELE"
						<< std::endl;
				}
				if (is_multiframe(ds)) multiframe = true;
				if ((count_images > 0) && (
					rows_tmp1        != rows_tmp0      ||
					columns_tmp1     != columns_tmp0   ||
					sop_tmp1         != sop_tmp0       ||
					ba_tmp1          != ba_tmp0        ||
					bs_tmp0          != bs_tmp1        ||
					hb_tmp0          != hb_tmp1        ||
					pr_tmp1          != pr_tmp0        ||
					localizer_tmp1   != localizer_tmp0 ||
					photometric_tmp1 != photometric_tmp0))
				{
					mixed = true;
				}
			}
			images.push_back(filenames.at(x));
			count_images++;
			sop_tmp1 = sop_tmp0;
			rows_tmp1 = rows_tmp0;
			columns_tmp1 = columns_tmp0;
			ba_tmp1 = ba_tmp0;
			bs_tmp1 = bs_tmp0;
			hb_tmp1 = hb_tmp0;
			pr_tmp1 = pr_tmp0;
			photometric_tmp1 = photometric_tmp0;
			localizer_tmp1 = localizer_tmp0;
		}
#if 1
		//
		// Warning about Modality LUT Sequence
		//
		{
			if (!asked_about_modality_lut)
			{
				if (has_modality_lut_sq(ds))
				{
					if (pb) pb->hide();
					QApplication::processEvents();
					QMessageBox mbox;
					mbox.setWindowModality(Qt::ApplicationModal);
					mbox.setIcon(QMessageBox::Information);
					mbox.setText(QString(
						"Warning:\nModality LUT palette lookup\n"
						"is currently not supported.\n"
						"Adjust level/window manually, if required."));
					mbox.exec();
					if (pb) pb->show();
				}
				asked_about_modality_lut = true;
			}
		}
		//
		//
		//
#endif
	}
	//
	//
	//
	// is multiseries?
	if (
		!ultrasound &&
		!multiframe &&
		!enhanced &&
		!mixed &&
		!mosaic &&
		!uihgrid)
	{
		const size_t size0 = slice_pos_list.size();
		slice_pos_list.sort();
		slice_pos_list.unique();
		const size_t size1 = slice_pos_list.size();
		if (size0 != size1 &&
			size0 == (size_t)images.size() &&
			images.size()%size1 == 0)
		{
			multiseries = true;
		}
		slice_pos_list.clear();
		if (multiseries)
		{
			// does every image have unique instance id?
			bool unique_instance_nums = false;
			std::list<int> slices_instance_nums;
			{
				for (std::map<unsigned int,SliceInstance>::const_iterator it =
						slice_pos_map.begin();
					it != slice_pos_map.end();
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
				if (inst_size0==inst_size1 &&
					inst_size0==(size_t)images.size())
				{
					unique_instance_nums = true;
				}
				slices_instance_nums.clear();
			}
			std::map<unsigned int, unsigned int> slices_instance_map;
			if (unique_instance_nums)
			{
				for (std::map<unsigned int, SliceInstance>::const_iterator it =
						slice_pos_map.begin();
					it != slice_pos_map.end();
					++it)
				{
					slices_instance_map[it->second.instance_number]=it->second.id;
				}
				std::vector<unsigned int> image_ids;
				for (std::map<unsigned int, unsigned int>::const_iterator it =
						slices_instance_map.begin();
					it != slices_instance_map.end();
					++it)
				{
					image_ids.push_back(it->second);
				}
				// is sequential or interleaved?
				bool interleaved = false;
				for (unsigned int k = 0; k < image_ids.size(); k+=size1)
				{
					QStringList images_tmp;
					for (unsigned int j = 0; j < size1; j++)
					{
						const unsigned int id__= image_ids.at(k+j);
						if (id__<(unsigned int)images.size())
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
					for (int k = 0; k < extracted_images.size(); k++)
					{
						extracted_images[k].clear();
					}
					extracted_images.clear();
					// map: key - unique instance number, value - slice position
					std::map<unsigned int, long> slices_pos_map2;
					// list of all slice positions
					std::list<long> slices_pos_list2;
					for (std::map<unsigned int, SliceInstance>::const_iterator it =
							slice_pos_map.begin();
						it != slice_pos_map.end();
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
					unsigned int g = 0;
					// assign id for every unique slice postion
					std::map<unsigned int, long> slices_pos_ids;
					for (std::list<long>::const_iterator it =
							slices_pos_list2.begin();
						it != slices_pos_list2.end(); ++it)
					{
						slices_pos_ids[g]=*it;
						++g;
					}
					// find instance numbers for every unique slice postion
					std::vector< std::vector<unsigned int> > slices_;
					for (std::map<unsigned int, long>::const_iterator it =
						slices_pos_ids.begin();
						it != slices_pos_ids.end();
						++it)
					{
						std::vector<unsigned int> tmp_list;
						for (std::map<unsigned int, long>::const_iterator it2 =
								slices_pos_map2.begin();
							it2 != slices_pos_map2.end();
							++it2)
						{
							if (it->second == it2->second)
							{
								tmp_list.push_back(it2->first);
							}
						}
						slices_.push_back(tmp_list);
					}
					if (images.size()%unique_slice_pos_size!=0)
					{
						multiseries = false;
					}
					else
					{
						for (unsigned int j = 0; j < images.size()/unique_slice_pos_size; j++)
						{
							QStringList images_tmp;
							for (unsigned int k = 0; k < unique_slice_pos_size; k++)
							{
								if (k<slices_.size() && j<slices_.at(k).size())
								{
									const unsigned int id_0_ = slices_.at(k).at(j);
									if (slices_instance_map.count(id_0_)>0)
									{
										const unsigned int id_1_ =
											slices_instance_map[slices_.at(k).at(j)];
										if (id_1_<(unsigned int)images.size())
										{
											images_tmp.push_back(
												images.at(id_1_));
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
					for (unsigned int j = 0; j < slices_.size(); j++)
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
	if (ultrasound && (load_type == 0 || load_type == 3))
	{
		// TODO PR for ultrasound
		for (int x = 0; x < images.size(); x++)
		{
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
			QStringList images_tmp;
			images_tmp << images.at(x);
			ImageVariant * ivariant = new ImageVariant(
				CommonUtils::get_next_id(),
				false,
				true,
				NULL,
				0);
			message_ = read_ultrasound(
				&ok,
				ivariant,
				images_tmp,
				settings,
				pb);
			if (ok)
			{
				ivariant->filenames = QStringList(images_tmp);
				ivariants.push_back(ivariant);
			}
			else
			{
				delete ivariant;
			}
		}
	}
	else if (multiseries && (load_type == 0))
	{
		// TODO
		for (int k = 0; k < extracted_images.size(); k++)
		{
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
			std::vector<QString> images__;
			std::vector<QString> images_ipp;
			for (int j = 0; j < extracted_images.at(k).size(); j++)
			{
				images__.push_back(extracted_images.at(k).at(j));
			}
			if (images__.size()>1)
			{
				mdcm::Sorter2 sorter;
				sorter.SetSortFunction(sort0_);
				sorter.StableSort(images__);
				images_ipp = sorter.GetFilenames();
			}
			else if (images__.size()==1)
			{
				images_ipp.push_back(images__.at(0));
			}
			QStringList images_tmp;
			for (size_t j = 0; j < images_ipp.size(); j++)
			{
				images_tmp << images_ipp.at(j);
			}
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					gl,
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
					max_3d_tex_size,
					gl,
					ok3d,
					settings,
					pb,
					tolerance,
					true);
				if (ok)
				{
					ivariant->filenames = QStringList(images_tmp);
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
		for (int x = 0; x < images.size(); x++)
		{
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
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
						0, NULL, false,
						false,
						enh_original_frames,
						settings,
						pb,
						tolerance);
					if (!message_.isEmpty() || !ok)
					{
						for (unsigned int jjj = 0; jjj < supp_color_images.size(); jjj++)
						{
							if (supp_color_images.at(jjj))
							{
								delete supp_color_images[jjj];
								supp_color_images[jjj] = NULL;
							}
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
							max_3d_tex_size, gl, ok3d,
							false,
							enh_original_frames,
							settings,
							pb,
							tolerance,
							true);
						if (
							!message_.isEmpty() ||
							!ok ||
							(supp_color_images.size() != supp_grey_images.size()))
						{
							for (unsigned int jjj = 0; jjj < supp_color_images.size(); jjj++)
							{
								delete supp_color_images[jjj];
								supp_color_images[jjj] = NULL;
							}
							supp_color_images.clear();
							for (unsigned int jjj = 0; jjj < supp_grey_images.size(); jjj++)
							{
								delete supp_grey_images[jjj];
								supp_grey_images[jjj] = NULL;
							}
							supp_grey_images.clear();
							supp_palette_failed = true;
						}
						if (!supp_palette_failed)
						{
							// load 2 separate images for debug
#if 0
							for (unsigned int jjj = 0; jjj < supp_color_images.size(); jjj++)
							{
								ivariants.push_back(supp_color_images[jjj]);
							}
							for (unsigned int jjj = 0; jjj < supp_grey_images.size(); jjj++)
							{
								ivariants.push_back(supp_grey_images[jjj]);
							}
#else
							for (unsigned int jjj = 0; jjj < supp_color_images.size(); jjj++)
							{
								if (!(
									(supp_grey_images.at(jjj)->di->idimx == supp_color_images.at(jjj)->di->idimx) &&
									(supp_grey_images.at(jjj)->di->idimy == supp_color_images.at(jjj)->di->idimy) &&
									(supp_grey_images.at(jjj)->di->idimz == supp_color_images.at(jjj)->di->idimz)))
								{
									for (unsigned int zzz = 0; zzz < supp_color_images.size(); zzz++)
									{
										delete supp_color_images[zzz];
									}
									for (unsigned int zzz = 0; zzz < supp_grey_images.size(); zzz++)
									{
										delete supp_grey_images[zzz];
									}
									return QString("Supplemental LUT failed,\ninternal error");
								}
								CommonUtils::calculate_minmax_scalar(supp_grey_images[jjj]);
								QString supp_palette_error("");
								ImageVariant * v = new ImageVariant(
									CommonUtils::get_next_id(), ok3d, true, gl, 0);
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
									for (unsigned int zzz = 0; zzz < supp_color_images.size(); zzz++)
									{
										delete supp_color_images[zzz];
									}
									for (unsigned int zzz = 0; zzz < supp_grey_images.size(); zzz++)
									{
										delete supp_grey_images[zzz];
									}
									return QString("Supplemental LUT failed,\nwrong image type");
								}
								if (!supp_palette_error.isEmpty())
								{
									for (unsigned int zzz = 0; zzz < supp_color_images.size(); zzz++)
									{
										delete supp_color_images[zzz];
									}
									for (unsigned int zzz = 0;
										zzz < supp_grey_images.size();
										zzz++)
									{
										delete supp_grey_images[zzz];
									}
									delete v;
									return QString("Internal error,\n") + supp_palette_error;
								}
								CommonUtils::copy_slices(v,supp_grey_images.at(jjj));
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
								IconUtils::icon(v);
								v->filenames = QStringList(supp_color_images.at(jjj)->filenames);
								ivariants.push_back(v);
								delete supp_grey_images[jjj];
								supp_grey_images[jjj] = NULL;
								delete supp_color_images[jjj];
								supp_color_images[jjj] = NULL;
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
						max_3d_tex_size, gl, ok3d,
						false,
						enh_original_frames,
						settings,
						pb,
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
					0, NULL, false,
					false,
					enh_original_frames,
					settings,
					pb,
					tolerance,
					false);
			}
		}
	}
	else if (mosaic && (load_type == 0))
	{
		// TODO
		for (int x = 0; x < images.size(); x++)
		{
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
			QStringList images_tmp;
			images_tmp << images.at(x);
			ImageVariant * ivariant = new ImageVariant(
				CommonUtils::get_next_id(),
				ok3d,
				!wsettings->get_3d(),
				gl,
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
				max_3d_tex_size,
				gl,
				ok3d,
				settings,
				pb,
				tolerance,
				true);
			if (ok)
			{
				ivariant->filenames = QStringList(images_tmp);
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
		for (int x = 0; x < images.size(); x++)
		{
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
			QStringList images_tmp;
			images_tmp << images.at(x);
			ImageVariant * ivariant = new ImageVariant(
				CommonUtils::get_next_id(),
				ok3d,
				!wsettings->get_3d(),
				gl,
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
				max_3d_tex_size,
				gl,
				ok3d,
				settings,
				pb,
				tolerance,
				true);
			if (ok)
			{
				ivariant->filenames = QStringList(images_tmp);
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
		for (int x = 0; x < images.size(); x++)
		{
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
			QStringList images_tmp;
			images_tmp << images.at(x);
			if (load_type == 0||load_type == 2)
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					gl,
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
					max_3d_tex_size,
					gl,
					ok3d,
					settings,
					pb,
					tolerance,
					true);
				if (ok)
				{
					QString sop_instance_uid("");
					read_sop_instance_uid(
						images.at(x),
						sop_instance_uid);
					for (int z = 0; z < ivariant->di->idimz; z++)
					{
						ivariant->image_instance_uids[z] = sop_instance_uid;
					}
					ivariant->filenames = QStringList(images_tmp);
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
					NULL,
					0);
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					false,
					ivariant,
					images_tmp,
					0,
					NULL,
					false,
					settings,
					pb,
					tolerance,
					false);
				if (ok)
				{
					QString sop_instance_uid("");
					read_sop_instance_uid(
						images.at(x),
						sop_instance_uid);
					for (int z = 0; z < ivariant->di->idimz; z++)
					{
						ivariant->image_instance_uids[z] = sop_instance_uid;
					}
					ivariant->filenames = QStringList(images_tmp);
					ivariants.push_back(ivariant);
				}
				else
				{
					delete ivariant;
				}
			}
		}
	}
	else if (mixed && (load_type == 0||load_type == 2))
	{
		// TODO
		const mdcm::Tag tt(0x0008,0x0008);
		const mdcm::Tag tr(0x0028,0x0010);
		const mdcm::Tag tc(0x0028,0x0011);
		const mdcm::Tag ta(0x0028,0x0100);
		std::vector<MixedDicomSeriesInfo> msi;
		for (int x = 0; x < images.size(); x++)
		{
			MixedDicomSeriesInfo si;
			si.rows      = -1;
			si.columns   = -1;
			si.allocated =  0;
			si.localizer =  false;
			si.file      = QString(images.at(x));
			mdcm::Reader reader;
			reader.SetFileName(
				filenames.at(x).toLocal8Bit().constData());
			if (!reader.ReadUpToTag(mdcm::Tag(0x0028,0x0101))) continue;
			const mdcm::File    & file = reader.GetFile();
			const mdcm::DataSet & ds   = file.GetDataSet();
			if (ds.IsEmpty()) continue;
			unsigned short r = 0, c = 0, a = 0;
			if (get_us_value(ds,tr,&r))
			{
				si.rows = (int)r;
			}
			if (get_us_value(ds,tc,&c))
			{
				si.columns = (int)c;
			}
			if (get_us_value(ds,ta,&a))
			{
				si.allocated = a;
			}
			QString s;
			if (get_string_value(ds,tt,s))
			{
				if (s.contains(QString("LOCALIZER")))
					si.localizer = true;
			}
			msi.push_back(si);
		}
		QMultiMap<QString, QString> l0;
		for (size_t x = 0; x < msi.size(); x++)
		{
			const MixedDicomSeriesInfo & i = msi.at(x);
			if (i.rows != -1 && i.columns != -1 && i.allocated > 0)
			{
				const QString k1 =
					QString::number(i.rows) +
					QString("x") +
					QString::number(i.columns) +
					QString("x") +
					QString::number((int)i.allocated) +
					(i.localizer ? QString("L") : QString(""));
				l0.insert(k1, i.file);
			}
		}
		QList<QStringList> fff;
		const QList<QString> & l1 = l0.keys();
		const QSet<QString> s1 = l1.toSet();
		QSet<QString>::const_iterator it1 = s1.begin();
		while (it1 != s1.end())
		{
			QStringList ff;
			const QList<QString> & q = l0.values(*it1);
			for (int y = 0; y < q.size(); y++)
			{
				ff.push_back(q.at(y));
			}
			fff.push_back(ff);
			++it1;
		}
		for (int x = 0; x < fff.size(); x++)
		{
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
			std::vector<QString> images__;
			std::vector<QString> images_ipp;
			for (int k = 0; k < fff.at(x).size(); k++)
			{
				images__.push_back(fff.at(x).at(k));
			}
			if (images__.size()>1)
			{
				mdcm::Sorter2 sorter;
				sorter.SetSortFunction(sort0_);
				sorter.StableSort(images__);
				images_ipp = sorter.GetFilenames();
			}
			else if (images__.size()==1)
			{
				images_ipp.push_back(images__.at(0));
			}
			QStringList images_tmp;
			for (size_t j = 0; j < images_ipp.size(); j++)
			{
				images_tmp << images_ipp.at(j);
			}
			if (load_type == 0||load_type == 2)
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					gl,
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
					max_3d_tex_size,
					gl,
					ok3d,
					settings,
					pb,
					tolerance,
					true);
				if (ok)
				{
					ivariant->filenames = QStringList(images_tmp);
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
					NULL,
					0);
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					elscint,
					ivariant,
					images_tmp,
					0,
					NULL,
					false,
					settings,
					pb,
					tolerance,
					false);
				if (ok)
				{
					ivariant->filenames = QStringList(images_tmp);
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
			if (pb) pb->setValue(-1);
			QApplication::processEvents();
			std::vector<QString> images__;
			std::vector<QString> images_ipp;
			for (int k = 0; k < images.size(); k++)
			{
				images__.push_back(images.at(k));
			}
			if (images__.size()>1)
			{
				mdcm::Sorter2 sorter;
				sorter.SetSortFunction(sort0_);
				sorter.StableSort(images__);
				images_ipp = sorter.GetFilenames();
			}
			else if (images__.size()==1)
			{
				images_ipp.push_back(images__.at(0));
			}
			QStringList images_tmp;
			for (size_t j = 0; j < images_ipp.size(); j++)
			{
				images_tmp << images_ipp.at(j);
			}
			if (load_type == 0||load_type == 2)
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(),
					ok3d,
					!wsettings->get_3d(),
					gl,
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
					max_3d_tex_size,
					gl,
					ok3d,
					settings,
					pb,
					tolerance,
					true);
				if (ok)
				{
					ivariant->filenames = QStringList(images_tmp);
					{
						QList<QString> l_uids =
							ivariant->image_instance_uids.values();
						if (!l_uids.empty())
						{
							const size_t l_size = l_uids.size();
							if (l_size > 1)
							{
								QSet<QString> s_uids = l_uids.toSet();
								const size_t s_size = s_uids.size();
								if (l_size > s_size) count_uid_errors++;
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
					NULL,
					0);
				message_ = read_series(
					&ok,
					false,
					false,
					false,
					elscint,
					ivariant,
					images_tmp,
					0,
					NULL,
					false,
					settings,
					pb,
					tolerance,
					false);
				if (ok)
				{
					ivariant->filenames = QStringList(images_tmp);
					{
						QList<QString> l_uids =
							ivariant->image_instance_uids.values();
						if (!l_uids.empty())
						{
							const size_t l_size = l_uids.size();
							if (l_size > 1)
							{
								QSet<QString> s_uids = l_uids.toSet();
								const size_t s_size = s_uids.size();
								if (l_size > s_size) count_uid_errors++;
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
		}
	}
	if (count_uid_errors > 0)
	{
		if (!message_.isEmpty()) message_.append(QString("\n"));
		message_.append(
			QString("Warning: UIDs are not unique (") +
			QVariant(count_uid_errors).toString() +
			QString(")"));
	}
	//
	//
	if (load_type != 0) return message_;
	//
	for (size_t x = 0; x < rtstructs.size(); x++)
		ivariants.push_back(rtstructs[x]);
	for (size_t x = 0; x < spectroscopy_images.size(); x++)
		ivariants.push_back(spectroscopy_images[x]);
	for (size_t x = 0; x < meshes.size(); x++)
		ivariants.push_back(meshes[x]);
	//
	if (!rtstruct_ref_search.empty())
	{
		for (long x = 0; x < rtstruct_ref_search.size(); x++)
		{
			std::vector<ImageVariant*> tmp_ivariants_rtstruct;
			const bool ref_ok =
				process_contrours_ref(
					rtstruct_ref_search.at(x),
					rtstruct_ref_search_path,
					tmp_ivariants_rtstruct,
					max_3d_tex_size, gl, ok3d,
					settings,
					pb);
			bool ref2_ok = false;
			if (ref_ok)
			{
				for (unsigned int y = 0;
					y < tmp_ivariants_rtstruct.size();
					y++)
				{
					tmp_ivariants_rtstruct[y]->filenames =
						QStringList(
							QDir::toNativeSeparators(
								rtstruct_ref_search.at(x)));
					ivariants.push_back(tmp_ivariants_rtstruct[y]);
				}
			}
			else
			{
#ifndef USE_WORKSTATION_MODE
				const QString tmpp = QString(
#ifdef _WIN32
					"DICOM/"
#else
					"../DICOM/"
#endif
				);
				ref2_ok = process_contrours_ref(
					rtstruct_ref_search.at(x),
					tmpp,
					tmp_ivariants_rtstruct,
					max_3d_tex_size, gl, ok3d,
					settings,
					pb);
				if (ref2_ok)
				{
					for (unsigned int y = 0;
						y < tmp_ivariants_rtstruct.size();
						y++)
					{
						tmp_ivariants_rtstruct[y]->filenames =
							QStringList(
								QDir::toNativeSeparators(
									rtstruct_ref_search.at(x)));
						ivariants.push_back(tmp_ivariants_rtstruct[y]);
					}
				}
				if (!ref2_ok)
#endif
				{
					bool ok22 = false;
					const QString s(
						"<html><head/><body><p align=\"center\">"
						"<p>Select directory to start recursive "
						"search.</body></html>");
					if (pb) pb->hide();
					QFileInfo reffi(filenames.at(x));
					FindRefDialog * d =
						new FindRefDialog(wsettings->get_scale_icons());
					d->set_text(s);
					d->set_path(QDir::toNativeSeparators(reffi.absolutePath()));
					if (d->exec() == QDialog::Accepted)
					{
						rtstruct_ref_search_path = d->get_path();
						ok22 = true;
					}
					delete d;
					if (pb) pb->show();
					QApplication::processEvents();
					if (ok22)
					{
						ref2_ok = process_contrours_ref(
							rtstruct_ref_search.at(x),
							rtstruct_ref_search_path,
							tmp_ivariants_rtstruct,
							max_3d_tex_size, gl, ok3d,
							settings,
							pb);
						if (ref2_ok)
						{
							for (unsigned int y = 0;
								y < tmp_ivariants_rtstruct.size();
								y++)
							{
								tmp_ivariants_rtstruct[y]->filenames =
									QStringList(
										QDir::toNativeSeparators(
											rtstruct_ref_search.at(x)));
								ivariants.push_back(tmp_ivariants_rtstruct[y]);
							}
						}
					}
				}
			}
			if (!(ref_ok || ref2_ok))
			{
				mdcm::Reader reader;
				reader.SetFileName(
					rtstruct_ref_search.at(x).
						toLocal8Bit().constData());
				const bool str_f_ok = reader.Read();
				if (str_f_ok)
				{
					ImageVariant * ivariant = new ImageVariant(
						CommonUtils::get_next_id(),
						ok3d, !wsettings->get_3d(), gl, 0);
					ivariant->filenames = QStringList(
						QDir::toNativeSeparators(
							rtstruct_ref_search.at(x)));
					const mdcm::File & file = reader.GetFile();
					const mdcm::DataSet & ds = file.GetDataSet();
					load_contour(ds,ivariant);
					ContourUtils::calculate_rois_center(ivariant);
					ivariants.push_back(ivariant);
				}
			}
		}
		if (pb) pb->setValue(-1);
		QApplication::processEvents();
	}
	//
	// init contours
	for (unsigned int x = 0; x < ivariants.size(); x++)
	{
		ImageVariant * v = ivariants[x];
		if (!v->di->rois.empty())
		{
			ContourUtils::calculate_contours_uv(v);
			if (ok3d && gl)
			{
				long long contours_count = 0;
				for (int j = 0; j < v->di->rois.size(); j++)
				{
					Contours::const_iterator ic =
						v->di->rois.at(j).contours.constBegin();
					while (ic != v->di->rois.at(j).contours.constEnd())
					{
						const Contour * c = ic.value();
						if (c) contours_count++;
						++ic;
					}
				}
				for (int z = 0; z < v->di->rois.size(); z++)
				{
					ContourUtils::generate_roi_vbos(gl, v->di->rois[z],false);
				}
			}
			ContourUtils::map_contours_all(v);
			ContourUtils::contours_build_path_all(v);
		}
	}
	//
	if (!pdf_files.empty())
	{
		if (pb) pb->hide();
		for (int x = 0; x < pdf_files.size(); x++)
		{
			const QString pdff = QFileDialog::getSaveFileName(
				NULL,
				QString("Select file"),
				QDir::toNativeSeparators(
					CommonUtils::get_save_dir() +
					QDir::separator() +
					CommonUtils::get_save_name() +
					QString(".pdf")),
				QString("All Files (*)"),
				(QString*)NULL
				 //,QFileDialog::DontUseNativeDialog
				);
			if (!pdff.isEmpty())
			{
				QFileInfo fi(pdff);
				CommonUtils::set_save_dir(
					QDir::toNativeSeparators(fi.absolutePath()));
				write_encapsulated(
					QDir::toNativeSeparators(pdf_files.at(x)),
					QDir::toNativeSeparators(pdff));
			}
		}
		if (pb) { pb->show(); pb->setValue(-1); }
		QApplication::processEvents();
	}
	//
	if (!stl_files.empty())
	{
		if (pb) pb->hide();
		for (int x = 0; x < stl_files.size(); x++)
		{
			const QString stlf = QFileDialog::getSaveFileName(
				NULL,
				QString("Select file"),
				QDir::toNativeSeparators(
					CommonUtils::get_save_dir() +
					QDir::separator() +
					CommonUtils::get_save_name() +
					QString(".stl")),
				QString("All Files (*)"),
				(QString*)NULL
				 //,QFileDialog::DontUseNativeDialog
				);
			if (!stlf.isEmpty())
			{
				QFileInfo fi(stlf);
				CommonUtils::set_save_dir(
					QDir::toNativeSeparators(fi.absolutePath()));
				write_encapsulated(
					QDir::toNativeSeparators(stl_files.at(x)),
					QDir::toNativeSeparators(stlf));
			}
		}
		if (pb) { pb->show(); pb->setValue(-1); }
		QApplication::processEvents();
	}
	//
	if (!video_files.empty())
	{
		if (pb) pb->hide();
		for (int x = 0; x < video_files.size(); x++)
		{
			if (pb) pb->hide();
			const QString tmp943 =
				QDir::toNativeSeparators(video_files.at(x));
			const QString suf = suffix_mpeg(tmp943);
			const QString video_file_name =
				QFileDialog::getSaveFileName(
					NULL,
					QString("Select file"),
					QDir::toNativeSeparators(
						CommonUtils::get_save_dir() +
						QDir::separator() +
						CommonUtils::get_save_name() +
						suf),
					QString("All Files (*)"),
					(QString*)NULL
					 //,QFileDialog::DontUseNativeDialog
					);
			if (!video_file_name.isEmpty())
			{
				QFileInfo fi(video_file_name);
				CommonUtils::set_save_dir(
					QDir::toNativeSeparators(fi.absolutePath()));
				write_mpeg(
					tmp943,
					QDir::toNativeSeparators(video_file_name));
			}
		}
		if (pb) { pb->show(); pb->setValue(-1); }
		QApplication::processEvents();
	}
	//
	if (!grey_softcopy_pr_files.empty())
	{
		if (pb) pb->show();
		unsigned int count = 0;
		const QString file0 = grey_softcopy_pr_files.at(0);
		QFileInfo p0(file0);
#ifdef USE_WORKSTATION_MODE
		QString p = QDir::toNativeSeparators(p0.absolutePath());
#else
		QString p = QString(
#ifdef _WIN32
			"DICOM/"
#else
			"../DICOM/"
#endif
			);
#endif
		for (int x = 0; x < grey_softcopy_pr_files.size(); x++)
		{
			if (pb)
			{
				pb->setLabelText(
					QString("Searching referenced files"));
				pb->setValue(-1);
			}
			QApplication::processEvents();
			QList<PrRefSeries> refs;
			read_pr_ref(p, grey_softcopy_pr_files.at(x), refs, pb);
			QApplication::processEvents();
			for (int y = 0; y < refs.size(); y++)
			{
				if (pb)
				{
					pb->setLabelText(QString("Loading ... "));
					pb->setValue(-1);
				}
				QApplication::processEvents();
				QStringList ref_files;
				for (int z = 0; z < refs.at(y).images.size(); z++)
					ref_files.push_back(refs.at(y).images.at(z).file);
				if (ref_files.size() < 1)
					continue;
				ref_files.sort();
				std::vector<ImageVariant*> ref_ivariants;
				const QString message_pr_ref =
					read_dicom(
						ref_ivariants,
						ref_files,
						max_3d_tex_size,
						gl,
						mesh_shader,
						ok3d,
						settings,
						pb,
						1);
				for (unsigned int z = 0; z < ref_ivariants.size(); z++)
				{
					if (pb) pb->setValue(-1);
					QApplication::processEvents();
					count++;
					bool spatial_transform = false;
					ImageVariant * pr_image =
						PrConfigUtils::make_pr_monochrome(
							ref_ivariants.at(z), 
							refs.at(y),
							wsettings,
							gl,
							ok3d,
							&spatial_transform);
					if (pr_image)
					{
						pr_image->filenames = QStringList(
							QDir::toNativeSeparators(
								grey_softcopy_pr_files.at(x)));
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
						pr_image->modality = QString("OT");
						pr_image->iod = QString(
							"Grayscale Softcopy Presentation State");
						pr_image->sop = QString("");
						pr_image->di->skip_texture = !wsettings->get_3d();
						pr_image->rescale_disabled = false;
						if (spatial_transform)
						{
							pr_image->equi = false;
							pr_image->di->hide_orientation = true;
							pr_image->di->filtering = 0;
						}
						bool pr_load_ok = false;
						if (pb) { pb->setValue(-1); QApplication::processEvents(); }
						if (
							pr_image->image_type >=  0 &&
							pr_image->image_type <  10)
						{
							pr_load_ok = CommonUtils::reload_monochrome(
								pr_image,
								ok3d,
								gl,
								max_3d_tex_size,
								wsettings->get_resize(),
								wsettings->get_size_x(),
								wsettings->get_size_y());
						}
						else if (pr_image->image_type == 14)
						{
							pr_load_ok = CommonUtils::reload_rgb_rgba(
								pr_image);
						}
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
							IconUtils::icon(pr_image);
							ivariants.push_back(pr_image);
						}
						else
						{
							delete pr_image;
							pr_image = NULL;
						}
					}
					if (ref_ivariants.at(z))
					{
						delete ref_ivariants[z];
						ref_ivariants[z] = NULL;
					}
				}
				if (!message_pr_ref.isEmpty())
					message_.append(QString("\n") + message_pr_ref);
			}
		}
		if (count < 1 && message_.isEmpty())
		{
#ifdef USE_WORKSTATION_MODE
			QFileInfo fi99(QDir::toNativeSeparators(p));
			p = QDir::toNativeSeparators(fi99.absolutePath());
#else
			p = QString();
#endif
			const QString s(
				"<html><head/><body>"
				"<span style=\"font-style:italic;\">"
				"Grayscale Softcopy Presentation</span>"
				"<p>"
				"Select directory for recursive scan"
				"</p>"
				"</body></html>");
			if (pb) pb->hide();
			QApplication::processEvents();
			FindRefDialog * d =
				new FindRefDialog(wsettings->get_scale_icons());
			d->set_text(s);
			d->set_path(p);
			bool _ok = false;
			if (d->exec() == QDialog::Accepted)
			{
				_ok = true;
				p = d->get_path();
			}
			delete d;
			if (pb) pb->show();
			if (!_ok)
			{
				return QString("");
			}
			for (int x = 0; x < grey_softcopy_pr_files.size(); x++)
			{
				if (pb)
				{
					pb->setLabelText(QString("Searching referenced files"));
					pb->setValue(-1);
				}
				QApplication::processEvents();
				QList<PrRefSeries> refs;
				read_pr_ref(p, grey_softcopy_pr_files.at(x), refs, pb);
				QApplication::processEvents();
				for (int y = 0; y < refs.size(); y++)
				{
					if (pb)
					{
						pb->setLabelText(QString("Loading ... "));
						pb->setValue(-1);
					}
					QApplication::processEvents();
					QStringList ref_files;
					for (int z = 0; z < refs.at(y).images.size(); z++)
						ref_files.push_back(refs.at(y).images.at(z).file);
					if (ref_files.size() < 1) continue;
					ref_files.sort();
					std::vector<ImageVariant*> ref_ivariants;
					const QString message_pr_ref =
						read_dicom(
							ref_ivariants,
							ref_files,
							max_3d_tex_size,
							gl,
							mesh_shader,
							ok3d,
							settings,
							pb,
							1);
					for (unsigned int z = 0; z < ref_ivariants.size(); z++)
					{
						if (pb) pb->setValue(-1);
						QApplication::processEvents();
						count++;
						bool spatial_transform = false;
						ImageVariant * pr_image =
							PrConfigUtils::make_pr_monochrome(
								ref_ivariants.at(z), 
								refs.at(y),
								wsettings,
								gl,
								ok3d,
								&spatial_transform);
						if (pr_image)
						{
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
							pr_image->modality = QString("OT");
							pr_image->iod = QString(
								"Grayscale Softcopy Presentation State");
							pr_image->sop = QString("");
							pr_image->di->skip_texture = !wsettings->get_3d();
							pr_image->rescale_disabled = false;
							if (spatial_transform)
							{
								pr_image->equi = false;
								pr_image->di->hide_orientation = true;
								pr_image->di->filtering = 0;
							}
							bool pr_load_ok = false;
							if (pb) { pb->setValue(-1); QApplication::processEvents(); }
							if (
								pr_image->image_type >=  0 &&
								pr_image->image_type <  10)
							{
								pr_load_ok = CommonUtils::reload_monochrome(
									pr_image,
									ok3d,
									gl,
									max_3d_tex_size,
									wsettings->get_resize(),
									wsettings->get_size_x(),
									wsettings->get_size_y());
							}
							else if (pr_image->image_type == 14)
							{
								pr_load_ok = CommonUtils::reload_rgb_rgba(
									pr_image);
							}
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
								IconUtils::icon(pr_image);
								ivariants.push_back(pr_image);
							}
							else
							{
								delete pr_image;
								pr_image = NULL;
							}
						}
						if (ref_ivariants.at(z))
						{
							delete ref_ivariants[z];
							ref_ivariants[z] = NULL;
						}
					}
					if (!message_pr_ref.isEmpty())
						message_.append(QString("\n") + message_pr_ref);
				}
			}
		}
	}
	if (
		!color_softcopy_pr_files.empty() ||
		!pseudo_color_softcopy_pr_files.empty() ||
		!blending_softcopy_pr_files.empty() ||
		!xaxrf_softcopy_pr_files.empty() ||
		!advanced_blending_softcopy_pr_files.empty())
	{
		QString pr_warn("");
		if (!color_softcopy_pr_files.empty())
			pr_warn += QString("Color Softcopy Presentation\n");
		if (!pseudo_color_softcopy_pr_files.empty())
			pr_warn += QString("Pseudo-color Softcopy Presentation\n");
		if (!blending_softcopy_pr_files.empty())
			pr_warn += QString("Blending Softcopy Presentation\n");
		if (!xaxrf_softcopy_pr_files.empty())
			pr_warn += QString("XA/XRF Grayscale Softcopy Presentation\n");
		if (!advanced_blending_softcopy_pr_files.empty())
			pr_warn += QString("Advanced Blending Softcopy Presentation\n");
		pr_warn += QString("is currently not supported");
		QMessageBox mbox;
		mbox.setWindowModality(Qt::ApplicationModal);
		mbox.addButton(QMessageBox::Close);
		mbox.setIcon(QMessageBox::Warning);
		mbox.setText(pr_warn);
		mbox.exec();
	}
	//
	return message_;
}

#ifdef ENHANCED_PRINT_INFO
#undef ENHANCED_PRINT_INFO
#endif


