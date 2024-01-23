#ifndef A_ULTRASOUNDREGIONDATA_H
#define A_ULTRASOUNDREGIONDATA_H

#include <QString>

class UltrasoundRegionData
{
public:
	unsigned short m_RegionSpatialFormat{};
	unsigned short m_RegionDataType{};
	bool           m_FlagsBool{};
	bool           m_ReferencePixelBool{};
	unsigned short m_PhysicalUnitsXDirection{};
	unsigned short m_PhysicalUnitsYDirection{};
	bool           m_ReferencePixelPhysicalValueBool{};
	double         m_PhysicalDeltaX{1.0};
	double         m_PhysicalDeltaY{1.0};
	bool           m_PixelComponentOrganizationBool{};
	unsigned short m_PixelComponentOrganization{};
	bool           m_TMLineBool{};
	QString        m_UnitXString;
	QString        m_UnitYString;
	QString        m_SpatialFormatString;
	QString        m_DataTypeString;
	QString        m_PixelComponentOrganizationString;
	unsigned int   m_X0{};
	unsigned int   m_Y0{};
	unsigned int   m_X1{};
	unsigned int   m_Y1{};
	unsigned int   m_RegionFlags{};
	int            m_ReferencePixelX0{};
	int            m_ReferencePixelY0{};
	double         m_ReferencePixelPhysicalValueX{};
	double         m_ReferencePixelPhysicalValueY{};
	int            m_TMLinePositionX0{};
	int            m_TMLinePositionY0{};
	int            m_TMLinePositionX1{};
	int            m_TMLinePositionY1{};
};

#endif

