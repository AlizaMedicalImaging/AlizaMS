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
#include "prconfigutils.h"
#include "settingswidget.h"
#include "dicomutils.h"
#include "codecutils.h"
#include "commonutils.h"
#include "itkShiftScaleImageFilter.h"
#include "itkIntensityWindowingImageFilter.h"
#include "itk/itkSigmoid2ImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkImageSliceIteratorWithIndex.h"
#include "itkImageLinearConstIteratorWithIndex.h"
#include "itkImageLinearIteratorWithIndex.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkCenteredEuler3DTransform.h"
#include "itkResampleImageFilter.h"
#include "itkFlipImageFilter.h"
#include "itkAffineTransform.h"
#include <itkImageDuplicator.h>
#include <QMap>
#include <QMultiMap>
#include <QApplication>
#include <vector>
#include <iostream>
#include "mdcmDataElement.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmItem.h"
#include "mdcmOverlay.h"

template<typename T> void set_spacing(
	typename T::Pointer & image,
	double x, double y)
{
	if (image.IsNull()) return;
	typename T::SpacingType s = image->GetSpacing();
	s[0] = x;
	s[1] = y;
	image->SetSpacing(s);
}

template<typename T> void set_asp_ratio(
	typename T::Pointer & image,
	double x, double y)
{
	if (image.IsNull()) return;
	typename T::SpacingType s = image->GetSpacing();
	s[0] *= x;
	s[1] *= y;
	image->SetSpacing(s);
}

template<typename T> void set_darea(
	ImageVariant * v,
	const typename T::Pointer & image,
	int areaTLx,
	int areaTLy,
	int areaBRx,
	int areaBRy)
{
	if (!v) return;
	if (image.IsNull()) return;
	const typename T::SizeType s =
		image->GetLargestPossibleRegion().GetSize();
	const int dimz = s[2];
	for (int x = 0; x < dimz; ++x)
	{
		PRDisplayArea a;
		a.top_left_x     = areaTLx;
		a.top_left_y     = areaTLy;
		a.bottom_right_x = areaBRx;
		a.bottom_right_y = areaBRy;
		v->pr_display_areas[x] = a;
	}
}

template<typename T> QString to_rgb(
	typename RGBImageTypeUC::Pointer & rgb_image,
	const typename T::Pointer & image)
{
	if (image.IsNull()) return QString("Image is NULL");
	try
	{
		rgb_image = RGBImageTypeUC::New();
		rgb_image->SetRegions(
			static_cast<typename RGBImageTypeUC::RegionType>(
				image->GetLargestPossibleRegion()));
		rgb_image->SetSpacing(
			static_cast<typename RGBImageTypeUC::SpacingType>(
				image->GetSpacing()));
		rgb_image->SetOrigin(
			static_cast<typename RGBImageTypeUC::PointType>(
				image->GetOrigin()));
		rgb_image->SetDirection(
			static_cast<typename RGBImageTypeUC::DirectionType>(
				image->GetDirection()));
		rgb_image->Allocate();
		typename itk::ImageRegionConstIterator<T> iterator(
			image, image->GetLargestPossibleRegion());
		typename itk::ImageRegionIterator<RGBImageTypeUC> rgb_iterator(
			rgb_image, rgb_image->GetLargestPossibleRegion());
		iterator.GoToBegin();
		rgb_iterator.GoToBegin();
		while(!iterator.IsAtEnd())
		{
			const unsigned char c =
				static_cast<unsigned char>(iterator.Get());
			RGBPixelUC pixel;
			pixel.SetRed(c);
			pixel.SetGreen(c);
			pixel.SetBlue(c);
			rgb_iterator.Set(pixel);
			++rgb_iterator;
			++iterator;
		}
	}
	catch (itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	return QString("");
}

template<typename T> QString intensity_filter2(
	const typename T::Pointer & image,
	typename ImageTypeF::Pointer & out_image,
	double shift, double scale)
{
	if (image.IsNull()) return QString("Image is NULL");
	typedef itk::ShiftScaleImageFilter<T, ImageTypeF>
		ShiftScaleImageFilterType;
	typename ShiftScaleImageFilterType::Pointer filter =
		ShiftScaleImageFilterType::New();
	try
	{
		filter->SetInput(image);
		filter->SetShift(shift);
		filter->SetScale(scale);
		filter->Update();
		out_image = filter->GetOutput();
	}
	catch (itk::ExceptionObject & ex)
	{ return QString(ex.GetDescription()); }
	if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	else return QString("Output image is NULL");
	return QString("");
}

template<typename T> QString to_char(
	const typename T::Pointer & image,
	typename ImageTypeUC::Pointer & out_image,
	double width_, double center_)
{
	if (image.IsNull()) return QString("Image is NULL");
	typedef  itk::IntensityWindowingImageFilter<T, ImageTypeUC>
		FilterType;
	typedef  itk::RescaleIntensityImageFilter<ImageTypeUC, ImageTypeUC>
		RescaleFilterType;
	typename FilterType::Pointer filter0 = FilterType::New();
	typename RescaleFilterType::Pointer filter1 =
		RescaleFilterType::New();
	try
	{
		filter0->SetInput(image);
		filter0->SetWindowLevel(
			static_cast<typename T::PixelType>(width_),
			static_cast<typename T::PixelType>(center_));
		filter0->SetOutputMinimum(static_cast<
			typename ImageTypeUC::PixelType>(0));
		filter0->SetOutputMaximum(static_cast<
			typename ImageTypeUC::PixelType>(255));
		filter1->SetInput(filter0->GetOutput());
		filter1->Update();
		out_image = filter1->GetOutput();
	}
	catch (itk::ExceptionObject & ex)
	{ return QString( ex.GetDescription()); }
	if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	else return QString("Output image is NULL");
	return QString("");
}

template<typename T> QString to_char_sigm(
	const typename T::Pointer & image,
	typename ImageTypeUC::Pointer & out_image,
	double width_, double center_)
{
	if (image.IsNull()) return QString("Image is NULL");
	typedef  itk::Sigmoid2ImageFilter<T, ImageTypeUC> FilterType;
	typedef  itk::RescaleIntensityImageFilter<ImageTypeUC, ImageTypeUC>
		RescaleFilterType;
	typename FilterType::Pointer filter0 = FilterType::New();
	typename RescaleFilterType::Pointer filter1 = RescaleFilterType::New();
	try
	{
		filter0->SetInput(image);
		filter0->SetAlpha(static_cast<typename T::PixelType>(width_));
		filter0->SetBeta(static_cast<typename T::PixelType>(center_));
		filter0->SetOutputMinimum(static_cast<
			typename ImageTypeUC::PixelType>(0));
		filter0->SetOutputMaximum(static_cast<
			typename ImageTypeUC::PixelType>(255));
		filter1->SetInput(filter0->GetOutput());
		filter1->Update();
		out_image = filter1->GetOutput();
	}
	catch (itk::ExceptionObject & ex)
	{
		return QString( ex.GetDescription());
	}
	if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	else return QString("Output image is NULL");
	return QString("");
}

template<typename Tin, typename Tout> QString extract_one_slice(
	const typename Tin::Pointer & image,
	typename Tout::Pointer & out_image,
	const int idx)
{
	if (image.IsNull()) return QString("Image is NULL");
	typedef itk::ExtractImageFilter<Tin, Tout> FilterType;
	const typename Tin::RegionType inRegion =
		image->GetLargestPossibleRegion();
	const typename Tin::SizeType size = inRegion.GetSize();
	typename Tin::IndexType index     = inRegion.GetIndex();
	typename Tin::RegionType outRegion;
	typename Tin::SizeType out_size;
	typename FilterType::Pointer filter = FilterType::New();
	index[2] = idx;
	out_size[0]=size[0];
	out_size[1]=size[1];
	out_size[2]=0;
	outRegion.SetSize(out_size);
	outRegion.SetIndex(index);
	try
	{
		filter->SetInput(image);
		filter->SetExtractionRegion(outRegion);
		filter->SetDirectionCollapseToIdentity();
		filter->Update();
		out_image = filter->GetOutput();
	}
	catch (itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	else return QString("Out image is NULL");
	return QString("");
}

static void rotate_flip_points(
	ImageVariant * ivariant,
	const double rotation,
	const bool flip)
{
	if (!(rotation != 0 || flip)) return;
	typedef itk::AffineTransform<double, 2> TransformType;
	const int dx = (int)ivariant->di->idimx;
	const int dy = (int)ivariant->di->idimy;
	const int dz = (int)ivariant->di->idimz;
	bool tmp0 = false;
	bool tmp1 = false;
	bool tmp2 = false;
	for (int z = 0; z < dz; ++z)
	{
		int ax = 0;
		int ay = 0;
		int px = 0;
		int py = 0;
		double cx = 0 + dx / 2.0;
		double cy = 0 + dy / 2.0;
		if (ivariant->pr_display_areas.contains(z))
		{
			if (!(
				ivariant->pr_display_areas.value(z).top_left_x == 1 &&
				ivariant->pr_display_areas.value(z).top_left_y == 1 &&
				dx == ivariant->pr_display_areas.value(z)
					.bottom_right_x &&
				dy == ivariant->pr_display_areas.value(z)
					.bottom_right_y))
			{
				ax = ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
				ay = ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
				px = ivariant->pr_display_areas.value(z).top_left_x;
				py = ivariant->pr_display_areas.value(z).top_left_y;
				cx = 0 + px + ax/2.0;
				cy = 0 + py + ay/2.0;
			}
		}
		else
		{
			ax = dx;
			ay = dy;
		}
		int idx0 = -2;
		int idx1 = -2;
		int idx2 = -2;
		if (ivariant->pr_text_annotations.contains(z))
		{
			idx0 = z;
		}
		else if (!tmp0 &&
			ivariant->pr_text_annotations.contains(-1))
		{
			idx0 = -1;
			tmp0 = true;
		}
		if (ivariant->pr_graphicobjects.contains(z))
		{
			idx1 = z;
		}
		else if (!tmp1 &&
			ivariant->pr_graphicobjects.contains(-1))
		{
			idx1 = -1;
			tmp1 = true;
		}
		if (!tmp2 &&
			ivariant->pr_display_shutters.contains(-1))
		{
			idx2 = -1;
			tmp2 = true;
		}
		if (rotation != 0)
		{
			TransformType::Pointer transform =
				TransformType::New();
			TransformType::OutputVectorType translation1;
			translation1[0] = -cx;
			translation1[1] = -cy;
			TransformType::OutputVectorType translation2;
			translation2[0] = cx;
			translation2[1] = cy;
			const double a = rotation*0.0174532925199432957692369;
			transform->SetIdentity();
			transform->Translate(translation1);
			transform->Rotate2D(a, false);
			transform->Translate(translation2, false);
			if (idx0 >= -1)
			{
				const int j =
					ivariant->pr_text_annotations
						.value(idx0).size();
				for (int k = 0; k < j; ++k)
				{
					if (ivariant->pr_text_annotations
						.value(idx0)
						.at(k)
						.BoundingBoxAnnotationUnits == QString("PIXEL"))
					{
						{
							TransformType::InputPointType in;
							in[0] = ivariant->pr_text_annotations
								.value(idx0)
								.at(k)
								.bb_top_left_x;
							in[1] = ivariant->pr_text_annotations
								.value(idx0)
								.at(k)
								.bb_top_left_y;
							TransformType::OutputPointType out =
								transform->TransformPoint(in);
							if (flip)
							{
								if (out[0] < cx)
								{
									const double ix = out[0] + 2*(cx - out[0]);
									ivariant->pr_text_annotations[idx0][k]
										.bb_top_left_x = ix;
								}
								else if (out[0] > cx)
								{
									const double ix = out[0] - 2*(out[0] - cx);
									ivariant->pr_text_annotations[idx0][k]
										.bb_top_left_x = ix;
								}
								else
								{
									ivariant->pr_text_annotations[idx0][k]
										.bb_top_left_x = out[0];
								}
								ivariant->pr_text_annotations[idx0][k].bb_top_left_y = out[1];
							}
							else
							{
								ivariant->pr_text_annotations[idx0][k].bb_top_left_x = out[0];
								ivariant->pr_text_annotations[idx0][k].bb_top_left_y = out[1];
							}
						}
						{
							TransformType::InputPointType in;
							in[0] = ivariant->pr_text_annotations
								.value(idx0)
								.at(k)
								.bb_bottom_right_x;
							in[1] = ivariant->pr_text_annotations
								.value(idx0)
								.at(k)
								.bb_bottom_right_y;
							TransformType::OutputPointType out =
								transform->TransformPoint(in);
							if (flip)
							{
								if (out[0] < cx)
								{
									const double ix = out[0] + 2*(cx - out[0]);
									ivariant->pr_text_annotations[idx0][k]
										.bb_bottom_right_x = ix;
								}
								else if (out[0] > cx)
								{
									const double ix = out[0] - 2*(out[0] - cx);
									ivariant->pr_text_annotations[idx0][k]
										.bb_bottom_right_x = ix;
								}
								else
								{
									ivariant->pr_text_annotations[idx0][k]
										.bb_bottom_right_x = out[0];
								}
								ivariant->pr_text_annotations[idx0][k].bb_bottom_right_y = out[1];
							}
							else
							{
								ivariant->pr_text_annotations[idx0][k].bb_bottom_right_x = out[0];
								ivariant->pr_text_annotations[idx0][k].bb_bottom_right_y = out[1];
							}
						}
					}
					if (ivariant->pr_text_annotations
						.value(idx0)
						.at(k)
						.AnchorPointAnnotationUnits == QString("PIXEL"))
					{
						TransformType::InputPointType in;
						in[0] = ivariant->pr_text_annotations
							.value(idx0)
							.at(k)
							.anchor_x;
						in[1] = ivariant->pr_text_annotations
							.value(idx0)
							.at(k)
							.anchor_y;
						TransformType::OutputPointType out =
							transform->TransformPoint(in);
						if (flip)
						{
							if (out[0] < cx)
							{
								const double ix = out[0] + 2*(cx - out[0]);
								ivariant->pr_text_annotations[idx0][k]
									.anchor_x = ix;
							}
							else if (out[0] > cx)
							{
								const double ix = out[0] - 2*(out[0] - cx);
								ivariant->pr_text_annotations[idx0][k]
									.anchor_x = ix;
							}
							else
							{
								ivariant->pr_text_annotations[idx0][k]
									.anchor_x = out[0];
							}
							ivariant->pr_text_annotations[idx0][k].anchor_y = out[1];
						}
						else
						{
							ivariant->pr_text_annotations[idx0][k].anchor_x = out[0];
							ivariant->pr_text_annotations[idx0][k].anchor_y = out[1];
						}
					}
				}
			}
			if (idx1 >= -1)
			{
				const int j =
					ivariant->pr_graphicobjects
						.value(idx1).size();
				for (int k = 0; k < j; ++k)
				{
					if ((ivariant->pr_graphicobjects
							.value(idx1)
							.at(k)
							.GraphicAnnotationUnits == QString("PIXEL"))
						&&
						(ivariant->pr_graphicobjects
							.value(idx1)
							.at(k)
							.GraphicData.empty() == false)
						&&
						(ivariant->pr_graphicobjects
							.value(idx1)
							.at(k)
							.GraphicData.size()%2 == 0))
					{
						for (
							unsigned int u = 0;
							u < ivariant->pr_graphicobjects
								.value(idx1)
								.at(k)
								.GraphicData.size();
							u+=2)
						{
							TransformType::InputPointType in;
							in[0] = ivariant->pr_graphicobjects
								.value(idx1)
								.at(k)
								.GraphicData.at(u);
							in[1] = ivariant->pr_graphicobjects
								.value(idx1)
								.at(k)
								.GraphicData.at(u+1);
							TransformType::OutputPointType out =
								transform->TransformPoint(in);
							if (flip)
							{
								if (out[0] < cx)
								{
									const double ix = out[0] + 2*(cx - out[0]);
									ivariant->pr_graphicobjects[idx1][k]
										.GraphicData[u] = ix;
								}
								else if (out[0] > cx)
								{
									const double ix = out[0] - 2*(out[0] - cx);
									ivariant->pr_graphicobjects[idx1][k]
										.GraphicData[u] = ix;
								}
								else
								{
									ivariant->pr_graphicobjects[idx1][k]
										.GraphicData[u] = out[0];
								}
								ivariant->pr_graphicobjects[idx1][k].GraphicData[u+1] = out[1];
							}
							else
							{
								ivariant->pr_graphicobjects[idx1][k].GraphicData[u]   = out[0];
								ivariant->pr_graphicobjects[idx1][k].GraphicData[u+1] = out[1];
							}
						}
					}
				}
			}
			if (idx2 >= -1)
			{
				const PRDisplayShutter & a =
					ivariant->pr_display_shutters.value(idx2);
				if (!(
					a.ShutterLeftVerticalEdge    == -1 && 
					a.ShutterRightVerticalEdge   == -1 &&
					a.ShutterUpperHorizontalEdge == -1 &&
					a.ShutterLowerHorizontalEdge == -1))
				{
					TransformType::InputPointType in1;
					in1[0] = (double)
						a.ShutterLeftVerticalEdge;
					in1[1] = (double)
						a.ShutterUpperHorizontalEdge;
					TransformType::OutputPointType out1 =
						transform->TransformPoint(in1);
					TransformType::InputPointType in2;
					in2[0] = (double)
						a.ShutterRightVerticalEdge;
					in2[1] = (double)
						a.ShutterLowerHorizontalEdge;
					TransformType::OutputPointType out2 =
						transform->TransformPoint(in2);
					if (flip)
					{
						if (out1[0] < cx)
						{
							const double ix = out1[0] + 2*(cx - out1[0]);
							ivariant->pr_display_shutters[idx2]
								.ShutterLeftVerticalEdge = (int)ix;
						}
						else if (out1[0] > cx)
						{
							const double ix = out1[0] - 2*(out1[0] - cx);
							ivariant->pr_display_shutters[idx2]
								.ShutterLeftVerticalEdge = (int)ix;
						}
						else
						{
							ivariant->pr_display_shutters[idx2]
								.ShutterLeftVerticalEdge = (int)out1[0];
						}
						ivariant->pr_display_shutters[idx2]
							.ShutterUpperHorizontalEdge = (int)out1[1];
						if (out2[0] < cx)
						{
							const double ix = out2[0] + 2*(cx - out2[0]);
							ivariant->pr_display_shutters[idx2]
								.ShutterRightVerticalEdge = (int)ix;
						}
						else if (out2[0] > cx)
						{
							const double ix = out2[0] - 2*(out2[0] - cx);
							ivariant->pr_display_shutters[idx2]
								.ShutterRightVerticalEdge = (int)ix;
						}
						else
						{
							ivariant->pr_display_shutters[idx2]
								.ShutterRightVerticalEdge = (int)out2[0];
						}
						ivariant->pr_display_shutters[idx2]
							.ShutterLowerHorizontalEdge = (int)out2[1];
					}
					else
					{
						ivariant->pr_display_shutters[idx2]
							.ShutterLeftVerticalEdge = (int)out1[0];
						ivariant->pr_display_shutters[idx2]
							.ShutterUpperHorizontalEdge = (int)out1[1];
						ivariant->pr_display_shutters[idx2]
							.ShutterRightVerticalEdge = (int)out2[0];
						ivariant->pr_display_shutters[idx2]
							.ShutterLowerHorizontalEdge = (int)out2[1];
					}
				}
				if (!(
					a.CenterofCircularShutter_x == -1 &&
					a.CenterofCircularShutter_y == -1 &&
					a.RadiusofCircularShutter   == -1))
				{
					TransformType::InputPointType in;
					in[0] = (double)
						a.CenterofCircularShutter_x;
					in[1] = (double)
						a.CenterofCircularShutter_y;
					TransformType::OutputPointType out =
						transform->TransformPoint(in);
					if (flip)
					{
						if (out[0] < cx)
						{
							const double ix = out[0] + 2*(cx - out[0]);
							ivariant->pr_display_shutters[idx2]
								.CenterofCircularShutter_x =
									(int)ix;
						}
						else if (out[0] > cx)
						{
							const double ix = out[0] - 2*(out[0] - cx);
							ivariant->pr_display_shutters[idx2]
								.CenterofCircularShutter_x =
									(int)ix;
						}
						else
						{
							ivariant->pr_display_shutters[idx2]
								.CenterofCircularShutter_x =
									(int)out[0];
						}
						ivariant->pr_display_shutters[idx2]
							.CenterofCircularShutter_y =
									(int)out[1];
					}
					else
					{
						ivariant->pr_display_shutters[idx2]
							.CenterofCircularShutter_x =
									(int)out[0];
						ivariant->pr_display_shutters[idx2]
							.CenterofCircularShutter_y =
									(int)out[1];
					}
				}
				if ((a.VerticesofthePolygonalShutter.size() > 1) &&
						(a.VerticesofthePolygonalShutter.size()%2 == 0))
				{
					for (unsigned int j = 0;
						j < a.VerticesofthePolygonalShutter.size(); j+=2)
					{
						TransformType::InputPointType in;
						in[0] = (double)
							a.VerticesofthePolygonalShutter.at(j+1);
						in[1] = (double)
							a.VerticesofthePolygonalShutter.at(j);
						TransformType::OutputPointType out =
							transform->TransformPoint(in);
						if (flip)
						{
							if (out[0] < cx)
							{
								const double ix = out[0] + 2*(cx - out[0]);
								ivariant->pr_display_shutters[idx2]
									.VerticesofthePolygonalShutter[j+1] =
										(int)ix;
							}
							else if (out[0] > cx)
							{
								const double ix = out[0] - 2*(out[0] - cx);
								ivariant->pr_display_shutters[idx2]
									.VerticesofthePolygonalShutter[j+1] =
										(int)ix;
							}
							else
							{
								ivariant->pr_display_shutters[idx2]
									.VerticesofthePolygonalShutter[j+1] =
										(int)out[0];
							}
							ivariant->pr_display_shutters[idx2]
								.VerticesofthePolygonalShutter[j] =
									(int)out[1];
						}
						else
						{
							ivariant->pr_display_shutters[idx2]
								.VerticesofthePolygonalShutter[j+1] =
									(int)out[0];
							ivariant->pr_display_shutters[idx2]
								.VerticesofthePolygonalShutter[j] =
									(int)out[1];
						}
					}
				}
			}
		}
		else if (flip)
		{
			if (idx0 >= -1)
			{
				const int j = ivariant->pr_text_annotations.value(idx0).size();
				for (int k = 0; k < j; ++k)
				{
					if (ivariant->pr_text_annotations
						.value(idx0)
						.at(k)
						.BoundingBoxAnnotationUnits == QString("PIXEL"))
					{
						const double in0 = ivariant->pr_text_annotations
							.value(idx0)
							.at(k)
							.bb_top_left_x;
						if (in0 < cx)
						{
							const double ix = in0 + 2*(cx - in0);
							ivariant->pr_text_annotations[idx0][k]
								.bb_top_left_x = ix;
						}
						else if (in0 > cx)
						{
							const double ix = in0 - 2*(in0 - cx);
							ivariant->pr_text_annotations[idx0][k]
								.bb_top_left_x = ix;
						}
						const double in1 = ivariant->pr_text_annotations
							.value(idx0)
							.at(k)
							.bb_bottom_right_x;
						if (in1 < cx)
						{
							const double ix = in1 + 2*(cx - in1);
							ivariant->pr_text_annotations[idx0][k]
								.bb_bottom_right_x = ix;
						}
						else if (in1 > cx)
						{
							const double ix = in1 - 2*(in1 - cx);
							ivariant->pr_text_annotations[idx0][k]
								.bb_bottom_right_x = ix;
						}
					}
					if (ivariant->pr_text_annotations
						.value(idx0)
						.at(k)
						.AnchorPointAnnotationUnits == QString("PIXEL"))
					{
						const double in0 = ivariant->pr_text_annotations
							.value(idx0)
							.at(k)
							.anchor_x;
						if (in0 < cx)
						{
							const double ix = in0 + 2*(cx - in0);
							ivariant->pr_text_annotations[idx0][k]
								.anchor_x = ix;
						}
						else if (in0 > cx)
						{
							const double ix = in0 - 2*(in0 - cx);
							ivariant->pr_text_annotations[idx0][k]
								.anchor_x = ix;
						}
					}
				}
			}
			if (idx1 >= -1)
			{
				const int j = ivariant->pr_graphicobjects.value(idx1).size();
				for (int k = 0; k < j; ++k)
				{
					if ((ivariant->pr_graphicobjects
							.value(idx1)
							.at(k)
							.GraphicAnnotationUnits == QString("PIXEL"))
						&&
						(ivariant->pr_graphicobjects
							.value(idx1)
							.at(k)
							.GraphicData.empty() == false)
						&&
						(ivariant->pr_graphicobjects
							.value(idx1)
							.at(k)
							.GraphicData.size()%2 == 0))
					{
						for (
							unsigned int u = 0;
							u < ivariant->pr_graphicobjects
								.value(idx1)
								.at(k)
								.GraphicData.size();
							u+=2)
						{
							const double in0 = ivariant->pr_graphicobjects
								.value(idx1)
								.at(k)
								.GraphicData.at(u);
							if (in0 < cx)
							{
								const double ix = in0 + 2*(cx - in0);
								ivariant->pr_graphicobjects[idx1][k]
									.GraphicData[u] = ix;
							}
							else if (in0 > cx)
							{
								const double ix = in0 - 2*(in0 - cx);
								ivariant->pr_graphicobjects[idx1][k]
									.GraphicData[u] = ix;
							}
						}
					}
				}
			}
			if (idx2 >= -1)
			{
				const PRDisplayShutter & a =
					ivariant->pr_display_shutters.value(idx2);
				if (!(
					a.ShutterLeftVerticalEdge    == -1 && 
					a.ShutterRightVerticalEdge   == -1 &&
					a.ShutterUpperHorizontalEdge == -1 &&
					a.ShutterLowerHorizontalEdge == -1))
				{
					const double out1 =
						ivariant->pr_display_shutters[idx2]
							.ShutterLeftVerticalEdge;
					const double out2 =
						ivariant->pr_display_shutters[idx2]
							.ShutterRightVerticalEdge;
					if (out1 < cx)
					{
						const double ix = out1 + 2*(cx - out1);
						ivariant->pr_display_shutters[idx2]
							.ShutterLeftVerticalEdge = (int)ix;
					}
					else if (out1 > cx)
					{
						const double ix = out1 - 2*(out1 - cx);
						ivariant->pr_display_shutters[idx2]
							.ShutterLeftVerticalEdge = (int)ix;
					}
					if (out2 < cx)
					{
						const double ix = out2 + 2*(cx - out2);
						ivariant->pr_display_shutters[idx2]
							.ShutterRightVerticalEdge = (int)ix;
					}
					else if (out2 > cx)
					{
						const double ix = out2 - 2*(out2 - cx);
						ivariant->pr_display_shutters[idx2]
							.ShutterRightVerticalEdge = (int)ix;
					}
				}
				if (!(
					a.CenterofCircularShutter_x == -1 &&
					a.CenterofCircularShutter_y == -1 &&
					a.RadiusofCircularShutter   == -1))
				{
					const double out =
						ivariant->pr_display_shutters[idx2]
							.CenterofCircularShutter_x;
					if (out < cx)
					{
						const double ix = out + 2*(cx - out);
						ivariant->pr_display_shutters[idx2]
							.CenterofCircularShutter_x =
								(int)ix;
					}
					else if (out > cx)
					{
						const double ix = out - 2*(out - cx);
						ivariant->pr_display_shutters[idx2]
							.CenterofCircularShutter_x =
								(int)ix;
					}
				}
				if ((a.VerticesofthePolygonalShutter.size() > 1) &&
						(a.VerticesofthePolygonalShutter.size()%2 == 0))
				{
					for (unsigned int j = 0;
						j < a.VerticesofthePolygonalShutter.size(); j+=2)
					{
						const double out =
							ivariant->pr_display_shutters[idx2]
								.VerticesofthePolygonalShutter.at(j+1);
						if (out < cx)
						{
							const double ix = out + 2*(cx - out);
							ivariant->pr_display_shutters[idx2]
								.VerticesofthePolygonalShutter[j+1] =
									(int)ix;
						}
						else if (out > cx)
						{
							const double ix = out - 2*(out - cx);
							ivariant->pr_display_shutters[idx2]
								.VerticesofthePolygonalShutter[j+1] =
									(int)ix;
						}
					}
				}
			}
		}
	}
}

template class itk::NumericTraits<RGBPixelUC>;

template<typename T, typename T2d> QString rotate_flip_slice_by_slice(
	const typename T::Pointer & image,
	typename T::Pointer & out_image,
	const ImageVariant * ivariant,
	const double rotation,
	const bool flip)
{
	if (image.IsNull()) return QString("image.IsNull()");
	try
	{
		out_image = T::New();
		out_image->SetOrigin(image->GetOrigin());
		out_image->SetSpacing(image->GetSpacing());
		out_image->SetDirection(image->GetDirection());
		out_image->SetRegions(image->GetLargestPossibleRegion());
		out_image->Allocate();
		out_image->FillBuffer(
			itk::NumericTraits<typename T::PixelType>::ZeroValue());
	}
	catch (itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	typedef itk::FlipImageFilter<T2d> FlipFilterType;
	typedef itk::ResampleImageFilter<T2d,T2d> ResampleType;
	typedef itk::AffineTransform<double, 2> TransformType;
	typedef itk::LinearInterpolateImageFunction<T2d, double>
		InterpolatorType;
	typedef itk::ImageSliceIteratorWithIndex<T> SliceIterator;
	typedef itk::ImageLinearConstIteratorWithIndex<T2d> ConstIterator;
	typedef itk::ImageLinearIteratorWithIndex<T2d> LinearIterator;
	typedef itk::ImageDuplicator<T2d> DuplicatorType;
	const int dz = (int)image->GetLargestPossibleRegion().GetSize()[2];
	for (int z = 0; z < dz; ++z)
	{
		typename T2d::Pointer tmp0;
		typename T2d::Pointer tmp1;
		const QString error = extract_one_slice<T,T2d>(image, tmp0, z);
		const int dx = (int)image->GetLargestPossibleRegion().GetSize()[0];
		const int dy = (int)image->GetLargestPossibleRegion().GetSize()[1];
		int ax = 0;
		int ay = 0;
		int px = 0;
		int py = 0;
		if (tmp0.IsNull()) return QString("tmp0.IsNull()");
		if (error.isEmpty())
		{
			bool apply_display_area = false;
			if (ivariant->pr_display_areas.contains(z))
			{
				if (!(
					ivariant->pr_display_areas.value(z).top_left_x == 1 &&
					ivariant->pr_display_areas.value(z).top_left_y == 1 &&
					dx == ivariant->pr_display_areas.value(z)
						.bottom_right_x &&
					dy == ivariant->pr_display_areas.value(z)
						.bottom_right_y))
				{
					ax = ivariant->pr_display_areas.value(z)
							.bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					ay = ivariant->pr_display_areas.value(z)
							.bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					px = ivariant->pr_display_areas.value(z).top_left_x;
					py = ivariant->pr_display_areas.value(z).top_left_y;
					apply_display_area = true;
				}
			}
			if (!apply_display_area)
			{
				if (rotation != 0.0)
				{
					const unsigned int dx0 =
						tmp0->GetLargestPossibleRegion().GetSize()[0];
					const unsigned int dy0 =
						tmp0->GetLargestPossibleRegion().GetSize()[1];
					const double sx = tmp0->GetSpacing()[0];
					const double sy = tmp0->GetSpacing()[1];
					const double ox = tmp0->GetOrigin()[0];
					const double oy = tmp0->GetOrigin()[1];
					typename TransformType::Pointer transform =
						TransformType::New();
					typename ResampleType::Pointer filter0 =
						ResampleType::New();
					typename InterpolatorType::Pointer interpolator =
						InterpolatorType::New();
					typename FlipFilterType::Pointer filter1 =
						FlipFilterType::New();
					const double cx = ox + dx0 * sx / 2.0;
					const double cy = oy + dy0 * sy / 2.0;
					typename TransformType::OutputVectorType translation1;
					translation1[0] = -cx;
					translation1[1] = -cy;
					typename TransformType::OutputVectorType translation2;
					translation2[0] = cx;
					translation2[1] = cy;
					const double a = rotation*0.0174532925199432957692369;
					itk::FixedArray<bool, 2> f;
					f[0] = true;
					f[1] = false;
					transform->SetIdentity();
					transform->Translate(translation1);
					transform->Rotate2D(-a, false);
					transform->Translate(translation2, false);
					try
					{
						filter0->SetInput(tmp0);
						filter0->SetInterpolator(interpolator);
						filter0->SetDefaultPixelValue(
							itk::NumericTraits<
								typename T2d::PixelType>::ZeroValue());
						filter0->SetTransform(transform);
						filter0->SetOutputOrigin(tmp0->GetOrigin());
						filter0->SetOutputSpacing(tmp0->GetSpacing());
						filter0->SetOutputDirection(tmp0->GetDirection());
						filter0->SetSize(
							tmp0->GetLargestPossibleRegion().GetSize());
						if (flip)
						{
							filter1->SetInput(filter0->GetOutput());
							filter1->SetFlipAxes(f);
							filter1->Update();
							tmp1 = filter1->GetOutput();
						}
						else
						{
							filter0->Update();
							tmp1 = filter0->GetOutput();
						}
					}
					catch (itk::ExceptionObject & ex)
					{
						return QString(ex.GetDescription());
					}
				}
				else if (flip)
				{
					typename FlipFilterType::Pointer filter =
						FlipFilterType::New();
					itk::FixedArray<bool, 2> f;
					f[0] = true;
					f[1] = false;
					try
					{
						filter->SetInput(tmp0);
						filter->SetFlipAxes(f);
						filter->Update();
						tmp1 = filter->GetOutput();
					}
					catch (itk::ExceptionObject & ex)
					{
						return QString(ex.GetDescription());
					}
				}
				else
				{
					tmp1 = tmp0;
				}
			}
			else
			{
				if (rotation != 0.0||flip)
				{
					if (rotation != 0.0)
					{
						const double sx = tmp0->GetSpacing()[0];
						const double sy = tmp0->GetSpacing()[1];
						const double ox = tmp0->GetOrigin()[0];
						const double oy = tmp0->GetOrigin()[1];
						typename TransformType::Pointer transform =
							TransformType::New();
						typename ResampleType::Pointer filter0 =
							ResampleType::New();
						typename InterpolatorType::Pointer interpolator =
							InterpolatorType::New();
						const double cx = ox + px*sx + ax*sx/2.0;
						const double cy = oy + py*sy + ay*sy/2.0;
						typename TransformType::OutputVectorType
							translation1;
						translation1[0] = -cx;
						translation1[1] = -cy;
						typename TransformType::OutputVectorType
							translation2;
						translation2[0] = cx;
						translation2[1] = cy;
						const double a =
							rotation*0.0174532925199432957692369;
						transform->SetIdentity();
						transform->Translate(translation1);
						transform->Rotate2D(-a, false);
						transform->Translate(translation2, false);
						try
						{
							filter0->SetInput(tmp0);
							filter0->SetInterpolator(interpolator);
							filter0->SetDefaultPixelValue(
								itk::NumericTraits<
									typename
										T2d::PixelType>::ZeroValue());
							filter0->SetTransform(transform);
							filter0->SetOutputOrigin(tmp0->GetOrigin());
							filter0->SetOutputSpacing(tmp0->GetSpacing());
							filter0->SetOutputDirection(
								tmp0->GetDirection());
							filter0->SetSize(
								tmp0->GetLargestPossibleRegion()
									.GetSize());
							filter0->Update();
							tmp1 = filter0->GetOutput();
						}
						catch (itk::ExceptionObject & ex)
						{
							return QString(ex.GetDescription());
						}
					}
					else
					{
						tmp1 = tmp0;
					}
					if (flip)
					{
						typename T2d::Pointer tmp2;
						typename DuplicatorType::Pointer
							duplicator = DuplicatorType::New();
						try
						{
							duplicator->SetInputImage(tmp1);
							duplicator->Update();
							tmp2 = duplicator->GetOutput();
							LinearIterator it(
								tmp1,
								tmp1->GetLargestPossibleRegion());
							it.SetDirection(0);
							it.GoToBegin();
							const int j__ = px + ax/2;
							int x__ = 0;
							int y__ = 0;
							const int idimx =
								tmp1->GetLargestPossibleRegion().GetSize()[0];
							while(!it.IsAtEnd())
							{
								while(!it.IsAtEndOfLine())
								{
									if (x__ < j__)
									{
										const int ix = x__ + 2*(j__ - x__);
										if (ix >= idimx || ix < 0)
										{
											it.Set(itk::NumericTraits<
												typename
													T2d::PixelType>::ZeroValue());
										}
										else
										{
											typename T2d::IndexType i;
											i[0] = ix;
											i[1] =  y__;
											it.Set(tmp2->GetPixel(i));
										}
									}
									else if (x__ > j__)
									{
										const int ix = x__ - 2*(x__ - j__);
										if (ix >= idimx || ix < 0)
										{
											it.Set(itk::NumericTraits<
												typename
													T2d::PixelType>::ZeroValue());
										}
										else
										{
											typename T2d::IndexType i;
											i[0] = ix;
											i[1] =  y__;
											it.Set(tmp2->GetPixel(i));
										}
									}
									else
									{
										typename T2d::IndexType i;
										i[0] = x__;
										i[1] = y__;
										it.Set(tmp2->GetPixel(i));
									}
									++x__;
									++it;
								}
								x__ = 0;
								++y__;
								it.NextLine();
							}
						}
						catch(itk::ExceptionObject & ex)
						{
							return QString(ex.GetDescription());
						}
					}
				}
				else
				{
					tmp1 = tmp0;
				}
			}
			if (tmp1.IsNull()) return QString("tmp1.IsNull()");
			if (tmp1->GetLargestPossibleRegion().GetSize()[0] == 0 ||
					tmp1->GetLargestPossibleRegion().GetSize()[1] == 0)
			{
				return QString("Internal error");
			}
			QApplication::processEvents();
			try
			{
				ConstIterator it0(
					tmp1,
					tmp1->GetLargestPossibleRegion());
				it0.SetDirection(0);
				SliceIterator it2(
					out_image,
					out_image->GetLargestPossibleRegion());
				it2.SetFirstDirection(0);
				it2.SetSecondDirection(1);
				int j = 0;
				it0.GoToBegin();
				it2.GoToBegin();
				while(!(it2.IsAtEnd() || it0.IsAtEnd()))
				{
					if (j == z)
					{
						while (!(it2.IsAtEndOfSlice() || it0.IsAtEnd()))
						{
							while (!it2.IsAtEndOfLine())
							{
								if (!it0.IsAtEnd())
								{
									it2.Set(static_cast<
										typename T::PixelType>(
											it0.Get()));
									++it0;
								}
								++it2;
							}
							it0.NextLine();
							it2.NextLine();
						}
					}
					it2.NextSlice();
					++j;
				}
			}
			catch(itk::ExceptionObject & ex)
			{
				return QString(ex.GetDescription());
			}
			QApplication::processEvents();
		}
		else
		{
			return error;
		}
	}
	if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	else return QString("Output image is NULL");
	return QString("");
}

template QString rotate_flip_slice_by_slice<RGBImageTypeUC, RGBImage2DTypeUC>(
	const RGBImageTypeUC::Pointer &,
	RGBImageTypeUC::Pointer &,
	const ImageVariant *,
	const double,
	const bool);

template<typename T, typename T2d> QString levels_slice_by_slice(
	const typename T::Pointer & image,
	typename ImageTypeUC::Pointer & out_image,
	const QMap<int, double> & widths,
	const QMap<int, double> & centers,
	const QMap<int, QString> & lut_functions,
	const QMap<int, QStringList> & refs,
	const SOPInstanceUids & slices_uids)
{
	if (image.IsNull()) return QString("image.IsNull()");
	try
	{
		out_image = ImageTypeUC::New();
		out_image->SetOrigin(static_cast<
			typename ImageTypeUC::PointType>(
				image->GetOrigin()));
		out_image->SetSpacing(static_cast<
			typename ImageTypeUC::SpacingType>(
				image->GetSpacing()));
		out_image->SetDirection(static_cast<
			typename ImageTypeUC::DirectionType>(
				image->GetDirection()));
		out_image->SetRegions(static_cast<
			typename ImageTypeUC::RegionType>(
				image->GetLargestPossibleRegion()));
		out_image->Allocate();
		out_image->FillBuffer(0);
	}
	catch (itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	typedef itk::IntensityWindowingImageFilter<T2d, Image2DTypeUC>
		LinearFilterType;
	typedef itk::Sigmoid2ImageFilter<T2d, Image2DTypeUC>
		SigmoidFilterType;
	typedef itk::RescaleIntensityImageFilter<Image2DTypeUC, Image2DTypeUC>
		RescaleFilterType;
	typedef itk::ImageSliceIteratorWithIndex<ImageTypeUC>
		SliceIterator;
	typedef itk::ImageLinearConstIteratorWithIndex<Image2DTypeUC>
		ConstIterator;
	for (
		QMap<int, QStringList>::const_iterator it = refs.constBegin();
		it != refs.constEnd();
		++it)
	{
		const int k = it.key();
		const QStringList & l = it.value();
		for (int z = 0; z < l.size(); z+=2)
		{
			const QString uid =
				l.at(z+0).trimmed().remove(QChar('\0'));
			const QString frames =
				l.at(z+1)
					.trimmed()
					.remove(QChar(' '))
					.remove(QChar('\0'));
			QList<int> idxs;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				Qt::SkipEmptyParts);
#else
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				QString::SkipEmptyParts);
#endif
			if (frames_tmp1.size() < 1)
			{
				SOPInstanceUids::const_iterator it1 =
					slices_uids.constBegin();
				while (it1 != slices_uids.constEnd())
				{
					if (uid == it1.value()
						.trimmed()
						.remove(QChar('\0')))
					{
						idxs.push_back(it1.key());
					}
					++it1;
				}
			}
			else
			{
				for (int x = 0; x < frames_tmp1.size(); ++x)
				{
					bool tmp99 = false;
					const int tmp98 = QVariant(
						frames_tmp1.at(x)
							.trimmed()
							.remove(QChar('\0'))).toInt(&tmp99);
					if (tmp99 && tmp98 > 0)
					{
						idxs.push_back(tmp98-1);
					}
				}
			}
			if (idxs.empty())
			{
				std::cout
					<< "idxs.size()<1 k=" << k << " "
					<< uid.toStdString()
					<< std::endl;
				continue;
			}
			for (int x = 0; x < idxs.size(); ++x)
			{
				const QString uid_ =
					(slices_uids.contains(idxs.at(x)))
					?
					slices_uids.value(idxs.at(x))
						.trimmed()
						.remove(QChar('\0'))
					:
					QString("");
				if (!uid_.isEmpty() && (uid_ == uid))
				{
					typename T2d::Pointer tmp0;
					typename Image2DTypeUC::Pointer tmp1;
					const QString error =
						extract_one_slice<T,T2d>(image, tmp0, idxs.at(x));
					if (tmp0.IsNull()) return QString("tmp0.IsNull()");
					if (error.isEmpty())
					{
						if (
							lut_functions.contains(k) &&
							(lut_functions.value(k)
								.trimmed()
								.remove(QChar('\0'))
								.toUpper() ==
							QString("SIGMOID")))
						{
							typename SigmoidFilterType::Pointer
								filter0 = SigmoidFilterType::New();
							typename RescaleFilterType::Pointer
								filter1 = RescaleFilterType::New();
							try
							{
								filter0->SetInput(tmp0);
								filter0->SetAlpha(
									static_cast<typename T2d::PixelType>(
										widths.value(k)));
								filter0->SetBeta(
									static_cast<typename T2d::PixelType>(
										centers.value(k)));
								filter0->SetOutputMinimum(
									static_cast<typename
										Image2DTypeUC::PixelType>(0));
								filter0->SetOutputMaximum(
									static_cast<typename
										Image2DTypeUC::PixelType>(255));
								filter1->SetInput(filter0->GetOutput());
								filter1->Update();
								tmp1 = filter1->GetOutput();
							}
							catch (itk::ExceptionObject & ex)
							{ return QString( ex.GetDescription()); }
						}
						else
						{
							typename LinearFilterType::Pointer
								filter0 = LinearFilterType::New();
							typename RescaleFilterType::Pointer
								filter1 = RescaleFilterType::New();
							try
							{
								filter0->SetInput(tmp0);
								filter0->SetWindowLevel(
									static_cast<typename T2d::PixelType>(
										widths.value(k)),
									static_cast<typename T2d::PixelType>(
										centers.value(k)));
								filter0->SetOutputMinimum(
									static_cast<
										typename
											Image2DTypeUC::PixelType>(
												0));
								filter0->SetOutputMaximum(
									static_cast<
										typename
											Image2DTypeUC::PixelType>(
												255));
								filter1->SetInput(filter0->GetOutput());
								filter1->Update();
								tmp1 = filter1->GetOutput();
							}
							catch (itk::ExceptionObject & ex)
							{ return QString( ex.GetDescription()); }
						}
						if (tmp1.IsNull()) return QString("tmp1.IsNull()");
						//
						QApplication::processEvents();
						//
						try
						{
							ConstIterator it0(
								tmp1,
								tmp1->GetLargestPossibleRegion());
							it0.SetDirection(0);
							SliceIterator it2(
								out_image,
								out_image->GetLargestPossibleRegion());
							it2.SetFirstDirection(0);
							it2.SetSecondDirection(1);
							int j = 0;
							it0.GoToBegin();
							it2.GoToBegin();
							while(!(it2.IsAtEnd() || it0.IsAtEnd()))
							{
								if (j == idxs.at(x))
								{
									while (!(it2.IsAtEndOfSlice() ||
										it0.IsAtEnd()))
									{
										while (!it2.IsAtEndOfLine())
										{
											if (!it0.IsAtEnd())
											{
												it2.Set(
													static_cast<
														typename
															ImageTypeUC::
																PixelType>(
													it0.Get()));
												++it0;
											}
											++it2;
										}
										it0.NextLine();
										it2.NextLine();
									}
								}
								it2.NextSlice();
								++j;
							}
						}
						catch(itk::ExceptionObject & ex)
						{
							return QString(ex.GetDescription());
						}
						QApplication::processEvents();
					}
					else
					{
						return error;
					}
				}
			}
		}
	}
	if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	else return QString("Output image is NULL");
	return QString("");
}

static void areas_slice_by_slice(
	ImageVariant * v,
	const QMap<int, int> & areasTLx,
	const QMap<int, int> & areasTLy,
	const QMap<int, int> & areasBRx,
	const QMap<int, int> & areasBRy,
	const QMap<int, QStringList> & refs,
	const SOPInstanceUids & slices_uids)
{
	if (!v) return;
	for (
		QMap<int, QStringList>::const_iterator it = refs.constBegin();
		it != refs.constEnd();
		++it)
	{
		const int k = it.key();
		const QStringList & l = it.value();
		for (int z = 0; z < l.size(); z+=2)
		{
			const QString uid =
				l.at(z+0).trimmed().remove(QChar('\0'));
			const QString frames =
				l.at(z+1).trimmed().remove(QChar(' ')).remove(QChar('\0'));
			QList<int> idxs;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				Qt::SkipEmptyParts);
#else
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				QString::SkipEmptyParts);
#endif
			if (frames_tmp1.empty())
			{
				SOPInstanceUids::const_iterator it1 =
					slices_uids.constBegin();
				while (it1 != slices_uids.constEnd())
				{
					if (uid == it1.value().trimmed().remove(QChar('\0')))
					{
						idxs.push_back(it1.key());
					}
					++it1;
				}
			}
			else
			{
				for (int x = 0; x < frames_tmp1.size(); ++x)
				{
					bool tmp99 = false;
					const int tmp98 = QVariant(
						frames_tmp1.at(x)
							.trimmed()
							.remove(QChar('\0'))).toInt(&tmp99);
					if (tmp99 && tmp98 > 0)
					{
						idxs.push_back(tmp98-1);
					}
				}
			}
			if (idxs.empty())
			{
				std::cout
					<< "idxs.size()<1 k=" << k << " "
					<< uid.toStdString()
					<< std::endl;
				continue;
			}
			for (int x = 0; x < idxs.size(); ++x)
			{
				const QString uid_ =
					(slices_uids.contains(idxs.at(x)))
					?
					slices_uids.value(idxs.at(x))
						.trimmed()
						.remove(QChar('\0'))
					:
					QString("");
				if (!uid_.isEmpty() && (uid_ == uid))
				{
					const int areaTLx = areasTLx.value(k);
					const int areaTLy = areasTLy.value(k);
					const int areaBRx = areasBRx.value(k);
					const int areaBRy = areasBRy.value(k);
					if (!(
						areaTLx == -1 &&
						areaTLy == -1 &&
						areaBRx == -1 &&
						areaBRy == -1))
					{
						PRDisplayArea a;
						a.top_left_x     = areaTLx;
						a.top_left_y     = areaTLy;
						a.bottom_right_x = areaBRx;
						a.bottom_right_y = areaBRy;
						v->pr_display_areas[idxs.at(x)] = a;
					}
				}
			}
		}
	}
}

static void text_slice_by_slice(
	ImageVariant * v,
	const QMap<int, int>     & has_bb,
	const QMap<int, double>  & bb_top_left_x,
	const QMap<int, double>  & bb_top_left_y,
	const QMap<int, double>  & bb_bottom_r_x,
	const QMap<int, double>  & bb_bottom_r_y,
	const QMap<int, QString> & bb_unit,
	const QMap<int, int>     & has_anchor,
	const QMap<int, double>  & anchor_x,
	const QMap<int, double>  & anchor_y,
	const QMap<int, QString> & a_unit,
	const QMap<int, QString> & a_visibility,
	const QMap<int, QString> & text,
	const QMap<int, int>     & has_textstyle,
	const QMap<int, QString> & FontName,
	const QMap<int, QString> & FontNameType,
	const QMap<int, QString> & CSSFontName,
	const QMap<int, QString> & HorizontalAlignment,
	const QMap<int, QString> & VerticalAlignment,
	const QMap<int, QString> & ShadowStyle,
	const QMap<int, QString> & Underlined,
	const QMap<int, QString> & Bold,
	const QMap<int, QString> & Italic,
	const QMap<int, double>  & ShadowOffsetX,
	const QMap<int, double>  & ShadowOffsetY,
	const QMap<int, double>  & ShadowOpacity,
	const QMap<int, int>     & TextColorCIELabValue_L,
	const QMap<int, int>     & TextColorCIELabValue_a,
	const QMap<int, int>     & TextColorCIELabValue_b,
	const QMap<int, int>     & ShadowColorCIELabValue_L,
	const QMap<int, int>     & ShadowColorCIELabValue_a,
	const QMap<int, int>     & ShadowColorCIELabValue_b,
	const QMap<int, QStringList> & refs,
	const SOPInstanceUids & slices_uids)
{
	if (!v) return;
	for (
		QMap<int, QStringList>::const_iterator it = refs.constBegin();
		it != refs.constEnd();
		++it)
	{
		const int k = it.key();
		const QStringList & l = it.value();
		for (int z = 0; z < l.size(); z+=2)
		{
			const QString uid =
				l.at(z+0).trimmed().remove(QChar('\0'));
			const QString frames =
				l.at(z+1)
					.trimmed()
					.remove(QChar(' '))
					.remove(QChar('\0'));
			QList<int> idxs;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				Qt::SkipEmptyParts);
#else
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				QString::SkipEmptyParts);
#endif
			if (frames_tmp1.empty())
			{
				SOPInstanceUids::const_iterator it1 =
					slices_uids.constBegin();
				while (it1 != slices_uids.constEnd())
				{
					if (uid == it1.value().trimmed().remove(QChar('\0')))
					{
						idxs.push_back(it1.key());
					}
					++it1;
				}
			}
			else
			{
				for (int x = 0; x < frames_tmp1.size(); ++x)
				{
					bool tmp99 = false;
					const int tmp98 = QVariant(
						frames_tmp1.at(x)
							.trimmed()
							.remove(QChar('\0'))).toInt(&tmp99);
					if (tmp99 && tmp98 > 0)
					{
						idxs.push_back(tmp98-1);
					}
				}
			}
			if (idxs.empty())
			{
				std::cout
					<< "idxs.size()<1 k=" << k << " "
					<< uid.toStdString()
					<< std::endl;
				continue;
			}
			for (int x = 0; x < idxs.size(); ++x)
			{
				const QString uid_ =
					(slices_uids.contains(idxs.at(x)))
					?
					slices_uids.value(idxs.at(x))
						.trimmed().remove(QChar('\0'))
					:
					QString("");
				if (!uid_.isEmpty() && (uid_ == uid))
				{
					PRTextAnnotation a;
					a.has_bb = (has_bb.value(k) == 1)
						? true : false;
					a.has_anchor = (has_anchor.value(k) == 1)
						? true : false;
					a.has_textstyle = (has_textstyle.value(k) == 1)
						? true : false;
					a.bb_top_left_x              = bb_top_left_x.value(k);
					a.bb_top_left_y              = bb_top_left_y.value(k);
					a.bb_bottom_right_x          = bb_bottom_r_x.value(k);
					a.bb_bottom_right_y          = bb_bottom_r_y.value(k);
					a.BoundingBoxAnnotationUnits = bb_unit.value(k);
					a.anchor_x                   = anchor_x.value(k);
					a.anchor_y                   = anchor_y.value(k);
					a.AnchorPointAnnotationUnits = a_unit.value(k);
					a.AnchorPointVisibility      = a_visibility.value(k);
					a.UnformattedTextValue       = text.value(k);
					a.FontName                   = FontName.value(k);
					a.FontNameType               = FontNameType.value(k);
					a.CSSFontName                = CSSFontName.value(k);
					a.HorizontalAlignment        =
						HorizontalAlignment.value(k);
					a.VerticalAlignment          =
						VerticalAlignment.value(k);
					a.ShadowStyle                = ShadowStyle.value(k);
					a.Underlined                 = Underlined.value(k);
					a.Bold                       = Bold.value(k);
					a.Italic                     = Italic.value(k);
					a.ShadowOffsetX              = ShadowOffsetX.value(k);
					a.ShadowOffsetY              = ShadowOffsetY.value(k);
					a.ShadowOpacity              = ShadowOpacity.value(k);
					a.TextColorCIELabValue_L     =
						TextColorCIELabValue_L.value(k);
					a.TextColorCIELabValue_a     =
						TextColorCIELabValue_a.value(k);
					a.TextColorCIELabValue_b     =
						TextColorCIELabValue_b.value(k);
					a.ShadowColorCIELabValue_L   =
						ShadowColorCIELabValue_L.value(k);
					a.ShadowColorCIELabValue_a   =
						ShadowColorCIELabValue_a.value(k);
					a.ShadowColorCIELabValue_b   =
						ShadowColorCIELabValue_b.value(k);
					if (v->pr_text_annotations.contains(idxs.at(x)))
					{
						v->pr_text_annotations[idxs.at(x)].push_back(a);
					}
					else
					{
						QList<PRTextAnnotation> tas;
						tas.push_back(a);
						v->pr_text_annotations.insert(idxs.at(x), tas);
					}
				}
			}
		}
	}
}

static void graphic_slice_by_slice(
	ImageVariant * v,
	const QMap<int, QString>  & GraphicType,
	const QMap<int, QString>  & GraphicAnnotationUnits,
	const QMap<int, QVariant> & gdata,
	const QMap<int, QString>  & GraphicFilled,
	const QMap<int, QStringList> & refs,
	const SOPInstanceUids & slices_uids)
{
	if (!v) return;
	for (
		QMap<int, QStringList>::const_iterator it = refs.constBegin();
		it != refs.constEnd();
		++it)
	{
		const int k = it.key();
		const QStringList & l = it.value();
		for (int z = 0; z < l.size(); z+=2)
		{
			const QString uid =
				l.at(z+0).trimmed().remove(QChar('\0'));
			const QString frames =
				l.at(z+1)
					.trimmed()
					.remove(QChar(' '))
					.remove(QChar('\0'));
			QList<int> idxs;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				Qt::SkipEmptyParts);
#else
			const QStringList frames_tmp1 = frames.split(
				QString("\\"),
				QString::SkipEmptyParts);
#endif
			if (frames_tmp1.empty())
			{
				SOPInstanceUids::const_iterator it1 =
					slices_uids.constBegin();
				while (it1 != slices_uids.constEnd())
				{
					if (uid == it1.value().trimmed().remove(QChar('\0')))
					{
						idxs.push_back(it1.key());
					}
					++it1;
				}
			}
			else
			{
				for (int x = 0; x < frames_tmp1.size(); ++x)
				{
					bool tmp99 = false;
					const int tmp98 = QVariant(
						frames_tmp1.at(x)
							.trimmed()
							.remove(QChar('\0'))).toInt(&tmp99);
					if (tmp99 && tmp98 > 0)
					{
						idxs.push_back(tmp98-1);
					}
				}
			}
			if (idxs.empty())
			{
				std::cout
					<< "idxs.size()<1 k=" << k << " "
					<< uid.toStdString()
					<< std::endl;
				continue;
			}
			for (int x = 0; x < idxs.size(); ++x)
			{
				const QString uid_ =
					(slices_uids.contains(idxs.at(x)))
					?
					slices_uids.value(idxs.at(x))
						.trimmed().remove(QChar('\0'))
					:
					QString("");
				if (!uid_.isEmpty() && (uid_ == uid))
				{
					PRGraphicObject a;
					a.GraphicType = GraphicType.value(k);
					a.GraphicAnnotationUnits =
						GraphicAnnotationUnits.value(k);
					a.GraphicData.clear();
					const QList<QVariant> & l465 =
						gdata.value(k).toList();
					for (int z = 0; z < l465.size(); ++z)
					{
						a.GraphicData.push_back(
							static_cast<float>(l465.at(z).toDouble()));
					}
					a.GraphicFilled = GraphicFilled.value(k);
					if (v->pr_graphicobjects.contains(idxs.at(x)))
					{
						v->pr_graphicobjects[idxs.at(x)].push_back(a);
					}
					else
					{
						QList<PRGraphicObject> gas;
						gas.push_back(a);
						v->pr_graphicobjects.insert(idxs.at(x), gas);
					}
				}
			}
		}
	}
}

PrConfigUtils::PrConfigUtils()
{
}

PrConfigUtils::~PrConfigUtils()
{
}

void PrConfigUtils::read_modality_lut(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
#if 0
	// TODO
	const mdcm::Tag tModalityLUTSequence(0x0028,0x3000);
	if (ds.FindDataElement(tModalityLUTSequence))
	{
		std::cout
			<< "Presentation State: Modality"
				" LUT Sequence (0028,3000)" << std::endl;
	}
	// LUT Descriptor(0028,3002)
	// LUT Explanation (0028,3003)
	  // Modality LUT Type (0028,3004)
	  // OD The number in the LUT represents thousands of optical density
	  //   HU Hounsfield Units (CT)
	  //   US Unspecified
	  //   Other values are permitted, but are not defined by the
	  //     DICOM Standard
	// LUT Data (0028,3006)
#endif
	const mdcm::Tag tRescaleIntercept(0x0028,0x1052);
	const mdcm::Tag tRescaleSlope(0x0028,0x1053);
	std::vector<double> RescaleIntercept;
	std::vector<double> RescaleSlope;
	if (DicomUtils::get_ds_values(ds,tRescaleIntercept,RescaleIntercept)
		&&
		DicomUtils::get_ds_values(ds,tRescaleSlope,RescaleSlope) &&
		RescaleIntercept.size() >= 1 &&
		RescaleIntercept.size() == RescaleSlope.size())
	{
		PrConfig c;
		c.id = 1;
		c.desc = QString("rescale");
		c.values.push_back(QVariant(RescaleIntercept.at(0))); // 0
		c.values.push_back(QVariant(RescaleSlope.at(0))); // 1
		ref.prconfig.push_back(c);
	}
}

void PrConfigUtils::read_voi_lut(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	const mdcm::Tag tSoftcopyVOILUTSequence(0x0028,0x3110);
	const mdcm::Tag tVOILUTSequence(0x0028,0x3010);  // TODO
	// LUT Descriptor(0028,3002)
	// LUT Explanation (0028,3003)
	// LUT Data (0028,3006)
	const mdcm::Tag tReferencedImageSequence(0x0008,0x1140);
	const mdcm::Tag tReferencedSOPInstanceUID(0x0008,0x1155);
	const mdcm::Tag tReferencedFrameNumber(0x0008,0x1160);
	const mdcm::Tag tWindowCenter(0x0028,0x1050);
	const mdcm::Tag tWindowWidth(0x0028,0x1051);
	const mdcm::Tag tVOILUTFunction(0x0028,0x1056);
	if (ds.FindDataElement(tSoftcopyVOILUTSequence))
	{
		const mdcm::DataElement & de =
			ds.GetDataElement(tSoftcopyVOILUTSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			de.GetValueAsSQ();
		if (!sq) return;
		const unsigned int number_of_items =
			sq->GetNumberOfItems();
		for (unsigned int x = 0; x < number_of_items; ++x)
		{
			const mdcm::Item & item = sq->GetItem(x+1);
			const mdcm::DataSet & nestedds =
				item.GetNestedDataSet();
			std::vector<double> centers;
			std::vector<double> widths;
			const bool ok1 = DicomUtils::get_ds_values(
				nestedds, tWindowCenter, centers);
			const bool ok2 = DicomUtils::get_ds_values(
				nestedds, tWindowWidth,  widths);
			if (ok1 && ok2 && !centers.empty() && !widths.empty())
			{
				PrConfig c;
				c.id = 2;
				c.desc = QString("VOI LUT");
				c.values.push_back(QVariant(centers.at(0))); // 0
				c.values.push_back(QVariant(widths.at(0)));  // 1
				QString s;
				if (DicomUtils::get_string_value(
						nestedds,tVOILUTFunction,s))
				{
					c.values.push_back(QVariant(s));
				}
				else
				{
					c.values.push_back(QVariant(QString(""))); // 2
				}
				if (nestedds.FindDataElement(tReferencedImageSequence))
				{
					const mdcm::DataElement & de1 =
						nestedds.GetDataElement(tReferencedImageSequence);
					mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 =
						de1.GetValueAsSQ();
					if (!sq1) continue;
					const unsigned int number_of_items1 =
						sq1->GetNumberOfItems();
					for (unsigned int x1 = 0; x1 < number_of_items1; ++x1)
					{
						const mdcm::Item & item1 = sq1->GetItem(x1+1);
						const mdcm::DataSet & nestedds1 =
							item1.GetNestedDataSet();
						QString s1, s2;
						if (DicomUtils::get_string_value(
								nestedds1,tReferencedSOPInstanceUID,s1))
						{
							c.values.push_back(QVariant(s1)); // 3+
							if (DicomUtils::get_string_value(
									nestedds1,tReferencedFrameNumber,s2))
							{
								c.values.push_back(QVariant(s2)); //
							}
							else
							{
								c.values.push_back(QVariant(QString(""))); //
							}
						}

					}
				}
				ref.prconfig.push_back(c);
			}
		}
	}
}

void PrConfigUtils::read_display_areas(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	// Displayed Area Selection Sequence 0070|005a SQ
	// Displayed Area Top Left Hand Corner 0070|0052 SL
	// Displayed Area Bottom Right Hand Corner 0070|0053 SL
	// Pixel Origin Interpretation 0048,0301 CS
		// FRAME
		// relative to individual frame
		// 
		// VOLUME
		// relative to Total Image Matrix
		// 
		// If not present, TLHC and BRHC are defined
		//    relative to the frame pixel origins.
	// Presentation Size Mode 0070|0100 CS
	// Presentation Pixel Aspect Ratio 0070|0102 IS
	// Presentation Pixel Spacing 0070|0101 DS
	// Presentation Pixel Magnification Ratio 0070|0103 FL

	const mdcm::Tag tDisplayedAreaSelectionSequence(0x0070,0x005a);
	const mdcm::Tag tAreaTopLeftHandCorner(0x0070,0x0052);
	const mdcm::Tag tBottomRightHandCorner(0x0070,0x0053);
	const mdcm::Tag tPixelOriginInterpretation(0x0048,0x0301);
	const mdcm::Tag tPresentationSizeMode(0x0070,0x0100);
	const mdcm::Tag tPresentationPixelAspectRatio(0x0070,0x0102);
	const mdcm::Tag tPresentationPixelSpacing(0x0070,0x0101);
	const mdcm::Tag tPresentationPixelMagnificationRatio(0x0070,0x0103);
	const mdcm::Tag tReferencedImageSequence(0x0008,0x1140);
	const mdcm::Tag tReferencedSOPInstanceUID(0x0008,0x1155);
	const mdcm::Tag tReferencedFrameNumber(0x0008,0x1160);
	if (ds.FindDataElement(tDisplayedAreaSelectionSequence))
	{
		const mdcm::DataElement & de =
			ds.GetDataElement(tDisplayedAreaSelectionSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			de.GetValueAsSQ();
		if (!sq) return;
		const unsigned int number_of_items =
			sq->GetNumberOfItems();
		for (unsigned int x = 0; x < number_of_items; ++x)
		{
			PrConfig c;
			c.id = 4;
			c.desc = QString("Display Areas");
			const mdcm::Item & item = sq->GetItem(x+1);
			const mdcm::DataSet & nestedds =
				item.GetNestedDataSet();
			std::vector<int> AreaTopLeftHandCorner;
			if (
				DicomUtils::get_sl_values(
					nestedds,
					tAreaTopLeftHandCorner,
					AreaTopLeftHandCorner) &&
				AreaTopLeftHandCorner.size() == 2)
			{
				c.values.push_back(QVariant(
					AreaTopLeftHandCorner.at(0))); // 0
				c.values.push_back(QVariant(
					AreaTopLeftHandCorner.at(1))); // 1
			}
			else
			{
				c.values.push_back(QVariant((int)-1));
				c.values.push_back(QVariant((int)-1));
			}
			std::vector<int> BottomRightHandCorner;
			if (
				DicomUtils::get_sl_values(
					nestedds,
					tBottomRightHandCorner,
					BottomRightHandCorner) &&
				BottomRightHandCorner.size() == 2)
			{
				c.values.push_back(QVariant(
					BottomRightHandCorner.at(0))); // 2
				c.values.push_back(QVariant(
					BottomRightHandCorner.at(1))); // 3
			}
			else
			{
				c.values.push_back(QVariant((int)-1));
				c.values.push_back(QVariant((int)-1));
			}
			QString PresentationSizeMode;
			if (
				DicomUtils::get_string_value(
					nestedds,
					tPresentationSizeMode,
					PresentationSizeMode))
			{
				c.values.push_back(QVariant(
					PresentationSizeMode
						.trimmed()
						.remove(QChar('\0')))); // 4
			}
			else
			{
				c.values.push_back(QVariant(QString("")));
			}
			std::vector<int> PresentationPixelAspectRatio;
			if (
				DicomUtils::get_is_values(
					nestedds,
					tPresentationPixelAspectRatio,
					PresentationPixelAspectRatio) &&
				PresentationPixelAspectRatio.size() == 2)
			{
				c.values.push_back(QVariant(
					PresentationPixelAspectRatio.at(0))); // 5
				c.values.push_back(QVariant(
					PresentationPixelAspectRatio.at(1))); // 6
			}
			else
			{
				c.values.push_back(QVariant((int)1));
				c.values.push_back(QVariant((int)1));
			}
			std::vector<double> PresentationPixelSpacing;
			if (
				DicomUtils::get_ds_values(
					nestedds,
					tPresentationPixelSpacing,
					PresentationPixelSpacing) &&
				PresentationPixelSpacing.size() == 2)
			{
				c.values.push_back(QVariant(
					PresentationPixelSpacing.at(0))); // 7
				c.values.push_back(QVariant(
					PresentationPixelSpacing.at(1))); // 8
			}
			else
			{
				c.values.push_back(QVariant((double)1));
				c.values.push_back(QVariant((double)1));
			}
			std::vector<float> PresentationPixelMagnificationRatio;
			if (
				DicomUtils::get_fl_values(
					nestedds,
					tPresentationPixelMagnificationRatio,
					PresentationPixelMagnificationRatio) &&
				PresentationPixelMagnificationRatio.size() == 2)
			{
				c.values.push_back(QVariant((double)
					PresentationPixelMagnificationRatio.at(0))); // 9
				c.values.push_back(QVariant((double)
					PresentationPixelMagnificationRatio.at(1))); // 10
			}
			else
			{
				c.values.push_back(QVariant((double)1));
				c.values.push_back(QVariant((double)1));
			}
			//
			if (nestedds.FindDataElement(tReferencedImageSequence))
			{
				const mdcm::DataElement & de1 =
					nestedds.GetDataElement(tReferencedImageSequence);
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 =
					de1.GetValueAsSQ();
				if (!sq1) continue;
				const unsigned int number_of_items1 =
					sq1->GetNumberOfItems();
				for (unsigned int x1 = 0; x1 < number_of_items1; ++x1)
				{
					const mdcm::Item & item1 = sq1->GetItem(x1+1);
					const mdcm::DataSet & nestedds1 =
						item1.GetNestedDataSet();
					QString s1, s2;
					if (DicomUtils::get_string_value(
							nestedds1,tReferencedSOPInstanceUID,s1))
					{
						c.values.push_back(QVariant(s1)); // 11+
						if (DicomUtils::get_string_value(
								nestedds1,tReferencedFrameNumber,s2))
						{
							c.values.push_back(QVariant(s2)); //
						}
						else
						{
							c.values.push_back(QVariant(QString(""))); //
						}
					}
				}
			}
			ref.prconfig.push_back(c);
		}
	}
}

void PrConfigUtils::read_graphic_layers(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	// Graphic Layer Sequence 0070|0060 SQ
	// Graphic Layer 0070|0002 CS
	// Graphic Layer Order 0070|0062 IS
}

void PrConfigUtils::read_spatial_transformation(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	const mdcm::Tag tImageHorizontalFlip(0x0070,0x0041);
	// Y N
	const mdcm::Tag tImageRotation(0x0070,0x0042);
	// clockwise in degrees, 0 90 180 270
	QString ImageHorizontalFlip("");
	const bool flip = DicomUtils::get_string_value(
		ds,tImageHorizontalFlip,ImageHorizontalFlip);
	unsigned short ImageRotation = 0;
	const bool rotation = DicomUtils::get_us_value(
		ds,tImageRotation,&ImageRotation);
	if (rotation||flip)
	{
		PrConfig c;
		c.id = 3;
		c.desc = QString("Spatial");
		if (flip)  // 0
			c.values.push_back(QVariant(ImageHorizontalFlip));
		else
			c.values.push_back(QVariant(QString("")));
		if (rotation)  // 1
			c.values.push_back(QVariant((int)ImageRotation));
		else
			c.values.push_back(QVariant((int)0));
		ref.prconfig.push_back(c);
	}
}

static void get_overlays(
	const mdcm::DataSet & ds,
	std::vector<uint16_t> & l)
{
	mdcm::Tag t(0x6000,0x0000);
	while(true)
	{
		const mdcm::DataElement & e = ds.FindNextDataElement(t);
		if(e.GetTag().GetGroup() > 0x60FF)
		{
			break;
		}
		else if(e.GetTag().IsPrivate())
		{
			t.SetGroup((uint16_t)(e.GetTag().GetGroup()+1));
			t.SetElement(0);
		}
		else
		{
			// This is a potential overlay element, let's check this is not a broken LEADTOOL image,
			// or prova0001.dcm:
			// (5000,0000) UL 0 # 4, 1 GenericGroupLength
			// (6000,0000) UL 0 # 4, 1 GenericGroupLength
			// (6001,0000) UL 28 # 4, 1 PrivateGroupLength
			// (6001,0010) LT [PAPYRUS 3.0] # 12, 1 PrivateCreator
			// (6001,1001) LT (no value available) # 0, 0 Unknown Tag & Data
			// 
			// FIXME:
			// In order to support : mdcmData/SIEMENS_GBS_III-16-ACR_NEMA_1.acr
			//   mdcmDataExtra/mdcmSampleData/images_of_interest/XA_GE_JPEG_02_with_Overlays.dcm
			// I cannot simply check for overlay_group,3000 this would not work
			// I would need a strong euristick
			// 
			t = e.GetTag();
			mdcm::Tag tOverlayData(t.GetGroup(), 0x3000);
			if(ds.FindDataElement(tOverlayData))
			{
				const mdcm::DataElement & eOverlayData =
					ds.GetDataElement(tOverlayData);
				if(!eOverlayData.IsEmpty())
					l.push_back(t.GetGroup());
			}
			t.SetGroup((uint16_t)(t.GetGroup()+2));
			t.SetElement(0);
		}
	}
}

static void read_overlays(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	std::vector<mdcm::Overlay> overlays;
	std::vector<uint16_t> l;
	get_overlays(ds, l);
	const unsigned int s = l.size();
	if (s < 1) return;
	for (unsigned int i = 0; i < s; ++i)
	{
		mdcm::Overlay o;
		mdcm::Tag t(0x6000,0x0000);
		t.SetGroup(l.at(i));
		const mdcm::DataElement & e =
			ds.FindNextDataElement(t);
		mdcm::DataElement e2 = e;
		while(e2.GetTag().GetGroup() == l.at(i))
		{
			o.Update(e2);
			t.SetElement((uint16_t)
				(e2.GetTag().GetElement()+1));
			e2 = ds.FindNextDataElement(t);
		}
		//std::ostringstream u;
		//o.Decompress(u);
		overlays.push_back(o);
	}
	if (overlays.empty()) return;
	QMultiMap<int, SliceOverlay> slice_overlays;
	for (unsigned int i = 0; i < overlays.size(); ++i)
	{
		mdcm::Overlay & o = overlays[i];
		const unsigned int NumberOfFrames =
			(unsigned int)o.GetNumberOfFrames();
		const unsigned int FrameOrigin =
			(unsigned int)o.GetFrameOrigin();
		const int o_dimx = (int)o.GetColumns();
		const int o_dimy = (int)o.GetRows();
		const int o_x    = (int)o.GetOrigin()[0];
		const int o_y    = (int)o.GetOrigin()[1];
		if (NumberOfFrames > 0 && FrameOrigin > 0)
		{
			const size_t obuffer_size = o.GetUnpackBufferLength();
			if (obuffer_size%NumberOfFrames != 0) continue;
			const size_t fbuffer_size = obuffer_size/NumberOfFrames;
			char * tmp0;
			try { tmp0 = new char[obuffer_size]; }
			catch (std::bad_alloc&) { continue; }
			if (!tmp0) continue;
			const bool obuffer_ok = o.GetUnpackBuffer(
				tmp0, obuffer_size);
			if (!obuffer_ok)
			{
				delete [] tmp0;
				continue;
			}
			int idx = FrameOrigin - 1;
			for (unsigned int y = 0; y < NumberOfFrames; ++y)
			{
				SliceOverlay overlay;
				overlay.dimx = o_dimx;
				overlay.dimy = o_dimy;
				overlay.x    = o_x;
				overlay.y    = o_y;
				const size_t p = idx*o_dimx*o_dimy;
				idx++;
				for (size_t j = 0; j < fbuffer_size; ++j)
				{
					const size_t jj = p + j;
					if (jj < obuffer_size)
					{
						overlay.data.push_back(tmp0[jj]);
					}
					else
					{
						std::cout
							<< "warning: read_overlays() jj="
							<< jj << " obuffer_size"
							<< obuffer_size << std::endl;
					}
				}
				slice_overlays.insert(idx-1, overlay);
#if 0
				std::cout
					<< "obuffer_size=" << obuffer_size
					<< " fbuffer_size=" << fbuffer_size
					<< " idx=" << idx-1
					<< " p=" << p << std::endl;
#endif
			}
			delete [] tmp0;
		}
		else
		{
			SliceOverlay overlay;
			overlay.dimx = o_dimx;
			overlay.dimy = o_dimy;
			overlay.x    = o_x;
			overlay.y    = o_y;
			const size_t obuffer_size =
				o.GetUnpackBufferLength();
			char * tmp0;
			try { tmp0 = new char[obuffer_size]; }
			catch (std::bad_alloc&) { continue; }
			if (!tmp0) continue;
			const bool obuffer_ok = o.GetUnpackBuffer(
				tmp0, obuffer_size);
			if (!obuffer_ok)
			{
				delete [] tmp0;
				continue;
			}
			for (size_t j = 0; j < obuffer_size; ++j)
			{
				overlay.data.push_back(tmp0[j]);
			}
			delete [] tmp0;
			slice_overlays.insert(0, overlay);
		}
	}
	const QList<int> keys = slice_overlays.keys();
	for (int x = 0; x < keys.size(); ++x)
	{
		const int idx = keys.at(x);
		SliceOverlays l2 = slice_overlays.values(idx);
		if (!ref.image_overlays.all_overlays.contains(idx))
		{
			ref.image_overlays.all_overlays[idx] = l2;
		}
		else
		{
			for (int j = 0; j < l2.size(); ++j)
			{
				ref.image_overlays.all_overlays[idx]
					.push_back(l2[j]);
			}
		}
	}
	slice_overlays.clear();
}

void PrConfigUtils::read_graphic_objects(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	const mdcm::Tag tGraphicAnnotationSequence(0x0070,0x0001);
	const mdcm::Tag tReferencedImageSequence(0x0008,0x1140);
	const mdcm::Tag tReferencedSOPInstanceUID(0x0008,0x1155);
	const mdcm::Tag tReferencedFrameNumber(0x0008,0x1160);
	const mdcm::Tag tGraphicObjectSequence(0x0070,0x0009);
	const mdcm::Tag tGraphicAnnotationUnits(0x0070,0x0005);
	const mdcm::Tag tGraphicDimensions(0x0070,0x0020);//2
	const mdcm::Tag tNumberofGraphicPoints(0x0070,0x0021);
	const mdcm::Tag tGraphicData(0x0070,0x0022);//VR::FL VM::VM2_n
	//POINT POLYLINE INTERPOLATED CIRCLE ELLIPSE
	const mdcm::Tag tGraphicType(0x0070,0x0023);
	const mdcm::Tag tGraphicFilled(0x0070,0x0024);//Y,N
	const mdcm::Tag tCompoundGraphicInstanceID(0x0070,0x0226);
	const mdcm::Tag tGraphicGroupID(0x0070,0x0295);
	const mdcm::Tag tTrackingID(0x0062,0x0020);
	const mdcm::Tag tTrackingUID(0x0062,0x0021);
	//
	const mdcm::Tag tLineStyleSequence(0x0070,0x0232);
	const mdcm::Tag tPatternOnColorCIELabValue(0x0070,0x0251);
	const mdcm::Tag tPatternOffColorCIELabValue(0x0070,0x0252);
	const mdcm::Tag tPatternOnOpacity(0x0070,0x0284);
	const mdcm::Tag tPatternOffOpacity(0x0070,0x0285);
	const mdcm::Tag tLineThickness(0x0070,0x0253);
	//SOLID DASHED
	const mdcm::Tag tLineDashingStyle(0x0070,0x0254);
	//0x00ff defines the dashes with an equal size
	const mdcm::Tag tLinePattern(0x0070,0x0255);
	//NORMAL OUTLINED OFF
	const mdcm::Tag tShadowStyle(0x0070,0x0244);
	const mdcm::Tag tShadowOffsetX(0x0070,0x0245);
	const mdcm::Tag tShadowOffsetY(0x0070,0x0246);
	const mdcm::Tag tShadowColorCIELabValue(0x0070,0x0247);
	const mdcm::Tag tShadowOpacity(0x0070,0x0258);//0.0-1.0
	//
	const mdcm::Tag tFillStyleSequence(0x0070,0x0233);
	//SOLID STIPPELED
	const mdcm::Tag tFillMode(0x0070,0x0257);
	//A binary fill pattern. A set bit corresponds to
	//foreground. An unset bit corresponds to background.
	//A 128 byte value defining a 32x32 1 bit matrix.
	//This fill pattern is replicated in tiles inside the
	//boundaries of the graphic type. The most significant
	//bit corresponds to the leftmost pixel in the row.
	//The fill pattern relates to display pixels where
	//one bit value corresponds to one display pixel.
	const mdcm::Tag tFillPattern(0x0070,0x0256);
	//
	QString t00080005("");
	const bool t00080005_ok =
		DicomUtils::get_string_value(ds, mdcm::Tag(0x0008,0x0005), t00080005);
	(void)t00080005_ok;
	if (ds.FindDataElement(tGraphicAnnotationSequence))
	{
		const mdcm::DataElement & de =
			ds.GetDataElement(tGraphicAnnotationSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			de.GetValueAsSQ();
		if (!sq) return;
		const unsigned int number_of_items =
			sq->GetNumberOfItems();
		for (unsigned int x = 0; x < number_of_items; ++x)
		{
			const mdcm::Item & item = sq->GetItem(x+1);
			const mdcm::DataSet & nestedds =
				item.GetNestedDataSet();
			QStringList ref_sop;
			if (nestedds.FindDataElement(tReferencedImageSequence))
			{
				const mdcm::DataElement & de1 =
					nestedds.GetDataElement(tReferencedImageSequence);
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 =
					de1.GetValueAsSQ();
				if (!sq1) continue;
				const unsigned int number_of_items1 =
					sq1->GetNumberOfItems();
				for (unsigned int x1 = 0; x1 < number_of_items1; ++x1)
				{
					const mdcm::Item & item1 = sq1->GetItem(x1+1);
					const mdcm::DataSet & nestedds1 =
						item1.GetNestedDataSet();
					QString s1, s2;
					const bool s1_ok = DicomUtils::get_string_value(
						nestedds1,tReferencedSOPInstanceUID,s1);
					const bool s2_ok = DicomUtils::get_string_value(
						nestedds1,tReferencedFrameNumber,s2);
					ref_sop.push_back(s1);
					ref_sop.push_back(s2);
					(void)s1_ok;
					(void)s2_ok;
				}
			}
			if (nestedds.FindDataElement(tGraphicObjectSequence))
			{
				const mdcm::DataElement & de2 =
					nestedds.GetDataElement(tGraphicObjectSequence);
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq2 =
					de2.GetValueAsSQ();
				if (!sq2) continue;
				const unsigned int number_of_items2 =
					sq2->GetNumberOfItems();
				for (unsigned int x2 = 0; x2 < number_of_items2; ++x2)
				{
					PrConfig c;
					c.id = 6;
					c.desc = QString("Graphic Object");
					const mdcm::Item & item2 = sq2->GetItem(x2+1);
					const mdcm::DataSet & nestedds2 =
						item2.GetNestedDataSet();
					QString GraphicType;
					if (DicomUtils::get_string_value(
						nestedds2,
						tGraphicType,
						GraphicType))
					{
						GraphicType = GraphicType
							.trimmed()
							.toUpper()
							.remove(QChar('\0'));
					}
					std::vector<float> GraphicData;
					if (!(!GraphicType.isEmpty() &&
						(DicomUtils::get_fl_values(
							nestedds2,
							tGraphicData,
							GraphicData) && GraphicData.size()>1)))
					{
						continue;
					}
					c.values.push_back(QVariant(GraphicType)); // 0
					QString GraphicAnnotationUnits;
					if (DicomUtils::get_string_value(
						nestedds2,
						tGraphicAnnotationUnits,
						GraphicAnnotationUnits))
					{
						GraphicAnnotationUnits = GraphicAnnotationUnits
							.trimmed()
							.toUpper()
							.remove(QChar('\0'));
						c.values.push_back(QVariant(
							GraphicAnnotationUnits)); // 1
					}
					else
					{
						c.values.push_back(QVariant(QString(""))); // 1
					}
					{
						QList<QVariant> q;
						for (unsigned int x3 = 0;
							x3 < GraphicData.size();
							++x3)
						{
							q.push_back(QVariant(
								(double)GraphicData.at(x3)));
						}
						c.values.push_back(QVariant(q)); // 2
					}
					QString GraphicFilled;
					if (DicomUtils::get_string_value(
						nestedds2,
						tGraphicFilled,
						GraphicFilled))
					{
						GraphicFilled = GraphicFilled
							.trimmed()
							.toUpper()
							.remove(QChar('\0'));
						c.values.push_back(QVariant(
							GraphicFilled)); // 3
					}
					else
					{
						c.values.push_back(QVariant(QString(""))); // 3
					}
					//
					for (int y = 0; y < ref_sop.size(); ++y)
					{
						c.values.push_back(ref_sop.at(y)); // 4+
					}
					ref.prconfig.push_back(c);
				}
			}
		}
	}
}

void PrConfigUtils::read_text_annotations(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	const mdcm::Tag tReferencedImageSequence(0x0008,0x1140);
	const mdcm::Tag tReferencedSOPInstanceUID(0x0008,0x1155);
	const mdcm::Tag tReferencedFrameNumber(0x0008,0x1160);
	const mdcm::Tag tGraphicAnnotationSequence(0x0070,0x0001);
	const mdcm::Tag tGraphicLayer(0x0070,0x0002);
	const mdcm::Tag tTextObjectSequence(0x0070,0x0008);
	const mdcm::Tag tBoundingBoxAnnotationUnits(0x0070,0x0003);
	const mdcm::Tag tAnchorPointAnnotationUnits(0x0070,0x0004);
	const mdcm::Tag tUnformattedTextValue(0x0070,0x0006);
	const mdcm::Tag tBoundingBoxTopLeftHandCorner(0x0070,0x0010);
	const mdcm::Tag tBoundingBoxBottomRightHandCorner(0x0070,0x0011);
	const mdcm::Tag tBoundingBoxTextHorizontalJustification(0x0070,0x0012);
	const mdcm::Tag tAnchorPoint(0x0070,0x0014);
	const mdcm::Tag tAnchorPointVisibility(0x0070,0x0015);
	const mdcm::Tag tCompoundGraphicInstanceID(0x0070,0x0226);
	const mdcm::Tag tGraphicGroupID(0x0070,0x0295);
	const mdcm::Tag tTrackingID(0x0062,0x0020);
	const mdcm::Tag tTrackingUID(0x0062,0x0021);
	const mdcm::Tag tTextStyleSequence(0x0070,0x0231);
	const mdcm::Tag tFontName(0x0070,0x0227);
	const mdcm::Tag tFontNameType(0x0070,0x0228);
	const mdcm::Tag tCSSFontName(0x0070,0x0229);
	const mdcm::Tag tTextColorCIELabValue(0x0070,0x0241);
	// LEFT, CENTER, RIGHT
	const mdcm::Tag tHorizontalAlignment(0x0070,0x0242);
	// TOP, CENTER, BOTTOM
	const mdcm::Tag tVerticalAlignment(0x0070,0x0243);
	// NORMAL, OUTLINED, OFF
	const mdcm::Tag tShadowStyle(0x0070,0x0244);   
	const mdcm::Tag tShadowOffsetX(0x0070,0x0245);
	const mdcm::Tag tShadowOffsetY(0x0070,0x0246);
	const mdcm::Tag tShadowColorCIELabValue(0x0070,0x0247);
	const mdcm::Tag tShadowOpacity(0x0070,0x0258); // 0.0-1.0
	const mdcm::Tag tUnderlined(0x0070,0x0248);    // Y, N
	const mdcm::Tag tBold(0x0070,0x0249);          // Y, N
	const mdcm::Tag tItalic(0x0070,0x0250);        // Y, N
	QString t00080005("");
	const bool t00080005_ok =
		DicomUtils::get_string_value(ds, mdcm::Tag(0x0008,0x0005), t00080005);
	(void)t00080005_ok;
	if (ds.FindDataElement(tGraphicAnnotationSequence))
	{
		const mdcm::DataElement & de =
			ds.GetDataElement(tGraphicAnnotationSequence);
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			de.GetValueAsSQ();
		if (!sq) return;
		const unsigned int number_of_items =
			sq->GetNumberOfItems();
		for (unsigned int x = 0; x < number_of_items; ++x)
		{
			const mdcm::Item & item = sq->GetItem(x+1);
			const mdcm::DataSet & nestedds =
				item.GetNestedDataSet();
			QStringList ref_sop;
			if (nestedds.FindDataElement(tReferencedImageSequence))
			{
				const mdcm::DataElement & de1 =
					nestedds.GetDataElement(tReferencedImageSequence);
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq1 =
					de1.GetValueAsSQ();
				if (!sq1) continue;
				const unsigned int number_of_items1 =
					sq1->GetNumberOfItems();
				for (unsigned int x1 = 0; x1 < number_of_items1; ++x1)
				{
					const mdcm::Item & item1 = sq1->GetItem(x1+1);
					const mdcm::DataSet & nestedds1 =
						item1.GetNestedDataSet();
					QString s1, s2;
					const bool s1_ok = DicomUtils::get_string_value(
						nestedds1,tReferencedSOPInstanceUID,s1);
					const bool s2_ok = DicomUtils::get_string_value(
						nestedds1,tReferencedFrameNumber,s2);
					ref_sop.push_back(s1);
					ref_sop.push_back(s2);
					(void)s1_ok;
					(void)s2_ok;
				}
			}
			if (nestedds.FindDataElement(tTextObjectSequence))
			{
				const mdcm::DataElement & de2 =
					nestedds.GetDataElement(tTextObjectSequence);
				mdcm::SmartPointer<mdcm::SequenceOfItems> sq2 =
					de2.GetValueAsSQ();
				if (!sq2) continue;
				const unsigned int number_of_items2 =
					sq2->GetNumberOfItems();
				for (unsigned int x2 = 0; x2 < number_of_items2; ++x2)
				{
					PrConfig c;
					c.id = 5;
					c.desc = QString("Text Annotations");
					const mdcm::Item & item2 = sq2->GetItem(x2+1);
					const mdcm::DataSet & nestedds2 =
						item2.GetNestedDataSet();
					std::vector<float> AnchorPoint;
					std::vector<float> AreaTopLeftHandCorner;
					std::vector<float> BottomRightHandCorner;
					if ((DicomUtils::get_fl_values(
								nestedds2,
								tBoundingBoxTopLeftHandCorner,
								AreaTopLeftHandCorner) &&
									AreaTopLeftHandCorner.size() == 2) &&
						(DicomUtils::get_fl_values(
							nestedds2,
							tBoundingBoxBottomRightHandCorner,
							BottomRightHandCorner) &&
								BottomRightHandCorner.size() == 2))
					{
						c.values.push_back(QVariant(
							(int)1));                      // 0
						c.values.push_back(QVariant(
							AreaTopLeftHandCorner.at(0))); // 1
						c.values.push_back(QVariant(
							AreaTopLeftHandCorner.at(1))); // 2
						c.values.push_back(QVariant(
							BottomRightHandCorner.at(0))); // 3
						c.values.push_back(QVariant(
							BottomRightHandCorner.at(1))); // 4
						QString BoundingBoxAnnotationUnits;
						if (DicomUtils::get_string_value(
								nestedds2,
								tBoundingBoxAnnotationUnits,
								BoundingBoxAnnotationUnits))
						{
							BoundingBoxAnnotationUnits =
								BoundingBoxAnnotationUnits
									.trimmed()
									.toUpper()
									.remove(QChar('\0'));
							c.values.push_back(
								QVariant(
									BoundingBoxAnnotationUnits)); // 5
						}
						else
						{
							c.values.push_back(QVariant(
								QString(""))); // 5
						}
					}
					else
					{
						c.values.push_back(QVariant((int)0));      // 0
						c.values.push_back(QVariant((double)0));   // 1
						c.values.push_back(QVariant((double)0));   // 2
						c.values.push_back(QVariant((double)0));   // 3
						c.values.push_back(QVariant((double)0));   // 4
						c.values.push_back(QVariant(QString(""))); // 5
					}
					if (DicomUtils::get_fl_values(
								nestedds2,
								tAnchorPoint,
								AnchorPoint) &&
							AnchorPoint.size() == 2)
					{
						c.values.push_back(QVariant(
							(int)1));            // 6
						c.values.push_back(QVariant(
							AnchorPoint.at(0))); // 7
						c.values.push_back(QVariant(
							AnchorPoint.at(1))); // 8
						QString AnchorPointAnnotationUnits;
						if (DicomUtils::get_string_value(
								nestedds2,
								tAnchorPointAnnotationUnits,
								AnchorPointAnnotationUnits))
						{
							AnchorPointAnnotationUnits =
								AnchorPointAnnotationUnits
									.trimmed()
									.toUpper()
									.remove(QChar('\0'));
							c.values.push_back(
								QVariant(
									AnchorPointAnnotationUnits)); // 9
						}
						else
						{
							c.values.push_back(QVariant(
								QString(""))); // 9
						}
						QString AnchorPointVisibility;
						if (DicomUtils::get_string_value(
								nestedds2,
								tAnchorPointVisibility,
								AnchorPointVisibility))
						{
							AnchorPointVisibility =
								AnchorPointVisibility
									.trimmed()
									.toUpper()
									.remove(QChar('\0'));
							c.values.push_back(
								QVariant(AnchorPointVisibility)); // 10
						}
						else
						{
							c.values.push_back(QVariant(
								QString(""))); // 10
						}
					}
					else
					{
						c.values.push_back(QVariant((int)0));      // 6
						c.values.push_back(QVariant((double)0));   // 7
						c.values.push_back(QVariant((double)0));   // 8
						c.values.push_back(QVariant(QString(""))); // 9
						c.values.push_back(QVariant(QString(""))); //10 
					}
					if (nestedds2.FindDataElement(tUnformattedTextValue))
					{
						const mdcm::DataElement & e5 =
							nestedds2.GetDataElement(
								tUnformattedTextValue);
						if (
							!e5.IsEmpty() &&
							!e5.IsUndefinedLength() &&
							e5.GetByteValue())
						{
							QByteArray ba5(
								e5.GetByteValue()->GetPointer(),
								e5.GetByteValue()->GetLength());
							QString tmp5 =
								CodecUtils::toUTF8(
									&ba5,
									t00080005.toLatin1().constData());
							c.values.push_back(tmp5); // 11
						}
					}
					else
					{
						c.values.push_back(QVariant(QString(""))); // 11
					}
					if (nestedds2.FindDataElement(tTextStyleSequence))
					{
						const mdcm::DataElement & de3 =
							nestedds2.GetDataElement(tTextStyleSequence);
						mdcm::SmartPointer<mdcm::SequenceOfItems> sq3 =
							de3.GetValueAsSQ();
						if (sq3 && sq3->GetNumberOfItems() == 1)
						{
							c.values.push_back(QVariant((int)1)); // 12
							const mdcm::Item & item3 = sq3->GetItem(1);
							const mdcm::DataSet & nestedds3 =
								item3.GetNestedDataSet();
							QString FontName;
							if (DicomUtils::get_string_value(
									nestedds3,
									tFontName,
									FontName))
							{
								FontName =
									FontName
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(FontName)); // 13
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 13
							}
							QString FontNameType;
							if (DicomUtils::get_string_value(
									nestedds3,
									tFontNameType,
									FontNameType))
							{
								FontNameType =
									FontNameType
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(FontNameType)); // 14
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 14
							}
							QString CSSFontName;
							if (DicomUtils::get_string_value(
									nestedds3,
									tCSSFontName,
									CSSFontName))
							{
								CSSFontName =
									CSSFontName
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(CSSFontName)); // 15
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 15
							}
							QString HorizontalAlignment;
							if (DicomUtils::get_string_value(
									nestedds3,
									tHorizontalAlignment,
									HorizontalAlignment))
							{
								HorizontalAlignment =
									HorizontalAlignment
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(HorizontalAlignment)); // 16
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 16
							}
							QString VerticalAlignment;
							if (DicomUtils::get_string_value(
									nestedds3,
									tVerticalAlignment,
									VerticalAlignment))
							{
								VerticalAlignment =
									VerticalAlignment
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(VerticalAlignment)); // 17
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 17
							}
							QString ShadowStyle;
							if (DicomUtils::get_string_value(
									nestedds3,
									tShadowStyle,
									ShadowStyle))
							{
								ShadowStyle =
									ShadowStyle
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(ShadowStyle)); // 18
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 18
							}
							QString Underlined;
							if (DicomUtils::get_string_value(
									nestedds3,
									tUnderlined,
									Underlined))
							{
								Underlined =
									Underlined
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(Underlined)); // 19
							}
							else
							{
								c.values.push_back(
									QVariant(QString(""))); // 19
							}
							QString Bold;
							if (DicomUtils::get_string_value(
									nestedds3,
									tBold,
									Bold))
							{
								Bold =
									Bold
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(Bold)); // 20
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 20
							}
							QString Italic;
							if (DicomUtils::get_string_value(
									nestedds3,
									tItalic,
									Italic))
							{
								Italic =
									Italic
										.trimmed()
										.toUpper()
										.remove(QChar('\0'));
								c.values.push_back(
									QVariant(Italic)); // 21
							}
							else
							{
								c.values.push_back(QVariant(
									QString(""))); // 21
							}
							//
							// TODO
							//tShadowOffsetX
							//tShadowOffsetY
							//tShadowOpacity
							c.values.push_back(QVariant((double)0)); // 22
							c.values.push_back(QVariant((double)0)); // 23
							c.values.push_back(QVariant((double)1)); // 24
							//
							//
							std::vector<unsigned short>
								TextColorCIELabValue;
							if (DicomUtils::get_us_values(nestedds3,
									tTextColorCIELabValue,
									TextColorCIELabValue) &&
								TextColorCIELabValue.size()==3)
							{
								QList<QVariant> l789;
								l789.push_back(QVariant(
									(int)TextColorCIELabValue[0]));
								l789.push_back(QVariant(
									(int)TextColorCIELabValue[1]));
								l789.push_back(QVariant(
									(int)TextColorCIELabValue[2]));
								c.values.push_back(QVariant(l789)); // 25
							}
							else
							{
								c.values.push_back(QVariant()); // 25
							}
							std::vector<unsigned short>
								ShadowColorCIELabValue;
							if (DicomUtils::get_us_values(nestedds3,
									tShadowColorCIELabValue,
									ShadowColorCIELabValue) &&
								ShadowColorCIELabValue.size()==3)
							{
								QList<QVariant> l789;
								l789.push_back(QVariant(
									(int)ShadowColorCIELabValue[0]));
								l789.push_back(QVariant(
									(int)ShadowColorCIELabValue[1]));
								l789.push_back(QVariant(
									(int)ShadowColorCIELabValue[2]));
								c.values.push_back(QVariant(l789)); // 26
							}
							else
							{
								c.values.push_back(QVariant()); // 26
							}
						}
					}
					else
					{
						c.values.push_back(QVariant((int)0)); // 12
						c.values.push_back(QVariant(QString(""))); // 13
						c.values.push_back(QVariant(QString(""))); // 14
						c.values.push_back(QVariant(QString(""))); // 15
						c.values.push_back(QVariant(QString(""))); // 16
						c.values.push_back(QVariant(QString(""))); // 17
						c.values.push_back(QVariant(QString(""))); // 18
						c.values.push_back(QVariant(QString(""))); // 19
						c.values.push_back(QVariant(QString(""))); // 20
						c.values.push_back(QVariant(QString(""))); // 21
						c.values.push_back(QVariant((double)0)); // 22
						c.values.push_back(QVariant((double)0)); // 23
						c.values.push_back(QVariant((double)1)); // 24
						c.values.push_back(QVariant()); // 25
						c.values.push_back(QVariant()); // 26
					}
					for (int y = 0; y < ref_sop.size(); ++y)
					{
						c.values.push_back(ref_sop.at(y)); // 27+
					}
					ref.prconfig.push_back(c);
				}
			}
		}
	}
}

void PrConfigUtils::read_display_shutter(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	const mdcm::Tag tShutterShape(0x0018,0x1600);
	//
	// RECTANGULAR
	const mdcm::Tag tShutterLeftVerticalEdge(0x0018,0x1602);
	const mdcm::Tag tShutterRightVerticalEdge(0x0018,0x1604);
	const mdcm::Tag tShutterUpperHorizontalEdge(0x0018,0x1606);
	const mdcm::Tag tShutterLowerHorizontalEdge(0x0018,0x1608);
	// CIRCULAR
	const mdcm::Tag tCenterofCircularShutter(0x0018,0x1610);
	const mdcm::Tag tRadiusofCircularShutter(0x0018,0x1612);
	// POLYGONAL
	const mdcm::Tag tVerticesofthePolygonalShutter(0x0018,0x1620);
	//
	const mdcm::Tag tShutterPresentationValue(0x0018,0x1622);
	const mdcm::Tag tShutterPresentationColorCIELabValue(0x0018,0x1624);
	QString ShutterShape;
	if (!DicomUtils::get_string_value(
			ds, tShutterShape, ShutterShape))
	{
		return;
	}
	PrConfig c;
	c.id = 7;
	c.desc = QString("Display Shutter");
	ShutterShape = ShutterShape.trimmed().remove(QChar('\0'));
	c.values.push_back(QVariant(ShutterShape)); // 0
	int ShutterLeftVerticalEdge;
	int ShutterRightVerticalEdge;
	int ShutterUpperHorizontalEdge;
	int ShutterLowerHorizontalEdge;
	if (
		DicomUtils::get_is_value(
			ds,
			tShutterLeftVerticalEdge,
			&ShutterLeftVerticalEdge) &&
		DicomUtils::get_is_value(
			ds,
			tShutterRightVerticalEdge,
			&ShutterRightVerticalEdge) &&
		DicomUtils::get_is_value(
			ds,
			tShutterUpperHorizontalEdge,
			&ShutterUpperHorizontalEdge) &&
		DicomUtils::get_is_value(
			ds,
			tShutterLowerHorizontalEdge,
			&ShutterLowerHorizontalEdge))
	{
		c.values.push_back(
			QVariant((int)ShutterLeftVerticalEdge)); // 1
		c.values.push_back(
			QVariant((int)ShutterRightVerticalEdge)); // 2 
		c.values.push_back(
			QVariant((int)ShutterUpperHorizontalEdge)); // 3
			c.values.push_back(
		QVariant((int)ShutterLowerHorizontalEdge)); // 4
	}
	else
	{
		c.values.push_back(QVariant((int)-1)); // 1
		c.values.push_back(QVariant((int)-1)); // 2
		c.values.push_back(QVariant((int)-1)); // 3
		c.values.push_back(QVariant((int)-1)); // 4
	}
	std::vector<int> CenterofCircularShutter;
	int RadiusofCircularShutter;
	if (
		DicomUtils::get_is_values(
			ds,
			tCenterofCircularShutter,
			CenterofCircularShutter) &&
		(CenterofCircularShutter.size() == 2) &&
		DicomUtils::get_is_value(
			ds,
			tRadiusofCircularShutter,
			&RadiusofCircularShutter))
	{
		c.values.push_back(
			QVariant((int)CenterofCircularShutter.at(0))); // 5
		c.values.push_back(
			QVariant((int)CenterofCircularShutter.at(1))); // 6
		c.values.push_back(
			QVariant((int)RadiusofCircularShutter)); // 7
	}
	else
	{
		c.values.push_back(QVariant((int)-1)); // 5
		c.values.push_back(QVariant((int)-1)); // 6
		c.values.push_back(QVariant((int)-1)); // 7
	}
	std::vector<int> VerticesofthePolygonalShutter;
	if (DicomUtils::get_is_values(
			ds,
			tVerticesofthePolygonalShutter,
			VerticesofthePolygonalShutter) &&
		(VerticesofthePolygonalShutter.size() > 1))
	{
		QList<QVariant> l;
		for (unsigned int x = 0;
			x < VerticesofthePolygonalShutter.size();
			++x)
		{
			l.push_back(QVariant(
				(int)VerticesofthePolygonalShutter.at(x)));
		}
		c.values.push_back(QVariant(l)); // 8
	}
	else
	{
		c.values.push_back(QVariant()); // 8
	}
	unsigned short ShutterPresentationValue;
	if (DicomUtils::get_us_value(
			ds,
			tShutterPresentationValue,
			&ShutterPresentationValue))
	{
		c.values.push_back(QVariant(
			(int)ShutterPresentationValue)); // 9
	}
	else
	{
		c.values.push_back(QVariant((int)-1)); // 9
	}
	std::vector<unsigned short> ShutterPresentationColorCIELabValue;
	if (DicomUtils::get_us_values(
			ds,
			tShutterPresentationColorCIELabValue,
			ShutterPresentationColorCIELabValue) &&
		(ShutterPresentationColorCIELabValue.size()==3))
	{
		QList<QVariant> l;
		for (unsigned int x = 0; x < 3; ++x)
		{
			l.push_back(QVariant(
				(int)ShutterPresentationColorCIELabValue.at(x)));
		}
		c.values.push_back(QVariant(l)); // 10
	}
	else
	{
		c.values.push_back(QVariant()); // 10
	}
	ref.prconfig.push_back(c);
}

void PrConfigUtils::read_pr(
	const mdcm::DataSet & ds,
	PrRefSeries & ref)
{
	read_modality_lut(ds,ref);
	QApplication::processEvents();
	read_voi_lut(ds,ref);
	QApplication::processEvents();
	read_display_areas(ds,ref);
	QApplication::processEvents();
	read_graphic_layers(ds,ref);
	QApplication::processEvents();
	read_spatial_transformation(ds,ref);
	QApplication::processEvents();
	read_overlays(ds,ref);
	QApplication::processEvents();
	read_text_annotations(ds,ref);
	QApplication::processEvents();
	read_graphic_objects(ds,ref);
	QApplication::processEvents();
	read_display_shutter(ds,ref);
	QApplication::processEvents();
//
#if 0
	std::cout << "-------" << std::endl;
	for (int x = 0; x < ref.prconfig.size(); ++x)
	{
		std::cout << ref.prconfig.at(x).desc.toStdString();
#if 0
		std::cout << ": ";
		for (int z = 0; z < ref.prconfig.at(x).values.size(); ++z)
		{
			std::cout
				<< ref.prconfig.at(x).values.at(z)
					.toString().toStdString() << " | ";
		}
#endif
		std::cout << std::endl;
	}
	std::cout << "-------" << std::endl;
#endif
}

//#define PRINT_MAKE_PR_MONOCHROME
ImageVariant * PrConfigUtils::make_pr_monochrome(
	const ImageVariant * ivariant,
	const PrRefSeries & ref,
	const SettingsWidget * w,
	GLWidget * gl,
	bool ok3d,
	bool * spatial_transform)
{
	if (!ivariant) return NULL;
#ifdef PRINT_MAKE_PR_MONOCHROME
	std::cout << "+++++++++++++"<< std::endl;
#endif
	QString error("");
	double shift = 0.0, scale = 1.0;
	bool rescale_found = false;
	short rotation = 0;
	bool flip = false;
	const QList<PrConfig> l = ref.prconfig;
	ImageVariant * v = new ImageVariant(
		CommonUtils::get_next_id(),
		ok3d,
		!w->get_3d(),
		gl,
		0);
	v->di->filtering = w->get_filtering();
	//
	// Modality LUT
	//
	{
		for (int x = 0; x < l.size(); ++x)
		{
			QApplication::processEvents();
			if (l.at(x).id == 1 && l.at(x).values.size() >= 2)
			{
				shift = l.at(x).values.at(0).toDouble();
				scale = l.at(x).values.at(1).toDouble();
				rescale_found = true;
#ifdef PRINT_MAKE_PR_MONOCHROME
				std::cout << "Modality LUT (Intercept, Slope)"
					<< std::endl;
#endif
				break;
			}
		}
		if (!rescale_found)
		{
			shift = ivariant->di->shift_tmp;
			scale = ivariant->di->scale_tmp;
		}
#if 0
		{
			QMap<int, QString>::const_iterator it =
				ivariant->image_instance_uids.constBegin();
			std::cout << "Instance UIDs (dimZ="
				<< ivariant->di->idimz <<") :" << std::endl;
			while (it != ivariant->image_instance_uids.constEnd())
			{
				std::cout << " " << it.key() << " "
					<< it.value().toStdString() << std::endl;
				++it;
			}
		}
#endif
		if      (ivariant->image_type==0) error=intensity_filter2<ImageTypeSS> (ivariant->pSS, v->pF,shift,scale);
		else if (ivariant->image_type==1) error=intensity_filter2<ImageTypeUS> (ivariant->pUS, v->pF,shift,scale);
		else if (ivariant->image_type==2) error=intensity_filter2<ImageTypeSI> (ivariant->pSI, v->pF,shift,scale);
		else if (ivariant->image_type==3) error=intensity_filter2<ImageTypeUI> (ivariant->pUI, v->pF,shift,scale);
		else if (ivariant->image_type==4) error=intensity_filter2<ImageTypeUC> (ivariant->pUC, v->pF,shift,scale);
		else if (ivariant->image_type==5) error=intensity_filter2<ImageTypeF>  (ivariant->pF,  v->pF,shift,scale);
		else if (ivariant->image_type==6) error=intensity_filter2<ImageTypeD>  (ivariant->pD,  v->pF,shift,scale);
		else if (ivariant->image_type==7) error=intensity_filter2<ImageTypeSLL>(ivariant->pSLL,v->pF,shift,scale);
		else if (ivariant->image_type==8) error=intensity_filter2<ImageTypeULL>(ivariant->pULL,v->pF,shift,scale);
		else { error = QString("Wrong image type"); }
		if (error.isEmpty())
		{
			v->image_type = 5;
			v->di->shift_tmp = shift;
			v->di->scale_tmp = scale;
		}
		else
		{
			std::cout << error.toStdString() << std::endl;
			delete v;
			return NULL;
		}
	}
	//
	// VOI LUT
	//
	{
		QMap<int, QStringList> voi_lut_images;
		QMap<int, double>      window_centers;
		QMap<int, double>      window_widths;
		QMap<int, QString>     lut_functions;
		int voi_luts = 0;
		for (int x = 0; x < l.size(); ++x)
		{
			const int values_size = l.at(x).values.size();
			if (l.at(x).id == 2 &&  values_size >= 3)
			{
				window_centers[voi_luts] = l.at(x).values.at(0).toDouble();
				window_widths[voi_luts]  = l.at(x).values.at(1).toDouble();
				lut_functions[voi_luts]  = l.at(x).values.at(2).toString();
				if (values_size >= 4)
				{
					QStringList sl;
					for (int z = 3; z < values_size; ++z)
						sl.push_back(l.at(x).values.at(z).toString());
					if (sl.size() > 0) voi_lut_images[voi_luts] = sl;
				}
				++voi_luts;
			}
		}
		QApplication::processEvents();
		if (voi_luts > 0)
		{
			if (voi_lut_images.empty())
			{
#ifdef PRINT_MAKE_PR_MONOCHROME
					std::cout << "VOI LUT" << std::endl;
#endif
				if (
					lut_functions.contains(0) &&
					lut_functions.value(0).toUpper() == QString("SIGMOID"))
				{
					error =
						to_char_sigm<ImageTypeF>(
							v->pF,
							v->pUC,
							window_widths.value(0),
							window_centers.value(0));
				}
				else
				{
					error =
						to_char<ImageTypeF>(
							v->pF,
							v->pUC,
							window_widths.value(0),
							window_centers.value(0));
				}
			}
			else
			{
#ifdef PRINT_MAKE_PR_MONOCHROME
				std::cout << "VOI LUT slice by slice" << std::endl;
#endif
				error =
					levels_slice_by_slice<ImageTypeF,Image2DTypeF>(
						v->pF, v->pUC,
						window_widths, window_centers,
						lut_functions,
						voi_lut_images,
						ivariant->image_instance_uids);
			}
			if (error.isEmpty())
			{
				if (v->pF.IsNotNull()) v->pF->DisconnectPipeline();
				v->pF = NULL;
				v->image_type = 4;
				v->di->us_window_center =
					v->di->default_us_window_center = 128.0;
				v->di->us_window_width  =
					v->di->default_us_window_width  = 255.0;
				v->di->lut_function = 0;
				v->di->shift_tmp = 0.0;
				v->di->scale_tmp = 1.0;
			}
			else
			{
				std::cout << error.toStdString() << std::endl;
				delete v;
				return NULL;
			}
		}
		else
		{
			v->di->us_window_center = v->di->default_us_window_center =
				ivariant->di->us_window_center;
			v->di->us_window_width = v->di->default_us_window_width  =
				ivariant->di->us_window_width;
			v->di->lut_function =
				ivariant->di->lut_function;
		}
	}
	//
	// Display areas
	//
	{
		QApplication::processEvents();
		QMap<int, QStringList> area_images;
		QMap<int, int>         areasTLx;
		QMap<int, int>         areasTLy;
		QMap<int, int>         areasBRx;
		QMap<int, int>         areasBRy;
		QMap<int, double>      aspect_ratios0;
		QMap<int, double>      aspect_ratios1;
		QMap<int, double>      pixel_spacings0;
		QMap<int, double>      pixel_spacings1;
		QMap<int, QString>     size_modes;
		int areas = 0;
		for (int x = 0; x < l.size(); ++x)
		{
			const int values_size = l.at(x).values.size();
			if (l.at(x).id == 4 && values_size >= 11)
			{
				areasTLx[areas]        = l.at(x).values.at(0).toInt();
				areasTLy[areas]        = l.at(x).values.at(1).toInt();
				areasBRx[areas]        = l.at(x).values.at(2).toInt();
				areasBRy[areas]        = l.at(x).values.at(3).toInt();
				size_modes[areas]      = l.at(x).values.at(4).toString();
				aspect_ratios0[areas]  = l.at(x).values.at(5).toDouble();
				aspect_ratios1[areas]  = l.at(x).values.at(6).toDouble();
				pixel_spacings0[areas] = l.at(x).values.at(7).toDouble();
				pixel_spacings1[areas] = l.at(x).values.at(8).toDouble();
				if (values_size >= 12)
				{
					QStringList sl;
					for (int z = 11; z < values_size; ++z)
						sl.push_back(l.at(x).values.at(z).toString());
					if (sl.size() > 0) area_images[areas] = sl;
				}
				++areas;
			}
		}
		if (areas > 0)
		{
			if (area_images.empty())
			{
#ifdef PRINT_MAKE_PR_MONOCHROME
				std::cout << "Display Areas" << std::endl;
#endif
				const double px = pixel_spacings1.value(0);
				const double py = pixel_spacings0.value(0);
				if (!(
					px > 0.99999 && px < 1.00001 &&
					py > 0.99999 && py < 1.00001))
				{
					if      (v->image_type== 0) set_spacing<ImageTypeSS>   (v->pSS,    px,py);
					else if (v->image_type== 1) set_spacing<ImageTypeUS>   (v->pUS,    px,py);
					else if (v->image_type== 2) set_spacing<ImageTypeSI>   (v->pSI,    px,py);
					else if (v->image_type== 3) set_spacing<ImageTypeUI>   (v->pUI,    px,py);
					else if (v->image_type== 4) set_spacing<ImageTypeUC>   (v->pUC,    px,py);
					else if (v->image_type== 5) set_spacing<ImageTypeF>    (v->pF,     px,py);
					else if (v->image_type== 6) set_spacing<ImageTypeD>    (v->pD,     px,py);
					else if (v->image_type== 7) set_spacing<ImageTypeSLL>  (v->pSLL,   px,py);
					else if (v->image_type== 8) set_spacing<ImageTypeULL>  (v->pULL,   px,py);
					else if (v->image_type==10) set_spacing<RGBImageTypeSS>(v->pSS_rgb,px,py);
					else if (v->image_type==11) set_spacing<RGBImageTypeUS>(v->pUS_rgb,px,py);
					else if (v->image_type==12) set_spacing<RGBImageTypeSI>(v->pSI_rgb,px,py);
					else if (v->image_type==13) set_spacing<RGBImageTypeUI>(v->pUI_rgb,px,py);
					else if (v->image_type==14) set_spacing<RGBImageTypeUC>(v->pUC_rgb,px,py);
					else if (v->image_type==15) set_spacing<RGBImageTypeF> (v->pF_rgb, px,py);
					else if (v->image_type==16) set_spacing<RGBImageTypeD> (v->pD_rgb, px,py);
					else ;;
				}
				//
				// TODO Presentation Size Mode
				const double ax = aspect_ratios1.value(0);
				const double ay = aspect_ratios0.value(0);
				if (!(
					ax > 0.99999f && ax < 1.00001 &&
					ay > 0.99999f && ay < 1.00001))
				{
					if      (v->image_type== 0) set_asp_ratio<ImageTypeSS>   (v->pSS,    ax,ay);
					else if (v->image_type== 1) set_asp_ratio<ImageTypeUS>   (v->pUS,    ax,ay);
					else if (v->image_type== 2) set_asp_ratio<ImageTypeSI>   (v->pSI,    ax,ay);
					else if (v->image_type== 3) set_asp_ratio<ImageTypeUI>   (v->pUI,    ax,ay);
					else if (v->image_type== 4) set_asp_ratio<ImageTypeUC>   (v->pUC,    ax,ay);
					else if (v->image_type== 5) set_asp_ratio<ImageTypeF>    (v->pF,     ax,ay);
					else if (v->image_type== 6) set_asp_ratio<ImageTypeD>    (v->pD,     ax,ay);
					else if (v->image_type== 7) set_asp_ratio<ImageTypeSLL>  (v->pSLL,   ax,ay);
					else if (v->image_type== 8) set_asp_ratio<ImageTypeULL>  (v->pULL,   ax,ay);
					else if (v->image_type==10) set_asp_ratio<RGBImageTypeSS>(v->pSS_rgb,ax,ay);
					else if (v->image_type==11) set_asp_ratio<RGBImageTypeUS>(v->pUS_rgb,ax,ay);
					else if (v->image_type==12) set_asp_ratio<RGBImageTypeSI>(v->pSI_rgb,ax,ay);
					else if (v->image_type==13) set_asp_ratio<RGBImageTypeUI>(v->pUI_rgb,ax,ay);
					else if (v->image_type==14) set_asp_ratio<RGBImageTypeUC>(v->pUC_rgb,ax,ay);
					else if (v->image_type==15) set_asp_ratio<RGBImageTypeF> (v->pF_rgb, ax,ay);
					else if (v->image_type==16) set_asp_ratio<RGBImageTypeD> (v->pD_rgb, ax,ay);
					else ;;
				}
				if (!(
					areasTLx.value(0) == -1 &&
					areasTLy.value(0) == -1 &&
					areasBRx.value(0) == -1 &&
					areasBRy.value(0) == -1))
				{
					const int aTLx = areasTLx.value(0);
					const int aTLy = areasTLy.value(0);
					const int aBRx = areasBRx.value(0);
					const int aBRy = areasBRy.value(0);
					if      (v->image_type== 0) set_darea<ImageTypeSS>   (v, v->pSS,    aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 1) set_darea<ImageTypeUS>   (v, v->pUS,    aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 2) set_darea<ImageTypeSI>   (v, v->pSI,    aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 3) set_darea<ImageTypeUI>   (v, v->pUI,    aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 4) set_darea<ImageTypeUC>   (v, v->pUC,    aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 5) set_darea<ImageTypeF>    (v, v->pF,     aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 6) set_darea<ImageTypeD>    (v, v->pD,     aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 7) set_darea<ImageTypeSLL>  (v, v->pSLL,   aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type== 8) set_darea<ImageTypeULL>  (v, v->pULL,   aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type==10) set_darea<RGBImageTypeSS>(v, v->pSS_rgb,aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type==11) set_darea<RGBImageTypeUS>(v, v->pUS_rgb,aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type==12) set_darea<RGBImageTypeSI>(v, v->pSI_rgb,aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type==13) set_darea<RGBImageTypeUI>(v, v->pUI_rgb,aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type==14) set_darea<RGBImageTypeUC>(v, v->pUC_rgb,aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type==15) set_darea<RGBImageTypeF> (v, v->pF_rgb, aTLx, aTLy, aBRx, aBRy);
					else if (v->image_type==16) set_darea<RGBImageTypeD> (v, v->pD_rgb, aTLx, aTLy, aBRx, aBRy);
					else ;;
				}
			}
			else if (!area_images.empty())
			{
#ifdef PRINT_MAKE_PR_MONOCHROME
				std::cout << "Display Areas slice by slice, may skip some params" << std::endl;
#endif
				const double px = pixel_spacings1.value(0);
				const double py = pixel_spacings0.value(0);
				if (!(
					px > 0.99999 && px < 1.00001 &&
					py > 0.99999 && py < 1.00001))
				{
					bool one_spacing = true;
					QMap<int, double>::const_iterator it0 = pixel_spacings1.begin();
					while (it0 != pixel_spacings1.end())
					{
						const double tmp0 = it0.value();
						if (!(px > tmp0-0.00001 && px < tmp0+0.00001))
						{
							one_spacing = false;
							break;
						}
						++it0;
					}
					if (one_spacing)
					{
						QMap<int, double>::const_iterator it1 = pixel_spacings0.begin();
						while (it1 != pixel_spacings0.end())
						{
							const double tmp1 = it1.value();
							if (!(py > tmp1-0.00001 && py < tmp1+0.00001))
							{
								one_spacing = false;
								break;
							}
							++it1;
						}
					}
					if (one_spacing)
					{
						if      (v->image_type== 0) set_spacing<ImageTypeSS>   (v->pSS,    px,py);
						else if (v->image_type== 1) set_spacing<ImageTypeUS>   (v->pUS,    px,py);
						else if (v->image_type== 2) set_spacing<ImageTypeSI>   (v->pSI,    px,py);
						else if (v->image_type== 3) set_spacing<ImageTypeUI>   (v->pUI,    px,py);
						else if (v->image_type== 4) set_spacing<ImageTypeUC>   (v->pUC,    px,py);
						else if (v->image_type== 5) set_spacing<ImageTypeF>    (v->pF,     px,py);
						else if (v->image_type== 6) set_spacing<ImageTypeD>    (v->pD,     px,py);
						else if (v->image_type== 7) set_spacing<ImageTypeSLL>  (v->pSLL,   px,py);
						else if (v->image_type== 8) set_spacing<ImageTypeULL>  (v->pULL,   px,py);
						else if (v->image_type==10) set_spacing<RGBImageTypeSS>(v->pSS_rgb,px,py);
						else if (v->image_type==11) set_spacing<RGBImageTypeUS>(v->pUS_rgb,px,py);
						else if (v->image_type==12) set_spacing<RGBImageTypeSI>(v->pSI_rgb,px,py);
						else if (v->image_type==13) set_spacing<RGBImageTypeUI>(v->pUI_rgb,px,py);
						else if (v->image_type==14) set_spacing<RGBImageTypeUC>(v->pUC_rgb,px,py);
						else if (v->image_type==15) set_spacing<RGBImageTypeF> (v->pF_rgb, px,py);
						else if (v->image_type==16) set_spacing<RGBImageTypeD> (v->pD_rgb, px,py);
						else ;;
					}
				}
				//
				const double ax = aspect_ratios1.value(0);
				const double ay = aspect_ratios0.value(0);
				if (!(
					ax > 0.99999f && ax < 1.00001 &&
					ay > 0.99999f && ay < 1.00001))
				{
					bool one_aspect = true;
					QMap<int, double>::const_iterator it0 = aspect_ratios1.begin();
					while (it0 != aspect_ratios1.end())
					{
						const double tmp0 = it0.value();
						if (!(ax > tmp0-0.00001 && ax < tmp0+0.00001))
						{
							one_aspect = false;
							break;
						}
						++it0;
					}
					if (one_aspect)
					{
						QMap<int, double>::const_iterator it1 = aspect_ratios0.begin();
						while (it1 != aspect_ratios0.end())
						{
							const double tmp1 = it1.value();
							if (!(ay > tmp1-0.00001 && ay < tmp1+0.00001))
							{
								one_aspect = false;
								break;
							}
							++it1;
						}
					}
					if (one_aspect)
					{
						if      (v->image_type== 0) set_asp_ratio<ImageTypeSS>   (v->pSS,    ax,ay);
						else if (v->image_type== 1) set_asp_ratio<ImageTypeUS>   (v->pUS,    ax,ay);
						else if (v->image_type== 2) set_asp_ratio<ImageTypeSI>   (v->pSI,    ax,ay);
						else if (v->image_type== 3) set_asp_ratio<ImageTypeUI>   (v->pUI,    ax,ay);
						else if (v->image_type== 4) set_asp_ratio<ImageTypeUC>   (v->pUC,    ax,ay);
						else if (v->image_type== 5) set_asp_ratio<ImageTypeF>    (v->pF,     ax,ay);
						else if (v->image_type== 6) set_asp_ratio<ImageTypeD>    (v->pD,     ax,ay);
						else if (v->image_type== 7) set_asp_ratio<ImageTypeSLL>  (v->pSLL,   ax,ay);
						else if (v->image_type== 8) set_asp_ratio<ImageTypeULL>  (v->pULL,   ax,ay);
						else if (v->image_type==10) set_asp_ratio<RGBImageTypeSS>(v->pSS_rgb,ax,ay);
						else if (v->image_type==11) set_asp_ratio<RGBImageTypeUS>(v->pUS_rgb,ax,ay);
						else if (v->image_type==12) set_asp_ratio<RGBImageTypeSI>(v->pSI_rgb,ax,ay);
						else if (v->image_type==13) set_asp_ratio<RGBImageTypeUI>(v->pUI_rgb,ax,ay);
						else if (v->image_type==14) set_asp_ratio<RGBImageTypeUC>(v->pUC_rgb,ax,ay);
						else if (v->image_type==15) set_asp_ratio<RGBImageTypeF> (v->pF_rgb, ax,ay);
						else if (v->image_type==16) set_asp_ratio<RGBImageTypeD> (v->pD_rgb, ax,ay);
						else ;;
					}
				}
				//
				areas_slice_by_slice(
					v,
					areasTLx,
					areasTLy,
					areasBRx,
					areasBRy,
					area_images,
					ivariant->image_instance_uids);
			}
		}
	}
	//
	// Spatial transform
	//
	{
		QApplication::processEvents();
		for (int x = 0; x < l.size(); ++x)
		{
			if (l.at(x).id == 3 && l.at(x).values.size() == 2)
			{
				// Rotation
				const double d = (double)l.at(x).values.at(1).toInt();
				rotation = l.at(x).values.at(1).toInt();
				// Flip horiz.
				const QString f = l.at(x).values.at(0).toString()
					.trimmed()
					.remove(QChar('\0'));
				flip = (f == QString("Y"));
				if (d != 0.0 || flip)
				{
					*spatial_transform = true;
					if (v->image_type == 4)
					{
						ImageTypeUC::Pointer tmp0;
						error = rotate_flip_slice_by_slice<
							ImageTypeUC,Image2DTypeUC>(
								v->pUC, tmp0, v, d, (f == QString("Y")));
						if (error.isEmpty())
						{
							if (v->pUC.IsNotNull())
								v->pUC->DisconnectPipeline();
							v->pUC = tmp0;
						}
					}
					else if (v->image_type == 5)
					{
						ImageTypeF::Pointer tmp0;
						error = rotate_flip_slice_by_slice<
							ImageTypeF,Image2DTypeF>(
								v->pF, tmp0, v, d, (f == QString("Y")));
						if (error.isEmpty())
						{
							if (v->pF.IsNotNull())
								v->pF->DisconnectPipeline();
							v->pF = tmp0;
						}
					}
					else if (v->image_type == 14)
					{
						RGBImageTypeUC::Pointer tmp0;
						error = rotate_flip_slice_by_slice<
							RGBImageTypeUC,RGBImage2DTypeUC>(
								v->pUC_rgb, tmp0, v, d,
									(f == QString("Y")));
						if (error.isEmpty())
						{
							if (v->pUC_rgb.IsNotNull())
								v->pUC_rgb->DisconnectPipeline();
							v->pUC_rgb = tmp0;
						}
					}
					else
					{
						error = QString("Wrong image type");
					}
					if (!error.isEmpty())
					{
						std::cout << error.toStdString() << std::endl;
						delete v;
						return NULL;
					}
				}
				break;
			}
		}
	}
	CommonUtils::get_dimensions_(v);
	//
	// Text annotations
	//
	{
		QApplication::processEvents();
		QMap<int, QStringList> text_images;
		QMap<int, int>     has_bb;
		QMap<int, double>  bb_top_left_x;
		QMap<int, double>  bb_top_left_y;
		QMap<int, double>  bb_bottom_r_x;
		QMap<int, double>  bb_bottom_r_y;
		QMap<int, QString> bb_unit;
		QMap<int, int>     has_anchor;
		QMap<int, double>  anchor_x;
		QMap<int, double>  anchor_y;
		QMap<int, QString> a_unit;
		QMap<int, QString> a_visibility;
		QMap<int, QString> text;
		QMap<int, int>     has_textstyle;
		QMap<int, QString> FontName;
		QMap<int, QString> FontNameType;
		QMap<int, QString> CSSFontName;
		QMap<int, QString> HorizontalAlignment;
		QMap<int, QString> VerticalAlignment;
		QMap<int, QString> ShadowStyle;
		QMap<int, QString> Underlined;
		QMap<int, QString> Bold;
		QMap<int, QString> Italic;
		QMap<int, double>  ShadowOffsetX;
		QMap<int, double>  ShadowOffsetY;
		QMap<int, double>  ShadowOpacity;
		QMap<int, int>     TextColorCIELabValue_L;
		QMap<int, int>     TextColorCIELabValue_a;
		QMap<int, int>     TextColorCIELabValue_b;
		QMap<int, int>     ShadowColorCIELabValue_L;
		QMap<int, int>     ShadowColorCIELabValue_a;
		QMap<int, int>     ShadowColorCIELabValue_b;
		int texts = 0;
		for (int x = 0; x < l.size(); ++x)
		{
			const int values_size = l.at(x).values.size();
			if (l.at(x).id == 5 && values_size >= 26)
			{
				has_bb[texts]              = l.at(x).values.at( 0).toInt();
				bb_top_left_x[texts]       = l.at(x).values.at( 1).toDouble();
				bb_top_left_y[texts]       = l.at(x).values.at( 2).toDouble();
				bb_bottom_r_x[texts]       = l.at(x).values.at( 3).toDouble();
				bb_bottom_r_y[texts]       = l.at(x).values.at( 4).toDouble();
				bb_unit[texts]             = l.at(x).values.at( 5).toString();
				has_anchor[texts]          = l.at(x).values.at( 6).toInt();
				anchor_x[texts]            = l.at(x).values.at( 7).toDouble();
				anchor_y[texts]            = l.at(x).values.at( 8).toDouble();
				a_unit[texts]              = l.at(x).values.at( 9).toString();
				a_visibility[texts]        = l.at(x).values.at(10).toString();
				text[texts]                = l.at(x).values.at(11).toString();
				has_textstyle[texts]       = l.at(x).values.at(12).toInt();
				FontName[texts]            = l.at(x).values.at(13).toString();
				FontNameType[texts]        = l.at(x).values.at(14).toString();
				CSSFontName[texts]         = l.at(x).values.at(15).toString();
				HorizontalAlignment[texts] = l.at(x).values.at(16).toString();
				VerticalAlignment[texts]   = l.at(x).values.at(17).toString();
				ShadowStyle[texts]         = l.at(x).values.at(18).toString();
				Underlined[texts]          = l.at(x).values.at(19).toString();
				Bold[texts]                = l.at(x).values.at(20).toString();
				Italic[texts]              = l.at(x).values.at(21).toString();
				ShadowOffsetX[texts]       = l.at(x).values.at(22).toDouble();
				ShadowOffsetY[texts]       = l.at(x).values.at(23).toDouble();
				ShadowOpacity[texts]       = l.at(x).values.at(24).toDouble();
				{
					const QList<QVariant> & l453 =
						l.at(x).values.at(25).toList();
					if (l453.size()==3)
					{
						TextColorCIELabValue_L[texts] =
							l453.at(0).toInt();
						TextColorCIELabValue_a[texts] =
							l453.at(1).toInt();
						TextColorCIELabValue_b[texts] =
							l453.at(2).toInt();
					}
					else
					{
						TextColorCIELabValue_L[texts] = -1;
						TextColorCIELabValue_a[texts] = -1;
						TextColorCIELabValue_b[texts] = -1;
					}
				}
				{
					const QList<QVariant> & l453 =
						l.at(x).values.at(26).toList();
					if (l453.size()==3)
					{
						ShadowColorCIELabValue_L[texts] =
							l453.at(0).toInt();
						ShadowColorCIELabValue_a[texts] =
							l453.at(1).toInt();
						ShadowColorCIELabValue_b[texts] =
							l453.at(2).toInt();
					}
					else
					{
						ShadowColorCIELabValue_L[texts] = -1;
						ShadowColorCIELabValue_a[texts] = -1;
						ShadowColorCIELabValue_b[texts] = -1;
					}
				}
				if (values_size >= 28)
				{
					QStringList sl;
					for (int z = 27; z < values_size; ++z)
					{
						sl.push_back(l.at(x).values.at(z).toString());
					}
					if (!sl.empty()) text_images[texts] = sl;
				}
				++texts;
			}
		}
		if (texts > 0)
		{
			if (text_images.empty())
			{
				QList<PRTextAnnotation> tas;
				for (int y = 0; y < texts; ++y)
				{
					PRTextAnnotation a;
					a.has_bb = (has_bb.value(y) == 1) ? true : false;
					a.has_anchor = (has_anchor.value(y) == 1) ? true : false;
					a.has_textstyle = (has_textstyle.value(y) == 1) ? true : false;
					a.bb_top_left_x              = bb_top_left_x.value(y);
					a.bb_top_left_y              = bb_top_left_y.value(y);
					a.bb_bottom_right_x          = bb_bottom_r_x.value(y);
					a.bb_bottom_right_y          = bb_bottom_r_y.value(y);
					a.BoundingBoxAnnotationUnits = bb_unit.value(y);
					a.anchor_x                   = anchor_x.value(y);
					a.anchor_y                   = anchor_y.value(y);
					a.AnchorPointAnnotationUnits = a_unit.value(y);
					a.AnchorPointVisibility      = a_visibility.value(y);
					a.UnformattedTextValue       = text.value(y);
					a.FontName                   = FontName.value(y);
					a.FontNameType               = FontNameType.value(y);
					a.CSSFontName                = CSSFontName.value(y);
					a.HorizontalAlignment        =
						HorizontalAlignment.value(y);
					a.VerticalAlignment          =
						VerticalAlignment.value(y);
					a.ShadowStyle                = ShadowStyle.value(y);
					a.Underlined                 = Underlined.value(y);
					a.Bold                       = Bold.value(y);
					a.Italic                     = Italic.value(y);
					a.ShadowOffsetX              = ShadowOffsetX.value(y);
					a.ShadowOffsetY              = ShadowOffsetY.value(y);
					a.ShadowOpacity              = ShadowOpacity.value(y);
					a.TextColorCIELabValue_L     =
						TextColorCIELabValue_L.value(y);
					a.TextColorCIELabValue_a     =
						TextColorCIELabValue_a.value(y);
					a.TextColorCIELabValue_b     =
						TextColorCIELabValue_b.value(y);
					a.ShadowColorCIELabValue_L   =
						ShadowColorCIELabValue_L.value(y);
					a.ShadowColorCIELabValue_a   =
						ShadowColorCIELabValue_a.value(y);
					a.ShadowColorCIELabValue_b   =
						ShadowColorCIELabValue_b.value(y);
					tas.push_back(a);
				}
				v->pr_text_annotations.insert(-1, tas);
			}
			else
			{
				text_slice_by_slice(
					v,
					has_bb,
					bb_top_left_x,
					bb_top_left_y,
					bb_bottom_r_x,
					bb_bottom_r_y,
					bb_unit,
					has_anchor,
					anchor_x,
					anchor_y,
					a_unit,
					a_visibility,
					text,
					has_textstyle,
					FontName,
					FontNameType,
					CSSFontName,
					HorizontalAlignment,
					VerticalAlignment,
					ShadowStyle,
					Underlined,
					Bold,
					Italic,
					ShadowOffsetX,
					ShadowOffsetY,
					ShadowOpacity,
					TextColorCIELabValue_L,
					TextColorCIELabValue_a,
					TextColorCIELabValue_b,
					ShadowColorCIELabValue_L,
					ShadowColorCIELabValue_a,
					ShadowColorCIELabValue_b,
					text_images,
					ivariant->image_instance_uids);
			}
#ifdef PRINT_MAKE_PR_MONOCHROME
			std::cout << "Text annotations" << std::endl;
			{
				PRTextAnnotations::const_iterator it8 =
					v->pr_text_annotations.begin();
				while (it8 != v->pr_text_annotations.end())
				{
					const QList< PRTextAnnotation > & lll = it8.value();
					for (int y = 0; y < lll.size(); ++y)
					{
						std::cout << it8.key() << " "
							<< lll.at(y).UnformattedTextValue.toStdString()
							<< std::endl;
					}
					++it8;
				}
			}
#endif
		}
	}
	//
	// Graphic annotations
	//
	{
		QApplication::processEvents();
		QMap<int, QStringList> graphic_images;
		QMap<int, QString>  GraphicType;
		QMap<int, QString>  GraphicAnnotationUnits;
		QMap<int, QVariant> gdata; 
		////QMap<int, unsigned int> NumberofGraphicPoints;
		//QMap<int, int> LinePatternOnColorCIELabValue_L;
		//QMap<int, int> LinePatternOnColorCIELabValue_a;
		//QMap<int, int> LinePatternOnColorCIELabValue_b;
		//QMap<int, int> LinePatternOffColorCIELabValue_L;
		//QMap<int, int> LinePatternOffColorCIELabValue_a;
		//QMap<int, int> LinePatternOffColorCIELabValue_b;
		//QMap<int, double> LinePatternOnOpacity;
		//QMap<int, double> LinePatternOffOpacity;
		//QMap<int, double> LineThickness;
		//QMap<int, unsigned int> LinePattern;
		//QMap<int, double> ShadowOffsetX;
		//QMap<int, double> ShadowOffsetY;
		//QMap<int, int> ShadowColorCIELabValue_L;
		//QMap<int, int> ShadowColorCIELabValue_a;
		//QMap<int, int> ShadowColorCIELabValue_b;
		//QMap<int, double> ShadowOpacity;
		//QMap<int, int> FillPatternOnColorCIELabValue_L;
		//QMap<int, int> FillPatternOnColorCIELabValue_a;
		//QMap<int, int> FillPatternOnColorCIELabValue_b;
		//QMap<int, int> FillPatternOffColorCIELabValue_L;
		//QMap<int, int> FillPatternOffColorCIELabValue_a;
		//QMap<int, int> FillPatternOffColorCIELabValue_b;
		//QMap<int, double> FillPatternOnOpacity;
		//QMap<int, double> FillPatternOffOpacity;
		//QMap<int, int> CompoundGraphicInstanceID;
		//QMap<int, int> GraphicGroupID;
		//QMap<int, QString> LineDashingStyle;
		//QMap<int, QString> ShadowStyle;
		QMap<int, QString> GraphicFilled;
		//QMap<int, QString> FillMode;
		//QMap<int, QString> TrackingID;
		//QMap<int, QString> TrackingUID;
		////std::vector<unsigned char> FillPattern;

		int graphics = 0;
		for (int x = 0; x < l.size(); ++x)
		{
			const int values_size = l.at(x).values.size();
			if (l.at(x).id == 6 && values_size >= 4)
			{
				GraphicType[graphics] = l.at(x).values.at(0).toString();
				GraphicAnnotationUnits[graphics] = l.at(x).values.at(1).toString();
				gdata[graphics] = QVariant(l.at(x).values.at(2).toList());
				GraphicFilled[graphics] = l.at(x).values.at(3).toString();
				if (values_size >= 5)
				{
					QStringList sl;
					for (int z = 4; z < values_size; ++z)
					{
						sl.push_back(l.at(x).values.at(z).toString());
					}
					if (!sl.empty()) graphic_images[graphics] = sl;
				}
				++graphics;
			}
		}
		if (graphics > 0)
		{
			if (graphic_images.empty())
			{
				QList<PRGraphicObject> gs;
				for (int y = 0; y < graphics; ++y)
				{
					PRGraphicObject a;
					a.GraphicType = GraphicType.value(y);
					a.GraphicAnnotationUnits =
						GraphicAnnotationUnits.value(y);
					a.GraphicData.clear();
					const QList<QVariant> & l465 =
						gdata.value(y).toList();
					for (int z = 0; z < l465.size(); ++z)
					{
						a.GraphicData.push_back(
							static_cast<float>(l465.at(z).toDouble()));
					}
					a.GraphicFilled = GraphicFilled.value(y);
					gs.push_back(a);
				}
				v->pr_graphicobjects.insert(-1, gs);
			}
			else
			{
				graphic_slice_by_slice(
					v,
					GraphicType,
					GraphicAnnotationUnits,
					gdata,
					GraphicFilled,
					graphic_images,
					ivariant->image_instance_uids);
			}
#ifdef PRINT_MAKE_PR_MONOCHROME
			std::cout << "Graphic" << std::endl;
			{
				PRGraphicObjects::const_iterator it8 =
					v->pr_graphicobjects.begin();
				while (it8 != v->pr_graphicobjects.end())
				{
					const QList< PRGraphicObject > & lll = it8.value();
					for (int y = 0; y < lll.size(); ++y)
					{
						std::cout << it8.key() << " "
							<< lll.at(y).GraphicType.toStdString()
							<< std::endl;
					}
					++it8;
				}
			}
#endif
		}
	}
	//
	// Display shutter
	//
	{
		QApplication::processEvents();
		for (int x = 0; x < l.size(); ++x)
		{
			const int values_size = l.at(x).values.size();
			if (l.at(x).id == 7 && values_size == 11)
			{
				PRDisplayShutter a;
				a.ShutterShape               = l.at(x).values.at(0).toString();
				a.ShutterLeftVerticalEdge    = l.at(x).values.at(1).toInt();
				a.ShutterRightVerticalEdge   = l.at(x).values.at(2).toInt();
				a.ShutterUpperHorizontalEdge = l.at(x).values.at(3).toInt();
				a.ShutterLowerHorizontalEdge = l.at(x).values.at(4).toInt();
				a.CenterofCircularShutter_y  = l.at(x).values.at(5).toInt();
				a.CenterofCircularShutter_x  = l.at(x).values.at(6).toInt();
				a.RadiusofCircularShutter    = l.at(x).values.at(7).toInt();
				{
					const QList<QVariant> & q = l.at(x).values.at(8).toList();
					if (!q.empty() && (q.size()%2 == 0))
					{
						for (int j = 0; j < q.size(); ++j)
						{
							a.VerticesofthePolygonalShutter.push_back(
								q.at(j).toInt());
						}
					}
				}
				a.ShutterPresentationValue = l.at(x).values.at(9).toInt();
				{
					const QList<QVariant> & q = l.at(x).values.at(10).toList();
					if (q.size() == 3)
					{
						a.ShutterPresentationColorCIELabValue_L =
							q.at(0).toInt();
						a.ShutterPresentationColorCIELabValue_a =
							q.at(1).toInt();
						a.ShutterPresentationColorCIELabValue_b =
							q.at(2).toInt();
					}
				}
				v->pr_display_shutters.insert(-1, a);
				break;
			}
		}
#ifdef PRINT_MAKE_PR_MONOCHROME
		std::cout << "Display shutter" << std::endl;
		{
			PRDisplayShutters::const_iterator it8 =
				v->pr_display_shutters.begin();
			while (it8 != v->pr_display_shutters.end())
			{
				const PRDisplayShutter & dsh = it8.value();
				std::cout << it8.key() << " "
					<< dsh.ShutterShape.toStdString()
					<< std::endl;
				++it8;
			}
		}
#endif
	}
	//
	//
	//
	// Apply spatial tranform to pixel items
	if (rotation != 0 || flip)
	{
		rotate_flip_points(v, (double)rotation, flip);
	}
	else
	{
		//
		// Overlays
		//
		QApplication::processEvents();
		QMap<int,SliceOverlays>::const_iterator it3 =
			ref.image_overlays.all_overlays.begin();
		while (it3 != ref.image_overlays.all_overlays.end())
		{
			const int key = it3.key();
			const SliceOverlays & overlays = it3.value();
			++it3;
			for (int x = 0; x < overlays.size(); ++x)
			{
				SliceOverlay o = overlays.at(x);
				if (!v->image_overlays.all_overlays.contains(key))
				{
					v->image_overlays.all_overlays[key] =
						SliceOverlays();
				}
				v->image_overlays.all_overlays[key].push_back(o);
			}
		}
	}
	//
	//
	//
#ifdef PRINT_MAKE_PR_MONOCHROME
	std::cout << "+++++++++++++"<< std::endl;
#endif
	return v;
}
#ifdef PRINT_MAKE_PR_MONOCHROME
#undef PRINT_MAKE_PR_MONOCHROME
#endif

ImageVariant * PrConfigUtils::make_pr_rgb(
	const ImageVariant * ivariant,
	const PrRefSeries & ref,
	const SettingsWidget * w)
{
	// TODO
	return NULL;
}

ImageVariant * PrConfigUtils::make_levels_monochrome(
	const ImageVariant * ivariant,
	const PrRefSeries & ref,
	const SettingsWidget * w,
	GLWidget * gl,
	bool ok3d)
{
	// TODO
	return NULL;
}
