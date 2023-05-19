#ifndef A_DICOMUTILS_H
#define A_DICOMUTILS_H

#include "structures.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <QWidget>
#include <mdcmTag.h>
#include <mdcmPrivateTag.h>
#include <mdcmDataSet.h>
#include <mdcmPixelFormat.h>
#include <mdcmPhotometricInterpretation.h>
#include <mdcmVR.h>
#include <mdcmDicts.h>
#include <vector>
#include <map>
#include <string>

enum class EnhancedIODLoadingType : short
{
	NotDefined = 0,
	PreferUniformVolumes = 1,
	StrictMultipleImages = 2,
	StrictSingleImage = 3,
	SkipDimensionOrganization = 4
};

enum class DICOMLoadingType : short
{
	Default = 0,
	PRReference = 1,
	RTReference = 2,
	SRReference = 3,
	RWVReference = 4
};

typedef struct
{
	std::string uid;
	mdcm::Tag index_pointer;
	mdcm::Tag group_pointer;
} DimIndex;

typedef std::vector<DimIndex> DimIndexSq;

typedef struct
{
	unsigned int id;
	std::vector<unsigned int> idx;
} DimIndexValue;

typedef std::vector<DimIndexValue> DimIndexValues;

class FrameGroup
{
public:
	FrameGroup() {}
	~FrameGroup() {}
	int    id{-1};
	int    stack_id{-1};
	int    in_stack_pos_num{-1};
	int    temp_pos_idx{-1};
	double temp_pos_off{};
	double us_temp_pos_unknown{};
	double rescale_intercept{};
	double rescale_slope{1.0};
	QString pat_pos;
	QString pat_orient;
	QString pix_spacing;
	QString slice_thick;
	QString window_center;
	QString window_width;
	QString lut_function;
	QString data_type;
	QString frame_body_part;
	QString frame_laterality;
	QString rescale_type;
	QString frame_acquisition_datetime;
	QString frame_reference_datetime;
	bool stack_id_ok{};
	bool in_stack_pos_num_ok{};
	bool temp_pos_idx_ok{};
	bool vol_orient_ok{};
	bool vol_pos_ok{};
	bool temp_pos_off_ok{};
	bool us_temp_pos_unknown_ok{};
	bool rescale_ok{};
	int ref_segment_num{-1};
	double vol_pos[3]{};
	double vol_orient[6]{};
};

typedef std::vector<FrameGroup> FrameGroupValues;

typedef struct
{
	unsigned int type;
	QList<QVariant> values;
} GEMSParam;

class PrConfig
{
public:
	PrConfig() {}
	~PrConfig() {}
	int id{-1};
	QString desc;
	QList<QVariant> values;
};

class PrRefImage
{
public:
	PrRefImage() {}
	~PrRefImage() {}
	QString uid;
	QString file;
	QList<unsigned int> frames;
};

class PrRefSeries
{
public:
	PrRefSeries()  {}
	~PrRefSeries() {}
	QString uid;
	QList<PrRefImage> images;
	QList<PrConfig> prconfig;
	ImageOverlays image_overlays;
};

class DicomUtils
{
public:
	DicomUtils();
	~DicomUtils();
	static QString convert_pn_value(const QString&);
	static QString get_pn_value2(
		const mdcm::DataSet&,
		const mdcm::Tag&,
		const char*);
	static bool get_us_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		unsigned short*);
	static bool get_ss_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		signed short*);
	static bool get_sl_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		int *);
	static bool get_ul_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		unsigned int*);
	static bool get_fd_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		double*);
	static bool priv_get_fd_value(
		const mdcm::DataSet&, const mdcm::PrivateTag&,
		double*);
	static bool get_fl_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		float*);
	static bool priv_get_fl_value(
		const mdcm::DataSet&, const mdcm::PrivateTag&,
		float*);
	static bool get_us_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<unsigned short> &);
	static bool get_ss_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<signed short> &);
	static bool get_sl_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<int> &);
	static bool get_ul_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<unsigned int> &);
	static bool get_fd_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<double> &);
	static bool priv_get_fd_values(
		const mdcm::DataSet& ds, const mdcm::PrivateTag&,
		std::vector<double> &);
	static bool get_fl_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<float> &);
	static bool get_ds_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<double> &);
	static bool priv_get_ds_values(
		const mdcm::DataSet&, const mdcm::PrivateTag&,
		std::vector<double> &);
	static bool get_is_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		int*);
	static bool get_is_values(
		const mdcm::DataSet&, const mdcm::Tag&,
		std::vector<int> &);
	static bool get_at_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		unsigned short*, unsigned short*);
	static bool get_string_value(
		const mdcm::DataSet&, const mdcm::Tag&,
		QString&);
	static bool priv_get_string_value(
		const mdcm::DataSet&, const mdcm::PrivateTag&,
		QString&);
	static QString generate_id();
	static bool is_image(
		const mdcm::DataSet&,
		unsigned short*,unsigned short*,
		unsigned short*,unsigned short*,unsigned short*,
		short*,
		bool*);
	static bool has_functional_groups(const mdcm::DataSet&);
	static bool has_supp_palette(const mdcm::DataSet&);
	static bool has_modality_lut_sq(const mdcm::DataSet&);
	static bool check_encapsulated(const mdcm::DataSet&);
	static bool is_multiframe(const mdcm::DataSet&);
	static void read_image_info(
		const QString&,
		unsigned short*,
		unsigned short*,
		QString&,
		QString&,
		QString&,
		QString&,
		QString&);
	static void read_image_info_rtdose(
		const QString&,
		unsigned short*,
		unsigned short*,
		unsigned short*,
		QString&,
		QString&,
		QString&,
		std::vector<double> &);
	static void load_contour(
		const mdcm::DataSet&, ImageVariant*);
	static int read_instance_number(const mdcm::DataSet&);
	static QString read_instance_uid(const mdcm::DataSet&);
	static void read_window(
		const mdcm::DataSet&, double*, double*, short*);
	static void read_us_regions(
		const mdcm::DataSet&,
		ImageVariant*);
	static bool read_slices(
		const QStringList&, ImageVariant*,
		const bool, const bool,
		float);
	static bool read_slices_uihgrid(
		const mdcm::DataSet&, ImageVariant*,
		const bool, const bool,
		float);
	static bool read_slices_rtdose(
		const QString&, ImageVariant*,
		const bool, const bool,
		float);
	static void read_dimension_index_sq(
		const mdcm::DataSet&, DimIndexSq&);
	static bool read_dimension_index_values(
		const mdcm::DataSet&,
		const unsigned int,
		DimIndexValues&);
	static bool read_group_sq(
		const mdcm::DataSet&,
		const mdcm::Tag&,
		const DimIndexSq&,
		DimIndexValues&,
		FrameGroupValues&);
	static void read_frame_times(
		const mdcm::DataSet&, ImageVariant*, int);
	static QString read_anatomic_sq(
		const mdcm::DataSet&);
	static QString read_series_laterality(
		const mdcm::DataSet&);
	static QString read_image_laterality(
		const mdcm::DataSet&);
	static QString read_body_part(const mdcm::DataSet&);
	static void read_acquisition_time(
		const mdcm::DataSet&, QString&, QString&);
	static void read_ivariant_info_tags(
		const mdcm::DataSet&, ImageVariant*);
	static void read_pet_attributes(
		const mdcm::DataSet&, ImageVariant*);
	static bool get_patient_position(
		const QString&, double*);
	static bool get_patient_orientation(
		const QString&, double*);
	static bool get_pixel_spacing(const QString&, double*);
	static bool generate_geometry(
			std::vector<ImageSlice*> &,
			const std::vector<double*> &,
			const unsigned int, const unsigned int,
			const double, const double, double*,
			bool*, bool*,
			double*, double*, double*,
			double*,
			float*,float*,float*,
			float*,float*,float*,
			float*,float*,float*,
			float);
	static bool build_gems_dictionary(
		QMap<QString,int> &,
		const mdcm::DataSet&);
	static void read_gems_params(
		QMap<int,GEMSParam> &,
		const mdcm::DataElement&,
		const QMap<QString,int> &);
	static void enhanced_get_indices(
		const DimIndexSq & sq,
		int*,int*,int*,int*,int*, int*,int*,
		const short);
	static void enhanced_process_values(
		FrameGroupValues&, const FrameGroupValues&);
	static void enhanced_check_rescale(
		const mdcm::DataSet&,
		FrameGroupValues&);
	static void print_sq(const DimIndexSq&);
	static void print_func_group(const FrameGroupValues&);
	static bool read_shutter(
		const mdcm::DataSet&,
		PRDisplayShutter&);
	static QString read_enhanced(
		bool*, const QString&, std::vector<ImageVariant*> &,
		bool,
		const bool,
		const short,
		const QWidget*,
		const float,
		const bool);
	static QString read_enhanced_supp_palette(
		bool*, const QString&, std::vector<ImageVariant*> &,
		bool,
		const bool,
		const short,
		const QWidget*,
		const float);
	static QString read_ultrasound(
		bool*, const short,
		ImageVariant*,
		const QStringList&,
		const QWidget*);
	static QString read_nuclear(
		bool*, const short,
		ImageVariant*,
		const QStringList&,
		bool,
		const QWidget*);
	static QString read_series(
		bool*,
		const bool,
		const bool,
		const bool,
		const bool,
		ImageVariant*, const QStringList&,
		bool,
		const QWidget*,
		float,
		bool);
	static bool convert_elscint(
		const QString,
		const QString);
	static QString read_buffer(
		bool*, std::vector<char*> &,
		ImageOverlays &, const int,
		AnatomyMap &, const int,
		const QString&, const bool,
		mdcm::PixelFormat&, const bool,
		mdcm::PhotometricInterpretation&,
		unsigned int*, unsigned int*, unsigned int*,
		double*,double*,double*,
		double*,double*,double*,
		double*,
		double*,double*,
		const bool,
		const bool,
		const bool,
		const bool,
		const bool,
		const bool,
		const bool,
		const bool,
		const bool,
		int*,
		unsigned long long*,
		const bool, bool *);
	static QString read_enhanced_common(
		bool*,
		std::vector<ImageVariant*> &,
		const QString&,
		const QString&,
		const std::vector<char*> &,
		const ImageOverlays&,
		const unsigned int,
		const unsigned int,
		const mdcm::PixelFormat&,
		const mdcm::PhotometricInterpretation&,
		std::vector
			< std::map
				<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >
			> &,
		const DimIndexValues&,
		const FrameGroupValues&,
		const bool,
		const bool,
		const QWidget*,
		double*,
		const int,
		const double, const double, const double,
		const double, const double, const double,
		const double, const double,
		const bool,
		const bool,
		const float);
	static bool enhanced_process_indices(
		std::vector
			< std::map
				<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >
			> &,
		const DimIndexValues&,
		const FrameGroupValues&,
		const int, const int, const int, const int, const int, const int,
		const short);
	static QString read_enhanced_3d_8d(
		bool*,
		std::vector<ImageVariant*> &,
		const QString &,
		const QString &,
		const std::vector<char*> &,
		const ImageOverlays&,
		const unsigned int, const unsigned int,
		const mdcm::PixelFormat&,
		const mdcm::PhotometricInterpretation&,
		const int, const int, const int, const int, const int, const int,
		const DimIndexValues&,
		const FrameGroupValues&,
		const bool,
		const bool,
		const QWidget*,
		double*,
		const int,
		const double, const double, const double,
		const double, const double, const double,
		const double, const double,
		const bool,
		const bool,
		const float,
		const short);
	static bool is_not_interleaved(const QStringList&);
	static bool is_mosaic(const mdcm::DataSet&);
	static bool is_uih_grid(const mdcm::DataSet&);
	static bool is_elscint(const mdcm::DataSet&);
	static bool is_elscint_rgb(const mdcm::DataSet&);
	static void write_encapsulated(const QString&, const QString&);
	static QString suffix_mpeg(const QString&);
	static void write_mpeg(const QString&, const QString&);
	static bool is_dicom_file(const QString&);
	static void scan_dir_for_rtstruct_image(
		const QString&, QList<QStringList> &);
	static void scan_files_for_rtstruct_image(
		const QString&, QList<QStringList> &);
	static bool process_contrours_ref(
		const QString&, const QString&,
		std::vector<ImageVariant*> &,
		bool,
		short,
		const QWidget*);
	static QString find_file_from_uid(
		const QString&,
		const QString&);
	static bool scan_files_for_instance_uid(
		const QString&,
		const QString&,
		QString&);
	static void read_pr_ref(
		const QString&,
		const QString&,
		QList<PrRefSeries> &);
	static QString read_enhmr_spectro_info(
		const mdcm::DataSet&,
		bool);
	static QString read_enhct_info(
		const mdcm::DataSet&);
	static void global_force_suppllut(
		short);
	static mdcm::VR get_vr(
		const mdcm::DataSet &,
		const mdcm::Tag&,
		const bool,
		const mdcm::Dicts&);
	static bool compatible_sq(
		const mdcm::DataSet&,
		const mdcm::Tag&,
		const bool,
		const mdcm::Dicts&);
	static QString generate_uid();
	static bool read_gray_lut(
		const mdcm::DataSet&,
		const mdcm::Tag,
		QList<QVariant> &,
		QList<QVariant> &,
		bool*,
		bool*);
	//
	// Type of object processing
	//
	// 0
	// 1 referenced in PR
	// 2 referenced in RT
	// 3 referenced in SR
	static QString read_dicom(
		std::vector<ImageVariant*> & ,
		QStringList &,
		QStringList &,
		QStringList &,
		QStringList &,
		QStringList &,
		const QStringList&,
		bool,
		const QWidget*,
		short, // type of object processing
		short);
};

#endif

