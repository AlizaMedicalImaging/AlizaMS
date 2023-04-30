#include "graphicsview.h"
#include "graphicswidget.h"
#include <QtGlobal>
#include <QColor>
#include <QRectF>
#include <QStyleOptionButton>
#include <QTimer>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QList>
#include <QGraphicsDropShadowEffect>
#include <itkImageRegionConstIterator.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkMath.h>
#include "contourutils.h"
#include "colorspace/colorspace.h"

GraphicsView::GraphicsView(GraphicsWidget * p) : parent(p)
{
	setAcceptDrops(false);
	setFrameStyle(QFrame::Plain);
	setFrameShape(QFrame::NoFrame);
	QGraphicsScene * scene_ = new QGraphicsScene(this);
	scene_->setItemIndexMethod(QGraphicsScene::NoIndex);
	const QRectF rectf(0, 0, 100, 100);
	scene_->setSceneRect(rectf);
	setScene(scene_);
	QColor bc = qApp->palette().color(QPalette::Window);
	setBackgroundBrush(QBrush(bc));
	shutter_color = bc;
	//setCacheMode(CacheNone);
	setRenderHints(QPainter::Antialiasing);//QPainter::SmoothPixmapTransform);
#ifdef DELETE_GRAPHICSIMAGEITEM
	image_item = nullptr;
#else
	image_item = new QGraphicsPixmapItem();
	image_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	if (parent && !parent->get_smooth())
	{
		image_item->setTransformationMode(Qt::FastTransformation);
	}
	else
	{
		image_item->setTransformationMode(Qt::SmoothTransformation);
	}
	image_item->setCacheMode(QGraphicsItem::NoCache);
	image_item->setPos(0, 0);
	image_item->setZValue(-1.0);
	scene_->addItem(image_item);
#endif
	setViewportUpdateMode(FullViewportUpdate);
	setTransformationAnchor(AnchorUnderMouse);
	setDragMode(QGraphicsView::ScrollHandDrag);
	//
	handle_rect = new RectItem(0, 0, 100, 100);
	topHandle = new HandleItem(this, handle_rect, HandleItem::TopHandle);
	topHandle->setParentItem(handle_rect);
	rightHandle = new HandleItem(this, handle_rect, HandleItem::RightHandle);
	rightHandle->setParentItem(handle_rect);
	leftHandle = new HandleItem(this, handle_rect, HandleItem::LeftHandle);
	leftHandle->setParentItem(handle_rect);
	bottomHandle = new HandleItem(this, handle_rect, HandleItem::BottomHandle);
	bottomHandle->setParentItem(handle_rect);
	handle_rect->setZValue(1e+19 - 5);
	handle_rect->hide();
	scene_->addItem(handle_rect);
	//
	selection_item = new RectItem(0, 0, 100, 100);
	selection_item->set_pen2(1.0);
	selection_item->setZValue(1e+19 - 5);
	scene_->addItem(selection_item);
	selection_item->hide();
	//
	QPen xpen;
	xpen.setBrush(QBrush(QColor(255, 0, 0, 255)));
	xpen.setWidth(0);
	xpen.setStyle(Qt::DashLine);
	line_x = new QGraphicsPathItem();
	line_x->setPen(xpen);
	line_x->setZValue(1e+19 - 4);
	scene_->addItem(line_x);
	//
	QPen ypen;
	ypen.setBrush(QBrush(QColor(0, 255, 0, 255)));
	ypen.setWidth(0);
	ypen.setStyle(Qt::DashLine);
	line_y = new QGraphicsPathItem();
	line_y->setPen(ypen);
	line_y->setZValue(1e+19 - 3);
	scene_->addItem(line_y);
	//
	QPen zpen;
	zpen.setBrush(QBrush(QColor(0, 0, 255, 255)));
	zpen.setWidth(0);
	zpen.setStyle(Qt::DashLine);
	line_z = new QGraphicsPathItem();
	line_z->setPen(zpen);
	line_z->setZValue(1e+19 - 2);
	scene_->addItem(line_z);
	//
	measurment_line = new QGraphicsPathItem();
	measurment_line->setZValue(1e+19 - 1);
	scene_->addItem(measurment_line);
	//
	pr_area = new QGraphicsRectItem(0, 0, 100, 100);
	pr_area->setFlag(QGraphicsItem::ItemIsMovable, false);
	pr_area->setFlag(QGraphicsItem::ItemIsFocusable, false);
	pr_area->setAcceptHoverEvents(false);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	pr_area->setAcceptedMouseButtons(Qt::NoButton);
#else
	pr_area->setAcceptedMouseButtons(0);
#endif
	pr_area->setZValue(1e+19 - 6);
	pr_area->hide();
	QBrush pbrush(QColor(146, 90, 248));
	QPen ppen;
	ppen.setBrush(pbrush);
	ppen.setWidth(0);
	ppen.setStyle(Qt::SolidLine);
	pr_area->setPen(ppen);
	scene_->addItem(pr_area);
	//
	paint_brush = new QGraphicsPathItem();
	paint_brush->setFlag(QGraphicsItem::ItemIsMovable, false);
	paint_brush->setFlag(QGraphicsItem::ItemIsFocusable, false);
	paint_brush->setAcceptHoverEvents(false);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	paint_brush->setAcceptedMouseButtons(Qt::NoButton);
#else
	paint_brush->setAcceptedMouseButtons(0);
#endif
	QPen bpen;
	bpen.setBrush(QBrush(QColor(Qt::blue)));
	bpen.setWidth(0);
	bpen.setStyle(Qt::SolidLine);
	bpen.setCapStyle(Qt::RoundCap);
	bpen.setJoinStyle(Qt::RoundJoin);
	paint_brush->setPen(bpen);
	paint_brush->setZValue(1e+19);
	paint_brush->hide();
	scene_->addItem(paint_brush);
}

GraphicsView::~GraphicsView()
{
	clear_paths();
	clear_collision_paths();
	clear_us_regions();
	clear_prtexts_items();
	clear_prgraphicobjects_items();
	clear_shutters();
}

void GraphicsView::keyPressEvent(QKeyEvent * e)
{
	switch (e->key())
	{
	case Qt::Key_Plus:
		zoom_in();
		break;
	case Qt::Key_Equal:
		zoom_in();
		break;
	case Qt::Key_Minus:
		zoom_out();
		break;
	case Qt::Key_0:
		{
			parent->update_image(1, false, true);
			update_selection_rect_width();
		}
		break;
	case Qt::Key_1:
		{
			m_scale = 1.0;
			parent->update_image(0, false, true);
			update_selection_rect_width();
		}
		break;
	case Qt::Key_X:
		flipX();
		break;
	case Qt::Key_Y:
		flipY();
		break;
	case Qt::Key_F:
		{
			parent->update_image(1, false, true);
			update_selection_rect_width();
		}
		break;
	case Qt::Key_G:
		parent->get_screen();
		break;
	case Qt::Key_C:
		break;
	case Qt::Key_P:
		parent->show_image_info();
		break;
	case Qt::Key_S:
		{
			const bool t = parent->get_smooth();
			parent->set_smooth(!t);
		}
		break;
	default:
		QGraphicsView::keyPressEvent(e);
		break;
	}
}

void GraphicsView::keyReleaseEvent(QKeyEvent * e)
{
	switch (e->key())
	{
	case Qt::Key_Shift :
	default:
		break;
	}
	QGraphicsView::keyReleaseEvent(e);
}

void GraphicsView::wheelEvent(QWheelEvent * e)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	scale_view(pow(2.0, -e->angleDelta().y() / 480.0));
#else
	scale_view(pow(2.0, -e->delta() / 480.0));
#endif
}

void GraphicsView::drawBackground(QPainter * painter, const QRectF & rect)
{
	QGraphicsView::drawBackground(painter, rect);
}

void GraphicsView::drawForeground(QPainter * painter, const QRectF & rect)
{
	Q_UNUSED(rect);
	Q_UNUSED(painter);
}

void GraphicsView::scale_view(double scale_factor)
{
	m_scale *= scale_factor;
	setTransform(transform().scale(scale_factor, scale_factor));
	update_selection_rect_width();
}

void GraphicsView::update_selection_rect_width()
{
	if (handle_rect->get_current_pen() == 2)
		handle_rect->set_pen2(3.0 / m_scale);
	else
		handle_rect->set_pen1(2.0 / m_scale);
	selection_item->set_pen2(1.0 / m_scale);
}

void GraphicsView::zoom_in()
{
	scale_view(1.015);
}

void GraphicsView::zoom_out()
{
	scale_view(1.0 / 1.015);
}

void GraphicsView::clear_paths()
{
	if (paths.empty()) return;
	for (int i = 0; i < paths.size(); ++i)
	{
		if (paths.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(paths[i]));
			delete paths[i];
			paths[i] = nullptr;
		}
	}
	paths.clear();
}

void GraphicsView::clear_collision_paths()
{
	if (collision_paths.empty()) return;
	for (int i = 0; i < collision_paths.size(); ++i)
	{
		if (collision_paths.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(collision_paths[i]));
			delete collision_paths[i];
			collision_paths[i] = nullptr;
		}
	}
	collision_paths.clear();
}

void GraphicsView::clear_us_regions()
{
	if (us_regions.empty()) return;
	for (int i = 0; i < us_regions.size(); ++i)
	{
		if (us_regions.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(us_regions[i]));
			delete us_regions[i];
			us_regions[i] = nullptr;
		}
	}
	us_regions.clear();
}

void GraphicsView::clear_prtexts_items()
{
	if (!prtexts.empty())
	{
		for (int i = 0; i < prtexts.size(); ++i)
		{
			if (prtexts.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prtexts[i]));
				delete prtexts[i];
				prtexts[i] = nullptr;
			}
		}
		prtexts.clear();
	}
	if (!prtextanchors.empty())
	{
		for (int i = 0; i < prtextanchors.size(); ++i)
		{
			if (prtextanchors.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prtextanchors[i]));
				delete prtextanchors[i];
				prtextanchors[i] = nullptr;
			}
		}
		prtextanchors.clear();
	}
}

void GraphicsView::clear_prgraphicobjects_items()
{
	if (!prpoints.empty())
	{
		for (int i = 0; i < prpoints.size(); ++i)
		{
			if (prpoints.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prpoints[i]));
				delete prpoints[i];
				prpoints[i] = nullptr;
			}
		}
		prpoints.clear();
	}
	if (!prpolylines.empty())
	{
		for (int i = 0; i < prpolylines.size(); ++i)
		{
			if (prpolylines.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prpolylines[i]));
				delete prpolylines[i];
				prpolylines[i] = nullptr;
			}
		}
		prpolylines.clear();
	}
	if (!printerpolated.empty())
	{
		for (int i = 0; i < printerpolated.size(); ++i)
		{
			if (printerpolated.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(printerpolated[i]));
				delete printerpolated[i];
				printerpolated[i] = nullptr;
			}
		}
		printerpolated.clear();
	}
	if (!prcircles.empty())
	{
		for (int i = 0; i < prcircles.size(); ++i)
		{
			if (prcircles.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prcircles[i]));
				delete prcircles[i];
				prcircles[i] = nullptr;
			}
		}
		prcircles.clear();
	}
	if (!prellipses.empty())
	{
		for (int i = 0; i < prellipses.size(); ++i)
		{
			if (prellipses.at(i))
			{
				scene()->removeItem(static_cast<QGraphicsItem*>(prellipses[i]));
				delete prellipses[i];
				prellipses[i] = nullptr;
			}
		}
		prellipses.clear();
	}
}

void GraphicsView::clear_shutters()
{
	if (display_shutters.empty()) return;
	for (int i = 0; i < display_shutters.size(); ++i)
	{
		if (display_shutters.at(i))
		{
			scene()->removeItem(static_cast<QGraphicsItem*>(display_shutters[i]));
			delete display_shutters[i];
			display_shutters[i] = nullptr;
		}
	}
	display_shutters.clear();
}

void GraphicsView::mousePressEvent(QMouseEvent * e)
{
	if (e->button() == Qt::RightButton)
	{
		if (parent->slider_m)
		{
			int tmp0;
			if (Qt::ControlModifier == QApplication::keyboardModifiers())
			{
				tmp0 = parent->slider_m->slices_slider->value() - 1;
			}
			else
			{
				tmp0 = parent->slider_m->slices_slider->value() + 1;
			}
			if (tmp0 < 0) tmp0 = parent->slider_m->slices_slider->maximum();
			if (tmp0 > parent->slider_m->slices_slider->maximum()) tmp0 = 0;
			parent->slider_m->set_slice(tmp0);
		}
	}
	else if (e->button() == Qt::MiddleButton)
	{
		const QPoint p0 = e->pos();
		const QPointF p1 = mapToScene(p0);
		if (parent->get_show_cursor())
		{
			get_pixel_value2(p1.x(), p1.y());
		}
		else
		{
			get_pixel_value(p1.x(), p1.y());
		}
	}
	else if (e->button() == Qt::LeftButton)
	{
		switch (parent->get_mouse_modus())
		{
		case 1:
			{
				const QPoint p0 = e->pos();
				lines_at(p0.x(), p0.y());
			}
			break;
		case 2:
			{
				m0_set = true;
				m1_set = false;
				const QPoint p0 = e->pos();
				const QPointF p1 = mapToScene(p0);
				m0_win_pos_x = p1.x();
				m0_win_pos_y = p1.y();
				measure();
			}
			break;
		case 4:
		case 5:
		case 6:
		default:
			QGraphicsView::mousePressEvent(e);
			break;
		}
	}
	else
	{
		QGraphicsView::mousePressEvent(e);
	}
}

void GraphicsView::mouseDoubleClickEvent(QMouseEvent * e)
{
	QGraphicsView::mouseDoubleClickEvent(e);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent * e)
{
	if (e->button() == Qt::LeftButton)
	{
		switch (parent->get_mouse_modus())
		{
		case 1:
			{
				const QPoint p0 = e->pos();
				lines_at(p0.x(), p0.y());
			}
			break;
		case 2:
			{
				m0_set = true;
				m1_set = true;
				const QPoint p0 = e->pos();
				const QPointF p1 = mapToScene(p0);
				m1_win_pos_x = p1.x();
				m1_win_pos_y = p1.y();
				measure();
				m0_set = false;
				m1_set = false;
			}
			break;
		case 4:
		case 5:
		default:
			QGraphicsView::mouseReleaseEvent(e);
			break;
		}
	}
	else if (e->button() == Qt::MiddleButton)
	{
		get_pixel_value(-1, -1);
	}
	else
	{
		QGraphicsView::mouseReleaseEvent(e);
	}
}

void GraphicsView::mouseMoveEvent(QMouseEvent * e)
{
	const short mm = parent->get_mouse_modus();
	if (parent->get_show_cursor() && !(e->buttons() & Qt::MiddleButton))
	{
		const QPoint p0 = e->pos();
		const QPointF p1 = mapToScene(p0);
		get_pixel_value(p1.x(), p1.y());
	}
	if (e->buttons() & Qt::RightButton)
	{
		const QPoint p0 = e->pos();
		set_win_old_position(new_win_pos_x, new_win_pos_y);
		set_win_new_position(p0.x(), p0.y());
		if (parent->slider_m)
		{
			const int tmp0 = old_win_pos_x-new_win_pos_x;
			int tmp1;
			if (tmp0 > 0)
			{
				tmp1 = parent->slider_m->slices_slider->value() - 1;
			}
			else
			{
				tmp1 = parent->slider_m->slices_slider->value() + 1;
			}
			if (tmp1 < 0) tmp1 = 0;
			if (tmp1 > parent->slider_m->slices_slider->maximum())
			{
				tmp1 = parent->slider_m->slices_slider->maximum();
			}
			parent->slider_m->set_slice(tmp1);
		}
	}
	else if (e->buttons() & Qt::MiddleButton)
	{
		const QPoint p0 = e->pos();
		const QPointF p1 = mapToScene(p0);
		if (parent->get_show_cursor())
		{
			get_pixel_value2(p1.x(), p1.y());
		}
		else
		{
			get_pixel_value(p1.x(), p1.y());
		}
	}
	else if (e->buttons() & Qt::LeftButton)
	{
		switch (mm)
		{
		case 1:
			{
				const QPoint p0 = e->pos();
				lines_at(p0.x(), p0.y());
			}
			break;
		case 2:
			{
				if (m0_set == true)
				{
					m1_set = true;
					const QPoint p0 = e->pos();
					const QPointF p1 = mapToScene(p0);
					m1_win_pos_x = p1.x();
					m1_win_pos_y = p1.y();
					measure();
				}
			}
			break;
		case 4:
		case 5:
		case 6:
		default:
			QGraphicsView::mouseMoveEvent(e);
			break;
		}
	}
	else
	{
		QGraphicsView::mouseMoveEvent(e);
	}
}

void GraphicsView::flip()
{
	m_angle = 0;
	animate_flip();
}

void GraphicsView::animate_flip()
{
	if (!image_item) return;
	if (m_angle >= 360) return;
	m_angle += 10;
	QRectF r = image_item->boundingRect();
	image_item->setTransform(QTransform()
		.translate(r.width() / 2.0, r.height() / 2.0)
		.rotate(m_angle, Qt::YAxis)
		.translate(-r.width() / 2.0, -r.height() / 2.0));
	QTimer::singleShot(25, this, SLOT(animate_flip()));
}

void GraphicsView::emit_bb_update()
{
	emit bb_changed();
}

void GraphicsView::set_win_old_position(int x, int y)
{
	old_win_pos_x = x;
	old_win_pos_y = y;
}

void GraphicsView::set_win_new_position(int x, int y)
{
	new_win_pos_x = x;
	new_win_pos_y = y;
}

void GraphicsView::set_win_last_position(int x, int y)
{
	last_win_pos_x = x;
	last_win_pos_y = y;
}

void GraphicsView::update_background_color()
{
	QColor bc = qApp->palette().color(QPalette::Window);
	setBackgroundBrush(QBrush(bc));
	shutter_color = bc;
}

void GraphicsView::draw_frames(short a)
{
	if (!parent->image_container.image3D) return;
	switch (a)
	{
	case 0:
		{
			QPainterPath path_x_line;
			line_x->setPath(path_x_line);
			QPainterPath path_y_line;
			path_y_line.moveTo(
				parent->image_container.image3D->di->selected_y_slice + 0.5,
				0);
			path_y_line.lineTo(
				parent->image_container.image3D->di->selected_y_slice + 0.5,
				parent->image_container.image2D->idimy);
			line_y->setPath(path_y_line);
			QPainterPath path_z_line;
			path_z_line.moveTo(
				0,
				parent->image_container.image3D->di->selected_z_slice + 0.5);
			path_z_line.lineTo(
				parent->image_container.image2D->idimx,
				parent->image_container.image3D->di->selected_z_slice + 0.5);
			line_z->setPath(path_z_line);
		}
		break;
	case 1:
		{
			QPainterPath path_x_line;
			path_x_line.moveTo(
				parent->image_container.image3D->di->selected_x_slice + 0.5,
				0);
			path_x_line.lineTo(
				parent->image_container.image3D->di->selected_x_slice + 0.5,
				parent->image_container.image2D->idimy);
			line_x->setPath(path_x_line);
			QPainterPath path_y_line;
			line_y->setPath(path_y_line);
			QPainterPath path_z_line;
			path_z_line.moveTo(
				0,
				parent->image_container.image3D->di->selected_z_slice + 0.5);
			path_z_line.lineTo(
				parent->image_container.image2D->idimx,
				parent->image_container.image3D->di->selected_z_slice + 0.5);
			line_z->setPath(path_z_line);
		}
		break;
	case 2:
		{
			QPainterPath path_x_line;
			path_x_line.moveTo(
				parent->image_container.image3D->di->selected_x_slice + 0.5,
				0);
			path_x_line.lineTo(
				parent->image_container.image3D->di->selected_x_slice + 0.5,
				parent->image_container.image2D->idimy);
			line_x->setPath(path_x_line);
			QPainterPath path_y_line;
			path_y_line.moveTo(
				0,
				parent->image_container.image3D->di->selected_y_slice + 0.5);
			path_y_line.lineTo(
				parent->image_container.image2D->idimx,
				parent->image_container.image3D->di->selected_y_slice + 0.5);
			line_y->setPath(path_y_line);
			QPainterPath path_z_line;
			line_z->setPath(path_z_line);
		}
		break;
	default:
		break;
	}
}

void GraphicsView::draw_us_regions()
{
	clear_us_regions();
	if (parent->get_axis() != 2) return;
	if (!parent->image_container.image3D) return;
	for (int x = 0;
		x < parent->image_container.image3D->usregions.size();
		++x)
	{
		const UltrasoundRegionData & r =
			parent->image_container.image3D->usregions.at(x);
		const unsigned int minX0 = r.m_X0;
		const unsigned int minY0 = r.m_Y0;
		const unsigned int maxX1 = r.m_X1;
		const unsigned int maxY1 = r.m_Y1;
		const unsigned int upper_left_x  = minX0;
		const unsigned int upper_left_y  = minY0;
		const unsigned int upper_right_x = maxX1;
		const unsigned int upper_right_y = minY0;
		const unsigned int lower_left_x  = minX0;
		const unsigned int lower_left_y  = maxY1;
		const unsigned int lower_right_x = maxX1;
		const unsigned int lower_right_y = maxY1;
		const unsigned int priority =
			(r.m_FlagsBool && ((r.m_RegionFlags & 1) == 0)) ? 0 : 1;
		const unsigned short spatial = r.m_RegionSpatialFormat;
		QColor color(0xff, 0xff, 0xff);
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
			color = QColor(0x00, 0xff, 0xff); // Wave form
			break;
		case 0x5:
			color = QColor(0xff, 0x00, 0xff); // Graphics
			break;
		case 0:
		default:
			break;
		}
		QGraphicsPathItem * i = new QGraphicsPathItem();
		QPen pen;
		pen.setBrush(QBrush(color));
		pen.setWidth(0);
		if (priority == 0) pen.setStyle(Qt::SolidLine);
		else               pen.setStyle(Qt::DotLine);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		i->setFlag(QGraphicsItem::ItemIsSelectable, false);
		i->setPen(pen);
		QPainterPath p;
		p.moveTo(lower_left_x,  lower_left_y);
		p.lineTo(upper_left_x,  upper_left_y);
		p.lineTo(upper_right_x, upper_right_y);
		p.lineTo(lower_right_x, lower_right_y);
		p.lineTo(lower_left_x,  lower_left_y);
		if (r.m_ReferencePixelBool)
			p.addRect(
				r.m_ReferencePixelX0,
				r.m_ReferencePixelY0,
				1.0,
				1.0);
		i->setPath(p);
		us_regions.push_back(i);
		if (scene()) scene()->addItem(i);
	}
}

void GraphicsView::set_empty_lines()
{
	QPainterPath path___;
	line_x->setPath(path___);
	line_y->setPath(path___);
	line_z->setPath(path___);
}

void GraphicsView::set_empty_distance()
{
	QPainterPath path___;
	measurment_line->setPath(path___);
}

void GraphicsView::set_widget_m(GraphicsWidget * w)
{
	widget_m = w;
}

void GraphicsView::set_widget_y(GraphicsWidget * w)
{
	widget_y = w;
}

void GraphicsView::set_widget_x(GraphicsWidget * w)
{
	widget_x = w;
}

void GraphicsView::set_handleitems_cursors(bool t)
{
	if (t)
	{
		if (leftHandle)   leftHandle->setCursor(Qt::SizeHorCursor);
		if (rightHandle)  rightHandle->setCursor(Qt::SizeHorCursor);
		if (topHandle)    topHandle->setCursor(Qt::SizeVerCursor);
		if (bottomHandle) bottomHandle->setCursor(Qt::SizeVerCursor);
	}
	else
	{
		if (leftHandle)   leftHandle->unsetCursor();
		if (rightHandle)  rightHandle->unsetCursor();
		if (topHandle)    topHandle->unsetCursor();
		if (bottomHandle) bottomHandle->unsetCursor();
	}
}

void GraphicsView::lines_at(double x, double y)
{
	const QPoint p0(x, y);
	const QPointF p1 = mapToScene(p0);
	if (parent->image_container.image3D &&
		parent->image_container.image3D->equi)
	{
		int tmpa = static_cast<int>(p1.x());
		if (tmpa < 0) tmpa = 0;
		int tmpb = static_cast<int>(p1.y());
		if (tmpb < 0) tmpb = 0;
		if (parent->is_main())
		{
			switch (parent->get_axis())
			{
			case 0:
				{
					if (tmpa >= parent->image_container.image3D->di->idimy)
					{
						tmpa = parent->image_container.image3D->di->idimy - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb = parent->image_container.image3D->di->idimz - 1;
					}
					parent->image_container.image3D->di->selected_y_slice = tmpa;
					parent->image_container.image3D->di->selected_z_slice = tmpb;
				}
				break;
			case 1:
				{
					if (tmpa >= parent->image_container.image3D->di->idimx)
					{
						tmpa = parent->image_container.image3D->di->idimx - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb = parent->image_container.image3D->di->idimz - 1;
					}
					parent->image_container.image3D->di->selected_x_slice = tmpa;
					parent->image_container.image3D->di->selected_z_slice = tmpb;
				}
				break;
			case 2:
				{
					if (tmpa >= parent->image_container.image3D->di->idimx)
					{
						tmpa = parent->image_container.image3D->di->idimx - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimy)
					{
						tmpb = parent->image_container.image3D->di->idimy - 1;
					}
					if (parent->is_multiview())
					{
						if (widget_x && !widget_x->run__ && widget_x->slider_m)
						{
							widget_x->slider_m->set_slice(tmpa);
						}
						if (widget_y && !widget_y->run__ && widget_y->slider_m)
						{
							widget_y->slider_m->set_slice(tmpb);
						}
					}
					else
					{
						parent->image_container.image3D->di->selected_x_slice = tmpa;
						parent->image_container.image3D->di->selected_y_slice = tmpb;
					}
				}
				break;
			default:
				break;
			}
		}
		else
		{
			switch (parent->get_axis())
			{
			case 0:
				{
					if (tmpa >= parent->image_container.image3D->di->idimy)
					{
						tmpa =parent->image_container.image3D->di->idimy - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb =parent->image_container.image3D->di->idimz - 1;
					}
					if (widget_y && widget_y->slider_m)
					{
						widget_y->slider_m->set_slice(tmpa);
					}
					if (widget_m && !widget_m->run__ && widget_m->slider_m)
					{
						widget_m->slider_m->set_slice(tmpb);
					}
				}
				break;
			case 1:
				{
					if (tmpa >= parent->image_container.image3D->di->idimx)
					{
						tmpa = parent->image_container.image3D->di->idimx - 1;
					}
					if (tmpb >= parent->image_container.image3D->di->idimz)
					{
						tmpb = parent->image_container.image3D->di->idimz - 1;
					}
					if (widget_x && widget_x->slider_m)
					{
						widget_x->slider_m->set_slice(tmpa);
					}
					if (widget_m && !widget_m->run__ && widget_m->slider_m)
					{
						widget_m->slider_m->set_slice(tmpb);
					}
				}
				break;
			default:
				break;
			}
		}
		draw_frames(parent->get_axis());
	}
}

void GraphicsView::measure()
{
	if (!parent->image_container.image3D) return;
	if (m0_set && m1_set)
	{
		parent->update_measurement(
			m0_win_pos_x,
			m0_win_pos_y,
			m1_win_pos_x,
			m1_win_pos_y);
	}
	else
	{
		parent->set_measure_text("");
	}
}

void GraphicsView::get_pixel_value(double x, double y)
{
	if (!parent->image_container.image3D) return;
	parent->update_pixel_value(x, y);
}

void GraphicsView::get_pixel_value2(double x, double y)
{
	if (!parent->image_container.image3D) return;
	parent->update_pixel_value2(x, y);
}

void GraphicsView::flipX()
{
	global_flip_x = !global_flip_x;
	parent->update_image(0, false, true);
}

void GraphicsView::flipY()
{
	global_flip_y = !global_flip_y;
	parent->update_image(0, false, true);
}

void GraphicsView::draw_prtexts(const ImageVariant * ivariant)
{
#if 0
	{
		double L_, a_, b_;
		ColorSpace_::Rgb2Lab(
			&L_, &a_, &b_, 5.0 / 255.0, 220.0 / 255.0, 5.0 / 255.0);
		const unsigned short L = static_cast<unsigned short>((L_ / 100.0) * 65535.0);
		const unsigned short a =
			static_cast<unsigned short>(((a_ + 128.0) / 255.0) * 65535.0);
		const unsigned short b =
			static_cast<unsigned short>(((b_ + 128.0) / 255.0) * 65535.0);
		std::cout <<  L << " " << a << " " << b << std::endl;
	}
#endif
	clear_prtexts_items();
	if (parent->get_axis() != 2) return;
	if (!ivariant) return;
	if (ivariant->pr_text_annotations.empty()) return;
	int idx = -1;
	const int z = ivariant->di->selected_z_slice;
	if (ivariant->pr_text_annotations.contains(z))
	{
		idx = z;
	}
	else if (ivariant->pr_text_annotations.contains(-1))
	{
		;;
	}
	else
	{
		return;
	}
	const QList<PRTextAnnotation> & l =
		ivariant->pr_text_annotations.value(idx);
	for (int x = 0; x < l.size(); ++x)
	{
		QGraphicsTextItem * i = new QGraphicsTextItem();
		i->setFlag(QGraphicsItem::ItemIsMovable, false);
		i->setFlag(QGraphicsItem::ItemIsFocusable, false);
		i->setAcceptHoverEvents(false);
		double posx = 0.0;
		double posy = 0.0;
		double r = 0.0;
		if (l.at(x).has_bb)
		{
			double bottom_right_posx = 0.0;
			double bottom_right_posy = 0.0;
			if (l.at(x).BoundingBoxAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						posx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).bb_top_left_x;
						posy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).bb_top_left_y;
						bottom_right_posx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).bb_bottom_right_x;
						bottom_right_posy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).bb_bottom_right_y;
					}
					else
					{
						continue;
					}
				}
				else
				{
					posx = ivariant->di->idimx * l.at(x).bb_top_left_x;
					posy = ivariant->di->idimy * l.at(x).bb_top_left_y;
					bottom_right_posx = ivariant->di->idimx * l.at(x).bb_bottom_right_x;
					bottom_right_posy = ivariant->di->idimy * l.at(x).bb_bottom_right_y;
				}
			}
			else if (l.at(x).BoundingBoxAnnotationUnits == QString("PIXEL"))
			{
				posx = l.at(x).bb_top_left_x;
				posy = l.at(x).bb_top_left_y;
				bottom_right_posx = l.at(x).bb_bottom_right_x;
				bottom_right_posy = l.at(x).bb_bottom_right_y;
			}
			if (posx < bottom_right_posx &&
				posy < bottom_right_posy)
			{
				;;
			}
			else if (posx > bottom_right_posx &&
				posy > bottom_right_posy)
			{
				r = 180.0;
			}
			else if (posx < bottom_right_posx &&
				posy > bottom_right_posy)
			{
				r = 270.0;
			}
			else if (posx > bottom_right_posx &&
				posy < bottom_right_posy)
			{
				r = 90.0;
			}
		}
		else if (l.at(x).has_anchor)
		{
			if (l.at(x).AnchorPointAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						posx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).anchor_x;
						posy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).anchor_y;
					}
					else
					{
						continue;
					}
				}
				else
				{
					posx = ivariant->di->idimx * l.at(x).anchor_x;
					posy = ivariant->di->idimy * l.at(x).anchor_y;
				}
			}
			else if (l.at(x).AnchorPointAnnotationUnits == QString("PIXEL"))
			{
				posx = l.at(x).anchor_x;
				posy = l.at(x).anchor_y;
			}
		}
		const bool anchor_visible =
			l.at(x).AnchorPointVisibility == QString("Y")
			? true : false;
		if (l.at(x).has_anchor && anchor_visible)
		{
			double aposx = 0.0;
			double aposy = 0.0;
			if (l.at(x).AnchorPointAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						aposx =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).anchor_x;
						aposy =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).anchor_y;
					}
					else
					{
						continue;
					}
				}
				else
				{
					aposx = ivariant->di->idimx * l.at(x).anchor_x;
					aposy = ivariant->di->idimy * l.at(x).anchor_y;
				}
			}
			else if (l.at(x).AnchorPointAnnotationUnits == QString("PIXEL"))
			{
				aposx = l.at(x).anchor_x;
				aposy = l.at(x).anchor_y;
			}
			QPen pen;
			pen.setBrush(QBrush(QColor(255, 127, 23)));
			QPainterPath p;
			p.moveTo(aposx,aposy - 3);
			p.lineTo(aposx,aposy + 3);
			p.moveTo(aposx - 3, aposy);
			p.lineTo(aposx + 3, aposy);
			GraphicsPathItem * a = new GraphicsPathItem();
			a->setFlag(QGraphicsItem::ItemIsMovable, false);
			a->setFlag(QGraphicsItem::ItemIsFocusable, false);
			a->setAcceptHoverEvents(false);
			a->setPath(p);
			a->setPen(pen);
			a->setZValue(1e+18);
			scene()->addItem(static_cast<QGraphicsItem*>(a));
			prtextanchors.push_back(a);
		}
		i->setRotation(r);
		i->setPos(posx, posy);
		if (l.at(x).has_textstyle)
		{
			QFont font = i->font();
			if (!l.at(x).FontName.isEmpty())
			{
				font.setFamily(l.at(x).FontName);
			}
			else if (!l.at(x).CSSFontName.isEmpty())
			{
				font.setFamily(l.at(x).CSSFontName);
			}
			if (!l.at(x).Bold.isEmpty())
			{
				if (l.at(x).Bold == QString("Y")) font.setBold(true);
			}
			if (!l.at(x).Italic.isEmpty())
			{
				if (l.at(x).Italic == QString("Y")) font.setItalic(true);
			}
			if (!l.at(x).Underlined.isEmpty())
			{
				if (l.at(x).Underlined == QString("Y")) font.setUnderline(true);
			}
			if (!(
				l.at(x).TextColorCIELabValue_L == -1 &&
				l.at(x).TextColorCIELabValue_a == -1 &&
				l.at(x).TextColorCIELabValue_b == -1))
			{
				double R, G, B;
				double _L = (static_cast<double>(l.at(x).TextColorCIELabValue_L) / 65535.0) * 100.0;
				double _a = ((static_cast<unsigned short>
					(l.at(x).TextColorCIELabValue_a) - 0x8080) / 65535.0) * 255.0;
				double _b = ((static_cast<unsigned short>
					(l.at(x).TextColorCIELabValue_b) - 0x8080) / 65535.0) * 255.0;
				ColorSpace_::Lab2Rgb(&R, &G, &B, _L, _a, _b);
				i->setDefaultTextColor(QColor(
					static_cast<unsigned int>(R * 255.0),
					static_cast<unsigned int>(G * 255.0),
					static_cast<unsigned int>(B * 255.0),
					255));
			}
			else
			{
				i->setDefaultTextColor(QColor(150, 0, 150, 255));
			}
			i->setFont(font);
			if (l.at(x).ShadowStyle == QString("NORMAL"))
			{
				QGraphicsDropShadowEffect * shadow = new QGraphicsDropShadowEffect(i);
				shadow->setXOffset(1.5); // not supported
				shadow->setYOffset(1.5); // not supported
				double opacity = 255.0;
				if (l.at(x).ShadowOpacity > 0 && l.at(x).ShadowOpacity < 1)
				{
					opacity *= l.at(x).ShadowOpacity;
				}
				if (!(
					l.at(x).ShadowColorCIELabValue_L == -1 &&
					l.at(x).ShadowColorCIELabValue_a == -1 &&
					l.at(x).ShadowColorCIELabValue_b == -1))
				{
					double R, G, B;
					double _L = (static_cast<double>(l.at(x).ShadowColorCIELabValue_L) / 65535.0) * 100.0;
					double _a = ((static_cast<unsigned short>
						(l.at(x).ShadowColorCIELabValue_a) - 0x8080) / 65535.0) * 255.0;
					double _b = ((static_cast<unsigned short>
						(l.at(x).ShadowColorCIELabValue_b) - 0x8080) / 65535.0) * 255.0;
					ColorSpace_::Lab2Rgb(&R, &G, &B, _L, _a, _b);
					shadow->setColor(
						QColor(
							static_cast<unsigned int>(R * 255.0),
							static_cast<unsigned int>(G * 255.0),
							static_cast<unsigned int>(B * 255.0),
							static_cast<unsigned int>(opacity)));
				}
				else
				{
					shadow->setColor(QColor(128, 128, 128, static_cast<unsigned int>(opacity)));
				}
				i->setGraphicsEffect(shadow);
			}
			else if (l.at(x).ShadowStyle == QString("OUTLINED"))
			{
				;; // TODO
			}
		}
		else
		{
			i->setDefaultTextColor(QColor(150, 0, 150, 255));
			if (true)
			{
				QGraphicsDropShadowEffect * shadow = new QGraphicsDropShadowEffect(i);
				shadow->setXOffset(1.5);
				shadow->setYOffset(1.5);
				shadow->setColor(QColor(128, 128, 128, 255));
				i->setGraphicsEffect(shadow);
			}
		}
		i->setPlainText(l.at(x).UnformattedTextValue);
		i->setZValue(1e+18);
		scene()->addItem(static_cast<QGraphicsItem*>(i));
		prtexts.push_back(i);
	}
}

void GraphicsView::draw_prgraphics(const ImageVariant * ivariant)
{
	// TODO complete
	clear_prgraphicobjects_items();
	if (parent->get_axis() != 2) return;
	if (!ivariant) return;
	if (ivariant->pr_graphicobjects.empty()) return;
	int idx = -1;
	const int z = ivariant->di->selected_z_slice;
	if (ivariant->pr_graphicobjects.contains(z))
	{
		idx = z;
	}
	else if (ivariant->pr_graphicobjects.contains(-1))
	{
		;;
	}
	else
	{
		return;
	}
	const QList<PRGraphicObject> & l = ivariant->pr_graphicobjects.value(idx);
	for (int x = 0; x < l.size(); ++x)
	{
		if (!(l.at(x).GraphicData.size() > 1 && (l.at(x).GraphicData.size() % 2 == 0)))
		{
			continue;
		}
		const QString t = l.at(x).GraphicType;
		if (t == QString("POLYLINE"))
		{
			QPainterPath p;
			if (l.at(x).GraphicAnnotationUnits == QString("PIXEL"))
			{
				p.moveTo(
					l.at(x).GraphicData.at(0),
					l.at(x).GraphicData.at(1));
				for (unsigned int y = 2; y < l.at(x).GraphicData.size(); y += 2)
				{
					p.lineTo(
						l.at(x).GraphicData.at(y),
						l.at(x).GraphicData.at(y + 1));
				}
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						for (unsigned int y = 0; y < l.at(x).GraphicData.size(); y += 2)
						{
							const double px =
								ivariant->pr_display_areas.value(z).top_left_x +
								dimx * l.at(x).GraphicData.at(y);
							const double py =
								ivariant->pr_display_areas.value(z).top_left_y +
								dimy * l.at(x).GraphicData.at(y + 1);
							if (y > 1) p.lineTo(px, py);
							else       p.moveTo(px, py);
						}
					}
				}
				else
				{
					for (unsigned int y = 0;
						y < l.at(x).GraphicData.size(); y += 2)
					{
						const double px = ivariant->di->idimx * l.at(x).GraphicData.at(y);
						const double py = ivariant->di->idimy * l.at(x).GraphicData.at(y + 1);
						if (y > 1) p.lineTo(px, py);
						else       p.moveTo(px, py);
					}
				}
			}
			else
			{
				continue;
			}
			{
				QGraphicsPathItem * i = new QGraphicsPathItem();
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				if (l.at(x).GraphicFilled == QString("Y"))
				{
					QBrush brush(QColor(131, 108, 154, 255));
					brush.setStyle(Qt::SolidPattern);
					i->setPen(QPen(Qt::NoPen));
					i->setBrush(brush);
				}
				else
				{
					QPen pen;
					pen.setBrush(QBrush(QColor(192, 30, 200, 255)));
					pen.setWidth(0);
					pen.setStyle(Qt::SolidLine);
					pen.setCapStyle(Qt::RoundCap);
					pen.setJoinStyle(Qt::RoundJoin);
					i->setPen(pen);
				}
				i->setPath(p);
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prpolylines.push_back(i);
			}
		}
		else if (t == QString("ELLIPSE"))
		{
			if (l.at(x).GraphicData.size() != 8) continue;
			double major0_x = 0.0;
			double major0_y = 0.0;
			double major1_x = 0.0;
			double major1_y = 0.0;
			double minor0_x = 0.0;
			double minor0_y = 0.0;
			double minor1_x = 0.0;
			double minor1_y = 0.0;
			if (l.at(x).GraphicAnnotationUnits == QString("PIXEL"))
			{
				major0_x = l.at(x).GraphicData.at(0);
				major0_y = l.at(x).GraphicData.at(1);
				major1_x = l.at(x).GraphicData.at(2);
				major1_y = l.at(x).GraphicData.at(3);
				minor0_x = l.at(x).GraphicData.at(4);
				minor0_y = l.at(x).GraphicData.at(5);
				minor1_x = l.at(x).GraphicData.at(6);
				minor1_y = l.at(x).GraphicData.at(7);
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						major0_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(0);
						major0_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(1);
						major1_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(2);
						major1_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(3);
						minor0_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(4);
						minor0_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(5);
						minor1_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(6);
						minor1_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(7);
					}
					else
					{
						continue;
					}
				}
				else
				{
					major0_x = ivariant->di->idimx * l.at(x).GraphicData.at(0);
					major0_y = ivariant->di->idimy * l.at(x).GraphicData.at(1);
					major1_x = ivariant->di->idimx * l.at(x).GraphicData.at(2);
					major1_y = ivariant->di->idimy * l.at(x).GraphicData.at(3);
					minor0_x = ivariant->di->idimx * l.at(x).GraphicData.at(4);
					minor0_y = ivariant->di->idimy * l.at(x).GraphicData.at(5);
					minor1_x = ivariant->di->idimx * l.at(x).GraphicData.at(6);
					minor1_y = ivariant->di->idimy * l.at(x).GraphicData.at(7);
				}
			}
			else
			{
				continue;
			}
			{
				const double mid_major_x = (major0_x + major1_x) * 0.5;
				const double mid_major_y = (major0_y + major1_y) * 0.5;
				const double x0__ = major1_x - major0_x;
				const double y0__ = major1_y - major0_y;
				const double x1__ = minor1_x - minor0_x;
				const double y1__ = minor1_y - minor0_y;
				const double d0   = sqrt(x0__ * x0__ + y0__ * y0__);
				const double d1   = sqrt(x1__ * x1__ + y1__ * y1__);
				const double ma_j = 1.0 / d0;
				const double ma_nx = x0__ * ma_j;
				const double ma_ny = y0__ * ma_j;
				const double mi_j = 1.0 / d1;
				const double mi_nx = x1__ * mi_j;
				const double mi_ny = y1__ * mi_j;
				const double start = 0.0;
				const double span  = 360.0;
				QGraphicsPathItem * i = new QGraphicsPathItem();
				{
					QTransform ttt;
					ttt.translate(mid_major_x, mid_major_y);
					i->setTransform(ttt, true);
				}
				{
					QTransform ttt;
					ttt.setMatrix(ma_nx, ma_ny, 0, -mi_nx, -mi_ny, 0, 0, 0, 1);
					i->setTransform(ttt, true);
				}
				QRectF r___(-0.5 * d0, -0.5 * d1, d0, d1);
				QPainterPath path;
				path.arcMoveTo(r___, start);
				path.arcTo(r___, start, span);
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				if (l.at(x).GraphicFilled == QString("Y"))
				{
					QBrush brush(QColor(131, 108, 154, 255));
					brush.setStyle(Qt::SolidPattern);
					i->setPen(QPen(Qt::NoPen));
					i->setBrush(brush);
				}
				else
				{
					QPen pen;
					pen.setBrush(QBrush(QColor(192, 30, 200, 255)));
					pen.setWidth(0);
					pen.setStyle(Qt::SolidLine);
					pen.setCapStyle(Qt::RoundCap);
					pen.setJoinStyle(Qt::RoundJoin);
					i->setPen(pen);
				}
				i->setPath(path);
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prellipses.push_back(i);
			}
		}
		else if (t == QString("CIRCLE"))
		{
			if (l.at(x).GraphicData.size() != 4) continue;
			double center_x = 0.0;
			double center_y = 0.0;
			double point_x  = 0.0;
			double point_y  = 0.0;
			if (l.at(x).GraphicAnnotationUnits ==
					QString("PIXEL"))
			{
				center_x = l.at(x).GraphicData.at(0);
				center_y = l.at(x).GraphicData.at(1);
				point_x  = l.at(x).GraphicData.at(2);
				point_y  = l.at(x).GraphicData.at(3);
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						center_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(0);
						center_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(1);
						point_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(2);
						point_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(3);
					}
					else
					{
						continue;
					}
				}
				else
				{
					center_x = ivariant->di->idimx * l.at(x).GraphicData.at(0);
					center_y = ivariant->di->idimy * l.at(x).GraphicData.at(1);
					point_x = ivariant->di->idimx * l.at(x).GraphicData.at(2);
					point_y = ivariant->di->idimy * l.at(x).GraphicData.at(3);
				}
			}
			else
			{
				continue;
			}
			{
				const double x__ = point_x - center_x;
				const double y__ = point_y - center_y;
				const double distance = sqrt(x__ * x__ + y__ * y__);
				QGraphicsEllipseItem * i =
					new QGraphicsEllipseItem(
						center_x - distance,
						center_y - distance,
						2 * distance,
						2 * distance);
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				if (l.at(x).GraphicFilled == QString("Y"))
				{
					QBrush brush(QColor(131, 108, 154, 255));
					brush.setStyle(Qt::SolidPattern);
					i->setPen(QPen(Qt::NoPen));
					i->setBrush(brush);
				}
				else
				{
					QPen pen;
					pen.setBrush(QBrush(QColor(192, 30, 200, 255)));
					pen.setWidth(0);
					pen.setStyle(Qt::SolidLine);
					pen.setCapStyle(Qt::RoundCap);
					pen.setJoinStyle(Qt::RoundJoin);
					i->setPen(pen);
				}
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prcircles.push_back(i);
			}
		}
		else if (t == QString("POINT"))
		{
			if (l.at(x).GraphicData.size()!=2) continue;
			double point_x  = 0.0;
			double point_y  = 0.0;
			if (l.at(x).GraphicAnnotationUnits ==
					QString("PIXEL"))
			{
				point_x  = l.at(x).GraphicData.at(0);
				point_y  = l.at(x).GraphicData.at(1);
			}
			else if (l.at(x).GraphicAnnotationUnits == QString("DISPLAY"))
			{
				if (ivariant->pr_display_areas.contains(z))
				{
					const int dimx =
						ivariant->pr_display_areas.value(z).bottom_right_x -
						ivariant->pr_display_areas.value(z).top_left_x;
					const int dimy =
						ivariant->pr_display_areas.value(z).bottom_right_y -
						ivariant->pr_display_areas.value(z).top_left_y;
					if (dimx > 0 && dimy > 0)
					{
						point_x =
							ivariant->pr_display_areas.value(z).top_left_x +
							dimx * l.at(x).GraphicData.at(0);
						point_y =
							ivariant->pr_display_areas.value(z).top_left_y +
							dimy * l.at(x).GraphicData.at(1);
					}
					else
					{
						continue;
					}
				}
				else
				{
					point_x = ivariant->di->idimx * l.at(x).GraphicData.at(0);
					point_y = ivariant->di->idimy * l.at(x).GraphicData.at(1);
				}
			}
			else
			{
				continue;
			}
			{
				QBrush brush(QColor(192, 30, 200, 255));
				brush.setStyle(Qt::SolidPattern);
				QGraphicsEllipseItem * i =
					new QGraphicsEllipseItem(
						point_x - 1.0,
						point_y - 1.0,
						2.0,
						2.0);
				i->setFlag(QGraphicsItem::ItemIsMovable, false);
				i->setFlag(QGraphicsItem::ItemIsFocusable, false);
				i->setAcceptHoverEvents(false);
				i->setPen(QPen(Qt::NoPen));
				i->setBrush(brush);
				i->setZValue(1e+17);
				scene()->addItem(static_cast<QGraphicsItem*>(i));
				prpoints.push_back(i);
			}
		}
		else if (t == QString("INTERPOLATED"))
		{
		}
	}
}

void GraphicsView::draw_shutter(const ImageVariant * ivariant)
{
	clear_shutters();
	if (parent->get_axis() != 2) return;
	if (!ivariant) return;
	if (ivariant->pr_display_shutters.empty()) return;
	int idx = -1;
	if (ivariant->pr_display_shutters.contains(-1))
	{
		;;
	}
	else
	{
		const int z = ivariant->di->selected_z_slice;
		if (ivariant->pr_display_shutters.contains(z))
		{
			idx = z;
		}
		else
		{
			return;
		}
	}
	const PRDisplayShutter & a =
		ivariant->pr_display_shutters.value(idx);
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	const QStringList & l =
			a.ShutterShape.split(QString("\\"), Qt::SkipEmptyParts);
#else
	const QStringList & l =
			a.ShutterShape.split(QString("\\"), QString::SkipEmptyParts);
#endif
	for (int x = 0; x < l.size(); ++x)
	{
		if (l.at(x).trimmed() == QString("RECTANGULAR"))
		{
			if (a.ShutterLeftVerticalEdge    == -1 &&
				a.ShutterRightVerticalEdge   == -1 &&
				a.ShutterUpperHorizontalEdge == -1 &&
				a.ShutterLowerHorizontalEdge == -1)
			{
				continue;
			}
			QGraphicsPathItem * i = new QGraphicsPathItem();
			i->setFlag(QGraphicsItem::ItemIsMovable, false);
			i->setFlag(QGraphicsItem::ItemIsFocusable, false);
			i->setAcceptHoverEvents(false);
			QBrush brush(shutter_color);
			brush.setStyle(Qt::SolidPattern);
			i->setPen(QPen(Qt::NoPen));
			i->setBrush(brush);
			QPainterPath p;
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						-2,
						-2,
						ivariant->di->idimx + 2,
						ivariant->di->idimy + 2)));
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						(a.ShutterLeftVerticalEdge    - 1),
						(a.ShutterUpperHorizontalEdge - 1),
						a.ShutterRightVerticalEdge -
							a.ShutterLeftVerticalEdge,
						a.ShutterLowerHorizontalEdge -
							a.ShutterUpperHorizontalEdge)));
			i->setPath(p);
			i->setZValue(1e+16);
			scene()->addItem(static_cast<QGraphicsItem*>(i));
			display_shutters.push_back(i);
		}
		else if (l.at(x).trimmed() == QString("CIRCULAR"))
		{
			if (a.CenterofCircularShutter_x == -1 &&
				a.CenterofCircularShutter_y == -1 &&
				a.RadiusofCircularShutter   == -1)
			{
				continue;
			}
			if (a.RadiusofCircularShutter <= 0)
			{
				continue;
			}
			QGraphicsPathItem * i = new QGraphicsPathItem();
			i->setFlag(QGraphicsItem::ItemIsMovable, false);
			i->setFlag(QGraphicsItem::ItemIsFocusable, false);
			i->setAcceptHoverEvents(false);
			QBrush brush(shutter_color);
			brush.setStyle(Qt::SolidPattern);
			i->setPen(QPen(Qt::NoPen));
			i->setBrush(brush);
			QPainterPath p;
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						-2,
						-2,
						ivariant->di->idimx + 2,
						ivariant->di->idimy + 2)));
			p.moveTo(0, 0);
			const int d = 2 * a.RadiusofCircularShutter;
			p.addEllipse(
				(a.CenterofCircularShutter_x - 1) - a.RadiusofCircularShutter,
				(a.CenterofCircularShutter_y - 1) - a.RadiusofCircularShutter,
				d,
				d);
			i->setPath(p);
			i->setZValue(1e+16);
			scene()->addItem(static_cast<QGraphicsItem*>(i));
			display_shutters.push_back(i);
		}
		else if (l.at(x).trimmed() == QString("POLYGONAL"))
		{
			if (a.VerticesofthePolygonalShutter.empty())
			{
				continue;
			}
			if (!(a.VerticesofthePolygonalShutter.size() > 1 &&
					a.VerticesofthePolygonalShutter.size() % 2 == 0))
			{
				continue;
			}
			QVector<QPoint> points;
			for (unsigned int j = 0;
				j < a.VerticesofthePolygonalShutter.size(); j += 2)
			{
				points.push_back(QPoint(
					a.VerticesofthePolygonalShutter.at(j + 1) - 1,
					a.VerticesofthePolygonalShutter.at(    j) - 1));
			}
			QGraphicsPathItem * i = new QGraphicsPathItem();
			i->setFlag(QGraphicsItem::ItemIsMovable, false);
			i->setFlag(QGraphicsItem::ItemIsFocusable, false);
			i->setAcceptHoverEvents(false);
			QBrush brush(shutter_color);
			brush.setStyle(Qt::SolidPattern);
			i->setPen(QPen(Qt::NoPen));
			i->setBrush(brush);
			QPainterPath p;
			p.moveTo(0, 0);
			p.addPolygon(
				QPolygon(
					QRect(
						-2,
						-2,
						ivariant->di->idimx + 2,
						ivariant->di->idimy + 2)));
			p.moveTo(0, 0);
			p.addPolygon(QPolygon(points));
			i->setPath(p);
			i->setZValue(1e+16);
			scene()->addItem(static_cast<QGraphicsItem*>(i));
			display_shutters.push_back(i);
		}
	}
}

