#include "studygraphicswidget.h"
#include "studyviewwidget.h"
#include <QtGlobal>
#include <QVBoxLayout>
#include <QImage>
#include <QGraphicsPixmapItem>
#include <itkExtractImageFilter.h>
#include <itkRegionOfInterestImageFilter.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include "processimagethreadLUT.hxx"
#include "graphicsutils.h"
#include "commonutils.h"
#include "updateqtcommand.h"
#include <limits>
#ifndef WIN32
#include <unistd.h>
#endif

namespace
{

void gImageCleanupHandler2(void * info)
{
	if (!info) return;
	unsigned char * p = static_cast<unsigned char*>(info);
	delete [] p;
	info = NULL;
}

template<typename Tin, typename Tout> QString get_slice2_(
	const typename Tin::Pointer & image,
	ImageVariant2D * v2d,
	typename Tout::Pointer & out_image,
	int idx)
{
	if (image.IsNull())
	{
		return QString("get_slice2_<>() : image.IsNull()");
	}
	typedef itk::ExtractImageFilter<Tin, Tout> FilterType;
	const typename Tin::RegionType inRegion =
		image->GetLargestPossibleRegion();
	const typename Tin::SizeType size = inRegion.GetSize();
	typename Tin::IndexType index = inRegion.GetIndex();
	typename Tin::RegionType outRegion;
	typename Tin::SizeType out_size;
	typename FilterType::Pointer filter = FilterType::New();
	index[2] = idx;
	out_size[0] = size[0];
	out_size[1] = size[1];
	out_size[2] = 0;
	outRegion.SetSize(out_size);
	outRegion.SetIndex(index);
	try
	{
		filter->SetInput(image);
		filter->SetExtractionRegion(outRegion);
		filter->SetDirectionCollapseToIdentity();
		filter->Update();
	}
	catch (const itk::ExceptionObject & ex)
	{
		return QString(ex.GetDescription());
	}
	out_image = filter->GetOutput();
	if (out_image.IsNotNull())
	{
		out_image->DisconnectPipeline();
	}
	else
	{
		return QString("Out image is NULL");
	}
	if (v2d)
	{
		v2d->idimx = out_image->GetLargestPossibleRegion().GetSize()[0];
		v2d->idimy = out_image->GetLargestPossibleRegion().GetSize()[1];
	}
	return QString();
}

template<typename T> void load_rgb_image2(
	const typename T::Pointer & image,
	const ImageContainer & image_container,
	StudyGraphicsWidget * widget,
	const short fit)
{
	if (image.IsNull()) return;
	if (!image_container.image2D) return;
	if (!image_container.image3D) return;
	const ImageVariant * ivariant = image_container.image3D;
	const QString & rai = image_container.image2D->orientation_string;
	const QString & laterality = image_container.image2D->laterality;
	const QString & body_part = image_container.image2D->body_part;
	const QString & orientation_20_20 = image_container.orientation_20_20;
	const short image_type = image_container.image2D->image_type;
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
	// clear
	if (widget->graphicsview->image_item)
	{
		widget->graphicsview->scene()->removeItem(widget->graphicsview->image_item);
		delete widget->graphicsview->image_item;
		widget->graphicsview->image_item = NULL;
	}
	widget->graphicsview->image_item = new QGraphicsPixmapItem();
	widget->graphicsview->image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (widget->get_smooth())
		widget->graphicsview->image_item->setTransformationMode(Qt::SmoothTransformation);
	else
		widget->graphicsview->image_item->setTransformationMode(Qt::FastTransformation);
	widget->graphicsview->image_item->setCacheMode(QGraphicsItem::NoCache);
	widget->graphicsview->image_item->setPos(0, 0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#else
	if (!widget->graphicsview->image_item) return;
#endif
	const typename T::RegionType region = image->GetLargestPossibleRegion();
	const typename T::SizeType   size   = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	//
	QString top_string = QString(""), left_string = QString("");
	bool flip_x = false, flip_y = false;
	double scale__;
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	if (spacing[0] != spacing[1])
	{
		if (spacing[1]>spacing[0]) coeff_size_1 = spacing[1] / spacing[0];
		else                       coeff_size_0 = spacing[0] / spacing[1];
	}
	const double xratio = widget->graphicsview->width()  / (size[0] * coeff_size_0);
	const double yratio = widget->graphicsview->height() / (size[1] * coeff_size_1);
	if (fit == 1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else
	{
		scale__ = widget->graphicsview->m_scale;
	}
	//
	const unsigned short bits_allocated   = ivariant->di->bits_allocated;
	const unsigned short bits_stored      = ivariant->di->bits_stored;
	const bool           hide_orientation = ivariant->di->hide_orientation;
	//
	unsigned int j_ = 0;
	unsigned char * p__;
	try
	{
		p__ = new unsigned char[size[0] * size[1] * 3];
	}
	catch (const std::bad_alloc&)
	{
		return;
	}
	//
	if (image_type == 11)
	{
		const double tmp_max =
			(bits_allocated > 0 && bits_stored > 0 && bits_stored < bits_allocated)
				? pow(2, bits_stored) - 1
				: static_cast<double>(USHRT_MAX);
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				p__[j_ + 2] = static_cast<unsigned char>((iterator.Get().GetBlue()  / tmp_max) * 255.0);
				p__[j_ + 1] = static_cast<unsigned char>((iterator.Get().GetGreen() / tmp_max) * 255.0);
				p__[j_ + 0] = static_cast<unsigned char>((iterator.Get().GetRed()   / tmp_max) * 255.0);
				j_ += 3;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;;
		}
	}
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (vrange > 0.0)
		{
			try
			{
				itk::ImageRegionConstIterator<T> iterator(image, region);
				iterator.GoToBegin();
				while(!iterator.IsAtEnd())
				{
					const double b = iterator.Get().GetBlue();
					const double g = iterator.Get().GetGreen();
					const double r = iterator.Get().GetRed();
					p__[j_ + 2] = static_cast<unsigned char>(255.0 * ((b + (-vmin)) / vrange));
					p__[j_ + 1] = static_cast<unsigned char>(255.0 * ((g + (-vmin)) / vrange));
					p__[j_ + 0] = static_cast<unsigned char>(255.0 * ((r + (-vmin)) / vrange));
					j_ += 3;
 					++iterator;
				}
			}
			catch (const itk::ExceptionObject &)
			{
				;;
			}
		}
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QImage tmpi(p__, size[0], size[1], 3 * size[0], QImage::Format_RGB888, gImageCleanupHandler2, p__);
#else
	QImage tmpi(p__, size[0], size[1], 3 * size[0], QImage::Format_RGB888);
#endif
	//
	if (widget->get_enable_overlays()) GraphicsUtils::draw_overlays(ivariant, tmpi);
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	GraphicsUtils::gen_labels(
		2, hide_orientation,
		rai, laterality, body_part,
		orientation_20_20,
		left_string, top_string,
		global_flip_x, global_flip_y,
		&flip_x, &flip_y);
	//
	QTransform t = QTransform();
	if (spacing[1] != spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	if (flip_y && flip_x) t = t.scale(-1.0, -1.0);
	else if (flip_y)      t = t.scale( 1.0, -1.0);
	else if (flip_x)      t = t.scale(-1.0,  1.0);
	if (global_flip_x)    t = t.scale(-1.0,  1.0);
	if (global_flip_y)    t = t.scale( 1.0, -1.0);
	//
	const QRectF rectf(0, 0, size[0], size[1]);
	widget->graphicsview->scene()->setSceneRect(rectf);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p__)
	{
		delete [] p__;
		p__ = NULL;
	}
#endif
}

template<typename T> void load_rgba_image2(
	const typename T::Pointer & image,
	const ImageContainer & image_container,
	StudyGraphicsWidget * widget,
	const short fit)
{
	if (image.IsNull()) return;
	if (!image_container.image2D) return;
	if (!image_container.image3D) return;
	const ImageVariant * ivariant = image_container.image3D;
	const QString & rai = image_container.image2D->orientation_string;
	const short image_type = image_container.image2D->image_type;
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
	// clear
	if (widget->graphicsview->image_item)
	{
		widget->graphicsview->scene()->removeItem(widget->graphicsview->image_item);
		delete widget->graphicsview->image_item;
		widget->graphicsview->image_item = NULL;
	}
	widget->graphicsview->image_item = new QGraphicsPixmapItem();
	widget->graphicsview->image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (widget->get_smooth())
		widget->graphicsview->image_item->setTransformationMode(Qt::SmoothTransformation);
	else
		widget->graphicsview->image_item->setTransformationMode(Qt::FastTransformation);
	widget->graphicsview->image_item->setCacheMode(QGraphicsItem::NoCache);
	widget->graphicsview->image_item->setPos(0, 0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#else
	if (!widget->graphicsview->image_item) return;
#endif
	const typename T::RegionType region = image->GetLargestPossibleRegion();
	const typename T::SizeType   size   = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	//
	QString top_string = QString(""), left_string = QString("");
	bool flip_x = false, flip_y = false;
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	double scale__;
	if (spacing[0] != spacing[1])
	{
		if (spacing[1] > spacing[0]) coeff_size_1 = spacing[1] / spacing[0];
		else                         coeff_size_0 = spacing[0] / spacing[1];
	}
	const double xratio = widget->graphicsview->width()  / (size[0] * coeff_size_0);
	const double yratio = widget->graphicsview->height() / (size[1] * coeff_size_1);
	if (fit == 1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else
	{
		scale__ = widget->graphicsview->m_scale;
	}
	//
	const unsigned short bits_allocated   = ivariant->di->bits_allocated;
	const unsigned short bits_stored      = ivariant->di->bits_stored;
	const bool           hide_orientation = ivariant->di->hide_orientation;
	//
	unsigned long long j_ = 0;
	unsigned char * p__;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	try
	{
		p__ = new unsigned char[size[0] * size[1] * 4];
	}
	catch (const std::bad_alloc&)
	{
		return;
	}
	if (image_type == 21)
	{
		const double tmp_max =
			(bits_allocated > 0 && bits_stored > 0 && bits_stored < bits_allocated)
				? pow(2, bits_stored) - 1
				: static_cast<double>(USHRT_MAX);
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				p__[j_ + 3] = static_cast<unsigned char>((iterator.Get().GetAlpha() / tmp_max) * 255.0);
				p__[j_ + 2] = static_cast<unsigned char>((iterator.Get().GetBlue()  / tmp_max) * 255.0);
				p__[j_ + 1] = static_cast<unsigned char>((iterator.Get().GetGreen() / tmp_max) * 255.0);
				p__[j_ + 0] = static_cast<unsigned char>((iterator.Get().GetRed()   / tmp_max) * 255.0);
				j_ += 4;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;;
		}
	}
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (vrange > 0.0)
		{
			try
			{
				itk::ImageRegionConstIterator<T> iterator(image, region);
				iterator.GoToBegin();
				while(!iterator.IsAtEnd())
				{
					const double a = iterator.Get().GetAlpha();
					const double b = iterator.Get().GetBlue();
					const double g = iterator.Get().GetGreen();
					const double r = iterator.Get().GetRed();
					p__[j_+3] = static_cast<unsigned char>(255.0 * ((a + (-vmin)) / vrange));
					p__[j_+2] = static_cast<unsigned char>(255.0 * ((b + (-vmin)) / vrange));
					p__[j_+1] = static_cast<unsigned char>(255.0 * ((g + (-vmin)) / vrange));
					p__[j_+0] = static_cast<unsigned char>(255.0 * ((r + (-vmin)) / vrange));
					j_ += 4;
 					++iterator;
				}
			}
			catch (const itk::ExceptionObject &)
			{
				;;
			}
		}
	}
	QImage tmpi(p__, size[0], size[1], 4 * size[0], QImage::Format_RGBA8888, gImageCleanupHandler2, p__);
#else
	try
	{
		p__ = new unsigned char[size[0] * size[1] * 3];
	}
	catch (const std::bad_alloc&)
	{
		return;
	}
	if (image_type == 21)
	{
		const double tmp_max =
			(bits_allocated > 0 && bits_stored > 0 && bits_stored < bits_allocated)
				? pow(2, bits_stored) - 1
				: static_cast<double>(USHRT_MAX);
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				if (iterator.Get().GetAlpha() > 0)
				{
					const double alpha = iterator.Get().GetAlpha() / tmp_max;
					const double one_minus_alpha = 1.0 - alpha;
					const double tmp_whi = one_minus_alpha * USHRT_MAX;
					const double tmp_red = tmp_whi + alpha * iterator.Get().GetRed();
					const double tmp_gre = tmp_whi + alpha * iterator.Get().GetGreen();
					const double tmp_blu = tmp_whi + alpha * iterator.Get().GetBlue();
					p__[j_ + 2] = static_cast<unsigned char>((tmp_blu / tmp_max) * 255.0);
					p__[j_ + 1] = static_cast<unsigned char>((tmp_gre / tmp_max) * 255.0);
					p__[j_ + 0] = static_cast<unsigned char>((tmp_red / tmp_max) * 255.0);
				}
				else
				{
					p__[j_ + 2] = 255;
					p__[j_ + 1] = 255;
					p__[j_ + 0] = 255;
				}
				j_ += 3;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;;
		}
	}
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (vrange > 0.0)
		{
			try
			{
				itk::ImageRegionConstIterator<T> iterator(image, region);
				iterator.GoToBegin();
				while(!iterator.IsAtEnd())
				{
					const double alpha = iterator.Get().GetAlpha() / vrange;
					const double one_minus_alpha = 1.0 - alpha;
					const double tmp_whi = one_minus_alpha*vrange;
					const double tmp_red = tmp_whi + alpha*iterator.Get().GetRed();
					const double tmp_gre = tmp_whi + alpha*iterator.Get().GetGreen();
					const double tmp_blu = tmp_whi + alpha*iterator.Get().GetBlue();
					if (alpha > 0)
					{
						p__[j_ + 2] = static_cast<unsigned char>((tmp_blu / vrange) * 255.0);
						p__[j_ + 1] = static_cast<unsigned char>((tmp_gre / vrange) * 255.0);
						p__[j_ + 0] = static_cast<unsigned char>((tmp_red / vrange) * 255.0);
					}
					else
					{
						p__[j_ + 2] = 255;
						p__[j_ + 1] = 255;
						p__[j_ + 0] = 255;
					}
					j_ += 3;
 					++iterator;
				}
			}
			catch (const itk::ExceptionObject &)
			{
				;;
			}
		}
	}
	QImage tmpi(p__, size[0], size[1], 3 * size[0], QImage::Format_RGB888);
#endif
	//
	if (widget->get_enable_overlays())
		GraphicsUtils::draw_overlays(ivariant, tmpi);
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	GraphicsUtils::gen_labels(
		2, hide_orientation,
		rai, QString(""), QString(""),
		QString(""),
		left_string, top_string,
		global_flip_x, global_flip_y,
		&flip_x, &flip_y);
	//
	QTransform t = QTransform();
	if (spacing[1] != spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	if (flip_y && flip_x) t = t.scale(-1.0, -1.0);
	else if (flip_y)      t = t.scale( 1.0, -1.0);
	else if (flip_x)      t = t.scale(-1.0,  1.0);
	if (global_flip_x)    t = t.scale(-1.0,  1.0);
	if (global_flip_y)    t = t.scale( 1.0, -1.0);
	//
	const QRectF rectf(0, 0, size[0], size[1]);
	widget->graphicsview->scene()->setSceneRect(rectf);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p__)
	{
		delete [] p__;
		p__ = NULL;
	}
#endif
}

template<typename T> void load_rgb_char_image2(
	const typename T::Pointer & image,
	const ImageContainer & image_container,
	StudyGraphicsWidget * widget,
	const short fit)
{
	if (image.IsNull()) return;
	if (!image_container.image2D) return;
	if (!image_container.image3D) return;
	const ImageVariant * ivariant = image_container.image3D;
	const short image_type = image_container.image2D->image_type;
	if (image_type != 14) return;
	const QString & rai = image_container.image2D->orientation_string;
	const QString & laterality = image_container.image2D->laterality;
	const QString & body_part = image_container.image2D->body_part;
	const QString & orientation_20_20 = image_container.orientation_20_20;
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
	// clear
	if (widget->graphicsview->image_item)
	{
		widget->graphicsview->scene()->removeItem(widget->graphicsview->image_item);
		delete widget->graphicsview->image_item;
		widget->graphicsview->image_item = NULL;
	}
	//
	widget->graphicsview->image_item = new QGraphicsPixmapItem();
	widget->graphicsview->image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (widget->get_smooth())
		widget->graphicsview->image_item->setTransformationMode(Qt::SmoothTransformation);
	else
		widget->graphicsview->image_item->setTransformationMode(Qt::FastTransformation);
	widget->graphicsview->image_item->setCacheMode(QGraphicsItem::NoCache);
	widget->graphicsview->image_item->setPos(0, 0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#else
	if (!widget->graphicsview->image_item) return;
#endif
	const typename T::RegionType  region  = image->GetLargestPossibleRegion();
	const typename T::SizeType    size    = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	QString top_string = QString(""), left_string = QString("");
	bool flip_x = false, flip_y = false;
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	double scale__;
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	//
	unsigned char * p;
	try
	{
		p = reinterpret_cast<unsigned char *>(image->GetBufferPointer());
	}
	catch (const itk::ExceptionObject &)
	{
		return;
	}
	if (!p) return;
	//
	if (spacing[0] != spacing[1])
	{
		if (spacing[1] > spacing[0]) coeff_size_1 = spacing[1] / spacing[0];
		else                         coeff_size_0 = spacing[0] / spacing[1];
	}
	const double xratio = widget->graphicsview->width()  / (size[0] * coeff_size_0);
	const double yratio = widget->graphicsview->height() / (size[1] * coeff_size_1);
	if (fit == 1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else
	{
		scale__ = widget->graphicsview->m_scale;
	}
	//
	QImage tmpi = QImage(p, size[0], size[1], 3 * size[0], QImage::Format_RGB888);
	//
	if (widget->get_enable_overlays())
		GraphicsUtils::draw_overlays(ivariant, tmpi);
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	const bool hide_orientation = ivariant->di->hide_orientation;
	GraphicsUtils::gen_labels(
		2, hide_orientation,
		rai, laterality, body_part,
		orientation_20_20,
		left_string, top_string,
		global_flip_x, global_flip_y,
		&flip_x, &flip_y);
	//
	QTransform t = QTransform();
	if (spacing[1] != spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	if (flip_y && flip_x) t = t.scale(-1.0, -1.0);
	else if (flip_y)      t = t.scale( 1.0, -1.0);
	else if (flip_x)      t = t.scale(-1.0,  1.0);
	if (global_flip_x)    t = t.scale(-1.0,  1.0);
	if (global_flip_y)    t = t.scale( 1.0, -1.0);
	//
	const QRectF rectf(0, 0, size[0], size[1]);
	widget->graphicsview->scene()->setSceneRect(rectf);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->graphicsview->setTransform(t);
}

template<typename T> void load_rgba_char_image2(
	const typename T::Pointer & image,
	const ImageContainer & image_container,
	StudyGraphicsWidget * widget,
	const short fit)
{
	if (image.IsNull()) return;
	if (!image_container.image2D) return;
	if (!image_container.image3D) return;
	const ImageVariant * ivariant = image_container.image3D;
	const short image_type = image_container.image2D->image_type;
	if (image_type != 24) return;
	const QString & rai = image_container.image2D->orientation_string;
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
	// clear
	if (widget->graphicsview->image_item)
	{
		widget->graphicsview->scene()->removeItem(widget->graphicsview->image_item);
		delete widget->graphicsview->image_item;
		widget->graphicsview->image_item = NULL;
	}
	//
	widget->graphicsview->image_item = new QGraphicsPixmapItem();
	widget->graphicsview->image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (widget->get_smooth())
		widget->graphicsview->image_item->setTransformationMode(Qt::SmoothTransformation);
	else
		widget->graphicsview->image_item->setTransformationMode(Qt::FastTransformation);
	widget->graphicsview->image_item->setCacheMode(QGraphicsItem::NoCache);
	widget->graphicsview->image_item->setPos(0, 0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#else
	if (!widget->graphicsview->image_item) return;
#endif
	const typename T::RegionType region   = image->GetLargestPossibleRegion();
	const typename T::SizeType   size     = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	QString top_string = QString(""), left_string = QString("");
	bool flip_x = false, flip_y = false;
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	double scale__;
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	//
	if (spacing[0] != spacing[1])
	{
		if (spacing[1] > spacing[0]) coeff_size_1 = spacing[1] / spacing[0];
		else                         coeff_size_0 = spacing[0] / spacing[1];
	}
	const double xratio = widget->graphicsview->width()  / (size[0] * coeff_size_0);
	const double yratio = widget->graphicsview->height() / (size[1] * coeff_size_1);
	if (fit == 1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else
	{
		scale__ = widget->graphicsview->m_scale;
	}
	//
	QImage tmpi;
	unsigned char * p;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	try
	{
		p = reinterpret_cast<unsigned char *>(image->GetBufferPointer());
	}
	catch (const itk::ExceptionObject & ex)
	{
		return;
	}
	if (!p) return;
	tmpi = QImage(p, size[0], size[1], 4 * size[0], QImage::Format_RGBA8888);
#else
	try
	{
		p = new unsigned char[size[0] * size[1] * 3];
	}
	catch (const std::bad_alloc&)
	{
		return;
	}
	try
	{
		unsigned long long j_ = 0;
		itk::ImageRegionConstIterator<T> iterator(image, region);
		iterator.GoToBegin();
		while (!iterator.IsAtEnd())
		{
			if (iterator.Get().GetAlpha()>0)
			{
				const double alpha = iterator.Get().GetAlpha() / 255.0;
				const double one_minus_alpha = 1.0 - alpha;
				const double tmp_whi = one_minus_alpha * 255.0;
				const double tmp_red = tmp_whi + alpha * iterator.Get().GetRed();
				const double tmp_gre = tmp_whi + alpha * iterator.Get().GetGreen();
				const double tmp_blu = tmp_whi + alpha * iterator.Get().GetBlue();
				p[j_ + 2] = static_cast<unsigned char>(tmp_blu);
				p[j_ + 1] = static_cast<unsigned char>(tmp_gre);
				p[j_ + 0] = static_cast<unsigned char>(tmp_red);
			}
			else
			{
				p[j_ + 2] = 255;
				p[j_ + 1] = 255;
				p[j_ + 0] = 255;
			}
			j_ += 3;
			++iterator;
		}
	}
	catch (const itk::ExceptionObject &)
	{
		;;
	}
	tmpi = QImage(p, size[0], size[1], 3 * size[0], QImage::Format_RGB888);
#endif
	//
	if (widget->get_enable_overlays())
	{
		GraphicsUtils::draw_overlays(ivariant, tmpi);
	}
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	const bool hide_orientation = ivariant->di->hide_orientation;
	GraphicsUtils::gen_labels(
		2, hide_orientation,
		rai, QString(""), QString(""),
		QString(""),
		left_string, top_string,
		global_flip_x, global_flip_y,
		&flip_x, &flip_y);
	//
	QTransform t = QTransform();
	if (spacing[1] != spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	if (flip_y && flip_x) t = t.scale(-1.0, -1.0);
	else if (flip_y)      t = t.scale( 1.0, -1.0);
	else if (flip_x)      t = t.scale(-1.0,  1.0);
	if (global_flip_x)    t = t.scale(-1.0,  1.0);
	if (global_flip_y)    t = t.scale( 1.0, -1.0);
	//
	const QRectF rectf(0, 0, size[0], size[1]);
	widget->graphicsview->scene()->setSceneRect(rectf);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p)
	{
		delete [] p;
		p = NULL;
	}
#endif
}

template<typename T> void load_image2(
	const typename T::Pointer & image,
	const ImageContainer & image_container,
	StudyGraphicsWidget * widget,
	const short fit)
{
	if (image.IsNull()) return;
	if (!image_container.image2D) return;
	if (!image_container.image3D) return;
	const ImageVariant * ivariant = image_container.image3D;
	const QString & rai = image_container.image2D->orientation_string;
	const QString & laterality = image_container.image2D->laterality;
	const QString & body_part = image_container.image2D->body_part;
	const QString & orientation_20_20 = image_container.orientation_20_20;
	//
	const typename T::SpacingType spacing = image->GetSpacing();
	const typename T::RegionType region   = image->GetLargestPossibleRegion();
	const typename T::SizeType size       = region.GetSize();
	const short lut = image_container.selected_lut_ext;
	//
	const unsigned int p_size = 3 * size[0] * size[1];
	unsigned char * p;
	try
	{
		p = new unsigned char[p_size];
	}
	catch (const std::bad_alloc&)
	{
		return;
	}
	//
	double window_center, window_width;
	short lut_function;
	if (image_container.level_locked_ext)
	{
		if (ivariant->frame_levels.contains(image_container.selected_z_slice_ext))
		{
			const FrameLevel & fl =
				ivariant->frame_levels.value(image_container.selected_z_slice_ext);
			window_center = fl.us_window_center;
			window_width = fl.us_window_width;
			lut_function = fl.lut_function;
		}
		else
		{
			window_center = ivariant->di->default_us_window_center;
			window_width = ivariant->di->default_us_window_width;
			lut_function = ivariant->di->default_lut_function;
		}
	}
	else
	{
		window_center = image_container.us_window_center_ext;
		window_width = image_container.us_window_width_ext;
		lut_function = image_container.lut_function_ext;
	}
	//
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
#if 1
	const int num_threads = QThread::idealThreadCount();
#else
	int num_threads = QThread::idealThreadCount();
	if (num_threads > 1) num_threads - 1;
#endif
	const int tmp99 = size[1] % num_threads;
#if 0
	if (!widget->threadsLUT_.empty())
	{
		std::cout << "!widget->threadsLUT_.empty()" << std::endl;
	}
#endif
	if (tmp99 == 0)
	{
		unsigned int j = 0;
		for (int i = 0; i < num_threads; ++i)
		{
			const int size_0 = size[0];
			const int size_1 = size[1] / num_threads;
			const int index_0 = 0;
			const int index_1 = i * size_1;
			ProcessImageThreadLUT_<T> * t__ = new ProcessImageThreadLUT_<T>(
						image,
						p,
						size_0, size_1,
						index_0, index_1, j,
						window_center, window_width,
						lut, false, lut_function);
			j += 3 * size_0 * size_1;
			widget->threadsLUT_.push_back(static_cast<QThread*>(t__));
			t__->start();
		}
	}
	else
	{
		unsigned int j = 0;
		unsigned int block = 64;
		if (static_cast<float>(size[1]) / static_cast<float>(block) > 16.0f)
		{
			block = 128;
		}
		const int tmp100 = size[1] % block;
		const int incr = static_cast<int>(floor(size[1] / static_cast<double>(block)));
		if (size[1] > block)
		{
			for (int i = 0; i < incr; ++i)
			{
				const int size_0 = size[0];
				const int index_0 = 0;
				const int index_1 = i * block;
				ProcessImageThreadLUT_<T> * t__ = new ProcessImageThreadLUT_<T>(
							image,
							p,
							size_0, block,
							index_0, index_1, j,
							window_center, window_width,
							lut, false, lut_function);
				j += 3 * size_0 * block;
				widget->threadsLUT_.push_back(static_cast<QThread*>(t__));
				t__->start();
			}
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(
						image,
						p,
						size[0], tmp100,
						0, incr * block, j,
						window_center, window_width,
						lut, false, lut_function);
			widget->threadsLUT_.push_back(static_cast<QThread*>(lt__));
			lt__->start();
		}
		else
		{
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(
						image,
						p,
						size[0], size[1],
						0, 0, 0,
						window_center, window_width,
						lut, false, lut_function);
			widget->threadsLUT_.push_back(static_cast<QThread*>(lt__));
			lt__->start();
		}
	}
	//
	const size_t threadsLUT_size = widget->threadsLUT_.size();
	while (true)
	{
		size_t b__ = 0;
		for (size_t i = 0; i < threadsLUT_size; ++i)
		{
			if (widget->threadsLUT_.at(i)->isFinished()) ++b__;
		}
		if (b__ == threadsLUT_size) break;
	}
	for (size_t i = 0; i < threadsLUT_size; ++i)
	{
		delete widget->threadsLUT_[i];
		widget->threadsLUT_[i] = NULL;
	}
	widget->threadsLUT_.clear();
	//
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	const QRectF rectf(0, 0, size[0], size[1]);
	double scale__;
	if (spacing[0] != spacing[1])
	{
		if (spacing[1] > spacing[0]) coeff_size_1 = spacing[1] / spacing[0];
		else                         coeff_size_0 = spacing[0] / spacing[1];
	}
	QString top_string = QString(""), left_string = QString("");
	bool flip_y = false, flip_x = false;
	//
	widget->graphicsview->scene()->setSceneRect(rectf);
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
	// clear
	if (widget->graphicsview->image_item)
	{
		widget->graphicsview->scene()->removeItem(widget->graphicsview->image_item);
		delete widget->graphicsview->image_item;
		widget->graphicsview->image_item = NULL;
	}
	//
	widget->graphicsview->image_item = new QGraphicsPixmapItem();
	widget->graphicsview->image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (widget->get_smooth())
		widget->graphicsview->image_item->setTransformationMode(Qt::SmoothTransformation);
	else
		widget->graphicsview->image_item->setTransformationMode(Qt::FastTransformation);
	widget->graphicsview->image_item->setCacheMode(QGraphicsItem::NoCache);
	widget->graphicsview->image_item->setPos(0, 0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QImage tmpi(p, size[0], size[1], 3 * size[0], QImage::Format_RGB888, gImageCleanupHandler2, p);
#else
	QImage tmpi(p, size[0], size[1], 3 * size[0], QImage::Format_RGB888);
#endif
	//
	if (widget->get_enable_overlays()) GraphicsUtils::draw_overlays(ivariant, tmpi);
	//
	const double xratio = widget->graphicsview->width()  / (size[0]*coeff_size_0);
	const double yratio = widget->graphicsview->height() / (size[1]*coeff_size_1);
	if (fit == 1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else
	{
		scale__ = widget->graphicsview->m_scale;
	}
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	QTransform t = QTransform();
	if (spacing[1] != spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	//
	const bool hide_orientation = ivariant->di->hide_orientation;
	GraphicsUtils::gen_labels(
		2, hide_orientation,
		rai, laterality, body_part,
		orientation_20_20,
		left_string, top_string,
		global_flip_x, global_flip_y,
		&flip_x, &flip_y);
	//
	//std::cout << "Flip x=" << flip_x << " y=" << flip_y << std::endl;
	//
	widget->set_top_string(top_string);
	widget->set_left_string(left_string);
	//	
	if (flip_y && flip_x) t = t.scale(-1.0, -1.0);
	else if (flip_y)      t = t.scale( 1.0, -1.0);
	else if (flip_x)      t = t.scale(-1.0,  1.0);
	if (global_flip_x)    t = t.scale(-1.0,  1.0);
	if (global_flip_y)    t = t.scale( 1.0, -1.0);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p)
	{
		delete [] p;
		p = NULL;
	}
#endif
}

double get_distance4(
	const double x0,
	const double y0,
	const double x1,
	const double y1)
{
	const double x = x1 - x0;
	const double y = y1 - y0;
	return sqrt(x * x + y * y);
}

template<typename T> double get_distance3(
	const typename T::Pointer & image,
	const ImageVariant * ivariant,
	const double x0, const double y0,
	const double x1, const double y1,
	unsigned int z)
{
	if (image.IsNull()) return -1;
	if (!ivariant)      return -1;
	if (x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0) return -1;
	itk::ContinuousIndex<float, 3> idx0;
	itk::ContinuousIndex<float, 3> idx1;
	itk::Point<float, 3> j0;
	itk::Point<float, 3> j1;
	const float x0_ = static_cast<float>(x0);
	const float y0_ = static_cast<float>(y0);
	const float x1_ = static_cast<float>(x1);
	const float y1_ = static_cast<float>(y1);
	double d = -1;
	bool ok = false;
	//
	if (
		x0_ < ivariant->di->idimx &&
		x1_ < ivariant->di->idimx &&
		y0_ < ivariant->di->idimy &&
		y1_ < ivariant->di->idimy)
	{
		ok = true;
		idx0[2] = idx1[2] = z;
		idx0[0] = x0_;
		idx0[1] = y0_;
		idx1[0] = x1_;
		idx1[1] = y1_;
	}
	//
	if (ok)
	{
		image->TransformContinuousIndexToPhysicalPoint(idx0, j0);
		image->TransformContinuousIndexToPhysicalPoint(idx1, j1);
		d = j0.EuclideanDistanceTo(j1);
	}
	return d;
}

}

static unsigned long long StudyGraphicsWidget_id = 0;

StudyGraphicsWidget::StudyGraphicsWidget()
{
	widget_id = ++StudyGraphicsWidget_id;
	studyview = NULL;
	slider = NULL;
	top_label = NULL;
	left_label = NULL;
	measure_label = NULL;
	icon_button = NULL;
	smooth_ = true;
	mouse_modus = 0;
	enable_shutter = true;
	enable_overlays = true;
	image_container.image3D = NULL;
	image_container.axis = 2;
	image_container.image2D = new ImageVariant2D();
	graphicsview = new StudyGraphicsView(this);
	QVBoxLayout * l = new QVBoxLayout(this);
	l->addWidget(graphicsview);
	l->setSpacing(0);
	l->setContentsMargins(0, 0, 0, 0);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

StudyGraphicsWidget::~StudyGraphicsWidget()
{
	if (mutex.tryLock(3000))
	{
		for (unsigned int i = 0; i < threadsLUT_.size(); ++i)
		{
			if (threadsLUT_.at(i))
			{
				if (threadsLUT_.at(i)->isRunning()) threadsLUT_[i]->exit();
				delete threadsLUT_[i];
				threadsLUT_[i] = NULL;
			}
		}
		if (image_container.image2D)
		{
			delete image_container.image2D;
			image_container.image2D = NULL;
		}
		image_container.image3D = NULL;
		mutex.unlock();
	}
}

void StudyGraphicsWidget::set_studyview(StudyViewWidget * v)
{
	studyview = v;
}

void StudyGraphicsWidget::set_slider(QSlider * s)
{
	slider = s;
}

void StudyGraphicsWidget::set_top_label(QLabel * l)
{
	top_label = l;
}

void StudyGraphicsWidget::set_left_label(QLabel * l)
{
	left_label = l;
}

void StudyGraphicsWidget::set_measure_label(QLabel * l)
{
	measure_label = l;
}

void StudyGraphicsWidget::set_icon_button(QToolButton * b)
{
	icon_button = b;
}

void StudyGraphicsWidget::set_top_string(const QString & s)
{
	if (top_label) top_label->setText(s);
}

void StudyGraphicsWidget::set_left_string(const QString & s)
{
	if (left_label) left_label->setText(s);
}

void StudyGraphicsWidget::set_measure_text(const QString & s)
{
	if (measure_label) measure_label->setText(s);
}

void StudyGraphicsWidget::set_center(double x)
{
	mutex.lock();
	image_container.us_window_center_ext = x;
	update_image(0, false);
	mutex.unlock();
}

void StudyGraphicsWidget::set_width(double x)
{
	mutex.lock();
	image_container.us_window_width_ext = x;
	update_image(0, false);
	mutex.unlock();
}

void StudyGraphicsWidget::set_lut(short x)
{
	mutex.lock();
	image_container.selected_lut_ext = x;
	update_image(0, false);
	mutex.unlock();
}

void StudyGraphicsWidget::set_lut_function(short x)
{
	mutex.lock();
	image_container.lut_function_ext = x;
	update_image(0, false);
	mutex.unlock();
}

void StudyGraphicsWidget::set_locked_window(bool t)
{
	mutex.lock();
	image_container.level_locked_ext = t;
	update_image(0, false);
	mutex.unlock();
}

void StudyGraphicsWidget::reset_level()
{
	mutex.lock();
	if (image_container.image3D)
	{
		image_container.us_window_center_ext =
			image_container.image3D->di->default_us_window_center;
		image_container.us_window_width_ext =
			image_container.image3D->di->default_us_window_width;
		image_container.lut_function_ext =
			image_container.image3D->di->default_lut_function;
		update_image(0, false);
	}
	mutex.unlock();
}

void StudyGraphicsWidget::update_image_color(int r, int g, int b)
{
	if (icon_button)
	{
		QPixmap pp(14, 14);
		QColor c(r, g, b);
		pp.fill(c);
		icon_button->setIcon(pp);
	}
}

void StudyGraphicsWidget::closeEvent(QCloseEvent * e)
{
	e->accept();
}

void StudyGraphicsWidget::leaveEvent(QEvent*)
{
}

void StudyGraphicsWidget::update_pr_area()
{
	if (!image_container.image3D) return;
	const int idx = image_container.selected_z_slice_ext;
	if (image_container.image3D->pr_display_areas.contains(idx))
	{
		const int tl_x =
			image_container.image3D->
				pr_display_areas.value(idx).top_left_x - 1;
		const int tl_y =
			image_container.image3D->
				pr_display_areas.value(idx).top_left_y - 1;
		const int s_x =
			image_container.image3D->
				pr_display_areas.value(idx).bottom_right_x - tl_x;
		const int s_y =
			image_container.image3D->
				pr_display_areas.value(idx).bottom_right_y - tl_y;
		QRectF r = QRectF(QPointF(tl_x, tl_y), QSize(s_x, s_y));
		graphicsview->pr_area->resetTransform();
		graphicsview->pr_area->setRect(r);
		graphicsview->pr_area->show();
	}
}

void StudyGraphicsWidget::update_image(
	const short fit,
	const bool lock)
{
	if (!image_container.image3D) return;
	if (!image_container.image2D) return;
	//
	if (lock) mutex.lock();
	//
	switch (image_container.image2D->image_type)
	{
	case 0: load_image2<Image2DTypeSS>(
				image_container.image2D->pSS,
				image_container,
				this,
				fit);
		break;
	case 1: load_image2<Image2DTypeUS>(
				image_container.image2D->pUS,
				image_container,
				this,
				fit);
		break;
	case 2: load_image2<Image2DTypeSI>(
				image_container.image2D->pSI,
				image_container,
				this,
				fit);
		break;
	case 3: load_image2<Image2DTypeUI>(
				image_container.image2D->pUI,
				image_container,
				this,
				fit);
		break;
	case 4: load_image2<Image2DTypeUC>(
				image_container.image2D->pUC,
				image_container,
				this,
				fit);
		break;
	case 5: load_image2<Image2DTypeF>(
				image_container.image2D->pF,
				image_container,
				this,
				fit);
		break;
	case 6: load_image2<Image2DTypeD>(
				image_container.image2D->pD,
				image_container,
				this,
				fit);
		break;
	case 7: load_image2<Image2DTypeSLL>(
				image_container.image2D->pSLL,
				image_container,
				this,
				fit);
		break;
	case 8: load_image2<Image2DTypeULL>(
				image_container.image2D->pULL,
				image_container,
				this,
				fit);
		break;
	case 10: load_rgb_image2<RGBImage2DTypeSS>(
				image_container.image2D->pSS_rgb,
				image_container,
				this,
				fit);
		break;
	case 11: load_rgb_image2<RGBImage2DTypeUS>(
				image_container.image2D->pUS_rgb,
				image_container,
				this,
				fit);
		break;
	case 12: load_rgb_image2<RGBImage2DTypeSI>(
				image_container.image2D->pSI_rgb,
				image_container,
				this,
				fit);
		break;
	case 13: load_rgb_image2<RGBImage2DTypeUI>(
				image_container.image2D->pUI_rgb,
				image_container,
				this,
				fit);
		break;
	case 14: load_rgb_char_image2<RGBImage2DTypeUC>(
				image_container.image2D->pUC_rgb,
				image_container,
				this,
				fit);
		break;
	case 15: load_rgb_image2<RGBImage2DTypeF>(
				image_container.image2D->pF_rgb,
				image_container,
				this,
				fit);
		break;
	case 16: load_rgb_image2<RGBImage2DTypeD>(
				image_container.image2D->pD_rgb,
				image_container,
				this,
				fit);
		break;
	case 20: load_rgba_image2<RGBAImage2DTypeSS>(
				image_container.image2D->pSS_rgba,
				image_container,
				this,
				fit);
		break;
	case 21: load_rgba_image2<RGBAImage2DTypeUS>(
				image_container.image2D->pUS_rgba,
				image_container,
				this,
				fit);
		break;
	case 22: load_rgba_image2<RGBAImage2DTypeSI>(
				image_container.image2D->pSI_rgba,
				image_container,
				this,
				fit);
		break;
	case 23: load_rgba_image2<RGBAImage2DTypeUI>(
				image_container.image2D->pUI_rgba,
				image_container,
				this,
				fit);
		break;
	case 24: load_rgba_char_image2<RGBAImage2DTypeUC>(
				image_container.image2D->pUC_rgba,
				image_container,
				this,
				fit);
		break;
	case 25: load_rgba_image2<RGBAImage2DTypeF>(
				image_container.image2D->pF_rgba,
				image_container,
				this,
				fit);
		break;
	case 26: load_rgba_image2<RGBAImage2DTypeD>(
				image_container.image2D->pD_rgba,
				image_container,
				this,
				fit);
		break;
	default:
		break;
	}
	//
	if (lock) mutex.unlock();
}

void StudyGraphicsWidget::clear_(bool lock)
{
	if (lock) mutex.lock();
	if (graphicsview->image_item)
	{
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
		if (graphicsview->scene()) graphicsview->scene()->removeItem(graphicsview->image_item);
		delete graphicsview->image_item;
		graphicsview->image_item = NULL;
#else
		QPixmap p(16, 16);
		graphicsview->image_item->setPixmap(p);
#endif
	}
	if (slider)
	{
		disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice(int)));
		slider->setValue(0);
		slider->setMaximum(1);
		slider->setEnabled(false);
	}
	if (icon_button)
	{
		disconnect(icon_button, SIGNAL(toggled(bool)), this, SLOT(toggle_single(bool)));
		icon_button->setIcon(QPixmap());
		icon_button->setEnabled(false);
	}
	graphicsview->clear_collision_paths();
	graphicsview->pr_area->hide();
	set_top_string(QString(""));
	set_left_string(QString(""));
	set_measure_text(QString(""));
	if (image_container.image2D)
	{
		image_container.image2D->image_type = -1;
		image_container.image2D->orientation_string = QString("");
		image_container.image2D->laterality = QString("");
		image_container.image2D->body_part = QString("");
		image_container.image2D->idimx = 0;
		image_container.image2D->idimy = 0;
		if (image_container.image2D->pSS.IsNotNull())
		{
			image_container.image2D->pSS->DisconnectPipeline();
			image_container.image2D->pSS = NULL;
		}
		if (image_container.image2D->pUS.IsNotNull())
		{
			image_container.image2D->pUS->DisconnectPipeline();
			image_container.image2D->pUS = NULL;
		}
		if (image_container.image2D->pSI.IsNotNull())
		{
			image_container.image2D->pSI->DisconnectPipeline();
			image_container.image2D->pSI = NULL;
		}
		if (image_container.image2D->pUI.IsNotNull())
		{
			image_container.image2D->pUI->DisconnectPipeline();
			image_container.image2D->pUI = NULL;
		}
		if (image_container.image2D->pUC.IsNotNull())
		{
			image_container.image2D->pUC->DisconnectPipeline();
			image_container.image2D->pUC = NULL;
		}
		if (image_container.image2D->pF.IsNotNull())
		{
			image_container.image2D->pF->DisconnectPipeline();
			image_container.image2D->pF = NULL;
		}
		if (image_container.image2D->pD.IsNotNull())
		{
			image_container.image2D->pD->DisconnectPipeline();
			image_container.image2D->pD = NULL;
		}
		if (image_container.image2D->pSLL.IsNotNull())
		{
			image_container.image2D->pSLL->DisconnectPipeline();
			image_container.image2D->pSLL = NULL;
		}
		if (image_container.image2D->pULL.IsNotNull())
		{
			image_container.image2D->pULL->DisconnectPipeline();
			image_container.image2D->pULL = NULL;
		}
		if (image_container.image2D->pSS_rgb.IsNotNull())
		{
			image_container.image2D->pSS_rgb->DisconnectPipeline();
			image_container.image2D->pSS_rgb = NULL;
		}
		if (image_container.image2D->pUS_rgb.IsNotNull())
		{
			image_container.image2D->pUS_rgb->DisconnectPipeline();
			image_container.image2D->pUS_rgb = NULL;
		}
		if (image_container.image2D->pSI_rgb.IsNotNull())
		{
			image_container.image2D->pSI_rgb->DisconnectPipeline();
			image_container.image2D->pSI_rgb = NULL;
		}
		if (image_container.image2D->pUI_rgb.IsNotNull())
		{
			image_container.image2D->pUI_rgb->DisconnectPipeline();
			image_container.image2D->pUI_rgb = NULL;
		}
		if (image_container.image2D->pUC_rgb.IsNotNull())
		{
			image_container.image2D->pUC_rgb->DisconnectPipeline();
			image_container.image2D->pUC_rgb = NULL;
		}
		if (image_container.image2D->pF_rgb .IsNotNull())
		{
			image_container.image2D->pF_rgb->DisconnectPipeline();
			image_container.image2D->pF_rgb = NULL;
		}
		if (image_container.image2D->pD_rgb.IsNotNull())
		{
			image_container.image2D->pD_rgb->DisconnectPipeline();
			image_container.image2D->pD_rgb = NULL;
		}
		if (image_container.image2D->pSS_rgba.IsNotNull())
		{
			image_container.image2D->pSS_rgba->DisconnectPipeline();
			image_container.image2D->pSS_rgba = NULL;
		}
		if (image_container.image2D->pUS_rgba.IsNotNull())
		{
			image_container.image2D->pUS_rgba->DisconnectPipeline();
			image_container.image2D->pUS_rgba = NULL;
		}
		if (image_container.image2D->pSI_rgba.IsNotNull())
		{
			image_container.image2D->pSI_rgba->DisconnectPipeline();
			image_container.image2D->pSI_rgba = NULL;
		}
		if (image_container.image2D->pUI_rgba.IsNotNull())
		{
			image_container.image2D->pUI_rgba->DisconnectPipeline();
			image_container.image2D->pUI_rgba = NULL;
		}
		if (image_container.image2D->pUC_rgba.IsNotNull())
		{
			image_container.image2D->pUC_rgba->DisconnectPipeline();
			image_container.image2D->pUC_rgba = NULL;
		}
		if (image_container.image2D->pF_rgba.IsNotNull())
		{
			image_container.image2D->pF_rgba->DisconnectPipeline();
			image_container.image2D->pF_rgba = NULL;
		}
		if (image_container.image2D->pD_rgba.IsNotNull())
		{
			image_container.image2D->pD_rgba->DisconnectPipeline();
			image_container.image2D->pD_rgba = NULL;
		}
	}
	image_container.image3D = NULL;
	graphicsview->set_empty_distance();
	graphicsview->clear_us_regions();
	graphicsview->clear_prtexts_items();
	graphicsview->clear_prgraphicobjects_items();
	graphicsview->clear_shutters();
	if (lock) mutex.unlock();
}

void StudyGraphicsWidget::set_image(
	ImageVariant * v,
	const short fit,
	const bool /* always draw US regions */)
{
	if (graphicsview->image_item)
	{
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
		graphicsview->scene()->removeItem(graphicsview->image_item);
		delete graphicsview->image_item;
		graphicsview->image_item = NULL;
#else
		QPixmap p(16, 16);
		graphicsview->image_item->setPixmap(p);
#endif
	}
	graphicsview->clear_collision_paths();
	graphicsview->clear_us_regions();
	graphicsview->clear_prtexts_items();
	graphicsview->clear_prgraphicobjects_items();
	graphicsview->clear_shutters();
	graphicsview->set_empty_distance();
	graphicsview->pr_area->hide();
	set_top_string(QString(""));
	set_left_string(QString(""));
	set_measure_text(QString(""));
	image_container.orientation_20_20 = QString("");
	//
	if (!v)
	{
		image_container.selected_z_slice_ext = -1;
		image_container.us_window_center_ext = 0.0;
		image_container.us_window_width_ext = 1e-6;
		image_container.selected_lut_ext = 0;
		image_container.lut_function_ext = 0;
		image_container.level_locked_ext = true;
		return;
	}
	//
	int x = 0;
	QString error_;
	//
	mutex.lock();
	//
	image_container.image3D = v;
	//
	image_container.image2D->image_type = -1;
	image_container.image2D->orientation_string = QString("");
	image_container.image2D->laterality = QString("");
	image_container.image2D->body_part = QString("");
	image_container.image2D->idimx = 0;
	image_container.image2D->idimy = 0;
	if (image_container.image2D->pSS.IsNotNull())
	{
		image_container.image2D->pSS->DisconnectPipeline();
		image_container.image2D->pSS = NULL;
	}
	if (image_container.image2D->pUS.IsNotNull())
	{
		image_container.image2D->pUS->DisconnectPipeline();
		image_container.image2D->pUS = NULL;
	}
	if (image_container.image2D->pSI.IsNotNull())
	{
		image_container.image2D->pSI->DisconnectPipeline();
		image_container.image2D->pSI = NULL;
	}
	if (image_container.image2D->pUI.IsNotNull())
	{
		image_container.image2D->pUI->DisconnectPipeline();
		image_container.image2D->pUI = NULL;
	}
	if (image_container.image2D->pUC.IsNotNull())
	{
		image_container.image2D->pUC->DisconnectPipeline();
		image_container.image2D->pUC = NULL;
	}
	if (image_container.image2D->pF.IsNotNull())
	{
		image_container.image2D->pF->DisconnectPipeline();
		image_container.image2D->pF = NULL;
	}
	if (image_container.image2D->pD.IsNotNull())
	{
		image_container.image2D->pD->DisconnectPipeline();
		image_container.image2D->pD = NULL;
	}
	if (image_container.image2D->pSLL.IsNotNull())
	{
		image_container.image2D->pSLL->DisconnectPipeline();
		image_container.image2D->pSLL = NULL;
	}
	if (image_container.image2D->pULL.IsNotNull())
	{
		image_container.image2D->pULL->DisconnectPipeline();
		image_container.image2D->pULL = NULL;
	}
	if (image_container.image2D->pSS_rgb.IsNotNull())
	{
		image_container.image2D->pSS_rgb->DisconnectPipeline();
		image_container.image2D->pSS_rgb = NULL;
	}
	if (image_container.image2D->pUS_rgb.IsNotNull())
	{
		image_container.image2D->pUS_rgb->DisconnectPipeline();
		image_container.image2D->pUS_rgb = NULL;
	}
	if (image_container.image2D->pSI_rgb.IsNotNull())
	{
		image_container.image2D->pSI_rgb->DisconnectPipeline();
		image_container.image2D->pSI_rgb = NULL;
	}
	if (image_container.image2D->pUI_rgb.IsNotNull())
	{
		image_container.image2D->pUI_rgb->DisconnectPipeline();
		image_container.image2D->pUI_rgb = NULL;
	}
	if (image_container.image2D->pUC_rgb.IsNotNull())
	{
		image_container.image2D->pUC_rgb ->DisconnectPipeline();
		image_container.image2D->pUC_rgb = NULL;
	}
	if (image_container.image2D->pF_rgb.IsNotNull())
	{
		image_container.image2D->pF_rgb->DisconnectPipeline();
		image_container.image2D->pF_rgb = NULL;
	}
	if (image_container.image2D->pD_rgb.IsNotNull())
	{
		image_container.image2D->pD_rgb->DisconnectPipeline();
		image_container.image2D->pD_rgb = NULL;
	}
	if (image_container.image2D->pSS_rgba.IsNotNull())
	{
		image_container.image2D->pSS_rgba->DisconnectPipeline();
		image_container.image2D->pSS_rgba = NULL;
	}
	if (image_container.image2D->pUS_rgba.IsNotNull())
	{
		image_container.image2D->pUS_rgba->DisconnectPipeline();
		image_container.image2D->pUS_rgba = NULL;
	}
	if (image_container.image2D->pSI_rgba.IsNotNull())
	{
		image_container.image2D->pSI_rgba->DisconnectPipeline();
		image_container.image2D->pSI_rgba = NULL;
	}
	if (image_container.image2D->pUI_rgba.IsNotNull())
	{
		image_container.image2D->pUI_rgba->DisconnectPipeline();
		image_container.image2D->pUI_rgba = NULL;
	}
	if (image_container.image2D->pUC_rgba.IsNotNull())
	{
		image_container.image2D->pUC_rgba->DisconnectPipeline();
		image_container.image2D->pUC_rgba = NULL;
	}
	if (image_container.image2D->pF_rgba.IsNotNull())
	{
		image_container.image2D->pF_rgba->DisconnectPipeline();
		image_container.image2D->pF_rgba = NULL;
	}
	if (image_container.image2D->pD_rgba.IsNotNull())
	{
		image_container.image2D->pD_rgba->DisconnectPipeline();
		image_container.image2D->pD_rgba = NULL;
	}
	//
	x = image_container.image3D->di->idimz / 2;
	image_container.selected_z_slice_ext = x;
	image_container.us_window_center_ext = v->di->us_window_center;
	image_container.us_window_width_ext = v->di->us_window_width;
	image_container.selected_lut_ext = v->di->selected_lut;
	image_container.lut_function_ext = v->di->lut_function;
	image_container.level_locked_ext = v->di->lock_level2D;
	//
	switch (v->image_type)
	{
	case 0: error_ = get_slice2_<ImageTypeSS, Image2DTypeSS>(
				v->pSS, image_container.image2D, image_container.image2D->pSS, x);
		break;
	case 1: error_ = get_slice2_<ImageTypeUS, Image2DTypeUS>(
				v->pUS, image_container.image2D, image_container.image2D->pUS, x);
		break;
	case 2: error_ = get_slice2_<ImageTypeSI, Image2DTypeSI>(
				v->pSI, image_container.image2D, image_container.image2D->pSI, x);
		break;
	case 3: error_ = get_slice2_<ImageTypeUI, Image2DTypeUI>(
				v->pUI, image_container.image2D, image_container.image2D->pUI, x);
		break;
	case 4: error_ = get_slice2_<ImageTypeUC, Image2DTypeUC>(
				v->pUC, image_container.image2D, image_container.image2D->pUC, x);
		break;
	case 5: error_ = get_slice2_<ImageTypeF, Image2DTypeF>(
				v->pF, image_container.image2D, image_container.image2D->pF, x);
		break;
	case 6: error_ = get_slice2_<ImageTypeD, Image2DTypeD>(
				v->pD, image_container.image2D, image_container.image2D->pD, x);
		break;
	case 7: error_ = get_slice2_<ImageTypeSLL, Image2DTypeSLL>(
				v->pSLL, image_container.image2D, image_container.image2D->pSLL, x);
		break;
	case 8: error_ = get_slice2_<ImageTypeULL, Image2DTypeULL>(
				v->pULL, image_container.image2D, image_container.image2D->pULL, x);
		break;
	case 10: error_ = get_slice2_<RGBImageTypeSS, RGBImage2DTypeSS>(
				v->pSS_rgb, image_container.image2D, image_container.image2D->pSS_rgb, x);
		break;
	case 11: error_ = get_slice2_<RGBImageTypeUS, RGBImage2DTypeUS>(
				v->pUS_rgb, image_container.image2D, image_container.image2D->pUS_rgb, x);
		break;
	case 12: error_ = get_slice2_<RGBImageTypeSI, RGBImage2DTypeSI>(
				v->pSI_rgb, image_container.image2D, image_container.image2D->pSI_rgb, x);
		break;
	case 13: error_ = get_slice2_<RGBImageTypeUI, RGBImage2DTypeUI>(
				v->pUI_rgb, image_container.image2D, image_container.image2D->pUI_rgb, x);
		break;
	case 14: error_ = get_slice2_<RGBImageTypeUC, RGBImage2DTypeUC>(
				v->pUC_rgb, image_container.image2D, image_container.image2D->pUC_rgb, x);
		break;
	case 15: error_ = get_slice2_<RGBImageTypeF, RGBImage2DTypeF>(
				v->pF_rgb, image_container.image2D, image_container.image2D->pF_rgb, x);
		break;
	case 16: error_ = get_slice2_<RGBImageTypeD, RGBImage2DTypeD>(
				v->pD_rgb, image_container.image2D, image_container.image2D->pD_rgb, x);
		break;
	case 20: error_ = get_slice2_<RGBAImageTypeSS, RGBAImage2DTypeSS>(
				v->pSS_rgba, image_container.image2D, image_container.image2D->pSS_rgba, x);
		break;
	case 21: error_ = get_slice2_<RGBAImageTypeUS, RGBAImage2DTypeUS>(
				v->pUS_rgba, image_container.image2D, image_container.image2D->pUS_rgba, x);
		break;
	case 22: error_ = get_slice2_<RGBAImageTypeSI, RGBAImage2DTypeSI>(
				v->pSI_rgba, image_container.image2D, image_container.image2D->pSI_rgba, x);
		break;
	case 23: error_ = get_slice2_<RGBAImageTypeUI, RGBAImage2DTypeUI>(
				v->pUI_rgba, image_container.image2D, image_container.image2D->pUI_rgba, x);
		break;
	case 24: error_ = get_slice2_<RGBAImageTypeUC, RGBAImage2DTypeUC>(
				v->pUC_rgba, image_container.image2D, image_container.image2D->pUC_rgba, x);
		break;
	case 25: error_ = get_slice2_<RGBAImageTypeF, RGBAImage2DTypeF>(
				v->pF_rgba, image_container.image2D, image_container.image2D->pF_rgba, x);
		break;
	case 26: error_ = get_slice2_<RGBAImageTypeD, RGBAImage2DTypeD>(
				v->pD_rgba, image_container.image2D, image_container.image2D->pD_rgba, x);
		break;
	default:
		{
			clear_(false);
			goto quit__;
		}
	}
	//
	if (error_.isEmpty())
	{
		image_container.image2D->image_type = v->image_type;
	}
	else
	{
		goto quit__;
	}
	//
	{
		const bool check_consistence =
			((static_cast<int>(v->di->image_slices.size()) == v->di->idimz) &&
				static_cast<int>(v->di->image_slices.size()) > x);
		if (v->equi)
		{
			image_container.image2D->orientation_string = v->orientation_string;
		}
		else
		{
			if (check_consistence &&
				!v->di->image_slices.at(x)->slice_orientation_string.isEmpty())
			{
				image_container.image2D->orientation_string =
					v->di->image_slices.at(x)->slice_orientation_string;
			}
		}
	}
	if (v->orientations_20_20.contains(x))
	{
		const QString s2020 = v->orientations_20_20.value(x);
		if (!s2020.isEmpty())
		{
			image_container.orientation_20_20 = s2020;
		}
	}
	if (v->anatomy.contains(x))
	{
		image_container.image2D->laterality = v->anatomy.value(x).laterality;
		image_container.image2D->body_part  = v->anatomy.value(x).body_part;
	}
	//
	if (slider)
	{
		slider->setMaximum(v->di->idimz - 1);
		slider->setValue(x);
		connect(slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice(int)));
		slider->setEnabled(true);
	}
	if (icon_button)
	{
		QPixmap pp(14, 14);
		QColor c(
			static_cast<int>(v->di->R * 255.0f),
			static_cast<int>(v->di->G * 255.0f),
			static_cast<int>(v->di->B * 255.0f));
		pp.fill(c);
		icon_button->setIcon(pp);
		connect(icon_button, SIGNAL(toggled(bool)), this, SLOT(toggle_single(bool)));
		icon_button->setEnabled(true);
	}
	//
	update_image(fit, false);
	//
	graphicsview->draw_us_regions();
	if (!v->pr_display_areas.empty())
	{
		update_pr_area();
	}
	else
	{
		if (graphicsview->pr_area->isVisible()) graphicsview->pr_area->hide();
	}
quit__:
	mutex.unlock();
}

void StudyGraphicsWidget::update_background_color()
{
	graphicsview->update_background_color();
}

void StudyGraphicsWidget::set_smooth(bool t)
{
	smooth_ = t;
#ifndef DELETE_STUDYGRAPHICSIMAGEITEM
	if (graphicsview->image_item)
	{
		if (t)
		{
			graphicsview->image_item->setTransformationMode(
				Qt::SmoothTransformation);
		}
		else
		{
			graphicsview->image_item->setTransformationMode(
				Qt::FastTransformation);
		}
	}
#endif
	update_image(0, true);
}

bool StudyGraphicsWidget::get_smooth() const
{
	return smooth_;
}

void StudyGraphicsWidget::set_mouse_modus(short m, bool us_regions)
{
	mouse_modus = m;
	set_measure_text(QString(""));
	switch (mouse_modus)
	{
	case 2: // measure distance
		{
			graphicsview->setTransformationAnchor(
				QGraphicsView::AnchorUnderMouse);
			graphicsview->setDragMode(QGraphicsView::NoDrag);
			graphicsview->setCursor(Qt::ArrowCursor);
			graphicsview->measurment_line->show();
			graphicsview->draw_us_regions();
		}
		break;
	default:
		{
			graphicsview->setTransformationAnchor(
				QGraphicsView::AnchorUnderMouse);
			graphicsview->setCursor(Qt::OpenHandCursor);
			graphicsview->setDragMode(QGraphicsView::ScrollHandDrag);
			graphicsview->measurment_line->hide();
			graphicsview->set_empty_distance();
			if (us_regions) graphicsview->draw_us_regions();
			else            graphicsview->clear_us_regions();
		}
		break;
	}
}

short StudyGraphicsWidget::get_mouse_modus() const
{
	return mouse_modus;
}

bool StudyGraphicsWidget::get_enable_shutter() const
{
	return enable_shutter;
}

bool StudyGraphicsWidget::get_enable_overlays() const
{
	return enable_overlays;
}

void StudyGraphicsWidget::set_enable_shutter(bool t)
{
	enable_shutter = t;
}

void StudyGraphicsWidget::set_enable_overlays(bool t)
{
	enable_overlays = t;
}

void StudyGraphicsWidget::set_selected_slice(int x)
{
	if (graphicsview->image_item)
	{
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
		graphicsview->scene()->removeItem(graphicsview->image_item);
		delete graphicsview->image_item;
		graphicsview->image_item = NULL;
#else
		QPixmap p(16, 16);
		graphicsview->image_item->setPixmap(p);
#endif
	}
	graphicsview->clear_collision_paths();
	graphicsview->clear_us_regions();
	graphicsview->clear_prtexts_items();
	graphicsview->clear_prgraphicobjects_items();
	graphicsview->clear_shutters();
	graphicsview->set_empty_distance();
	graphicsview->pr_area->hide();
	set_top_string(QString(""));
	set_left_string(QString(""));
	set_measure_text(QString(""));
	image_container.orientation_20_20 = QString("");
	image_container.selected_z_slice_ext = x;
	//
	if (!image_container.image3D) return;
	QString error_;
	//
	mutex.lock();
	//
	image_container.image2D->image_type = image_container.image3D->image_type;
	image_container.image2D->orientation_string = QString("");
	image_container.image2D->laterality = QString("");
	image_container.image2D->body_part  = QString("");
	image_container.image2D->idimx = 0;
	image_container.image2D->idimy = 0;
	if (image_container.image2D->pSS.IsNotNull())
		image_container.image2D->pSS->DisconnectPipeline();
	image_container.image2D->pSS = NULL;
	if (image_container.image2D->pUS.IsNotNull())
		image_container.image2D->pUS->DisconnectPipeline();
	image_container.image2D->pUS = NULL;
	if (image_container.image2D->pSI.IsNotNull())
		image_container.image2D->pSI->DisconnectPipeline();
	image_container.image2D->pSI = NULL;
	if (image_container.image2D->pUI.IsNotNull())
		image_container.image2D->pUI->DisconnectPipeline();
	image_container.image2D->pUI = NULL;
	if (image_container.image2D->pUC.IsNotNull())
		image_container.image2D->pUC->DisconnectPipeline();
	image_container.image2D->pUC = NULL;
	if (image_container.image2D->pF.IsNotNull())
		image_container.image2D->pF->DisconnectPipeline();
	image_container.image2D->pF = NULL;
	if (image_container.image2D->pD.IsNotNull())
		image_container.image2D->pD->DisconnectPipeline();
	image_container.image2D->pD = NULL;
	if (image_container.image2D->pSLL.IsNotNull())
		image_container.image2D->pSLL->DisconnectPipeline();
	image_container.image2D->pSLL = NULL;
	if (image_container.image2D->pULL.IsNotNull())
		image_container.image2D->pULL->DisconnectPipeline();
	image_container.image2D->pULL = NULL;
	if (image_container.image2D->pSS_rgb.IsNotNull())
		image_container.image2D->pSS_rgb->DisconnectPipeline();
	image_container.image2D->pSS_rgb = NULL;
	if (image_container.image2D->pUS_rgb.IsNotNull())
		image_container.image2D->pUS_rgb->DisconnectPipeline();
	image_container.image2D->pUS_rgb = NULL;
	if (image_container.image2D->pSI_rgb.IsNotNull())
		image_container.image2D->pSI_rgb->DisconnectPipeline();
	image_container.image2D->pSI_rgb = NULL;
	if (image_container.image2D->pUI_rgb.IsNotNull())
		image_container.image2D->pUI_rgb->DisconnectPipeline();
	image_container.image2D->pUI_rgb = NULL;
	if (image_container.image2D->pUC_rgb.IsNotNull())
		image_container.image2D->pUC_rgb ->DisconnectPipeline();
	image_container.image2D->pUC_rgb = NULL;
	if (image_container.image2D->pF_rgb.IsNotNull())
		image_container.image2D->pF_rgb->DisconnectPipeline();
	image_container.image2D->pF_rgb = NULL;
	if (image_container.image2D->pD_rgb.IsNotNull())
		image_container.image2D->pD_rgb->DisconnectPipeline();
	image_container.image2D->pD_rgb = NULL;
	if (image_container.image2D->pSS_rgba.IsNotNull())
		image_container.image2D->pSS_rgba->DisconnectPipeline();
	image_container.image2D->pSS_rgba = NULL;
	if (image_container.image2D->pUS_rgba.IsNotNull())
		image_container.image2D->pUS_rgba->DisconnectPipeline();
	image_container.image2D->pUS_rgba = NULL;
	if (image_container.image2D->pSI_rgba.IsNotNull())
		image_container.image2D->pSI_rgba->DisconnectPipeline();
	image_container.image2D->pSI_rgba = NULL;
	if (image_container.image2D->pUI_rgba.IsNotNull())
		image_container.image2D->pUI_rgba->DisconnectPipeline();
	image_container.image2D->pUI_rgba = NULL;
	if (image_container.image2D->pUC_rgba.IsNotNull())
		image_container.image2D->pUC_rgba->DisconnectPipeline();
	image_container.image2D->pUC_rgba = NULL;
	if (image_container.image2D->pF_rgba.IsNotNull())
		image_container.image2D->pF_rgba->DisconnectPipeline();
	image_container.image2D->pF_rgba = NULL;
	if (image_container.image2D->pD_rgba.IsNotNull())
		image_container.image2D->pD_rgba->DisconnectPipeline();
	image_container.image2D->pD_rgba = NULL;
	//
	switch (image_container.image3D->image_type)
	{
	case 0: error_ = get_slice2_<ImageTypeSS, Image2DTypeSS>(
				image_container.image3D->pSS, image_container.image2D, image_container.image2D->pSS, x);
		break;
	case 1: error_ = get_slice2_<ImageTypeUS, Image2DTypeUS>(
				image_container.image3D->pUS, image_container.image2D, image_container.image2D->pUS, x);
		break;
	case 2: error_ = get_slice2_<ImageTypeSI, Image2DTypeSI>(
				image_container.image3D->pSI, image_container.image2D, image_container.image2D->pSI, x);
		break;
	case 3: error_ = get_slice2_<ImageTypeUI, Image2DTypeUI>(
				image_container.image3D->pUI, image_container.image2D, image_container.image2D->pUI, x);
		break;
	case 4: error_ = get_slice2_<ImageTypeUC, Image2DTypeUC>(
				image_container.image3D->pUC, image_container.image2D, image_container.image2D->pUC, x);
		break;
	case 5: error_ = get_slice2_<ImageTypeF, Image2DTypeF>(
				image_container.image3D->pF, image_container.image2D, image_container.image2D->pF, x);
		break;
	case 6: error_ = get_slice2_<ImageTypeD, Image2DTypeD>(
				image_container.image3D->pD, image_container.image2D, image_container.image2D->pD, x);
		break;
	case 7: error_ = get_slice2_<ImageTypeSLL, Image2DTypeSLL>(
				image_container.image3D->pSLL, image_container.image2D, image_container.image2D->pSLL, x);
		break;
	case 8: error_ = get_slice2_<ImageTypeULL, Image2DTypeULL>(
				image_container.image3D->pULL, image_container.image2D, image_container.image2D->pULL, x);
		break;
	case 10: error_ = get_slice2_<RGBImageTypeSS, RGBImage2DTypeSS>(
				image_container.image3D->pSS_rgb, image_container.image2D, image_container.image2D->pSS_rgb, x);
		break;
	case 11: error_ = get_slice2_<RGBImageTypeUS, RGBImage2DTypeUS>(
				image_container.image3D->pUS_rgb, image_container.image2D, image_container.image2D->pUS_rgb, x);
		break;
	case 12: error_ = get_slice2_<RGBImageTypeSI, RGBImage2DTypeSI>(
				image_container.image3D->pSI_rgb, image_container.image2D, image_container.image2D->pSI_rgb, x);
		break;
	case 13: error_ = get_slice2_<RGBImageTypeUI, RGBImage2DTypeUI>(
				image_container.image3D->pUI_rgb, image_container.image2D, image_container.image2D->pUI_rgb, x);
		break;
	case 14: error_ = get_slice2_<RGBImageTypeUC, RGBImage2DTypeUC>(
				image_container.image3D->pUC_rgb, image_container.image2D, image_container.image2D->pUC_rgb, x);
		break;
	case 15: error_ = get_slice2_<RGBImageTypeF, RGBImage2DTypeF>(
				image_container.image3D->pF_rgb, image_container.image2D, image_container.image2D->pF_rgb, x);
		break;
	case 16: error_ = get_slice2_<RGBImageTypeD, RGBImage2DTypeD>(
				image_container.image3D->pD_rgb, image_container.image2D, image_container.image2D->pD_rgb, x);
		break;
	case 20: error_ = get_slice2_<RGBAImageTypeSS, RGBAImage2DTypeSS>(
				image_container.image3D->pSS_rgba, image_container.image2D, image_container.image2D->pSS_rgba, x);
		break;
	case 21: error_ = get_slice2_<RGBAImageTypeUS, RGBAImage2DTypeUS>(
				image_container.image3D->pUS_rgba, image_container.image2D, image_container.image2D->pUS_rgba, x);
		break;
	case 22: error_ = get_slice2_<RGBAImageTypeSI, RGBAImage2DTypeSI>(
				image_container.image3D->pSI_rgba, image_container.image2D, image_container.image2D->pSI_rgba, x);
		break;
	case 23: error_ = get_slice2_<RGBAImageTypeUI, RGBAImage2DTypeUI>(
				image_container.image3D->pUI_rgba, image_container.image2D, image_container.image2D->pUI_rgba, x);
		break;
	case 24: error_ = get_slice2_<RGBAImageTypeUC, RGBAImage2DTypeUC>(
				image_container.image3D->pUC_rgba, image_container.image2D, image_container.image2D->pUC_rgba, x);
		break;
	case 25: error_ = get_slice2_<RGBAImageTypeF, RGBAImage2DTypeF>(
				image_container.image3D->pF_rgba, image_container.image2D, image_container.image2D->pF_rgba, x);
		break;
	case 26: error_ = get_slice2_<RGBAImageTypeD, RGBAImage2DTypeD>(
				image_container.image3D->pD_rgba, image_container.image2D, image_container.image2D->pD_rgba, x);
		break;
	default:
		{
			clear_(false);
			goto quit__;
		}
	}
	//
	if (!error_.isEmpty()) goto quit__;
	//
	{
		const bool check_consistence =
			((static_cast<int>(image_container.image3D->di->image_slices.size()) == image_container.image3D->di->idimz) &&
				static_cast<int>(image_container.image3D->di->image_slices.size()) > x);
		if (image_container.image3D->equi)
		{
			image_container.image2D->orientation_string = image_container.image3D->orientation_string;
		}
		else
		{
			if (check_consistence &&
				!image_container.image3D->di->image_slices.at(x)->slice_orientation_string.isEmpty())
			{
				image_container.image2D->orientation_string =
					image_container.image3D->di->image_slices.at(x)->slice_orientation_string;
			}
		}
	}
	if (image_container.image3D->orientations_20_20.contains(x))
	{
		const QString s2020 = image_container.image3D->orientations_20_20.value(x);
		if (!s2020.isEmpty())
		{
			image_container.orientation_20_20 = s2020;
		}
	}
	if (image_container.image3D->anatomy.contains(x))
	{
		image_container.image2D->laterality = image_container.image3D->anatomy.value(x).laterality;
		image_container.image2D->body_part  = image_container.image3D->anatomy.value(x).body_part;
	}
	//
	update_image(0, false);
	//
	graphicsview->draw_us_regions();
	if (!image_container.image3D->pr_display_areas.empty())
	{
		update_pr_area();
	}
	else
	{
		if (graphicsview->pr_area->isVisible()) graphicsview->pr_area->hide();
	}
	//
	if (studyview)
	{
		if (studyview->get_active_id() == image_container.image3D->id)
		{
			studyview->update_level(&image_container);
		}
		studyview->update_scouts();
	}
quit__:
	mutex.unlock();
}

void StudyGraphicsWidget::toggle_single(bool t)
{
	if (!studyview) return;
	if (t) studyview->set_single(widget_id);
	else   studyview->restore_multi(widget_id);
}

void StudyGraphicsWidget::set_active()
{
	if (studyview) studyview->set_active_image(&image_container);
}

void StudyGraphicsWidget::update_measurement(
	double x0,
	double y0,
	double x1,
	double y1)
{
	if (!image_container.image3D) return;
	const ImageVariant * ivariant = image_container.image3D;
	const unsigned int z = image_container.selected_z_slice_ext;
	QString tmp0("");
	if (!ivariant->usregions.empty())
	{
		QVector<int> ids;
		QVector<int> ids2;
		QVector<int> high_priority_regions;
		for (int x = 0; x < ivariant->usregions.size(); ++x)
		{
			if (x0 >= ivariant->usregions.at(x).m_X0 &&
				y0 >= ivariant->usregions.at(x).m_Y0 &&
				x0 <= ivariant->usregions.at(x).m_X1 &&
				y0 <= ivariant->usregions.at(x).m_Y1 &&
				x1 >= ivariant->usregions.at(x).m_X0 &&
				y1 >= ivariant->usregions.at(x).m_Y0 &&
				x1 <= ivariant->usregions.at(x).m_X1 &&
				y1 <= ivariant->usregions.at(x).m_Y1)
			{
				if (!ivariant->usregions.at(x).m_UnitXString.isEmpty()||
					!ivariant->usregions.at(x).m_UnitYString.isEmpty())
					ids.push_back(x);
				ids2.push_back(x);
			}
		}
		for (int x = 0; x < ids.size(); ++x)
		{
			if (ivariant->usregions.at(ids.at(x)).m_FlagsBool)
			{
				const unsigned int flags =
					ivariant->usregions.at(ids.at(x)).m_RegionFlags;
				if ((flags & 1) == 0)
				{
					high_priority_regions.push_back(ids.at(x));
				}
			}
		}
		//
		QString data_type("");
		for (int x = 0; x < ids2.size(); ++x)
		{
			if (!ivariant->usregions.at(
					ids2.at(x)).m_DataTypeString.isEmpty())
			{
				data_type.append(
					ivariant->usregions.at(ids2.at(x)).m_DataTypeString);
			}
			if (ids2.size() > 1 && x != ids2.size() - 1 && !data_type.isEmpty())
			{
				data_type.append(QString("/"));
			}
		}
		//
		if (!ids.empty())
		{
			bool tmp1x = false;
			bool tmp1y = false;
			const unsigned int id =
				(high_priority_regions.size() == 1)
				? high_priority_regions.at(0)
				: ids.at(0);
			const double dx =
				ivariant->usregions.at(id).m_PhysicalDeltaX;
			const double dy =
				ivariant->usregions.at(id).m_PhysicalDeltaY;
			const double x0_ = x0 * dx;
			const double y0_ = y0 * dy;
			const double x1_ = x1 * dx;
			const double y1_ = y1 * dy;
			const QString measure_textx =
				ivariant->usregions.at(id).m_UnitXString;
			const QString measure_texty =
				ivariant->usregions.at(id).m_UnitYString;
			const unsigned short spatial =
				ivariant->usregions.at(id).m_RegionSpatialFormat;
			QColor color(Qt::cyan);
			switch (spatial)
			{
			case 0x1:
				color = QColor(0x00, 0xff, 0x00); // 2D
				break;
			case 0x2:
				color = QColor(0xff, 0x00, 0x00); // M-Mode
				break;
			case 0x3:
				color = QColor(0x00, 0x00, 0xff); // Spectral
				break;
			case 0x4:
				color = QColor(0x00, 0xff, 0xff); // Waveform
				break;
			case 0x5:
				color = QColor(0xff, 0x00, 0xff); // Graphics
				break;
			case 0:
			default:
				break;
			}
			if (measure_textx == measure_texty)
			{
				const double d   = get_distance4(x0_, y0_, x1_, y1_);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				tmp0 = QString::asprintf("%.3f", d);
#else
				tmp0.sprintf("%.3f", d);
#endif
				tmp0.append(QString(" ") + measure_textx);
			}
			else
			{
				if (!measure_textx.isEmpty())
				{
					tmp1x = true;
					const double d0 = get_distance4(x0_, y0_, x1_, y0_);
					QString tmp0x;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp0x = QString::asprintf("%.3f", d0);
#else
					tmp0x.sprintf("%.3f", d0);
#endif
					tmp0.append(
						QString("X: ") +
						tmp0x +
						QString(" ") +
						measure_textx);
				}
				if (!measure_textx.isEmpty() && !measure_texty.isEmpty())
				{
					tmp0.append(QString(" | "));
				}
				if (!measure_texty.isEmpty())
				{
					tmp1y = true;
					const double d1 = get_distance4(x0_, y0_, x0_, y1_);
					QString tmp0y;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
					tmp0y = QString::asprintf("%.3f", d1);
#else
					tmp0y.sprintf("%.3f", d1);
#endif
					tmp0.append(
						QString("Y: ") +
						tmp0y +
						QString(" ") +
						measure_texty);
				}
			}
			//
			QPainterPath path;
			QPen pen;
			pen.setBrush(QBrush(color));
			pen.setWidth(0);
			graphicsview->measurment_line->setPen(pen);
			if (tmp1x||tmp1y)
			{
				if (tmp1x)
				{
					const double x0___ = x0;
					const double y0___ = y0;
					const double x1___ = x1;
					const double y1___ = y0;
					path.moveTo(x0___, y0___);
					path.lineTo(x1___, y1___);
				}
				if (tmp1y)
				{
					const double x0___ = x0;
					const double y0___ = y0;
					const double x1___ = x0;
					const double y1___ = y1;
					path.moveTo(x0___, y0___);
					path.lineTo(x1___, y1___);
				}
			}
			else
			{
				const double x0___ = x0;
				const double y0___ = y0;
				const double x1___ = x1;
				const double y1___ = y1;
				path.moveTo(x0___, y0___);
				path.lineTo(x1___, y1___);
			}
			graphicsview->measurment_line->setPath(path);
			if (!data_type.isEmpty() && !tmp0.isEmpty())
			{
				data_type.append(QString(" | "));
			}
			set_measure_text(data_type + tmp0);
		}
		else
		{
			QPainterPath path;
			graphicsview->measurment_line->setPath(path);
			set_measure_text(data_type);
		}
	}
	else
	{
		double d = -1;
		switch (ivariant->image_type)
		{
		case 0:
			d = get_distance3<ImageTypeSS>(ivariant->pSS, ivariant, x0, y0, x1, y1, z);
			break;
		case 1:
			d = get_distance3<ImageTypeUS>(ivariant->pUS, ivariant, x0, y0, x1, y1, z);
			break;
		case 2:
			d = get_distance3<ImageTypeSI>(ivariant->pSI, ivariant, x0, y0, x1, y1, z);
			break;
		case 3:
			d = get_distance3<ImageTypeUI>(ivariant->pUI, ivariant, x0, y0, x1, y1, z);
			break;
		case 4:
			d = get_distance3<ImageTypeUC>(ivariant->pUC, ivariant, x0, y0, x1, y1, z);
			break;
		case 5:
			d = get_distance3<ImageTypeF>(ivariant->pF, ivariant, x0, y0, x1, y1, z);
			break;
		case 6:
			d = get_distance3<ImageTypeD>(ivariant->pD, ivariant, x0, y0, x1, y1, z);
			break;
		case 7:
			d = get_distance3<ImageTypeSLL>(ivariant->pSLL, ivariant, x0, y0, x1, y1, z);
			break;
		case 8:
			d = get_distance3<ImageTypeULL>(ivariant->pULL, ivariant, x0, y0, x1, y1, z);
			break;
		case 10:
			d = get_distance3<RGBImageTypeSS>(ivariant->pSS_rgb, ivariant, x0, y0, x1, y1, z);
			break;
		case 11:
			d = get_distance3<RGBImageTypeUS>(ivariant->pUS_rgb, ivariant, x0, y0, x1, y1, z);
			break;
		case 12:
			d = get_distance3<RGBImageTypeSI>(ivariant->pSI_rgb, ivariant, x0, y0, x1, y1, z);
			break;
		case 13:
			d = get_distance3<RGBImageTypeUI>(ivariant->pUI_rgb, ivariant, x0, y0, x1, y1, z);
			break;
		case 14:
			d = get_distance3<RGBImageTypeUC>(ivariant->pUC_rgb, ivariant, x0, y0, x1, y1, z);
			break;
		case 15:
			d = get_distance3<RGBImageTypeF>(ivariant->pF_rgb, ivariant, x0, y0, x1, y1, z);
			break;
		case 16:
			d = get_distance3<RGBImageTypeD>(ivariant->pD_rgb, ivariant, x0, y0, x1, y1, z);
			break;
		case 20:
			d = get_distance3<RGBAImageTypeSS>(ivariant->pSS_rgba, ivariant, x0, y0, x1, y1, z);
			break;
		case 21:
			d = get_distance3<RGBAImageTypeUS>(ivariant->pUS_rgba, ivariant, x0, y0, x1, y1, z);
			break;
		case 22:
			d = get_distance3<RGBAImageTypeSI>(ivariant->pSI_rgba, ivariant, x0, y0, x1, y1, z);
			break;
		case 23:
			d = get_distance3<RGBAImageTypeUI>(ivariant->pUI_rgba, ivariant, x0, y0, x1, y1, z);
			break;
		case 24:
			d = get_distance3<RGBAImageTypeUC>(ivariant->pUC_rgba, ivariant, x0, y0, x1, y1, z);
			break;
		case 25:
			d = get_distance3<RGBAImageTypeF>(ivariant->pF_rgba, ivariant, x0, y0, x1, y1, z);
			break;
		case 26:
			d = get_distance3<RGBAImageTypeD>(ivariant->pD_rgba, ivariant, x0, y0, x1, y1, z);
			break;
		default:
			break;
		}
		if (d >= 0.0)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
			tmp0 = QString::asprintf("%.3f", d);
#else
			tmp0.sprintf("%.3f", d);
#endif
			tmp0.append(QString(" ") + ivariant->unit_str);
			QPainterPath path;
			QPen pen;
			pen.setBrush(QBrush(Qt::cyan));
			pen.setWidth(0);
			graphicsview->measurment_line->setPen(pen);
			path.moveTo(x0, y0);
			path.lineTo(x1, y1);
			graphicsview->measurment_line->setPath(path);
		}
		set_measure_text(tmp0);
	}
}

