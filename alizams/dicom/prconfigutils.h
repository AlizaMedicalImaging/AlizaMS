#ifndef PrConfigUtils_H__
#define PrConfigUtils_H__

#include "mdcmDataSet.h"
#include <QList>

class PrRefSeries;
class ImageVariant;
class PrConfig;
class SettingsWidget;
class GLWidget;
class PrConfigUtils
{
public:
	PrConfigUtils();
	~PrConfigUtils();
	static void read_modality_lut(const mdcm::DataSet &, PrRefSeries &);
	static void read_voi_lut(const mdcm::DataSet &, PrRefSeries &);
	static void read_display_areas(const mdcm::DataSet &, PrRefSeries &);
	static void read_graphic_layers(const mdcm::DataSet &, PrRefSeries &);
	static void read_spatial_transformation(
		const mdcm::DataSet&,PrRefSeries&);
	static void read_graphic_objects(
		const mdcm::DataSet&,
		PrRefSeries&);
	static void read_text_annotations(
		const mdcm::DataSet&,
		PrRefSeries&);
	static void read_display_shutter(
		const mdcm::DataSet&,
		PrRefSeries&);
	static void read_pr(const mdcm::DataSet &, PrRefSeries &);
	static ImageVariant * make_pr_monochrome(
		const ImageVariant*,
		const PrRefSeries &,
		const SettingsWidget*,
		GLWidget*,
		bool,
		bool*);
	static ImageVariant * make_pr_rgb(
		const ImageVariant*,
		const PrRefSeries &,
		const SettingsWidget*);
	static ImageVariant * make_levels_monochrome(
		const ImageVariant*,
		const PrRefSeries &,
		const SettingsWidget*,
		GLWidget*,
		bool);
};

#endif
