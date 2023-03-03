#ifndef A_LUTWIDGET_H
#define A_LUTWIDGET_H

#include <QWidget>
#include <QComboBox>

class LUTWidget: public QWidget
{
Q_OBJECT
public:
	LUTWidget(float);
	~LUTWidget();
	void add_items1();
	void add_items2();
	int get_lut() const;
	QComboBox * comboBox;

public slots:
	void set_lut(int);

private:
	float scale_icons;
};

#endif

