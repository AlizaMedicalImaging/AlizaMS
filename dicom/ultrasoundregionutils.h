#ifndef A_ULTRASOUNDREGIONUTILS_H
#define A_ULTRASOUNDREGIONUTILS_H

#include "ultrasoundregiondata.h"
#include <mdcmDataSet.h>
#include <QList>

class UltrasoundRegionUtils
{
public:
	UltrasoundRegionUtils() {}
	~UltrasoundRegionUtils(){}
	static void GetBitAlignedPositions(
		const mdcm::DataSet&,
		UltrasoundRegionData&);
	static void GetRanges(const mdcm::DataSet&, UltrasoundRegionData&);
	static void GetTableLookUp(
		const mdcm::DataSet&,
		UltrasoundRegionData&);
	static void GetCodeSequenceLookUp(
		const mdcm::DataSet&,
		UltrasoundRegionData&);
	static void Read(const mdcm::DataSet&, QList<UltrasoundRegionData> &);
};

#endif

