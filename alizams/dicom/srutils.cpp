#define TMP_IMAGE_IN_MEMORY

#include "structures.h"
#include "srutils.h"
#include "dicomutils.h"
#include "codecutils.h"
#include "commonutils.h"
#include <mdcmItem.h>
#include <mdcmSmartPointer.h>
#include <mdcmDataElement.h>
#include <mdcmSequenceOfItems.h>
#include <mdcmUIDs.h>
#include <QtGlobal>
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QUrl>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include "processimagethreadLUT.hxx"
#include "settingswidget.h"
#include "itkExtractImageFilter.h"
#include "itkMath.h"

template<typename Tin, typename Tout> QString gs3(
	const typename Tin::Pointer & image,
	typename Tout::Pointer & out_image,
	int idx)
{
	if (image.IsNull())
		return QString("gs3<>() : image.IsNull()");
	typedef itk::ExtractImageFilter<Tin, Tout> FilterType;
	typename FilterType::Pointer filter = FilterType::New();
	const typename Tin::RegionType inRegion =
		image->GetLargestPossibleRegion();
	const typename Tin::SizeType size = inRegion.GetSize();
	typename Tin::IndexType index = inRegion.GetIndex();
	typename Tin::RegionType outRegion;
	typename Tin::SizeType out_size;
	if (idx >= (int)size[2])
	{
		return QString("gs3<>() : invalid index");
	}
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
	else return QString("Output image is NULL");
	return QString("");
}

void srImageCleanupHandler(void * info)
{
	if (!info) return;
	unsigned char * p = (unsigned char*)info;
	delete [] p;
	info = NULL;
}

template<typename T> SRImage li3(
	const typename T::Pointer & image,
	ImageVariant * ivariant)
{
	if (image.IsNull()||!ivariant) return SRImage();
	const typename T::SpacingType spacing = image->GetSpacing();
	const typename T::RegionType region   = image->GetLargestPossibleRegion();
	const typename T::SizeType size       = region.GetSize();
	const short lut = 0;
	const int lut_function = 0;
	const unsigned int p_size = 3*size[0]*size[1];
	unsigned char * p = NULL;
	try { p = new unsigned char[p_size]; }
	catch (std::bad_alloc&) { p = NULL; }
	if (!p) return SRImage();
	//
	std::vector<QThread*> threadsLUT_;
	const int num_threads = QThread::idealThreadCount();
	const int tmp99 = size[1]%num_threads;
	const double center = ivariant->di->us_window_center;
	const double width  = ivariant->di->us_window_width;
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
						center, width,
						lut, false,lut_function);
			j += 3*(size_0*size_1);
			threadsLUT_.push_back(static_cast<QThread*>(t__));
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
							center, width,
							lut, false,lut_function);
				j += 3*(size_0*block);
				threadsLUT_.push_back(static_cast<QThread*>(t__));
				t__->start();
			}
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(image,
						p,
						size[0],  tmp100,
						0, incr*block, j,
						ivariant->di->us_window_center, ivariant->di->us_window_width,
						lut, false,lut_function);
			threadsLUT_.push_back(static_cast<QThread*>(lt__));
			lt__->start();
		}
		else
		{
			ProcessImageThreadLUT_<T> * lt__ = new ProcessImageThreadLUT_<T>(image,
						p,
						size[0],  size[1],
						0, 0, 0,
						center, width,
						lut, false,lut_function);
			threadsLUT_.push_back(static_cast<QThread*>(lt__));
			lt__->start();
		}
	}
	//
	const unsigned short threadsLUT_size = threadsLUT_.size();
	while (true)
	{
		unsigned short b__ = 0;
		for (int i=0; i < threadsLUT_size; i++)
		{
			if (threadsLUT_.at(i)->isFinished()) { b__++; }
		}
		if (b__==threadsLUT_size) break;
	}
	for (int i=0; i < threadsLUT_size; i++)
	{
		delete threadsLUT_[i];
	}
	threadsLUT_.clear();
	//
	SRImage sr;
	sr.sx = spacing[0];
	sr.sy = spacing[1];
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	sr.i = QImage(p,size[0],size[1],3*size[0],QImage::Format_RGB888,srImageCleanupHandler,p);
#else
	sr.i = QImage(p,size[0],size[1],3*size[0],QImage::Format_RGB888);
	sr.p = p;
#endif
	return sr;
}

template<typename T> SRImage lrgb3(
	const typename T::Pointer & image,
	ImageVariant * ivariant)
{
	if (!ivariant||image.IsNull()) return SRImage();
	const typename T::RegionType region = image->GetLargestPossibleRegion();
	const typename T::SizeType size = region.GetSize();
	const typename T::SpacingType spacing = image->GetSpacing();
	unsigned char * p__  = NULL;
	unsigned int j_ = 0;
	const unsigned short bits_allocated   = ivariant->di->bits_allocated;
	const unsigned short bits_stored      = ivariant->di->bits_stored;
	const unsigned short high_bit         = ivariant->di->high_bit;
	if (ivariant->image_type == 11)
	{
		const double tmp_max
			= ((bits_allocated > 0 && bits_stored > 0) &&
				bits_stored < bits_allocated &&
				(high_bit==bits_stored-1))
				? pow(2, bits_stored) - 1 : static_cast<double>(USHRT_MAX);
		try { p__ = new unsigned char[size[0] * size[1] * 3]; }
		catch (std::bad_alloc&) { p__ = NULL; }
		if (!p__) return SRImage();
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
		if (!p__) return SRImage();
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
	SRImage sr;
	sr.sx = spacing[0];
	sr.sy = spacing[1];
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	sr.i = QImage(p__,size[0],size[1],3*size[0],QImage::Format_RGB888,srImageCleanupHandler,p__);
#else
	sr.i = QImage(p__,size[0],size[1],3*size[0],QImage::Format_RGB888);
	sr.p = p__;
#endif
	return sr;
}

void SRUtils::read_IMAGE(
	const mdcm::DataSet & nds,
	const QString & charset,
	const QString & path,
	QString & s,
	QStringList & tmpfiles,
	std::vector<SRImage> & srimages,
	QTextEdit * textEdit,
	const std::vector<SRGraphic> & grobjects,
	bool info,
	const QWidget * wsettings,
	QProgressDialog * pb)
{
	QString tmpfile("");
	const mdcm::DataElement & e8  =
		nds.GetDataElement(mdcm::Tag(0x0008,0x1199));
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq8 =
		e8.GetValueAsSQ();
	if (!sq8) return;
	const SettingsWidget * settings =
		static_cast<const SettingsWidget*>(wsettings);
	const bool skip_images = settings->get_sr_skip_images();
	const unsigned int nitems8 = sq8->GetNumberOfItems();
	for(unsigned int i8 = 0; i8 < nitems8; ++i8)
	{
		const mdcm::Item & item8 = sq8->GetItem(i8+1);
		const mdcm::DataSet & nds8 =
			item8.GetNestedDataSet();
		QString ReferencedSOPClassUID;
		if (DicomUtils::get_string_value(
				nds8,
				mdcm::Tag(0x0008,0x1150),
				ReferencedSOPClassUID))
		{
			mdcm::UIDs uid;
			uid.SetFromUID(
				ReferencedSOPClassUID
					.toLatin1()
					.constData());
			QString uidname =
				QString::fromLatin1(uid.GetName());
			if (!uidname.isEmpty())
			{
				uidname = uidname.trimmed();
				if (info)
				{
					s += QString("<span class='y3'>IMAGE: ") +
						uidname +
						QString("</span><br />");
				}
			}
			else
			{
				ReferencedSOPClassUID =
					ReferencedSOPClassUID.trimmed();
				if (info)
				{
					s += QString("<span class='y3'>IMAGE: ") +
						ReferencedSOPClassUID +
						QString("</span><br />");
				}
			}
		}
		QString ReferencedSOPInstanceUID;
		if (DicomUtils::get_string_value(
				nds8,
				mdcm::Tag(0x0008,0x1155),
				ReferencedSOPInstanceUID))
		{
			if (info)
			{
				s += QString("<span class='y3'>IMAGE: ") +
					ReferencedSOPInstanceUID.trimmed() +
					QString("</span><br />");
			}
			else if (skip_images)
			{
				s += QString("<span class='yy'>IMAGE: ") +
					ReferencedSOPInstanceUID.trimmed() +
					QString("</span><br />");
			}
			if (skip_images) continue;
			int idx = 0;
			std::vector<int> refframes;
			if (DicomUtils::get_is_values(
				nds8,
				mdcm::Tag(0x0008,0x1160),
				refframes))
			{
				if (refframes.size() == 1)
				{
					const int refframe = refframes.at(0);
					if (refframe >= 1) idx = refframe - 1;
				}
			}
			QString sf = DicomUtils::find_file_from_uid(
				QDir::toNativeSeparators(path),
				ReferencedSOPInstanceUID.trimmed());
			if (sf.isEmpty())
			{
				// FIXME
				sf = DicomUtils::find_file_from_uid(
					QDir::toNativeSeparators(path+QString("/..")),
					ReferencedSOPInstanceUID);
			}
			if (info)
			{
				s += QString("<span class='y3'>IMAGE: ") +
					QDir::toNativeSeparators(sf) +
					QString("</span><br />");
			}
			std::vector<ImageVariant*> ivariants;
			std::vector<ImageVariant2D*> ivariants2;
			QString e_ = DicomUtils::read_dicom(
				ivariants,
				QStringList(sf),
				0,
				NULL,
				NULL,
				false,
				wsettings,
				pb,
				3,
				true);
			if (e_.isEmpty() && ivariants.size()==1 && ivariants.at(0))
			{
				ImageVariant * v = ivariants[0];
				ImageVariant2D * v2 = new ImageVariant2D();
				CommonUtils::get_dimensions_(v);
				CommonUtils::calculate_minmax_scalar(v);
				switch(v->image_type)
				{
				case 0:
					e_=gs3<ImageTypeSS,Image2DTypeSS>(v->pSS,v2->pSS,idx);
					break;
				case 1:
					e_=gs3<ImageTypeUS,Image2DTypeUS>(v->pUS,v2->pUS,idx);
					break;
				case 2:
					e_=gs3<ImageTypeSI,Image2DTypeSI>(v->pSI,v2->pSI,idx);
					break;
				case 3:
					e_=gs3<ImageTypeUI,Image2DTypeUI>(v->pUI,v2->pUI,idx);
					break;
				case 4:
					e_=gs3<ImageTypeUC,Image2DTypeUC>(v->pUC,v2->pUC,idx);
					break;
				case 5:
					e_=gs3<ImageTypeF,Image2DTypeF>(v->pF,v2->pF,idx);
					break;
				case 6:
					e_=gs3<ImageTypeD,Image2DTypeD>(v->pD,v2->pD,idx);
					break;
				case 7:
					e_=gs3<ImageTypeSLL,Image2DTypeSLL>(v->pSLL,v2->pSLL,idx);
					break;
				case 8:
					e_=gs3<ImageTypeULL,Image2DTypeULL>(v->pULL,v2->pULL,idx);
					break;
				case 10:
					e_=gs3<RGBImageTypeSS,RGBImage2DTypeSS>(v->pSS_rgb,v2->pSS_rgb,idx);
					break;
				case 11:
					e_=gs3<RGBImageTypeUS,RGBImage2DTypeUS>(v->pUS_rgb,v2->pUS_rgb,idx);
					break;
				case 12:
					e_=gs3<RGBImageTypeSI,RGBImage2DTypeSI>(v->pSI_rgb,v2->pSI_rgb,idx);
					break;
				case 13:
					e_=gs3<RGBImageTypeUI,RGBImage2DTypeUI>(v->pUI_rgb,v2->pUI_rgb,idx);
					break;
				case 14:
					e_=gs3<RGBImageTypeUC,RGBImage2DTypeUC>(v->pUC_rgb,v2->pUC_rgb,idx);
					break;
				case 15:
					e_=gs3<RGBImageTypeF,RGBImage2DTypeF>(v->pF_rgb,v2->pF_rgb,idx);
					break;
				case 16:
					e_=gs3<RGBImageTypeD,RGBImage2DTypeD>(v->pD_rgb,v2->pD_rgb,idx);
					break;
				default:
					e_ = QString("image type not supported");
					break;
				}
				ivariants2.push_back(v2);
				if (e_.isEmpty())
				{
					v2->image_type = v->image_type;
					SRImage pm;
					switch(v2->image_type)
					{
					case 0: pm=li3<Image2DTypeSS>(v2->pSS,v);
						break;
					case 1: pm=li3<Image2DTypeUS>(v2->pUS,v);
						break;
					case 2: pm=li3<Image2DTypeSI>(v2->pSI,v);
						break;
					case 3: pm=li3<Image2DTypeUI>(v2->pUI,v);
						break;
					case 4: pm=li3<Image2DTypeUC>(v2->pUC,v);
						break;
					case 5: pm=li3<Image2DTypeF>(v2->pF,v);
						break;
					case 6: pm=li3<Image2DTypeD>(v2->pD,v);
						break;
					case 7: pm=li3<Image2DTypeSLL>(v2->pSLL,v);
						break;
					case 8: pm=li3<Image2DTypeULL>(v2->pULL,v);
						break;
					case 10: pm=lrgb3<RGBImage2DTypeSS>(v2->pSS_rgb,v);
						break;
					case 11: pm=lrgb3<RGBImage2DTypeUS>(v2->pUS_rgb,v);
						break;
					case 12: pm=lrgb3<RGBImage2DTypeSI>(v2->pSI_rgb,v);
						break;
					case 13: pm=lrgb3<RGBImage2DTypeUI>(v2->pUI_rgb,v);
						break;
					case 14: pm=lrgb3<RGBImage2DTypeUC>(v2->pUC_rgb,v);
						break;
					case 15: pm=lrgb3<RGBImage2DTypeF>(v2->pF_rgb,v);
						break;
					case 16: pm=lrgb3<RGBImage2DTypeD>(v2->pD_rgb,v);
						break;
					default:
						break;
					}
/*
POINT
a single pixel denoted by a single (column,row) pair

MULTIPOINT
multiple pixels each denoted by an (column,row) pair

POLYLINE
a series of connected line segments with ordered vertices
denoted by (column,row) pairs; if the first and last
vertices are the same it is a closed polygon

CIRCLE
a circle defined by two (column,row) pairs. The first
point is the central pixel. The second point is a
pixel on the perimeter of the circle

ELLIPSE
an ellipse defined by four pixel (column,row) pairs,
the first two points specifying the endpoints of the
major axis and the second two points specifying the
endpoints of the minor axis of an ellipse
*/
					if (!grobjects.empty())
					{
						QPen pen;
						pen.setBrush(QBrush(QColor(255,0,0)));
						QBrush brush(QColor(255,0,0));
						brush.setStyle(Qt::SolidPattern);
						for (size_t yy = 0; yy < grobjects.size(); yy++)
						{
							const SRGraphic & sg = grobjects.at(yy);
							const size_t gsize = sg.GraphicData.size();
							if ((gsize < 2) || (gsize%2 != 0)) continue;
							if (sg.GraphicType == QString("POLYLINE"))
							{
#if 1
								bool check_single_point = true;
								double tmp__0 = sg.GraphicData.at(0);
								double tmp__1 = sg.GraphicData.at(1);
								for (size_t yyy = 2; yyy < gsize; yyy+=2)
								{
									if (
										(!itk::Math::FloatAlmostEqual<float>(
											(float)tmp__0,
											(float)sg.GraphicData.at(yyy))) ||
										(!itk::Math::FloatAlmostEqual<float>(
											(float)tmp__1,
											(float)sg.GraphicData.at(yyy+1)))
										)
									{
										check_single_point = false;
										break;
									}
									tmp__0 = sg.GraphicData.at(yyy  );
									tmp__1 = sg.GraphicData.at(yyy+1);
								}
								// workaround error
								if (gsize == 2 || check_single_point)
								{
									QPainter painter;
									painter.begin(&(pm.i));
									painter.setPen(QPen(Qt::NoPen));
									painter.setBrush(brush);
									painter.drawEllipse(
										sg.GraphicData.at(0) - 3,
										sg.GraphicData.at(1) - 3,
										6,
										6);
									painter.end();
								}
								else
#endif
								{
									QPainterPath pp;
									pp.moveTo(
										sg.GraphicData.at(0),
										sg.GraphicData.at(1));
									for (size_t yyy = 2;
										yyy < gsize;
										yyy+=2)
									{
										pp.lineTo(
											sg.GraphicData.at(yyy),
											sg.GraphicData.at(yyy+1));
									}
									QPainter painter;
									painter.begin(&(pm.i));
									painter.setPen(pen);
									painter.drawPath(pp);
									painter.end();
								}
							}
							else if (
								sg.GraphicType == QString("POINT") ||
								sg.GraphicType == QString("MULTIPOINT"))
							{
								QPainter painter;
								painter.begin(&(pm.i));
								painter.setPen(QPen(Qt::NoPen));
								painter.setBrush(brush);
								for (size_t yyy = 0; yyy < gsize; yyy+=2)
								{
									painter.drawEllipse(
										sg.GraphicData.at(yyy    ) - 3,
										sg.GraphicData.at(yyy + 1) - 3,
										6,
										6);
								}
								painter.end();
							}
							else if (sg.GraphicType == QString("CIRCLE"))
							{
								const double center_x =
									sg.GraphicData.at(0);
								const double center_y =
									sg.GraphicData.at(1);
								const double point_x  =
									sg.GraphicData.at(2);
								const double point_y  =
									sg.GraphicData.at(3);
								const double x__ = point_x - center_x;
								const double y__ = point_y - center_y;
								const double distance =
									sqrt(x__*x__ + y__*y__);
								QPainter painter;
								painter.begin(&(pm.i));
								painter.setPen(pen);
								painter.drawEllipse(
									center_x - distance,
									center_y - distance,
									2*distance,
									2*distance);
								painter.end();
							}
							else if (sg.GraphicType == QString("ELLIPSE"))
							{
								const float  major0_x = sg.GraphicData.at(0);
								const float  major0_y = sg.GraphicData.at(1);
								const float  major1_x = sg.GraphicData.at(2);
								const float  major1_y = sg.GraphicData.at(3);
								const float  minor0_x = sg.GraphicData.at(4);
								const float  minor0_y = sg.GraphicData.at(5);
								const float  minor1_x = sg.GraphicData.at(6);
								const float  minor1_y = sg.GraphicData.at(7);
								const double mid_major_x = (major0_x + major1_x)*0.5;
								const double mid_major_y = (major0_y + major1_y)*0.5;
								const double x0__ = major1_x - major0_x;
								const double y0__ = major1_y - major0_y;
								const double x1__ = minor1_x - minor0_x;
								const double y1__ = minor1_y - minor0_y;
								const double d0   = sqrt(x0__*x0__ + y0__*y0__);
								const double d1   = sqrt(x1__*x1__ + y1__*y1__);
								const double ma_j = 1.0/d0;
								const double ma_nx = x0__ * ma_j;
								const double ma_ny = y0__ * ma_j;
								const double mi_j = 1.0/d1;
								const double mi_nx = x1__ * mi_j;
								const double mi_ny = y1__ * mi_j;
								const double start = 0.0;
								const double span  = 360.0;
								QPainter painter;
								painter.begin(&(pm.i));
								painter.setPen(pen);
								{
									painter.save();
									painter.setBrush(Qt::NoBrush);
									{
										QTransform ttt;
										ttt.translate(mid_major_x, mid_major_y);
										painter.setTransform(ttt, true);
									}
									{
										QTransform ttt;
										ttt.setMatrix(ma_nx, ma_ny, 0, -mi_nx, -mi_ny, 0, 0, 0, 1);
										painter.setTransform(ttt, true);
									}
									QRectF r___(-0.5*d0, -0.5*d1, d0, d1);
									QPainterPath path;
									path.arcMoveTo(r___, start);
									path.arcTo(r___, start, span);
									painter.drawPath(path);
									painter.restore();
								}
								painter.end();
							}
							else // error
							{
								s += QString("<span class='red2'>") +
									sg.GraphicType +
									QString(" skipped</span><br />");
							}
						}
					}
					if (!itk::Math::FloatAlmostEqual<float>(
						(float)pm.sx,
						(float)pm.sy))
					{
						double coeff_size_0 = 1.0;
						double coeff_size_1 = 1.0;
						if (pm.sy > pm.sx) coeff_size_1 = pm.sy / pm.sx;
						else               coeff_size_0 = pm.sx / pm.sy;
						const double ix = pm.i.width()  * coeff_size_0;
						const double iy = pm.i.height() * coeff_size_1;
						QImage si = pm.i.scaled(
							(int)(ix + 0.5),
							(int)(iy + 0.5),
							Qt::IgnoreAspectRatio,
							Qt::SmoothTransformation);
						pm.i = si;
					}
					const int max_width = settings->get_sr_image_width();
					if (max_width >= 64 && pm.i.width() > max_width)
					{
						QImage si = pm.i.scaledToWidth(
							max_width,
							Qt::SmoothTransformation);
						pm.i = si;
					}
#ifdef TMP_IMAGE_IN_MEMORY
					tmpfile =
						QString("i")+
						QVariant(
							(qulonglong)
								QDateTime::currentMSecsSinceEpoch())
									.toString();
					textEdit->document()->addResource(
						QTextDocument::ImageResource,
						QUrl(tmpfile),
						pm.i);
					srimages.push_back(pm);
#else
					tmpfile = QDir::toNativeSeparators(
						QDir::tempPath() +
						QString("/i")+
						QVariant(
							(qulonglong)
								QDateTime::currentMSecsSinceEpoch())
									.toString() +
						QString(".png"));
					pm.i.save(tmpfile);
					tmpfiles.push_back(tmpfile);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
					pm.i = QImage();
					if (pm.p) delete [] pm.p;
#endif
#endif
					s += QString("<p style=\"margin-left: 0px\">") +
						QString("<img src=\"") + tmpfile +
						QString("\" /></p>");
				}
				else
				{
					s += QString("<span class='yy'>") +
						QString("IMAGE: ") + ReferencedSOPInstanceUID +
						QString(
							"</span><br /><span class='red2'>"
							"IMAGE: error (1) ") +
						e_.trimmed() +
						QString("</span><br />");
				}
			}
			else
			{
				s += QString("<span class='yy'>") +
					QString("IMAGE: ") + ReferencedSOPInstanceUID +
					QString(
						"</span><br /><span class='red2'>"
						"IMAGE: error (2) ") +
					e_.trimmed() +
					QString("</span><br />");
			}
			for (size_t yy = 0; yy < ivariants.size(); yy++)
			{
				if (ivariants.at(yy)) delete ivariants[yy];
			}
			ivariants.clear();
			for (size_t yy = 0; yy < ivariants2.size(); yy++)
			{
				if (ivariants2.at(yy)) delete ivariants2[yy];
			}
			ivariants2.clear();
		}
	}
}

bool SRUtils::read_SCOORD(
	const mdcm::DataSet & nds,
	const QString & charset,
	const QString & path,
	QString & s,
	QStringList & tmpfiles,
	std::vector<SRImage> & srimages,
	QTextEdit * textEdit,
	bool info,
	const QWidget * wsettings,
	QProgressDialog * pb)
{
	const SettingsWidget * settings =
		static_cast<const SettingsWidget*>(wsettings);
	const bool skip_images = settings->get_sr_skip_images();
	QString GraphicType;
	if (DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0070,0x0023),
			GraphicType))
	{
		GraphicType = GraphicType.trimmed();
		s += QString("<span class='yy'>") +
			GraphicType +
			QString("</span><br />");
	}
	std::vector<float> GraphicData;
	DicomUtils::get_fl_values(
		nds,
		mdcm::Tag(0x0070,0x0022),
		GraphicData);
	QString PixelOriginInterpretation;
	if (DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0048,0x0301),
			PixelOriginInterpretation))
	{
		PixelOriginInterpretation =
			PixelOriginInterpretation
				.trimmed();
		if (info)
		{
			s += QString("<span class='y3'>") +
				PixelOriginInterpretation +
				QString("</span><br />");
		}
	}
	QString FiducialUID;
	if (DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0070,0x031A),
			FiducialUID))
	{
		FiducialUID = FiducialUID.trimmed();
		if (info)
		{
			s += QString("<span class='y3'>") +
				FiducialUID +
				QString("</span><br />");
		}
	}

/////////////////////
	size_t other_ = 0;
	if (nds.FindDataElement(mdcm::Tag(0x0040,0xa730)))
	{
		const mdcm::DataElement & e  =
			nds.GetDataElement(mdcm::Tag(0x0040,0xa730));
		mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
			e.GetValueAsSQ();
		if (sq)
		{
			const unsigned int nitems = sq->GetNumberOfItems();
			for(unsigned int i = 0; i < nitems; ++i)
			{
				QString ValueType;
				QString RelationshipType;
				const mdcm::Item & item = sq->GetItem(i+1);
				const mdcm::DataSet & nds1 = item.GetNestedDataSet();
				if (DicomUtils::get_string_value(
					nds1, mdcm::Tag(0x0040,0xa040), ValueType))
				{
					ValueType = ValueType.trimmed().toUpper();
				}
				if (DicomUtils::get_string_value(
					nds1, mdcm::Tag(0x0040,0xa010), RelationshipType))
				{
					RelationshipType = RelationshipType.trimmed().toUpper();
				}
				if (!skip_images &&
					ValueType == QString("IMAGE")
#if 1
					&& RelationshipType == QString("SELECTED FROM")
#endif
					)
				{
					std::vector<SRGraphic> tmp001;
					SRGraphic sg;
					sg.GraphicType = GraphicType;
					sg.PixelOriginInterpretation =
						PixelOriginInterpretation;
					sg.FiducialUID = FiducialUID;
					sg.GraphicData = GraphicData;
					tmp001.push_back(sg);
					read_IMAGE(
						nds1,
						charset,
						path,
						s,
						tmpfiles,
						srimages,
						textEdit,
						tmp001,
						info,
						wsettings,
						pb);	
				}
				else
				{
					other_++;
				}
				if (nds1.FindDataElement(mdcm::Tag(0x0040,0xa730)))
				{
					other_++;
				}
			}
		}
	}
	if (other_ == 0)
	{
		return true;
	}
	return false;
}

void SRUtils::read_PNAME(
	const mdcm::DataSet & nds,
	const QString & charset,
	QString & s)
{
	if(!nds.FindDataElement(mdcm::Tag(0x0040,0xa123)))
		return;
	const mdcm::DataElement & e2 =
		nds.GetDataElement(mdcm::Tag(0x0040,0xa123));
	if (
		!e2.IsEmpty() &&
		!e2.IsUndefinedLength() &&
		e2.GetByteValue())
	{
		QByteArray ba(
			e2.GetByteValue()->GetPointer(),
			e2.GetByteValue()->GetLength());
		QString TextValue =
			CodecUtils::toUTF8(
				&ba,
				charset.toLatin1().constData());
		TextValue = TextValue.trimmed();
		s += QString("<span class='y'>") +
			DicomUtils::convert_pn_value(TextValue) +
			QString("</span><br />");
	}
}

void SRUtils::read_TEXT(
	const mdcm::DataSet & nds,
	const QString & charset,
	QString & s)
{
	if(!nds.FindDataElement(mdcm::Tag(0x0040,0xa160)))
		return;
	const mdcm::DataElement & e2 =
		nds.GetDataElement(mdcm::Tag(0x0040,0xa160));
	if (
		!e2.IsEmpty() &&
		!e2.IsUndefinedLength() &&
		e2.GetByteValue())
	{
		QByteArray ba(
			e2.GetByteValue()->GetPointer(),
			e2.GetByteValue()->GetLength());
		const QString TextValue =
			CodecUtils::toUTF8(
				&ba, charset.toLatin1().constData());
		s += QString("<span class='y'>") +
			TextValue.trimmed() +
			QString("</span><br />");
	}
}

void SRUtils::read_NUM(
	const mdcm::DataSet & nds,
	const QString & charset,
	QString & s)
{
	if (!nds.FindDataElement(mdcm::Tag(0x0040,0xa300)))
		return;
	const mdcm::DataElement & e3  =
		nds.GetDataElement(mdcm::Tag(0x0040,0xa300));
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq3 =
		e3.GetValueAsSQ();
	if (!sq3) return;
	for(unsigned int i3 = 0;
		i3 < sq3->GetNumberOfItems();
		i3++)
	{
		const mdcm::Item & item3 = sq3->GetItem(i3+1);
		const mdcm::DataSet & nds3 =
			item3.GetNestedDataSet();
		QString NumericValue;
		if (DicomUtils::get_string_value(
				nds3,
				mdcm::Tag(0x0040,0xa30a),
				NumericValue))
		{
			QString unit("");
			if (nds3.FindDataElement(
					mdcm::Tag(0x0040,0x08ea)))
			{
				const mdcm::DataElement & e5  =
					nds3.GetDataElement(
						mdcm::Tag(0x0040,0x08ea));
				mdcm::SmartPointer<mdcm::SequenceOfItems>
					sq5 =
						e5.GetValueAsSQ();
				if (sq5 && (sq5->GetNumberOfItems()==1))
				{
					const mdcm::Item & item5 =
						sq5->GetItem(1);
					const mdcm::DataSet & nds5 =
						item5.GetNestedDataSet();
					if (nds5.FindDataElement(
							mdcm::Tag(0x0008,0x0104)))
					{
						const mdcm::DataElement & e7  =
							nds5.GetDataElement(
								mdcm::Tag(0x0008,0x0104));
						if (
							!e7.IsEmpty() &&
							!e7.IsUndefinedLength() &&
							e7.GetByteValue())
						{
							QByteArray ba7(
								e7.GetByteValue()->
									GetPointer(),
								e7.GetByteValue()->
									GetLength());
							const QString tmp5 =
								CodecUtils::toUTF8(
									&ba7,
									charset
										.toLatin1()
										.constData());
							unit = tmp5
								.trimmed();
						}
					}
					if (nds5.FindDataElement(
							mdcm::Tag(0x0008,0x0100)))
					{
						const mdcm::DataElement & e7  =
							nds5.GetDataElement(
								mdcm::Tag(0x0008,0x0100));
						if (
							!e7.IsEmpty() &&
							!e7.IsUndefinedLength() &&
							e7.GetByteValue())
						{
							QByteArray ba7(
								e7.GetByteValue()->
									GetPointer(),
								e7.GetByteValue()->
									GetLength());
							const QString tmp5 =
								CodecUtils::toUTF8(
									&ba7,
									charset
										.toLatin1()
										.constData());
							if (unit.trimmed().toUpper() !=
									QString("NO UNITS"))
							{
								unit += QString(" (") +
									tmp5.trimmed() + QString(")");
							}
						}
					}
				}
			}
			const QString u =
				(unit.trimmed().toUpper() !=
					QString("NO UNITS"))
				? unit : QString("");
			s += QString("<span class='y'>") +
				NumericValue.trimmed() +
				QString(" ") + u +
				QString("</span><br />");
		}
	}
}

void SRUtils::read_CODE(
	const mdcm::DataSet & nds,
	const QString & charset,
	QString & s)
{
	if (!nds.FindDataElement(mdcm::Tag(0x0040,0xa168))) return;
	QString CodeMeaning2;
	const mdcm::DataElement & e4  =
		nds.GetDataElement(mdcm::Tag(0x0040,0xa168));
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq4 =
		e4.GetValueAsSQ();
	if (sq4)
	{
		const unsigned int nitems4 = sq4->GetNumberOfItems();
		for(unsigned int i4 = 0; i4 < nitems4; ++i4)
		{
			const mdcm::Item & item4 = sq4->GetItem(i4+1);
			const mdcm::DataSet & nds4 =
				item4.GetNestedDataSet();
			if (nds4.FindDataElement(mdcm::Tag(0x0008,0x0104)))
			{
				const mdcm::DataElement & e5 =
					nds4.GetDataElement(
						mdcm::Tag(0x0008,0x0104));
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
							charset.toLatin1().constData());
					if (!CodeMeaning2.isEmpty())
					{
						CodeMeaning2 += QString(" ");
					}
					CodeMeaning2 += tmp5.trimmed();
				}
			}
		}
	}
	if (!CodeMeaning2.isEmpty())
	{
		s += QString("<span class='y'>") +
			CodeMeaning2 +
			QString("</span><br />");
	}
}

void SRUtils::read_DATE(const mdcm::DataSet & nds, QString & s)
{
	QString Date;
	if (DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0040,0xa121),
			Date))
	{
		Date = Date.trimmed();
		const QDate date_ =
			QDate::fromString(Date, QString("yyyyMMdd"));
		s += QString("<span class='y'>") +
			date_.toString(QString("d MMM yyyy")) +
			QString("</span><br />");
	}
}

void SRUtils::read_DATETIME(const mdcm::DataSet & nds, QString & s)
{
	QString DateTime;
	if (!DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0040,0xa120),
			DateTime)) return;
	DateTime = DateTime.trimmed();
	QString tmp0;
	if (!DateTime.isEmpty())
	{
		const int point_idx =
			DateTime.indexOf(QString("."));
		if (point_idx == 14||point_idx == -1)
		{
			const QString time1_s =
				DateTime.left(14);
			const QDateTime time_ =
				QDateTime::fromString(
					time1_s,
					QString("yyyyMMddHHmmss"));
			tmp0 = time_.toString(
				QString("d MMM yyyy HH:mm:ss"));
			if (point_idx == 14)
			{
				tmp0.append(QString(".") +
					DateTime.right(
						DateTime.length()-15));
			}
		}
		else
		{
			tmp0 = DateTime;
		}
	}
	s += QString("<span class='y'>") +
		tmp0 + QString("</span><br />");
}

void SRUtils::read_TIME(const mdcm::DataSet & nds, QString & s)
{
	QString Time;
	if (!DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0040,0xa122),
			Time)) return;
	Time = Time.trimmed();
	QString tmp0;
	if (!Time.isEmpty())
	{
		const int point_idx =
			Time.indexOf(QString("."));
		if (point_idx == 6||point_idx == -1)
		{
			const QString time1_s =
				Time.left(6);
			const QDateTime time_ =
				QDateTime::fromString(
					time1_s,
					QString("HHmmss"));
			tmp0 = time_.toString(
				QString("HH:mm:ss"));
			if (point_idx == 6)
			{
				tmp0.append(QString(".") +
					Time.right(
						Time.length()-7));
			}
		}
		else
		{
			tmp0 = Time;
		}
	}
	s += QString("<span class='y'>") +
		tmp0 + QString("</span><br />");
}

void SRUtils::read_SCOORD3D(const mdcm::DataSet & nds, QString & s)
{
	// TODO
	QString GraphicType;
	if (DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0070,0x0023),
			GraphicType))
	{
		GraphicType = GraphicType.trimmed();
		s += QString("<span class='y'>") +
			GraphicType +
			QString("</span><br />");
	}
	std::vector<float> graphic_data;
	DicomUtils::get_fl_values(
		nds,
		mdcm::Tag(0x0070,0x0022),
		graphic_data);
	QString ReferencedFrameofReferenceUID;
	if (DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x3006,0x0024),
			ReferencedFrameofReferenceUID))
	{
		ReferencedFrameofReferenceUID =
			ReferencedFrameofReferenceUID.trimmed();
		s += QString("<span class='y'>") + ReferencedFrameofReferenceUID +
			QString("</span><br />");
	}
	QString FiducialUID;
	if (DicomUtils::get_string_value(
			nds,
			mdcm::Tag(0x0070,0x031A),
			FiducialUID))
	{
		FiducialUID = FiducialUID.trimmed();
		s += QString("<span class='y'>") +
			FiducialUID +
			QString("</span><br />");
	}
}

QString SRUtils::read_UIDREF(const mdcm::DataSet & nds, QString & s)
{
	QString UID;
	if (!DicomUtils::get_string_value(
		nds, mdcm::Tag(0x0040,0xa124), UID))
	{
		return QString("");
	}
	s += QString("<span class='y'>") +
		UID.trimmed().remove(QChar('\0')) +
		QString("</span><br />");
	return UID.trimmed();
}

QString SRUtils::read_sr_title1(
	const mdcm::DataSet & ds,
	const QString & charset)
{
	const QString s =
		DicomUtils::get_pn_value2(
			ds,
			mdcm::Tag(0x0010,0x0010),
			charset.toLatin1().constData());
	return s;
}

QString SRUtils::read_sr_title2(
	const mdcm::DataSet & ds,
	const QString & charset)
{
	QString s("<p>");
	if (ds.FindDataElement(
		mdcm::Tag(0x0010,0x0010)))
	{
		s += QString("<span class='t1'>") +
			DicomUtils::get_pn_value2(
				ds,
				mdcm::Tag(0x0010,0x0010),
				charset.toLatin1().constData()) +
			QString("</span>");
	}
	QString d;
	if (DicomUtils::get_string_value(
			ds,
			mdcm::Tag(0x0010,0x0030),
			d))
	{
		const QDate qd =
			QDate::fromString(
				d.trimmed(),
				QString("yyyyMMdd"));
		s += QString("<br /><span class='yy'>") +
			qd.toString(QString("d MMM yyyy")) +
			QString("</span>");
	}
	QString id;
	if (DicomUtils::get_string_value(
			ds,
			mdcm::Tag(0x0010,0x0020),
			id))
	{
		s += QString("<br /><span class='yy'>") +
			id.trimmed() +
			QString("</span>");
	}
	if (ds.FindDataElement(
		mdcm::Tag(0x0008,0x0090)))
	{
		const QString tmp0 =
			DicomUtils::get_pn_value2(
				ds,
				mdcm::Tag(0x0008,0x0090),
				charset.toLatin1().constData());
		if (!tmp0.isEmpty())
		{
			s += QString("<br /><span class='yy'>Referring ") +
				tmp0 +
				QString("</span>");
		}
	}
	s += QString("</p>");
	return s;
}

QStringList SRUtils::read_referenced(
	const mdcm::DataSet & nds,
	QString & s)
{
	if (!nds.FindDataElement(mdcm::Tag(0x0008,0x1199)))
		return QStringList();
	const mdcm::DataElement & e8  =
		nds.GetDataElement(mdcm::Tag(0x0008,0x1199));
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq8 =
		e8.GetValueAsSQ();
	if (!sq8) return QStringList();
	QStringList l;
	const unsigned int nitems8 = sq8->GetNumberOfItems();
	for(unsigned int i8 = 0; i8 < nitems8; ++i8)
	{
		const mdcm::Item & item8 = sq8->GetItem(i8+1);
		const mdcm::DataSet & nds8 =
			item8.GetNestedDataSet();
		QString ReferencedSOPClassUID;
		if (DicomUtils::get_string_value(
				nds8,
				mdcm::Tag(0x0008,0x1150),
				ReferencedSOPClassUID))
		{
			mdcm::UIDs uid;
			uid.SetFromUID(
				ReferencedSOPClassUID
					.toLatin1()
					.constData());
			QString uidname =
				QString::fromLatin1(uid.GetName());
			if (!uidname.isEmpty())
			{
				uidname = uidname.trimmed();
				s += QString("<span class='y'>") +
					uidname +
					QString("</span><br />");
			}
			else
			{
				ReferencedSOPClassUID =
					ReferencedSOPClassUID
						.trimmed()
						.remove(QChar('\0'));
				s += QString("<span class='y'>") +
					ReferencedSOPClassUID +
					QString("</span><br />");
			}
		}
		QString ReferencedSOPInstanceUID;
		if (DicomUtils::get_string_value(
				nds8,
				mdcm::Tag(0x0008,0x1155),
				ReferencedSOPInstanceUID))
		{
			l.push_back(ReferencedSOPInstanceUID.trimmed());
			s += QString("<span class='y'>") +
				ReferencedSOPInstanceUID.trimmed().remove(QChar('\0')) +
				QString("</span><br />");
		}
		std::vector<int> refframes;
		if (DicomUtils::get_is_values(
				nds8,
				mdcm::Tag(0x0008,0x1160),
				refframes))
		{
			QString ff("");
			for (size_t k = 0; k < refframes.size(); k++)
			{
				ff.append(
					QVariant(refframes.at(k)).toString() +
					QString(" "));
			}
			s += QString("<span class='y'>") +
				ff.trimmed() +
				QString("</span><br />");
		}
	}
	return l;
}

QString SRUtils::read_sr_content_sq(
	const mdcm::DataSet & ds,
	const QString & charset,
	const QString & path,
	const QWidget * wsettings,
	QTextEdit * textEdit,
	QProgressDialog * pb,
	QStringList & tmpfiles,
	std::vector<SRImage> & srimages,
	int indent,
	bool info,
	const QString & chapter,
	bool title)
{
	if (!ds.FindDataElement(mdcm::Tag(0x0040,0xa730)))
		return QString("");
	const mdcm::DataElement & e  =
		ds.GetDataElement(mdcm::Tag(0x0040,0xa730));
	mdcm::SmartPointer<mdcm::SequenceOfItems> sq =
		e.GetValueAsSQ();
	if (!sq) return QString("");
	QString s("");
	if (title) s += read_sr_title2(ds, charset);
	QString tmp_chapter("");
	const SettingsWidget * settings =
		static_cast<const SettingsWidget*>(wsettings);
	const bool print_chapters = settings->get_sr_chapters();
	const unsigned int nitems = sq->GetNumberOfItems();
	for(unsigned int i = 0; i < nitems; ++i)
	{
		const QString cs =
			(!chapter.isEmpty())
			? (chapter + QString(".") + QVariant(i+1).toString())
			: (QVariant(i+1).toString());
		//
		//
		s += QString("<p style=\"margin-left: ") +
			QVariant(indent).toString() +
			QString("px\">");
		//
		//
		if (print_chapters)
		{
			s += QString("<span class='yy'>") +
				cs +
				QString("</span><br />");
		}
		QString ValueType;
		QString RelationshipType;
		QString ContinuityOfContent;
		QString CodeMeaning;
		std::vector<unsigned int> ReferencedContentItemIdentifier;
		//
		const mdcm::Item & item = sq->GetItem(i+1);
		const mdcm::DataSet & nds =
			item.GetNestedDataSet();
		if (DicomUtils::get_string_value(
				nds, mdcm::Tag(0x0040,0xa040), ValueType))
		{
			ValueType = ValueType.trimmed().toUpper();
			if (info)
			{
				s += QString("<span class='y3'>Value Type: ") +
					ValueType + QString("</span><br />");
			}
		}
		if (DicomUtils::get_string_value(
				nds, mdcm::Tag(0x0040,0xa010), RelationshipType))
		{
			// TODO
			RelationshipType = RelationshipType.trimmed().toUpper();
			if (info)
			{
				s += QString("<span class='y3'>Relationship Type: ") +
					RelationshipType + QString("</span><br />");
			}
		}
		if (DicomUtils::get_string_value(
				nds, mdcm::Tag(0x0040,0xa050), ContinuityOfContent))
		{
			ContinuityOfContent = ContinuityOfContent.trimmed().toUpper();
			if (info)
			{
				s += QString("<span class='y3'>Continuity Of Content: ") +
					ContinuityOfContent + QString("</span><br />");
			}
		}
		if (nds.FindDataElement(mdcm::Tag(0x0040,0xa043)))
		{
			const mdcm::DataElement & e4  =
				nds.GetDataElement(mdcm::Tag(0x0040,0xa043));
			mdcm::SmartPointer<mdcm::SequenceOfItems> sq4 =
				e4.GetValueAsSQ();
			if (sq4)
			{
				const unsigned int nitems4 = sq4->GetNumberOfItems();
				for(unsigned int i4 = 0; i4 < nitems4; ++i4)
				{
					const mdcm::Item & item4 = sq4->GetItem(i4+1);
					const mdcm::DataSet & nds4 =
						item4.GetNestedDataSet();
					if (nds4.FindDataElement(mdcm::Tag(0x0008,0x0104)))
					{
						const mdcm::DataElement & e5 =
							nds4.GetDataElement(mdcm::Tag(0x0008,0x0104));
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
									&ba5, charset.toLatin1().constData());
							if (!CodeMeaning.isEmpty())
							{
								CodeMeaning += QString(" ");
							}
							CodeMeaning += tmp5.trimmed();
						}
					}
				}
			}
			if (!CodeMeaning.isEmpty())
			{
				s += QString("<span class='y9'>") +
					CodeMeaning +
					QString("</span><br />");
			}
		}
		if (DicomUtils::get_ul_values(
				nds,
				mdcm::Tag(0x0040,0xdb73),
				ReferencedContentItemIdentifier))
		{
			// TODO
			if (info)
			{
				QString identifiers("");
				for (
					unsigned int z = 0;
					z < ReferencedContentItemIdentifier.size();
					z++)
				{
					identifiers +=
						QVariant(
							(int)ReferencedContentItemIdentifier.at(z))
								.toString() + QString(" " );
				}
				s += QString(
						"<span class='y3'>"
						"Referenced Content Item Identifier: ") +
					identifiers +
					QString("</span><br />");
			}
		}

//////////////////////////////////////////////////////////////////
//
//
// Value
//
//
		if (ValueType == QString("CONTAINER"))
		{
			;;
		}
		else if (ValueType == QString("PNAME"))
		{
			read_PNAME(nds, charset, s);
		}
		else if (ValueType == QString("TEXT"))
		{
			read_TEXT(nds, charset, s);
		}
		else if (ValueType == QString("UIDREF"))
		{
			const QString uidref = read_UIDREF(nds, s);
		}
		else if (ValueType == QString("NUM"))
		{
			read_NUM(nds, charset, s);
		}
		else if (ValueType == QString("CODE"))
		{
			read_CODE(nds, charset, s);
		}
		else if (ValueType == QString("DATE"))
		{
			read_DATE(nds, s);
		}
		else if (ValueType == QString("DATETIME"))
		{
			read_DATETIME(nds, s);
		}
		else if (ValueType == QString("TIME"))
		{
			read_TIME(nds, s);
		}
		else if (ValueType == QString("IMAGE"))
		{
			std::vector<SRGraphic> tmp001;
			read_IMAGE(
				nds,
				charset,
				path,
				s,
				tmpfiles,
				srimages,
				textEdit,
				tmp001,
				info,
				wsettings,
				pb);
		}
		else if (
			ValueType == QString("COMPOSITE") ||
			ValueType == QString("WAVEFORM"))
		{
			// TODO
			s += QString("<span class='red2'>") +
				ValueType + QString("</span><br />");
			const QStringList & l = read_referenced(nds, s);
		}
		else if (ValueType == QString("SCOORD"))
		{
			const bool continue_ = read_SCOORD(
				nds,
				charset,
				path,
				s,
				tmpfiles,
				srimages,
				textEdit,
				info,
				wsettings,
				pb);
			if (continue_)
			{
				//
				//
				s += QString("</p>");
				//
				//
				continue;
			}
		}
		else if (ValueType == QString("SCOORD3D"))
		{
			// TODO
			s += QString("<span class='red2'>SCOORD3D</span><br />");
			read_SCOORD3D(nds, s);
		}
		else
		{
			if (!ValueType.isEmpty())
			{
				s += QString("<span class='red2'>Value ") +
					ValueType +
					QString(" is skipped</span><br />");
			}
		}
		//
		//
		s += QString("</p>");
		//
		//
//
//
//
//
//
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//
//
// Recursion
//
//
		if (nds.FindDataElement(mdcm::Tag(0x0040,0xa730)))
		{
			const int x = indent + 16;
			s += read_sr_content_sq(
				nds,
				charset,
				path,
				wsettings,
				textEdit,
				pb,
				tmpfiles,
				srimages,
				x,
				info,
				cs);
		}
//
//
//
//
//
//////////////////////////////////////////////////////////////////
	}
	return s;
}

#ifdef TMP_IMAGE_IN_MEMORY
#undef TMP_IMAGE_IN_MEMORY
#endif

