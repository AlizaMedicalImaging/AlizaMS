#ifndef ProcessImageThreadLUT_H___
#define ProcessImageThreadLUT_H___

#include <QThread>

#include "itkImage.h"
#include "itkImageRegionConstIterator.h"

#include "luts.h"

template<typename T> class ProcessImageThreadLUT_ : public QThread
{
public:
	ProcessImageThreadLUT_(
		const typename T::Pointer & image_, unsigned char * p_,
		const int size_0_,   const int size_1_,
		const int index_0_,  const int index_1_, const unsigned int j_,
		const double window_center_, const double window_width_,
		const short lut_, const bool alt_mode_, const short lut_function_)
		:
		image(image_),
		p(p_),
		size_0(size_0_),   size_1(size_1_),
		index_0(index_0_), index_1(index_1_),
		j(j_),
		window_center(window_center_), window_width(window_width_),
		lut(lut_),
		alt_mode(alt_mode_),
		lut_function(lut_function_)
	{
	}
	~ProcessImageThreadLUT_()
	{
	}
	//
	void run()
	{
		typename T::SizeType size;
		size[0] = size_0;
		size[1] = size_1;
		typename T::IndexType index;
		index[0] = index_0;
		index[1] = index_1;
 		typename T::RegionType region;
		region.SetSize(size);
		region.SetIndex(index);
		typename itk::ImageRegionConstIterator<T> iterator(image, region);
		iterator.GoToBegin();
		unsigned int j_ = j;
		//
		unsigned char * tmp_p0 = NULL;
		int tmp__size = 0;
		switch(lut)
		{
		case  0: break;
		case  1: { tmp_p0 = const_cast<unsigned char *>(default_lut);       tmp__size = default_lut_size; } break;
		case  2: { tmp_p0 = const_cast<unsigned char *>(black_rainbow_lut); tmp__size = black_rainbow_size; } break;
		case  3: { tmp_p0 = const_cast<unsigned char *>(syngo_lut);         tmp__size = syngo_lut_size; } break;
		case  4: { tmp_p0 = const_cast<unsigned char *>(hot_iron);          tmp__size = hot_iron_size; } break;
		case  5: { tmp_p0 = const_cast<unsigned char *>(hot_metal_blue);    tmp__size = hot_metal_blue_size; } break;
		case  6: { tmp_p0 = const_cast<unsigned char *>(pet_dicom_lut);     tmp__size = pet_dicom_lut_size; } break;
		case  7: { tmp_p0 = const_cast<unsigned char *>(pet20_dicom_lut);   tmp__size = pet20_dicom_lut_size; } break;
		default: return;
		}
		const unsigned char * tmp_p1 = const_cast<const unsigned char *>(tmp_p0);
		const double wmin = window_center - window_width*0.5;
		const double wmax = window_center + window_width*0.5;
		const double div_ = (window_width > 0.0) ? window_width : 0.00001;
		//
		while (!iterator.IsAtEnd())
		{
			const float v = static_cast<const float>(iterator.Get());
			if ((v >= wmin) && (v <= wmax))
			{
				if (lut_function == 2)
				{
					const double x = -6.0 * ((v-window_center) / div_);
					const double r = 1.0 / (1.0+exp(x));
					switch(lut)
					{
					case 0:
						{
							const unsigned char c = static_cast<unsigned char>(UCHAR_MAX*r);
							p[j_+0] = c;
							p[j_+1] = c;
							p[j_+2] = c;
						}
						break;
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						{
							int z = static_cast<int>(r*tmp__size);
							if (z < 0) z=0;
							if (z > (tmp__size-1)) z = tmp__size-1;
							p[j_+0] = tmp_p1[z*3+0];
							p[j_+1] = tmp_p1[z*3+1];
							p[j_+2] = tmp_p1[z*3+2];
						}
						break;
					case 8:
					case 9:
					case 10:
					case 11:
					case 12:
						{
							int z = static_cast<int>(v);
							if (z < 0) z=0;
							if (z > (tmp__size-1)) z = tmp__size-1;
							p[j_+0] = tmp_p1[z*3+0];
							p[j_+1] = tmp_p1[z*3+1];
							p[j_+2] = tmp_p1[z*3+2];
						}
						break;
					default: break;
					}
				}
				else
				{
					const double r = (v-wmin) / div_;
					switch(lut)
					{
					case 0:
						{
							const unsigned char c = static_cast<unsigned char>(UCHAR_MAX*r);
							p[j_+0] = c;
							p[j_+1] = c;
							p[j_+2] = c;
						}
						break;
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						{
							int z = static_cast<int>(r*tmp__size);
							if (z<0) z=0;
							if (z>(tmp__size-1)) z = tmp__size-1;
							p[j_+0] = tmp_p1[z*3+0];
							p[j_+1] = tmp_p1[z*3+1];
							p[j_+2] = tmp_p1[z*3+2];
						}
						break;
					case 8:
					case 9:
					case 10:
					case 11:
					case 12:
						{
							int z = static_cast<int>(v);
							if (z < 0) z=0;
							if (z > (tmp__size-1)) z = tmp__size-1;
							p[j_+0] = tmp_p1[z*3+0];
							p[j_+1] = tmp_p1[z*3+1];
							p[j_+2] = tmp_p1[z*3+2];
						}
						break;
					default: break;
					}
				}
			}
			else
			{
				if (v < wmin)
				{
					switch(lut)
					{
					case 0:
						{
							p[j_+0] = 0;
							p[j_+1] = 0;
							p[j_+2] = 0;
						}
						break;
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
					case 10:
					case 11:
					case 12:
						{
							p[j_+0] = tmp_p1[0];
							p[j_+1] = tmp_p1[1];
							p[j_+2] = tmp_p1[2];
						}
						break;
					default: break;
					}
				}
				else if (v > wmax)
				{
					switch(lut)
					{
					case 0:
						if (alt_mode)
						{
							p[j_+0] = 0;
							p[j_+1] = 0;
							p[j_+2] = 0;
						}
						else
						{
							p[j_+0] = UCHAR_MAX;
							p[j_+1] = UCHAR_MAX;
							p[j_+2] = UCHAR_MAX;
						}
						break;
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
					case 10:
					case 11:
					case 12:
						if (alt_mode)
						{
							p[j_+0] = tmp_p1[0];
							p[j_+1] = tmp_p1[1];
							p[j_+2] = tmp_p1[2];
						}
						else
						{
							const unsigned int z = tmp__size-1;
							p[j_+0] = tmp_p1[z*3+0];
							p[j_+1] = tmp_p1[z*3+1];
							p[j_+2] = tmp_p1[z*3+2];
						}
						break;
					default: break;
					}
				}
				else {;;}
			}
			//
			j_ += 3;
 			++iterator;
		}
	}
	//
private:
	typename T::Pointer image;
	unsigned char * p;
	const int size_0;
	const int size_1;
	const int index_0;
	const int index_1;
	const unsigned int j;
	const double window_center;
	const double window_width;
	const short lut;
	const bool  alt_mode;
	const short lut_function;
};

#endif // ProcessImageThreadLUT_H___
