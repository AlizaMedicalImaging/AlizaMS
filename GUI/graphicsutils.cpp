#include "graphicsutils.h"
#include "structures.h"
#include <QPainter>
#include <QPixmap>
#include <iostream>
#include "imageinfodialog.h"

template<typename T> const QString print_itk_info(
	const typename T::Pointer & image)
{
	if (image.IsNull()) return QString("");
	std::ostringstream s;
	image->Print(s);
	const QString t = QString::fromStdString(s.str());
	return t;
}

template<typename T> QString get_scalar_pixel_value__(
	const typename T::Pointer & image,
	const ImageVariant * ivariant,
	const int axis,
	const double x, const double y,
	const int selected_x_slice,
	const int selected_y_slice,
	const int selected_z_slice,
	long long * label)
{
	QString s("");
	if (x < 0 || y < 0) return s;
	if (image.IsNull()) return s;
	if (!ivariant)      return s;
	const unsigned int x_ = static_cast<unsigned int>(x);
	const unsigned int y_ = static_cast<unsigned int>(y);
	typename T::IndexType idx;
	bool ok = false;
	//
	switch(axis)
	{
	case 0:
		{
			if (ivariant->equi)
			{
				if (x_<(unsigned int)ivariant->di->idimy &&
					y_<(unsigned int)ivariant->di->idimz)
				{
					ok = true;
					idx[0] = selected_x_slice;
					idx[1] = x_;
					idx[2] = y_;
				}
			}
		}
		break;
	case 1:
		{
			if (ivariant->equi)
			{
				if (x_<(unsigned int)ivariant->di->idimx &&
					y_<(unsigned int)ivariant->di->idimz)
				{
					ok = true;
					idx[1] = selected_y_slice;
					idx[0] = x_;
					idx[2] = y_;
				}
			}
		}
		break;
	case 2:
		{
			if (x_<(unsigned int)ivariant->di->idimx &&
				y_<(unsigned int)ivariant->di->idimy)
			{
				ok = true;
				idx[2] = selected_z_slice;
				idx[0] = x_;
				idx[1] = y_;
			}
		}
		break;
	default: break;
	}
	//
	if (ok)
	{
		const QString idx_ =
			QString(" [ ") +
			QVariant(static_cast<int>(idx[0])).toString() + QString(",") +
			QVariant(static_cast<int>(idx[1])).toString() + QString(",") +
			QVariant(static_cast<int>(idx[2])).toString() +
			QString(" ]");
		const typename T::PixelType p = image->GetPixel(idx);
		switch(ivariant->image_type)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 7:
		case 8:
			{
				const long long tmp0 = static_cast<long long>(p);
				*label = tmp0;
				s.append(QVariant(tmp0).toString() + idx_);
			}
			break;
		case 5:
		case 6:
			{
				const double tmp0 = static_cast<double>(p);
				*label = static_cast<long long>(p);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
				s += QString::asprintf("%.6f",tmp0);
#else
				s.sprintf("%.6f",tmp0);
#endif
				s.append(idx_);
			}
			break;
		default: break;
		}
	}
	return s;
}

template<typename T> QString get_rgb_pixel_value__(
	const typename T::Pointer & image,
	const ImageVariant * ivariant,
	const int axis,
	const double x, const double y,
	const int sx,
	const int sy,
	const int sz)
{
	QString s("");
	if (x < 0 || y < 0) return s;
	if (image.IsNull()) return s;
	if (!ivariant)      return s;
	const unsigned int x_ = static_cast<unsigned int>(x);
	const unsigned int y_ = static_cast<unsigned int>(y);
	typename T::IndexType idx;
	bool ok = false;
	//
	switch(axis)
	{
	case 0:
		{
			if (ivariant->equi)
			{
				if (x_<(unsigned int)ivariant->di->idimy &&
					y_<(unsigned int)ivariant->di->idimz)
				{
					ok = true;
					idx[0] = sx;
					idx[1] = x_;
					idx[2] = y_;
				}
			}
		}
		break;
	case 1:
		{
			if (ivariant->equi)
			{
				if (x_<(unsigned int)ivariant->di->idimx &&
					y_<(unsigned int)ivariant->di->idimz)
				{
					ok = true;
					idx[1] = sy;
					idx[0] = x_;
					idx[2] = y_;
				}
			}
		}
		break;
	case 2:
		{
			if (x_<(unsigned int)ivariant->di->idimx &&
				y_<(unsigned int)ivariant->di->idimy)
			{
				ok = true;
				idx[2] = sz;
				idx[0] = x_;
				idx[1] = y_;
			}
		}
		break;
	default: break;
	}
	//
	if (ok)
	{
		const QString idx_ =
			QString(" [ ") +
			QVariant(static_cast<int>(idx[0])).toString() + QString(",") +
			QVariant(static_cast<int>(idx[1])).toString() + QString(",") +
			QVariant(static_cast<int>(idx[2])).toString() +
			QString(" ]");
		const typename T::PixelType p = image->GetPixel(idx);
		switch(ivariant->image_type)
		{
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
			{
				const int tmp0 = static_cast<int>(p.GetRed());
				const int tmp1 = static_cast<int>(p.GetGreen());
				const int tmp2 = static_cast<int>(p.GetBlue());
				s.append(QVariant(tmp0).toString()+QString(",") +
						QVariant(tmp1).toString()+QString(",") +
						QVariant(tmp2).toString() + idx_);
			}
			break;
		case 15:
		case 16:
			{
				const double tmp0 = static_cast<double>(p.GetRed());
				const double tmp1 = static_cast<double>(p.GetGreen());
				const double tmp2 = static_cast<double>(p.GetBlue());
				QString tmp0s;
				QString tmp1s;
				QString tmp2s;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
				tmp0s = QString::asprintf("%.3f",tmp0);
				tmp1s = QString::asprintf("%.3f",tmp1);
				tmp2s = QString::asprintf("%.3f",tmp2);
#else
				tmp0s.sprintf("%.3f",tmp0);
				tmp1s.sprintf("%.3f",tmp1);
				tmp2s.sprintf("%.3f",tmp2);
#endif
				s.append(tmp0s+QString(",")+tmp1s+QString(",")+tmp2s);
				s.append(idx_);
			}
			break;
		default: break;
		}
	}
	return s;
}


template<typename T> QString get_rgba_pixel_value__(
	const typename T::Pointer & image,
	const ImageVariant * ivariant,
	const int axis,
	const double x, const double y,
	const int sx,
	const int sy,
	const int sz)
{
	QString s("");
	if (x < 0 || y < 0) return s;
	if (image.IsNull()) return s;
	if (!ivariant)      return s;
	const unsigned int x_ = static_cast<unsigned int>(x);
	const unsigned int y_ = static_cast<unsigned int>(y);
	typename T::IndexType idx;
	bool ok = false;
	//
	switch(axis)
	{
	case 0:
		{
			if (ivariant->equi)
			{
				if (x_<(unsigned int)ivariant->di->idimy &&
					y_<(unsigned int)ivariant->di->idimz)
				{
					ok = true;
					idx[0] = sx;
					idx[1] = x_;
					idx[2] = y_;
				}
			}
		}
		break;
	case 1:
		{
			if (ivariant->equi)
			{
				if (x_<(unsigned int)ivariant->di->idimx &&
					y_<(unsigned int)ivariant->di->idimz)
				{
					ok = true;
					idx[1] = sy;
					idx[0] = x_;
					idx[2] = y_;
				}
			}
		}
		break;
	case 2:
		{
			if (x_<(unsigned int)ivariant->di->idimx &&
				y_<(unsigned int)ivariant->di->idimy)
			{
				ok = true;
				idx[2] = sz;
				idx[0] = x_;
				idx[1] = y_;
			}
		}
		break;
	default: break;
	}
	//
	if (ok)
	{
		const QString idx_ =
			QString(" [ ") +
			QVariant(static_cast<int>(idx[0])).toString() + QString(",") +
			QVariant(static_cast<int>(idx[1])).toString() + QString(",") +
			QVariant(static_cast<int>(idx[2])).toString() +
			QString(" ]");
		const typename T::PixelType p = image->GetPixel(idx);
		switch(ivariant->image_type)
		{
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
			{
				const int tmp0 = static_cast<int>(p.GetRed());
				const int tmp1 = static_cast<int>(p.GetGreen());
				const int tmp2 = static_cast<int>(p.GetBlue());
				const int tmp3 = static_cast<int>(p.GetAlpha());
				s.append(QVariant(tmp0).toString()+QString(",") +
						QVariant(tmp1).toString()+QString(",") +
						QVariant(tmp2).toString()+QString(",") +
						QVariant(tmp3).toString() + idx_);
			}
			break;
		case 25:
		case 26:
			{
				const double tmp0 = static_cast<double>(p.GetRed());
				const double tmp1 = static_cast<double>(p.GetGreen());
				const double tmp2 = static_cast<double>(p.GetBlue());
				const double tmp3 = static_cast<double>(p.GetAlpha());
				QString tmp0s;
				QString tmp1s;
				QString tmp2s;
				QString tmp3s;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
				tmp0s = QString::asprintf("%.3f",tmp0);
				tmp1s = QString::asprintf("%.3f",tmp1);
				tmp2s = QString::asprintf("%.3f",tmp2);
				tmp3s = QString::asprintf("%.3f",tmp3);
#else
				tmp0s.sprintf("%.3f",tmp0);
				tmp1s.sprintf("%.3f",tmp1);
				tmp2s.sprintf("%.3f",tmp2);
				tmp3s.sprintf("%.3f",tmp3);
#endif
				s.append(tmp0s+QString(",")+tmp1s+QString(",")+tmp2s+QString(",")+tmp3s);
				s.append(idx_);
			}
			break;
		default: break;
		}
	}
	return s;
}

QString GraphicsUtils::flip_label(const QString & s)
{
	if (s.isEmpty()) return QString("");
	else if (s == QString("I")) return QString("S");
	else if (s == QString("S")) return QString("I");
	else if (s == QString("A")) return QString("P");
	else if (s == QString("P")) return QString("A");
	else if (s == QString("L")) return QString("R");
	else if (s == QString("R")) return QString("L");
	else ;;
	return QString("");
}

void GraphicsUtils::gen_labels(
	const short axis,
	const bool hide_orientation,
	const QString & rai,
	const QString & laterality,
	const QString & body_part,
	QString & left_string,
	QString & top_string,
	const bool global_flip_x,
	const bool global_flip_y,
	bool * flip_x, bool * flip_y)
{
	if (!hide_orientation && !rai.isEmpty() && rai.size()>=3)
	{
		switch(axis)
		{
		case 0:
			{
				if (rai.at(2)==QChar('I'))
				{
					*flip_y = true;
					top_string = QString("S");
				}
				else if (rai.at(2)==QChar('P'))
				{
					*flip_y = true;
					top_string = QString("A");
				}
				else
				{
					top_string = QString(rai.at(2));
				}
				if (rai.at(1)==QChar('L'))
				{
					*flip_x = true;
					left_string = QString("R");
				}
				else
				{
					left_string = QString(rai.at(1));
				}
			}
			break;
		case 1:
			{
				if (rai.at(2)==QChar('I'))
				{
					*flip_y = true;
					top_string = QString("S");
				}
				else if (rai.at(2)==QChar('P'))
				{
					*flip_y = true;
					top_string = QString("A");
				}
				else
				{
					top_string = QString(rai.at(2));
				}
				if (rai.at(0)==QChar('L'))
				{	
					*flip_x = true;
					left_string = QString("R");
				}
				else
				{
					left_string = QString(rai.at(0));
				}
			}
			break;
		case 2:
			{
				if (rai.at(1)==QChar('I'))
				{
					*flip_y = true;
					top_string = QString("S");
				}
				else if (rai.at(1)==QChar('P'))
				{
					*flip_y = true;
					top_string = QString("A");
				}
				else
				{
					top_string = QString(rai.at(1));
				}
				if (rai.at(0)==QChar('L'))
				{
					*flip_x = true;
					left_string = QString("R");
				}
				else
				{
					left_string = QString(rai.at(0));
				}
			}
			break;
		default: break;
		}
		if (global_flip_x)
		{
			left_string = flip_label(left_string);
		}
		if (global_flip_y)
		{
			top_string = flip_label(top_string);
		}
		left_string.prepend(QString("<b>"));
		left_string.append(QString("</b>"));
		top_string.prepend(QString("<b>"));
		top_string.append(QString("</b>"));
	}
	else
	{
		top_string = QString("");
		left_string = QString("");
	}
	if (body_part.isEmpty())
	{
		if (!laterality.isEmpty())
		{
			if (laterality.toUpper() == QString("L"))
			{
				top_string.prepend(QString(
					"<small><i>Laterality:</i> left</small><br/>"));
			}
			else if (laterality.toUpper() == QString("R"))
			{
				top_string.prepend(QString(
					"<small><i>Laterality:</i> right</small><br/>"));
			}
			else if (laterality.toUpper() == QString("B"))
			{
				top_string.prepend(QString(
					"<small><i>Laterality:</i> both</small><br/>"));
			}
		}
	}
	else
	{
		QString tmp_anatomy = QString("<small>") + body_part;
		if (laterality.isEmpty())
		{
			tmp_anatomy.append(QString("</small><br/>"));
		}
		else
		{
			if (laterality.toUpper() == QString("L"))
			{
				tmp_anatomy.append(QString(
					", left</small><br/>"));
			}
			else if (laterality.toUpper() == QString("R"))
			{
				tmp_anatomy.append(QString(
					", right</small><br/>"));
			}
			else if (laterality.toUpper() == QString("B"))
			{
				tmp_anatomy.append(QString(
					", laterality: both</small><br/>"));
			}
			else
			{
				tmp_anatomy.append(QString("</small><br/>"));
			}
		}
		top_string.prepend(tmp_anatomy);
	}
}

void GraphicsUtils::draw_overlays(
	const ImageVariant * ivariant,
	QImage & tmpi)
{
	if (!ivariant->image_overlays.all_overlays.empty())
	{
		const int ov_idx = ivariant->di->selected_z_slice;
		if (ivariant->image_overlays.all_overlays.contains(ov_idx))
		{
			const SliceOverlays ov =
				ivariant->image_overlays.all_overlays.value(ov_idx);
			for (int ox = 0; ox < ov.size(); ox++)
			{
				const unsigned char * tmp_p_ =
					reinterpret_cast< const unsigned char* >(
						&(ov.at(ox).data[0]));
				if (tmp_p_)
				{
					QImage oi(
						tmp_p_,
						ov.at(ox).dimx,
						ov.at(ox).dimy,
						ov.at(ox).dimx,
						QImage::Format_Indexed8);
					QPainter * painter = new QPainter;
					painter->begin(&tmpi);
					painter->setCompositionMode(
						QPainter::CompositionMode_Lighten);
					painter->drawPixmap(
						QPointF((float)(ov.at(ox).x-1),
						(float)(ov.at(ox).y-1)),
						QPixmap::fromImage(oi));
					painter->end();
					delete painter;
				}
			}
		}
	}
}

void GraphicsUtils::print_image_info(
	const ImageVariant * v)
{
	if (!v) return;
	const short image_type = v->image_type;
	QString t;
	switch(image_type)
	{
	case   0: t = QString("signed short");       break;
	case   1: t = QString("unsigned short");     break;
	case   2: t = QString("signed int");         break;
	case   3: t = QString("unsigned int");       break;
	case   4: t = QString("unsigned char");      break;
	case   5: t = QString("float");  			 break;
	case   6: t = QString("double"); 			 break;
	case   7: t = QString("signed long long");   break;
	case   8: t = QString("unsigned long long"); break;
	case  10: t = QString("RGB signed short");   break;
	case  11: t = QString("RGB unsigned short"); break;
	case  12: t = QString("RGB signed int");     break;
	case  13: t = QString("RGB unsigned int");   break;
	case  14: t = QString("RGB unsigned char");  break;
	case  15: t = QString("RGB float");          break;
	case  16: t = QString("RGB double");         break;
	case  20: t = QString("RGBA signed short");  break;
	case  21: t = QString("RGBA unsigned short");break;
	case  22: t = QString("RGBA signed int");    break;
	case  23: t = QString("RGBA unsigned int");  break;
	case  24: t = QString("RGBA unsigned char"); break;
	case  25: t = QString("RGBA float");         break;
	case  26: t = QString("RGBA double");        break;
	case 100: t = QString("ROI");                break;
	case 200: t = QString("Mesh");               break;
	case 300: t = QString("Spectroscopy");       break;
	default : t = QString("unknown");            break;
	}
	QString s("");
	QString s0 =
		QString("Object ID ") +
		QVariant(v->id).toString();
	QString s1 =
		QString("Type:  ") +
		QVariant((int)image_type).toString() +
		QString(",  ") + t +
		((v->ybr) ? QString(", Y'CbCr\n") : QString("\n")) +
		QString("Uniform:  ") +
		QString((v->equi) ? "true" : "false") +
		QString(
			(v->di->hide_orientation)
			?
			" (disabled)\n"
			:
			"\n");
	if (!v->orientation_string.isEmpty())
		s1.append(QString("Orient.:  ") +
		v->orientation_string + QString("\n"));
	if (!v->interpretation.isEmpty())
		s1.append(QString("Interpret.:  ") +
		v->interpretation + QString("\n"));
	if (!v->iod.isEmpty())
		s1.append(QString("IOD:  ") + v->iod +
		QString("\n"));
	if (!v->sop.isEmpty())
		s1.append(QString("SOP:  ") + v->sop +
		QString("\n"));
	if (!v->study_uid.isEmpty())
		s1.append(QString("Study:  ") +
		v->study_uid + QString("\n"));
	if (!v->frame_of_ref_uid.isEmpty())
		s1.append(QString("Fr. of Ref.:  ") +
		v->frame_of_ref_uid + QString("\n"));
	if (!v->ioinfo.isEmpty())
		s1.append(v->ioinfo + QString("\n"));
	s1.append(
		QString("Acq.: ") + v->acquisition_date +
		v->acquisition_time + QString("\n"));
	s1.append(
		QString("Inst.: ") +
		QVariant(v->instance_number).toString() +
		QString("\n"));
	s1.append(
		QString("4D ID: ") +
		QVariant(v->group_id).toString() +
		QString("\n"));
	switch(v->image_type)
	{
	case 0:
		s = print_itk_info<ImageTypeSS>(v->pSS);
		break;
	case 1:
		s = print_itk_info<ImageTypeUS>(v->pUS);
		break;
	case 2:
		s = print_itk_info<ImageTypeSI>(v->pSI);
		break;
	case 3:
		s = print_itk_info<ImageTypeUI>(v->pUI);
		break;
	case 4:
		s = print_itk_info<ImageTypeUC>(v->pUC);
		break;
	case 5:
		s = print_itk_info<ImageTypeF>(v->pF);
		break;
	case 6:
		s = print_itk_info<ImageTypeD>(v->pD);
		break;
	case 7:
		s = print_itk_info<ImageTypeSLL>(v->pSLL);
		break;
	case 8:
		s = print_itk_info<ImageTypeULL>(v->pULL);
		break;
	case 10:
		s = print_itk_info<RGBImageTypeSS>(v->pSS_rgb);
		break;
	case 11:
		s = print_itk_info<RGBImageTypeUS>(v->pUS_rgb);
		break;
	case 12:
		s = print_itk_info<RGBImageTypeSI>(v->pSI_rgb);
		break;
	case 13:
		s = print_itk_info<RGBImageTypeUI>(v->pUI_rgb);
		break;
	case 14:
		s = print_itk_info<RGBImageTypeUC>(v->pUC_rgb);
		break;
	case 15:
		s = print_itk_info<RGBImageTypeF>(v->pF_rgb);
		break;
	case 16:
		s = print_itk_info<RGBImageTypeD>(v->pD_rgb);
		break;
	case 20:
		s = print_itk_info<RGBAImageTypeSS>(v->pSS_rgba);
		break;
	case 21:
		s = print_itk_info<RGBAImageTypeUS>(v->pUS_rgba);
		break;
	case 22:
		s = print_itk_info<RGBAImageTypeSI>(v->pSI_rgba);
		break;
	case 23:
		s = print_itk_info<RGBAImageTypeUI>(v->pUI_rgba);
		break;
	case 24:
		s = print_itk_info<RGBAImageTypeUC>(v->pUC_rgba);
		break;
	case 25:
		s = print_itk_info<RGBAImageTypeF>(v->pF_rgba);
		break;
	case 26:
		s = print_itk_info<RGBAImageTypeD>(v->pD_rgba);
		break;
	default:
		break;
	}
	if (!v->filenames.empty())
	{
		s.append(QString("\n\nSource files:\n"));
		for (int x = 0; x < v->filenames.size(); x++)
		{
			s.append(v->filenames.at(x) + QString("\n"));
		}
	}
	ImageInfoDialog * d = new ImageInfoDialog;
	d->set_label0(s0);
	d->set_label1(s1);
	if (!s.isEmpty()) d->set_text(s);
	d->adjust();
	d->exec();
	delete d;
}

QString GraphicsUtils::get_scalar_pixel_value(
	const ImageVariant * v,
	const int a,
	const double x, const double y,
	const int sx,
	const int sy,
	const int sz,
	const bool)
{
	if (!v) return QString("");
	QString d("");
	long long l = -99999999999999;
	switch(v->image_type)
	{
	case 0:
		d = get_scalar_pixel_value__<ImageTypeSS>(
			v->pSS, v, a, x, y, sx, sy, sz, &l);
		break;
	case 1:
		d = get_scalar_pixel_value__<ImageTypeUS>(
			v->pUS, v, a, x, y, sx, sy, sz, &l);
		break;
	case 2:
		d = get_scalar_pixel_value__<ImageTypeSI>(
			v->pSI, v, a, x, y, sx, sy, sz, &l);
		break;
	case 3:
		d = get_scalar_pixel_value__<ImageTypeUI>(
			v->pUI, v, a, x, y, sx, sy, sz, &l);
		break;
	case 4:
		d = get_scalar_pixel_value__<ImageTypeUC>(
			v->pUC, v, a, x, y, sx, sy, sz, &l);
		break;
	case 5:
		d = get_scalar_pixel_value__<ImageTypeF>(
			v->pF, v, a, x, y, sx, sy, sz, &l);
		break;
	case 6:
		d = get_scalar_pixel_value__<ImageTypeD>(
			v->pD, v, a, x, y, sx, sy, sz, &l);
		break;
	case 7:
		d = get_scalar_pixel_value__<ImageTypeSLL>(
			v->pSLL, v, a, x, y, sx, sy, sz, &l);
		break;
	case 8:
		d = get_scalar_pixel_value__<ImageTypeULL>(
			v->pULL, v, a, x, y, sx, sy, sz, &l);
		break;
	default :
		break;
	}
	return d;
}

QString GraphicsUtils::get_rgb_pixel_value(
	const ImageVariant * v,
	const int a,
	const double x, const double y,
	const int sx,
	const int sy,
	const int sz)
{
	if (!v) return QString("");
	QString d("");
	switch(v->image_type)
	{
	case 10:
		d = get_rgb_pixel_value__<RGBImageTypeSS>(
			v->pSS_rgb, v, a, x, y, sx, sy, sz);
		break;
	case 11:
		d = get_rgb_pixel_value__<RGBImageTypeUS>(
			v->pUS_rgb, v, a, x, y, sx, sy, sz);
		break;
	case 12:
		d = get_rgb_pixel_value__<RGBImageTypeSI>(
			v->pSI_rgb, v, a, x, y, sx, sy, sz);
		break;
	case 13:
		d = get_rgb_pixel_value__<RGBImageTypeUI>(
			v->pUI_rgb, v, a, x, y, sx, sy, sz);
		break;
	case 14:
		d = get_rgb_pixel_value__<RGBImageTypeUC>(
			v->pUC_rgb, v, a, x, y, sx, sy, sz);
		break;
	case 15:
		d = get_rgb_pixel_value__<RGBImageTypeF>(
			v->pF_rgb, v, a, x, y, sx, sy, sz);
		break;
	case 16:
		d = get_rgb_pixel_value__<RGBImageTypeD>(
			v->pD_rgb, v, a, x, y, sx, sy, sz);
		break;
	default :
		break;
	}
	return d;
}

QString GraphicsUtils::get_rgba_pixel_value(
	const ImageVariant * v,
	const int a,
	const double x, const double y,
	const int sx,
	const int sy,
	const int sz)
{
	if (!v) return QString("");
	QString d("");
	switch(v->image_type)
	{
	case 20:
		d = get_rgba_pixel_value__<RGBAImageTypeSS>(
			v->pSS_rgba, v, a, x, y, sx, sy, sz);
		break;
	case 21:
		d = get_rgba_pixel_value__<RGBAImageTypeUS>(
			v->pUS_rgba, v, a, x, y, sx, sy, sz);
		break;
	case 22:
		d = get_rgba_pixel_value__<RGBAImageTypeSI>(
			v->pSI_rgba, v, a, x, y, sx, sy, sz);
		break;
	case 23:
		d = get_rgba_pixel_value__<RGBAImageTypeUI>(
			v->pUI_rgba, v, a, x, y, sx, sy, sz);
		break;
	case 24:
		d = get_rgba_pixel_value__<RGBAImageTypeUC>(
			v->pUC_rgba, v, a, x, y, sx, sy, sz);
		break;
	case 25:
		d = get_rgba_pixel_value__<RGBAImageTypeF>(
			v->pF_rgba, v, a, x, y, sx, sy, sz);
		break;
	case 26:
		d = get_rgba_pixel_value__<RGBAImageTypeD>(
			v->pD_rgba, v, a, x, y, sx, sy, sz);
		break;
	default :
		break;
	}
	return d;
}

void GraphicsUtils::draw_cross_out(QImage & tmpi)
{
	const QSize s = tmpi.size();
	QPen pen;
	pen.setBrush(QBrush(QColor(255,0,0)));
	QPainter * painter = new QPainter;
	painter->begin(&tmpi);
	painter->setPen(pen);
	painter->drawLine(QPointF(0,0), QPointF(s.width(),s.height()));
	painter->drawLine(QPointF(0,s.height()), QPointF(s.width(),0));
	painter->end();
	delete painter;
}

