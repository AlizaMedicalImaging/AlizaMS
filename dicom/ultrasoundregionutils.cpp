#include <mdcmTag.h>
#include <mdcmItem.h>
#include <mdcmElement.h>
#include <mdcmDataElement.h>
#include <mdcmVR.h>
#include <mdcmVM.h>
#include <mdcmSequenceOfItems.h>
#include <iostream>
#include <cmath>
#include "ultrasoundregionutils.h"
#include "commonutils.h"
#include "dicomutils.h"

void UltrasoundRegionUtils::GetBitAlignedPositions(const mdcm::DataSet&, UltrasoundRegionData&)
{
	// TODO
}

void UltrasoundRegionUtils::GetRanges(const mdcm::DataSet&, UltrasoundRegionData&)
{
	// TODO
}

void UltrasoundRegionUtils::GetTableLookUp(const mdcm::DataSet&, UltrasoundRegionData&)
{
	// TODO
}

void UltrasoundRegionUtils::GetCodeSequenceLookUp(const mdcm::DataSet&, UltrasoundRegionData&)
{
	// TODO
}

void UltrasoundRegionUtils::Read(const mdcm::DataSet & ds, QList<UltrasoundRegionData> & regions)
{
	if (ds.IsEmpty()) return;

	const mdcm::Tag tSequenceOfUltrasoundRegions(0x0018,0x6011);

	if (!ds.FindDataElement(tSequenceOfUltrasoundRegions)) return;

	const mdcm::DataElement & deSequenceOfUltrasoundRegions	=
		ds.GetDataElement(tSequenceOfUltrasoundRegions);
	mdcm::SmartPointer<mdcm::SequenceOfItems> sqSequenceOfUltrasoundRegions =
		deSequenceOfUltrasoundRegions.GetValueAsSQ();
	if (!sqSequenceOfUltrasoundRegions) return;

	const mdcm::Tag tRegionSpatialFormat(0x0018,0x6012);
	const mdcm::Tag tRegionDataType(0x0018,0x6014);
	const mdcm::Tag tRegionFlags(0x018,0x6016);
	const mdcm::Tag tRegionLocationMinX0(0x0018,0x6018);
	const mdcm::Tag tRegionLocationMinY0(0x0018,0x601a);
	const mdcm::Tag tRegionLocationMaxX1(0x0018,0x601c);
	const mdcm::Tag tRegionLocationMaxY1(0x0018,0x601e);
	const mdcm::Tag tReferencePixelX0(0x018,0x6020);
	const mdcm::Tag tReferencePixelY0(0x018,0x6022);
	const mdcm::Tag tPhysicalUnitsXDirection(0x0018,0x6024);
	const mdcm::Tag tPhysicalUnitsYDirection(0x0018,0x6026);
	const mdcm::Tag tReferencePixelPhysicalValueX(0x0018,0x6028);
	const mdcm::Tag tReferencePixelPhysicalValueY(0x0018,0x602a);
	const mdcm::Tag tPhysicalDeltaX(0x0018,0x602c);
	const mdcm::Tag tPhysicalDeltaY(0x0018,0x602e);
	const mdcm::Tag tPixelComponentOrganization(0x0018, 0x6044);
	const mdcm::Tag tDopplerSampleVolumeXPosition(0x0018,0x6039);
	const mdcm::Tag tDopplerSampleVolumeYPosition(0x0018,0x603b);
	const mdcm::Tag tTMLinePositionX0(0x0018,0x603d);
	const mdcm::Tag tTMLinePositionY0(0x0018,0x603f);
	const mdcm::Tag tTMLinePositionX1(0x0018,0x6041);
	const mdcm::Tag tTMLinePositionY1(0x0018,0x6043);

	for (unsigned int x = 0; x < sqSequenceOfUltrasoundRegions->GetNumberOfItems(); x++)
	{
		const mdcm::Item & item = sqSequenceOfUltrasoundRegions->GetItem(x+1);
		const mdcm::DataSet & nestedds = item.GetNestedDataSet();
		if (nestedds.IsEmpty())
		{
			continue;
		}

		unsigned int RegionLocationMinX0, RegionLocationMinY0;
		unsigned int RegionLocationMaxX1, RegionLocationMaxY1;
		if (!DicomUtils::get_ul_value(nestedds,tRegionLocationMinX0,&RegionLocationMinX0) ||
			!DicomUtils::get_ul_value(nestedds,tRegionLocationMinY0,&RegionLocationMinY0) ||
			!DicomUtils::get_ul_value(nestedds,tRegionLocationMaxX1,&RegionLocationMaxX1) ||
			!DicomUtils::get_ul_value(nestedds,tRegionLocationMaxY1,&RegionLocationMaxY1))
		{
			continue;
		}

		UltrasoundRegionData r;

		r.m_X0 = RegionLocationMinX0;
		r.m_Y0 = RegionLocationMinY0;
		r.m_X1 = RegionLocationMaxX1;
		r.m_Y1 = RegionLocationMaxY1;

		int ReferencePixelX0, ReferencePixelY0;
		if (DicomUtils::get_sl_value(nestedds,tReferencePixelX0,&ReferencePixelX0) &&
			DicomUtils::get_sl_value(nestedds,tReferencePixelY0,&ReferencePixelY0))
		{
			r.m_ReferencePixelBool = true;
			r.m_ReferencePixelX0 = ReferencePixelX0;
			r.m_ReferencePixelY0 = ReferencePixelY0;
		}

		double PhysicalDeltaX, PhysicalDeltaY;
		if (DicomUtils::get_fd_value(nestedds,tPhysicalDeltaX,&PhysicalDeltaX))
		{
			r.m_PhysicalDeltaX = PhysicalDeltaX;
		}
		if (DicomUtils::get_fd_value(nestedds,tPhysicalDeltaY,&PhysicalDeltaY))
		{
			r.m_PhysicalDeltaY = PhysicalDeltaY;
		}

		unsigned short PhysicalUnitsXDirection;
		if (DicomUtils::get_us_value(nestedds,tPhysicalUnitsXDirection, &PhysicalUnitsXDirection))
		{
			r.m_PhysicalUnitsXDirection = PhysicalUnitsXDirection;
			switch(PhysicalUnitsXDirection)
			{
			case 0x1:
				r.m_UnitXString = QString("%");
				break;
			case 0x2:
				r.m_UnitXString = QString("dB");
				break;
			case 0x3:
				r.m_UnitXString = QString("cm");
				break;
			case 0x4:
				r.m_UnitXString = QString("s");
				break;
			case 0x5:
				r.m_UnitXString = QString("Hz");
				break;
			case 0x6:
				r.m_UnitXString = QString("dB/s");
				break;
			case 0x7:
				r.m_UnitXString = QString("cm/s");
				break;
			case 0x8:
				r.m_UnitXString = QString("cm") + QString(QChar(0x00B2));
				break;
			case 0x9:
				r.m_UnitXString = QString("cm") + QString(QChar(0x00B2)) + QString("/s");
				break;
			case 0xa:
				r.m_UnitXString = QString("cm") + QString(QChar(0x00B3));
				break;
			case 0xb:
				r.m_UnitXString = QString("cm") + QString(QChar(0x00B3)) + QString("/s");
				break;
			case 0xc:
				r.m_UnitXString = QString(QChar(0x00B0));
				break;
			default:
				break;
			}
		}

		unsigned short PhysicalUnitsYDirection;
		if (DicomUtils::get_us_value(nestedds,tPhysicalUnitsYDirection, &PhysicalUnitsYDirection))
		{
			r.m_PhysicalUnitsYDirection = PhysicalUnitsYDirection;
			switch(PhysicalUnitsYDirection)
			{
			case 0x1:
				r.m_UnitYString = QString("%");
				break;
			case 0x2:
				r.m_UnitYString = QString("dB");
				break;
			case 0x3:
				r.m_UnitYString = QString("cm");
				break;
			case 0x4:
				r.m_UnitYString = QString("s");
				break;
			case 0x5:
				r.m_UnitYString = QString("Hz");
				break;
			case 0x6:
				r.m_UnitYString = QString("dB/s");
				break;
			case 0x7:
				r.m_UnitYString = QString("cm/s");
				break;
			case 0x8:
				r.m_UnitYString = QString("cm") + QString(QChar(0x00B2));
				break;
			case 0x9:
				r.m_UnitYString = QString("cm") + QString(QChar(0x00B2)) + QString("/s");
				break;
			case 0xa:
				r.m_UnitYString = QString("cm") + QString(QChar(0x00B3));
				break;
			case 0xb:
				r.m_UnitYString = QString("cm") + QString(QChar(0x00B3)) + QString("/s");
				break;
			case 0xc:
				r.m_UnitYString = QString(QChar(0x00B0));
				break;
			default:
				break;
			}
		}

		double ReferencePixelPhysicalValueX, ReferencePixelPhysicalValueY;
		if (
			DicomUtils::get_fd_value(nestedds,tReferencePixelPhysicalValueX,&ReferencePixelPhysicalValueX) &&
			DicomUtils::get_fd_value(nestedds,tReferencePixelPhysicalValueY,&ReferencePixelPhysicalValueY))
		{
			r.m_ReferencePixelPhysicalValueBool = true;
			r.m_ReferencePixelPhysicalValueX = ReferencePixelPhysicalValueX;
			r.m_ReferencePixelPhysicalValueY = ReferencePixelPhysicalValueY;
		}

		unsigned int RegionFlags;
		if (DicomUtils::get_ul_value(nestedds,tRegionFlags,&RegionFlags))
		{
			r.m_FlagsBool = true;
			r.m_RegionFlags = RegionFlags;
		}

		unsigned short RegionSpatialFormat;
		if (DicomUtils::get_us_value(nestedds,tRegionSpatialFormat, &RegionSpatialFormat))
		{
			r.m_RegionSpatialFormat = RegionSpatialFormat;
			switch(RegionSpatialFormat)
			{
			case 0x1:
				r.m_SpatialFormatString = QString("2D");
				break;
			case 0x2:
				r.m_SpatialFormatString = QString("M-Mode");
				break;
			case 0x3:
				r.m_SpatialFormatString = QString("Spectral");
				break;
			case 0x4:
				r.m_SpatialFormatString = QString("Waveform");
				break;
			case 0x5:
				r.m_SpatialFormatString = QString("Graphics");
				break;
			default:
				break;
			}
		}

		unsigned short RegionDataType;
		if (DicomUtils::get_us_value(nestedds,tRegionDataType, &RegionDataType))
		{
			r.m_RegionDataType = RegionDataType;
			switch(RegionDataType)
			{
			case 0x1:
				r.m_DataTypeString = QString("Tissue");
				break;
			case 0x2:
				r.m_DataTypeString = QString("Color Flow");
				break;
			case 0x3:
				r.m_DataTypeString = QString("PW Spectral Doppler");
				break;
			case 0x4:
				r.m_DataTypeString = QString("CW Spectral Doppler");
				break;
			case 0x5:
				r.m_DataTypeString = QString("Doppler Mean Trace");
				break;
			case 0x6:
				r.m_DataTypeString = QString("Doppler Mode Trace");
				break;
			case 0x7:
				r.m_DataTypeString = QString("Doppler Max Trace");
				break;
			case 0x8:
				r.m_DataTypeString = QString("Volume Trace");
				break;
			case 0x9:
				r.m_DataTypeString = QString("");
				break;
			case 0xa:
				r.m_DataTypeString = QString("ECG Trace");
				break;
			case 0xb:
				r.m_DataTypeString = QString("Pulse Trace");
				break;
			case 0xc:
				r.m_DataTypeString = QString("Phonocardiogram Trace");
				break;
			case 0xd:
				r.m_DataTypeString = QString("Gray bar");
				break;
			case 0xe:
				r.m_DataTypeString = QString("Color bar");
				break;
			case 0xf:
				r.m_DataTypeString = QString("Integrated Backscatter");
				break;
			case 0x10:
				r.m_DataTypeString = QString("Area Trace");
				break;
			case 0x11:
				r.m_DataTypeString = QString("d(area)/dt");
				break;
			case 0x12: // "Other"
			default:
				break;
			}
		}

		unsigned short PixelComponentOrganization;
		if (DicomUtils::get_us_value(nestedds,tPixelComponentOrganization,&PixelComponentOrganization))
		{
			r.m_PixelComponentOrganizationBool = true;
			r.m_PixelComponentOrganization = PixelComponentOrganization;
			// http://dicom.nema.org/medical/dicom/current/output/html/part03.html#sect_C.8.5.5.1.4
			switch (PixelComponentOrganization)
			{
			case 0:
				{
					r.m_PixelComponentOrganizationString = QString("Bit aligned positions");
					GetBitAlignedPositions(nestedds, r);
				}
				break;
			case 1:
				{
					r.m_PixelComponentOrganizationString = QString("Ranges");
					GetRanges(nestedds, r);
				}
				break;
			case 2:
				{
					r.m_PixelComponentOrganizationString = QString("Table look up");
					GetTableLookUp(nestedds, r);
				}
				break;
			case 3:
				{
					r.m_PixelComponentOrganizationString = QString("Code Sequence look up");
					GetCodeSequenceLookUp(nestedds, r);
				}
				break;
			default:
				break;
			}
		}

		int TMLinePositionX0, TMLinePositionY0;
		int TMLinePositionX1, TMLinePositionY1;
		if (DicomUtils::get_sl_value(nestedds,tTMLinePositionX0,&TMLinePositionX0) &&
			DicomUtils::get_sl_value(nestedds,tTMLinePositionY0,&TMLinePositionY0) &&
			DicomUtils::get_sl_value(nestedds,tTMLinePositionX1,&TMLinePositionX1) &&
			DicomUtils::get_sl_value(nestedds,tTMLinePositionY1,&TMLinePositionY1))
		{
			r.m_TMLineBool = true;
			r.m_TMLinePositionX0 = TMLinePositionX0;
			r.m_TMLinePositionY0 = TMLinePositionY0;
			r.m_TMLinePositionX1 = TMLinePositionX1;
			r.m_TMLinePositionY1 = TMLinePositionY1;
		}

		regions.push_back(r);
	}
}

