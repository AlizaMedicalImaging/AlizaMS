#ifndef A_PROCESSIMAGETHREADLUT_H
#define A_PROCESSIMAGETHREADLUT_H

// clang-format off

#include <QThread>
#include <itkImage.h>
#include <itkImageRegionConstIterator.h>
#include "luts.h"

template<typename T> class ProcessImageThreadLUT_ : public QThread
{
public:
	ProcessImageThreadLUT_(
		const typename T::Pointer & image_,
		unsigned char * p_,
		const int size_0_, const int size_1_,
		const int index_0_, const int index_1_,
		const unsigned int j_,
		const double window_center_, const double window_width_,
		const short lut_,
		const bool alt_mode_,
		const short lut_function_)
		:
		image(image_),
		p(p_),
		size_0(size_0_), size_1(size_1_),
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

	void run() override
	{
		typename T::SizeType size;
		size[0] = size_0;
		size[1] = size_1;
		typename T::IndexType index;
		index[0] = index_0;
		index[1] = index_1;
		const typename T::RegionType region(index, size);
		const unsigned char * tmp_p1{};
		int tmp__size{};
		switch (lut)
		{
		case 0:
			break;
		case 1:
			{
				tmp_p1 = default_lut;
				tmp__size = default_lut_size;
			}
			break;
		case 2:
			{
				tmp_p1 = black_rainbow_lut;
				tmp__size = black_rainbow_size;
			}
			break;
		case 3:
			{
				tmp_p1 = syngo_lut;
				tmp__size = syngo_lut_size;
			}
			break;
		case 4:
			{
				tmp_p1 = hot_iron;
				tmp__size = hot_iron_size;
			}
			break;
		case 5:
			{
				tmp_p1 = hot_metal_blue;
				tmp__size = hot_metal_blue_size;
			}
			break;
		case 6:
			{
				tmp_p1 = pet_dicom_lut;
				tmp__size = pet_dicom_lut_size;
			}
			break;
		case 7:
			{
				tmp_p1 = pet20_dicom_lut;
				tmp__size = pet20_dicom_lut_size;
			}
			break;
		case 8:
			{
				tmp_p1 = labels_lut;
				tmp__size = labels_lut_size;
			}
			break;
		default:
			return;
		}
		//
		short tmp_lut_function;
		double wmin, wmax, div_;
		double window_center_minus_0_point_5{}; // used only for LINEAR
		const double window_width_minus_one = window_width - 1.0;
		if (lut_function == 1)
		{
			// DICOM LINEAR works with window_width >= 1,
			// fallback to LINEAR_EXACT otherwise,
			// this should not happen, the proper LUT function should be set before.
			window_center_minus_0_point_5 = window_center - 0.5;
			if (window_width_minus_one > 0.0)
			{
				wmin = window_center_minus_0_point_5 - (window_width_minus_one * 0.5);
				wmax = window_center_minus_0_point_5 + (window_width_minus_one * 0.5);
				div_ = window_width_minus_one;
				tmp_lut_function = 1;
			}
			else
			{
				wmin = window_center - window_width * 0.5;
				wmax = window_center + window_width * 0.5;
				div_ = (window_width > 0.0) ? window_width : 0.00001;
				tmp_lut_function = 0;
#if 0
				std::cout << "Warning: forced LUT function to LINEAR_EXACT" << std::endl;
#endif
			}
		}
		else
		{
			wmin = window_center - window_width * 0.5;
			wmax = window_center + window_width * 0.5;
			div_ = (window_width > 0.0) ? window_width : 0.00001;
			tmp_lut_function = lut_function;
		}
		//
		typename itk::ImageRegionConstIterator<T> iterator(image, region);
		iterator.GoToBegin();
		while (!iterator.IsAtEnd())
		{
			const double v = iterator.Get();
			if (v > wmin && v <= wmax)
			{
				double r;
				if (tmp_lut_function == 2) // SIGMOID
				{
					const double x = -4.0 * ((v - window_center) / div_);
					r = 1.0 / (1.0 + exp(x));
				}
				else if (tmp_lut_function == 1) // LINEAR
				{
					// if (x <= c - 0.5 - (w - 1) / 2), then y = ymin
					// else if (x > c - 0.5 + (w - 1) / 2), then y = ymax
					// else y = ((x - (c - 0.5)) / (w - 1) + 0.5) * (ymax - ymin) + ymin
					r = (v - window_center_minus_0_point_5) / div_ + 0.5;
				}
				else // LINEAR_EXACT
				{
					// if (x <= c - w / 2), then y = ymin
					// else if (x > c + w / 2), then y = ymax
					// else y = ((x - c) / w + 0.5) * (ymax - ymin) + ymin
					r = ((v - window_center) / div_) + 0.5;
				}
				switch (lut)
				{
				case 0:
					{
						const unsigned char c = static_cast<unsigned char>(UCHAR_MAX * r);
						p[j]     = c;
						p[j + 1] = c;
						p[j + 2] = c;
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
						int z = static_cast<int>(r * tmp__size);
						if (z < 0) z = 0;
						if (z > (tmp__size - 1)) z = tmp__size - 1;
						p[j]     = tmp_p1[z * 3];
						p[j + 1] = tmp_p1[z * 3 + 1];
						p[j + 2] = tmp_p1[z * 3 + 2];
					}
					break;
				case 8:
					{
						int z = static_cast<int>(v);
						if (z < 0) z = 0;
						if (z > (tmp__size - 1)) z = tmp__size - 1;
						p[j + 0] = tmp_p1[z * 3];
						p[j + 1] = tmp_p1[z * 3 + 1];
						p[j + 2] = tmp_p1[z * 3 + 2];
					}
					break;
				default:
					break;
				}
			}
			else if (v <= wmin)
			{
				switch (lut)
				{
				case 0:
					{
						p[j]     = 0;
						p[j + 1] = 0;
						p[j + 2] = 0;
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
					{
						p[j]     = tmp_p1[0];
						p[j + 1] = tmp_p1[1];
						p[j + 2] = tmp_p1[2];
					}
					break;
				default:
					break;
				}
			}
			else if (v > wmax)
			{
				switch (lut)
				{
				case 0:
					if (alt_mode)
					{
						p[j]     = 0;
						p[j + 1] = 0;
						p[j + 2] = 0;
					}
					else
					{
						p[j]     = UCHAR_MAX;
						p[j + 1] = UCHAR_MAX;
						p[j + 2] = UCHAR_MAX;
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
					if (alt_mode)
					{
						p[j]     = tmp_p1[0];
						p[j + 1] = tmp_p1[1];
						p[j + 2] = tmp_p1[2];
					}
					else
					{
						const unsigned int z = tmp__size - 1;
						p[j]     = tmp_p1[z * 3];
						p[j + 1] = tmp_p1[z * 3 + 1];
						p[j + 2] = tmp_p1[z * 3 + 2];
					}
					break;
				default:
					break;
				}
			}
			j += 3;
			++iterator;
		}
	}

private:
	const typename T::Pointer image;
	unsigned char * p;
	const int size_0;
	const int size_1;
	const int index_0;
	const int index_1;
	unsigned int j;
	const double window_center;
	const double window_width;
	const short lut;
	const bool  alt_mode;
	const short lut_function;
};

#endif

