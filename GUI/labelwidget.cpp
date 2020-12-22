#include "labelwidget.h"

#include  <QHBoxLayout>

LabelWidget::LabelWidget(QWidget * p) : QWidget(p)
{
	label = new QLabel(this);
	label->setMaximumSize(QSize(32, 32));
	label->setScaledContents(true);
	red   = QPixmap(":/bitmaps/red.svg");
	green = QPixmap(":/bitmaps/green.svg");
	blue  = QPixmap(":/bitmaps/blue.svg");
	label_is_blue = label_is_green = label_is_red = false;
	QHBoxLayout * l = new QHBoxLayout(this);
	l->setContentsMargins(0,0,0,0); l->setSpacing(0);
	l->addWidget(label);
	label->setPixmap(red);
	label->hide(); 
}

LabelWidget::~LabelWidget()
{
}

void LabelWidget::set_indicator_red()
{
	label_is_red = true; label_is_blue = label_is_green = false;
	label->setPixmap(red);
}

void LabelWidget::set_indicator_green()
{
	label_is_green = true; label_is_blue = label_is_red = false;
	label->setPixmap(green);
}

void LabelWidget::set_indicator_blue()
{
	label_is_blue = true; label_is_green = label_is_red = false;
	label->setPixmap(blue);
}

bool LabelWidget::is_red()   const { return label_is_red; }

bool LabelWidget::is_green() const { return label_is_green; }

bool LabelWidget::is_blue()  const { return label_is_blue; }


