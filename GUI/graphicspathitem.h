#ifndef A_GRAPHICSPATHITEM_H
#define A_GRAPHICSPATHITEM_H

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

class GraphicsPathItem : public QGraphicsPathItem
{
public:
	GraphicsPathItem() = default;
	~GraphicsPathItem() = default;
	int get_contour_id() const;
	void set_contour_id(int);
	int get_roi_id() const;
	void set_roi_id(int);
	long long get_tmp_id() const;
	void set_tmp_id(long long);
	int get_axis() const;
	void set_axis(int);
	int get_slice() const;
	void set_slice(int);
	int get_type() const;
	void set_type(int);
	QPainterPath shape() const override;

protected:
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	int contour_id{-1};
	int roi_id{-1};
	long long tmp_id{-1};
	int axis{-1};
	int slice{-1};
	// 1 - closed planar
	// 2 - open planar
	int contour_type{2};
};

#endif

