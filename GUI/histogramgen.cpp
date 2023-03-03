#include "structures.h"
#include "histogramgen.h"
#include <itkImage.h>
#include <itkScalarImageToHistogramGenerator.h>
#include <itkImageToHistogramFilter.h>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QPalette>
#include "updateqtcommand.h"

namespace
{

template<typename T> QString calculate_histogramm(
	const typename T::Pointer & image,
	ImageVariant * v)
{
	if (image.IsNull() || !v)
	{
		return QString("image.IsNull() || !v");
	}
	typedef itk::Statistics::ScalarImageToHistogramGenerator<T>
		HistogramGeneratorType;
	typedef typename HistogramGeneratorType::HistogramType
		HistogramType;
	typename HistogramGeneratorType::Pointer histogram_generator =
		HistogramGeneratorType::New();
	QString error__("");
	const double range = round(v->di->rmax - v->di->rmin);
	if (range < 0.0)
	{
		return QString("range < 0");
	}
	unsigned int bins_size = static_cast<unsigned int>(range) + 1;
	if (bins_size > 2048)
	{
		bins_size = 2048;
	}
	else if (bins_size < 256 && (v->image_type == 5 || v->image_type == 6))
	{
		bins_size = 256;
	}
	//
	int * bins;
	try
	{
		bins = new int[bins_size];
	}
	catch (const std::bad_alloc &)
	{
		return QString("std::bad_alloc");
	}
	//
	typename UpdateQtCommand::Pointer update_qt_command =
		UpdateQtCommand::New();
	try
	{
		histogram_generator->SetInput(image);
		histogram_generator->SetNumberOfBins(bins_size);
		histogram_generator->SetAutoHistogramMinimumMaximum(false);
		histogram_generator->SetHistogramMax(v->di->rmax);
		histogram_generator->SetHistogramMin(v->di->rmin);
		histogram_generator->AddObserver(
			itk::ProgressEvent(), update_qt_command);
		histogram_generator->Compute();
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	//
	int tmp0 = 1;
	const HistogramType * h = histogram_generator->GetOutput();
	for (unsigned int x = 0; x < bins_size; ++x)
	{
		bins[x] = h->GetFrequency(x, 0);
		if (bins[x] > tmp0) tmp0 = bins[x];
	}
	const double tmp2 = tmp0 > 2 ? log(static_cast<double>(tmp0)) : 0.30102;
	//
	{
		QPixmap pixmap;
		QPainter painter;
		QPainterPath p;
		QColor fgcolor = qApp->palette().color(QPalette::Highlight);
		QColor bgcolor = qApp->palette().color(QPalette::Window);
		QBrush brush(Qt::SolidPattern); brush.setColor(fgcolor);
		QPen pen; pen.setColor(fgcolor);
		pen.setWidth(3);
		const unsigned int pixmap_w = bins_size < 2048 ? 2048 : bins_size;
		const unsigned int pixmap_h = 1024;
		pixmap = QPixmap(pixmap_w, pixmap_h);
		if (pixmap.isNull())
		{
			delete [] bins;
			return QString("pixmap.isNull()");
		}
		pixmap.fill(bgcolor);
		//
		painter.begin(&pixmap);
		painter.setPen(pen);
		painter.setBrush(brush);
		double first_x = 0.0;
		double last_x  = 0.0;
		for (unsigned int x = 0; x < bins_size; ++x)
		{
			const double x_ = pixmap_w * x / static_cast<double>(bins_size);
			const double y_ = (bins[x] > 0)
				? pixmap_h * log(static_cast<double>(bins[x])) / tmp2
				: 0.0;
			if (x == 0)
			{
				first_x = x_;
				p.moveTo(x_, pixmap_h);
				p.lineTo(x_, pixmap_h - y_);
			}
			else if (x == bins_size - 1)
			{
				last_x = x_;
				p.lineTo(x_, pixmap_h - y_);
			}
			else p.lineTo(x_, pixmap_h - y_);
		}
		delete [] bins;
		p.lineTo(last_x, pixmap_h);
		p.lineTo(first_x, pixmap_h);
		painter.drawPath(p);
		painter.end();
		//
		v->histogram = pixmap;
	}
	//
	return QString("");
}

template<typename T> QString calculate_histogramm_rgb(
	const typename T::Pointer & image,
	ImageVariant * v)
{
	typedef itk::Statistics::ImageToHistogramFilter<T>
		HistogramFilter;
	typedef typename HistogramFilter::HistogramSizeType
		HistogramSize;
	typedef typename HistogramFilter::HistogramMeasurementVectorType
		HistogramMeasurementVector;
	typedef typename HistogramFilter::HistogramType HistogramType;
	typename HistogramFilter::Pointer filter = HistogramFilter::New();
	HistogramSize size(3);
	HistogramMeasurementVector bin_min(3);
	HistogramMeasurementVector bin_max(3);
	//
	const unsigned int bins_size = 256;
	size[0] = size[1] = size[2] = bins_size;
	bin_min[0] = bin_min[1] = bin_min[2] = 0;
	bin_max[0] = bin_max[1] = bin_max[2] = 255;
	//
	UpdateQtCommand::Pointer update_qt_command = UpdateQtCommand::New();
	//
	try
	{
		filter->SetHistogramSize(size);
		filter->SetHistogramBinMinimum(bin_min);
		filter->SetHistogramBinMaximum(bin_max);
		filter->SetInput(image);
		filter->AddObserver(itk::ProgressEvent(), update_qt_command);
		filter->Update();
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	//
	const HistogramType * histogram = filter->GetOutput();
	if (bins_size > histogram->GetSize()[0])
	{
		return QString("bins_size > histogram->GetSize()[0]");
	}
	if (bins_size > histogram->GetSize()[1])
	{
		return QString("bins_size > histogram->GetSize()[1]");
	}
	if (bins_size > histogram->GetSize()[2])
	{
		return QString("bins_size > histogram->GetSize()[2]");
	}
	//
	int * bins0;
	int * bins1;
	int * bins2;
	try
	{
		bins0 = new int[bins_size];
		bins1 = new int[bins_size];
		bins2 = new int[bins_size];
	}
	catch (const std::bad_alloc &)
	{
		return QString("std::bad_alloc");
	}
	//
	int tmp0 = 1;
	for (unsigned int x = 0; x < bins_size; ++x)
	{
		bins0[x] = histogram->GetFrequency(x, 0);
		if (bins0[x] > tmp0) tmp0 = bins0[x];
		bins1[x] = histogram->GetFrequency(x, 1);
		if (bins1[x] > tmp0) tmp0 = bins1[x];
		bins2[x] = histogram->GetFrequency(x, 2);
		if (bins2[x] > tmp0) tmp0 = bins2[x];
	}
	const double tmp2 = tmp0 > 2 ? log(static_cast<double>(tmp0)) : 0.30102;
	//
	{
		QPixmap pixmap;
		QPainter painter;
		QPainterPath p0, p1, p2;
		QPen pen0;
		QPen pen1;
		QPen pen2;
		//
		QColor bgcolor = qApp->palette().color(QPalette::Window);
		const unsigned long pixmap_w = (bins_size * 3 < 2048)
			? 2048
			: bins_size * 3; // curr. always 256 bins
		const unsigned long pixmap_h = 1024;
		pixmap = QPixmap(pixmap_w, pixmap_h);
		if (pixmap.isNull())
		{
			delete [] bins0;
			delete [] bins1;
			delete [] bins2;
			return QString("pixmap.isNull()");
		}
		pixmap.fill(bgcolor);
		//
		const double tmp100 = pixmap_w / (static_cast<double>(bins_size) * 3.0);
		const double tmp101 = 2.0 * tmp100;
		const double tmp102 = 3.0 * tmp100;
		const double tmp103 = 0.8 * tmp100;
		for (unsigned int x = 0; x < bins_size; ++x)
		{
			const double x_  = tmp102 * x;
			const double y0_ = (bins0[x] > 0) ? pixmap_h * log(static_cast<double>(bins0[x])) / tmp2 : 0.0;
			const double y1_ = (bins1[x] > 0) ? pixmap_h * log(static_cast<double>(bins1[x])) / tmp2 : 0.0;
			const double y2_ = (bins2[x] > 0) ? pixmap_h * log(static_cast<double>(bins2[x])) / tmp2 : 0.0;
			p0.moveTo(x_,          pixmap_h);
			p0.lineTo(x_,          pixmap_h - y0_);
			p1.moveTo(x_ + tmp100, pixmap_h);
			p1.lineTo(x_ + tmp100, pixmap_h - y1_);
			p2.moveTo(x_ + tmp101, pixmap_h);
			p2.lineTo(x_ + tmp101, pixmap_h - y2_);
		}
		delete [] bins0;
		delete [] bins1;
		delete [] bins2;
		pen0.setWidth(tmp103); pen0.setColor(Qt::red);
		pen1.setWidth(tmp103); pen1.setColor(Qt::green);
		pen2.setWidth(tmp103); pen2.setColor(Qt::blue);
		//
		painter.begin(&pixmap);
		painter.setPen(pen0);
		painter.drawPath(p0);
		painter.setPen(pen1);
		painter.drawPath(p1);
		painter.setPen(pen2);
		painter.drawPath(p2);
		painter.end();
		//
		v->histogram = pixmap;
	}
	//
	return QString("");
}

}

void HistogramGen::run()
{
	QString error__;
	if (!ivariant)
	{
		error = QString("!ivariant");
		return;
	}
	switch (ivariant->image_type)
	{
	case  0:
		error = calculate_histogramm<ImageTypeSS>(ivariant->pSS, ivariant);
		break;
	case  1:
		error = calculate_histogramm<ImageTypeUS>(ivariant->pUS, ivariant);
		break;
	case  2:
		error = calculate_histogramm<ImageTypeSI>(ivariant->pSI, ivariant);
		break;
	case  3:
		error = calculate_histogramm<ImageTypeUI>(ivariant->pUI, ivariant);
		break;
	case  4:
		error = calculate_histogramm<ImageTypeUC>(ivariant->pUC, ivariant);
		break;
	case  5:
		error = calculate_histogramm<ImageTypeF>(ivariant->pF, ivariant);
		break;
	case  6:
		error = calculate_histogramm<ImageTypeD>(ivariant->pD, ivariant);
		break;
	case  7:
		error = calculate_histogramm<ImageTypeSLL>(ivariant->pSLL, ivariant);
		break;
	case  8:
		error = calculate_histogramm<ImageTypeULL>(ivariant->pULL, ivariant);
		break;
	case 10:
		error = calculate_histogramm_rgb<RGBImageTypeSS>(ivariant->pSS_rgb, ivariant);
		break;
	case 11:
		error = calculate_histogramm_rgb<RGBImageTypeUS>(ivariant->pUS_rgb, ivariant);
		break;
	case 12:
		error = calculate_histogramm_rgb<RGBImageTypeSI>(ivariant->pSI_rgb, ivariant);
		break;
	case 13:
		error = calculate_histogramm_rgb<RGBImageTypeUI>(ivariant->pUI_rgb, ivariant);
		break;
	case 14:
		error = calculate_histogramm_rgb<RGBImageTypeUC>(ivariant->pUC_rgb, ivariant);
		break;
	case 15:
		error = calculate_histogramm_rgb<RGBImageTypeF>(ivariant->pF_rgb, ivariant);
		break;
	case 16:
		error = calculate_histogramm_rgb<RGBImageTypeD>(ivariant->pD_rgb, ivariant);
		break;
	default:
		break;
	}
}

QString HistogramGen::get_error() const
{
	return error;
}

