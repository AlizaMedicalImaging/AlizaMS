#ifndef A_SpectroscopyData_H
#define A_SpectroscopyData_H

#include <QString>
#include <vector>

class SpectroscopyData
{
public:
	SpectroscopyData() :
		m_NumberOfFrames(1),
		m_Rows(0),
		m_Columns(0),
		m_DataPointRows(0),
		m_DataPointColumns(0),
		m_DataRepresentation(""),
		m_SignalDomainColumns(""),
		m_SignalDomainRows("")
	{
	}
	~SpectroscopyData()
	{
	}
	int            m_NumberOfFrames;
	unsigned short m_Rows;
	unsigned short m_Columns;
	unsigned int   m_DataPointRows;
	unsigned int   m_DataPointColumns;
	QString m_DataRepresentation;
	QString m_SignalDomainColumns;
	QString m_SignalDomainRows;
	std::vector<float> m_FirstOrderPhaseCorrectionAngle;
	std::vector<float> m_SpectroscopyData;
};

#endif
