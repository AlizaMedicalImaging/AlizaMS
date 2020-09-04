#ifndef COMMONUTILS__H_
#define COMMONUTILS__H_

#include <QStringList>
#include <QString>
#include <QByteArray>
#include <QProgressDialog>
#include <QWidget>
#include <QPair>
#include <QList>
#include <mdcmTag.h>
#include <mdcmPrivateTag.h>
#include <mdcmDataSet.h>
#include <mdcmPixelFormat.h>
#include <mdcmPhotometricInterpretation.h>
#include <itkMatrix.h>
#include <vector>
#include <string>

class ImageVariant;
class GLWidget;
class ShaderObj;
class ImageSlice;
class SpectroscopySlice;
class CommonUtils
{
public:
	CommonUtils();
	~CommonUtils();
	static int get_next_id();
	static int get_next_group_id();
	static float random_range(float, float);
	static QString convert_orientation_flag(unsigned int);
	static double set_digits(double, int);
	static QString get_orientation2(const double*);
	static void get_orientation3(char*, float, float, float);
	static void calculate_center_notuniform(
		const std::vector<ImageSlice*> &,float*,float*,float*);
	static void calculate_center_notuniform(
		const std::vector<SpectroscopySlice*> &,float*,float*,float*);
	static void generate_cubeslice(
		std::vector<ImageSlice*> &,
		const QString &,
		const unsigned int, const unsigned int,
		const float, const float, const float,
		const float, const float, const float,
		const float, const float, const float,
		const float, const float, const float,
		const double*);
	static void generate_spectroscopyslice(
		std::vector<SpectroscopySlice*> &,
		const QString&,
		const bool, GLWidget*,
		const float, const float, const float,
		const float, const float, const float,
		const float, const float, const float,
		const float, const float, const float,
		unsigned int, unsigned int);
	static void calculate_rgb_minmax(ImageVariant*);
	static void calculate_rgba_minmax(ImageVariant*);
	static void copy_slices(ImageVariant*, const ImageVariant*);
	static void copy_essential(ImageVariant*, const ImageVariant*);
	static void calculate_minmax_scalar(ImageVariant*);
	static bool reload_monochrome(
		ImageVariant*,
		bool, GLWidget*, int max_3d_tex_size,
		bool=false, unsigned int=0, unsigned int=0);
	static void reset_bb(ImageVariant*);
	static bool reload_rgb_rgba(ImageVariant*);
	static void copy_imagevariant_info(
		ImageVariant*, const ImageVariant*);
	static void copy_imagevariant_info_min(
		ImageVariant*,
		const ImageVariant*);
	static void copy_frametimes(ImageVariant*, const ImageVariant*);
	static void copy_usregions(ImageVariant*, const ImageVariant*);
	static void copy_imagevariant_overlays(
		ImageVariant*, const ImageVariant*);
	static void get_dimensions_(ImageVariant*);
	static QString get_orientation1(
		const ImageVariant*, unsigned int*);
	static QString gen_itk_image(
		bool*,
		std::vector<char*> &, bool,
		const mdcm::PixelFormat&,
		const mdcm::PhotometricInterpretation&,
		ImageVariant*,
		itk::Matrix<double,3,3> &,
		unsigned int, unsigned int, unsigned int,
		double, double, double,
		double, double, double,
		bool, bool,
		bool, unsigned int, unsigned int,
		bool,
		int, GLWidget*, QProgressDialog*,
		bool=false);
	static void read_geometry_from_image_(ImageVariant*);
	static QString get_screenshot_dir();
	static void set_screenshot_dir(const QString&);
	static QString get_screenshot_name(const QString&);
	static QString get_screenshot_name2();
	static QString get_save_dir();
	static void set_save_dir(const QString&);
	static QString get_save_name();
	static QString get_open_dir();
	static void set_open_dir(const QString&);
	static QString apply_per_slice_rescale(
		ImageVariant*,
		const QList< QPair<double, double> > &);
	static void get_pixel_values(
		const QList<ImageVariant*> &,
		int,
		int,
		int,
		QList<double> &);
	static double calculate_max_delta(const ImageVariant*);
	static void random_RGB(float*, float*, float*);
};

#endif // COMMONUTILS__H_
