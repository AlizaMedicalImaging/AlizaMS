#define USE_GET_TOTAL_MEM

// Caution, must be in sync with mdcmJPEGBITSCodec.hxx,
// not tested
#define TRY_SUPPORT_YBR_PARTIAL_422

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt5-core.h"
#else
#include "CG/glwidget-qt5.h"
#endif
#else
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt4-core.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#endif
#include "structures.h"
#include "vectormath/scalar/vectormath.h"
#include "commonutils.h"
#include <itkVersion.h>
#include <itkIndex.h>
#include <itkImageRegionIterator.h>
#include <itkSpatialOrientation.h>
#include <itkSpatialOrientationAdapter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkImageSliceConstIteratorWithIndex.h>
#include <itkImageSliceIteratorWithIndex.h>
#include <itkIdentityTransform.h>
#include <itkResampleImageFilter.h>
#include <QSet>
#include <QApplication>
#include <QFileInfo>
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include "settingswidget.h"
#include "updateqtcommand.h"
#include <iostream>
#include <list>
#include <cstdlib>
#include <random>
#include <chrono>
#include <functional>
#include <cfloat>
#include <atomic>
#include "dicomutils.h"
#include "colorspace/colorspace.h"
#ifdef USE_GET_TOTAL_MEM
#if (defined  __FreeBSD__ || defined __APPLE__)
#include <sys/sysctl.h>
#elif (defined  __GNUC__ && !defined  _WIN32)
#include <sys/sysinfo.h>
#endif
#endif

#ifdef ALIZA_LINUX_DEBUG_MEM
#include <unistd.h>
#include <ios>
#include <fstream>
#endif

namespace
{

static QString screenshot_dir("");
static QString save_dir("");
static QString open_dir("");
static double saved_total_memory = 0.0;

typedef Vectormath::Scalar::Vector3 sVector3;

double abs_max(double x, double y, double z)
{
	const double x_ = fabs(x);
	const double y_ = fabs(y);
	const double z_ = fabs(z);
	double r;
	r = (x_ > y_) ? x_ : y_;
	r = (z_ > r)  ? z_ : r;
	return r;
}

bool check_direction_matrix(const itk::Matrix<itk::SpacePrecisionType, 3, 3> & d)
{
	const bool r =
		d[0][0] > -0.000001 && d[0][0] < 0.000001 &&
		d[1][0] > -0.000001 && d[1][0] < 0.000001 &&
		d[2][0] > -0.000001 && d[2][0] < 0.000001 &&
		d[0][1] > -0.000001 && d[0][1] < 0.000001 &&
		d[1][1] > -0.000001 && d[1][1] < 0.000001 &&
		d[2][1] > -0.000001 && d[2][1] < 0.000001 &&
		d[0][2] > -0.000001 && d[0][2] < 0.000001 &&
		d[1][2] > -0.000001 && d[1][2] < 0.000001 &&
		d[2][2] > -0.000001 && d[2][2] < 0.000001;
	return r;
}

template<typename T> void calculate_min_max(
	const typename T::Pointer & image,
	ImageVariant * iv)
{
	if (image.IsNull()) return;
	typedef  itk::MinimumMaximumImageCalculator<T> MinMaxCalculator;
	typename MinMaxCalculator::Pointer min_max_calculator =
		MinMaxCalculator::New();
	typename UpdateQtCommand::Pointer update_qt_command =
		UpdateQtCommand::New();
	double cubemin{};
	double cubemax{};
	try
	{
		min_max_calculator->AddObserver(itk::ProgressEvent(), update_qt_command);
		min_max_calculator->SetImage(image);
		min_max_calculator->SetRegion(image->GetLargestPossibleRegion());
		min_max_calculator->Compute();
		const double cubemin_tmp = static_cast<double>(min_max_calculator->GetMinimum());
		const double cubemax_tmp = static_cast<double>(min_max_calculator->GetMaximum());
		// Default values of the filter are 'min > max' for e.g. an empty image.
		if (!(cubemin_tmp > cubemax_tmp))
		{
			cubemin = cubemin_tmp;
			cubemax = cubemax_tmp;
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
#ifdef ALIZA_VERBOSE
		std::cout << ex.GetDescription() << std::endl;
#endif
		return;
	}
	if (iv->di->maxwindow)
	{
		switch (iv->image_type)
		{
		case 0:
			{
				if (iv->di->bits_allocated > 0 && iv->di->bits_stored > 0 &&
					(iv->di->bits_stored < iv->di->bits_allocated))
				{
					const double tmp0 = pow(2.0, static_cast<double>(iv->di->bits_stored - 1));
					iv->di->rmin = -tmp0;
					iv->di->rmax = tmp0 - 1.0;
					if (cubemin < iv->di->rmin || cubemax > iv->di->rmax)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: bits stored = " << iv->di->bits_stored
							<< " (ignored), min = " << cubemin << ", max = " << cubemax << std::endl;
#endif
						iv->di->rmin = SHRT_MIN;
						iv->di->rmax = SHRT_MAX;
						iv->di->bits_stored = iv->di->bits_allocated;
						iv->di->high_bit = iv->di->bits_stored - 1;
					}
				}
				else
				{
					iv->di->rmin = SHRT_MIN;
					iv->di->rmax = SHRT_MAX;
				}
			}
			break;
		case 1:
			{
				if (iv->di->bits_allocated > 0 && iv->di->bits_stored > 0 &&
					(iv->di->bits_stored < iv->di->bits_allocated))
				{
					iv->di->rmin = 0.0;
					iv->di->rmax = pow(2.0, static_cast<double>(iv->di->bits_stored)) - 1.0;
					if (cubemax > iv->di->rmax)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: bits stored = " << iv->di->bits_stored
							<< " (ignored), min = 0, max = " << cubemax << std::endl;
#endif
						iv->di->rmax = USHRT_MAX;
						iv->di->bits_stored = iv->di->bits_allocated;
						iv->di->high_bit = iv->di->bits_stored - 1;
					}
				}
				else
				{
					iv->di->rmin = 0.0;
					iv->di->rmax = USHRT_MAX;
				}
			}
			break;
		case 4:
			{
				if (iv->di->bits_allocated > 0 && iv->di->bits_stored > 0 &&
					(iv->di->bits_stored < iv->di->bits_allocated))
				{
					iv->di->rmin = 0.0;
					iv->di->rmax = pow(2.0, static_cast<double>(iv->di->bits_stored)) - 1.0;
					if (cubemax > iv->di->rmax)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: bits stored = " << iv->di->bits_stored
							<< " (ignored), min = 0, max = " << cubemax << std::endl;
#endif
						iv->di->rmax = UCHAR_MAX;
						iv->di->bits_stored = iv->di->bits_allocated;
						iv->di->high_bit = iv->di->bits_stored - 1;
					}
				}
				else
				{
					iv->di->rmin = 0.0;
					iv->di->rmax = UCHAR_MAX;
				}
			}
			break;
		case 2:
		case 3:
		case 5:
		case 6:
		case 7:
		case 8:
			{
				iv->di->rmin = cubemin;
				iv->di->rmax = cubemax;
			}
			break;
		default:
			return;
		}
		iv->di->vmin = cubemin;
		iv->di->vmax = cubemax;
	}
	else
	{
		switch (iv->image_type)
		{
		case 0:
			{
				if (iv->di->bits_allocated > 0 && iv->di->bits_stored > 0 &&
					(iv->di->bits_stored < iv->di->bits_allocated))
				{
					const double tmp0 = pow(2.0, static_cast<double>(iv->di->bits_stored - 1));
					const double rmin = -tmp0;
					const double rmax = tmp0 - 1.0;
					if (cubemin < rmin || cubemax > rmax)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: bits stored = " << iv->di->bits_stored
							<< " (ignored), min = " << cubemin << ", max = " << cubemax << std::endl;
#endif
						iv->di->bits_stored = iv->di->bits_allocated;
						iv->di->high_bit = iv->di->bits_stored - 1;
					}
				}
			}
			break;
		case 1:
			{
				if (iv->di->bits_allocated > 0 && iv->di->bits_stored > 0 &&
					(iv->di->bits_stored < iv->di->bits_allocated))
				{
					const double rmax = pow(2.0, static_cast<double>(iv->di->bits_stored)) - 1.0;
					if (cubemax > rmax)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: bits stored = " << iv->di->bits_stored
							<< " (ignored), min = 0, max = " << cubemax << std::endl;
#endif
						iv->di->bits_stored = iv->di->bits_allocated;
						iv->di->high_bit = iv->di->bits_stored - 1;
					}
				}
			}
			break;
		case 4:
			{
				if (iv->di->bits_allocated > 0 && iv->di->bits_stored > 0 &&
					(iv->di->bits_stored < iv->di->bits_allocated))
				{
					const double rmax = pow(2.0, static_cast<double>(iv->di->bits_stored)) - 1.0;
					if (cubemax > rmax)
					{
#ifdef ALIZA_VERBOSE
						std::cout << "Warning: bits stored = " << iv->di->bits_stored
							<< " (ignored), min = 0, max = " << cubemax << std::endl;
#endif
						iv->di->bits_stored = iv->di->bits_allocated;
						iv->di->high_bit = iv->di->bits_stored - 1;
					}
				}
			}
			break;
		case 2:
		case 3:
		case 5:
		case 6:
		case 7:
		case 8:
			break;
		default:
			return;
		}
		iv->di->rmin = iv->di->vmin = cubemin;
		iv->di->rmax = iv->di->vmax = cubemax;
	}
	const double vmax_minus_vmin = iv->di->vmax - iv->di->vmin;
	const double rmax_minus_rmin = iv->di->rmax - iv->di->rmin;
	if ((iv->di->us_window_width <= -999999.0) ||
		((iv->di->default_us_window_width > (iv->di->vmax - iv->di->vmin)) ||
			(iv->di->default_us_window_center > iv->di->vmax ||
				iv->di->default_us_window_center < iv->di->vmin)))
	{
		if (iv->image_type == 4)
		{
			iv->di->default_us_window_center = iv->di->us_window_center = 128.0;
			iv->di->default_us_window_width  = iv->di->us_window_width  = 255.0;
			iv->di->default_lut_function = iv->di->lut_function = 0;
		}
		else
		{
			const double tmp110 = (vmax_minus_vmin > 0.0) ? vmax_minus_vmin : rmax_minus_rmin;
			const double tmp111 = (vmax_minus_vmin > 0.0) ? iv->di->vmin    : iv->di->rmin;
			if (tmp110 > 0)
			{
				iv->di->default_us_window_center = iv->di->us_window_center = ((tmp110 / 2.0) - (-tmp111));
				iv->di->default_us_window_width  = iv->di->us_window_width  = tmp110;
			}
			else
			{
				iv->di->default_us_window_center = iv->di->us_window_center = 0.5 * iv->di->vmax;
				iv->di->default_us_window_width  = iv->di->us_window_width  = fabs(iv->di->vmax);
			}
		}
	}
	//
	if (iv->di->default_us_window_width <= 1.0 || ((iv->di->vmax - iv->di->vmin) <= 1.0))
	{
		if (iv->di->default_lut_function == 1)
		{
			iv->di->default_lut_function = iv->di->lut_function = 0;
		}
	}
	//
	{
		std::vector<int> undef_windows;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		FrameLevels::const_iterator it = iv->frame_levels.cbegin();
		while (it != iv->frame_levels.cend())
#else
		FrameLevels::const_iterator it = iv->frame_levels.constBegin();
		while (it != iv->frame_levels.constEnd())
#endif
		{
			const FrameLevel & fl = it.value();
			if (fl.us_window_width <= -999999.0)
			{
				undef_windows.push_back(it.key());
			}
			++it;
		}
		if (!undef_windows.empty())
		{
			FrameLevel fl;
			fl.us_window_center = iv->di->default_us_window_center;
			fl.us_window_width = iv->di->default_us_window_width;
			fl.lut_function = iv->di->default_lut_function;
			for (size_t x = 0; x < undef_windows.size(); ++x)
			{
				iv->frame_levels[undef_windows.at(x)] = fl;
			}
		}
	}
	//
	iv->di->window_center = (iv->di->us_window_center + (-iv->di->rmin)) / rmax_minus_rmin;
	iv->di->window_width  = iv->di->us_window_width / rmax_minus_rmin;
	if (iv->di->window_width  <= 0) iv->di->window_width  = 1e-6;
	if (iv->di->window_width   > 1) iv->di->window_width  = 1.0;
	if (iv->di->window_center  < 0) iv->di->window_center = 0.0;
	if (iv->di->window_center  > 1) iv->di->window_center = 1.0;
	if ((iv->di->rmax - iv->di->rmin) < 3.0) iv->di->disable_int_level = true;
	else iv->di->disable_int_level = false;
}

template <typename T> void calculate_rgb_minmax_(
	const typename T::Pointer image,
	ImageVariant * ivariant)
{
	if (image.IsNull()) return;
	const typename T::RegionType region = image->GetLargestPossibleRegion();
	double max_r = std::numeric_limits<double>::min();
	double max_g = std::numeric_limits<double>::min();
	double max_b = std::numeric_limits<double>::min();
	double min_r = std::numeric_limits<double>::max();
	double min_g = std::numeric_limits<double>::max();
	double min_b = std::numeric_limits<double>::max();
	try
	{
		itk::ImageRegionConstIterator<T> iterator(image, region);
		iterator.GoToBegin();
		while (!iterator.IsAtEnd())
		{
			const double b = iterator.Get().GetBlue();
			const double g = iterator.Get().GetGreen();
			const double r = iterator.Get().GetRed();
			if (b > max_b) max_b = b;
			if (g > max_g) max_g = g;
			if (r > max_r) max_r = r;
			if (b < min_b) min_b = b;
			if (g < min_g) min_g = g;
			if (r < min_r) min_r = r;
			++iterator;
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
#ifdef ALIZA_VERBOSE
		std::cout << ex.GetDescription() << std::endl;
#else
		(void)ex;
#endif
	}
	double min_ = std::numeric_limits<double>::max();
	double mins[3];
	mins[0] = min_r;
	mins[1] = min_g;
	mins[2] = min_b;
	for (unsigned int x = 0; x < 3; ++x)
	{
		if (mins[x] < min_) min_ = mins[x];
	}
	double max_ = std::numeric_limits<double>::min();
	double maxs[3];
	maxs[0] = max_r;
	maxs[1] = max_g;
	maxs[2] = max_b;
	for (unsigned int x = 0; x < 3; ++x)
	{
		if (maxs[x] > max_) max_ = maxs[x];
	}
	ivariant->di->vmin = min_;
	ivariant->di->vmax = max_;
}

template <typename T> void calculate_rgba_minmax_(
	const typename T::Pointer image,
	ImageVariant * ivariant)
{
	if (image.IsNull()) return;
	const typename T::RegionType region = image->GetLargestPossibleRegion();
	double max_r = std::numeric_limits<double>::min();
	double max_g = std::numeric_limits<double>::min();
	double max_b = std::numeric_limits<double>::min();
	double max_a = std::numeric_limits<double>::min();
	double min_r = std::numeric_limits<double>::max();
	double min_g = std::numeric_limits<double>::max();
	double min_b = std::numeric_limits<double>::max();
	double min_a = std::numeric_limits<double>::max();
	try
	{
		itk::ImageRegionConstIterator<T> iterator(image, region);
		iterator.GoToBegin();
		while (!iterator.IsAtEnd())
		{
			const double a = iterator.Get().GetAlpha();
			const double b = iterator.Get().GetBlue();
			const double g = iterator.Get().GetGreen();
			const double r = iterator.Get().GetRed();
			if (a > max_a) max_a = a;
			if (b > max_b) max_b = b;
			if (g > max_g) max_g = g;
			if (r > max_r) max_r = r;
			if (a < min_a) min_a = a;
			if (b < min_b) min_b = b;
			if (g < min_g) min_g = g;
			if (r < min_r) min_r = r;
			++iterator;
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
#ifdef ALIZA_VERBOSE
		std::cout << ex.GetDescription() << std::endl;
#else
		(void)ex;
#endif
	}
	double min_ = std::numeric_limits<double>::max();
	double mins[4];
	mins[0] = min_r;
	mins[1] = min_g;
	mins[2] = min_b;
	mins[3] = min_a;
	for (unsigned int x = 0; x < 4; ++x)
	{
		if (mins[x] < min_) min_ = mins[x];
	}
	double max_ = std::numeric_limits<double>::min();
	double maxs[4];
	maxs[0] = max_r;
	maxs[1] = max_g;
	maxs[2] = max_b;
	maxs[3] = max_a;
	for (unsigned int x = 0; x < 4; ++x)
	{
		if (maxs[x] > max_) max_ = maxs[x];
	}
	ivariant->di->vmin = min_;
	ivariant->di->vmax = max_;
}

template<typename T> void get_dimensions(
	const typename T::Pointer & image,
	int * dimx, int * dimy, int * dimz,
	double * spacingx, double * spacingy, double * spacingz,
	float * originx, float * originy, float * originz)
{
	if (image.IsNull()) return;
	typename T::SizeType size;
	typename T::SpacingType spacing;
	typename T::PointType origin;
	try
	{
		size    = image->GetLargestPossibleRegion().GetSize();
		spacing = image->GetSpacing();
		origin  = image->GetOrigin();
	}
	catch (const itk::ExceptionObject & ex)
	{
#ifdef ALIZA_VERBOSE
		std::cout << ex.GetDescription() << std::endl;
#else
		(void)ex;
#endif
		return;
	}
	*dimx = size[0];
	*dimy = size[1];
	*dimz = size[2];
	*spacingx = spacing[0];
	*spacingy = spacing[1];
	*spacingz = spacing[2];
	*originx  = static_cast<float>(origin[0]);
	*originy  = static_cast<float>(origin[1]);
	*originz  = static_cast<float>(origin[2]);
}

template<typename T> QString get_orientation(
	const typename T::Pointer & image,
	unsigned int * result)
{
	if (image.IsNull()) return QString("");
	itk::SpatialOrientationAdapter adapter;
	*result = static_cast<unsigned int>(
		adapter.FromDirectionCosines(image->GetDirection()));
	const QString f =
		CommonUtils::convert_orientation_flag(static_cast<unsigned int>(
			adapter.FromDirectionCosines(image->GetDirection())));
	return f;
}

template<typename T> int generate_tex3d(
	ImageVariant * ivariant,
	const typename T::Pointer & image,
	size_t * size, double * spacing,
	QProgressDialog * pb,
	GLWidget * gl)
{
	if (image.IsNull() || !ivariant || !gl) return 1;
	if (size[0] < 1 || size[1] < 1)
	{
#ifdef ALIZA_VERBOSE
		std::cout << "(size[0] < 1||size[1] < 1)" << std::endl;
#endif
		return 1;
	}
	typedef itk::NearestNeighborInterpolateImageFunction<T, double> InterpolatorType;
	typedef itk::IdentityTransform<double, 3> IdentityTransformType;
	typedef itk::ResampleImageFilter<T, T> ScaleFilter;
	typedef itk::ImageSliceConstIteratorWithIndex<T> SliceConstIteratorType;
	std::string tt;
	int error__{};
	GLuint glerror__{};
	double rmin{};
	double rmax{};
	float * float_buf{};
	unsigned short * short_buf{};
	GLubyte * ub_buf{};
	typename T::Pointer out_image;
	bool scale{true};
	short texture_type{-1};
	const typename T::RegionType r__ = image->GetLargestPossibleRegion();
	const typename T::SizeType original_size = r__.GetSize();
	calculate_min_max<T>(image, ivariant);
	rmin = ivariant->di->rmin;
	rmax = ivariant->di->rmax;
	switch (ivariant->image_type)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 7:
		case 8:
			texture_type = 1; // GL_R16
			break;
		case 4:
			texture_type = 2; // GL_R8
			break;
		case 5:
		case 6:
			texture_type = 0; // GL_R16F
			break;
		default:
			return 1;
	}
#if 0
	{
		QString s("?");
		switch (texture_type)
		{
		case 0:
			s = QVariant((int)(
					(size[0]*size[1]*size[2]*sizeof(float))/
					1048576.0)).toString();
			break;
		case 1:
			s = QVariant((int)(
					(size[0]*size[1]*size[2]*sizeof(unsigned short))/
					1048576.0)).toString();
			break;
		case 2:
			s = QVariant((int)(
					(size[0]*size[1]*size[2]*sizeof(GLubyte))/
					1048576.0)).toString();
			break;
		default: break;
		}
		std::cout << "OpenGL " << s.toStdString() + " MB" << std::endl;
	}
#endif
	//
	if (pb)
	{
		pb->setLabelText(QString("Generating OpenGL texture"));
	}
	qApp->processEvents();
	//
	if (size[0] == original_size[0] && size[1] == original_size[1] && size[2] == original_size[2])
	{
		scale = false;
	}
	//
	if (scale)
	{
		typename ScaleFilter::Pointer scaleFilter = ScaleFilter::New();
		typename T::SizeType size_;
		size_[0] = size[0];
		size_[1] = size[1];
		size_[2] = size[2];
		typename T::SpacingType spacing_;
		spacing_[0] = spacing[0];
		spacing_[1] = spacing[1];
		spacing_[2] = spacing[2];
		try
		{
			scaleFilter->SetInput(image);
			scaleFilter->SetSize(size_);
			scaleFilter->SetOutputSpacing(spacing_);
			scaleFilter->SetOutputOrigin(image->GetOrigin());
			scaleFilter->SetOutputDirection(image->GetDirection());
			scaleFilter->SetInterpolator(InterpolatorType::New());
			scaleFilter->SetTransform(IdentityTransformType::New());
			scaleFilter->UpdateLargestPossibleRegion();
			out_image = scaleFilter->GetOutput();
		}
		catch (const itk::ExceptionObject & ex)
		{
#ifdef ALIZA_VERBOSE
			std::cout << ex.GetDescription() << std::endl;
#else
		 	(void)ex;
#endif
			return 1;
		}
		if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	}
	else
	{
		out_image = image;
	}
	//
	if (out_image.IsNotNull())
	{
		const typename T::SpacingType final_spacing = out_image->GetSpacing();
		ivariant->di->x_spacing = final_spacing[0];
		ivariant->di->y_spacing = final_spacing[1];
		ivariant->di->dimx = size[0];
		ivariant->di->dimy = size[1];
	}
	else
	{
#ifdef ALIZA_VERBOSE
		std::cout << "out_image.IsNull()" << std::endl;
#endif
		return 1;
	}
	//
	// array maximum size 0x7fffffff
	switch (texture_type)
	{
	case 0:
		{
			if ((size[0] * size[1] * size[2]) >= 0x7fffffff / sizeof(float))
			{
				return 2;
			}
			try
			{
				float_buf = new float[(size[0] * size[1] * size[2])];
			}
			catch (const std::bad_alloc&)
			{
				return 2;
			}
			tt = " GL_R16F";
		}
		break;
	case 1:
		{
			if ((size[0] * size[1] * size[2]) >= 0x7fffffff / sizeof(unsigned short))
			{
				return 2;
			}
			try
			{
				short_buf = new unsigned short[(size[0] * size[1] * size[2])];
			}
			catch (const std::bad_alloc&)
			{
				return 2;
			}
			tt = " GL_R16";
		}
		break;
	case 2:
		{
			if ((size[0] * size[1] * size[2]) >= 0x7fffffff / sizeof(GLubyte))
			{
				return 2;
			}
			try
			{
				ub_buf = new GLubyte[(size[0] * size[1] * size[2])];
			}
			catch (const std::bad_alloc&)
			{
				return 2;
			}
			tt = " GL_R8";
		}
		break;
	default:
		return 1;
	}
	//
	try
	{
		SliceConstIteratorType inIterator(
			out_image,
			out_image->GetLargestPossibleRegion());
		inIterator.SetFirstDirection(0);  // X axis
		inIterator.SetSecondDirection(1); // Y axis
		inIterator.GoToBegin();
		size_t j{};
		const double max_minus_min = (rmax-rmin > 0) ? rmax - rmin : 1e-9;
		while (!inIterator.IsAtEnd())
		{
			while (!inIterator.IsAtEndOfSlice())
			{
				while (!inIterator.IsAtEndOfLine())
				{
					const typename T::PixelType v = inIterator.Get();
					const double f = v;
					if (texture_type == 0) // GL_R16F
					{
						float_buf[j] = static_cast<float>((f + (-rmin)) / max_minus_min);
					}
					else if(texture_type == 1) // GL_R16
					{
						short_buf[j] = static_cast<unsigned short>(USHRT_MAX * ((f + (-rmin)) / max_minus_min));
					}
					else if(texture_type == 2) // GL_R8
					{
						ub_buf[j] = static_cast<GLubyte>(UCHAR_MAX * ((f + (-rmin)) / max_minus_min));
					}
					++j;
					++inIterator;
				}
				inIterator.NextLine();
			}
			inIterator.NextSlice();
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
#ifdef ALIZA_VERBOSE
		std::cout << ex.GetDescription() << std::endl;
#else
		(void)ex;
#endif
		error__ = 4;
		goto quit__;
	}
	//
	gl->makeCurrent();
#if 0
	if (!gl->isValid())
	{
		std::cout << "gl->isValid()=" << std::endl;
	}
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	glerror__ = gl->glGetError();
#else
	glerror__ = glGetError();
#endif
#if 0
	if (glerror__ != 0)
	{
		std::cout << "warning : OpenGL error (before texture generation) "
			<< std::hex << glerror__ << std::dec << std::endl;
	}
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	gl->glGenTextures(1, &(ivariant->di->cube_3dtex));
	gl->glBindTexture(GL_TEXTURE_3D, ivariant->di->cube_3dtex);
	switch (ivariant->di->filtering)
	{
	case 1: // bilinear
		{
			gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		break;
	case 2: // trilinear
		{
			gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			gl->glGenerateMipmap(GL_TEXTURE_3D);
		}
		break;
	default: // no
		{
			gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		break;
	}
	gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl->glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#else
	glGenTextures(1, &(ivariant->di->cube_3dtex));
	glBindTexture(GL_TEXTURE_3D, ivariant->di->cube_3dtex);
	switch (ivariant->di->filtering)
	{
	case 1: // bilinear
		{
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		break;
	case 2: // trilinear
		{
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glGenerateMipmap(GL_TEXTURE_3D);
		}
		break;
	default: // no
		{
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		break;
	}
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
	//
	// GL_UNPACK_ALIGNMENT/GL_PACK_ALIGNMENT
	// 1 byte-alignment
	// 2 rows aligned to even-numbered bytes
	// 4 word-alignment
	// 8 rows start on double-word boundaries
	//
	switch (texture_type)
	{
	case 0:
		{
			ivariant->di->tex_info = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			gl->glTexImage3D(
				GL_TEXTURE_3D, 0, GL_R16F,
				size[0], size[1], size[2],
				0, GL_RED, GL_FLOAT, float_buf);
#else
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glTexImage3D(
				GL_TEXTURE_3D, 0, GL_R16F,
				size[0], size[1], size[2],
				0, GL_RED, GL_FLOAT, float_buf);
#endif
		}
		break;
	case 1:
		{
			ivariant->di->tex_info = 1;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			gl->glTexImage3D(
				GL_TEXTURE_3D, 0, GL_R16,
				size[0], size[1], size[2],
				0, GL_RED, GL_UNSIGNED_SHORT, short_buf);
#else
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glTexImage3D(
				GL_TEXTURE_3D, 0, GL_R16,
				size[0], size[1], size[2],
				0, GL_RED, GL_UNSIGNED_SHORT, short_buf);
#endif
		}
		break;
	case 2:
		{
			ivariant->di->tex_info = 2;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			gl->glPixelStorei(
				GL_UNPACK_ALIGNMENT, 1);
			gl->glTexImage3D(
				GL_TEXTURE_3D, 0, GL_R8,
				size[0], size[1], size[2],
				0, GL_RED, GL_UNSIGNED_BYTE, ub_buf);
#else
			glPixelStorei(
				GL_UNPACK_ALIGNMENT, 1);
			glTexImage3D(
				GL_TEXTURE_3D, 0, GL_R8,
				size[0], size[1], size[2],
				0, GL_RED, GL_UNSIGNED_BYTE, ub_buf);
#endif
		}
		break;
	default:
		break;
	}
	//
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	glerror__ = gl->glGetError();
#else
	glerror__ = glGetError();
#endif
	if (glerror__ == 0x505)
	{
#if 0
		std::cout << "error : OpenGL error 0x505" << std::endl;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		gl->glBindTexture(GL_TEXTURE_3D, 0);
		gl->glDeleteTextures(1, &(ivariant->di->cube_3dtex));
#else
		glBindTexture(GL_TEXTURE_3D, 0);
		glDeleteTextures(1, &(ivariant->di->cube_3dtex));
#endif
		ivariant->di->cube_3dtex = 0;
		ivariant->di->tex_info = -1;
		error__ = 3;
		goto quit__;
	}
	else if (glerror__ != 0)
	{
#ifdef ALIZA_VERBOSE
		std::cout << "warning : OpenGL error " << std::hex << glerror__
			<< std::dec << std::endl;
#endif
	}
quit__:
	delete [] float_buf;
	delete [] short_buf;
	delete [] ub_buf;
	return error__;
}

template<typename T> void calc_center_from_image(
	ImageVariant * ivariant,
	const typename T::Pointer & image)
{
	if (image.IsNull()||!ivariant) return;
	const typename T::SizeType size =
		image->GetLargestPossibleRegion().GetSize();
	const typename T::PointType origin = image->GetOrigin();
	typename T::IndexType idx;
	idx[0] = size[0];
	idx[1] = size[1];
	idx[2] = size[2];
	itk::Point<float, 3> p;
	try
	{
		image->TransformIndexToPhysicalPoint(idx, p);
	}
	catch (const itk::ExceptionObject & ex)
	{
#ifdef ALIZA_VERBOSE
		std::cout << ex.GetDescription() << std::endl;
#else
		(void)ex;
#endif
		return;
	}
	sVector3 v0 = sVector3(
		static_cast<float>(origin[0]),
		static_cast<float>(origin[1]),
		static_cast<float>(origin[2]));
	sVector3 v1 = sVector3(p[0], p[1], p[2]);
	sVector3 cube_center = sVector3((v0 + v1) * 0.5f);
	ivariant->di->default_center_x = ivariant->di->center_x = cube_center.getX();
	ivariant->di->default_center_y = ivariant->di->center_y = cube_center.getY();
	ivariant->di->default_center_z = ivariant->di->center_z = cube_center.getZ();
}

template<typename T> void read_geometry_from_image(
	ImageVariant * ivariant,
	const typename T::Pointer & image)
{
	if (!ivariant) return;
	if (image.IsNull()) return;
	const typename T::SizeType size = image->GetLargestPossibleRegion().GetSize();
	sVector3 first     = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 last      = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 direction = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 up        = sVector3(0.0f, 0.0f, 0.0f);
	const typename T::DirectionType dircos = image->GetDirection();
	const double d1 = dircos[0][0];
	const double d2 = dircos[1][0];
	const double d3 = dircos[2][0];
	const double d4 = dircos[0][1];
	const double d5 = dircos[1][1];
	const double d6 = dircos[2][1];
	for (size_t z = 0; z < size[2]; ++z)
	{
		typename T::IndexType idx0, idx1, idx2, idx3;
		itk::Point<float, 3> p0, p1, p2, p3;
		idx0[0] = 0;
		idx1[0] = 0;
		idx2[0] = size[0] - 1;
		idx3[0] = size[0] - 1;
		idx0[1] = size[1] - 1;
		idx1[1] = 0;
		idx2[1] = size[1] - 1;
		idx3[1] = 0;
		idx0[2] = z;
		idx1[2] = z;
		idx2[2] = z;
		idx3[2] = z;
		try
		{
			image->TransformIndexToPhysicalPoint(idx0, p0);
			image->TransformIndexToPhysicalPoint(idx1, p1);
			image->TransformIndexToPhysicalPoint(idx2, p2);
			image->TransformIndexToPhysicalPoint(idx3, p3);
		}
		catch (const itk::ExceptionObject & ex)
		{
#ifdef ALIZA_VERBOSE
			std::cout << ex.GetDescription() << std::endl;
#else
			(void)ex;
#endif
			continue;
		}
		float x0 = p0[0], y0 = p0[1], z0 = p0[2];
		float x1 = p1[0], y1 = p1[1], z1 = p1[2];
		float x2 = p2[0], y2 = p2[1], z2 = p2[2];
		float x3 = p3[0], y3 = p3[1], z3 = p3[2];
		if (z == 0)
		{
			first = sVector3(x0, y0, z0);
			sVector3 tmp_up0 = sVector3(x1, y1, z1);
			sVector3 tmp_up1 = sVector3(x0, y0, z0);
			up = sVector3(normalize(tmp_up1-tmp_up0));
		}
		else if (z == size[2] - 1)
		{
			last = sVector3(x0, y0, z0);
		}
		const double ipp_iop[9] = {
			static_cast<double>(x1),
			static_cast<double>(y1),
			static_cast<double>(z1),
			d1, d2, d3, d4, d5, d6 };
		CommonUtils::generate_cubeslice(
			ivariant->di->image_slices,
			QString(""),
			size[2], z,
			x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3,
			ipp_iop);
	}
	direction = sVector3(normalize(last - first));
	ivariant->di->slices_direction_x = direction.getX();
	ivariant->di->slices_direction_y = direction.getY();
	ivariant->di->slices_direction_z = direction.getZ();
	ivariant->di->up_direction_x = up.getX();
	ivariant->di->up_direction_y = up.getY();
	ivariant->di->up_direction_z = up.getZ();
	calc_center_from_image<T>(ivariant, image);
	ivariant->equi = true;
	ivariant->di->slices_generated = true;
}

template <typename T> bool reload_monochrome_image(
	ImageVariant * ivariant,
	const typename T::Pointer & image,
	GLWidget * gl,
	int max_3d_tex_size,
	QProgressDialog * pb,
	bool resize = false,
	size_t size_x_ = 0,
	size_t size_y_ = 0,
	bool disable_gen_slices = false)
{
	if (!ivariant || image.IsNull()) return false;
	const bool generate_slices = (!disable_gen_slices && !ivariant->di->slices_generated);
	const bool calc_center = (!disable_gen_slices && !ivariant->di->slices_generated);
	get_dimensions<T>(image,
		&(ivariant->di->idimx),
		&(ivariant->di->idimy),
		&(ivariant->di->idimz),
		&(ivariant->di->ix_spacing),
		&(ivariant->di->iy_spacing),
		&(ivariant->di->iz_spacing),
		&(ivariant->di->ix_origin),
		&(ivariant->di->iy_origin),
		&(ivariant->di->iz_origin));
	size_t size_x, size_y, size_z;
	size_t isize[3];
	double dspacing[3];
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::SpacingType spacing;
	short count__{};
	double fx{};
	double fy{};
	double fz{};
	ivariant->di->close(generate_slices);
	region  = image->GetLargestPossibleRegion();
	size    = region.GetSize();
	spacing = image->GetSpacing();
	//
	if (max_3d_tex_size > 0 && max_3d_tex_size < static_cast<int>(size[2]))
	{
#ifdef ALIZA_VERBOSE
		std::cout << "Warning: can not use 3D texture, Z dim = "
			<< static_cast<int>(size[2])
			<< ", 3D max texture size = " << max_3d_tex_size
			<< std::endl;
#endif
		ivariant->di->skip_texture = true;
	}
	if (size[0] == 1 || size[1] == 1)
	{
		ivariant->di->skip_texture = true;
	}
	const bool ok3d =
		(max_3d_tex_size > 0 && !ivariant->di->skip_texture && ivariant->di->opengl_ok && gl);
	//
	if (generate_slices)
	{
		read_geometry_from_image<T>(ivariant, image);
	}
	if (calc_center)
	{
		calc_center_from_image<T>(ivariant, image);
	}
	//
	if (resize)
	{
		size_x = (size_x_<size[0]) ? size_x_ : size[0];
		size_y = (size_y_<size[1]) ? size_y_ : size[1];
	}
	else
	{
		size_x = size[0];
		size_y = size[1];
	}
	size_z = size[2];
	if (size_x > static_cast<size_t>(max_3d_tex_size)) size_x = max_3d_tex_size;
	if (size_y > static_cast<size_t>(max_3d_tex_size)) size_y = max_3d_tex_size;
	if (size_z > static_cast<size_t>(max_3d_tex_size)) size_z = max_3d_tex_size;
	fx = size_x/static_cast<double>(size[0]);
	size[0] *= fx;
	spacing[0] *= 1.0 / fx;
	fy = size_y/static_cast<double>(size[1]);
	size[1] *= fy;
	spacing[1] *= 1.0 / fy;
	fz = size_z/static_cast<double>(size[2]);
	size[2] *= fz;
	spacing[2] *= 1.0 / fz;
	isize[0] = size[0];
	isize[1] = size[1];
	isize[2] = size[2];
	dspacing[0] = static_cast<double>(spacing[0]);
	dspacing[1] = static_cast<double>(spacing[1]);
	dspacing[2] = static_cast<double>(spacing[2]);
	//
	bool ok = false;
	if (ok3d)
	{
		while (!ok)
		{
			++count__;
			int error__ = generate_tex3d<T>(
				ivariant,
				image,
				isize,
				dspacing,
				pb,
				gl);
			if (error__ == 0)
			{
				ok = true;
			}
			else
			{
#ifdef ALIZA_VERBOSE
				if (error__ == 2)
				{
					std::cout << "memory error (system)    "
							"... reducing texture size" << std::endl;
				}
				else if (error__ == 3)
				{
					std::cout << "memory error (graphics)  "
							"... reducing texture size" << std::endl;
				}
				else
				{
					std::cout << "error " << error__ << std::endl;
				}
#endif
				isize[0] *= 0.5;
				isize[1] *= 0.5;
				dspacing[0] *= 2.0;
				dspacing[1] *= 2.0;
				ivariant->di->close(generate_slices);
				if (count__ > 64)
				{
#ifdef ALIZA_VERBOSE
					std::cout
						<< "exit from loop after "
						<< count__
						<< " iterations, x = "
						<< isize[0] << ", y = "
						<< isize[1] << std::endl;
#endif
					break;
				}
			}
		}
	}
	else
	{
		calculate_min_max<T>(image, ivariant);
		ok = true;
	}
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	if (ivariant->equi && ivariant->orientation > 0 && !ivariant->orientation_string.isEmpty())
	{
		ivariant->di->origin[0] = ivariant->di->ix_origin;
		ivariant->di->origin[1] = ivariant->di->iy_origin;
		ivariant->di->origin[2] = ivariant->di->iz_origin;
		ivariant->di->origin_ok = true;
	}
	return ok;
}

template<typename T> bool reload_rgb_image(
	const typename T::Pointer & image,
	ImageVariant * ivariant,
	bool disable_gen_slices = false)
{
	if (!ivariant || image.IsNull()) return false;
	ivariant->di->skip_texture = true;
	const bool generate_slices = (!disable_gen_slices && !ivariant->di->slices_generated);
	const bool calc_center = (!disable_gen_slices && !ivariant->di->slices_generated);
	get_dimensions<T>(
		image,
		&(ivariant->di->idimx),
		&(ivariant->di->idimy),
		&(ivariant->di->idimz),
		&(ivariant->di->ix_spacing),
		&(ivariant->di->iy_spacing),
		&(ivariant->di->iz_spacing),
		&(ivariant->di->ix_origin),
		&(ivariant->di->iy_origin),
		&(ivariant->di->iz_origin));
	//
	if (generate_slices)
		read_geometry_from_image<T>(ivariant, image);
	if (calc_center)
		calc_center_from_image<T>(ivariant, image);
	//
	calculate_rgb_minmax_<T>(image, ivariant);
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	return true;
}

template<typename T> bool reload_rgba_image(
	const typename T::Pointer & image,
	ImageVariant * ivariant,
	bool disable_gen_slices = false)
{
	if (!ivariant||image.IsNull()) return false;
	ivariant->di->skip_texture = true;
	const bool generate_slices = (!disable_gen_slices && !ivariant->di->slices_generated);
	const bool calc_center = (!disable_gen_slices && !ivariant->di->slices_generated);
	get_dimensions<T>(
		image,
		&(ivariant->di->idimx),
		&(ivariant->di->idimy),
		&(ivariant->di->idimz),
		&(ivariant->di->ix_spacing),
		&(ivariant->di->iy_spacing),
		&(ivariant->di->iz_spacing),
		&(ivariant->di->ix_origin),
		&(ivariant->di->iy_origin),
		&(ivariant->di->iz_origin));
	//
	if (generate_slices)
		read_geometry_from_image<T>(ivariant, image);
	if (calc_center)
		calc_center_from_image<T>(ivariant, image);
	//
	calculate_rgba_minmax_<T>(image, ivariant);
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	return true;
}

QString print_idx_z_error(size_t idx_z)
{
	return (QString("Error, data at ") +
		QVariant(static_cast<unsigned long long>(idx_z)).toString() +
		QString(" is null"));
}

template<typename T> QString process_dicom_monochrome_image1(
	bool * ok,
	ImageVariant * ivariant,
	typename T::Pointer & image,
	const std::vector<char *> & data,
	const itk::Matrix<itk::SpacePrecisionType, 3, 3> & direction,
	const size_t dimx, const size_t dimy, const size_t dimz,
	const double origin_x, const double origin_y, const double origin_z,
	const double spacing_x, const double spacing_y, const double spacing_z,
	const short image_type,
	bool * bad_direction)
{
	if (data.size() != dimz)
	{
		*ok = false;
		return QString("Error: data size mismatch");
	}
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::IndexType start;
	typename T::PointType origin;
	typename T::SpacingType spacing;
	typedef itk::ImageSliceIteratorWithIndex<T> SliceIterator;
	typename UpdateQtCommand::Pointer update_qt_command;
	start.Fill(0);
	size[0] = dimx;
	size[1] = dimy;
	size[2] = dimz;
	region.SetIndex(start);
	region.SetSize(size);
	origin[0] = origin_x;
	origin[1] = origin_y;
	origin[2] = origin_z;
	spacing[0] = spacing_x;
	spacing[1] = spacing_y;
	spacing[2] = spacing_z;
	*bad_direction = check_direction_matrix(direction);
	try
	{
		image = T::New();
		image->SetRegions(region);
		image->Allocate();
		image->SetOrigin(origin);
		image->SetSpacing(spacing);
		if (*bad_direction == false) image->SetDirection(direction);
	}
	catch (const itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	ivariant->image_type = image_type;
	//
	try
	{
		size_t idx_z{};
		size_t j{};
		SliceIterator it(image, image->GetLargestPossibleRegion());
		it.SetFirstDirection(0);
		it.SetSecondDirection(1);
		it.GoToBegin();
		while (!it.IsAtEnd())
		{
			const void * vbuffer = static_cast<void*>(data.at(idx_z));
#if 1
			if (vbuffer == nullptr)
			{
				*ok = false;
				return print_idx_z_error(idx_z);
			}
#endif
			while (!it.IsAtEndOfSlice())
			{
				const typename T::PixelType * p__ = static_cast<const typename T::PixelType*>(vbuffer);
				while (!it.IsAtEndOfLine())
				{
					it.Set(p__[j]);
					++j;
					++it;
				}
				it.NextLine();
			}
			j = 0;
			++idx_z;
			it.NextSlice();
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	return QString("");
}

template<typename T> void process_dicom_monochrome_image2(
	ImageVariant * ivariant,
	typename T::Pointer & image,
	const bool gen_vertices,
	const bool bad_direction)
{
	const bool generate_slices = (gen_vertices && !ivariant->di->slices_generated);
	get_dimensions<T>(image,
		&(ivariant->di->idimx),
		&(ivariant->di->idimy),
		&(ivariant->di->idimz),
		&(ivariant->di->ix_spacing),
		&(ivariant->di->iy_spacing),
		&(ivariant->di->iz_spacing),
		&(ivariant->di->ix_origin),
		&(ivariant->di->iy_origin),
		&(ivariant->di->iz_origin));
	if (generate_slices)
	{
		read_geometry_from_image<T>(ivariant, image);
	}
	calculate_min_max<T>(image, ivariant);
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	if (bad_direction)
	{
		ivariant->equi = false;
		ivariant->orientation_string = QString("");
		ivariant->orientation = 0;
		for (size_t x = 0; x < ivariant->di->image_slices.size(); ++x)
		{
			 ivariant->di->image_slices[x]->slice_orientation_string = QString("");
		}
	}
}

template<typename T> QString process_dicom_rgb_image1(
	bool * ok,
	ImageVariant * ivariant,
	typename T::Pointer & image,
	const std::vector<char *> & data,
	const itk::Matrix<itk::SpacePrecisionType, 3, 3> & direction,
	const size_t dimx, const size_t dimy, const size_t dimz,
	const double origin_x, const double origin_y, const double origin_z,
	const double spacing_x, const double spacing_y, const double spacing_z,
	const short image_type,
	const short ybr, // 0 - no, 1 - full, 2 - partial
	const bool hsv,
	const int bitsstored,
	bool * bad_direction)
{
	if (data.size() != dimz)
	{
		*ok = false;
		return QString("Error: data size mismatch");
	}
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::IndexType start;
	typename T::PointType origin;
	typename T::SpacingType spacing;
	typedef itk::ImageSliceIteratorWithIndex<T> SliceIterator;
	start.Fill(0);
	size[0] = dimx;
	size[1] = dimy;
	size[2] = dimz;
	region.SetIndex(start);
	region.SetSize(size);
	origin[0] = origin_x;
	origin[1] = origin_y;
	origin[2] = origin_z;
	spacing[0] = spacing_x;
	spacing[1] = spacing_y;
	spacing[2] = spacing_z;
	*bad_direction = check_direction_matrix(direction);
	try
	{
		image = T::New();
		image->SetRegions(region);
		image->Allocate();
		image->SetOrigin(origin);
		image->SetSpacing(spacing);
		if (*bad_direction == false) image->SetDirection(direction);
	}
	catch (const itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	ivariant->image_type = image_type;
	//
	try
	{
		size_t idx_z{};
		size_t j{};
		SliceIterator it(image, image->GetLargestPossibleRegion());
		it.SetFirstDirection(0);
		it.SetSecondDirection(1);
		it.GoToBegin();
		while (!it.IsAtEnd())
		{
			const void * vbuffer = static_cast<void*>(data.at(idx_z));
#if 1
			if (vbuffer == nullptr)
			{
				*ok = false;
				return print_idx_z_error(idx_z);
			}
#endif
			const typename T::PixelType::ValueType * p__ =
				static_cast<const typename T::PixelType::ValueType*>(vbuffer);
			while (!it.IsAtEndOfSlice())
			{
				while (!it.IsAtEndOfLine())
				{
					typename T::PixelType p;
					if (ybr > 0)
					{
						// 8 bits
						int R, G, B;
						double Y  = p__[j];
						const double Cb = p__[j + 1] - 128;
						const double Cr = p__[j + 2] - 128;
						if (ybr == 1)
						{
							R = static_cast<int>(Y + (-0.000036820) * Cb +    1.401987577 * Cr);
							G = static_cast<int>(Y + (-0.344113281) * Cb + (-0.714103821) * Cr);
							B = static_cast<int>(Y +    1.771978117 * Cb + (-0.000134583) * Cr);
						}
						else if (ybr == 2)
						{
							// YBR_PARTIAL_422 is problematic
							Y -= 16;
							if (Y < 0) Y = 0; // invalid?
							R = static_cast<int>(1.164415463 * Y + (-0.000095036) * Cb +    1.596001878 * Cr);
							G = static_cast<int>(1.164415463 * Y + (-0.391724564) * Cb + (-0.813013368) * Cr);
							B = static_cast<int>(1.164415463 * Y +    2.017290682 * Cb + (-0.000135273) * Cr);
						}
						else
						{
							R = G = B = 0;
							*ok = false;
							return QString("Internal error");
						}
						if (R > 255) R = 255;
						if (G > 255) G = 255;
						if (B > 255) B = 255;
						p[0] = static_cast<typename T::PixelType::ValueType>(R < 0 ? 0 : R);
						p[1] = static_cast<typename T::PixelType::ValueType>(G < 0 ? 0 : G);
						p[2] = static_cast<typename T::PixelType::ValueType>(B < 0 ? 0 : B);
					}
					else if (hsv)
					{
						const double H = (p__[j] / 255.0) * 360.0;
						const double S = p__[j + 1]/ 255.0;
						const double V = p__[j + 2] / 255.0;
						double R, G, B;
						ColorSpace_::Hsv2Rgb(&R, &G, &B, H, S, V);
						p[0] = static_cast<typename T::PixelType::ValueType>(R * 255.0);
						p[1] = static_cast<typename T::PixelType::ValueType>(G * 255.0);
						p[2] = static_cast<typename T::PixelType::ValueType>(B * 255.0);
					}
					else
					{
						p[0] = static_cast<typename T::PixelType::ValueType>(p__[j]);
						p[1] = static_cast<typename T::PixelType::ValueType>(p__[j + 1]);
						p[2] = static_cast<typename T::PixelType::ValueType>(p__[j + 2]);
					}
					it.Set(p);
					j += 3;
					++it;
				}
				it.NextLine();
			}
			j = 0;
			++idx_z;
			it.NextSlice();
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	return QString("");
}

template<typename T> void process_dicom_rgb_image2(
	ImageVariant * ivariant,
	typename T::Pointer & image,
	const bool gen_vertices,
	const bool bad_direction)
{
	ivariant->di->skip_texture = true;
	const bool generate_slices = (gen_vertices && !ivariant->di->slices_generated);
	get_dimensions<T>(
		image,
		&(ivariant->di->idimx),
		&(ivariant->di->idimy),
		&(ivariant->di->idimz),
		&(ivariant->di->ix_spacing),
		&(ivariant->di->iy_spacing),
		&(ivariant->di->iz_spacing),
		&(ivariant->di->ix_origin),
		&(ivariant->di->iy_origin),
		&(ivariant->di->iz_origin));
	if (generate_slices)
	{
		read_geometry_from_image<T>(ivariant, image);
	}
	calculate_rgb_minmax_<T>(image, ivariant);
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	if (bad_direction)
	{
		ivariant->equi = false;
		ivariant->orientation_string = QString("");
		ivariant->orientation = 0;
		for (size_t x = 0; x < ivariant->di->image_slices.size(); ++x)
		{
			 ivariant->di->image_slices[x]->slice_orientation_string = QString("");
		}
	}
}

template<typename T> QString process_dicom_rgba_image1(
	bool * ok,
	ImageVariant * ivariant,
	typename T::Pointer & image,
	const std::vector<char *> & data,
	const itk::Matrix<itk::SpacePrecisionType, 3, 3> & direction,
	const size_t dimx, const size_t dimy, const size_t dimz,
	const double origin_x, const double origin_y, const double origin_z,
	const double spacing_x, const double spacing_y, const double spacing_z,
	const short image_type,
	const bool cmyk,
	const bool argb,
	bool * bad_direction)
{
	if (data.size() != dimz)
	{
		*ok = false;
		return QString("Error: data size mismatch");
	}
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::IndexType start;
	typename T::PointType origin;
	typename T::SpacingType spacing;
	typedef itk::ImageSliceIteratorWithIndex<T> SliceIterator;
	start.Fill(0);
	size[0] = dimx;
	size[1] = dimy;
	size[2] = dimz;
	region.SetIndex(start);
	region.SetSize(size);
	origin[0] = origin_x;
	origin[1] = origin_y;
	origin[2] = origin_z;
	spacing[0] = spacing_x;
	spacing[1] = spacing_y;
	spacing[2] = spacing_z;
	*bad_direction = check_direction_matrix(direction);
	try
	{
		image = T::New();
		image->SetRegions(region);
		image->Allocate();
		image->SetOrigin(origin);
		image->SetSpacing(spacing);
		if (*bad_direction == false) image->SetDirection(direction);
	}
	catch (const itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	ivariant->image_type = image_type;
	//
	try
	{
		size_t idx_z{};
		size_t j{};
		SliceIterator it(image, image->GetLargestPossibleRegion());
		it.SetFirstDirection(0);
		it.SetSecondDirection(1);
		it.GoToBegin();
		while (!it.IsAtEnd())
		{
			const typename T::PixelType::ValueType * p__ =
				reinterpret_cast<typename T::PixelType::ValueType*>(data.at(idx_z));
#if 1
			if (p__ == nullptr)
			{
				*ok = false;
				return print_idx_z_error(idx_z);
			}
#endif
			while (!it.IsAtEndOfSlice())
			{
				while (!it.IsAtEndOfLine())
				{
					typename T::PixelType p;
					if (cmyk)
					{
						// CMYK (Retired)
						// Pixel data represent a color image described by cyan, magenta,
						// yellow, and black image  planes. The minimum sample value for
						// each CMYK plane represents a minimum intensity of the color.
						// This value may be used only when Samples per Pixel (0028,0002)
						// has a value of 4.
						const float C = static_cast<float>(p__[j]);
						const float M = static_cast<float>(p__[j + 1]);
						const float Y = static_cast<float>(p__[j + 2]);
						const float K = static_cast<float>(p__[j + 3]);
#if 1
						p[0] = static_cast<typename T::PixelType::ValueType>((C * K) / 255.0f);
						p[1] = static_cast<typename T::PixelType::ValueType>((M * K) / 255.0f);
						p[2] = static_cast<typename T::PixelType::ValueType>((Y * K) / 255.0f);
						p[3] = 255;
#else
						if (p__[j+3] != 255)
						{
							p[0] = static_cast<typename T::PixelType::ValueType>(((255.0f - C) * (255.0f - K)) / 255.0f);
							p[1] = static_cast<typename T::PixelType::ValueType>(((255.0f - M) * (255.0f - K)) / 255.0f);
							p[2] = static_cast<typename T::PixelType::ValueType>(((255.0f - Y) * (255.0f - K)) / 255.0f);
							p[3] = 255;
						}
						else
						{
							p[0] = static_cast<typename T::PixelType::ValueType>(255.0f - C);
							p[1] = static_cast<typename T::PixelType::ValueType>(255.0f - M);
							p[2] = static_cast<typename T::PixelType::ValueType>(255.0f - Y);
							p[3] = 255;
						}
#endif
					}
					else if (argb)
					{
						// ARGB (Retired)
						// Pixel data represent a color image
						// described by red, green, blue, and alpha
						// image planes. The minimum sample value for each
						// RGB plane represents minimum intensity of the
						// color. The alpha plane is passed through
						// Palette Color Lookup Tables. If the alpha pixel
						// value is greater than 0, the red, green, and blue
						// lookup table values override the red, green, and
						// blue, pixel plane colors.
						//
						// FIXME
						const float tmp_max = 255.0f;
						const float alpha = static_cast<float>(p__[j + 3]) / tmp_max;
						const float one_minus_alpha = 1.0f - alpha;
						const float tmp_oth = one_minus_alpha * 0.0f;
						const float tmp_red = tmp_oth + alpha * static_cast<float>(p__[j]);
						const float tmp_gre = tmp_oth + alpha * static_cast<float>(p__[j + 1]);
						const float tmp_blu = tmp_oth + alpha * static_cast<float>(p__[j + 2]);
						p[0] = static_cast<typename T::PixelType::ValueType>((tmp_red / tmp_max) * 255.0f);
						p[1] = static_cast<typename T::PixelType::ValueType>((tmp_gre / tmp_max) * 255.0f);
						p[2] = static_cast<typename T::PixelType::ValueType>((tmp_blu / tmp_max) * 255.0f);
						p[3] = 255;
					}
					else
					{
						p[0] = static_cast<typename T::PixelType::ValueType>(p__[j]);
						p[1] = static_cast<typename T::PixelType::ValueType>(p__[j + 1]);
						p[2] = static_cast<typename T::PixelType::ValueType>(p__[j + 2]);
						p[3] = static_cast<typename T::PixelType::ValueType>(p__[j + 3]);
					}
					it.Set(p);
					j += 4;
					++it;
				}
				it.NextLine();
			}
			j = 0;
			++idx_z;
			it.NextSlice();
		}
	}
	catch (const itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	return QString("");
}

template<typename T> void process_dicom_rgba_image2(
	ImageVariant * ivariant,
	typename T::Pointer & image,
	const bool gen_vertices,
	const bool bad_direction)
{
	ivariant->di->skip_texture = true;
	const bool generate_slices = (gen_vertices && !ivariant->di->slices_generated);
	get_dimensions<T>(
		image,
		&(ivariant->di->idimx),
		&(ivariant->di->idimy),
		&(ivariant->di->idimz),
		&(ivariant->di->ix_spacing),
		&(ivariant->di->iy_spacing),
		&(ivariant->di->iz_spacing),
		&(ivariant->di->ix_origin),
		&(ivariant->di->iy_origin),
		&(ivariant->di->iz_origin));
	if (generate_slices)
	{
		read_geometry_from_image<T>(ivariant, image);
	}
	calculate_rgba_minmax_<T>(image, ivariant);
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	if (bad_direction)
	{
		ivariant->equi = false;
		ivariant->orientation_string = QString("");
		ivariant->orientation = 0;
		for (size_t x = 0; x < ivariant->di->image_slices.size(); ++x)
		{
			 ivariant->di->image_slices[x]->slice_orientation_string = QString("");
		}
	}
}

template <typename Tin, typename Tout> QString apply_per_slice_rescale_(
	typename Tin::Pointer & image,
	typename Tout::Pointer & out_image,
	const QList< QPair<double, double> > rescale_values)
{
	if (image.IsNull()) return QString("image.IsNull()");
	typename Tin::SizeType size = image->GetLargestPossibleRegion().GetSize();
	const size_t size_x = size[0];
	const size_t size_y = size[1];
	const size_t size_z = size[2];
	if (size_z != static_cast<size_t>(rescale_values.size()))
		return QString("size_z != rescale_values.size()");
	try
	{
		out_image = Tout::New();
		out_image->SetRegions(
			static_cast<typename Tout::RegionType>(image->GetLargestPossibleRegion()));
		out_image->SetOrigin(
			static_cast<typename Tout::PointType>(image->GetOrigin()));
		out_image->SetSpacing(
			static_cast<typename Tout::SpacingType>(image->GetSpacing()));
		out_image->SetDirection(
			static_cast<typename Tout::DirectionType>(image->GetDirection()));
		out_image->Allocate();
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	for (size_t x = 0; x < size_z; ++x)
	{
		typename Tin::IndexType index;
		index[0] = 0;
		index[1] = 0;
		index[2] = x;
		typename Tin::SizeType size_;
		size_[0] = size_x;
		size_[1] = size_y;
		size_[2] = 1;
		typename Tin::RegionType region;
		try
		{
			region.SetIndex(index);
			region.SetSize(size_);
			itk::ImageRegionConstIterator<Tin> it0(image, region);
			it0.GoToBegin();
			itk::ImageRegionIterator<Tout> it1(out_image, region);
			it1.GoToBegin();
			while (!it0.IsAtEnd())
			{
				const typename Tin::PixelType v0 = it0.Get();
				const typename Tout::PixelType v1 =
					static_cast<typename Tout::PixelType>(
						v0 * rescale_values.at(x).second + rescale_values.at(x).first);
				it1.Set(v1);
				++it0;
				++it1;
			}
		}
		catch (const itk::ExceptionObject & ex)
		{
			return QString(ex.GetDescription());
		}
	}
	image->DisconnectPipeline();
	image = nullptr;
	return QString("");
}

template<typename T> double get_value(
	const typename T::Pointer & image,
	const int x,
	const int y,
	const int z)
{
	if (image.IsNull()) return 0.0;
	typename T::IndexType idx;
	idx[0] = x;
	idx[1] = y;
	idx[2] = z;
	double r{};
	try
	{
		r = static_cast<double>(image->GetPixel(idx));
	}
	catch (const itk::ExceptionObject &)
	{
		;
	}
	return r;
}

}

int CommonUtils::get_next_id()
{
	static std::atomic<int> id___{};
	++id___;
	return id___;
}

int CommonUtils::get_next_group_id()
{
	static std::atomic<int> group_id___{};
	++group_id___;
	return group_id___;
}

double CommonUtils::random_range(
	double lo, double hi, unsigned long long seed)
{
	auto f = std::bind(
		std::uniform_real_distribution<double>(lo, hi),
		std::mt19937_64(seed));
	return f();
}

QString CommonUtils::convert_orientation_flag(unsigned int in)
{
	switch (in)
	{
#if (((ITK_VERSION_MAJOR == 5 && ITK_VERSION_MINOR >= 3) || ITK_VERSION_MAJOR > 5) && defined TMP_USE_53_SPATIAL_ENUMS)
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RIP):
		return QString("RIP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LIP):
		return QString("LIP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RSP):
		return QString("RSP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LSP):
		return QString("LSP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RIA):
		return QString("RIA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LIA):
		return QString("LIA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RSA):
		return QString("RSA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LSA):
		return QString("LSA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_IRP):
		return QString("IRP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ILP):
		return QString("ILP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SRP):
		return QString("SRP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SLP):
		return QString("SLP");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_IRA):
		return QString("IRA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ILA):
		return QString("ILA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SRA):
		return QString("SRA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SLA):
		return QString("SLA");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RPI):
		return QString("RPI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LPI):
		return QString("LPI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RAI):
		return QString("RAI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LAI):
		return QString("LAI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RPS):
		return QString("RPS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LPS):
		return QString("LPS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RAS):
		return QString("RAS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_LAS):
		return QString("LAS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PRI):
		return QString("PRI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PLI):
		return QString("PLI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ARI):
		return QString("ARI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ALI):
		return QString("ALI");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PRS):
		return QString("PRS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PLS):
		return QString("PLS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ARS):
		return QString("ARS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ALS):
		return QString("ALS");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_IPR):
		return QString("IPR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SPR):
		return QString("SPR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_IAR):
		return QString("IAR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SAR):
		return QString("SAR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_IPL):
		return QString("IPL");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SPL):
		return QString("SPL");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_IAL):
		return QString("IAL");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_SAL):
		return QString("SAL");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PIR):
		return QString("PIR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PSR):
		return QString("PSR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_AIR):
		return QString("AIR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ASR):
		return QString("ASR");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PIL):
		return QString("PIL");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_PSL):
		return QString("PSL");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_AIL):
		return QString("AIL");
	case static_cast<unsigned int>(itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_ASL):
		return QString("ASL");
#else
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIP):
		return QString("RIP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIP):
		return QString("LIP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSP):
		return QString("RSP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSP):
		return QString("LSP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIA):
		return QString("RIA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA):
		return QString("LIA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSA):
		return QString("RSA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSA):
		return QString("LSA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRP):
		return QString("IRP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILP):
		return QString("ILP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRP):
		return QString("SRP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLP):
		return QString("SLP");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRA):
		return QString("IRA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILA):
		return QString("ILA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRA):
		return QString("SRA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLA):
		return QString("SLA");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI):
		return QString("RPI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPI):
		return QString("LPI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI):
		return QString("RAI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAI):
		return QString("LAI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPS):
		return QString("RPS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPS):
		return QString("LPS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS):
		return QString("RAS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS):
		return QString("LAS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRI):
		return QString("PRI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLI):
		return QString("PLI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARI):
		return QString("ARI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALI):
		return QString("ALI");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRS):
		return QString("PRS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLS):
		return QString("PLS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARS):
		return QString("ARS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALS):
		return QString("ALS");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPR):
		return QString("IPR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPR):
		return QString("SPR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAR):
		return QString("IAR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAR):
		return QString("SAR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPL):
		return QString("IPL");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPL):
		return QString("SPL");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAL):
		return QString("IAL");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAL):
		return QString("SAL");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIR):
		return QString("PIR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSR):
		return QString("PSR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIR):
		return QString("AIR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASR):
		return QString("ASR");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIL):
		return QString("PIL");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSL):
		return QString("PSL");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIL):
		return QString("AIL");
	case static_cast<unsigned int>(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASL):
		return QString("ASL");
#endif
	default:
		break;
	}
	return QString("");
}

// ITK is going to make a breaking change in spatial adapter, orientation enums
// are poorly documented and there are problems with understanding related
// to "from" and "towards" notation. So we shall stay clear and add both notations:
// "RAI-Code" and "LPS-Code" and always additionally a clear explantation "from ... to ...".
QString CommonUtils::convert_orientation_flag_rai_to_lps(const QString & rai)
{
	if (rai.length() < 3)
	{
#if 0
		std::cout << "Invalid RAI: " << rai.toStdString() << std::endl;
#endif
		return QString("");
	}
	const QChar rai0 = rai[0];
	const QChar rai1 = rai[1];
	const QChar rai2 = rai[2];
	QString lps("");
	if (rai0 == QChar('L'))
	{
		lps.append(QChar('R'));
	}
	else if (rai0 == QChar('R'))
	{
		lps.append(QChar('L'));
	}
	else if (rai0 == QChar('I'))
	{
		lps.append(QChar('S'));
	}
	else if (rai0 == QChar('S'))
	{
		lps.append(QChar('I'));
	}
	else if (rai0 == QChar('A'))
	{
		lps.append(QChar('P'));
	}
	else if (rai0 == QChar('P'))
	{
		lps.append(QChar('A'));
	}
	if (rai1 == QChar('L'))
	{
		lps.append(QChar('R'));
	}
	else if (rai1 == QChar('R'))
	{
		lps.append(QChar('L'));
	}
	else if (rai1 == QChar('I'))
	{
		lps.append(QChar('S'));
	}
	else if (rai1 == QChar('S'))
	{
		lps.append(QChar('I'));
	}
	else if (rai1 == QChar('A'))
	{
		lps.append(QChar('P'));
	}
	else if (rai1 == QChar('P'))
	{
		lps.append(QChar('A'));
	}
	if (rai2 == QChar('L'))
	{
		lps.append(QChar('R'));
	}
	else if (rai2 == QChar('R'))
	{
		lps.append(QChar('L'));
	}
	else if (rai2 == QChar('I'))
	{
		lps.append(QChar('S'));
	}
	else if (rai2 == QChar('S'))
	{
		lps.append(QChar('I'));
	}
	else if (rai2 == QChar('A'))
	{
		lps.append(QChar('P'));
	}
	else if (rai2 == QChar('P'))
	{
		lps.append(QChar('A'));
	}
	if (lps.length() == 3)
	{
		return lps;
	}
#if 0
	else
	{
		std::cout << "Invalid LPS: " << lps.toStdString() << std::endl;
	}
#endif
	return QString("");
}

double CommonUtils::set_digits(double i, int digits)
{
	const double t = pow(10.0, static_cast<double>(digits));
	return floor(i * t) / t;
}

QString CommonUtils::get_orientation2(const double * pat_orientation)
{
	const char RAI_codes[3][2] = { {'R', 'L'}, {'A', 'P'}, {'I', 'S'} };
	char rai[4]{};
	const double row_dircos_x = pat_orientation[0];
	const double row_dircos_y = pat_orientation[1];
	const double row_dircos_z = pat_orientation[2];
	const double col_dircos_x = pat_orientation[3];
	const double col_dircos_y = pat_orientation[4];
	const double col_dircos_z = pat_orientation[5];
	const double nrm_dircos_x = row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y;
	const double nrm_dircos_y = row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const double nrm_dircos_z = row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	bool oblique{};
	for (unsigned int i = 0; i < 3; ++i)
	{
		double dcos[3]{};
		double dabsmax{};
		switch (i)
		{
		case 0:
			dcos[0] = row_dircos_x;
			dcos[1] = row_dircos_y;
			dcos[2] = row_dircos_z;
			dabsmax = abs_max(row_dircos_x, row_dircos_y, row_dircos_z);
			break;
		case 1:
			dcos[0] = col_dircos_x;
			dcos[1] = col_dircos_y;
			dcos[2] = col_dircos_z;
			dabsmax = abs_max(col_dircos_x, col_dircos_y, col_dircos_z);
			break;
		case 2:
			dcos[0] = nrm_dircos_x;
			dcos[1] = nrm_dircos_y;
			dcos[2] = nrm_dircos_z;
			dabsmax = abs_max(nrm_dircos_x, nrm_dircos_y, nrm_dircos_z);
			break;
		}
		for(unsigned int j = 0; j < 3; ++j)
		{
			double dabs = fabs(dcos[j]);
			unsigned int dsgn = dcos[j] > 0 ? 0 : 1;
			if(dabs == 1.0)
			{
				rai[i] = RAI_codes[j][dsgn];
			}
			else if(dabs == dabsmax)
			{
				oblique = true;
				rai[i] = RAI_codes[j][dsgn];
			}
		}
	}
#ifdef ALIZA_VERBOSE
	if (oblique)
	{
		std::cout << "Oblique, closest to " << rai << std::endl;
	}
#else
	(void)oblique;
#endif
	QString s = QString::fromLatin1(rai);
	s.remove(QChar('\0'));
	return s;
}

void CommonUtils::get_orientation3(
	char * orientation,
	float x,
	float y,
	float z)
{
	char orientationX = x < 0.0f ? 'R' : 'L';
	char orientationY = y < 0.0f ? 'A' : 'P';
	char orientationZ = z < 0.0f ? 'F' : 'H';
	double absX = fabs(x), absY = fabs(y), absZ = fabs(z);
	for (int i = 0; i < 3; ++i)
	{
		if (absX > 0.0001 && absX > absY && absX > absZ)
		{
			*orientation++ = orientationX;
			absX = 0;
		}
		else if (absY > 0.0001 && absY>absX && absY > absZ)
		{
			*orientation++ = orientationY;
			absY = 0;
		}
		else if (absZ > 0.0001 && absZ > absX && absZ > absY)
		{
			*orientation++ = orientationZ;
			absZ = 0;
		}
		else
		{
			break;
		}
	}
}

void CommonUtils::calculate_center_notuniform(
	const std::vector<ImageSlice*> & slices,
	float * center_x, float * center_y, float * center_z)
{
	size_t j{};
	double tmpx{};
	double tmpy{};
	double tmpz{};
	for (size_t k = 0; k < slices.size(); ++k)
	{
		const ImageSlice * cs = slices.at(k);
		for (size_t z = 0; z <= 9; z += 3)
		{
			++j;
			tmpx += cs->fv[z];
			tmpy += cs->fv[z + 1];
			tmpz += cs->fv[z + 2];
		}
	}
	if (j > 0)
	{
		*center_x = static_cast<float>(tmpx / j);
		*center_y = static_cast<float>(tmpy / j);
		*center_z = static_cast<float>(tmpz / j);
	}
}

void CommonUtils::calculate_center_notuniform(
	const std::vector<SpectroscopySlice*> & slices,
	float * center_x, float * center_y, float * center_z)
{
	size_t j{};
	double tmpx{};
	double tmpy{};
	double tmpz{};
	for (size_t k = 0; k < slices.size(); ++k)
	{
		const SpectroscopySlice * cs = slices.at(k);
		for (size_t z = 0; z <= 9; z += 3)
		{
			++j;
			tmpx += cs->fv[z];
			tmpy += cs->fv[z + 1];
			tmpz += cs->fv[z + 2];
		}
	}
	if (j > 0)
	{
		*center_x = static_cast<float>(tmpx / j);
		*center_y = static_cast<float>(tmpy / j);
		*center_z = static_cast<float>(tmpz / j);
	}
}

void CommonUtils::generate_cubeslice(
			std::vector<ImageSlice*> & slices,
			const QString & orient,
			const unsigned int dimz, const unsigned int z,
			const float x0, const float y0, const float z0,
			const float x1, const float y1, const float z1,
			const float x2, const float y2, const float z2,
			const float x3, const float y3, const float z3,
			const double * ipp_iop)
{
	ImageSlice * cs = new ImageSlice;
	cs->v[ 0]  = x0;
	cs->v[ 1]  = y0;
	cs->v[ 2]  = z0;
	cs->v[ 3]  = x1;
	cs->v[ 4]  = y1;
	cs->v[ 5]  = z1;
	cs->v[ 6]  = x2;
	cs->v[ 7]  = y2;
	cs->v[ 8]  = z2;
	cs->v[ 9]  = x3;
	cs->v[10]  = y3;
	cs->v[11]  = z3;
	cs->tc[ 0] = 0.0f;
	cs->tc[ 1] = 1.0f;
	cs->tc[ 2] = z / static_cast<float>(dimz - 1);
	cs->tc[ 3] = 0.0f;
	cs->tc[ 4] = 0.0f;
	cs->tc[ 5] = z / static_cast<float>(dimz - 1);
	cs->tc[ 6] = 1.0f;
	cs->tc[ 7] = 1.0f;
	cs->tc[ 8] = z / static_cast<float>(dimz - 1);
	cs->tc[ 9] = 1.0f;
	cs->tc[10] = 0.0f;
	cs->tc[11] = z / static_cast<float>(dimz - 1);
	cs->fv[ 0] = x0;
	cs->fv[ 1] = y0;
	cs->fv[ 2] = z0;
	cs->fv[ 3] = x1;
	cs->fv[ 4] = y1;
	cs->fv[ 5] = z1;
	cs->fv[ 6] = x3;
	cs->fv[ 7] = y3;
	cs->fv[ 8] = z3;
	cs->fv[ 9] = x2;
	cs->fv[10] = y2;
	cs->fv[11] = z2;
	cs->ipp_iop[0] = ipp_iop[0];
	cs->ipp_iop[1] = ipp_iop[1];
	cs->ipp_iop[2] = ipp_iop[2];
	cs->ipp_iop[3] = ipp_iop[3];
	cs->ipp_iop[4] = ipp_iop[4];
	cs->ipp_iop[5] = ipp_iop[5];
	cs->ipp_iop[6] = ipp_iop[6];
	cs->ipp_iop[7] = ipp_iop[7];
	cs->ipp_iop[8] = ipp_iop[8];
	cs->slice_orientation_string = orient;
	slices.push_back(cs);
}

void CommonUtils::generate_spectroscopyslice(
			std::vector<SpectroscopySlice*> & slices,
			const QString & orient,
			const bool ok3d, GLWidget * gl,
			const float x0, const float y0, const float z0,
			const float x1, const float y1, const float z1,
			const float x2, const float y2, const float z2,
			const float x3, const float y3, const float z3,
			unsigned int columns_, unsigned int rows_)
{
	SpectroscopySlice * cs = new SpectroscopySlice;
	cs->fv[ 0] = x0;
	cs->fv[ 1] = y0;
	cs->fv[ 2] = z0;
	cs->fv[ 3] = x1;
	cs->fv[ 4] = y1;
	cs->fv[ 5] = z1;
	cs->fv[ 6] = x3;
	cs->fv[ 7] = y3;
	cs->fv[ 8] = z3;
	cs->fv[ 9] = x2;
	cs->fv[10] = y2;
	cs->fv[11] = z2;
	if (ok3d && gl)
	{
		if (columns_ >= 2 || rows_ >= 2)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			gl->makeCurrent();
			gl->glGenVertexArrays(1, &(cs->fvaoid));
			gl->glBindVertexArray(cs->fvaoid);
			gl->glGenBuffers(1, &(cs->fvboid));
			gl->glBindBuffer(GL_ARRAY_BUFFER, cs->fvboid);
			gl->glBufferData(GL_ARRAY_BUFFER, 4 * 3* sizeof(GLfloat), cs->fv, GL_STATIC_DRAW);
			gl->glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
			gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
			gl->glBindVertexArray(0);
#else
			gl->makeCurrent();
			glGenVertexArrays(1, &(cs->fvaoid));
			glBindVertexArray(cs->fvaoid);
			glGenBuffers(1, &(cs->fvboid));
			glBindBuffer(GL_ARRAY_BUFFER, cs->fvboid);
			glBufferData(GL_ARRAY_BUFFER, 4 * 3* sizeof(GLfloat), cs->fv, GL_STATIC_DRAW);
			glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(gl->frame_shader.position_handle);
			glBindVertexArray(0);
#endif
			GLWidget::increment_count_vbos(1);
			//
			if (columns_ > 2 && rows_ > 2)
			{
				const unsigned long lines_size = 3 * 2 * (columns_ - 2 + rows_ - 2);
				cs->lsize = lines_size / 3;
				GLfloat * v = new GLfloat[lines_size];
				const sVector3 X0(x0, y0, z0);
				const sVector3 X1(x1, y1, z1);
				const sVector3 YN(x1, y1, z1);
				const sVector3 XN(x2, y2, z2);
				const float    Xd = length(XN - X0);
				const float    Yd = length(YN - X0);
				const sVector3 Xn = normalize(XN - X0);
				const sVector3 Yn = normalize(YN - X0);
				float dx = Xd/static_cast<float>(columns_ - 1);
				float dy = Yd/static_cast<float>(rows_ - 1);
				const sVector3 p = X1 + dx * Xn;
				unsigned long j{};
				for (unsigned int x = 1; x < columns_ - 1; ++x)
				{
					const sVector3 from = X0 + (static_cast<float>(x) * dx) * Xn;
					const sVector3 to   = from + Yd * Yn;
					v[j] = from.getX();
					v[j + 1] = from.getY();
					v[j + 2] = from.getZ();
					v[j + 3] =   to.getX();
					v[j + 4] =   to.getY();
					v[j + 5] =   to.getZ();
					j += 6;
				}
				for (unsigned int x = 1; x < rows_ - 1; ++x)
				{
					sVector3 from = X0 + (static_cast<float>(x) * dy) * Yn;
					sVector3 to   = from + Xd * Xn;
					v[j] = from.getX();
					v[j + 1] = from.getY();
					v[j + 2] = from.getZ();
					v[j + 3] =   to.getX();
					v[j + 4] =   to.getY();
					v[j + 5] =   to.getZ();
					j += 6;
				}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
				gl->glGenVertexArrays(1, &(cs->lvaoid));
				gl->glBindVertexArray(cs->lvaoid);
				gl->glGenBuffers(1, &(cs->lvboid));
				gl->glBindBuffer(GL_ARRAY_BUFFER, cs->lvboid);
				gl->glBufferData(GL_ARRAY_BUFFER, lines_size*sizeof(GLfloat), v, GL_STATIC_DRAW);
				gl->glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
				gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
				gl->glBindVertexArray(0);
#else
				glGenVertexArrays(1, &(cs->lvaoid));
				glBindVertexArray(cs->lvaoid);
				glGenBuffers(1, &(cs->lvboid));
				glBindBuffer(GL_ARRAY_BUFFER, cs->lvboid);
				glBufferData(GL_ARRAY_BUFFER, lines_size*sizeof(GLfloat), v, GL_STATIC_DRAW);
				glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArray(gl->frame_shader.position_handle);
				glBindVertexArray(0);
#endif
				delete [] v;
				cs->psize = 2;
				GLfloat * v1 = new GLfloat[6];
				v1[0] = x1;
				v1[1] = y1;
				v1[2] = z1;
				v1[3] = p.getX();
				v1[4] = p.getY();
				v1[5] = p.getZ();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
				gl->glGenVertexArrays(1, &(cs->pvaoid));
				gl->glBindVertexArray(cs->pvaoid);
				gl->glGenBuffers(1, &(cs->pvboid));
				gl->glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
				gl->glBufferData(GL_ARRAY_BUFFER, 6*sizeof(GLfloat), v1, GL_STATIC_DRAW);
				gl->glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
				gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
				gl->glBindVertexArray(0);
#else
				glGenVertexArrays(1, &(cs->pvaoid));
				glBindVertexArray(cs->pvaoid);
				glGenBuffers(1, &(cs->pvboid));
				glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
				glBufferData(GL_ARRAY_BUFFER, 6*sizeof(GLfloat), v1, GL_STATIC_DRAW);
				glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
				glEnableVertexAttribArray(gl->frame_shader.position_handle);
				glBindVertexArray(0);
#endif
				delete [] v1;
				GLWidget::increment_count_vbos(2);
			}
		}
		else if (columns_ == 1 && rows_ == 1)
		{
			gl->makeCurrent();
			cs->psize = 1;
			GLfloat * v1 = new GLfloat[3];
			v1[0] = x1;
			v1[1] = y1;
			v1[2] = z1;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			gl->glGenVertexArrays(1, &(cs->pvaoid));
			gl->glBindVertexArray(cs->pvaoid);
			gl->glGenBuffers(1, &(cs->pvboid));
			gl->glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
			gl->glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat), v1, GL_STATIC_DRAW);
			gl->glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
			gl->glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
			gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
			gl->glBindVertexArray(0);
#else
			glGenVertexArrays(1, &(cs->pvaoid));
			glBindVertexArray(cs->pvaoid);
			glGenBuffers(1, &(cs->pvboid));
			glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
			glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat), v1, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
			glVertexAttribPointer(gl->frame_shader.position_handle, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(gl->frame_shader.position_handle);
			glBindVertexArray(0);
#endif
			delete [] v1;
			GLWidget::increment_count_vbos(1);
		}
	}
	cs->slice_orientation_string = orient;
	slices.push_back(cs);
}

void CommonUtils::calculate_rgb_minmax(ImageVariant * ivariant)
{
	if (!ivariant) return;
	switch (ivariant->image_type)
	{
	case 10:
		calculate_rgb_minmax_<RGBImageTypeSS>(ivariant->pSS_rgb, ivariant);
		break;
	case 11:
		calculate_rgb_minmax_<RGBImageTypeUS>(ivariant->pUS_rgb, ivariant);
		break;
	case 12:
		calculate_rgb_minmax_<RGBImageTypeSI>(ivariant->pSI_rgb, ivariant);
		break;
	case 13:
		calculate_rgb_minmax_<RGBImageTypeUI>(ivariant->pUI_rgb, ivariant);
		break;
	case 14:
		calculate_rgb_minmax_<RGBImageTypeUC>(ivariant->pUC_rgb, ivariant);
		break;
	case 15:
		calculate_rgb_minmax_<RGBImageTypeF>(ivariant->pF_rgb, ivariant);
		break;
	case 16:
		calculate_rgb_minmax_<RGBImageTypeD>(ivariant->pD_rgb, ivariant);
		break;
	default:
		break;
	}
}

void CommonUtils::calculate_rgba_minmax(ImageVariant * ivariant)
{
	if (!ivariant) return;
	switch (ivariant->image_type)
	{
	case 20:
		calculate_rgba_minmax_<RGBAImageTypeSS>(ivariant->pSS_rgba, ivariant);
		break;
	case 21:
		calculate_rgba_minmax_<RGBAImageTypeUS>(ivariant->pUS_rgba, ivariant);
		break;
	case 22:
		calculate_rgba_minmax_<RGBAImageTypeSI>(ivariant->pSI_rgba, ivariant);
		break;
	case 23:
		calculate_rgba_minmax_<RGBAImageTypeUI>(ivariant->pUI_rgba, ivariant);
		break;
	case 24:
		calculate_rgba_minmax_<RGBAImageTypeUC>(ivariant->pUC_rgba, ivariant);
		break;
	case 25:
		calculate_rgba_minmax_<RGBAImageTypeF>(ivariant->pF_rgba, ivariant);
		break;
	case 26:
		calculate_rgba_minmax_<RGBAImageTypeD>(ivariant->pD_rgba, ivariant);
		break;
	default:
		break;
	}
}

void CommonUtils::copy_slices(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest || !source) return;
	bool break_{};
	for (unsigned int x = 0; x < source->di->image_slices.size(); ++x)
	{
		const ImageSlice * scs = source->di->image_slices.at(x);
		if (scs)
		{
			ImageSlice * cs = new ImageSlice;
			cs->v[ 0]  = scs->v[ 0];
			cs->v[ 1]  = scs->v[ 1];
			cs->v[ 2]  = scs->v[ 2];
			cs->v[ 3]  = scs->v[ 3];
			cs->v[ 4]  = scs->v[ 4];
			cs->v[ 5]  = scs->v[ 5];
			cs->v[ 6]  = scs->v[ 6];
			cs->v[ 7]  = scs->v[ 7];
			cs->v[ 8]  = scs->v[ 8];
			cs->v[ 9]  = scs->v[ 9];
			cs->v[10]  = scs->v[10];
			cs->v[11]  = scs->v[11];
			cs->tc[ 0] = 0.0f;
			cs->tc[ 1] = 1.0f;
			cs->tc[ 2] = scs->tc[2];
			cs->tc[ 3] = 0.0f;
			cs->tc[ 4] = 0.0f;
			cs->tc[ 5] = scs->tc[5];
			cs->tc[ 6] = 1.0f;
			cs->tc[ 7] = 1.0f;
			cs->tc[ 8] = scs->tc[8];
			cs->tc[ 9] = 1.0f;
			cs->tc[10] = 0.0f;
			cs->tc[11] = scs->tc[11];
			cs->fv[ 0] = scs->fv[ 0];
			cs->fv[ 1] = scs->fv[ 1];
			cs->fv[ 2] = scs->fv[ 2];
			cs->fv[ 3] = scs->fv[ 3];
			cs->fv[ 4] = scs->fv[ 4];
			cs->fv[ 5] = scs->fv[ 5];
			cs->fv[ 6] = scs->fv[ 6];
			cs->fv[ 7] = scs->fv[ 7];
			cs->fv[ 8] = scs->fv[ 8];
			cs->fv[ 9] = scs->fv[ 9];
			cs->fv[10] = scs->fv[10];
			cs->fv[11] = scs->fv[11];
			cs->ipp_iop[0] = scs->ipp_iop[0];
			cs->ipp_iop[1] = scs->ipp_iop[1];
			cs->ipp_iop[2] = scs->ipp_iop[2];
			cs->ipp_iop[3] = scs->ipp_iop[3];
			cs->ipp_iop[4] = scs->ipp_iop[4];
			cs->ipp_iop[5] = scs->ipp_iop[5];
			cs->ipp_iop[6] = scs->ipp_iop[6];
			cs->ipp_iop[7] = scs->ipp_iop[7];
			cs->ipp_iop[8] = scs->ipp_iop[8];
			cs->slice_orientation_string = scs->slice_orientation_string;
			dest->di->image_slices.push_back(cs);
		}
		else
		{
			break_ = true;
			break;
		}
	}
	if (break_)
	{
		for (unsigned int x = 0; x < dest->di->image_slices.size(); ++x)
		{
			if (dest->di->image_slices.at(x))
			{
				delete dest->di->image_slices[x];
			}
		}
		dest->di->image_slices.clear();
		return;
	}
	dest->di->ix_origin = source->di->ix_origin;
	dest->di->iy_origin = source->di->iy_origin;
	dest->di->iz_origin = source->di->iz_origin;
	for (int j = 0; j < 6; ++j)
	{
		dest->di->dircos[j] = source->di->dircos[j];
	}
	dest->di->slices_direction_x = source->di->slices_direction_x;
	dest->di->slices_direction_y = source->di->slices_direction_y;
	dest->di->slices_direction_z = source->di->slices_direction_z;
	dest->di->up_direction_x     = source->di->up_direction_x;
	dest->di->up_direction_y     = source->di->up_direction_y;
	dest->di->up_direction_z     = source->di->up_direction_z;
	dest->di->default_center_x   = source->di->default_center_x;
	dest->di->default_center_y   = source->di->default_center_y;
	dest->di->default_center_z   = source->di->default_center_z;
	dest->di->center_x           = source->di->center_x;
	dest->di->center_y           = source->di->center_y;
	dest->di->center_z           = source->di->center_z;
	dest->di->slices_generated   = source->di->slices_generated;
	dest->di->slices_from_dicom  = source->di->slices_from_dicom;
	dest->one_direction          = source->one_direction;
	copy_essential(dest, source);
	for (int j = 0; j < 3; ++j)
	{
		dest->di->origin[j] = source->di->origin[j];
	}
	dest->di->origin_ok = source->di->origin_ok;
}

void CommonUtils::copy_essential(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest || !source) return;
	dest->equi = source->equi;
	dest->di->skip_texture = source->di->skip_texture;
	dest->di->hide_orientation = source->di->hide_orientation;
	if (source->ybr &&
		dest->image_type >= 10 && dest->image_type < 16 &&
		source->image_type >= 10 && source->image_type < 16)
	{
		dest->ybr = true;
	}
	copy_imagevariant_info(dest, source);
}

void CommonUtils::calculate_minmax_scalar(
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	switch (ivariant->image_type)
	{
	case 0:
		calculate_min_max<ImageTypeSS>(ivariant->pSS, ivariant);
		break;
	case 1:
		calculate_min_max<ImageTypeUS>(ivariant->pUS, ivariant);
		break;
	case 2:
		calculate_min_max<ImageTypeSI>(ivariant->pSI, ivariant);
		break;
	case 3:
		calculate_min_max<ImageTypeUI>(ivariant->pUI, ivariant);
		break;
	case 4:
		calculate_min_max<ImageTypeUC>(ivariant->pUC, ivariant);
		break;
	case 5:
		calculate_min_max<ImageTypeF>(ivariant->pF, ivariant);
		break;
	case 6:
		calculate_min_max<ImageTypeD>(ivariant->pD, ivariant);
		break;
	case 7:
		calculate_min_max<ImageTypeSLL>(ivariant->pSLL, ivariant);
		break;
	case 8:
		calculate_min_max<ImageTypeULL>(ivariant->pULL, ivariant);
		break;
	default:
		break;
	}
}

bool CommonUtils::reload_monochrome(
	ImageVariant * ivariant,
	bool ok3d,
	GLWidget * gl,
	int max_3d_tex_size,
	bool change_size,
	unsigned int size_x_,
	unsigned int size_y_)
{
	if (!ivariant) return false;
	bool ok{};
	if (ivariant->image_type == 0)
	{
		ok = reload_monochrome_image<ImageTypeSS>(
			ivariant, ivariant->pSS, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 1)
	{
		ok = reload_monochrome_image<ImageTypeUS>(
			ivariant, ivariant->pUS, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 2)
	{
		ok = reload_monochrome_image<ImageTypeSI>(
			ivariant, ivariant->pSI, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 3)
	{
		ok = reload_monochrome_image<ImageTypeUI>(
			ivariant, ivariant->pUI, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 4)
	{
		ivariant->di->maxwindow = true;
		ok = reload_monochrome_image<ImageTypeUC>(
			ivariant, ivariant->pUC, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 5)
	{
		ok = reload_monochrome_image<ImageTypeF>(
			ivariant, ivariant->pF, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 6)
	{
		ok = reload_monochrome_image<ImageTypeD>(
			ivariant, ivariant->pD, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 7)
	{
		ok = reload_monochrome_image<ImageTypeSLL>(
			ivariant, ivariant->pSLL, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else if (ivariant->image_type == 8)
	{
		ok = reload_monochrome_image<ImageTypeULL>(
			ivariant, ivariant->pULL, gl, max_3d_tex_size,
			nullptr,
			change_size, size_x_, size_y_);
	}
	else
	{
		return false;
	}
	return ok;
}

bool CommonUtils::reload_rgb_rgba(ImageVariant * ivariant)
{
	if (!ivariant) return false;
	if (ivariant->image_type == 10)
		reload_rgb_image<RGBImageTypeSS>(ivariant->pSS_rgb, ivariant);
	else if (ivariant->image_type == 11)
		reload_rgb_image<RGBImageTypeUS>(ivariant->pUS_rgb, ivariant);
	else if (ivariant->image_type == 12)
		reload_rgb_image<RGBImageTypeSI>(ivariant->pSI_rgb, ivariant);
	else if (ivariant->image_type == 13)
		reload_rgb_image<RGBImageTypeUI>(ivariant->pUI_rgb, ivariant);
	else if (ivariant->image_type == 14)
		reload_rgb_image<RGBImageTypeUC>(ivariant->pUC_rgb, ivariant);
	else if (ivariant->image_type == 15)
		reload_rgb_image<RGBImageTypeF>(ivariant->pF_rgb, ivariant);
	else if (ivariant->image_type == 16)
		reload_rgb_image<RGBImageTypeD>(ivariant->pD_rgb, ivariant);
	else if (ivariant->image_type == 20)
		reload_rgba_image<RGBAImageTypeSS>(ivariant->pSS_rgba, ivariant);
	else if (ivariant->image_type == 21)
		reload_rgba_image<RGBAImageTypeUS>(ivariant->pUS_rgba, ivariant);
	else if (ivariant->image_type == 22)
		reload_rgba_image<RGBAImageTypeSI>(ivariant->pSI_rgba, ivariant);
	else if (ivariant->image_type == 23)
		reload_rgba_image<RGBAImageTypeUI>(ivariant->pUI_rgba, ivariant);
	else if (ivariant->image_type == 24)
		reload_rgba_image<RGBAImageTypeUC>(ivariant->pUC_rgba, ivariant);
	else if (ivariant->image_type == 25)
		reload_rgba_image<RGBAImageTypeF>(ivariant->pF_rgba, ivariant);
	else if (ivariant->image_type == 26)
		reload_rgba_image<RGBAImageTypeD>(ivariant->pD_rgba, ivariant);
	else return false;
	return true;
}

void CommonUtils::copy_imagevariant_info(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	dest->instance_number    = source->instance_number;
	dest->study_uid          = source->study_uid;
	dest->series_uid         = source->series_uid;
	dest->study_date         = source->study_date;
	dest->study_time         = source->study_time;
	dest->study_id           = source->study_id;
	dest->frame_of_ref_uid   = source->frame_of_ref_uid;
	dest->modality           = source->modality;
	dest->iod                = source->iod;
	dest->sop                = source->sop;
	dest->series_description = source->series_description;
	dest->study_description  = source->study_description;
	dest->pat_name           = source->pat_name;
	dest->pat_id             = source->pat_id;
	dest->pat_birthdate      = source->pat_birthdate;
	dest->pat_sex            = source->pat_sex;
	dest->series_date        = source->series_date;
	dest->series_time        = source->series_time;
	dest->acquisition_date   = source->acquisition_date;
	dest->acquisition_time   = source->acquisition_time;
	dest->hardware           = source->hardware;
	dest->hardware_info      = source->hardware_info;
	dest->iinfo              = source->iinfo;
	dest->institution        = source->institution;
}

void CommonUtils::copy_frametimes(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	for (unsigned int x = 0; x < source->frame_times.size(); ++x)
	{
		dest->frame_times.push_back(source->frame_times.at(x));
	}
}

void CommonUtils::copy_usregions(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	for (int x = 0; x < source->usregions.size(); ++x)
	{
		dest->usregions.push_back(source->usregions[x]);
	}
}

void CommonUtils::copy_imagevariant_overlays(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QMap<int, SliceOverlays>::const_iterator it = source->image_overlays.all_overlays.cbegin();
	while (it != source->image_overlays.all_overlays.cend())
#else
	QMap<int, SliceOverlays>::const_iterator it = source->image_overlays.all_overlays.constBegin();
	while (it != source->image_overlays.all_overlays.constEnd())
#endif
	{
		const int source_key = it.key();
		const SliceOverlays & source_overlays = it.value();
		for (int x = 0; x < source_overlays.size(); ++x)
		{
			const SliceOverlay source_overlay = source_overlays.at(x);
			SliceOverlay overlay;
			overlay.dimx = source_overlay.dimx;
			overlay.dimy = source_overlay.dimy;
			overlay.x = source_overlay.x;
			overlay.y = source_overlay.y;
			for (size_t j = 0; j < source_overlay.data.size(); ++j)
			{
				overlay.data.push_back(source_overlay.data.at(j));
			}
			if (!dest->image_overlays.all_overlays.contains(source_key))
			{
				dest->image_overlays.all_overlays[source_key] = SliceOverlays();
			}
			dest->image_overlays.all_overlays[source_key].push_back(overlay);
		}
		++it;
	}
}

void CommonUtils::reset_bb(ImageVariant * ivariant)
{
	if (!ivariant) return;
	ivariant->di->irect_index[0] = 0;
	ivariant->di->irect_index[1] = 0;
	ivariant->di->irect_size[0] = ivariant->di->idimx;
	ivariant->di->irect_size[1] = ivariant->di->idimy;
	ivariant->di->selected_x_slice = (ivariant->di->idimx > 0) ? ivariant->di->idimx / 2 : 0;
	ivariant->di->selected_y_slice = (ivariant->di->idimy > 0) ? ivariant->di->idimy / 2 : 0;
	ivariant->di->selected_z_slice = (ivariant->di->idimz > 0) ? ivariant->di->idimz / 2 : 0;
	ivariant->di->from_slice = 0;
	ivariant->di->to_slice = ivariant->di->idimz-1;
}

void CommonUtils::get_dimensions_(ImageVariant * ivariant)
{
	if (!ivariant) return;
	switch (ivariant->image_type)
	{
	case 0:
		get_dimensions<ImageTypeSS>(
			ivariant->pSS,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 1:
		get_dimensions<ImageTypeUS>(
			ivariant->pUS,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 2:
		get_dimensions<ImageTypeSI>(
			ivariant->pSI,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 3:
		get_dimensions<ImageTypeUI>(
			ivariant->pUI,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 4:
		get_dimensions<ImageTypeUC>(
			ivariant->pUC,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 5:
		get_dimensions<ImageTypeF>(
			ivariant->pF,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 6:
		get_dimensions<ImageTypeD>(
			ivariant->pD,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 7:
		get_dimensions<ImageTypeSLL>(
			ivariant->pSLL,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 8:
		get_dimensions<ImageTypeULL>(
			ivariant->pULL,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 10:
		get_dimensions<RGBImageTypeSS>(
			ivariant->pSS_rgb,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 11:
		get_dimensions<RGBImageTypeUS>(
			ivariant->pUS_rgb,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 12:
		get_dimensions<RGBImageTypeSI>(
			ivariant->pSI_rgb,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 13:
		get_dimensions<RGBImageTypeUI>(
			ivariant->pUI_rgb,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 14:
		get_dimensions<RGBImageTypeUC>(
			ivariant->pUC_rgb,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 15:
		get_dimensions<RGBImageTypeF>(
			ivariant->pF_rgb,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 16:
		get_dimensions<RGBImageTypeD>(
			ivariant->pD_rgb,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 20:
		get_dimensions<RGBAImageTypeSS>(
			ivariant->pSS_rgba,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 21:
		get_dimensions<RGBAImageTypeUS>(
			ivariant->pUS_rgba,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 22:
		get_dimensions<RGBAImageTypeSI>(
			ivariant->pSI_rgba,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 23:
		get_dimensions<RGBAImageTypeUI>(
			ivariant->pUI_rgba,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 24:
		get_dimensions<RGBAImageTypeUC>(
			ivariant->pUC_rgba,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 25:
		get_dimensions<RGBAImageTypeF>(
			ivariant->pF_rgba,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	case 26:
		get_dimensions<RGBAImageTypeD>(
			ivariant->pD_rgba,
			&ivariant->di->idimx,
			&ivariant->di->idimy,
			&ivariant->di->idimz,
			&ivariant->di->ix_spacing,
			&ivariant->di->iy_spacing,
			&ivariant->di->iz_spacing,
			&ivariant->di->ix_origin,
			&ivariant->di->iy_origin,
			&ivariant->di->iz_origin);
		break;
	default:
		break;
	}
}

QString CommonUtils::get_orientation1(
	const ImageVariant * ivariant,
	unsigned int * result)
{
	if (!ivariant) return QString("");
	QString orient;
	switch (ivariant->image_type)
	{
	case 0:
		orient = get_orientation<ImageTypeSS>(ivariant->pSS, result);
		break;
	case 1:
		orient = get_orientation<ImageTypeUS>(ivariant->pUS, result);
		break;
	case 2:
		orient = get_orientation<ImageTypeSI>(ivariant->pSI, result);
		break;
	case 3:
		orient = get_orientation<ImageTypeUI>(ivariant->pUI, result);
		break;
	case 4:
		orient = get_orientation<ImageTypeUC>(ivariant->pUC, result);
		break;
	case 5:
		orient = get_orientation<ImageTypeF>(ivariant->pF, result);
		break;
	case 6:
		orient = get_orientation<ImageTypeD>(ivariant->pD, result);
		break;
	case 7:
		orient = get_orientation<ImageTypeSLL>(ivariant->pSLL, result);
		break;
	case 8:
		orient = get_orientation<ImageTypeULL>(ivariant->pULL, result);
		break;
	case 10:
		orient = get_orientation<RGBImageTypeSS>(ivariant->pSS_rgb, result);
		break;
	case 11:
		orient = get_orientation<RGBImageTypeUS>(ivariant->pUS_rgb, result);
		break;
	case 12:
		orient = get_orientation<RGBImageTypeSI>(ivariant->pSI_rgb, result);
		break;
	case 13:
		orient = get_orientation<RGBImageTypeUI>(ivariant->pUI_rgb, result);
		break;
	case 14:
		orient = get_orientation<RGBImageTypeUC>(ivariant->pUC_rgb, result);
		break;
	case 15:
		orient = get_orientation<RGBImageTypeF>(ivariant->pF_rgb, result);
		break;
	case 16:
		orient = get_orientation<RGBImageTypeD>(ivariant->pD_rgb, result);
		break;
	case 20:
		orient = get_orientation<RGBAImageTypeSS>(ivariant->pSS_rgba, result);
		break;
	case 21:
		orient = get_orientation<RGBAImageTypeUS>(ivariant->pUS_rgba, result);
		break;
	case 22:
		orient = get_orientation<RGBAImageTypeSI>(ivariant->pSI_rgba, result);
		break;
	case 23:
		orient = get_orientation<RGBAImageTypeUI>(ivariant->pUI_rgba, result);
		break;
	case 24:
		orient = get_orientation<RGBAImageTypeUC>(ivariant->pUC_rgba, result);
		break;
	case 25:
		orient = get_orientation<RGBAImageTypeF>(ivariant->pF_rgba, result);
		break;
	case 26:
		orient = get_orientation<RGBAImageTypeD>(ivariant->pD_rgba, result);
		break;
	default:
		break;
	}
	return orient;
}

QString CommonUtils::gen_itk_image(bool * ok,
	const std::vector<char*> & data,
	const mdcm::PixelFormat & pixelformat,
	const mdcm::PhotometricInterpretation & pi,
	ImageVariant * ivariant,
	itk::Matrix<itk::SpacePrecisionType, 3, 3> & direction,
	unsigned int dimx_, unsigned int dimy_, unsigned int dimz_,
	double origin_x, double origin_y, double origin_z,
	double spacing_x, double spacing_y, double spacing_z,
	bool geometry_from_image, bool allow_geometry_from_image,
	bool resize_, unsigned int size_x, unsigned int size_y,
	bool no_warn_rescale,
	bool use_icc,
	bool skip_ybr)
{
	*ok = false;
	QString error;
	const QString not_supported("Pixel format is not supported");
	const size_t dimx = dimx_;
	const size_t dimy = dimy_;
	const size_t dimz = dimz_;
	if (data.size() < 1) return QString("Invalid data");
	short ybr{};
	if (!skip_ybr && !use_icc)
	{
		if (pi == mdcm::PhotometricInterpretation::YBR_FULL ||
			pi == mdcm::PhotometricInterpretation::YBR_FULL_422)
		{
			ybr = 1;
		}
#ifdef TRY_SUPPORT_YBR_PARTIAL_422
		else if (pi == mdcm::PhotometricInterpretation::YBR_PARTIAL_422)
		{
			ybr = 2;
		}
#endif
	}
	const bool cmyk = (pi == mdcm::PhotometricInterpretation::CMYK);
	const bool argb = (pi == mdcm::PhotometricInterpretation::ARGB);
	const bool hsv  = (pi == mdcm::PhotometricInterpretation::HSV);
#ifdef ALIZA_VERBOSE
	if (argb)
	{
		std::cout << "DICOM ARGB will be opened incorrectly" << std::endl;
	}
#endif
	const unsigned short bits_allocated = pixelformat.GetBitsAllocated();
	const unsigned short bits_stored    = pixelformat.GetBitsStored();
	const unsigned short high_bit       = pixelformat.GetHighBit();
	if (bits_allocated > 0) ivariant->di->bits_allocated = bits_allocated;
	if (bits_stored > 0)    ivariant->di->bits_stored    = bits_stored;
	if (high_bit > 0)       ivariant->di->high_bit       = high_bit;
	// monochrome
	if (pixelformat.GetSamplesPerPixel() == 1)
	{
		// INT12 and UINT12 should be already converted or don't reach this point
		switch (pixelformat)
		{
		case mdcm::PixelFormat::INT12:
		case mdcm::PixelFormat::INT16:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeSS>(
					ok,
					ivariant,
					ivariant->pSS,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					0,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeSS>(
					ivariant,
					ivariant->pSS,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::UINT12:
		case mdcm::PixelFormat::UINT16:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeUS>(
					ok,
					ivariant,
					ivariant->pUS,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					1,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeUS>(
					ivariant,
					ivariant->pUS,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::INT32:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeSI>(
					ok,
					ivariant,
					ivariant->pSI,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					2,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeSI>(
					ivariant,
					ivariant->pSI,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::UINT32:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeUI>(
					ok,
					ivariant,
					ivariant->pUI,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					3,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeUI>(
					ivariant,
					ivariant->pUI,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::INT64:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeSLL>(
					ok,
					ivariant,
					ivariant->pSLL,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					7,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeSLL>(
					ivariant,
					ivariant->pSLL,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::UINT64:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeULL>(
					ok,
					ivariant,
					ivariant->pULL,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					8,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeULL>(
					ivariant,
					ivariant->pULL,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::INT8:
		case mdcm::PixelFormat::UINT8:
		case mdcm::PixelFormat::SINGLEBIT:
			{
				ivariant->di->maxwindow = true;
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeUC>(
					ok,
					ivariant,
					ivariant->pUC,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					4,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeUC>(
					ivariant,
					ivariant->pUC,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::FLOAT32:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeF>(
					ok,
					ivariant,
					ivariant->pF,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					5,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeF>(
					ivariant,
					ivariant->pF,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::FLOAT64:
			{
				bool bad_direction{true};
				error = process_dicom_monochrome_image1<ImageTypeD>(
					ok,
					ivariant,
					ivariant->pD,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					6,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_monochrome_image2<ImageTypeD>(
					ivariant,
					ivariant->pD,
					geometry_from_image,
					bad_direction);
			}
			break;
		default:
			error = not_supported;
			break;
		}
	}
	// RGB
	//
	// HSV (Retired)
	// Pixel data represent a color image described by hue, saturation, and
	// value image planes. The minimum sample value for each HSV plane
	// represents a minimum value of each vector. This value may be used only
	// when Samples per Pixel (0028,0002) has a value of 3
	//
	else if (pixelformat.GetSamplesPerPixel() == 3)
	{
		switch (pixelformat)
		{
		case mdcm::PixelFormat::UINT8:
		case mdcm::PixelFormat::INT8:
			{
				const int bitsstored = pixelformat.GetBitsStored();
				bool bad_direction{true};
				error = process_dicom_rgb_image1<RGBImageTypeUC>(
					ok,
					ivariant,
					ivariant->pUC_rgb,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					14,
					ybr,
					hsv,
					bitsstored,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_rgb_image2<RGBImageTypeUC>(
					ivariant,
					ivariant->pUC_rgb,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::UINT16:
			{
				const int bitsstored = pixelformat.GetBitsStored();
				bool bad_direction{true};
				error = process_dicom_rgb_image1<RGBImageTypeUS>(
					ok,
					ivariant,
					ivariant->pUS_rgb,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					11,
					ybr,
					false,
					bitsstored,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_rgb_image2<RGBImageTypeUS>(
					ivariant,
					ivariant->pUS_rgb,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::INT16:
			{
				bool bad_direction{true};
				error = process_dicom_rgb_image1<RGBImageTypeSS>(
					ok,
					ivariant,
					ivariant->pSS_rgb,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					10,
					false,
					false,
					0,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_rgb_image2<RGBImageTypeSS>(
					ivariant,
					ivariant->pSS_rgb,
					geometry_from_image,
					bad_direction);
			}
			break;
		case mdcm::PixelFormat::FLOAT32:
			{
				bool bad_direction{true};
				error = process_dicom_rgb_image1<RGBImageTypeF>(
					ok,
					ivariant,
					ivariant->pF_rgb,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					15,
					false,
					false,
					0,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_rgb_image2<RGBImageTypeF>(
					ivariant,
					ivariant->pF_rgb,
					geometry_from_image,
					bad_direction);
			}
			break;
		default:
			error = not_supported;
			break;
		}
		if (error.isEmpty() && skip_ybr &&
			(pi == mdcm::PhotometricInterpretation::YBR_FULL_422 ||
#ifdef TRY_SUPPORT_YBR_PARTIAL_422
			 pi == mdcm::PhotometricInterpretation::YBR_PARTIAL_422 ||
#endif
			 pi == mdcm::PhotometricInterpretation::YBR_FULL))
		{
			ivariant->ybr = true;
		}
	}
	// Workaround to handle 4-components DICOM,
	// formats are retired 2001, there no example files
	// available.
	else if (pixelformat.GetSamplesPerPixel() == 4)
	{
#if 1
		switch (pixelformat)
		{
		case mdcm::PixelFormat::UINT8:
		case mdcm::PixelFormat::INT8:
			{
				bool bad_direction{true};
				error = process_dicom_rgba_image1<RGBAImageTypeUC>(
					ok,
					ivariant,
					ivariant->pUC_rgba,
					data,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					24,
					cmyk,
					argb,
					&bad_direction);
				if (!error.isEmpty()) return error;
				process_dicom_rgba_image2<RGBAImageTypeUC>(
					ivariant,
					ivariant->pUC_rgba,
					geometry_from_image,
					bad_direction);
			}
			break;
		default:
			error = not_supported;
			break;
		}
#else
		error = not_supported;
#endif
	}
	else
	{
		error = not_supported;
	}
	//
	// overtwrite orientation, it may be default and wrong
	if (geometry_from_image && !allow_geometry_from_image)
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	if (ivariant->image_type >= 0 && ivariant->image_type < 10 && !no_warn_rescale)
	{
		ivariant->rescale_disabled = true;
	}
	if (ivariant->di->slices_from_dicom || allow_geometry_from_image)
	{
		ivariant->di->hide_orientation = false;
	}
	if (error.isEmpty()) *ok = true;
	return error;
}

void CommonUtils::read_geometry_from_image_(ImageVariant * v)
{
	if (!v) return;
	switch (v->image_type)
	{
	case  0: read_geometry_from_image<ImageTypeSS>(v, v->pSS);
		break;
	case  1: read_geometry_from_image<ImageTypeUS>(v, v->pUS);
		break;
	case  2: read_geometry_from_image<ImageTypeSI>(v, v->pSI);
		break;
	case  3: read_geometry_from_image<ImageTypeUI>(v, v->pUI);
		break;
	case  4: read_geometry_from_image<ImageTypeUC>(v, v->pUC);
		break;
	case  5: read_geometry_from_image<ImageTypeF>(v, v->pF);
		break;
	case  6: read_geometry_from_image<ImageTypeD>(v, v->pD);
		break;
	case  7: read_geometry_from_image<ImageTypeSLL>(v, v->pSLL);
		break;
	case  8: read_geometry_from_image<ImageTypeULL>(v, v->pULL);
		break;
	case 10: read_geometry_from_image<RGBImageTypeSS>(v, v->pSS_rgb);
		break;
	case 11: read_geometry_from_image<RGBImageTypeUS>(v, v->pUS_rgb);
		break;
	case 12: read_geometry_from_image<RGBImageTypeSI>(v, v->pSI_rgb);
		break;
	case 13: read_geometry_from_image<RGBImageTypeUI>(v, v->pUI_rgb);
		break;
	case 14: read_geometry_from_image<RGBImageTypeUC>(v, v->pUC_rgb);
		break;
	case 15: read_geometry_from_image<RGBImageTypeF>(v, v->pF_rgb);
		break;
	case 16: read_geometry_from_image<RGBImageTypeD>(v, v->pD_rgb);
		break;
	case 20: read_geometry_from_image<RGBAImageTypeSS>(v, v->pSS_rgba);
		break;
	case 21: read_geometry_from_image<RGBAImageTypeUS>(v, v->pUS_rgba);
		break;
	case 22: read_geometry_from_image<RGBAImageTypeSI>(v, v->pSI_rgba);
		break;
	case 23: read_geometry_from_image<RGBAImageTypeUI>(v, v->pUI_rgba);
		break;
	case 24: read_geometry_from_image<RGBAImageTypeUC>(v, v->pUC_rgba);
		break;
	case 25: read_geometry_from_image<RGBAImageTypeF>(v, v->pF_rgba);
		break;
	case 26: read_geometry_from_image<RGBAImageTypeD>(v, v->pD_rgba);
		break;
	default:
		break;
	}
}

void CommonUtils::save_total_memory()
{
	saved_total_memory = get_total_memory();
}

double CommonUtils::get_total_memory_saved()
{
	return saved_total_memory;
}

double CommonUtils::get_total_memory()
{
#ifdef USE_GET_TOTAL_MEM
	double total_gb{};
#if (defined _WIN32)
	MEMORYSTATUSEX memory_status;
	ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
	memory_status.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memory_status))
	{
		total_gb = static_cast<double>(static_cast<long double>(memory_status.ullTotalPhys) / 1073741824.0L);
	}
#elif (defined __FreeBSD__ || defined __APPLE__)
	unsigned long long ctlvalue;
	size_t len = sizeof(ctlvalue);
	int mib[2];
	mib[0] = CTL_HW;
#if (defined __FreeBSD__)
	mib[1] = HW_REALMEM;
#elif (defined __APPLE__)
	mib[1] = HW_MEMSIZE;
#endif
	sysctl(mib, 2, &ctlvalue, &len, nullptr, 0);
	total_gb = static_cast<double>(static_cast<long double>(ctlvalue) / 1073741824.0L);
#elif (defined  __GNUC__)
	struct sysinfo i;
	sysinfo(&i);
	total_gb = static_cast<double>(static_cast<long double>(i.totalram) / 1073741824.0L);
#endif
	return total_gb;
#else
	return 0.0;
#endif
}

QString CommonUtils::get_screenshot_dir()
{
	if (screenshot_dir.isEmpty())
	{
#ifdef _WIN32
		return (QDir::homePath() + QString("/") + QString("Desktop"));
#else
		return QDir::homePath();
#endif
	}
	return screenshot_dir;
}

void CommonUtils::set_screenshot_dir(const QString & s)
{
	screenshot_dir = s;
}

QString CommonUtils::get_screenshot_name(const QString & s)
{
	const QString d = (s.isEmpty())
		?
		QDateTime::currentDateTime().toString(QString("yyyyMMdd-hhmmss")) +
			QString(".png")
		:
		s + QString("/") + QDateTime::currentDateTime().toString(
				QString("yyyyMMdd-hhmmss")) + QString(".png");
	return d;
}

QString CommonUtils::get_screenshot_name2()
{
	return (QDateTime::currentDateTime().toString(QString("yyyyMMdd-hhmmss")));
}

QString CommonUtils::get_save_dir()
{
	if (save_dir.isEmpty())
	{
#ifdef _WIN32
		return (QDir::homePath() + QString("/") + QString("Desktop"));
#else
		return QDir::homePath();
#endif
	}
	return save_dir;
}

void CommonUtils::set_save_dir(const QString & s)
{
	save_dir = s;
}

QString CommonUtils::get_save_name()
{
	return QDateTime::currentDateTime().toString(QString("yyyyMMdd-hhmmss"));
}

QString CommonUtils::get_open_dir()
{
	return open_dir;
}

void CommonUtils::set_open_dir(const QString & s)
{
	open_dir = s;
}

QString CommonUtils::apply_per_slice_rescale(
	ImageVariant * ivariant,
	const QList< QPair<double, double> > & rescale_values)
{
	if (!ivariant) return QString("!ivariant");
	const short image_type = ivariant->image_type;
	if (!(image_type >= 0 && image_type < 10)) return QString("");
	bool float64{};
	const double float_max = (1 << FLT_MANT_DIG);
	if (ivariant->sop == QString("1.2.840.10008.5.1.4.1.1.128.1") ||
		ivariant->sop == QString(""))
	{
		float64 = true;
	}
	else
	{
		for (int x = 0; x < rescale_values.size(); ++x)
		{
			if (((abs(ivariant->di->vmax)*rescale_values.at(x).second +
					rescale_values.at(x).first) > float_max) ||
				((abs(ivariant->di->vmin)*rescale_values.at(x).second +
					rescale_values.at(x).first) > float_max))
			{
				float64 = true;
				break;
			}
		}
	}
	QString s;
	switch (image_type)
	{
	case 0:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeSS, ImageTypeD>(
				ivariant->pSS, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeSS, ImageTypeF>(
				ivariant->pSS, ivariant->pF, rescale_values);
		break;
	case 1:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeUS, ImageTypeD>(
				ivariant->pUS, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeUS, ImageTypeF>(
				ivariant->pUS, ivariant->pF, rescale_values);
		break;
	case 2:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeSI, ImageTypeD>(
				ivariant->pSI, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeSI, ImageTypeF>(
				ivariant->pSI, ivariant->pF, rescale_values);
		break;
	case 3:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeUI, ImageTypeD>(
				ivariant->pUI, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeUI, ImageTypeF>(
				ivariant->pUI, ivariant->pF, rescale_values);
		break;
	case 4:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeUC, ImageTypeD>(
				ivariant->pUC, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeUC, ImageTypeF>(
				ivariant->pUC, ivariant->pF, rescale_values);
		break;
	case 5:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeF, ImageTypeD>(
				ivariant->pF, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeF, ImageTypeF>(
				ivariant->pF, ivariant->pF, rescale_values);
		break;
	case 6:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeD, ImageTypeD>(
				ivariant->pD, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeD, ImageTypeF>(
				ivariant->pD, ivariant->pF, rescale_values);
		break;
	case 7:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeSLL, ImageTypeD>(
				ivariant->pSLL, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeSLL, ImageTypeF>(
				ivariant->pSLL, ivariant->pF, rescale_values);
		break;
	case 8:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeULL, ImageTypeD>(
				ivariant->pULL, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeULL, ImageTypeF>(
				ivariant->pULL, ivariant->pF, rescale_values);
		break;
	default:
		return QString("");
	}
	if (float64)
	{
		ivariant->image_type = 6;
		ivariant->di->bits_allocated = 64;
		ivariant->di->bits_stored = 64;
		ivariant->di->high_bit = 63;
	}
	else
	{
		ivariant->image_type = 5;
		ivariant->di->bits_allocated = 32;
		ivariant->di->bits_stored = 32;
		ivariant->di->high_bit = 31;
	}
	return s;
}

void CommonUtils::get_pixel_values(
	const QList<ImageVariant*> & images,
	const int x,
	const int y,
	const int z,
	QList<double> & values)
{
	for (int i = 0; i < images.size(); ++i)
	{
		if (!images.at(i)) continue;
		const short image_type = images.at(i)->image_type;
		double d{};
		switch (image_type)
		{
		case 0:
			d = get_value<ImageTypeSS>(images.at(i)->pSS, x, y, z);
			break;
		case 1:
			d = get_value<ImageTypeUS>(images.at(i)->pUS, x, y, z);
			break;
		case 2:
			d = get_value<ImageTypeSI>(images.at(i)->pSI, x, y, z);
			break;
		case 3:
			d = get_value<ImageTypeUI>(images.at(i)->pUI, x, y, z);
			break;
		case 4:
			d = get_value<ImageTypeUC>(images.at(i)->pUC, x, y, z);
			break;
		case 5:
			d = get_value<ImageTypeF>(images.at(i)->pF, x, y, z);
			break;
		case 6:
			d = get_value<ImageTypeD>(images.at(i)->pD, x, y, z);
			break;
		case 7:
			d = get_value<ImageTypeSLL>(images.at(i)->pSLL, x, y, z);
			break;
		case 8:
			d = get_value<ImageTypeULL>(images.at(i)->pULL, x, y, z);
			break;
		default:
			values.clear();
			return;
		}
		values.push_back(d);
	}
}

double CommonUtils::calculate_max_delta(const ImageVariant * v)
{
	if (!v) return 1000.0;
	double max_delta{};
	if (v->image_type == 300)
	{
		max_delta = 1000.0;
	}
	else if (v->image_type == 100)
	{
		double tmp0{};
		for (int x = 0; x < v->di->rois.size(); ++x)
		{
			if (v->di->rois.at(x).max_delta > tmp0)
				tmp0 = v->di->rois.at(x).max_delta;
		}
		max_delta = tmp0;
	}
	else if (v->image_type >= 0 && v->image_type <= 26)
	{
		const double delta_x = v->di->idimx * v->di->ix_spacing;
		const double delta_y = v->di->idimy * v->di->iy_spacing;
		const double delta_z = v->di->idimz * v->di->iz_spacing;
		const double tmp0 = (delta_x > delta_y) ? delta_x : delta_y;
		max_delta = (tmp0 > delta_z) ? tmp0 : delta_z;
	}
	if (max_delta <= 0) max_delta = 1000.0;
	return max_delta;
}

void CommonUtils::random_RGB(float * R, float * G, float * B)
{
	const unsigned long long seed =
		std::chrono::high_resolution_clock::now().time_since_epoch().count();
	const double H = random_range(0.0, 3600.0, seed);
	const double S = random_range(0.5,    1.0, seed / 7);
	const double V = random_range(0.4,    1.0, seed / 3);
	double R_, G_, B_;
	ColorSpace_::Hsv2Rgb(&R_, &G_, &B_, 0.1 * H, S, V);
	*R = static_cast<float>(R_);
	*G = static_cast<float>(G_);
	*B = static_cast<float>(B_);
}

// clang-format off
int CommonUtils::get_reference_count(const ImageVariant * v)
{
	if (!v)
	{
#ifdef ALIZA_VERBOSE
		std::cout << "Image is null (ref. count not possible)" << std::endl;
#endif
		return -1;
	}
	int x{};
	bool b{};
	if (v->pSS.IsNotNull())      {                       x = v->pSS->GetReferenceCount();     }
	if (v->pUS.IsNotNull())      {if (x > 0) {b = true;} x = v->pUS->GetReferenceCount();     }
	if (v->pSI.IsNotNull())      {if (x > 0) {b = true;} x = v->pSI->GetReferenceCount();     }
	if (v->pUI.IsNotNull())      {if (x > 0) {b = true;} x = v->pUI->GetReferenceCount();     }
	if (v->pUC.IsNotNull())      {if (x > 0) {b = true;} x = v->pUC->GetReferenceCount();     }
	if (v->pF.IsNotNull())       {if (x > 0) {b = true;} x = v->pF->GetReferenceCount();      }
	if (v->pD.IsNotNull())       {if (x > 0) {b = true;} x = v->pD->GetReferenceCount();      }
	if (v->pSLL.IsNotNull())     {if (x > 0) {b = true;} x = v->pSLL->GetReferenceCount();    }
	if (v->pULL.IsNotNull())     {if (x > 0) {b = true;} x = v->pULL->GetReferenceCount();    }
	if (v->pSS_rgb.IsNotNull())  {if (x > 0) {b = true;} x = v->pSS_rgb->GetReferenceCount(); }
	if (v->pUS_rgb.IsNotNull())  {if (x > 0) {b = true;} x = v->pUS_rgb->GetReferenceCount(); }
	if (v->pSI_rgb.IsNotNull())  {if (x > 0) {b = true;} x = v->pSI_rgb->GetReferenceCount(); }
	if (v->pUI_rgb.IsNotNull())  {if (x > 0) {b = true;} x = v->pUI_rgb->GetReferenceCount(); }
	if (v->pUC_rgb.IsNotNull())  {if (x > 0) {b = true;} x = v->pUC_rgb->GetReferenceCount(); }
	if (v->pF_rgb.IsNotNull())   {if (x > 0) {b = true;} x = v->pF_rgb->GetReferenceCount();  }
	if (v->pD_rgb.IsNotNull())   {if (x > 0) {b = true;} x = v->pD_rgb->GetReferenceCount();  }
	if (v->pSS_rgba.IsNotNull()) {if (x > 0) {b = true;} x = v->pSS_rgba->GetReferenceCount();}
	if (v->pUS_rgba.IsNotNull()) {if (x > 0) {b = true;} x = v->pUS_rgba->GetReferenceCount();}
	if (v->pSI_rgba.IsNotNull()) {if (x > 0) {b = true;} x = v->pSI_rgba->GetReferenceCount();}
	if (v->pUI_rgba.IsNotNull()) {if (x > 0) {b = true;} x = v->pUI_rgba->GetReferenceCount();}
	if (v->pUC_rgba.IsNotNull()) {if (x > 0) {b = true;} x = v->pUC_rgba->GetReferenceCount();}
	if (v->pF_rgba.IsNotNull())  {if (x > 0) {b = true;} x = v->pF_rgba->GetReferenceCount(); }
	if (v->pD_rgba.IsNotNull())  {if (x > 0) {b = true;} x = v->pD_rgba->GetReferenceCount(); }
#ifdef ALIZA_VERBOSE
	std::cout << "Ref. count = " << x << (b ? ", multiple images" : " ") << std::endl;
#else
	(void)b;
#endif
	return x;
}
// clang-format on

#ifdef ALIZA_LINUX_DEBUG_MEM
void CommonUtils::linux_print_memusage(const std::string & s)
{
   std::ifstream stat_stream("/proc/self/stat", std::ios_base::in);
   // Vars for leading unused entries
   std::string pid, comm, state, ppid, pgrp, session, tty_nr;
   std::string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   std::string utime, stime, cutime, cstime, priority, nice;
   std::string O, itrealvalue, starttime;
   // The two used fields
   unsigned long long vsize;
   long long rss;
   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest
   stat_stream.close();
   const unsigned long long page_size_kb = sysconf(_SC_PAGE_SIZE);
   const long double vm_usage = vsize / 1073741824.0L;
   const long double resident_set = (rss * page_size_kb) / 1073741824.0L;
   std::cout << s << ": VM = " << vm_usage << " GB, RSS = " << resident_set << " GB " << std::endl;
}
#endif

#ifdef USE_GET_TOTAL_MEM
#undef USE_GET_TOTAL_MEM
#endif

#ifdef TRY_SUPPORT_YBR_PARTIAL_422
#undef TRY_SUPPORT_YBR_PARTIAL_422
#endif
