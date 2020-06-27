#include "structures.h"
#include "commonutils.h"
#include "itkVersion.h"
#include "itkIndex.h"
#include "itkImageRegionIterator.h"
#include "itkMapContainer.h"
#include "itkSpatialOrientation.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkImageSliceConstIteratorWithIndex.h"
#include "itkImageSliceIteratorWithIndex.h"
#include "itkResampleImageFilter.h"
#include "itkIdentityTransform.h"
#include <vnl/vnl_vector_fixed.h>
#include <QSet>
#include <QApplication>
#include <QFileInfo>
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>
#include "settingswidget.h"
#include "iconutils.h"
#include "updateqtcommand.h"
#include <iostream>
#include <list>
#include <cstdlib>
#include "dicomutils.h"
#include "colorspace/colorspace.h"
#if (defined  __FreeBSD__)
#include <sys/sysctl.h>
#elif (defined  __GNUC__)
#include <sys/sysinfo.h>
#endif
#if QT_VERSION >= 0x050000
#include "CG/glwidget-qt5.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#include "vectormath/scalar/vectormath.h"

typedef Vectormath::Scalar::Vector3 sVector3;
typedef Vectormath::Scalar::Point3  sPoint3;
typedef Vectormath::Scalar::Vector4 sVector4;
typedef Vectormath::Scalar::Matrix4 sMatrix4;
typedef itk::Image<RGBPixelUS,4> RGBImage4DTypeUS;
typedef itk::Image<RGBPixelUC,4> RGBImage4DTypeUC;
typedef itk::Image<RGBPixelF, 4> RGBImage4DTypeF;
typedef itk::Image<RGBPixelD, 4> RGBImage4DTypeD;

static QString screenshot_dir("");
static QString save_dir("");
static QString open_dir("");

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
	double cubemin = 0.0, cubemax = 0.0;
	try
	{
		min_max_calculator->AddObserver(
			itk::ProgressEvent(), update_qt_command);
		min_max_calculator->SetImage(image);
		min_max_calculator->SetRegion(image->GetLargestPossibleRegion());
		min_max_calculator->Compute();
		cubemin =
			static_cast<double>(min_max_calculator->GetMinimum());
		cubemax =
			static_cast<double>(min_max_calculator->GetMaximum());
	}
	catch (itk::ExceptionObject & ex)
	{
		std::cout << ex << std::endl;
		return;
	}
	if (iv->di->maxwindow)
	{
		switch (iv->image_type)
		{
		case 0:
			{
				if (iv->di->bits_allocated>0 && iv->di->bits_stored>0 &&
					(iv->di->bits_stored<iv->di->bits_allocated) &&
					(iv->di->high_bit==iv->di->bits_stored-1))
				{
					iv->di->rmin = -(double)pow(2, (iv->di->bits_stored-1))-1;
					iv->di->rmax =  (double)pow(2, (iv->di->bits_stored-1))-1;
				}
				else
				{
					iv->di->rmin = (double)SHRT_MIN;
					iv->di->rmax = (double)SHRT_MAX;
				}
			}
			break;
		case 1:
			{
				if (iv->di->bits_allocated>0 && iv->di->bits_stored>0 &&
					(iv->di->bits_stored<iv->di->bits_allocated) &&
					(iv->di->high_bit==iv->di->bits_stored-1))
				{
					iv->di->rmin = 0;
					iv->di->rmax = (double)pow(2, (iv->di->bits_stored))-1;
				}
				else
				{
					iv->di->rmin = 0.0;
					iv->di->rmax = (double)USHRT_MAX;
				}
			}
			break;
		case 4:
			{
				if (iv->di->bits_allocated>0 && iv->di->bits_stored>0 &&
					(iv->di->bits_stored<iv->di->bits_allocated) &&
					(iv->di->high_bit==iv->di->bits_stored-1))
				{
					iv->di->rmin = 0.0;
					iv->di->rmax = (double)pow(2, (iv->di->bits_stored))-1;
				}
				else
				{
					iv->di->rmin = 0.0;
					iv->di->rmax = (double)UCHAR_MAX;
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
		iv->di->rmin = iv->di->vmin = cubemin;
		iv->di->rmax = iv->di->vmax = cubemax;
	}
	const double vmax_minus_vmin = iv->di->vmax-iv->di->vmin;
	const double rmax_minus_rmin = iv->di->rmax-iv->di->rmin;
	if ((iv->di->us_window_center <= -999999.0 && iv->di->us_window_width <= -999999.0) ||
		((iv->di->default_us_window_width > (iv->di->vmax - iv->di->vmin)) ||
			(iv->di->default_us_window_center > iv->di->vmax ||
				iv->di->default_us_window_center < iv->di->vmin)))
	{
		if (iv->image_type == 4)
		{
			iv->di->default_us_window_center = iv->di->us_window_center = 128.0;
			iv->di->default_us_window_width  = iv->di->us_window_width  = 255.0;
		}
		else
		{
			const double tmp110 = (vmax_minus_vmin>0) ? vmax_minus_vmin : rmax_minus_rmin;
			const double tmp111 = (vmax_minus_vmin>0) ? iv->di->vmin    : iv->di->rmin;
			iv->di->default_us_window_center = iv->di->us_window_center = ((tmp110/2.0)-(-tmp111));
			iv->di->default_us_window_width  = iv->di->us_window_width  = tmp110;
		}
	}
	iv->di->window_center =
		(iv->di->us_window_center+(-iv->di->rmin))/rmax_minus_rmin;
	iv->di->window_width  = iv->di->us_window_width/rmax_minus_rmin;
	if (iv->di->window_width  <= 0) iv->di->window_width  = 1e-6;
	if (iv->di->window_width   > 1) iv->di->window_width  = 1.0;
	if (iv->di->window_center  < 0) iv->di->window_center = 0.0;
	if (iv->di->window_center  > 1) iv->di->window_center = 1.0;
	if ((iv->di->rmax-iv->di->rmin) < 3.0) iv->di->disable_int_level = true;
	else iv->di->disable_int_level = false;
}

template <typename T> void calculate_rgb_minmax_(
	const typename T::Pointer image,
	ImageVariant * ivariant)
{
	if (image.IsNull()) return;
	const typename T::RegionType region =
		image->GetLargestPossibleRegion();
	double max_r = std::numeric_limits<double>::min();
	double max_g = std::numeric_limits<double>::min();
	double max_b = std::numeric_limits<double>::min();
	double min_r = std::numeric_limits<double>::max();
	double min_g = std::numeric_limits<double>::max();
	double min_b = std::numeric_limits<double>::max();
	itk::ImageRegionConstIterator<T> iterator(image, region);
	iterator.GoToBegin();
	while(!iterator.IsAtEnd())
	{
		const double b =
			static_cast<double>(iterator.Get().GetBlue());
		const double g =
			static_cast<double>(iterator.Get().GetGreen());
		const double r =
			static_cast<double>(iterator.Get().GetRed());
		if (b > max_b) max_b = b;
		if (g > max_g) max_g = g;
		if (r > max_r) max_r = r;
		if (b < min_b) min_b = b;
		if (g < min_g) min_g = g;
		if (r < min_r) min_r = r;
 		++iterator;
	}
	double min_ = std::numeric_limits<double>::max();
	double mins[3];
	mins[0] = min_r;
	mins[1] = min_g;
	mins[2] = min_b;
	for (unsigned int x = 0; x < 3; x++)
	{
		if (mins[x] < min_) min_ = mins[x];
	}
	double max_ = std::numeric_limits<double>::min();
	double maxs[3];
	maxs[0] = max_r;
	maxs[1] = max_g;
	maxs[2] = max_b;
	for (unsigned int x = 0; x < 3; x++)
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
	const typename T::RegionType region =
		image->GetLargestPossibleRegion();
	double max_r = std::numeric_limits<double>::min();
	double max_g = std::numeric_limits<double>::min();
	double max_b = std::numeric_limits<double>::min();
	double max_a = std::numeric_limits<double>::min();
	double min_r = std::numeric_limits<double>::max();
	double min_g = std::numeric_limits<double>::max();
	double min_b = std::numeric_limits<double>::max();
	double min_a = std::numeric_limits<double>::max();
	itk::ImageRegionConstIterator<T> iterator(image, region);
	iterator.GoToBegin();
	while(!iterator.IsAtEnd())
	{
		const double a =
			static_cast<double>(iterator.Get().GetAlpha());
		const double b =
			static_cast<double>(iterator.Get().GetBlue());
		const double g =
			static_cast<double>(iterator.Get().GetGreen());
		const double r =
			static_cast<double>(iterator.Get().GetRed());
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
	double min_ = std::numeric_limits<double>::max();
	double mins[4];
	mins[0] = min_r;
	mins[1] = min_g;
	mins[2] = min_b;
	mins[3] = min_a;
	for (unsigned int x = 0; x < 4; x++)
	{
		if (mins[x] < min_) min_ = mins[x];
	}
	double max_ = std::numeric_limits<double>::min();
	double maxs[4];
	maxs[0] = max_r;
	maxs[1] = max_g;
	maxs[2] = max_b;
	maxs[3] = max_a;
	for (unsigned int x = 0; x < 4; x++)
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
	catch (itk::ExceptionObject & ex)
	{
		std::cout << ex << std::endl;
		return;
	}
	*dimx = size[0];
	*dimy = size[1];
	*dimz = size[2];
	*spacingx = spacing[0];
	*spacingy = spacing[1];
	*spacingz = spacing[2];
	*originx  = (float)origin[0];
	*originy  = (float)origin[1];
	*originz  = (float)origin[2];
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
		CommonUtils::convert_orientation_flag(
			adapter.FromDirectionCosines(image->GetDirection()));
	return f;
}

template<typename T> int generate_tex3d(
	ImageVariant * ivariant,
	const typename T::Pointer & image,
	unsigned int * size, double * spacing,
	QProgressDialog * pb,
	GLWidget * gl)
{
	if (image.IsNull()||!ivariant||!gl) return 1;
	if (size[0] < 1||size[1] < 1)
	{
		std::cout << "(size[0] < 1||size[1] < 1)" << std::endl;
		return 1;
	}
	typedef itk::NearestNeighborInterpolateImageFunction<T,double> InterpolatorType;
	typedef itk::IdentityTransform<double,3> IdentityTransformType;
	typedef itk::ResampleImageFilter<T,T> ScaleFilter;
	typedef itk::ImageSliceConstIteratorWithIndex<T> SliceConstIteratorType;
	std::string tt;
	int error__ = 0;
	GLuint glerror__ = 0;
	double rmin = 0.0, rmax = 0.0;
	float * float_buf = NULL;
	unsigned short * short_buf = NULL;
	GLubyte * ub_buf = NULL;
	typename T::Pointer out_image;
	bool scale = true;
	short texture_type = -1;
	const typename T::RegionType r__ =
			image->GetLargestPossibleRegion();
	const typename T::SizeType original_size = r__.GetSize();
	calculate_min_max<T>(image, ivariant);
	rmin = ivariant->di->rmin;
	rmax = ivariant->di->rmax;
	switch(ivariant->image_type)
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
		switch(texture_type)
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
		pb->setValue(-1);
	}
	qApp->processEvents();
	//
	if (
		size[0]==original_size[0] &&
		size[1]==original_size[1] &&
		size[2]==original_size[2]) scale = false;
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
		catch (itk::ExceptionObject & ex)
		{
			std::cout << ex << std::endl;
			return 1;
		}
		if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	}
	else { out_image = image; }
	//
	if (out_image.IsNotNull())
	{
		const typename T::SpacingType final_spacing = out_image->GetSpacing();
		ivariant->di->x_spacing = final_spacing[0];
		ivariant->di->y_spacing = final_spacing[1];
		ivariant->di->dimx = size[0]; ivariant->di->dimy = size[1];
	}
	else
	{
		std::cout << "out_image.IsNull()" << std::endl;
		return 1;
	}
	//
	if (pb) pb->setValue(-1);
	qApp->processEvents();
	//
	// array maximum size 0x7fffffff
	switch(texture_type)
	{
	case 0:
		{
			if ((size[0]*size[1]*size[2])>= 0x7fffffff/sizeof(float))
				return 2;
			try	{ float_buf=new float[(size[0]*size[1]*size[2])]; }
			catch (std::bad_alloc&) { float_buf = NULL; }
			if (!float_buf) return 2;
			tt = " GL_R16F";
		}
		break;
	case 1:
		{
			if ((size[0]*size[1]*size[2]) >= 0x7fffffff/sizeof(unsigned short))
				return 2;
			try	{ short_buf=new unsigned short[(size[0]*size[1]*size[2])]; }
			catch (std::bad_alloc&) { short_buf=NULL; }
			if (!short_buf) return 2;
			tt = " GL_R16";
		}
		break;
	case 2:
		{
			if ((size[0]*size[1]*size[2]) >= 0x7fffffff/sizeof(GLubyte))
				return 2;
			try	{ ub_buf=new GLubyte[(size[0]*size[1]*size[2])]; }
			catch (std::bad_alloc&) { ub_buf = NULL; }
			if (!ub_buf) return 2;
			tt = " GL_R8";
		}
		break;
	default: return 1;
	}
	//
	if (pb) pb->setValue(-1);
	qApp->processEvents();
	//
	{
		SliceConstIteratorType inIterator(
			out_image,
			out_image->GetLargestPossibleRegion());
		inIterator.SetFirstDirection(0);  // X axis
		inIterator.SetSecondDirection(1); // Y axis
		inIterator.GoToBegin();
		int j = 0;
		const double max_minus_min =
			(rmax-rmin > 0) ? rmax-rmin : 1e-9;
		while(!inIterator.IsAtEnd())
		{
			while (!inIterator.IsAtEndOfSlice())
			{
				while (!inIterator.IsAtEndOfLine())
				{
					const typename T::PixelType v = inIterator.Get();
					const double f = static_cast<const double>(v);
					// GL_R16F
					if (texture_type == 0)
						float_buf[j] = static_cast<float>((f+(-rmin))/max_minus_min);
					// GL_R16
					else if(texture_type == 1)
						short_buf[j] = static_cast<unsigned short>(
							(double)USHRT_MAX*((f+(-rmin))/max_minus_min));
					// GL_R8
					else if(texture_type == 2)
						ub_buf[j] = static_cast<GLubyte>(
							(double)UCHAR_MAX*((f+(-rmin))/max_minus_min));
					j+=1;
					++inIterator;
				}
				inIterator.NextLine();
			}
			inIterator.NextSlice();
		}
	}
	//
	if (pb) pb->setValue(-1);
	qApp->processEvents();
	//
	gl->makeCurrent();
#if 0
	if (!gl->isValid())
	{
		std::cout << "gl->isValid()=" << std::endl;
	}
#endif
	glerror__ = glGetError();
#if 0
	if (glerror__ != 0)
	{
		std::cout
			<< "warning : OpenGL error (before texture generation)\n"
			<< glerror__ << std::endl;
	}
#endif
#if QT_VERSION >= 0x050000
	gl->glGenTextures(1, &(ivariant->di->cube_3dtex));
	glBindTexture(GL_TEXTURE_3D, ivariant->di->cube_3dtex);
	switch(ivariant->di->filtering)
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
	switch(ivariant->di->filtering)
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
#if QT_VERSION >= 0x050000
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
#if QT_VERSION >= 0x050000
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
#if QT_VERSION >= 0x050000
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
	default: break;
	}
	//
#if QT_VERSION >= 0x050000
	glerror__ = gl->glGetError();
#else
	glerror__ = glGetError();
#endif
	if (glerror__ == 0x505)
	{
#if 0
		std::cout
			<< "error : OpenGL error 0x505\n"
			<< glerror__ << std::endl;
#endif
#if QT_VERSION >= 0x050000
		gl->glBindTexture(GL_TEXTURE_3D, 0);
		gl->glDeleteTextures(1, &(ivariant->di->cube_3dtex));
#else
		glBindTexture(GL_TEXTURE_3D, 0);
		glDeleteTextures(1, &(ivariant->di->cube_3dtex));
#endif
		ivariant->di->cube_3dtex = 0;
		ivariant->di->tex_info = -1;
		error__=3;
		goto quit__;
	}
	GLWidget::increment_count_tex(1);
	if (glerror__ != 0)
	{
		std::cout
			<< "warning : OpenGL error\n"
			<< glerror__ << std::endl;
	}
	//
	if (pb) pb->setValue(-1);
	qApp->processEvents();
	//
quit__:
	if (float_buf) delete [] float_buf;
	if (short_buf) delete [] short_buf;
	if (ub_buf)    delete [] ub_buf; 
	return error__;
}

template<typename T> void read_geometry_from_image(
	ImageVariant * ivariant,
	const typename T::Pointer & image)
{
	if (!ivariant) return;
	if (image.IsNull()) return;
	const typename T::SizeType size =
		image->GetLargestPossibleRegion().GetSize();
	sVector3 first     =  sVector3(0.0f,0.0f,0.0f);
	sVector3 last      =  sVector3(0.0f,0.0f,0.0f);
	sVector3 direction =  sVector3(0.0f,0.0f,0.0f);
	sVector3 up        =  sVector3(0.0f,0.0f,0.0f);
	const typename T::DirectionType dircos =
		image->GetDirection();
	const double d1 = (double)dircos[0][0];
	const double d2 = (double)dircos[1][0];
	const double d3 = (double)dircos[2][0];
	const double d4 = (double)dircos[0][1];
	const double d5 = (double)dircos[1][1];
	const double d6 = (double)dircos[2][1];
	for (unsigned int z=0; z < size[2]; z++)
	{
		typename T::IndexType idx0, idx1, idx2, idx3;
		itk::Point<float,3> p0, p1, p2, p3;
		idx0[0]=0;         idx0[1]=size[1]-1; idx0[2]=z;
		idx1[0]=0;         idx1[1]=0;         idx1[2]=z;
		idx2[0]=size[0]-1; idx2[1]=size[1]-1; idx2[2]=z;
		idx3[0]=size[0]-1; idx3[1]=0;         idx3[2]=z;
		try
		{
			image->TransformIndexToPhysicalPoint(idx0, p0);
			image->TransformIndexToPhysicalPoint(idx1, p1);
			image->TransformIndexToPhysicalPoint(idx2, p2);
			image->TransformIndexToPhysicalPoint(idx3, p3);
		}
		catch (itk::ExceptionObject & ex)
		{
			std::cout << ex << std::endl;
			continue;
		}
		float x0 = p0[0], y0 = p0[1], z0 = p0[2];
		float x1 = p1[0], y1 = p1[1], z1 = p1[2];
		float x2 = p2[0], y2 = p2[1], z2 = p2[2];
		float x3 = p3[0], y3 = p3[1], z3 = p3[2];
		if (z==0)
		{
			first = sVector3(x0,y0,z0);
			sVector3 tmp_up0 = sVector3(x1,y1,z1);
			sVector3 tmp_up1 = sVector3(x0,y0,z0);
			up = sVector3(normalize(tmp_up1-tmp_up0));
		}
		else if (z==size[2]-1)
		{
			last = sVector3(x0,y0,z0);
		}
		const double ipp_iop[9] = {
			(double)x1, (double)y1, (double)z1,
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
	ivariant->equi = true;
	ivariant->di->slices_generated = true;
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
	idx[0]=size[0];
	idx[1]=size[1];
	idx[2]=size[2];
	itk::Point<float,3> p;
	try
	{
		image->TransformIndexToPhysicalPoint(idx, p);
	}
	catch (itk::ExceptionObject & ex)
	{
		std::cout << ex << std::endl;
		return;
	}
	sVector3 v0 = sVector3((float)origin[0], (float)origin[1], (float)origin[2]);
	sVector3 v1 = sVector3(p[0], p[1], p[2]);
	sVector3 cube_center = sVector3((v0+v1)*0.5f);
	ivariant->di->default_center_x = ivariant->di->center_x = cube_center.getX();
	ivariant->di->default_center_y = ivariant->di->center_y = cube_center.getY();
	ivariant->di->default_center_z = ivariant->di->center_z = cube_center.getZ();
}

template <typename T> bool reload_monochrome_image(
	ImageVariant * ivariant,
	const typename T::Pointer & image,
	GLWidget * gl,
	int max_3d_tex_size,
	QProgressDialog * pb,
	bool resize=false,
	unsigned int size_x_=0,
	unsigned int size_y_=0,
	bool disable_gen_slices=false)
{
	if (!ivariant||image.IsNull()) return false;
	const bool generate_slices =
		(!disable_gen_slices &&
			!ivariant->di->slices_generated);
	const bool calc_center =
		(!disable_gen_slices &&
			!ivariant->di->slices_generated);
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
	unsigned int size_x, size_y, size_z;
	unsigned int isize[3];
	double dspacing[3];
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::SpacingType spacing;
	short count__ = 0;
	double fx = 0.0, fy = 0.0, fz = 0.0;
	ivariant->di->close(generate_slices);
	region  = image->GetLargestPossibleRegion();
	size    = region.GetSize();
	spacing = image->GetSpacing();
	//
	if (max_3d_tex_size > 0 &&
		max_3d_tex_size < (int)size[2])
		ivariant->di->skip_texture = true;
	if (size[0]==1 || size[1]==1)
		ivariant->di->skip_texture = true;
	const bool ok3d =
		(max_3d_tex_size > 0 &&
		!ivariant->di->skip_texture &&
		ivariant->di->opengl_ok
		&& gl);
	//
	if (generate_slices)
		read_geometry_from_image<T>(ivariant,image);
	if (calc_center)
		calc_center_from_image<T>(ivariant,image);
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
	if (size_x>(unsigned int)max_3d_tex_size)
		size_x = (unsigned int)max_3d_tex_size;
	if (size_y>(unsigned int)max_3d_tex_size)
		size_y = (unsigned int)max_3d_tex_size;
	if (size_z>(unsigned int)max_3d_tex_size)
		size_z = (unsigned int)max_3d_tex_size;
	fx = (double)size_x/(double)size[0];
	size[0] *= fx;
	spacing[0] *= 1.0/fx;
	fy = (double)size_y/(double)size[1];
	size[1] *= fy;
	spacing[1] *= 1.0/fy;
	fz = (double)size_z/(double)size[2];
	size[2] *= fz;
	spacing[2] *= 1.0/fz;
	isize[0] = static_cast<int>(size[0]);
	isize[1] = static_cast<int>(size[1]);
	isize[2] = static_cast<int>(size[2]);
	dspacing[0] = static_cast<double>(spacing[0]);
	dspacing[1] = static_cast<double>(spacing[1]);
	dspacing[2] = static_cast<double>(spacing[2]);
	//
	bool ok = false;
	if (ok3d)
	{
		while (!ok)
		{
			count__++;
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
				if (error__ == 2)
					std::cout <<
							"memory error (system)    "
							"... reducing texture size"
						<< std::endl;
				else if (error__ == 3)
					std::cout <<
							"memory error (graphics)  "
							"... reducing texture size"
						<< std::endl;
				else
					std::cout
						<< "error " << error__
						<< std::endl;
				isize[0]    *= 0.5;
				isize[1]    *= 0.5;
				dspacing[0] *= 2.0;
				dspacing[1] *= 2.0;
				ivariant->di->close(generate_slices);
				if (count__>64)
				{
					std::cout
						<< "exit from loop after "
						<< count__
						<< " iterations, x = "
						<< isize[0] << ", y = "
						<< isize[1] << std::endl;
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
		ivariant->orientation_string = get_orientation<T>(
			image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	if (ivariant->equi &&
		ivariant->orientation > 0 &&
		!ivariant->orientation_string.isEmpty())
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
	bool disable_gen_slices=false)
{
	if (!ivariant||image.IsNull()) return false;
	ivariant->di->skip_texture = true;
	const bool generate_slices =
		(!disable_gen_slices &&
			!ivariant->di->slices_generated);
	const bool calc_center =
		(!disable_gen_slices &&
			!ivariant->di->slices_generated);
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
		calc_center_from_image<T>(ivariant,image);
	//
	calculate_rgb_minmax_<T>(image, ivariant);
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(
			image, &ivariant->orientation);
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
	bool disable_gen_slices=false)
{
	if (!ivariant||image.IsNull()) return false;
	ivariant->di->skip_texture = true;
	const bool generate_slices =
		(!disable_gen_slices &&
			!ivariant->di->slices_generated);
	const bool calc_center =
		(!disable_gen_slices &&
			!ivariant->di->slices_generated);
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
		calc_center_from_image<T>(ivariant,image);
	//
	calculate_rgba_minmax_<T>(image, ivariant);
	if (ivariant->equi)
	{
		ivariant->orientation_string = get_orientation<T>(
			image, &ivariant->orientation);
	}
	else
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	return true;
}

template<typename T> QString process_dicom_monochrome_image(
	bool * ok,
	ImageVariant * ivariant,
	typename T::Pointer & image,
	char * buffer,
	itk::Matrix<itk::SpacePrecisionType,3,3> & direction,
	unsigned int dimx, unsigned int dimy, unsigned int dimz,
	double origin_x, double origin_y, double origin_z,
	double spacing_x, double spacing_y, double spacing_z,
	short image_type,
	int max_3d_tex_size,
	bool gen_vertices,
	QProgressDialog * pb,
	GLWidget * gl,
	bool resize_=false,
	unsigned int size_x_=0, unsigned int size_y_=0)
{
	if (!buffer)
	{
		*ok = false;
		return QString(
			"process_dicom_monochrome_image : buffer==NULL");
	}
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::IndexType start;
	typename T::PointType origin;
	typename T::SpacingType spacing;
	typedef itk::ImageSliceIteratorWithIndex<T> SliceIterator;
	typename UpdateQtCommand::Pointer update_qt_command;
	if (pb)
	{
		pb->setLabelText(QString("Loading data... please wait"));
		pb->setValue(-1);
	}
	qApp->processEvents();
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
	const bool bad_direction = (	
		direction[0][0]>-0.000001 && direction[0][0]<0.000001 &&
		direction[1][0]>-0.000001 && direction[1][0]<0.000001 &&
		direction[2][0]>-0.000001 && direction[2][0]<0.000001 &&
		direction[0][1]>-0.000001 && direction[0][1]<0.000001 &&
		direction[1][1]>-0.000001 && direction[1][1]<0.000001 &&
		direction[2][1]>-0.000001 && direction[2][1]<0.000001 &&
		direction[0][2]>-0.000001 && direction[0][2]<0.000001 &&
		direction[1][2]>-0.000001 && direction[1][2]<0.000001 &&
		direction[2][2]>-0.000001 && direction[2][2]<0.000001)
			? true : false;
	try
	{
		image = T::New();
		image->SetRegions(region);
		image->Allocate();
		image->SetOrigin(origin);
		image->SetSpacing(spacing);
		if (!bad_direction) image->SetDirection(direction);
	}
	catch (itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	//
	ivariant->image_type = image_type;
	const typename T::PixelType * p__ =
		reinterpret_cast<typename T::PixelType*>(buffer);
	//
	SliceIterator it(image, image->GetLargestPossibleRegion());
	it.SetFirstDirection(0);
	it.SetSecondDirection(1);
	it.GoToBegin();
	int j = 0;	
	while(!it.IsAtEnd())
	{
		while (!it.IsAtEndOfSlice())
		{
			while (!it.IsAtEndOfLine())
			{
				it.Set(p__[j]);
				j+=1;
				++it;
			}
			it.NextLine();
		}
		it.NextSlice();
	}
	*ok = reload_monochrome_image<T>(
		ivariant,
		image,
		gl,
		max_3d_tex_size,
		pb,
		resize_,
		size_x_,
		size_y_,
		!gen_vertices);
	if (bad_direction)
	{
		ivariant->equi = false;
		ivariant->orientation_string = QString("");
		ivariant->orientation = 0;
		for (unsigned int x = 0; x < ivariant->di->image_slices.size(); x++)
			 ivariant->di->image_slices[x]->slice_orientation_string =
				QString("");
	}
	return QString("");
}

template<typename T> QString process_dicom_rgb_image(
	bool * ok,
	ImageVariant * ivariant,
	typename T::Pointer & image,
	char * buffer,
	itk::Matrix<itk::SpacePrecisionType,3,3> & direction,
	unsigned int dimx, unsigned int dimy, unsigned int dimz,
	double origin_x, double origin_y, double origin_z,
	double spacing_x, double spacing_y, double spacing_z,
	short image_type,
	bool ybr, bool gen_vertices,
	QProgressDialog * pb)
{
	if (!buffer)
	{
		*ok = false;
		return QString(
			"process_dicom_rgb_image : buffer==NULL");
	}
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::IndexType start;
	typename T::PointType origin;
	typename T::SpacingType spacing;
	typedef itk::ImageSliceIteratorWithIndex<T> SliceIterator;
	if (pb)
	{
		pb->setLabelText(QString("Loading data... please wait"));
		pb->setValue(-1);
	}
	qApp->processEvents();
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
	const bool bad_direction = (	
		direction[0][0]>-0.000001 && direction[0][0]<0.000001 &&
		direction[1][0]>-0.000001 && direction[1][0]<0.000001 &&
		direction[2][0]>-0.000001 && direction[2][0]<0.000001 &&
		direction[0][1]>-0.000001 && direction[0][1]<0.000001 &&
		direction[1][1]>-0.000001 && direction[1][1]<0.000001 &&
		direction[2][1]>-0.000001 && direction[2][1]<0.000001 &&
		direction[0][2]>-0.000001 && direction[0][2]<0.000001 &&
		direction[1][2]>-0.000001 && direction[1][2]<0.000001 &&
		direction[2][2]>-0.000001 && direction[2][2]<0.000001)
			? true : false;
	try
	{
		image = T::New();
		image->SetRegions(region);
		image->Allocate();
		image->SetOrigin(origin);
		image->SetSpacing(spacing);
		if (!bad_direction) image->SetDirection(direction);
	}
	catch (itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	//
	const typename T::PixelType::ValueType * p__ =
		reinterpret_cast<typename T::PixelType::ValueType*>(buffer);
	ivariant->image_type = image_type;
	//
	SliceIterator it(image, image->GetLargestPossibleRegion());
	it.SetFirstDirection(0);
	it.SetSecondDirection(1);
	it.GoToBegin();
	int j = 0;
	while(!it.IsAtEnd())
	{
		while (!it.IsAtEndOfSlice())
		{
			while (!it.IsAtEndOfLine())
			{
				typename T::PixelType p;
				if (ybr)
				{
					const double Y  = static_cast<double>(p__[j  ]);
					const double Cb = static_cast<double>(p__[j+1]);
					const double Cr = static_cast<double>(p__[j+2]);
					int R = static_cast<int>((Y + 1.402*(Cr-128)) + 0.5);
					int G = static_cast<int>((Y - (0.114*1.772*(Cb-128) + 0.299*1.402*(Cr-128))/0.587) + 0.5);
					int B = static_cast<int>((Y + 1.772*(Cb-128)) + 0.5);
					if (R < 0) { R = 0; }; if (R > 255) { R = 255; }
					if (G < 0) { G = 0; }; if (G > 255) { G = 255; }
					if (B < 0) { B = 0; }; if (B > 255) { B = 255; }
					p[0]=static_cast<typename T::PixelType::ValueType>(R);
					p[1]=static_cast<typename T::PixelType::ValueType>(G);
					p[2]=static_cast<typename T::PixelType::ValueType>(B);
				}
				else
				{
					p[0]=static_cast<typename T::PixelType::ValueType>(p__[j  ]);
					p[1]=static_cast<typename T::PixelType::ValueType>(p__[j+1]);
					p[2]=static_cast<typename T::PixelType::ValueType>(p__[j+2]);
				}
				it.Set(p);
				j+=3;
				++it;
			}
			it.NextLine();
		}
		it.NextSlice();
	}
	*ok = reload_rgb_image<T>(
		image, ivariant, !gen_vertices);
	if (bad_direction)
	{
		ivariant->equi = false;
		ivariant->orientation_string = QString("");
		ivariant->orientation = 0;
		for (unsigned int x = 0; x < ivariant->di->image_slices.size(); x++)
			 ivariant->di->image_slices[x]->slice_orientation_string =
				QString("");
	}
	return QString("");
}

template<typename T> QString process_dicom_rgba_image(
	bool * ok,
	ImageVariant * ivariant,
	typename T::Pointer & image,
	char * buffer,
	itk::Matrix<itk::SpacePrecisionType,3,3> & direction,
	unsigned int dimx, unsigned int dimy, unsigned int dimz,
	double origin_x, double origin_y, double origin_z,
	double spacing_x, double spacing_y, double spacing_z,
	short image_type,
	bool cmyk, bool argb, bool gen_vertices,
	QProgressDialog * pb)
{
	if (!buffer)
	{
		*ok = false;
		return QString(
			"process_dicom_rgb_image : buffer==NULL");
	}
	typename T::RegionType region;
	typename T::SizeType size;
	typename T::IndexType start;
	typename T::PointType origin;
	typename T::SpacingType spacing;
	typedef itk::ImageSliceIteratorWithIndex<T> SliceIterator;
	if (pb)
	{
		pb->setLabelText(QString("Loading data... please wait"));
		pb->setValue(-1);
	}
	qApp->processEvents();
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
	const bool bad_direction = (	
		direction[0][0]>-0.000001 && direction[0][0]<0.000001 &&
		direction[1][0]>-0.000001 && direction[1][0]<0.000001 &&
		direction[2][0]>-0.000001 && direction[2][0]<0.000001 &&
		direction[0][1]>-0.000001 && direction[0][1]<0.000001 &&
		direction[1][1]>-0.000001 && direction[1][1]<0.000001 &&
		direction[2][1]>-0.000001 && direction[2][1]<0.000001 &&
		direction[0][2]>-0.000001 && direction[0][2]<0.000001 &&
		direction[1][2]>-0.000001 && direction[1][2]<0.000001 &&
		direction[2][2]>-0.000001 && direction[2][2]<0.000001)
			? true : false;
	try
	{
		image = T::New();
		image->SetRegions(region);
		image->Allocate();
		image->SetOrigin(origin);
		image->SetSpacing(spacing);
		if (!bad_direction) image->SetDirection(direction);
	}
	catch (itk::ExceptionObject & ex)
	{
		*ok = false;
		return QString(ex.GetDescription());
	}
	//
	const typename T::PixelType::ValueType * p__ =
		reinterpret_cast<typename T::PixelType::ValueType*>(buffer);
	ivariant->image_type = image_type;
	//
	SliceIterator it(image, image->GetLargestPossibleRegion());
	it.SetFirstDirection(0);
	it.SetSecondDirection(1);
	it.GoToBegin();
	int j = 0;
	while(!it.IsAtEnd())
	{
		while (!it.IsAtEndOfSlice())
		{
			while (!it.IsAtEndOfLine())
			{
				typename T::PixelType p;
				{
					if (cmyk)
					{
						// TODO
						const float C = (float)p__[j  ];
						const float M = (float)p__[j+1];
						const float Y = (float)p__[j+2];
						const float K = (float)p__[j+3];
						if (p__[j+3]!=255)
						{
							p[0]=static_cast<typename T::PixelType::ValueType>(((255.0f-C)*(255.0f-K))/255.0f); 
							p[1]=static_cast<typename T::PixelType::ValueType>(((255.0f-M)*(255.0f-K))/255.0f); 
							p[2]=static_cast<typename T::PixelType::ValueType>(((255.0f-Y)*(255.0f-K))/255.0f);
							p[3]=255;
						}
						else
						{
							p[0]=static_cast<typename T::PixelType::ValueType>(255.0f-C);
							p[1]=static_cast<typename T::PixelType::ValueType>(255.0f-M);
							p[2]=static_cast<typename T::PixelType::ValueType>(255.0f-Y);
							p[3]=255;
						}
					}
					else if (argb)
					{
						// TODO
						// ARGB = Pixel data represent a color image
						// described by red, green, blue, and alpha
						// image planes. The minimum sample value for each
						// RGB plane represents minimum intensity of the
						// color. The alpha plane is passed through
						// Palette Color Lookup Tables. If the alpha pixel
						// value is greater than 0, the red, green, and blue
						// lookup table values override the red, green, and
						// blue, pixel plane colors.
						//
						const float tmp_max = 255.0f;
						const float alpha = (float)p__[j+3]/tmp_max;
						const float one_minus_alpha = 1.0f - alpha;
						const float tmp_oth = one_minus_alpha*0;
						const float tmp_red = tmp_oth + alpha*(float)(p__[j+0]);
						const float tmp_gre = tmp_oth + alpha*(float)(p__[j+1]);
						const float tmp_blu = tmp_oth + alpha*(float)(p__[j+2]);
						p[0]=static_cast<typename T::PixelType::ValueType>((tmp_red/tmp_max)*255.0f);
						p[1]=static_cast<typename T::PixelType::ValueType>((tmp_gre/tmp_max)*255.0f);
						p[2]=static_cast<typename T::PixelType::ValueType>((tmp_blu/tmp_max)*255.0f);
						p[3]=255;
					}
					else
					{
						p[0]=static_cast<typename T::PixelType::ValueType>(p__[j  ]);
						p[1]=static_cast<typename T::PixelType::ValueType>(p__[j+1]);
						p[2]=static_cast<typename T::PixelType::ValueType>(p__[j+2]);
						p[3]=static_cast<typename T::PixelType::ValueType>(p__[j+3]);
					}
				}
				it.Set(p);
				j+=4;
				++it;
			}
			it.NextLine();
		}
		it.NextSlice();
	}
	*ok = reload_rgba_image<T>(
		image, ivariant, !gen_vertices);
	if (bad_direction)
	{
		ivariant->equi = false;
		ivariant->orientation_string = QString("");
		ivariant->orientation = 0;
		for (unsigned int x = 0; x < ivariant->di->image_slices.size(); x++)
			 ivariant->di->image_slices[x]->slice_orientation_string =
				QString("");
	}
	return QString("");
}

template <typename Tin, typename Tout>
QString apply_per_slice_rescale_(
	typename Tin::Pointer & image,
	typename Tout::Pointer & out_image,
	const QList< QPair<double, double> > rescale_values)
{
	if (image.IsNull()) return QString("image.IsNull()");
	typename Tin::SizeType size =
		image->GetLargestPossibleRegion().GetSize();
	const int size_x = size[0];
	const int size_y = size[1];
	const int size_z = size[2];
	if (size_z != rescale_values.size())
		return QString("size_z != rescale_values.size()");
	try
	{
		out_image = Tout::New();
		out_image->SetRegions(
			static_cast<typename Tout::RegionType>(
				image->GetLargestPossibleRegion()));
		out_image->SetOrigin(
			static_cast<typename Tout::PointType>(
				image->GetOrigin()));
		out_image->SetSpacing(
			static_cast<typename Tout::SpacingType>(
				image->GetSpacing()));
		out_image->SetDirection(
			static_cast<typename Tout::DirectionType>(
				image->GetDirection()));
		out_image->Allocate();
	}
	catch (itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	for (int x = 0; x < size_z; x++)
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
		region.SetIndex(index);
		region.SetSize(size_);
		itk::ImageRegionConstIterator<Tin> it0(image, region);
		it0.GoToBegin();
		itk::ImageRegionIterator<Tout> it1(out_image, region);
		it1.GoToBegin();
		while(!it0.IsAtEnd())
		{
			const typename Tin::PixelType v0 = it0.Get();
			const typename Tout::PixelType v1 =
				static_cast<typename Tout::PixelType>(
					v0*rescale_values.at(x).second +
					rescale_values.at(x).first);
			it1.Set(v1);
			++it0;
			++it1;
		}
	}
	image->DisconnectPipeline();
	image = NULL;
	return QString("");
}

int CommonUtils::get_next_id()
{
	static int id___ = 0;
	id___+=1;
	return id___;
}

int CommonUtils::get_next_group_id()
{
	static int group_id___ = 0;
	group_id___+=1;
	return group_id___;
}

float CommonUtils::random_range(float lo, float hi)
{
	return lo+(hi-lo)*(rand()/(float)RAND_MAX);
}

QString CommonUtils::convert_orientation_flag(unsigned int in)
{
 	switch (in)
	{
  	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIP:
		return QString("RIP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIP:
		return QString("LIP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSP:
		return QString("RSP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSP:
		return QString("LSP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIA:
		return QString("RIA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA:
		return QString("LIA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSA:
		return QString("RSA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSA:
		return QString("LSA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRP:
		return QString("IRP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILP:
		return QString("ILP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRP:
		return QString("SRP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLP:
		return QString("SLP");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRA:
		return QString("IRA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILA:
		return QString("ILA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRA:
		return QString("SRA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLA:
		return QString("SLA");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI:
		return QString("RPI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPI:
		return QString("LPI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI:
		return QString("RAI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAI:
		return QString("LAI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPS:
		return QString("RPS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPS:
		return QString("LPS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS:
		return QString("RAS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS:
		return QString("LAS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRI:
		return QString("PRI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLI:
		return QString("PLI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARI:
		return QString("ARI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALI:
		return QString("ALI");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRS:
		return QString("PRS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLS:
		return QString("PLS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARS:
		return QString("ARS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALS:
		return QString("ALS");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPR:
		return QString("IPR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPR:
		return QString("SPR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAR:
		return QString("IAR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAR:
		return QString("SAR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPL:
		return QString("IPL");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPL:
		return QString("SPL");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAL:
		return QString("IAL");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAL:
		return QString("SAL");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIR:
		return QString("PIR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSR:
		return QString("PSR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIR:
		return QString("AIR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASR:
		return QString("ASR");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIL:
		return QString("PIL");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSL:
		return QString("PSL");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIL:
		return QString("AIL");
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASL:
		return QString("ASL");
	default:break;
	}
	return QString("");
}

double CommonUtils::set_digits(double i, int digits)
{
    const double t = pow(10.0, digits);
    return floor(i*t)/t;
}

QString CommonUtils::get_orientation2(const double * pat_orientation)
{
	const bool print_oblique = false;
	const char RAI_codes[3][2] = { {'R', 'L'}, {'A', 'P'}, {'I', 'S'} };
	QString s("");
	char rai[4]; rai[0] = 'x'; rai[1]= 'x'; rai[2]= 'x'; rai[3] = '\0';
	const double row_dircos_x = pat_orientation[0];
	const double row_dircos_y = pat_orientation[1];
	const double row_dircos_z = pat_orientation[2];
	const double col_dircos_x = pat_orientation[3];
	const double col_dircos_y = pat_orientation[4];
	const double col_dircos_z = pat_orientation[5];
	const double nrm_dircos_x = row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y; 
	const double nrm_dircos_y = row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const double nrm_dircos_z = row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	bool oblique = false;
	vnl_matrix_fixed<double, 3, 3> dir;
	const vnl_vector_fixed<double,3> c0(row_dircos_x,row_dircos_y,row_dircos_z);
	const vnl_vector_fixed<double,3> c1(col_dircos_x,col_dircos_y,col_dircos_z);
	const vnl_vector_fixed<double,3> c2(nrm_dircos_x,nrm_dircos_y,nrm_dircos_z);
	dir.set_column(0,c0);
	dir.set_column(1,c1);
	dir.set_column(2,c2);
	for (int i = 0; i < 3; i++)
	{
		vnl_vector_fixed<double,3> dcos = dir.get_column(i);
    	const double dabsmax = dcos.inf_norm();
		for(int j = 0; j < 3; j++)
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
	if (print_oblique && oblique)
		std::cout << "Oblique, closest to " << rai
			<< std::endl;
	s = QString::fromLatin1(rai);
	s.remove(QChar('\0'));
	return s;
}

void CommonUtils::get_orientation3(
	char * orientation,
	float x,
	float y,
	float z)
{
	char orientationX = x < 0 ? 'R' : 'L';
	char orientationY = y < 0 ? 'A' : 'P';
	char orientationZ = z < 0 ? 'F' : 'H';
	double absX = fabs(x), absY = fabs(y), absZ = fabs(z);
	for (int i=0; i<3; ++i)
	{
		if (absX>.0001 && absX>absY && absX>absZ)
		{
			*orientation++=orientationX; absX=0;
		}
		else if (absY>.0001 && absY>absX && absY>absZ)
		{
			*orientation++=orientationY; absY=0;
		}
		else if (absZ>.0001 && absZ>absX && absZ>absY)
		{
			*orientation++=orientationZ; absZ=0;
		}
		else break;
	}
}

void CommonUtils::calculate_center_notuniform(
	const std::vector<ImageSlice*> & slices,
	float * center_x, float * center_y, float * center_z)
{
	int j = 0;
	double tmpx = 0.0, tmpy = 0.0, tmpz = 0.0;
	for (unsigned int k = 0; k < slices.size(); k++)
	{
		const ImageSlice * cs = slices.at(k);
		for (int z = 0; z <= 9; z+=3)
		{
			j++;
			tmpx += (double)cs->fv[z  ];
			tmpy += (double)cs->fv[z+1];
			tmpz += (double)cs->fv[z+2]; 
		}
	}
	if (j>0)
	{	
		*center_x = (float)(tmpx/(double)j);
		*center_y = (float)(tmpy/(double)j);
		*center_z = (float)(tmpz/(double)j);
	}
}

void CommonUtils::calculate_center_notuniform(
	const std::vector<SpectroscopySlice*> & slices,
	float * center_x, float * center_y, float * center_z)
{
	int j = 0;
	double tmpx = 0.0, tmpy = 0.0, tmpz = 0.0;
	for (unsigned int k = 0; k < slices.size(); k++)
	{
		const SpectroscopySlice * cs = slices.at(k);
		for (int z = 0; z <= 9; z+=3)
		{
			j++;
			tmpx += (double)cs->fv[z  ];
			tmpy += (double)cs->fv[z+1];
			tmpz += (double)cs->fv[z+2]; 
		}
	}
	if (j>0)
	{	
		*center_x = (float)(tmpx/(double)j);
		*center_y = (float)(tmpy/(double)j);
		*center_z = (float)(tmpz/(double)j);
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
	cs->tc[ 2] = (float)z/(float)(dimz-1);
	cs->tc[ 3] = 0.0f;
	cs->tc[ 4] = 0.0f;
	cs->tc[ 5] = (float)z/(float)(dimz-1);
	cs->tc[ 6] = 1.0f;
	cs->tc[ 7] = 1.0f;
	cs->tc[ 8] = (float)z/(float)(dimz-1);
	cs->tc[ 9] = 1.0f;
	cs->tc[10] = 0.0f;
	cs->tc[11] = (float)z/(float)(dimz-1);
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
			const float x3, const float y3, const float z3,
			const float x2, const float y2, const float z2,
			unsigned int columns_, unsigned int rows_)
{
	SpectroscopySlice * cs = new SpectroscopySlice;
	cs->fv[ 0] = x0;
	cs->fv[ 1] = y0;
	cs->fv[ 2] = z0;
	cs->fv[ 3] = x1;
	cs->fv[ 4] = y1;
	cs->fv[ 5] = z1;
	cs->fv[ 6] = x2;
	cs->fv[ 7] = y2;
	cs->fv[ 8] = z2;
	cs->fv[ 9] = x3;
	cs->fv[10] = y3;
	cs->fv[11] = z3;
	if (ok3d && gl)
	{
		if (columns_ >= 2 || rows_ >= 2)
		{
#if QT_VERSION >= 0x050000
			gl->makeCurrent();
			gl->glGenVertexArrays(1, &(cs->fvaoid));
			gl->glBindVertexArray(cs->fvaoid);
			gl->glGenBuffers(1, &(cs->fvboid));
			gl->glBindBuffer(GL_ARRAY_BUFFER, cs->fvboid);
			gl->glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), cs->fv, GL_STATIC_DRAW);
			gl->glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
			gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
			gl->glBindVertexArray(0);
#else
			gl->makeCurrent();
			glGenVertexArrays(1, &(cs->fvaoid));
			glBindVertexArray(cs->fvaoid);
			glGenBuffers(1, &(cs->fvboid));
			glBindBuffer(GL_ARRAY_BUFFER, cs->fvboid);
			glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), cs->fv, GL_STATIC_DRAW);
			glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(gl->frame_shader.position_handle);
			glBindVertexArray(0);
#endif
			GLWidget::increment_count_vbos(1);
			//
			if (columns_ > 2 && rows_ > 2)
			{
				const unsigned long lines_size = 3*2*(columns_-2+rows_-2);
				cs->lsize = lines_size/3;
				GLfloat * v = new GLfloat[lines_size];
				const sVector3 X0(x0,y0,z0);
				const sVector3 X1(x1,y1,z1);
				const sVector3 YN(x1,y1,z1);
				const sVector3 XN(x3,y3,z3);
				const float    Xd = length(XN-X0);
				const float    Yd = length(YN-X0);
				const sVector3 Xn = sVector3(normalize(XN-X0));
				const sVector3 Yn = sVector3(normalize(YN-X0));
				float dx = Xd/(columns_-1);
				float dy = Yd/(rows_-1);
				const sVector3 p = X1 + dx*Xn;
				unsigned long j = 0;
				for (unsigned int x = 1; x < columns_-1; x++)
				{
					const sVector3 from = X0 + (x*dx)*Xn;
					const sVector3 to   = from + Yd*Yn;
					v[j  ] = from.getX();
					v[j+1] = from.getY();
					v[j+2] = from.getZ();
					v[j+3] =   to.getX();
					v[j+4] =   to.getY();
					v[j+5] =   to.getZ();
					j+=6;
				}
				for (unsigned int x = 1; x < rows_-1; x++)
				{
					sVector3 from = X0 + (x*dy)*Yn;
					sVector3 to   = from + Xd*Xn;
					v[j  ] = from.getX();
					v[j+1] = from.getY();
					v[j+2] = from.getZ();
					v[j+3] =   to.getX();
					v[j+4] =   to.getY();
					v[j+5] =   to.getZ();
					j+=6;
				}
#if QT_VERSION >= 0x050000
				gl->glGenVertexArrays(1, &(cs->lvaoid));
				gl->glBindVertexArray(cs->lvaoid);
				gl->glGenBuffers(1, &(cs->lvboid));
				gl->glBindBuffer(GL_ARRAY_BUFFER, cs->lvboid);
				gl->glBufferData(GL_ARRAY_BUFFER, lines_size*sizeof(GLfloat), v, GL_STATIC_DRAW);
				gl->glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
				gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
				gl->glBindVertexArray(0);
#else
				glGenVertexArrays(1, &(cs->lvaoid));
				glBindVertexArray(cs->lvaoid);
				glGenBuffers(1, &(cs->lvboid));
				glBindBuffer(GL_ARRAY_BUFFER, cs->lvboid);
				glBufferData(GL_ARRAY_BUFFER, lines_size*sizeof(GLfloat), v, GL_STATIC_DRAW);
				glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
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
#if QT_VERSION >= 0x050000
				gl->glGenVertexArrays(1, &(cs->pvaoid));
				gl->glBindVertexArray(cs->pvaoid);
				gl->glGenBuffers(1, &(cs->pvboid));
				gl->glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
				gl->glBufferData(GL_ARRAY_BUFFER, 6*sizeof(GLfloat), v1, GL_STATIC_DRAW);
				gl->glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
				gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
				gl->glBindVertexArray(0);
#else
				glGenVertexArrays(1, &(cs->pvaoid));
				glBindVertexArray(cs->pvaoid);
				glGenBuffers(1, &(cs->pvboid));
				glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
				glBufferData(GL_ARRAY_BUFFER, 6*sizeof(GLfloat), v1, GL_STATIC_DRAW);
				glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
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
#if QT_VERSION >= 0x050000
			gl->glGenVertexArrays(1, &(cs->pvaoid));
			gl->glBindVertexArray(cs->pvaoid);
			gl->glGenBuffers(1, &(cs->pvboid));
			gl->glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
			gl->glBufferData(GL_ARRAY_BUFFER, 3*sizeof(GLfloat), v1, GL_STATIC_DRAW);
			gl->glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
			gl->glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
			gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
			gl->glBindVertexArray(0);
#else
			glGenVertexArrays(1, &(cs->pvaoid));
			glBindVertexArray(cs->pvaoid);
			glGenBuffers(1, &(cs->pvboid));
			glBindBuffer(GL_ARRAY_BUFFER, cs->pvboid);
			glBufferData(GL_ARRAY_BUFFER, 3*sizeof(GLfloat), v1, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
			glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
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
	const short image_type = ivariant->image_type;
	switch(image_type)
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
	const short image_type = ivariant->image_type;
	switch(image_type)
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
	if (!dest||!source) return;
	bool break_ = false;
	for (unsigned int x = 0;
		x < source->di->image_slices.size();
		x++)
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
			cs->slice_orientation_string =
				scs->slice_orientation_string;
			dest->di->image_slices.push_back(cs);
		}
		else
		{
			break_ = true; break;
		}
	}
	if (break_)
	{
		for (unsigned int x = 0;
			x < dest->di->image_slices.size();
			x++)
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
	for (int j = 0; j < 6; j++)
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
	for (int j = 0; j < 3; j++)
	{
		dest->di->origin[j] = source->di->origin[j];
	}
	dest->di->origin_ok = source->di->origin_ok;
}

void CommonUtils::copy_essential(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest||!source) return;
	dest->equi                 = source->equi;
	dest->di->skip_texture     = source->di->skip_texture;
	dest->di->hide_orientation = source->di->hide_orientation;
	if (
		source->ybr && 
		dest->image_type >= 10 &&
		dest->image_type < 16 &&
		source->image_type >= 10 &&
		source->image_type < 16)
	{
		dest->ybr = true;
	}
	copy_imagevariant_info(dest, source);
}

void CommonUtils::calculate_minmax_scalar(
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	const short image_type = ivariant->image_type;
	switch(image_type)
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
	bool ok = false;
	if (ivariant->image_type==0)
	{
		ok = reload_monochrome_image<ImageTypeSS>(
			ivariant,ivariant->pSS,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==1)
	{
		ok = reload_monochrome_image<ImageTypeUS>(
			ivariant,ivariant->pUS,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==2)
	{
		ok = reload_monochrome_image<ImageTypeSI>(
			ivariant,ivariant->pSI,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==3)
	{
		ok = reload_monochrome_image<ImageTypeUI>(
			ivariant,ivariant->pUI,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==4)
	{
		ivariant->di->maxwindow = true;
		ok = reload_monochrome_image<ImageTypeUC>(
			ivariant,ivariant->pUC,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==5)
	{
		ok = reload_monochrome_image<ImageTypeF>(
			ivariant,ivariant->pF,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==6)
	{
		ok = reload_monochrome_image<ImageTypeD>(
			ivariant,ivariant->pD,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==7)
	{
		ok = reload_monochrome_image<ImageTypeSLL>(
			ivariant,ivariant->pSLL,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
	}
	else if (ivariant->image_type==8)
	{
		ok = reload_monochrome_image<ImageTypeULL>(
			ivariant,ivariant->pULL,gl,max_3d_tex_size,
			NULL,
			change_size,size_x_,size_y_);
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
	if (ivariant->image_type==10)
		reload_rgb_image<RGBImageTypeSS>(ivariant->pSS_rgb, ivariant);
	else if (ivariant->image_type==11)
		reload_rgb_image<RGBImageTypeUS>(ivariant->pUS_rgb, ivariant);
	else if (ivariant->image_type==12)
		reload_rgb_image<RGBImageTypeSI>(ivariant->pSI_rgb, ivariant);
	else if (ivariant->image_type==13)
		reload_rgb_image<RGBImageTypeUI>(ivariant->pUI_rgb, ivariant);
	else if (ivariant->image_type==14)
		reload_rgb_image<RGBImageTypeUC>(ivariant->pUC_rgb, ivariant);
	else if (ivariant->image_type==15)
		reload_rgb_image<RGBImageTypeF>(ivariant->pF_rgb, ivariant);
	else if (ivariant->image_type==16)
		reload_rgb_image<RGBImageTypeD>(ivariant->pD_rgb, ivariant);
	else if (ivariant->image_type==20)
		reload_rgba_image<RGBAImageTypeSS>(ivariant->pSS_rgba, ivariant);
	else if (ivariant->image_type==21)
		reload_rgba_image<RGBAImageTypeUS>(ivariant->pUS_rgba, ivariant);
	else if (ivariant->image_type==22)
		reload_rgba_image<RGBAImageTypeSI>(ivariant->pSI_rgba, ivariant);
	else if (ivariant->image_type==23)
		reload_rgba_image<RGBAImageTypeUI>(ivariant->pUI_rgba, ivariant);
	else if (ivariant->image_type==24)
		reload_rgba_image<RGBAImageTypeUC>(ivariant->pUC_rgba, ivariant);
	else if (ivariant->image_type==25)
		reload_rgba_image<RGBAImageTypeF>(ivariant->pF_rgba, ivariant);
	else if (ivariant->image_type==26)
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
	dest->instance_number         = source->instance_number;
	dest->study_uid               = source->study_uid;
	dest->series_uid              = source->series_uid;
	dest->study_date              = source->study_date;
	dest->study_time              = source->study_time;
	dest->study_id                = source->study_id;
	dest->frame_of_ref_uid        = source->frame_of_ref_uid;
	dest->modality                = source->modality;
	dest->iod                     = source->iod;
	dest->sop                     = source->sop;
	dest->series_description      = source->series_description;
	dest->study_description       = source->study_description;
	dest->pat_name                = source->pat_name;
	dest->pat_id                  = source->pat_id;
	dest->pat_birthdate           = source->pat_birthdate;
	dest->pat_sex                 = source->pat_sex;
	dest->series_date             = source->series_date;
	dest->series_time             = source->series_time;
	dest->acquisition_date        = source->acquisition_date;
	dest->acquisition_time        = source->acquisition_time;
	dest->hardware                = source->hardware;
	dest->hardware_info           = source->hardware_info;
	dest->iinfo                   = source->iinfo;
	dest->institution             = source->institution;
}

void CommonUtils::copy_imagevariant_info_min(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	dest->study_uid          = source->study_uid;
	dest->study_date         = source->study_date;
	dest->study_time         = source->study_time;
	dest->study_id           = source->study_id;
	dest->series_description = source->series_description;
	dest->study_description  = source->study_description;
	dest->pat_name           = source->pat_name;
	dest->pat_id             = source->pat_id;
	dest->pat_birthdate      = source->pat_birthdate;
	dest->pat_sex            = source->pat_sex;
}

void CommonUtils::copy_frametimes(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	for (unsigned int x = 0; x < source->frame_times.size(); x++)
		dest->frame_times.push_back(source->frame_times.at(x));
}

void CommonUtils::copy_usregions(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	for (int x = 0; x < source->usregions.size(); x++)
		dest->usregions.push_back(source->usregions[x]);
}

void CommonUtils::copy_imagevariant_overlays(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	QMap<int, SliceOverlays>::const_iterator it =
		source->image_overlays.all_overlays.constBegin();
	while (it != source->image_overlays.all_overlays.constEnd())
	{
		const int source_key = it.key();
		const SliceOverlays & source_overlays = it.value();
		for (int x = 0; x < source_overlays.size(); x++)
		{
			const SliceOverlay source_overlay =
				source_overlays.at(x);
			SliceOverlay overlay;
			overlay.dimx = source_overlay.dimx;
			overlay.dimy = source_overlay.dimy;
			overlay.x = source_overlay.x;
			overlay.y = source_overlay.y;
			for (size_t j = 0; j < source_overlay.data.size(); j++)
			{
				overlay.data.push_back(source_overlay.data.at(j));
			}
			if (!dest->image_overlays.all_overlays.contains(source_key))
			{
				dest->image_overlays.all_overlays[source_key] =
					SliceOverlays();
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
	ivariant->di->irect_size[0]  = ivariant->di->idimx;
	ivariant->di->irect_size[1]  = ivariant->di->idimy;
	ivariant->di->selected_x_slice =
		(ivariant->di->idimx>0) ? ivariant->di->idimx/2 : 0;
	ivariant->di->selected_y_slice =
		(ivariant->di->idimy>0) ? ivariant->di->idimy/2 : 0;
	ivariant->di->selected_z_slice =
		(ivariant->di->idimz>0) ? ivariant->di->idimz/2 : 0;
	ivariant->di->from_slice = 0;
	ivariant->di->to_slice = ivariant->di->idimz-1;
}

void CommonUtils::get_dimensions_(ImageVariant * ivariant)
{
	if (!ivariant) return;
	switch(ivariant->image_type)
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
	QString orient("");
	switch(ivariant->image_type)
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
	std::vector<char*> & data,
	bool delete_data,
	const mdcm::PixelFormat & pixelformat,
	const mdcm::PhotometricInterpretation & pi,
	ImageVariant * ivariant, 
	itk::Matrix<itk::SpacePrecisionType,3,3> & direction,
	unsigned int dimx, unsigned int dimy, unsigned int dimz,
	double origin_x, double origin_y, double origin_z,
	double spacing_x, double spacing_y, double spacing_z,
	bool geometry_from_image, bool allow_geometry_from_image,
	bool resize_, unsigned int size_x, unsigned int size_y,
	bool no_warn_rescale,
	int max_3d_tex_size, GLWidget * gl, QProgressDialog * pb,
	bool skip_ybr)
{
	*ok = false;
	QString error = QString("");
	const unsigned long data_size = data.size();
	if (data_size<1) return QString("data.size()<1");
	if (!data.at(0)) return QString("!data.at(0)");
	const bool ybr = !skip_ybr && (
		pi == mdcm::PhotometricInterpretation::YBR_FULL
#if 1
		|| pi == mdcm::PhotometricInterpretation::YBR_FULL_422
#endif
		);
	const bool cmyk = (pi == mdcm::PhotometricInterpretation::CMYK);
	const bool argb = (pi == mdcm::PhotometricInterpretation::ARGB);
	const unsigned short bits_allocated = pixelformat.GetBitsAllocated();
	const unsigned short bits_stored    = pixelformat.GetBitsStored();
	const unsigned short high_bit       = pixelformat.GetHighBit();
	if (bits_allocated > 0) ivariant->di->bits_allocated = bits_allocated;
	if (bits_stored > 0)    ivariant->di->bits_stored    = bits_stored;
	if (high_bit > 0)       ivariant->di->high_bit       = high_bit;
	// monochrome
	if (pixelformat.GetSamplesPerPixel()==1)
	{
		switch (pixelformat)
		{
		case mdcm::PixelFormat::INT12:
		case mdcm::PixelFormat::INT16:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*2];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 2; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeSS>(
					ok, ivariant, ivariant->pSS, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					0,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::UINT12:
		case mdcm::PixelFormat::UINT16:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*2];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 2; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeUS>(
					ok, ivariant, ivariant->pUS, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					1,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::INT32:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*4];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 4; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeSI>(
					ok, ivariant, ivariant->pSI, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					2,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::UINT32:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*4];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 4; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeUI>(
					ok, ivariant, ivariant->pUI, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					3,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::INT64:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*8];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 8; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeSLL>(
					ok, ivariant, ivariant->pSLL, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					7,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::UINT64:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*8];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 8; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeULL>(
					ok, ivariant, ivariant->pULL, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					8,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::INT8:
		case mdcm::PixelFormat::UINT8:
		case mdcm::PixelFormat::SINGLEBIT:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 1; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				ivariant->di->maxwindow = true;
				error = process_dicom_monochrome_image<ImageTypeUC>(
					ok, ivariant, ivariant->pUC, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					4,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::FLOAT16:
			return QString("PixelFormat FLOAT16 is not supported");
 		case mdcm::PixelFormat::FLOAT32:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*4];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 4; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeF>(
					ok, ivariant, ivariant->pF, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					5,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		case mdcm::PixelFormat::FLOAT64:
			{
				char * p__;
				if (data_size>1)
				{
					try
					{
						p__ = new char[dimx*dimy*dimz*8];
					}
					catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
					if (!p__) return QString("p__ == NULL");
					long inc = 0;
					for (unsigned int z_ = 0; z_ < dimz; z_++)
					{
						if (!data.at(z_))
						{
							return QString(
								QString("!data.at(") +
								QVariant((int)z_).toString() +
								QString(")"));
						}
						long inc_xy = 0;
						for (unsigned int y_ = 0; y_ < dimy; y_++)
						{
							for (unsigned int x_ = 0; x_ < dimx; x_++)
							{
								for (unsigned int ss_ = 0; ss_ < 8; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
						if (delete_data)
						{
							delete [] data[z_];
							data[z_] = NULL;
						}
					}
				}
				else { p__ = &data[0][0]; }
				error = process_dicom_monochrome_image<ImageTypeD>(
					ok, ivariant, ivariant->pD, p__,
					direction,
					dimx, dimy, dimz,
					origin_x, origin_y, origin_z,
					spacing_x, spacing_y, spacing_z,
					6,
					max_3d_tex_size, geometry_from_image,
					pb, gl,
					resize_, size_x, size_y);
				if (data_size>1) delete [] p__;
			}
			break;
		default:
			break;
		}
	}
	// RGB
	else if (pixelformat.GetSamplesPerPixel()==3)
	{
		if (
			pixelformat == mdcm::PixelFormat::UINT8 ||
			pixelformat == mdcm::PixelFormat::INT8)
		{
			char * p__;
			if (data_size>1)
			{
				try
				{
					p__ = new char[dimx*dimy*dimz*3];
				}
				catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
				if (!p__) return QString("p__ == NULL");
				long inc = 0;
				for (unsigned int z_ = 0; z_ < dimz; z_++)
				{
					if (!data.at(z_))
					{
						return QString(
							QString("!data.at(") +
							QVariant((int)z_).toString() +
							QString(")"));
					}
					long inc_xy = 0;
					for (unsigned int y_ = 0; y_ < dimy; y_++)
					{
						for (unsigned int x_ = 0; x_ < dimx; x_++)
						{
							for (unsigned int rgb__ = 0; rgb__ < 3; rgb__++)
							{
								for (unsigned int ss_ = 0; ss_ < 1; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
					}
					if (delete_data)
					{
						delete [] data[z_];
						data[z_] = NULL;
					}
				}
			}
			else { p__ = &data[0][0]; }
			error = process_dicom_rgb_image<RGBImageTypeUC>(
				ok, ivariant, ivariant->pUC_rgb, p__,
				direction,
				dimx, dimy, dimz,
				origin_x, origin_y, origin_z,
				spacing_x, spacing_y, spacing_z,
				14,
				ybr, geometry_from_image, pb);
			if (data_size>1) delete [] p__;
		}
		else if (
			pixelformat == mdcm::PixelFormat::UINT16 ||
			pixelformat == mdcm::PixelFormat::UINT12)
		{
			char * p__;
			if (data_size>1)
			{
				try
				{
					p__ = new char[dimx*dimy*dimz*6];
				}
				catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
				if (!p__) return QString("p__ == NULL");
				long inc = 0;
				for (unsigned int z_ = 0; z_ < dimz; z_++)
				{
					if (!data.at(z_))
					{
						return QString(
							QString("!data.at(") +
							QVariant((int)z_).toString() +
							QString(")"));
					}
					long inc_xy = 0;
					for (unsigned int y_ = 0; y_ < dimy; y_++)
					{
						for (unsigned int x_ = 0; x_ < dimx; x_++)
						{
							for (unsigned int rgb__ = 0; rgb__ < 3; rgb__++)
							{
								for (unsigned int ss_ = 0; ss_ < 2; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
					}
					if (delete_data)
					{
						delete [] data[z_];
						data[z_] = NULL;
					}
				}
			}
			else { p__ = &data[0][0]; }
			error = process_dicom_rgb_image<RGBImageTypeUS>(
				ok, ivariant, ivariant->pUS_rgb, p__,
				direction,
				dimx, dimy, dimz,
				origin_x, origin_y, origin_z,
				spacing_x, spacing_y, spacing_z,
				11,
				false, geometry_from_image, pb);
			if (data_size>1) delete [] p__;
		}
		else if (
			pixelformat == mdcm::PixelFormat::INT16 ||
			pixelformat == mdcm::PixelFormat::INT12)
		{
			char * p__;
			if (data_size>1)
			{
				try
				{
					p__ = new char[dimx*dimy*dimz*6];
				}
				catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
				if (!p__) return QString("p__ == NULL");
				long inc = 0;
				for (unsigned int z_ = 0; z_ < dimz; z_++)
				{
					if (!data.at(z_))
					{
						return QString(
							QString("!data.at(") +
							QVariant((int)z_).toString() +
							QString(")"));
					}
					long inc_xy = 0;
					for (unsigned int y_ = 0; y_ < dimy; y_++)
					{
						for (unsigned int x_ = 0; x_ < dimx; x_++)
						{
							for (unsigned int rgb__ = 0; rgb__ < 3; rgb__++)
							{
								for (unsigned int ss_ = 0; ss_ < 2; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
					}
					if (delete_data)
					{
						delete [] data[z_];
						data[z_] = NULL;
					}
				}
			}
			else { p__ = &data[0][0]; }
			error = process_dicom_rgb_image<RGBImageTypeSS>(
				ok, ivariant, ivariant->pSS_rgb, p__,
				direction,
				dimx, dimy, dimz,
				origin_x, origin_y, origin_z,
				spacing_x, spacing_y, spacing_z,
				10,
				false, geometry_from_image, pb);
			if (data_size>1) delete [] p__;
		}
		else if (pixelformat == mdcm::PixelFormat::FLOAT32)
		{
			char * p__;
			if (data_size>1)
			{
				try
				{
					p__ = new char[dimx*dimy*dimz*12];
				}
				catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
				if (!p__) return QString("p__ == NULL");
				long inc = 0;
				for (unsigned int z_ = 0; z_ < dimz; z_++)
				{
					if (!data.at(z_))
					{
						return QString(
							QString("!data.at(") +
							QVariant((int)z_).toString() +
							QString(")"));
					}
					long inc_xy = 0;
					for (unsigned int y_ = 0; y_ < dimy; y_++)
					{
						for (unsigned int x_ = 0; x_ < dimx; x_++)
						{
							for (unsigned int rgb__ = 0; rgb__ < 3; rgb__++)
							{
								for (unsigned int ss_ = 0; ss_ < 4; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
					}
					if (delete_data)
					{
						delete [] data[z_];
						data[z_] = NULL;
					}
				}
			}
			else { p__ = &data[0][0]; }
			error = process_dicom_rgb_image<RGBImageTypeF>(
				ok, ivariant, ivariant->pF_rgb, p__,
				direction,
				dimx, dimy, dimz,
				origin_x, origin_y, origin_z,
				spacing_x, spacing_y, spacing_z,
				15,
				false, geometry_from_image, pb);

			if (data_size>1) delete [] p__;
		}
		else { error = QString("pixelformat not supported"); }
		if (
			error.isEmpty() &&
			skip_ybr &&
			(pi == mdcm::PhotometricInterpretation::YBR_FULL
#if 1
			|| pi == mdcm::PhotometricInterpretation::YBR_FULL_422
#endif
			))
		{
			ivariant->ybr = true;
		}
	}
	// RGBA
	else if (pixelformat.GetSamplesPerPixel()==4)
	{
#define USE_4SAMPLES_PER_PIXEL
#ifdef USE_4SAMPLES_PER_PIXEL
		if (pixelformat == mdcm::PixelFormat::UINT8 ||
			pixelformat == mdcm::PixelFormat::INT8)
		{
			char * p__;
			if (data_size>1)
			{
				try
				{
					p__ = new char[dimx*dimy*dimz*4];
				}
				catch (std::bad_alloc&) { return QString("std::bad_alloc"); }
				if (!p__) return QString("p__ == NULL");
				long inc = 0;
				for (unsigned int z_ = 0; z_ < dimz; z_++)
				{
					if (!data.at(z_))
					{
						return QString(
							QString("!data.at(") +
							QVariant((int)z_).toString() +
							QString(")"));
					}
					long inc_xy = 0;
					for (unsigned int y_ = 0; y_ < dimy; y_++)
					{
						for (unsigned int x_ = 0; x_ < dimx; x_++)
						{
							for (unsigned int rgba__ = 0; rgba__ < 4; rgba__++)
							{
								for (unsigned int ss_ = 0; ss_ < 1; ss_++)
								{
									p__[inc] = data.at(z_)[inc_xy];
									inc++;
									inc_xy++;
								}
							}
						}
					}
					if (delete_data)
					{
						delete [] data[z_];
						data[z_] = NULL;
					}
				}
			}
			else { p__ = &data[0][0]; }
			error = process_dicom_rgba_image<RGBAImageTypeUC>(
				ok, ivariant, ivariant->pUC_rgba, p__,
				direction,
				dimx, dimy, dimz,
				origin_x, origin_y, origin_z,
				spacing_x, spacing_y, spacing_z,
				24,
				cmyk, argb, geometry_from_image, pb);
			if (data_size>1) delete [] p__;
		}
		else { error = QString("pixelformat not supported"); }
#else
		error = QString(
			"4 samples per pixel disabled for DICOM files");
#endif
	}
	else { error = QString("pixelformat not supported"); }
	//
	// overtwrite orientation, it may be default and wrong
	if (geometry_from_image && !allow_geometry_from_image)
	{
		ivariant->orientation = 0;
		ivariant->orientation_string = QString("");
	}
	if (ivariant->image_type>=0 &&
		ivariant->image_type<10 &&
		!no_warn_rescale)
		ivariant->rescale_disabled = true;
	if (ivariant->di->slices_from_dicom ||
		allow_geometry_from_image)
		ivariant->di->hide_orientation = false;
	return error;
}

void CommonUtils::read_geometry_from_image_(ImageVariant * v)
{
	if (!v) return;
	const short image_type = v->image_type;
	switch (image_type)
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
	default: break;
	}
}

QString CommonUtils::get_screenshot_dir()
{
	return QDir::toNativeSeparators(screenshot_dir);
}

void CommonUtils::set_screenshot_dir(const QString & s)
{
	if (s.isEmpty())
#ifdef _WIN32
		screenshot_dir =
			QDir::toNativeSeparators(
				QDir::homePath() +
				QDir::separator() +
				QString("Desktop"));
#else
		screenshot_dir =
			QDir::toNativeSeparators(QDir::homePath());
#endif
	else
		screenshot_dir = QDir::toNativeSeparators(s);

}

QString CommonUtils::get_screenshot_name(const QString & s)
{
	QString d = (s.isEmpty())
		?
			QDateTime::currentDateTime().toString(
				QString("yyyyMMdd-hhmmss")) +
			QString(".png")
		:
			QDir::toNativeSeparators(
				s +
				QDir::separator() +
				QDateTime::currentDateTime().toString(
					QString("yyyyMMdd-hhmmss")) +
				QString(".png"));
	return d;
}

QString CommonUtils::get_screenshot_name2()
{
	return
		QDateTime::currentDateTime().toString(
			QString("yyyyMMdd-hhmmss"));
}

QString CommonUtils::get_open_dir()
{
	return QDir::toNativeSeparators(open_dir);
}

void CommonUtils::set_open_dir(const QString & s)
{
	open_dir = QDir::toNativeSeparators(s);
}

QString CommonUtils::apply_per_slice_rescale(
	ImageVariant * ivariant,
	const QList< QPair<double, double> > & rescale_values)
{
	if (!ivariant) return QString("!ivariant");
	const short image_type = ivariant->image_type;
	if (!(image_type >= 0 && image_type < 10))
		return QString("");
	bool float64 = false;
	for (int x = 0; x < rescale_values.size(); x++)
	{
		if (
			((abs(ivariant->di->vmax)*rescale_values.at(x).second +
				rescale_values.at(x).first) > (double)FLT_MAX) ||
			((abs(ivariant->di->vmin)*rescale_values.at(x).second +
				rescale_values.at(x).first) > (double)FLT_MAX))
		{
			float64 = true;
			break;
		}
	}
	QString s("");
	switch(image_type)
	{
	case 0:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeSS,ImageTypeD>(
				ivariant->pSS, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeSS,ImageTypeF>(
				ivariant->pSS, ivariant->pF, rescale_values);
		break;
	case 1:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeUS,ImageTypeD>(
				ivariant->pUS, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeUS,ImageTypeF>(
				ivariant->pUS, ivariant->pF, rescale_values);
		break;
	case 2:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeSI,ImageTypeD>(
				ivariant->pSI, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeSI,ImageTypeF>(
				ivariant->pSI, ivariant->pF, rescale_values);
		break;
	case 3:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeUI,ImageTypeD>(
				ivariant->pUI, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeUI,ImageTypeF>(
				ivariant->pUI, ivariant->pF, rescale_values);
		break;
	case 4:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeUC,ImageTypeD>(
				ivariant->pUC, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeUC,ImageTypeF>(
				ivariant->pUC, ivariant->pF, rescale_values);
		break;
	case 5:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeF,ImageTypeD>(
				ivariant->pF, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeF,ImageTypeF>(
				ivariant->pF, ivariant->pF, rescale_values);
		break;
	case 6:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeD,ImageTypeD>(
				ivariant->pD, ivariant->pD, rescale_values);
		else
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeD,ImageTypeF>(
				ivariant->pD, ivariant->pF, rescale_values);
		break;
	case 7:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeSLL,ImageTypeD>(
				ivariant->pSLL, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeSLL,ImageTypeF>(
				ivariant->pSLL, ivariant->pF, rescale_values);
		break;
	case 8:
		if (float64)
			s = apply_per_slice_rescale_<ImageTypeULL,ImageTypeD>(
				ivariant->pULL, ivariant->pD, rescale_values);
		else
			s = apply_per_slice_rescale_<ImageTypeULL,ImageTypeF>(
				ivariant->pULL, ivariant->pF, rescale_values);
		break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
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

template<typename T> double get_value(
	const typename T::Pointer & image,
	const int x,
	const int y,
	const int z)
{
	if (image.IsNull()) return 0;
	typename T::IndexType idx;
	idx[0] = x;
	idx[1] = y;
	idx[2] = z;
	double r = 0;
	try
	{
		r = static_cast<double>(image->GetPixel(idx));
	}
	catch (itk::ExceptionObject &)
	{
		r = 0;
	}
	return r;
}

void CommonUtils::get_pixel_values(
	const QList<ImageVariant*> & images,
	const int x,
	const int y,
	const int z,
	QList<double> & values)
{
	for (int i = 0; i < images.size(); i++)
	{
		if (!images.at(i)) continue;
		const short image_type = images.at(i)->image_type;
		double d = 0;
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
	double max_delta = 0;
	if (v->image_type == 300)
	{
		max_delta = 1000.0;
	}
	else if (v->image_type == 100)
	{
		double tmp0 = 0;
		for (int x = 0; x < v->di->rois.size(); x++)
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
	const double H = CommonUtils::random_range(0.0f,359.999f);
	const double S = CommonUtils::random_range(0.5f,0.999f);
	const double V = CommonUtils::random_range(0.4f,0.999f);
	double R_, G_, B_;
	ColorSpace_::Hsv2Rgb(&R_, &G_, &B_, H, S, V);
	*R = (float)R_;
	*G = (float)G_;
	*B = (float)B_;
}
