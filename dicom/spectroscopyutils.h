#ifndef SPECTROSCOPYUTILS__H_
#define SPECTROSCOPYUTILS__H_

#include <mdcmDataSet.h>
#include <vector>
#include <QString>

class SpectroscopyData;
class ImageVariant;
class GLWidget;
class QProgressDialog;
class SpectroscopyUtils
{
public:
	SpectroscopyUtils() {}
	~SpectroscopyUtils(){}
	static bool Read(
		const mdcm::DataSet&,
		SpectroscopyData*);
	static QString ProcessData(
		const mdcm::DataSet&,
		std::vector<ImageVariant*> &,
		int, GLWidget*, bool,
		QProgressDialog*,
		float);
};
#endif // SPECTROSCOPYUTILS__H_
