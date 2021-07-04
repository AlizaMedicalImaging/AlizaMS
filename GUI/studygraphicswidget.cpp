#include "studygraphicswidget.h"
#include "studyviewwidget.h"
#include <QtGlobal>
#include <QVBoxLayout>
#include <QImage>
#include <QGraphicsPixmapItem>
#include "itkExtractImageFilter.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "processimagethreadLUT.hxx"
#include "graphicsutils.h"
#include "commonutils.h"
#include "updateqtcommand.h"
#include <limits>
#ifndef WIN32
#include <unistd.h>
#endif

void gImageCleanupHandler2(void * info)
{
	if (!info) return;
	unsigned char * p = (unsigned char*)info;
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
		return QString("get_slice2_<>() : image.IsNull()");
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
	}
	catch (itk::ExceptionObject & ex)
	{ return QString(ex.GetDescription()); }
	out_image = filter->GetOutput();
	if (out_image.IsNotNull()) out_image->DisconnectPipeline();
	else return QString("Out image is NULL");
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
	widget->graphicsview->image_item->setPos(0,0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#else
	if (!widget->graphicsview->image_item) return;
#endif
	const typename T::RegionType region = image->GetLargestPossibleRegion();
	const typename T::SizeType   size   = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	unsigned char * p__  = NULL;
	unsigned int j_ = 0;
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	//
	QString top_string = QString(""), left_string = QString("");
	bool flip_x = false, flip_y = false;
	double scale__;
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	if (spacing[0]!=spacing[1])
	{
		if (spacing[1]>spacing[0]) coeff_size_1 = spacing[1]/spacing[0];
		else                       coeff_size_0 = spacing[0]/spacing[1];
	}
	const double xratio =
		(double)widget->graphicsview->width()  / (double)(size[0]*coeff_size_0);
	const double yratio =
		(double)widget->graphicsview->height() / (double)(size[1]*coeff_size_1);
	if (fit==1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else scale__ = widget->graphicsview->m_scale;
	//
	const unsigned short bits_allocated   = ivariant->di->bits_allocated;
	const unsigned short bits_stored      = ivariant->di->bits_stored;
	const unsigned short high_bit         = ivariant->di->high_bit;
	const bool           hide_orientation = ivariant->di->hide_orientation;
	//
	if (image_type == 11)
	{
		const double tmp_max
			= ((bits_allocated > 0 && bits_stored > 0) &&
				bits_stored < bits_allocated &&
				(high_bit==bits_stored-1))
				? pow(2, bits_stored) - 1 : static_cast<double>(USHRT_MAX);
		try { p__ = new unsigned char[size[0] * size[1] * 3]; }
		catch (std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				p__[j_ + 2] =
					static_cast<unsigned char>(
						((double)iterator.Get().GetBlue()  / tmp_max) * 255.0);
				p__[j_ + 1] =
					static_cast<unsigned char>(
						((double)iterator.Get().GetGreen() / tmp_max) * 255.0);
				p__[j_ + 0] =
					static_cast<unsigned char>(
						((double)iterator.Get().GetRed()   / tmp_max) * 255.0);
				j_ += 3;
				++iterator;
			}
		}
		catch(itk::ExceptionObject &)
		{
			return;
		}
	}
	else
	{
		try { p__ = new unsigned char[size[0]*size[1]*3]; }
		catch(std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (vrange!=0)
		{
			try
			{
				itk::ImageRegionConstIterator<T> iterator(image, region);
				iterator.GoToBegin();
				while(!iterator.IsAtEnd())
				{
					const double b = static_cast<double>(iterator.Get().GetBlue());
					const double g = static_cast<double>(iterator.Get().GetGreen());
					const double r = static_cast<double>(iterator.Get().GetRed());
					p__[j_+2] = static_cast<unsigned char>(255.0*((b+(-vmin))/vrange));
					p__[j_+1] = static_cast<unsigned char>(255.0*((g+(-vmin))/vrange));
					p__[j_+0] = static_cast<unsigned char>(255.0*((r+(-vmin))/vrange));
					j_ += 3;
 					++iterator;
				}
			}
			catch(itk::ExceptionObject &)
			{
				return;
			}
		}
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QImage tmpi(p__,size[0],size[1],3*size[0],QImage::Format_RGB888,gImageCleanupHandler2,p__);
#else
	QImage tmpi(p__,size[0],size[1],3*size[0],QImage::Format_RGB888);
#endif
	//
	if (widget->get_enable_overlays())
		GraphicsUtils::draw_overlays(ivariant, tmpi);
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
	if (spacing[1]!=spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	if (flip_y && flip_x) t = t.scale(-1.0,-1.0);
	else if (flip_y)      t = t.scale( 1.0,-1.0);
	else if (flip_x)      t = t.scale(-1.0, 1.0);
	else ;;
	if (global_flip_x) { t = t.scale(-1.0, 1.0); }
	if (global_flip_y) { t = t.scale( 1.0,-1.0); }
	//
	const QRectF rectf(0,0,size[0],size[1]);
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
	widget->graphicsview->image_item->setPos(0,0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#else
	if (!widget->graphicsview->image_item) return;
#endif
	const typename T::RegionType region = image->GetLargestPossibleRegion();
	const typename T::SizeType   size   = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	unsigned char * p__   = NULL;
	unsigned long long j_ = 0;
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	//
	QString top_string = QString(""), left_string = QString("");
	bool flip_x = false, flip_y = false;
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	double scale__;
	if (spacing[0]!=spacing[1])
	{
		if (spacing[1]>spacing[0]) coeff_size_1 = spacing[1]/spacing[0];
		else                       coeff_size_0 = spacing[0]/spacing[1];
	}
	const double xratio =
		(double)widget->graphicsview->width()  / (double)(size[0]*coeff_size_0);
	const double yratio =
		(double)widget->graphicsview->height() / (double)(size[1]*coeff_size_1);
	if (fit==1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else scale__ = widget->graphicsview->m_scale;
	//
	const unsigned short bits_allocated   = ivariant->di->bits_allocated;
	const unsigned short bits_stored      = ivariant->di->bits_stored;
	const unsigned short high_bit         = ivariant->di->high_bit;
	const bool           hide_orientation = ivariant->di->hide_orientation;
	//
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	if (image_type == 21)
	{
		const double tmp_max
			= ((bits_allocated > 0 && bits_stored > 0) &&
				bits_stored < bits_allocated &&
				(high_bit==bits_stored-1))
				? pow(2, bits_stored) - 1 : static_cast<double>(USHRT_MAX);
		try { p__ = new unsigned char[size[0] * size[1] * 4]; }
		catch (std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				p__[j_ + 3] =
					static_cast<unsigned char>(
						((double)iterator.Get().GetAlpha() / tmp_max) * 255.0);
				p__[j_ + 2] =
					static_cast<unsigned char>(
						((double)iterator.Get().GetBlue()  / tmp_max) * 255.0);
				p__[j_ + 1] =
					static_cast<unsigned char>(
						((double)iterator.Get().GetGreen() / tmp_max) * 255.0);
				p__[j_ + 0] =
					static_cast<unsigned char>(
						((double)iterator.Get().GetRed()   / tmp_max) * 255.0);
				j_ += 4;
				++iterator;
			}
		}
		catch(itk::ExceptionObject &)
		{
			return;
		}
	}
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (!(vrange != 0)) return;
		try { p__ = new unsigned char[size[0]*size[1]*4]; }
		catch(std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while(!iterator.IsAtEnd())
			{
				const double a = static_cast<double>(iterator.Get().GetAlpha());
				const double b = static_cast<double>(iterator.Get().GetBlue());
				const double g = static_cast<double>(iterator.Get().GetGreen());
				const double r = static_cast<double>(iterator.Get().GetRed());
				p__[j_+3] = static_cast<unsigned char>(255.0*((a+(-vmin))/vrange));
				p__[j_+2] = static_cast<unsigned char>(255.0*((b+(-vmin))/vrange));
				p__[j_+1] = static_cast<unsigned char>(255.0*((g+(-vmin))/vrange));
				p__[j_+0] = static_cast<unsigned char>(255.0*((r+(-vmin))/vrange));
				j_ += 4;
 				++iterator;
			}
		}
		catch(itk::ExceptionObject &)
		{
			return;
		}
	}
	QImage tmpi(p__,size[0],size[1],4*size[0],QImage::Format_RGBA8888,gImageCleanupHandler2,p__);
#else
	if (image_type == 21)
	{
		const double tmp_max
			= ((bits_allocated > 0 && bits_stored > 0) &&
				bits_stored < bits_allocated &&
				(high_bit==bits_stored-1))
				? pow(2, bits_stored) - 1 : static_cast<double>(USHRT_MAX);
		try { p__ = new unsigned char[size[0] * size[1] * 3]; }
		catch (std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				if (iterator.Get().GetAlpha()>0)
				{
					const double alpha = static_cast<double>(iterator.Get().GetAlpha())/tmp_max;
					const double one_minus_alpha = 1.0 - alpha;
					const double tmp_whi = one_minus_alpha*USHRT_MAX;
					const double tmp_red =
						tmp_whi + alpha*static_cast<double>(iterator.Get().GetRed());
					const double tmp_gre =
						tmp_whi + alpha*static_cast<double>(iterator.Get().GetGreen());
					const double tmp_blu =
						tmp_whi + alpha*static_cast<double>(iterator.Get().GetBlue());
					p__[j_ + 2] = static_cast<unsigned char>((tmp_blu/tmp_max) * 255.0);
					p__[j_ + 1] = static_cast<unsigned char>((tmp_gre/tmp_max) * 255.0);
					p__[j_ + 0] = static_cast<unsigned char>((tmp_red/tmp_max) * 255.0);
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
		catch(itk::ExceptionObject &)
		{
			return;
		}
	}
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (!(vrange!=0)) return;
		try { p__ = new unsigned char[size[0]*size[1]*3]; }
		catch(std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
		try
		{
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while(!iterator.IsAtEnd())
			{
				const double a = static_cast<double>(iterator.Get().GetAlpha());
				const double b = static_cast<double>(iterator.Get().GetBlue());
				const double g = static_cast<double>(iterator.Get().GetGreen());
				const double r = static_cast<double>(iterator.Get().GetRed());
				const double one_minus_alpha = 1.0 - ((a+(-vmin))/vrange);
				const double tmp_whi = one_minus_alpha * 255.0;
				const double tmp_red = tmp_whi + a*(255.0*((r+(-vmin))/vrange));
				const double tmp_gre = tmp_whi + a*(255.0*((g+(-vmin))/vrange));
				const double tmp_blu = tmp_whi + a*(255.0*((b+(-vmin))/vrange));
				p__[j_+2] = static_cast<unsigned char>(tmp_blu);
				p__[j_+1] = static_cast<unsigned char>(tmp_gre);
				p__[j_+0] = static_cast<unsigned char>(tmp_red);
				j_ += 3;
 				++iterator;
			}
		}
		catch(itk::ExceptionObject &)
		{
			return;
		}
	}
	QImage tmpi(p__,size[0],size[1],3*size[0],QImage::Format_RGB888);
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
	if (spacing[1]!=spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	if (flip_y && flip_x) t = t.scale(-1.0,-1.0);
	else if (flip_y)      t = t.scale( 1.0,-1.0);
	else if (flip_x)      t = t.scale(-1.0, 1.0);
	else ;;
	if (global_flip_x) { t = t.scale(-1.0, 1.0); }
	if (global_flip_y) { t = t.scale( 1.0,-1.0); }
	//
	const QRectF rectf(0,0,size[0],size[1]);
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
	//
	widget->graphicsview->image_item = new QGraphicsPixmapItem();
	widget->graphicsview->image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (widget->get_smooth())
		widget->graphicsview->image_item->setTransformationMode(Qt::SmoothTransformation);
	else
		widget->graphicsview->image_item->setTransformationMode(Qt::FastTransformation);
	widget->graphicsview->image_item->setCacheMode(QGraphicsItem::NoCache);
	widget->graphicsview->image_item->setPos(0,0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#else
	if (!widget->graphicsview->image_item) return;
#endif
	const typename T::RegionType  region  = image->GetLargestPossibleRegion();
	const typename T::SizeType    size    = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	unsigned char * p   = NULL;
	QString top_string = QString(""), left_string = QString("");
	bool flip_x = false, flip_y = false;
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	double scale__;
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	if (image_type == 14)
	{
		try { p = reinterpret_cast<unsigned char *>(image->GetBufferPointer()); }
		catch (itk::ExceptionObject & ex) { std::cout << ex << std::endl; return; }
	}
	else
	{
		return;
	}
	if (!p) return;
	//
	if (spacing[0] != spacing[1])
	{
		if (spacing[1] > spacing[0]) coeff_size_1 = spacing[1]/spacing[0];
		else                         coeff_size_0 = spacing[0]/spacing[1];
	}
	const double xratio =
		(double)widget->graphicsview->width()  / (double)(size[0]*coeff_size_0);
	const double yratio =
		(double)widget->graphicsview->height() / (double)(size[1]*coeff_size_1);
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
	QImage tmpi = QImage(p,size[0],size[1],3*size[0],QImage::Format_RGB888);
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
	if (flip_y && flip_x) t = t.scale(-1.0,-1.0);
	else if (flip_y)      t = t.scale( 1.0,-1.0);
	else if (flip_x)      t = t.scale(-1.0, 1.0);
	else ;;
	if (global_flip_x) { t = t.scale(-1.0, 1.0); }
	if (global_flip_y) { t = t.scale( 1.0,-1.0); }
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
	//
	widget->graphicsview->image_item = new QGraphicsPixmapItem();
	widget->graphicsview->image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (widget->get_smooth())
		widget->graphicsview->image_item->setTransformationMode(Qt::SmoothTransformation);
	else
		widget->graphicsview->image_item->setTransformationMode(Qt::FastTransformation);
	widget->graphicsview->image_item->setCacheMode(QGraphicsItem::NoCache);
	widget->graphicsview->image_item->setPos(0,0);
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
	if (spacing[0]!=spacing[1])
	{
		if (spacing[1]>spacing[0]) coeff_size_1 = spacing[1]/spacing[0];
		else                       coeff_size_0 = spacing[0]/spacing[1];
	}
	const double xratio =
		(double)widget->graphicsview->width()  / (double)(size[0]*coeff_size_0);
	const double yratio =
		(double)widget->graphicsview->height() / (double)(size[1]*coeff_size_1);
	if (fit==1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else scale__ = widget->graphicsview->m_scale;
	//
	unsigned char * p = NULL;
	QImage tmpi;
	if (image_type == 24)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		try { p = reinterpret_cast<unsigned char *>(image->GetBufferPointer()); }
		catch (itk::ExceptionObject & ex) { std::cout << ex << std::endl; return; }
		if (!p) return;
		tmpi = QImage(p,size[0],size[1],4*size[0],QImage::Format_RGBA8888);
#else
		try { p = new unsigned char[size[0] * size[1] * 3]; }
		catch (std::bad_alloc&) { p = NULL; }
		if (!p) return;
		try
		{
			unsigned long long j_ = 0;
			itk::ImageRegionConstIterator<T> iterator(image, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				if (iterator.Get().GetAlpha()>0)
				{
					const double alpha =
						static_cast<double>(iterator.Get().GetAlpha())/255.0;
					const double one_minus_alpha = 1.0 - alpha;
					const double tmp_whi = one_minus_alpha * 255.0;
					const double tmp_red =
						tmp_whi + alpha*static_cast<double>(iterator.Get().GetRed());
					const double tmp_gre =
						tmp_whi + alpha*static_cast<double>(iterator.Get().GetGreen());
					const double tmp_blu =
						tmp_whi + alpha*static_cast<double>(iterator.Get().GetBlue());
					p[j_+2] = static_cast<unsigned char>(tmp_blu);
					p[j_+1] = static_cast<unsigned char>(tmp_gre);
					p[j_+0] = static_cast<unsigned char>(tmp_red);
				}
				else
				{
					p[j_+2] = 255;
					p[j_+1] = 255;
					p[j_+0] = 255;
				}
				j_ += 3;
				++iterator;
			}
		}
		catch(itk::ExceptionObject &)
		{
			return;
		}
		tmpi = QImage(p,size[0],size[1],3*size[0],QImage::Format_RGB888);
#endif
	}
	else return;
	//
	if (widget->get_enable_overlays())
		GraphicsUtils::draw_overlays(ivariant, tmpi);
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
	if (spacing[1]!=spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
	t = t.scale(scale__, scale__);
	if (flip_y && flip_x) t = t.scale(-1.0,-1.0);
	else if (flip_y)      t = t.scale( 1.0,-1.0);
	else if (flip_x)      t = t.scale(-1.0, 1.0);
	else ;;
	if (global_flip_x) { t = t.scale(-1.0, 1.0); }
	if (global_flip_y) { t = t.scale( 1.0,-1.0); }
	//
	const QRectF rectf(0,0,size[0],size[1]);
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
	const unsigned int p_size = 3*size[0]*size[1];
	unsigned char * p = NULL;
	try { p = new unsigned char[p_size]; }
	catch (std::bad_alloc&) { p = NULL; }
	if (!p) return;
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
	const int num_threads = QThread::idealThreadCount();
	const int tmp99 = size[1]%num_threads;
	if (!widget->threadsLUT_.empty())
	{
		std::cout << "load_image2<>() : widget->threadsLUT_.size()>0" << std::endl;
	}
	if (tmp99 == 0)
	{
		int j = 0;
		for (int i = 0; i < num_threads; ++i)
		{
			const int size_0 = size[0];
			const int size_1 = size[1]/num_threads;
			const int index_0 = 0;
			const int index_1 = i*size_1;
			ProcessImageThreadLUT_<T> * t__ = new ProcessImageThreadLUT_<T>(
						image,
						p,
						size_0,  size_1,
						index_0, index_1, j,
						window_center, window_width,
						lut, false, lut_function);
			j += 3*size_0*size_1;
			widget->threadsLUT_.push_back(static_cast<QThread*>(t__));
			t__->start();
		}
	}
	else
	{
		int j = 0;
		unsigned int block = 64;
		if (static_cast<float>(size[1])/static_cast<float>(block)>16.0f) block=128;
		const int tmp100 = size[1]%block;
		const int incr = (int)floor(size[1]/(double)block);
		if (size[1] > block)
		{
			for (int i = 0; i < incr; ++i)
			{
				const int size_0 = size[0];
				const int index_0 = 0;
				const int index_1 = i*block;
				ProcessImageThreadLUT_<T> * t__ = new ProcessImageThreadLUT_<T>(
							image,
							p,
							size_0,  block,
							index_0, index_1, j,
							window_center, window_width,
							lut, false,lut_function);
				j += 3*size_0*block;
				widget->threadsLUT_.push_back(static_cast<QThread*>(t__));
				t__->start();
			}
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(
						image,
						p,
						size[0],  tmp100,
						0, incr*block, j,
						window_center, window_width,
						lut, false,lut_function);
			widget->threadsLUT_.push_back(static_cast<QThread*>(lt__));
			lt__->start();
		}
		else
		{
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(
						image,
						p,
						size[0],  size[1],
						0, 0, 0,
						window_center, window_width,
						lut, false,lut_function);
			widget->threadsLUT_.push_back(static_cast<QThread*>(lt__));
			lt__->start();
		}
	}
	//
#if 0
#ifdef _WIN32
	Sleep(1);
#else
	usleep(1000);
#endif
#endif
	//
	const unsigned short threadsLUT_size = widget->threadsLUT_.size();
	while (true)
	{
		unsigned short b__ = 0;
		for (int i = 0; i < threadsLUT_size; ++i)
		{
			if (widget->threadsLUT_.at(i)->isFinished()) { ++b__; }
		}
		if (b__ == threadsLUT_size) break;
	}
	for (int i = 0; i < threadsLUT_size; ++i)
	{
		delete widget->threadsLUT_[i];
		widget->threadsLUT_[i] = NULL;
	}
	widget->threadsLUT_.clear();
	//
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	const QRectF rectf(0,0,size[0],size[1]);
	double scale__;
	if (spacing[0] != spacing[1])
	{
		if (spacing[1] > spacing[0]) coeff_size_1 = spacing[1]/spacing[0];
		else                         coeff_size_0 = spacing[0]/spacing[1];
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
	widget->graphicsview->image_item->setPos(0,0);
	widget->graphicsview->image_item->setZValue(-1.0);
	widget->graphicsview->scene()->addItem(widget->graphicsview->image_item);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QImage tmpi(p,size[0],size[1],3*size[0],QImage::Format_RGB888,gImageCleanupHandler2,p);
#else
	QImage tmpi(p,size[0],size[1],3*size[0],QImage::Format_RGB888);
#endif
	//
	if (widget->get_enable_overlays())
		GraphicsUtils::draw_overlays(ivariant, tmpi);
	//
	const double xratio = (double)widget->graphicsview->width()  / (double)(size[0]*coeff_size_0);
	const double yratio = (double)widget->graphicsview->height() / (double)(size[1]*coeff_size_1);
	if (fit==1)
	{
		scale__ = qMin(xratio, yratio);
		widget->graphicsview->m_scale = scale__;
	}
	else scale__ = widget->graphicsview->m_scale;
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	QTransform t = QTransform();
	if (spacing[1]!=spacing[0]) t = t.scale(coeff_size_0, coeff_size_1);
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
	if (flip_y && flip_x) t = t.scale(-1.0,-1.0);
	else if (flip_y)      t = t.scale( 1.0,-1.0);
	else if (flip_x)      t = t.scale(-1.0, 1.0);
	else ;;
	if (global_flip_x) { t = t.scale(-1.0, 1.0); }
	if (global_flip_y) { t = t.scale( 1.0,-1.0); }
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

StudyGraphicsWidget::StudyGraphicsWidget()
{
	studyview = NULL;
	slider = NULL;
	top_label = NULL;
	left_label = NULL;
	icon_label = NULL;
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
	l->setContentsMargins(0,0,0,0);
	setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Expanding);
}

StudyGraphicsWidget::~StudyGraphicsWidget()
{
	if (mutex.tryLock(30000))
	{
		for (unsigned int i=0; i<threads_.size(); ++i)
		{
			if (threads_.at(i))
			{
				if (threads_.at(i)->isRunning()) threads_[i]->exit();
				delete threads_[i];
				threads_[i] = NULL;
			}
		}
		for (unsigned int i=0; i<threadsLUT_.size(); ++i)
		{
			if (threadsLUT_.at(i))
			{
				if (threadsLUT_.at(i)->isRunning())
					threadsLUT_[i]->exit();
				delete threadsLUT_[i];
				threadsLUT_[i] = NULL;
			}
		}
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

void StudyGraphicsWidget::set_icon_label(QLabel * l)
{
	icon_label = l;
}

void StudyGraphicsWidget::set_top_string(const QString & s)
{
	if (top_label) top_label->setText(s);
}

void StudyGraphicsWidget::set_left_string(const QString & s)
{
	if (left_label) left_label->setText(s);
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

void StudyGraphicsWidget::update_image_color(int id, int r, int g, int b)
{
	// TODO
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
	switch(image_container.image2D->image_type)
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
		if (graphicsview->scene())
			graphicsview->scene()->removeItem(graphicsview->image_item);
		delete graphicsview->image_item;
		graphicsview->image_item = NULL;
#else
		QPixmap p(16,16);
		graphicsview->image_item->setPixmap(p);
#endif
	}
	if (slider)
	{
		disconnect(slider, SIGNAL(valueChanged(int)), this, SLOT(set_selected_slice(int)));
		slider->setValue(0);
		slider->setMaximum(1);
	}
	if (top_label)
	{
		top_label->setText(QString(""));
	}
	if (left_label)
	{
		left_label->setText(QString(""));
	}
	if (icon_label)
	{
		icon_label->setPixmap(QPixmap());
	}
	graphicsview->clear_collision_paths();
	graphicsview->pr_area->hide();
	if (image_container.image2D)
	{
		image_container.image2D->image_type=-1;
		image_container.image2D->orientation_string = QString("");
		image_container.image2D->laterality = QString("");
		image_container.image2D->body_part  = QString("");
		image_container.image2D->idimx = 0;
		image_container.image2D->idimy = 0;
		if (image_container.image2D->pSS.IsNotNull())
		{
			image_container.image2D->pSS->DisconnectPipeline();
			image_container.image2D->pSS =NULL;
		}
		if (image_container.image2D->pUS.IsNotNull())
		{
			image_container.image2D->pUS->DisconnectPipeline();
			image_container.image2D->pUS =NULL;
		}
		if (image_container.image2D->pSI.IsNotNull())
		{
			image_container.image2D->pSI->DisconnectPipeline();
			image_container.image2D->pSI =NULL;
		}
		if (image_container.image2D->pUI.IsNotNull())
		{
			image_container.image2D->pUI->DisconnectPipeline();
			image_container.image2D->pUI =NULL;
		}
		if (image_container.image2D->pUC.IsNotNull())
		{
			image_container.image2D->pUC->DisconnectPipeline();
			image_container.image2D->pUC =NULL;
		}
		if (image_container.image2D->pF.IsNotNull())
		{
			image_container.image2D->pF->DisconnectPipeline();
			image_container.image2D->pF =NULL;
		}
		if (image_container.image2D->pD.IsNotNull())
		{
			image_container.image2D->pD->DisconnectPipeline();
			image_container.image2D->pD =NULL;
		}
		if (image_container.image2D->pSLL.IsNotNull())
		{
			image_container.image2D->pSLL->DisconnectPipeline();
			image_container.image2D->pSLL =NULL;
		}
		if (image_container.image2D->pULL.IsNotNull())
		{
			image_container.image2D->pULL->DisconnectPipeline();
			image_container.image2D->pULL =NULL;
		}
		if (image_container.image2D->pSS_rgb.IsNotNull())
		{
			image_container.image2D->pSS_rgb->DisconnectPipeline();
			image_container.image2D->pSS_rgb =NULL;
		}
		if (image_container.image2D->pUS_rgb.IsNotNull())
		{
			image_container.image2D->pUS_rgb->DisconnectPipeline();
			image_container.image2D->pUS_rgb =NULL;
		}
		if (image_container.image2D->pSI_rgb.IsNotNull())
		{
			image_container.image2D->pSI_rgb->DisconnectPipeline();
			image_container.image2D->pSI_rgb =NULL;
		}
		if (image_container.image2D->pUI_rgb.IsNotNull())
		{
			image_container.image2D->pUI_rgb->DisconnectPipeline();
			image_container.image2D->pUI_rgb =NULL;
		}
		if (image_container.image2D->pUC_rgb.IsNotNull())
		{
			image_container.image2D->pUC_rgb->DisconnectPipeline();
			image_container.image2D->pUC_rgb =NULL;
		}
		if (image_container.image2D->pF_rgb .IsNotNull())
		{
			image_container.image2D->pF_rgb->DisconnectPipeline();
			image_container.image2D->pF_rgb =NULL;
		}
		if (image_container.image2D->pD_rgb.IsNotNull())
		{
			image_container.image2D->pD_rgb->DisconnectPipeline();
			image_container.image2D->pD_rgb =NULL;
		}
		if (image_container.image2D->pSS_rgba.IsNotNull())
		{
			image_container.image2D->pSS_rgba->DisconnectPipeline();
			image_container.image2D->pSS_rgba =NULL;
		}
		if (image_container.image2D->pUS_rgba.IsNotNull())
		{
			image_container.image2D->pUS_rgba->DisconnectPipeline();
			image_container.image2D->pUS_rgba =NULL;
		}
		if (image_container.image2D->pSI_rgba.IsNotNull())
		{
			image_container.image2D->pSI_rgba->DisconnectPipeline();
			image_container.image2D->pSI_rgba =NULL;
		}
		if (image_container.image2D->pUI_rgba.IsNotNull())
		{
			image_container.image2D->pUI_rgba->DisconnectPipeline();
			image_container.image2D->pUI_rgba =NULL;
		}
		if (image_container.image2D->pUC_rgba.IsNotNull())
		{
			image_container.image2D->pUC_rgba->DisconnectPipeline();
			image_container.image2D->pUC_rgba =NULL;
		}
		if (image_container.image2D->pF_rgba.IsNotNull())
		{
			image_container.image2D->pF_rgba->DisconnectPipeline();
			image_container.image2D->pF_rgba =NULL;
		}
		if (image_container.image2D->pD_rgba.IsNotNull())
		{
			image_container.image2D->pD_rgba->DisconnectPipeline();
			image_container.image2D->pD_rgba =NULL;
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
	const bool alw_usregs)
{
	if (graphicsview->image_item)
	{
#ifdef DELETE_STUDYGRAPHICSIMAGEITEM
		graphicsview->scene()->removeItem(graphicsview->image_item);
		delete graphicsview->image_item;
		graphicsview->image_item = NULL;
#else
		QPixmap p(16,16);
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
	image_container.orientation_20_20 = QString("");
	//
	if (!v)
	{
		image_container.selected_z_slice_ext=-1;
		image_container.us_window_center_ext=0.0;
		image_container.us_window_width_ext=1e-6;
		image_container.selected_lut_ext=0;
		image_container.lut_function_ext=0;
		image_container.level_locked_ext=true;
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
	image_container.image2D->image_type=-1;
	image_container.image2D->orientation_string=QString("");
	image_container.image2D->laterality=QString("");
	image_container.image2D->body_part=QString("");
	image_container.image2D->idimx=0;
	image_container.image2D->idimy=0;
	if (image_container.image2D->pSS.IsNotNull())
		image_container.image2D->pSS->DisconnectPipeline();
	image_container.image2D->pSS=NULL;
	if (image_container.image2D->pUS.IsNotNull())
		image_container.image2D->pUS->DisconnectPipeline();
	image_container.image2D->pUS=NULL;
	if (image_container.image2D->pSI.IsNotNull())
		image_container.image2D->pSI->DisconnectPipeline();
	image_container.image2D->pSI=NULL;
	if (image_container.image2D->pUI.IsNotNull())
		image_container.image2D->pUI->DisconnectPipeline();
	image_container.image2D->pUI=NULL;
	if (image_container.image2D->pUC.IsNotNull())
		image_container.image2D->pUC->DisconnectPipeline();
	image_container.image2D->pUC=NULL;
	if (image_container.image2D->pF.IsNotNull())
		image_container.image2D->pF->DisconnectPipeline();
	image_container.image2D->pF=NULL;
	if (image_container.image2D->pD.IsNotNull())
		image_container.image2D->pD->DisconnectPipeline();
	image_container.image2D->pD=NULL;
	if (image_container.image2D->pSLL.IsNotNull())
		image_container.image2D->pSLL->DisconnectPipeline();
	image_container.image2D->pSLL=NULL;
	if (image_container.image2D->pULL.IsNotNull())
		image_container.image2D->pULL->DisconnectPipeline();
	image_container.image2D->pULL=NULL;
	if (image_container.image2D->pSS_rgb.IsNotNull())
		image_container.image2D->pSS_rgb->DisconnectPipeline();
	image_container.image2D->pSS_rgb=NULL;
	if (image_container.image2D->pUS_rgb.IsNotNull())
		image_container.image2D->pUS_rgb->DisconnectPipeline();
	image_container.image2D->pUS_rgb=NULL;
	if (image_container.image2D->pSI_rgb.IsNotNull())
		image_container.image2D->pSI_rgb->DisconnectPipeline();
	image_container.image2D->pSI_rgb=NULL;
	if (image_container.image2D->pUI_rgb.IsNotNull())
		image_container.image2D->pUI_rgb->DisconnectPipeline();
	image_container.image2D->pUI_rgb=NULL;
	if (image_container.image2D->pUC_rgb.IsNotNull())
		image_container.image2D->pUC_rgb ->DisconnectPipeline();
	image_container.image2D->pUC_rgb=NULL;
	if (image_container.image2D->pF_rgb.IsNotNull())
		image_container.image2D->pF_rgb->DisconnectPipeline();
	image_container.image2D->pF_rgb=NULL;
	if (image_container.image2D->pD_rgb.IsNotNull())
		image_container.image2D->pD_rgb->DisconnectPipeline();
	image_container.image2D->pD_rgb=NULL;
	if (image_container.image2D->pSS_rgba.IsNotNull())
		image_container.image2D->pSS_rgba->DisconnectPipeline();
	image_container.image2D->pSS_rgba=NULL;
	if (image_container.image2D->pUS_rgba.IsNotNull())
		image_container.image2D->pUS_rgba->DisconnectPipeline();
	image_container.image2D->pUS_rgba=NULL;
	if (image_container.image2D->pSI_rgba.IsNotNull())
		image_container.image2D->pSI_rgba->DisconnectPipeline();
	image_container.image2D->pSI_rgba=NULL;
	if (image_container.image2D->pUI_rgba.IsNotNull())
		image_container.image2D->pUI_rgba->DisconnectPipeline();
	image_container.image2D->pUI_rgba=NULL;
	if (image_container.image2D->pUC_rgba.IsNotNull())
		image_container.image2D->pUC_rgba->DisconnectPipeline();
	image_container.image2D->pUC_rgba=NULL;
	if (image_container.image2D->pF_rgba.IsNotNull())
		image_container.image2D->pF_rgba->DisconnectPipeline();
	image_container.image2D->pF_rgba=NULL;
	if (image_container.image2D->pD_rgba.IsNotNull())
		image_container.image2D->pD_rgba->DisconnectPipeline();
	image_container.image2D->pD_rgba=NULL;
	//
	x = image_container.image3D->di->idimz/2;
	image_container.selected_z_slice_ext = x;
	image_container.us_window_center_ext = v->di->us_window_center;
	image_container.us_window_width_ext = v->di->us_window_width;
	image_container.selected_lut_ext = v->di->selected_lut;
	image_container.lut_function_ext = v->di->lut_function;
	image_container.level_locked_ext = v->di->lock_level2D;
	//
	switch(v->image_type)
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
	else { goto quit__; }
	//
	{
		const bool check_consistence =
			(((int)v->di->image_slices.size() == v->di->idimz) &&
				(int)v->di->image_slices.size() > x);
		if (v->equi)
		{
			image_container.image2D->orientation_string = v->orientation_string;
		}
		else
		{
			if (
				check_consistence &&
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
	}
	if (icon_label)
	{
		QPixmap pp(16, 16);
		QColor c((int)(v->di->R*255.0f),(int)(v->di->G*255.0f),(int)(v->di->B*255.0f));
		pp.fill(c);
		icon_label->setPixmap(pp);
	}
	//
	update_image(fit, false);
	//
	if (alw_usregs) graphicsview->draw_us_regions();
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

void StudyGraphicsWidget::update_measurement(
	double x0,
	double y0,
	double x1,
	double y1)
{
}

void StudyGraphicsWidget::set_mouse_modus(short m, bool us_regions)
{
	mouse_modus = m;
	switch(mouse_modus)
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
		QPixmap p(16,16);
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
	image_container.image2D->pSS=NULL;
	if (image_container.image2D->pUS.IsNotNull())
		image_container.image2D->pUS->DisconnectPipeline();
	image_container.image2D->pUS=NULL;
	if (image_container.image2D->pSI.IsNotNull())
		image_container.image2D->pSI->DisconnectPipeline();
	image_container.image2D->pSI=NULL;
	if (image_container.image2D->pUI.IsNotNull())
		image_container.image2D->pUI->DisconnectPipeline();
	image_container.image2D->pUI=NULL;
	if (image_container.image2D->pUC.IsNotNull())
		image_container.image2D->pUC->DisconnectPipeline();
	image_container.image2D->pUC=NULL;
	if (image_container.image2D->pF.IsNotNull())
		image_container.image2D->pF->DisconnectPipeline();
	image_container.image2D->pF=NULL;
	if (image_container.image2D->pD.IsNotNull())
		image_container.image2D->pD->DisconnectPipeline();
	image_container.image2D->pD=NULL;
	if (image_container.image2D->pSLL.IsNotNull())
		image_container.image2D->pSLL->DisconnectPipeline();
	image_container.image2D->pSLL=NULL;
	if (image_container.image2D->pULL.IsNotNull())
		image_container.image2D->pULL->DisconnectPipeline();
	image_container.image2D->pULL=NULL;
	if (image_container.image2D->pSS_rgb.IsNotNull())
		image_container.image2D->pSS_rgb->DisconnectPipeline();
	image_container.image2D->pSS_rgb=NULL;
	if (image_container.image2D->pUS_rgb.IsNotNull())
		image_container.image2D->pUS_rgb->DisconnectPipeline();
	image_container.image2D->pUS_rgb=NULL;
	if (image_container.image2D->pSI_rgb.IsNotNull())
		image_container.image2D->pSI_rgb->DisconnectPipeline();
	image_container.image2D->pSI_rgb=NULL;
	if (image_container.image2D->pUI_rgb.IsNotNull())
		image_container.image2D->pUI_rgb->DisconnectPipeline();
	image_container.image2D->pUI_rgb=NULL;
	if (image_container.image2D->pUC_rgb.IsNotNull())
		image_container.image2D->pUC_rgb ->DisconnectPipeline();
	image_container.image2D->pUC_rgb=NULL;
	if (image_container.image2D->pF_rgb.IsNotNull())
		image_container.image2D->pF_rgb->DisconnectPipeline();
	image_container.image2D->pF_rgb=NULL;
	if (image_container.image2D->pD_rgb.IsNotNull())
		image_container.image2D->pD_rgb->DisconnectPipeline();
	image_container.image2D->pD_rgb=NULL;
	if (image_container.image2D->pSS_rgba.IsNotNull())
		image_container.image2D->pSS_rgba->DisconnectPipeline();
	image_container.image2D->pSS_rgba=NULL;
	if (image_container.image2D->pUS_rgba.IsNotNull())
		image_container.image2D->pUS_rgba->DisconnectPipeline();
	image_container.image2D->pUS_rgba=NULL;
	if (image_container.image2D->pSI_rgba.IsNotNull())
		image_container.image2D->pSI_rgba->DisconnectPipeline();
	image_container.image2D->pSI_rgba=NULL;
	if (image_container.image2D->pUI_rgba.IsNotNull())
		image_container.image2D->pUI_rgba->DisconnectPipeline();
	image_container.image2D->pUI_rgba=NULL;
	if (image_container.image2D->pUC_rgba.IsNotNull())
		image_container.image2D->pUC_rgba->DisconnectPipeline();
	image_container.image2D->pUC_rgba=NULL;
	if (image_container.image2D->pF_rgba.IsNotNull())
		image_container.image2D->pF_rgba->DisconnectPipeline();
	image_container.image2D->pF_rgba=NULL;
	if (image_container.image2D->pD_rgba.IsNotNull())
		image_container.image2D->pD_rgba->DisconnectPipeline();
	image_container.image2D->pD_rgba=NULL;
	//
	switch(image_container.image3D->image_type)
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
			(((int)image_container.image3D->di->image_slices.size() == image_container.image3D->di->idimz) &&
				(int)image_container.image3D->di->image_slices.size() > x);
		if (image_container.image3D->equi)
		{
			image_container.image2D->orientation_string = image_container.image3D->orientation_string;
		}
		else
		{
			if (
				check_consistence &&
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
	if (studyview)
	{
		if (studyview->get_active_id() == image_container.image3D->id)
		{
			studyview->update_level(&image_container);
		}
		studyview->update_scouts();
	}
	//
	update_image(0, false);
	//
//	if (alw_usregs) FIXME
		graphicsview->draw_us_regions();
	if (!image_container.image3D->pr_display_areas.empty())
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

void StudyGraphicsWidget::set_active()
{
	if (studyview) studyview->set_active_image(&image_container);
}
