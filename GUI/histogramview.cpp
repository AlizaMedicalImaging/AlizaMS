#include "structures.h"
#include "histogramview.h"
#include "levelitem.h"
#include "aliza.h"
#include "commonutils.h"
#include <QtGlobal>
#include <QApplication>
#include <QPainterPath>
#include <QPalette>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

LevelRectItem::LevelRectItem(
	HistogramView * v,
	qreal x, qreal y,
	qreal w, qreal h,
	QGraphicsItem * p)
	: QGraphicsRectItem(x,y,w,h,p)
{
	view = v;
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, false);
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(false);
	setCursor(Qt::OpenHandCursor);
	pwidth = 3.0;
	QBrush brush(QColor(0xbc, 0x86, 0x2b));
	QPen pen;
	pen.setBrush(brush);
	pen.setWidthF(pwidth);
	pen.setStyle(Qt::SolidLine);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	setPen(pen);
}

LevelRectItem::~LevelRectItem()
{
}

qreal LevelRectItem::get_width() const
{
	return pwidth;
}

void LevelRectItem::set_width(double x)
{
	pwidth = x;
	QBrush brush(Qt::black);
	QPen pen;
	pen.setBrush(brush);
	pen.setWidthF(pwidth);
	pen.setStyle(Qt::SolidLine);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::RoundJoin);
	setPen(pen);
}

QRectF LevelRectItem::boundingRect() const
{
	return QGraphicsRectItem::boundingRect();
}

int LevelRectItem::type() const
{
    return Type;
}

QVariant LevelRectItem::itemChange(
	GraphicsItemChange change,
	const QVariant & data)
{
	if (change == ItemPositionChange)
	{
		QPointF newData  = data.toPointF();
		newData.setY(0.0);
		if (rect().center().x()+newData.x()>view->scene()->sceneRect().bottomRight().x())
		{
			newData.setX(view->scene()->sceneRect().bottomRight().x()-rect().center().x());
		}
		else if (rect().center().x()+newData.x()<view->scene()->sceneRect().bottomLeft().x())
		{
			newData.setX(view->scene()->sceneRect().bottomLeft().x()-rect().center().x());
		}
		view->window_center_update(rect().center().x()+newData.x());
 		return QGraphicsItem::itemChange(change, newData);
	}
	return QGraphicsItem::itemChange(change, data);
}

void LevelRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent * e)
{
	QGraphicsItem::mouseMoveEvent(e);
}

void LevelRectItem::mousePressEvent(QGraphicsSceneMouseEvent * e)
{
	setCursor(Qt::ClosedHandCursor);
	QGraphicsItem::mousePressEvent(e);
}

void LevelRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * e)
{
	setCursor(Qt::OpenHandCursor);
	QGraphicsItem::mouseReleaseEvent(e);
}

void LevelRectItem::keyPressEvent(QKeyEvent * e)
{
	QGraphicsItem::keyPressEvent(e);
}

void LevelRectItem::paint(
	QPainter * painter,
	const QStyleOptionGraphicsItem * option,
	QWidget * widget)
{
	QGraphicsRectItem::paint(painter, option, widget);
}

HistogramView::HistogramView(
	QWidget * parent,
	QObject * a_,
	QWidget * p,
	bool gl_)
	:
	QGraphicsView(parent), aliza(a_), multi_frame_ptr(p)
{
	setAcceptDrops(false);
	setFrameStyle(QFrame::Plain);
	setFrameShape(QFrame::NoFrame);
	QGraphicsScene * scene = new QGraphicsScene(this);
	setScene(scene);
	setBackgroundBrush(QBrush(qApp->palette().color(QPalette::Window)));
	setCacheMode(CacheNone);
	//setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	//
	setViewportUpdateMode(FullViewportUpdate);
	setTransformationAnchor(NoAnchor);
	setDragMode(NoDrag);
	//
	rect_item = new LevelRectItem(this,0,0,100,100);
	LevelItem * level_min = new LevelItem(rect_item,0,this);
	level_min->setParentItem(rect_item);
	LevelItem * level_max = new LevelItem(rect_item,1,this);
	level_max->setParentItem(rect_item);
	scene->addItem(rect_item);
	//
	pixmap = new QGraphicsPixmapItem();
	pixmap->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	pixmap->setPos(0,0);
	pixmap->setZValue(-1.0);
	scene->addItem(pixmap);
}

HistogramView::~HistogramView()
{
}

void HistogramView::clear__()
{
	QPixmap tmp1;
	pixmap->setPixmap(tmp1);
	rect_item->hide();
}

void HistogramView::drawBackground(
	QPainter * painter,
	const QRectF & rect)
{
	QGraphicsView::drawBackground(painter, rect);
}

void HistogramView::drawForeground(
	QPainter * painter,
	const QRectF & rect)
{
	Q_UNUSED(rect);
	Q_UNUSED(painter);
}

void HistogramView::update__(const ImageVariant * v)
{
	if (!v) return;
	const int w = this->width();
	const int h = this->height();
	scene()->setSceneRect(0,0,w,h);
	if (!v->histogram.isNull())
		pixmap->setPixmap(v->histogram.scaled(
			w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
	else clear__();
	//
	update_window(v);
}

void HistogramView::update_window(const ImageVariant * v)
{
	if (!v) return;
	if (!(v->image_type >= 0 && v->image_type < 10))
	{
		rect_item->hide();
		return;
	}
	const double h = scene()->sceneRect().height();
	const double w = scene()->sceneRect().width();
	double p1x = (v->di->window_center-v->di->window_width*0.5)*w;
	double p2x = (v->di->window_center+v->di->window_width*0.5)*w;
	rect_item->setRect(QRectF(p1x,0,p2x-p1x,h));
	rect_item->setPos(QPointF(0,0));
	if (!rect_item->isVisible()) rect_item->show();
}

void HistogramView::window_width_update_min(qreal x)
{
	if (aliza)
	{
		Aliza * p = static_cast<Aliza*>(aliza);
		const double j = scene()->sceneRect().width();
		p->width_from_histogram_min(j>0?x/j:0);
	}
}

void HistogramView::window_width_update_max(qreal x)
{
	if (aliza)
	{
		Aliza * p = static_cast<Aliza*>(aliza);
		const double j = scene()->sceneRect().width();
		p->width_from_histogram_max(j>0?x/j:0);
	}
}

void HistogramView::window_center_update(qreal x)
{
	if (aliza)
	{
		Aliza * p = static_cast<Aliza*>(aliza);
		const double j =
			scene()->sceneRect().width() > 0.0
			?
			x/scene()->sceneRect().width()
			:
			0.0;
		p->center_from_histogram(j);
	}
}

void HistogramView::update_bgcolor()
{
	setBackgroundBrush(QBrush(qApp->palette().color(QPalette::Window)));
}

void HistogramView::keyPressEvent(QKeyEvent * e)
{
	switch (e->key())
	{
	case Qt::Key_G :
		get_screen();
		break;
	default:
		QGraphicsView::keyPressEvent(e);
		break;
	}
}

void HistogramView::keyReleaseEvent(QKeyEvent * e)
{
	QGraphicsView::keyReleaseEvent(e);
}

void HistogramView::get_screen()
{
	QPixmap p;
	if (multi_frame_ptr && multi_frame_ptr->isVisible())
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		p = multi_frame_ptr->grab();
#else
		p = QPixmap::grabWidget(multi_frame_ptr);
#endif
	}
	else
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		p = this->grab();
#else
		p = QPixmap::grabWidget(this);
#endif
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
		QString("Select file: format by extension"),
		d,
		QString("All Files (*)"),
		(QString*)NULL
		 //,QFileDialog::DontUseNativeDialog
		);
	if (f.isEmpty()) return;
	QString file;
	QFileInfo fi(f);
	CommonUtils::set_screenshot_dir(fi.absolutePath());
	QString ext = fi.suffix();
	if (ext.isEmpty())
	{
		file = f + QString(".png");
		ext = QString("PNG");
	}
	else
	{
		file = f;
		ext = ext.trimmed().toUpper();
	}
	if (!p.save(file, ext.toLatin1().constData()))
	{
		QMessageBox mbox;
		mbox.setWindowModality(Qt::ApplicationModal);
		mbox.addButton(QMessageBox::Close);
		mbox.setIcon(QMessageBox::Warning);
		mbox.setText(QString("Could not save file"));
		mbox.exec();
	}
}
