#ifndef UltrasoundRegionUtils__H
#define UltrasoundRegionUtils__H

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

#endif // UltrasoundRegionUtils__H
