#ifndef A_SPECTROSCOPYDATA_H
#define A_SPECTROSCOPYDATA_H

#include <QString>
#include <vector>

class SpectroscopyData
{
public:
	SpectroscopyData() {}
	~SpectroscopyData() {}
	int            m_NumberOfFrames{1};
	unsigned short m_Rows{};
	unsigned short m_Columns{};
	unsigned int   m_DataPointRows{};
	unsigned int   m_DataPointColumns{};
	QString m_DataRepresentation;
	QString m_SignalDomainColumns;
	QString m_SignalDomainRows;
	std::vector<float> m_FirstOrderPhaseCorrectionAngle;
	std::vector<float> m_SpectroscopyData;
};

#endif

