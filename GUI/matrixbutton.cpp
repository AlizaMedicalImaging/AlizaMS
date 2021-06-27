#include "matrixbutton.h"
#include <QMenu>
#include <QFrame>
#include <QGridLayout>
#include <QPainter>
#include <QPen>
#include <QEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWidgetAction>
#include <QPalette>
#include <QFontMetrics>
#include <QTextOption>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QEnterEvent>
#include <QMargins>
#endif

class DimsChooser : public QFrame
{
public:
	DimsChooser(MatrixButton*, QAction*);
	QSize sizeHint() const override;
	void mouseMoveEvent(QMouseEvent*) override;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	void enterEvent(QEnterEvent*) override;
#else
	void enterEvent(QEvent*) override;
#endif
	void leaveEvent(QEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void paintEvent(QPaintEvent*) override;

private:
	int           m_column;
	int           m_row;
	double        m_column_w;
	double        m_row_h;
	int           m_left_margin;
	int           m_top_margin;
	int           m_plus_w;
	int           m_plus_h;
	MatrixButton *m_button;
	QAction      *m_action;
};

DimsChooser::DimsChooser(MatrixButton * b, QAction * a)
	:
	QFrame(),
	m_column(0),
	m_row(0),
	m_column_w(30),
	m_button(b),
	m_action(a)
{
	setFrameShadow(Sunken);
	setBackgroundRole(QPalette::Base);
	setFrameShape(StyledPanel);
	setMouseTracking(true);
	QFontMetrics metrics(font());
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
	m_column_w = metrics.horizontalAdvance(QString("XXXX")) + 2;
#else
	m_column_w = metrics.width(QString("XXXX")) + 2;
#endif
	m_row_h = metrics.height() + 2;
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
	const QMargins m = contentsMargins();
	m_left_margin = m.left() + 4;
	m_top_margin  = m.top() + 4;
#else
	getContentsMargins(&m_left_margin, &m_top_margin, &m_plus_w, &m_plus_h);
	m_left_margin = 4;
	m_top_margin  = 4;
#endif
	m_plus_w = m_left_margin + 5;
	m_plus_h = m_top_margin  + 5;
}

QSize DimsChooser::sizeHint() const
{
	return QSize( m_plus_w + 8 * m_column_w, m_plus_h + 8 * m_row_h);
}

void DimsChooser::mouseMoveEvent(QMouseEvent * e)
{
	const QPoint p = e->pos();
	m_column = qMin(7.0, ((p.x() - m_left_margin) / m_column_w));
	m_row    = qMin(7.0, ((p.y() - m_top_margin)  / m_row_h));
	repaint();
}

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void DimsChooser::enterEvent(QEnterEvent * e)
#else
void DimsChooser::enterEvent(QEvent * e)
#endif
{
	m_action->activate(QAction::Hover);
	QFrame::enterEvent(e);
}

void DimsChooser::leaveEvent(QEvent*)
{
	m_column = -1;
	m_row    = -1;
	repaint();
}

void DimsChooser::mouseReleaseEvent(QMouseEvent * e)
{
	if (contentsRect().contains(e->pos()))
	{
		m_button->emit_matrix_selected(m_row + 1, m_column + 1);
	}
	QFrame::mouseReleaseEvent(e);
}

void DimsChooser::paintEvent(QPaintEvent * e)
{
	QFrame::paintEvent(e);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(contentsRect(), palette().brush(QPalette::Base));
	painter.translate(m_left_margin, m_top_margin);
	painter.translate(0.5, 0.5);
	QPen pen = painter.pen();
	pen.setWidthF(0.5);
	painter.setPen(pen);
	painter.fillRect(
		QRectF(
			0.0,
			0.0,
			(m_column + 1) * m_column_w,
			(m_row    + 1) * m_row_h),
		palette().brush(QPalette::Highlight));
	for(int c = 0; c <= 8; ++c)
	{
		painter.drawLine(
			QPointF(c * m_column_w, 0.0),
			QPointF(c * m_column_w, 8 * m_row_h));
	}
	for(int r = 0; r <= 8; ++r)
	{
		painter.drawLine(
			QPointF(0.0,             r * m_row_h),
			QPointF(8  * m_column_w, r * m_row_h));
	}
	QTextOption o(Qt::AlignCenter);
	o.setUseDesignMetrics(true);
	painter.drawText(
		QRectF(
			0.0,
			0.0,
			m_column_w,
			m_row_h),
		QString("%1x%2")
			.arg(m_column + 1)
			.arg(m_row + 1),
		o);
	painter.end();
}

class DimsChooserAction : public QWidgetAction
{
public:
	DimsChooserAction(MatrixButton*);
	DimsChooser * m_widget;
};

DimsChooserAction::DimsChooserAction(MatrixButton * b)
 : QWidgetAction(NULL)
{
	m_widget = new DimsChooser(b, this);
	setDefaultWidget(m_widget);
}

MatrixButton::MatrixButton(float si)
{
	setFocusPolicy(Qt::NoFocus);
	setIconSize(QSize((int)(si*18),(int)(si*18)));
	setToolButtonStyle(Qt::ToolButtonIconOnly);
	setIcon(QIcon(":/bitmaps/grid.svg"));
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	p_action = new QAction(
		QIcon(":/bitmaps/grid.svg"),
		QString("Custom"),
		this);
	p_menu = new QMenu(this);
	p_menu->addAction(p_action);
	p_menu->addSeparator();
	p_menu->addAction(new DimsChooserAction(this));
	setMenu(p_menu);
	setPopupMode(InstantPopup);
}

void MatrixButton::emit_matrix_selected(int r, int c)
{
	p_menu->hide();
	emit matrix_selected(r, c);
}
