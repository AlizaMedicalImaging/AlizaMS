#ifndef Label__Widget__H
#define Label__Widget__H

#include <QWidget>
#include <QLabel>
#include <QPixmap>

class LabelWidget : public QWidget
{
	Q_OBJECT
public:
	LabelWidget(QWidget(*)=NULL);
	~LabelWidget();
	void set_indicator_red();
	void set_indicator_green();
	void set_indicator_blue();
	bool is_red()   const;
	bool is_green() const;
	bool is_blue()  const;
private:
	QLabel * label;
	QPixmap red, green, blue;
	bool label_is_red, label_is_green, label_is_blue;
};

#endif // Label__Widget__H
