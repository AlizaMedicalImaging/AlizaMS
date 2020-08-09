#include "graphicswidget.h"
#include <QtGlobal>
#include <QVBoxLayout>
#include <QSplitter>
#include <QImage>
#include <QSettings>
#include <QGraphicsPixmapItem>
#include <QFont>
#include <QMessageBox>
#include <QPainterPath>
#include <QBrush>
#include <QPen>
#include <QGraphicsPathItem>
#include <QProgressDialog>
#include <QDateTime>
#include <QPainter>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QScrollBar>
#include "itk/itkSigmoid2ImageFilter.h"
#include "itkIntensityWindowingImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkScalarImageToHistogramGenerator.h"
#include "itkImageToHistogramFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "processimagethreadLUT.hxx"
#include "graphicsutils.h"
#include "commonutils.h"
#include "contourutils.h"
#include "aliza.h"
#include "updateqtcommand.h"

#ifndef WIN32
#include <unistd.h>
#if QT_VERSION < QT_VERSION_CHECK(4,7,0)
#include <sys/time.h>
#endif
#endif

#include <limits>

void gImageCleanupHandler(void * info)
{
	if (!info) return;
	unsigned char * p = (unsigned char*)info;
	delete [] p;
	info = NULL;
}

static void draw_contours(
	const ImageVariant * ivariant,
	GraphicsWidget * widget)
{
	widget->graphicsview->scene()->clearSelection();
	widget->graphicsview->clear_paths();
	const short axis = widget->get_axis();
	const bool editable =
		(widget->get_mouse_modus() == 3) ? true : false;
	if (axis != 2) return;
	const int idx = ivariant->di->selected_z_slice;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		const int c_r = (int)(ivariant->di->rois.at(x).color.r*255.0f);
		const int c_g = (int)(ivariant->di->rois.at(x).color.g*255.0f);
		const int c_b = (int)(ivariant->di->rois.at(x).color.b*255.0f);
		if (ivariant->di->rois.at(x).show)
		{
			QList<int> indices;
			if (ivariant->di->rois.at(x).map.contains(idx))
			{
				indices = ivariant->di->rois.at(x).map.values(idx);
			}
			else
			{
				continue;
			}
			for (int z = 0; z < indices.size(); z++)
			{
				const Contour * c =
					ivariant->di->rois.at(x)
						.contours.value(indices.at(z));
				if (c)
				{
					QBrush brush;
					if (!ivariant->di->rois.at(x).random_color)
					{
						brush = QBrush(QColor(c_r,c_g,c_b));
					}
					else
					{
						const int rc_r = (int)(c->color.r*255.0f);
						const int rc_g = (int)(c->color.g*255.0f);
						const int rc_b = (int)(c->color.b*255.0f);
						brush = QBrush(QColor(rc_r,rc_g,rc_b));
					}
					QPen pen;
					pen.setBrush(brush);
					pen.setStyle(Qt::SolidLine);
					if (c->type == 1 || c->type == 2)
					{
						pen.setWidthF(widget->get_contours_width());
						GraphicsPathItem * pi_ =
							new GraphicsPathItem();
						pi_->set_roi_id(ivariant->di->rois.at(x).id);
						pi_->set_contour_id(c->id);
						pi_->setFlag(
							QGraphicsItem::ItemIsSelectable,
							editable);
						if (editable)
						{
							pi_->setCursor(Qt::PointingHandCursor);
							pi_->set_axis(2);
							pi_->set_slice(idx);
						}
						pi_->setPen(pen);
						pi_->setPath(c->path);
						widget->graphicsview->scene()->addItem(
							static_cast<QGraphicsItem*>(pi_));
						widget->graphicsview->paths.push_back(pi_);
					}
					else
					{
						pen.setWidthF(widget->get_contours_width());
						GraphicsPathItem * pi_ =
							new GraphicsPathItem();
						pi_->set_roi_id(ivariant->di->rois.at(x).id);
						pi_->set_contour_id(c->id);
						pi_->setFlag(
							QGraphicsItem::ItemIsSelectable,
							editable);
						if (editable)
							pi_->setCursor(Qt::PointingHandCursor);
						pi_->setPen(pen);
						QPainterPath p;
						for (int y = 0; y < c->dpoints.size(); y++)
						{
							if (c->dpoints.at(y).t == idx)
								p.addRect(
									c->dpoints.at(y).u-0.25f,
									c->dpoints.at(y).v-0.25f,
									0.5,
									0.5);
						}
						pi_->setPath(p);
						widget->graphicsview->scene()->addItem(
							static_cast<QGraphicsItem*>(pi_));
						widget->graphicsview->paths.push_back(pi_);
					}
				}
			}
		}
	}
}

template<typename Tin, typename Tout> QString get_slice_(
	short axis,
	const typename Tin::Pointer & image,
	ImageVariant2D * v2d,
	typename Tout::Pointer & out_image,
	int idx)
{
	if (image.IsNull())
		return QString("get_slice_<>() : image.IsNull()");
	typedef itk::ExtractImageFilter<Tin, Tout> FilterType;
	const typename Tin::RegionType inRegion =
		image->GetLargestPossibleRegion();
	const typename Tin::SizeType size = inRegion.GetSize();
	typename Tin::IndexType index     = inRegion.GetIndex();
	typename Tin::RegionType outRegion;
	typename Tin::SizeType out_size;
	typename FilterType::Pointer filter = FilterType::New();
	switch(axis)
	{
	case 0:
		{
			index[0] = idx;
			out_size[0]=0;
			out_size[1]=size[1];
			out_size[2]=size[2];
		}
		break;
	case 1:
		{
			index[1] = idx;
			out_size[0]=size[0];
			out_size[1]=0;
			out_size[2]=size[2];
		}
		break;
	case 2:
		{
			index[2] = idx;
			out_size[0]=size[0];
			out_size[1]=size[1];
			out_size[2]=0;
		}
		break;
	default :
		return QString("internal error: axis not set");
	}
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

template<typename T> QString contour_from_path(
		ROI * roi,
		const typename T::Pointer & image,
		const GraphicsPathItem * item)
{
	if (image.IsNull()) return QString("image is NULL");
	if (!roi)           return QString("ROI is NULL");
	if (!item)          return QString("item is NULL");
	if (item->get_roi_id() >= 0 && item->get_roi_id() == roi->id)
	{
		return
			QString("contour ") +
			QVariant(item->get_contour_id()).toString() +
			QString(" is already in ROI ") +
			QVariant(item->get_roi_id()).toString();
	}
	const QPainterPath & p = item->path();
	QVector<int> tmp0;
	QMapIterator< int, Contour* > it(roi->contours);
	while (it.hasNext())
	{
		it.next();
		const Contour * z = it.value();
		if (z) tmp0.push_back(z->id);
	}
	int a = 0;
	for (int j = 0; j < tmp0.size(); j++)
	{
		if (tmp0.at(j) >= a) a = tmp0.at(j);
	}
	tmp0.clear();
	a += 1;
	Contour * c = NULL;
	try { c = new Contour(); }
	catch (std::bad_alloc&) { c = NULL; }
	if (!c) return QString("std::bad_alloc exeption");
	c->id = a;
	c->roiid = roi->id;
	c->color.r = 0.0f;
	c->color.g = 0.0f;
	c->color.b = 1.0f;
	for (int x = 0; x < p.elementCount(); x++)
	{
		itk::ContinuousIndex<float, 3> idx;
		switch(item->get_axis())
		{
		case 0:
			{
				idx[0]=item->get_slice();
				idx[1]=p.elementAt(x).x;
				idx[2]=p.elementAt(x).y;
			}
			break;
		case 1:
			{
				idx[0]=p.elementAt(x).x;
				idx[1]=item->get_slice();
				idx[2]=p.elementAt(x).y;
			}
			break;
		case 2:
			{
				idx[0]=p.elementAt(x).x;
				idx[1]=p.elementAt(x).y;
				idx[2]=item->get_slice();
			}
			break;
		default:
			{
				return
					QString(
						"item->get_axis() - unknown value");
			}
		}
		itk::Point<float,3> j;
		image->TransformContinuousIndexToPhysicalPoint(idx, j);
		DPoint point;
		point.x = j[0];
		point.y = j[1];
		point.z = j[2];
		point.u = p.elementAt(x).x;
		point.v = p.elementAt(x).y;
		point.t = -1;
		c->dpoints.push_back(point);
	}
	if (item->get_axis() == 2)
		c->type = item->get_type();
	else
		c->type = 0;
	c->vao_initialized = false;
	roi->contours[c->id] = c;
	return QString("");
}

static QString contour_from_path_nonuniform(
		ROI * roi,
		const ImageVariant * ivariant,
		const GraphicsPathItem * item)
{
	if (!ivariant) return QString("image is NULL");
	if (ivariant->di->idimz != (int)ivariant->di->image_slices.size())
		return QString("Failed");
	if (!roi)  return QString("ROI is NULL");
	if (!item) return QString("item is NULL");
	if (item->get_axis() !=2 )
		return QString("Only original slices are supported");
	if (item->get_roi_id() >= 0 && item->get_roi_id() == roi->id)
	{
		return
			QString("Contour ") +
			QVariant(item->get_contour_id()).toString() +
			QString(" is already in ROI ") +
			QVariant(item->get_roi_id()).toString();
	}
	ImageTypeUC::Pointer image = ImageTypeUC::New();
	const bool ok = ContourUtils::phys_space_from_slice(
		ivariant, item->get_slice(), image);
	if (!ok) return QString("Could not extract slice");
	const QPainterPath & p = item->path();
	QVector<int> tmp0;
	QMapIterator< int, Contour* > it(roi->contours);
	while (it.hasNext())
	{
		it.next();
		const Contour * z = it.value();
		if (z) tmp0.push_back(z->id);
	}
	int a = 0;
	for (int j = 0; j < tmp0.size(); j++)
	{
		if (tmp0.at(j) >= a) a = tmp0.at(j);
	}
	tmp0.clear();
	a += 1;
	Contour * c = NULL;
	try { c = new Contour(); }
	catch (std::bad_alloc&) { c = NULL; }
	if (!c) return QString("std::bad_alloc exeption");
	c->id = a;
	c->roiid = roi->id;
	c->color.r = 0.0f;
	c->color.g = 0.0f;
	c->color.b = 1.0f;
	for (int x = 0; x < p.elementCount(); x++)
	{
		itk::ContinuousIndex<float, 3> idx;
		idx[0]=p.elementAt(x).x;
		idx[1]=p.elementAt(x).y;
		idx[2]=0;
		itk::Point<float,3> j;
		image->TransformContinuousIndexToPhysicalPoint(idx, j);
		DPoint point;
		point.x = j[0];
		point.y = j[1];
		point.z = j[2];
		point.u = p.elementAt(x).x;
		point.v = p.elementAt(x).y;
		point.t = -1;
		c->dpoints.push_back(point);
	}
	if (item->get_axis() == 2)
		c->type = item->get_type();
	else
		c->type = 0;
	c->vao_initialized = false;
	roi->contours[c->id] = c;
	return QString("");
}

template<typename T> void load_rgb_image(
	const typename T::Pointer & image,
	ImageVariant * ivariant,
	GraphicsWidget * widget,
	const QString & rai,
	const QString & laterality,
	const QString & body_part,
	const short image_type,
	const bool redraw_contours,
	short fit=0)
{
	if (!ivariant) return;
	if (image.IsNull()) return;
#ifdef DELETE_GRAPHICSIMAGEITEM
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
	const short axis = widget->get_axis();
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
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QImage tmpi(p__,size[0],size[1],3*size[0],QImage::Format_RGB888,gImageCleanupHandler,p__);
#else
	QImage tmpi(p__,size[0],size[1],3*size[0],QImage::Format_RGB888);
#endif
	//
	if (axis==2)
	{
		if (widget->get_enable_overlays())
			GraphicsUtils::draw_overlays(ivariant, tmpi);
	}
	else
	{
		if (!ivariant->equi||ivariant->orientation_string.isEmpty())
			GraphicsUtils::draw_cross_out(tmpi);
	}
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	GraphicsUtils::gen_labels(
		axis, hide_orientation,
		rai, laterality, body_part,
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
	if (redraw_contours && axis==2)
		draw_contours(ivariant, widget);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->set_top_label_text(top_string);
	widget->set_left_label_text(left_string);
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p__) delete [] p__;
#endif
}

template<typename T> void load_rgba_image(
	const typename T::Pointer & image,
	ImageVariant * ivariant,
	GraphicsWidget * widget,
	const QString & rai,
	const short image_type,
	const bool redraw_contours,
	short fit=0)
{
	if (!ivariant) return;
	if (image.IsNull()) return;
#ifdef DELETE_GRAPHICSIMAGEITEM
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
	unsigned long j_ = 0;
	const short axis = widget->get_axis();
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
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (!(vrange!=0)) return;
		try { p__ = new unsigned char[size[0]*size[1]*4]; }
		catch(std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
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
	QImage tmpi(p__,size[0],size[1],4*size[0],QImage::Format_RGBA8888,gImageCleanupHandler,p__);
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
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (!(vrange!=0)) return;
		try { p__ = new unsigned char[size[0]*size[1]*3]; }
		catch(std::bad_alloc&) { p__ = NULL; }
		if (!p__) return;
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
	QImage tmpi(p__,size[0],size[1],3*size[0],QImage::Format_RGB888);
#endif
	//
	if (axis==2)
	{
		if (widget->get_enable_overlays())
			GraphicsUtils::draw_overlays(ivariant, tmpi);
	}
	else
	{
		if (!ivariant->equi||ivariant->orientation_string.isEmpty())
			GraphicsUtils::draw_cross_out(tmpi);
	}
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	GraphicsUtils::gen_labels(
		axis, hide_orientation,
		rai, QString(""), QString(""),
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
	if (redraw_contours && axis==2)
		draw_contours(ivariant, widget);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->set_top_label_text(top_string);
	widget->set_left_label_text(left_string);
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p__) delete [] p__;
#endif
}

template<typename T> void load_rgb_char_image(
	const typename T::Pointer & image,
	ImageVariant * ivariant,
	GraphicsWidget * widget,
	const QString & rai,
	const QString & laterality,
	const QString & body_part,
	const short image_type,
	const bool redraw_contours,
	short fit=0)
{
	if (image.IsNull()) return;
#ifdef DELETE_GRAPHICSIMAGEITEM
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
	const short axis = widget->get_axis();
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	if (image_type == 14)
	{
		try { p = reinterpret_cast<unsigned char *>(image->GetBufferPointer()); }
		catch (itk::ExceptionObject & ex) { std::cout << ex << std::endl; return; }
	}
	else return;
	if (!p) return;
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
	QImage tmpi = QImage(p,size[0],size[1],3*size[0],QImage::Format_RGB888);
	//
	if (axis==2)
	{
		if (widget->get_enable_overlays())
			GraphicsUtils::draw_overlays(ivariant, tmpi);
	}
	else
	{
		if (!ivariant->equi||ivariant->orientation_string.isEmpty())
			GraphicsUtils::draw_cross_out(tmpi);
	}
	//
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	const bool hide_orientation = ivariant->di->hide_orientation;
	GraphicsUtils::gen_labels(
		axis, hide_orientation,
		rai, laterality, body_part,
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
	if (redraw_contours && axis==2)
		draw_contours(ivariant, widget);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->set_top_label_text(top_string);
	widget->set_left_label_text(left_string);
	widget->graphicsview->setTransform(t);
}

template<typename T> void load_rgba_char_image(
	const typename T::Pointer & image,
	ImageVariant * ivariant,
	GraphicsWidget * widget,
	const QString & rai,
	const short image_type,
	const bool redraw_contours,
	short fit=0)
{
	if (image.IsNull()) return;
#ifdef DELETE_GRAPHICSIMAGEITEM
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
	const short axis = widget->get_axis();
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
		unsigned long j_ = 0;
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
		tmpi = QImage(p,size[0],size[1],3*size[0],QImage::Format_RGB888);
#endif
	}
	else return;
	//
	if (axis==2)
	{
		if (widget->get_enable_overlays())
			GraphicsUtils::draw_overlays(ivariant, tmpi);
	}
	else
	{
		if (!ivariant->equi||ivariant->orientation_string.isEmpty())
			GraphicsUtils::draw_cross_out(tmpi);
	}
	widget->graphicsview->image_item->setPixmap(QPixmap::fromImage(tmpi));
	//
	const bool hide_orientation = ivariant->di->hide_orientation;
	GraphicsUtils::gen_labels(
		axis, hide_orientation,
		rai, QString(""), QString(""),
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
	if (redraw_contours && axis==2)
		draw_contours(ivariant, widget);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->set_top_label_text(top_string);
	widget->set_left_label_text(left_string);
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p) delete [] p;
#endif
}

template<typename T> void load_image(const typename T::Pointer & image,
	ImageVariant * ivariant,
	GraphicsWidget * widget,
	const QString & rai,
	const QString & laterality,
	const QString & body_part,
	const bool redraw_contours,
	short fit=0)
{
	if (image.IsNull()) return;
	if (!ivariant) return;
	//
	const typename T::SpacingType spacing = image->GetSpacing();
	const typename T::RegionType region   = image->GetLargestPossibleRegion();
	const typename T::SizeType size       = region.GetSize();
	const short lut      = ivariant->di->selected_lut;
	const bool  alt_mode = widget->get_alt_mode();
	const int lut_function = ivariant->di->lut_function;
	//
	const unsigned int p_size = 3*size[0]*size[1];
	unsigned char * p = NULL;
	try { p = new unsigned char[p_size]; }
	catch (std::bad_alloc&) { p = NULL; }
	if (!p) return;
	const short axis = widget->get_axis();
	const bool global_flip_x = widget->graphicsview->global_flip_x;
	const bool global_flip_y = widget->graphicsview->global_flip_y;
	const int num_threads = QThread::idealThreadCount();
	const int tmp99 = size[1]%num_threads;
	if (!widget->threadsLUT_.empty())
	{
		std::cout << "load_image4<>() : widget->threadsLUT_.size()>0" << std::endl;
	}
	if (tmp99==0)
	{
		int j=0;
		for (int i=0; i<num_threads; i++)
		{
			const int size_0 = size[0];
			const int size_1 = size[1]/num_threads;
			const int index_0 = 0;
			const int index_1 = i*size_1;
			ProcessImageThreadLUT_<T> * t__ = new ProcessImageThreadLUT_<T>(image,
						p,
						size_0,  size_1,
						index_0, index_1, j,
						ivariant->di->us_window_center, ivariant->di->us_window_width,
						lut, alt_mode,lut_function);
			j += 3*(size_0*size_1);
			widget->threadsLUT_.push_back(static_cast<QThread*>(t__));
			t__->start();
		}
	}
	else
	{
		int j=0;
		unsigned int block = 64;
		if (static_cast<float>(size[1])/static_cast<float>(block)>16.0f) block=128;
		const int tmp100 = size[1]%block;
		const int incr = (int)floor(size[1]/(double)block);
		if (size[1] > block)
		{
			for (int i=0; i<incr; i++)
			{
				const int size_0 = size[0];
				const int index_0 = 0;
				const int index_1 = i*block;
				ProcessImageThreadLUT_<T> * t__ = new ProcessImageThreadLUT_<T>(image,
							p,
							size_0,  block,
							index_0, index_1, j,
							ivariant->di->us_window_center, ivariant->di->us_window_width,
							lut, alt_mode,lut_function);
				j += 3*(size_0*block);
				widget->threadsLUT_.push_back(static_cast<QThread*>(t__));
				t__->start();
			}
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(image,
						p,
						size[0],  tmp100,
						0, incr*block, j,
						ivariant->di->us_window_center, ivariant->di->us_window_width,
						lut, alt_mode,lut_function);
			widget->threadsLUT_.push_back(static_cast<QThread*>(lt__));
			lt__->start();
		}
		else
		{
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(image,
						p,
						size[0],  size[1],
						0, 0, 0,
						ivariant->di->us_window_center, ivariant->di->us_window_width,
						lut, alt_mode,lut_function);
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
		for (int i=0; i < threadsLUT_size; i++)
		{
			if (widget->threadsLUT_.at(i)->isFinished()) { b__++; }
		}
		if (b__==threadsLUT_size) break;
	}
	for (int i=0; i < threadsLUT_size; i++)
	{
		delete widget->threadsLUT_[i];
	}
	widget->threadsLUT_.clear();
	//
	double coeff_size_0 = 1.0, coeff_size_1 = 1.0;
	const QRectF rectf(0,0,size[0],size[1]);
	double scale__;
	if (spacing[0]!=spacing[1])
	{
		if (spacing[1]>spacing[0]) coeff_size_1 = spacing[1]/spacing[0];
		else                       coeff_size_0 = spacing[0]/spacing[1];
	}
	QString top_string = QString(""), left_string = QString("");
	bool flip_y = false, flip_x = false;
	//
	widget->graphicsview->scene()->setSceneRect(rectf);
#ifdef DELETE_GRAPHICSIMAGEITEM
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
	QImage tmpi(p,size[0],size[1],3*size[0],QImage::Format_RGB888,gImageCleanupHandler,p);
#else
	QImage tmpi(p,size[0],size[1],3*size[0],QImage::Format_RGB888);
#endif
	//
	if (axis==2)
	{
		if (widget->get_enable_overlays())
			GraphicsUtils::draw_overlays(ivariant, tmpi);
	}
	else
	{
		if (!ivariant->equi||ivariant->orientation_string.isEmpty())
			GraphicsUtils::draw_cross_out(tmpi);
	}
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
		axis, hide_orientation,
		rai, laterality, body_part,
		left_string, top_string,
		global_flip_x, global_flip_y,
		&flip_x, &flip_y);
	//
	//std::cout << "Flip x=" << flip_x << " y=" << flip_y << std::endl;
	//
	if (flip_y && flip_x) t = t.scale(-1.0,-1.0);
	else if (flip_y)      t = t.scale( 1.0,-1.0);
	else if (flip_x)      t = t.scale(-1.0, 1.0);
	else ;;
	if (global_flip_x) { t = t.scale(-1.0, 1.0); }
	if (global_flip_y) { t = t.scale( 1.0,-1.0); }
	//
	widget->set_top_label_text(top_string);
	widget->set_left_label_text(left_string);
	//
	if (redraw_contours && axis==2)
		draw_contours(ivariant, widget);
	//
	widget->graphicsview->draw_shutter(ivariant);
	widget->graphicsview->draw_prgraphics(ivariant);
	widget->graphicsview->draw_prtexts(ivariant);
	//
	widget->graphicsview->setTransform(t);
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	tmpi = QImage();
	if (p) delete [] p;
#endif
}

static double get_distance2(
	const double x0,
	const double y0,
	const double x1,
	const double y1)
{
	const double x = x1 - x0;
	const double y = y1 - y0;
	return sqrt(x*x + y*y);
}

template<typename T> double get_distance(
	const typename T::Pointer & image,
	const ImageVariant * ivariant,
	const int axis,
	const double x0, const double y0,
	const double x1, const double y1)
{
	if (image.IsNull()) return -1;
	if (!ivariant)      return -1;
	if (x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0) return -1;
	itk::ContinuousIndex<float, 3> idx0;
	itk::ContinuousIndex<float, 3> idx1;
	itk::Point<float,3> j0;
	itk::Point<float,3> j1;
	const float x0_ = static_cast<float>(x0);
	const float y0_ = static_cast<float>(y0);
	const float x1_ = static_cast<float>(x1);
	const float y1_ = static_cast<float>(y1);
	double d = -1;
	bool ok = false;
	//
	switch(axis)
	{
	case 0:
		{
			if (ivariant->equi)
			{
				if (
					x0_<ivariant->di->idimy &&
					x1_<ivariant->di->idimy &&
					y0_<ivariant->di->idimz &&
					y1_<ivariant->di->idimz)
				{
					ok = true;
					idx0[0] = idx1[0]=ivariant->di->selected_x_slice;
					idx0[1] = x0_;
					idx0[2] = y0_;
					idx1[1] = x1_;
					idx1[2] = y1_;
				}
			}
		}
		break;
	case 1:
		{
			if (ivariant->equi)
			{
				if (
					x0_<ivariant->di->idimx &&
					x1_<ivariant->di->idimx &&
					y0_<ivariant->di->idimz &&
					y1_<ivariant->di->idimz)
				{
					ok = true;
					idx0[1] = idx1[1]=ivariant->di->selected_y_slice;
					idx0[0] = x0_;
					idx0[2] = y0_;
					idx1[0] = x1_;
					idx1[2] = y1_;
				}
			}
		}
		break;
	case 2:
		{
			if (
				x0_<ivariant->di->idimx &&
				x1_<ivariant->di->idimx &&
				y0_<ivariant->di->idimy &&
				y1_<ivariant->di->idimy)
			{
				ok = true;
				idx0[2] = idx1[2]=ivariant->di->selected_z_slice;
				idx0[0] = x0_;
				idx0[1] = y0_;
				idx1[0] = x1_;
				idx1[1] = y1_;
			}
		}
		break;
	default: return false;
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

GraphicsWidget::GraphicsWidget(
	short a,
	bool gl_,
	QLabel * top_label_,
	QLabel * left_label_,
	QLabel * measure_label_,
	QLineEdit * info_line_,
	QWidget * single_frame,
	QWidget * multi_frame,
	QWidget * p)
	:
	QWidget(p)
{
	run__ = false;
	axis = a;
	main = false;
	bb = false;
	gl = false;
	smooth_ = true;
	top_label  = top_label_;
	left_label = left_label_;
	measure_label = measure_label_;
	info_line = info_line_;
	single_frame_ptr = single_frame;
	multi_frame_ptr = multi_frame;
	alt_mode = false;
	show_cursor = false;
	mouse_modus = 0;
	enable_shutter = true;
	enable_overlays = true;
	frame_time_unit = 0; // 0 - ms; 1 - s
	frametime_2D = 120;
	contours_width = 0.0;
	toolbox2D = NULL;
	slider_m = NULL;
	anim2D_timer = new QTimer(this);
	anim2D_timer->setSingleShot(true);
	connect(anim2D_timer, SIGNAL(timeout()), this, SLOT(animate_()));
	image_container.image3D = NULL;
	image_container.image2D = new ImageVariant2D();
	graphicsview = new GraphicsView(this);
	QVBoxLayout * l = new QVBoxLayout(this);
	l->addWidget(graphicsview);
	l->setSpacing(0);
	l->setContentsMargins(0,0,0,0);
	setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Expanding);
}

GraphicsWidget::~GraphicsWidget()
{
	run__ = false;
	if (mutex.tryLock(30000))
	{
		for (unsigned int i=0; i<threads_.size(); i++)
		{
			if (threads_.at(i))
			{
				if (threads_.at(i)->isRunning()) threads_[i]->exit();
				delete threads_[i];
				threads_[i] = NULL;
			}
		}
		for (unsigned int i=0; i<threadsLUT_.size(); i++)
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

void GraphicsWidget::closeEvent(QCloseEvent * e)
{
	e->accept();
}

void GraphicsWidget::leaveEvent(QEvent*)
{
	update_pixel_value(-1,-1);
}

void GraphicsWidget::update_image__()
{
	update_image(0, true, true);
}

void GraphicsWidget::update_selection_rectangle()
{
	if (axis != 2) return;
	if (!image_container.image3D) return;
	QRectF r___ = QRectF(
		QPointF(
			image_container.image3D->di->irect_index[0],
			image_container.image3D->di->irect_index[1]),
		QSize(
			image_container.image3D->di->irect_size[0],
			image_container.image3D->di->irect_size[1]));
	graphicsview->handle_rect->resetTransform();
	graphicsview->handle_rect->setRect(r___);
	graphicsview->handle_rect->setPos(QPointF(0.0f,0.0f));
}

void GraphicsWidget::update_pr_area()
{
	if (axis != 2) return;
	if (!image_container.image3D) return;
	const int idx = image_container.image3D->di->selected_z_slice;
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

void GraphicsWidget::update_selection_item()
{
	if (axis == 2) return;
	if (!image_container.image3D) return;
	const float from_to =
		((image_container.image3D->di->to_slice + 1 -
			image_container.image3D->di->from_slice) > 0)
		? (image_container.image3D->di->to_slice + 1 -
			image_container.image3D->di->from_slice)
		: 1e-6;
	switch(axis)
	{
	case 0:
		{
			const QRectF r___ = QRectF(
				QPointF(
					(get_bb()
					? (float)image_container.image3D->di->irect_index[1]
					: 0.0f),
					(float)image_container.image3D->di->from_slice),
				QSizeF(
					(get_bb()
					? (float)image_container.image3D->di->irect_size[1]
					: (float)image_container.image3D->di->idimy),
					from_to));
			graphicsview->selection_item->resetTransform();
			graphicsview->selection_item->setRect(r___);
			graphicsview->selection_item->setPos(QPointF(0.0f,0.0f));
		}
		break;
	case 1:
		{
			const QRectF r___ = QRectF(
				QPointF(
					(get_bb()
					? (float)image_container.image3D->di->irect_index[0]
					: 0.0f),
					(float)image_container.image3D->di->from_slice),
				QSizeF(
					(get_bb()
					? (float)image_container.image3D->di->irect_size[0]
					: (float)image_container.image3D->di->idimx),
					from_to));
			graphicsview->selection_item->resetTransform();
			graphicsview->selection_item->setRect(r___);
			graphicsview->selection_item->setPos(QPointF(0.0f,0.0f));
		}
		break;
	default: break;
	}
}

void GraphicsWidget::update_image(
	short fit,
	bool redraw_contours,
	bool lock)
{
	if (!image_container.image3D) return;
	if (!image_container.image2D) return;
	//
	if (lock) mutex.lock();
	//
	switch(image_container.image2D->image_type)
	{
	case 0: load_image<Image2DTypeSS>(
				image_container.image2D->pSS,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 1: load_image<Image2DTypeUS>(
				image_container.image2D->pUS,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 2: load_image<Image2DTypeSI>(
				image_container.image2D->pSI,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 3: load_image<Image2DTypeUI>(
				image_container.image2D->pUI,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 4: load_image<Image2DTypeUC>(
				image_container.image2D->pUC,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 5: load_image<Image2DTypeF>(
				image_container.image2D->pF,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 6: load_image<Image2DTypeD>(
				image_container.image2D->pD,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 7: load_image<Image2DTypeSLL>(
				image_container.image2D->pSLL,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 8: load_image<Image2DTypeULL>(
				image_container.image2D->pULL,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				redraw_contours,
				fit);
		break;
	case 10: load_rgb_image<RGBImage2DTypeSS>(
				image_container.image2D->pSS_rgb,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 11: load_rgb_image<RGBImage2DTypeUS>(
				image_container.image2D->pUS_rgb,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 12: load_rgb_image<RGBImage2DTypeSI>(
				image_container.image2D->pSI_rgb,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 13: load_rgb_image<RGBImage2DTypeUI>(
				image_container.image2D->pUI_rgb,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 14: load_rgb_char_image<RGBImage2DTypeUC>(
				image_container.image2D->pUC_rgb,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 15: load_rgb_image<RGBImage2DTypeF>(
				image_container.image2D->pF_rgb,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 16: load_rgb_image<RGBImage2DTypeD>(
				image_container.image2D->pD_rgb,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->laterality,
				image_container.image2D->body_part,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 20: load_rgba_image<RGBAImage2DTypeSS>(
				image_container.image2D->pSS_rgba,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->image_type,
				fit);
		break;
	case 21: load_rgba_image<RGBAImage2DTypeUS>(
				image_container.image2D->pUS_rgba,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 22: load_rgba_image<RGBAImage2DTypeSI>(
				image_container.image2D->pSI_rgba,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 23: load_rgba_image<RGBAImage2DTypeUI>(
				image_container.image2D->pUI_rgba,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 24: load_rgba_char_image<RGBAImage2DTypeUC>(
				image_container.image2D->pUC_rgba,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 25: load_rgba_image<RGBAImage2DTypeF>(
				image_container.image2D->pF_rgba,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	case 26: load_rgba_image<RGBAImage2DTypeD>(
				image_container.image2D->pD_rgba,
				image_container.image3D,
				this,
				image_container.image2D->orientation_string,
				image_container.image2D->image_type,
				redraw_contours,
				fit);
		break;
	default: break;
	}
	//
	if (lock) mutex.unlock();
}

void GraphicsWidget::clear_(bool lock)
{
	if (lock) mutex.lock();
	if (graphicsview->image_item)
	{
#ifdef DELETE_GRAPHICSIMAGEITEM
		if (graphicsview->scene())
			graphicsview->scene()->removeItem(graphicsview->image_item);
		delete graphicsview->image_item;
		graphicsview->image_item = NULL;
#else
		QPixmap p(16,16);
		graphicsview->image_item->setPixmap(p);
#endif
	}
	graphicsview->clear_paths();
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
	set_top_label_text(QString(""));
	set_left_label_text(QString(""));
	set_info_line_text(QString(""));
	set_measure_text(QString(""));
	graphicsview->set_empty_lines();
	graphicsview->set_empty_distance();
	graphicsview->clear_us_regions();
	graphicsview->clear_prtexts_items();
	graphicsview->clear_prgraphicobjects_items();
	graphicsview->clear_shutters();
	if (lock) mutex.unlock();
}

float GraphicsWidget::get_offset_x()
{
	float offset_x = 0.0f;
	switch(axis)
	{
	case 0:
		{
			offset_x = 
				(bb)
				? (float)image_container.image3D->di->irect_index[1]
				: 0.0f;
		}
		break;
	case 1:
		{
			offset_x =
				(bb)
				? (float)image_container.image3D->di->irect_index[0]
				: 0.0f;
		}
		break;
	case 2:
		{
			offset_x =
				(bb)
				?
				graphicsview->handle_rect->boundingRect().topLeft().x() +
				graphicsview->handle_rect->pos().x() +
				0.5f*graphicsview->handle_rect->get_width()
				:
				0.0f;
		}
		break;
	default : break;
	}
	return offset_x;
}

float GraphicsWidget::get_offset_y()
{
	float offset_y = 0.0f;
	switch(axis)
	{
	case 0:
		{
			offset_y = (float)image_container.image3D->di->from_slice;
		}
		break;
	case 1:
		{
			offset_y = (float)(image_container.image3D->di->from_slice);
		}
		break;
	case 2:
		{
			offset_y = (bb)
			?
			graphicsview->handle_rect->boundingRect().topLeft().y() +
			graphicsview->handle_rect->pos().y() +
			0.5f*graphicsview->handle_rect->get_width()
			:
			0.0f;
		}
		break;
	default : break;
	}
	return offset_y;
}

void GraphicsWidget::set_slice_2D(
	ImageVariant * v,
	short fit,
	bool alw_usregs)
{
	if (graphicsview->image_item)
	{
#ifdef DELETE_GRAPHICSIMAGEITEM
		graphicsview->scene()->removeItem(graphicsview->image_item);
		delete graphicsview->image_item;
		graphicsview->image_item = NULL;
#else
		QPixmap p(16,16);
		graphicsview->image_item->setPixmap(p);
#endif
	}
	graphicsview->clear_paths();
	graphicsview->clear_collision_paths();
	graphicsview->clear_us_regions();
	graphicsview->clear_prtexts_items();
	graphicsview->clear_prgraphicobjects_items();
	graphicsview->clear_shutters();
	set_top_label_text("");
	set_left_label_text("");
	set_measure_text("");
	graphicsview->set_empty_distance();
	graphicsview->pr_area->hide();
	//
	if (!v) return;
	int x = 0;
	QString error_;
	//
	mutex.lock();
	//
	image_container.image3D = v;
	//
	if (!image_container.image2D) goto quit__;
	//
	image_container.image2D->image_type=-1;
	image_container.image2D->orientation_string = QString("");
	image_container.image2D->laterality = QString("");
	image_container.image2D->body_part  = QString("");
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
	switch(axis)
	{
	case   0: x = image_container.image3D->di->selected_x_slice; break;
	case   1: x = image_container.image3D->di->selected_y_slice; break;
	case   2: x = image_container.image3D->di->selected_z_slice; break;
	default : { clear_(false); goto quit__; }
	}
	//
	switch(v->image_type)
	{
	case 0: error_ = get_slice_<ImageTypeSS, Image2DTypeSS>(axis, v->pSS, image_container.image2D, image_container.image2D->pSS, x);
		break;
	case 1: error_ = get_slice_<ImageTypeUS, Image2DTypeUS>(axis, v->pUS, image_container.image2D, image_container.image2D->pUS, x);
		break;
	case 2: error_ = get_slice_<ImageTypeSI, Image2DTypeSI>(axis, v->pSI, image_container.image2D, image_container.image2D->pSI, x);
		break;
	case 3: error_ = get_slice_<ImageTypeUI, Image2DTypeUI>(axis, v->pUI, image_container.image2D, image_container.image2D->pUI, x);
		break;
	case 4: error_ = get_slice_<ImageTypeUC, Image2DTypeUC>(axis, v->pUC, image_container.image2D, image_container.image2D->pUC, x);
		break;
	case 5: error_ = get_slice_<ImageTypeF, Image2DTypeF>(axis, v->pF, image_container.image2D, image_container.image2D->pF, x);
		break;
	case 6: error_ = get_slice_<ImageTypeD, Image2DTypeD>(axis, v->pD, image_container.image2D, image_container.image2D->pD, x);
		break;
	case 7: error_ = get_slice_<ImageTypeSLL, Image2DTypeSLL>(axis, v->pSLL, image_container.image2D, image_container.image2D->pSLL, x);
		break;
	case 8: error_ = get_slice_<ImageTypeULL, Image2DTypeULL>(axis, v->pULL, image_container.image2D, image_container.image2D->pULL, x);
		break;
	case 10: error_ = get_slice_<RGBImageTypeSS, RGBImage2DTypeSS>(axis, v->pSS_rgb, image_container.image2D, image_container.image2D->pSS_rgb, x);
		break;
	case 11: error_ = get_slice_<RGBImageTypeUS, RGBImage2DTypeUS>(axis, v->pUS_rgb, image_container.image2D, image_container.image2D->pUS_rgb, x);
		break;
	case 12: error_ = get_slice_<RGBImageTypeSI, RGBImage2DTypeSI>(axis, v->pSI_rgb, image_container.image2D, image_container.image2D->pSI_rgb, x);
		break;
	case 13: error_ = get_slice_<RGBImageTypeUI, RGBImage2DTypeUI>(axis, v->pUI_rgb, image_container.image2D, image_container.image2D->pUI_rgb, x);
		break;
	case 14: error_ = get_slice_<RGBImageTypeUC, RGBImage2DTypeUC>(axis, v->pUC_rgb, image_container.image2D, image_container.image2D->pUC_rgb, x);
		break;
	case 15: error_ = get_slice_<RGBImageTypeF, RGBImage2DTypeF>(axis, v->pF_rgb, image_container.image2D, image_container.image2D->pF_rgb, x);
		break;
	case 16: error_ = get_slice_<RGBImageTypeD, RGBImage2DTypeD>(axis, v->pD_rgb, image_container.image2D, image_container.image2D->pD_rgb, x);
		break;
	case 20: error_ = get_slice_<RGBAImageTypeSS, RGBAImage2DTypeSS>(axis, v->pSS_rgba, image_container.image2D, image_container.image2D->pSS_rgba, x);
		break;
	case 21: error_ = get_slice_<RGBAImageTypeUS, RGBAImage2DTypeUS>(axis, v->pUS_rgba, image_container.image2D, image_container.image2D->pUS_rgba, x);
		break;
	case 22: error_ = get_slice_<RGBAImageTypeSI, RGBAImage2DTypeSI>(axis, v->pSI_rgba, image_container.image2D, image_container.image2D->pSI_rgba, x);
		break;
	case 23: error_ = get_slice_<RGBAImageTypeUI, RGBAImage2DTypeUI>(axis, v->pUI_rgba, image_container.image2D, image_container.image2D->pUI_rgba, x);
		break;
	case 24: error_ = get_slice_<RGBAImageTypeUC, RGBAImage2DTypeUC>(axis, v->pUC_rgba, image_container.image2D, image_container.image2D->pUC_rgba, x);
		break;
	case 25: error_ = get_slice_<RGBAImageTypeF, RGBAImage2DTypeF>(axis, v->pF_rgba, image_container.image2D, image_container.image2D->pF_rgba, x);
		break;
	case 26: error_ = get_slice_<RGBAImageTypeD, RGBAImage2DTypeD>(axis, v->pD_rgba, image_container.image2D, image_container.image2D->pD_rgba, x);
		break;
	default: { clear_(false); goto quit__; }
	}
	//
	if (error_.isEmpty())
	{
		image_container.image2D->image_type = v->image_type;
	}
	else { goto quit__; }
	//
	switch(axis)
	{
	case 0  :
		{
			if (v->equi)
				image_container.image2D->orientation_string =
					v->orientation_string;
		}
		break;
	case 1  : 
		{
			if (v->equi)
				image_container.image2D->orientation_string =
					v->orientation_string;
		}
		break;
	case 2  : 
		{
			const bool check_consistence =
				((int)v->di->image_slices.size() == v->di->idimz) &&
					(int)v->di->image_slices.size()>x;
			if (v->equi)
			{
				image_container.image2D->orientation_string =
					v->orientation_string;
			}
			else
			{
				if (
					check_consistence &&
					!v->di->image_slices.at(x)->slice_orientation_string.isEmpty())
					image_container.image2D->orientation_string =
						v->di->image_slices.at(x)->slice_orientation_string;
			}
			if (v->anatomy.contains(x))
			{
				image_container.image2D->laterality =
					v->anatomy.value(x).laterality;
				image_container.image2D->body_part  =
					v->anatomy.value(x).body_part;
			}
		}
		break;
	default : break; // never
	}
	update_image(fit, true, false);
	//
	if (alw_usregs) graphicsview->draw_us_regions();
	graphicsview->update_selection_rect_width();
	update_frames();
	update_selection_rectangle();
	update_selection_item();
	if (axis == 2)
	{
		if (!v->pr_display_areas.empty())
		{
			update_pr_area();
		}
		else
		{
			if (graphicsview->pr_area->isVisible())
				graphicsview->pr_area->hide();
		}
	}
quit__:
	mutex.unlock();
}

void GraphicsWidget::set_axis(int a)
{
	axis = a;
}

void GraphicsWidget::set_top_label(QLabel * i)
{
	top_label = i;
}

void GraphicsWidget::set_left_label(QLabel * i)
{
	left_label = i;
}

void GraphicsWidget::set_measure_label(QLabel * i)
{
	measure_label = i;
}

void GraphicsWidget::set_info_line(QLineEdit * i)
{
	info_line = i;
}

void GraphicsWidget::set_toolbox2D_widget(ToolBox2D * w)
{
	toolbox2D = w;
}

void GraphicsWidget::set_sliderwidget(SliderWidget * w)
{
	slider_m = w;
}

void GraphicsWidget::set_aliza(Aliza * a)
{
	aliza = a;
}

void GraphicsWidget::set_bb(bool t)
{
	bb = t;
}

bool GraphicsWidget::get_bb() const
{
	return bb;
}

void GraphicsWidget::start_animation()
{
	animate_();
}

void GraphicsWidget::stop_animation()
{
	run__ = false;
	anim2D_timer->stop();
}

void GraphicsWidget::animate_()
{
	int k = 0;
	if (!run__) return;
	double requested_time = frametime_2D;
	bool time_defined = false;
	if (!image_container.image3D) return;
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
	const qint64 t0 = QDateTime::currentMSecsSinceEpoch();
#else
	struct timeval tp0;
	gettimeofday(&tp0, NULL);
	const long long t0 =
		(long long) tp0.tv_sec * 1000L + tp0.tv_usec / 1000;
#endif
	switch(axis)
	{
	case 0:
		{
			k = image_container.image3D->di->selected_x_slice;
			if (k >= image_container.image3D->di->idimx-1 || k < 0)
				k = 0;
			else
				k+=1;
			image_container.image3D->di->selected_x_slice = k;
		}
		break;
	case 1:
		{
			k = image_container.image3D->di->selected_y_slice;
			if (k >= image_container.image3D->di->idimy-1 || k < 0)
				k = 0;
			else
				k+=1;
			image_container.image3D->di->selected_y_slice = k;
		}
		break;
	case 2:
		{
			k = image_container.image3D->di->selected_z_slice;
			if (k >= image_container.image3D->di->idimz-1 || k < 0)
				k = 0;
			else
				k+=1;
			if (image_container.image3D->di->lock_2Dview)
			{
				image_container.image3D->di->from_slice = k;
				if (image_container.image3D->di->lock_single)
				{
					image_container.image3D->di->to_slice = k;
				}
				else
				{
					image_container.image3D->di->to_slice =
						image_container.image3D->di->idimz-1;
				}
			}
			if (
				image_container.image3D->frame_times.size() >
					(unsigned int)k)
			{
				requested_time =
					image_container.image3D->frame_times.at(k);
				if (frame_time_unit == 1) requested_time*=1000;
				time_defined = true;
			}
			image_container.image3D->di->selected_z_slice = k;
		}
		break;
	default: return;
	}
	if (!((axis==2) && (image_container.image3D->di->idimz==1)))
	{
		set_slice_2D(image_container.image3D, 0, false);
		aliza->update_slice_from_animation(
			const_cast<const ImageVariant*>(image_container.image3D));
	}
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
	const qint64 t1 = QDateTime::currentMSecsSinceEpoch();
#else
	struct timeval tp1;
	gettimeofday(&tp1, NULL);
	const long long t1 =
		(long long)tp1.tv_sec * 1000L + (long long)tp1.tv_usec / 1000;
#endif
	const long int t = static_cast<int>(requested_time-(t1-t0));
	if (t <= 0)
	{
		// can not run at required speed
		anim2D_timer->start(2);
		if (!toolbox2D->is_red())
			toolbox2D->set_indicator_red();
	}
	else
	{
		anim2D_timer->start(t);
		if (time_defined)
		{
			if (!toolbox2D->is_green())
				toolbox2D->set_indicator_green();
		}
		else
		{
			if (!toolbox2D->is_blue())
				toolbox2D->set_indicator_blue();
		}
	}	
}

void GraphicsWidget::set_top_label_text(const QString & s)
{
	top_label->setText(s);
}

void GraphicsWidget::set_left_label_text(const QString & s)
{
	left_label->setText(s);
}

void GraphicsWidget::set_info_line_text(const QString & s)
{
	if (axis == 2) info_line->setText(s);
	else info_line->clear();
}

void GraphicsWidget::set_measure_text(const QString & s)
{
	measure_label->setText(s);
}

void GraphicsWidget::update_frames()
{
	if (!image_container.image3D||!image_container.image2D) return;
	if (1==get_mouse_modus() && image_container.image3D->equi)
		graphicsview->draw_frames(get_axis());
}

void GraphicsWidget::update_background_color()
{
	graphicsview->update_background_color();
}

void GraphicsWidget::set_main()
{
	main = true;
}

bool GraphicsWidget::is_main() const
{
	return main;
}

void GraphicsWidget::set_multiview(bool t)
{
	multi = t;
}

bool GraphicsWidget::is_multiview() const
{
	return multi;
}

void GraphicsWidget::set_smooth(bool t)
{
	smooth_ = t;
#ifndef DELETE_GRAPHICSIMAGEITEM
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
	update_image(0, false, true);
}

bool GraphicsWidget::get_smooth() const
{
	return smooth_;
}

void GraphicsWidget::update_measurement(
	double x0,
	double y0,
	double x1,
	double y1)
{
	if (!image_container.image3D) return;
	const ImageVariant * ivariant = image_container.image3D;
	const int a = get_axis();
	QString tmp0("");
	if (!ivariant->usregions.empty() && a == 2)
	{
		QVector<int> ids;
		QVector<int> ids2;
		QVector<int> high_priority_regions;
		for (int x = 0; x < ivariant->usregions.size(); x++)
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
		for (int x = 0; x < ids.size(); x++)
		{
			if (ivariant->usregions.at(ids.at(x)).m_FlagsBool)
			{
				const unsigned int flags =
					ivariant->usregions.at(ids.at(x)).m_RegionFlags;
				if ((flags & 1) == 0)
					high_priority_regions.push_back(ids.at(x));
			}
		}
		//
		QString data_type("");
		for (int x = 0; x < ids2.size(); x++)
		{
			if (
				!ivariant->usregions.at(
					ids2.at(x)).m_DataTypeString.isEmpty())
				data_type.append(
					ivariant->usregions.at(ids2.at(x)).m_DataTypeString);
			if (ids2.size()>1 && x!=ids2.size()-1 && !data_type.isEmpty())
				data_type.append(QString("/"));
		}
		//
		if (!ids.empty())
		{
			bool tmp1x = false;
			bool tmp1y = false;
			const unsigned int id =
				(high_priority_regions.size()==1)
				? high_priority_regions.at(0)
				: ids.at(0);
			const double dx  =
				ivariant->usregions.at(id).m_PhysicalDeltaX;
			const double dy  =
				ivariant->usregions.at(id).m_PhysicalDeltaY;
			const double x0_ = x0*dx;
			const double y0_ = y0*dy;
			const double x1_ = x1*dx;
			const double y1_ = y1*dy;
			const QString measure_textx  =
				ivariant->usregions.at(id).m_UnitXString;
			const QString measure_texty  =
				ivariant->usregions.at(id).m_UnitYString;
			const unsigned short spatial = 
				ivariant->usregions.at(id).m_RegionSpatialFormat;
			QColor color(Qt::cyan);
			switch(spatial)
			{
			case 0x1:
				color = QColor(0x00,0xff,0x00); // 2D
				break;
			case 0x2:
				color = QColor(0xff,0x00,0x00); // M-Mode
				break;
			case 0x3:
				color = QColor(0x00,0x00,0xff); // Spectral
				break;
			case 0x4:
				color = QColor(0x00,0xff,0xff); // Waveform
				break;
			case 0x5:
				color = QColor(0xff,0x00,0xff); // Graphics
				break;
			case 0:
			default:
				break;
			}
			if (measure_textx == measure_texty)
			{
				const double d   = get_distance2(x0_, y0_, x1_, y1_);
				tmp0.sprintf("%.3f",d);
				tmp0.append(QString(" ") + measure_textx);
			}
			else
			{
				if (!measure_textx.isEmpty())
				{
					tmp1x = true;
					const double d0 = get_distance2(x0_, y0_, x1_, y0_);
					QString tmp0x("");
					tmp0x.sprintf("%.3f",d0);
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
					const double d1 = get_distance2(x0_, y0_, x0_, y1_);
					QString tmp0y("");
					tmp0y.sprintf("%.3f",d1);
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
					path.moveTo(x0, y0);
					path.lineTo(x1, y0);
				}
				if (tmp1y)
				{
					path.moveTo(x0, y0);
					path.lineTo(x0, y1);
				}
			}
			else
			{
				path.moveTo(x0, y0);
				path.lineTo(x1, y1);
			}
			graphicsview->measurment_line->setPath(path);
			if (!data_type.isEmpty() && !tmp0.isEmpty())
				data_type.append(QString(" | "));
			measure_label->setText(data_type + tmp0);
		}
		else
		{
			QPainterPath path;
			graphicsview->measurment_line->setPath(path);
			measure_label->setText(data_type);
		}
	}
	else
	{
		double d = -1;
		switch(ivariant->image_type)
		{
		case 0:
			d = get_distance<ImageTypeSS>(ivariant->pSS, ivariant, a, x0, y0, x1, y1);
			break;
		case 1:
			d = get_distance<ImageTypeUS>(ivariant->pUS, ivariant, a, x0, y0, x1, y1);
			break;
		case 2:
			d = get_distance<ImageTypeSI>(ivariant->pSI, ivariant, a, x0, y0, x1, y1);
			break;
		case 3:
			d = get_distance<ImageTypeUI>(ivariant->pUI, ivariant, a, x0, y0, x1, y1);
			break;
		case 4:
			d = get_distance<ImageTypeUC>(ivariant->pUC, ivariant, a, x0, y0, x1, y1);
			break;
		case 5:
			d = get_distance<ImageTypeF>(ivariant->pF, ivariant, a, x0, y0, x1, y1);
			break;
		case 6:
			d = get_distance<ImageTypeD>(ivariant->pD, ivariant, a, x0, y0, x1, y1);
			break;
		case 7:
			d = get_distance<ImageTypeSLL>(ivariant->pSLL, ivariant, a, x0, y0, x1, y1);
			break;
		case 8:
			d = get_distance<ImageTypeULL>(ivariant->pULL, ivariant, a, x0, y0, x1, y1);
			break;
		case 10:
			d = get_distance<RGBImageTypeSS>(ivariant->pSS_rgb, ivariant, a, x0, y0, x1, y1);
			break;
		case 11:
			d = get_distance<RGBImageTypeUS>(ivariant->pUS_rgb, ivariant, a, x0, y0, x1, y1);
			break;
		case 12:
			d = get_distance<RGBImageTypeSI>(ivariant->pSI_rgb, ivariant, a, x0, y0, x1, y1);
			break;
		case 13:
			d = get_distance<RGBImageTypeUI>(ivariant->pUI_rgb, ivariant, a, x0, y0, x1, y1);
			break;
		case 14:
			d = get_distance<RGBImageTypeUC>(ivariant->pUC_rgb, ivariant, a, x0, y0, x1, y1);
			break;
		case 15:
			d = get_distance<RGBImageTypeF>(ivariant->pF_rgb, ivariant, a, x0, y0, x1, y1);
			break;
		case 16:
			d = get_distance<RGBImageTypeD>(ivariant->pD_rgb, ivariant, a, x0, y0, x1, y1);
			break;
		case 20:
			d = get_distance<RGBAImageTypeSS>(ivariant->pSS_rgba, ivariant, a, x0, y0, x1, y1);
			break;
		case 21:
			d = get_distance<RGBAImageTypeUS>(ivariant->pUS_rgba, ivariant, a, x0, y0, x1, y1);
			break;
		case 22:
			d = get_distance<RGBAImageTypeSI>(ivariant->pSI_rgba, ivariant, a, x0, y0, x1, y1);
			break;
		case 23:
			d = get_distance<RGBAImageTypeUI>(ivariant->pUI_rgba, ivariant, a, x0, y0, x1, y1);
			break;
		case 24:
			d = get_distance<RGBAImageTypeUC>(ivariant->pUC_rgba, ivariant, a, x0, y0, x1, y1);
			break;
		case 25:
			d = get_distance<RGBAImageTypeF>(ivariant->pF_rgba, ivariant, a, x0, y0, x1, y1);
			break;
		case 26:
			d = get_distance<RGBAImageTypeD>(ivariant->pD_rgba, ivariant, a, x0, y0, x1, y1);
			break;
		default:
			break;
		}
		if (d >= 0.0)
		{
			tmp0.sprintf("%.3f",d);
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
		measure_label->setText(tmp0);
	}
}

void GraphicsWidget::set_mouse_modus(short m, bool us_regions)
{
	mouse_modus = m;
	set_measure_text(QString(""));
	switch(mouse_modus)
	{
	case 1: // set slices
		{
			graphicsview->setTransformationAnchor(
				QGraphicsView::AnchorUnderMouse);
			graphicsview->setDragMode(QGraphicsView::NoDrag);
			graphicsview->setCursor(Qt::CrossCursor);
			if (main)
			{
				graphicsview->handle_rect->set_pen2(
					3.0/graphicsview->m_scale);
				graphicsview->set_handleitems_cursors(false);
			}
			graphicsview->measurment_line->hide();
			graphicsview->set_empty_distance();
			graphicsview->line_x->show();
			graphicsview->line_y->show();
			graphicsview->line_z->show();
			if (us_regions) graphicsview->draw_us_regions();
			else            graphicsview->clear_us_regions();
		}
		break;
	case 2: // measure distance
		{
			graphicsview->setTransformationAnchor(
				QGraphicsView::AnchorUnderMouse);
			graphicsview->setDragMode(QGraphicsView::NoDrag);
			graphicsview->setCursor(Qt::ArrowCursor);
			if (main)
			{
				graphicsview->handle_rect->set_pen2(
					3.0/graphicsview->m_scale);
				graphicsview->set_handleitems_cursors(false);
			}
			graphicsview->measurment_line->show();
			graphicsview->line_x->hide();
			graphicsview->line_y->hide();
			graphicsview->line_z->hide();
			graphicsview->set_empty_lines();
			graphicsview->draw_us_regions();
		}
		break;
	case 3: // edit ROIs
	case 4: // draw ROIs
	case 5: // plot
	case 6: // paint
	case 0: // default
	default:
		{
			graphicsview->setTransformationAnchor(
				QGraphicsView::AnchorUnderMouse);
			graphicsview->setCursor(Qt::OpenHandCursor);
			graphicsview->setDragMode(QGraphicsView::ScrollHandDrag);
			//graphicsview->unsetCursor();
			if (main)
			{
				graphicsview->handle_rect->set_pen1(
					3.0/graphicsview->m_scale);
				graphicsview->set_handleitems_cursors(true);
			}
			graphicsview->measurment_line->hide();
			graphicsview->line_x->hide();
			graphicsview->line_y->hide();
			graphicsview->line_z->hide();
			graphicsview->set_empty_distance();
			graphicsview->set_empty_lines();
			if (us_regions) graphicsview->draw_us_regions();
			else            graphicsview->clear_us_regions();
		}
		break;
	}
}

short GraphicsWidget::get_mouse_modus() const
{
	return mouse_modus;
}

void GraphicsWidget::set_show_cursor(bool t)
{
	show_cursor = t;
}

bool GraphicsWidget::get_show_cursor() const
{
	return show_cursor;
}

void GraphicsWidget::update_pixel_value(double x, double  y)
{
	if (x < 0 || y < 0 || !image_container.image3D)
	{
		info_line->setText("");
		return;
	}
	const int a = get_axis();
	const int lookup_id = image_container.image3D->di->lookup_id;
	const int sx = image_container.image3D->di->selected_x_slice;
	const int sy = image_container.image3D->di->selected_y_slice;
	const int sz = image_container.image3D->di->selected_z_slice;
	if (lookup_id >= 0)
	{
		const ImageVariant * v = aliza->get_image(lookup_id);
		if (
			v &&
			v->equi &&
			(v->di->idimx ==
				image_container.image3D->di->idimx) &&
			(v->di->idimy ==
				image_container.image3D->di->idimy) &&
			(v->di->idimz ==
				image_container.image3D->di->idimz) &&
			(v->orientation ==
				image_container.image3D->orientation) &&
			(v->di->ix_origin + 0.001 >
				image_container.image3D->di->ix_origin) &&
			(v->di->ix_origin - 0.001 <
				image_container.image3D->di->ix_origin) &&
			(v->di->iy_origin + 0.001 >
				image_container.image3D->di->iy_origin) &&
			(v->di->iy_origin - 0.001 <
				image_container.image3D->di->iy_origin) &&
			(v->di->iz_origin + 0.001 >
				image_container.image3D->di->iz_origin) &&
			(v->di->iz_origin - 0.001 <
				image_container.image3D->di->iz_origin))
		{
			const QString d = GraphicsUtils::get_scalar_pixel_value(
				v, a, x, y, sx, sy, sz, false);
			info_line->setText(d);
		}
		else
		{
			info_line->setText(QString(""));
		}
		return;
	}
	QString d;
	switch(image_container.image3D->image_type)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		d = GraphicsUtils::get_scalar_pixel_value(
			image_container.image3D, a, x, y, sx, sy, sz, false);
		break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		d = GraphicsUtils::get_rgb_pixel_value(
			image_container.image3D, a, x, y, sx, sy, sz);
		break;
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
		d = GraphicsUtils::get_rgba_pixel_value(
			image_container.image3D, a, x, y, sx, sy, sz);
		break;
	default :
		break;
	}
	info_line->setText(d);
}

void GraphicsWidget::update_pixel_value2(double x, double y)
{
	if (x < 0 || y < 0 || !image_container.image3D)
	{
		info_line->setText("");
		return;
	}
	const int a = get_axis();
	const int sx = image_container.image3D->di->selected_x_slice;
	const int sy = image_container.image3D->di->selected_y_slice;
	const int sz = image_container.image3D->di->selected_z_slice;
	QString d("");
	switch(image_container.image3D->image_type)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		d = GraphicsUtils::get_scalar_pixel_value(
			image_container.image3D, a, x, y, sx, sy, sz, true);
		break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		d = GraphicsUtils::get_rgb_pixel_value(
			image_container.image3D, a, x, y, sx, sy, sz);
		break;
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
		d = GraphicsUtils::get_rgba_pixel_value(
			image_container.image3D, a, x, y, sx, sy, sz);
		break;
	default :
		break;
	}
	info_line->setText(d);
}

void GraphicsWidget::set_frame_time_unit(bool t)
{
	if (t) frame_time_unit = 1;
	else   frame_time_unit = 0;
}

void GraphicsWidget::set_alt_mode(bool t)
{
	alt_mode = t;
	update_image(0, false, true);
}

bool GraphicsWidget::get_alt_mode() const
{
	return alt_mode;
}

QString GraphicsWidget::contours_from_selected_paths(
	ImageVariant * ivariant,
	ROI * roi)
{
	if (!ivariant) return QString("Image is NULL");
	if (!roi)      return QString("ROI is NULL");
	QString message("");
	int selected_items_size = 0;
	QList<long> tmp_ids;
	QList<QGraphicsItem*> selected_items;
	const bool lock = mutex.tryLock();
	if (!lock) goto quit__;
	selected_items = graphicsview->scene()->selectedItems();
	selected_items_size = selected_items.size();
	graphicsview->scene()->clearSelection();
	if (selected_items_size < 1)
	{
		message = QString("Nothing selected,\nclick at contour");
		goto quit__;
	}
	for (int x = 0; x < selected_items_size; x++)
	{
		QString s0 = QVariant(x+1).toString() + QString(": ");
		QString s1("");
		QGraphicsItem * gi = selected_items.at(x);
		GraphicsPathItem * p = static_cast<GraphicsPathItem*>(gi);
		if (!p) continue;
		const long tmp_id = p->get_tmp_id();
		if (tmp_id >= 0) tmp_ids.push_back(tmp_id);
		if (ivariant->equi)
		{
			switch(ivariant->image_type)
			{
			case  0: s1 = contour_from_path<ImageTypeSS>(roi, ivariant->pSS, p);
				break;
			case  1: s1 = contour_from_path<ImageTypeUS>(roi, ivariant->pUS, p); 
				break;
			case  2: s1 = contour_from_path<ImageTypeSI>(roi, ivariant->pSI, p);
				break;
			case  3: s1 = contour_from_path<ImageTypeUI>(roi, ivariant->pUI, p); 
				break;
			case  4: s1 = contour_from_path<ImageTypeUC>(roi, ivariant->pUC, p); 
				break;
			case  5: s1 = contour_from_path<ImageTypeF>(roi, ivariant->pF, p);
				break;
			case  6: s1 = contour_from_path<ImageTypeD>(roi, ivariant->pD, p); 
				break;
			case  7: s1 = contour_from_path<ImageTypeSLL>(roi, ivariant->pSLL, p); 
				break;
			case  8: s1 = contour_from_path<ImageTypeULL>(roi, ivariant->pULL, p); 
				break;
			case 10: s1 = contour_from_path<RGBImageTypeSS>(roi, ivariant->pSS_rgb, p);
				break;
			case 11: s1 = contour_from_path<RGBImageTypeUS>(roi, ivariant->pUS_rgb, p); 
				break;
			case 12: s1 = contour_from_path<RGBImageTypeSI>(roi, ivariant->pSI_rgb, p);
				break;
			case 13: s1 = contour_from_path<RGBImageTypeUI>(roi, ivariant->pUI_rgb, p); 
				break;
			case 14: s1 = contour_from_path<RGBImageTypeUC>(roi, ivariant->pUC_rgb, p); 
				break;
			case 15: s1 = contour_from_path<RGBImageTypeF>(roi, ivariant->pF_rgb, p);
				break;
			case 16: s1 = contour_from_path<RGBImageTypeD>(roi, ivariant->pD_rgb, p); 
				break;
			default: s1 = QString("Image type not supported");
				break;
			}
		}
		else
		{
			s1 = contour_from_path_nonuniform(roi, ivariant, p);
		}
		if (!s1.isEmpty()) message.append(s0 + s1 + QString("\n"));
	}
	graphicsview->clear_paths();
quit__:
	mutex.unlock();
	return message;
}

void GraphicsWidget::get_screen()
{
	QPixmap p;
	if (multi_frame_ptr && multi_frame_ptr->isVisible())
	{
		p = QPixmap::grabWidget(multi_frame_ptr);
	}
	else if (single_frame_ptr && single_frame_ptr->isVisible())
	{
		if (slider_m) slider_m->hide();
		qApp->processEvents();
		p = QPixmap::grabWidget(single_frame_ptr);
		if (slider_m) slider_m->show();
		qApp->processEvents();
	}
	if (p.isNull())
	{
		std::cout << "could not grab screen" << std::endl;
		return;
	}
	const QString saved_dir = CommonUtils::get_screenshot_dir();
	const QString d = CommonUtils::get_screenshot_name(saved_dir);
	const QString f = QFileDialog::getSaveFileName(
		NULL,
		QString(
			"Select file: format by extension"),
		d,
		QString("All Files (*)"),
		(QString*)NULL
		 //,QFileDialog::DontUseNativeDialog
		);
	if (f.isEmpty()) return;
	QString file;
	QFileInfo fi(f);
	CommonUtils::set_screenshot_dir(
		QDir::toNativeSeparators(
			fi.absolutePath()));
	QString ext = fi.suffix();
	if (ext.isEmpty())
	{
		file = QDir::toNativeSeparators(f + QString(".png"));
		ext = QString("PNG");
	}
	else
	{
		file = QDir::toNativeSeparators(f);
		ext = ext.trimmed().toUpper();
	}
	if (!p.save(file, ext.toLocal8Bit().constData()))
	{
		QMessageBox mbox;
		mbox.setWindowModality(Qt::ApplicationModal);
		mbox.addButton(QMessageBox::Close);
		mbox.setIcon(QMessageBox::Warning);
		mbox.setText(QString("Could not save file"));
		mbox.exec();
	}
}

int GraphicsWidget::get_frametime_2D() const
{
	return frametime_2D;
}

void GraphicsWidget::set_frametime_2D(int x)
{
	frametime_2D = x;
}

void GraphicsWidget::show_image_info()
{
	if (!image_container.image3D) return;
	const ImageVariant * v = image_container.image3D;
	GraphicsUtils::print_image_info(v);
}

void GraphicsWidget::set_contours_width(double x)
{
	contours_width = x;
	if (image_container.image3D)
		draw_contours(image_container.image3D, this);
}

double GraphicsWidget::get_contours_width() const
{
	return contours_width;
}

bool GraphicsWidget::get_enable_shutter() const
{
	return enable_shutter;
}

bool GraphicsWidget::get_enable_overlays() const
{
	return enable_overlays;
}

void GraphicsWidget::set_enable_shutter(bool t)
{
	enable_shutter = t;
}

void GraphicsWidget::set_enable_overlays(bool t)
{
	enable_overlays = t;
}

