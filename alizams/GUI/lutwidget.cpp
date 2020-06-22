#include "lutwidget.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QSizePolicy>
#include <QFont>

LUTWidget::LUTWidget(float si, QWidget * p, Qt::WindowFlags f) : QWidget(p, f)
{
	QFont small(QString("Arial"), 8);
	setFont(small);
	QHBoxLayout * l = new QHBoxLayout(this);
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(0);
	comboBox = new QComboBox(this);
	l->addWidget(comboBox);
    QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Preferred);
    sp.setHorizontalStretch(0);
    sp.setVerticalStretch(0);
    setSizePolicy(sp);
}

LUTWidget::~LUTWidget()
{
}

void LUTWidget::add_items1()
{
    QIcon icon;
    icon.addFile(QString(":/bitmaps/grey0.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon, QString(""));
    QIcon icon1;
    icon1.addFile(QString(":/bitmaps/lut1.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon1, QString("Hot metal 768"));
    QIcon icon2;
    icon2.addFile(QString(":/bitmaps/lut2.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon2, QString("RainbowB 1536"));
    QIcon icon3;
    icon3.addFile(QString(":/bitmaps/lut3.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon3, QString("Syngo 1792"));
    QIcon icon4;
    icon4.addFile(QString(":/bitmaps/lut4.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon4, QString("Hot iron"));
    QIcon icon5;
    icon5.addFile(QString(":/bitmaps/lut5.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon5, QString("Hot metal blue"));
    QIcon icon6;
    icon6.addFile(QString(":/bitmaps/lut6.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon6, QString("PET"));
    QIcon icon7;
    icon7.addFile(QString(":/bitmaps/lut7.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon7, QString("PET 20 steps"));
	//
    comboBox->setObjectName(QString("comboBox"));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
    comboBox->setSizePolicy(sizePolicy);
    comboBox->setIconSize(QSize((int)(18*scale_icons),(int)(18*scale_icons)));
    comboBox->setFrame(true);
	comboBox->setCurrentIndex(0);
}

void LUTWidget::add_items2()
{
    QIcon icon;
    icon.addFile(QString(":/bitmaps/grey0.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon, QString(""));
    QIcon icon1;
    icon1.addFile(QString(":/bitmaps/lut1.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon1, QString("Hot metal 768"));
    QIcon icon2;
    icon2.addFile(QString(":/bitmaps/lut2.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon2, QString("RainbowB 1536"));
    QIcon icon3;
    icon3.addFile(QString(":/bitmaps/lut3.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon3, QString("Syngo 1792"));
    QIcon icon4;
    icon4.addFile(QString(":/bitmaps/lut4.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon4, QString("Hot iron"));
    QIcon icon5;
    icon5.addFile(QString(":/bitmaps/lut5.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon5, QString("Hot metal blue"));
    QIcon icon6;
    icon6.addFile(QString(":/bitmaps/lut6.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon6, QString("PET"));
    QIcon icon7;
    icon7.addFile(QString(":/bitmaps/lut7.png"), QSize(), QIcon::Normal, QIcon::Off);
    comboBox->addItem(icon7, QString("PET 20 steps"));
    comboBox->setObjectName(QString("comboBox"));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
    comboBox->setSizePolicy(sizePolicy);
    comboBox->setIconSize(QSize((int)(18*scale_icons),(int)(18*scale_icons)));
    comboBox->setFrame(true);
	comboBox->setCurrentIndex(0);
}

void LUTWidget::set_lut(int i)
{
	comboBox->setCurrentIndex(i);
}

int LUTWidget::get_lut() const
{
	return comboBox->currentIndex();
}
