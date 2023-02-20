#include "lutwidget.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QSizePolicy>
#include <QFont>

LUTWidget::LUTWidget(float si)
{
	QFont small(QString("Arial"), 8);
	setFont(small);
	scale_icons = si;
	QHBoxLayout * l = new QHBoxLayout(this);
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	comboBox = new QComboBox(this);
	l->addWidget(comboBox);
    QSizePolicy sp(QSizePolicy::Minimum, QSizePolicy::Preferred);
    sp.setHorizontalStretch(0);
    sp.setVerticalStretch(0);
    setSizePolicy(sp);
	setToolTip(QString("LUT"));
}

LUTWidget::~LUTWidget()
{
}

void LUTWidget::add_items1()
{
    comboBox->addItem(QIcon(QString(":/bitmaps/grey0.png")), QString(""));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut1.png")),  QString("Hot metal 768"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut2.png")),  QString("RainbowB 1536"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut3.png")),  QString("Syngo 1792"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut4.png")),  QString("Hot iron"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut5.png")),  QString("Hot metal blue"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut6.png")),  QString("PET"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut7.png")),  QString("PET 20 steps"));
    comboBox->setObjectName(QString("comboBox"));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
    comboBox->setSizePolicy(sizePolicy);
    comboBox->setIconSize(QSize(
		static_cast<int>(18 * scale_icons), static_cast<int>(18 * scale_icons)));
    comboBox->setFrame(true);
	comboBox->setCurrentIndex(0);
}

void LUTWidget::add_items2()
{
    comboBox->addItem(QIcon(QString(":/bitmaps/grey0.png")), QString(""));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut1.png")),  QString("Hot metal 768"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut2.png")),  QString("RainbowB 1536"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut3.png")),  QString("Syngo 1792"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut4.png")),  QString("Hot iron"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut5.png")),  QString("Hot metal blue"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut6.png")),  QString("PET"));
    comboBox->addItem(QIcon(QString(":/bitmaps/lut7.png")),  QString("PET 20 steps"));
    comboBox->setObjectName(QString("comboBox"));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
    comboBox->setSizePolicy(sizePolicy);
    comboBox->setIconSize(QSize(
		static_cast<int>(18 * scale_icons), static_cast<int>(18 * scale_icons)));
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
