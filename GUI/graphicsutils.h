#ifndef A_GRAPHICSUTILS_H
#define A_GRAPHICSUTILS_H

#include <QString>
#include <QImage>

class ImageVariant;

class GraphicsUtils
{
public:
	static QString flip_label(const QString&);
	static void gen_labels(
		const short,
		const bool,
		const QString&, const QString&, const QString&,
		const QString&,
		QString&, QString&,
		const bool, const bool,
		bool*, bool*);
	static void draw_overlays(const ImageVariant*, QImage&);
	static void print_image_info(const ImageVariant*);
	static QString get_scalar_pixel_value(
		const ImageVariant*,
		const int,
		const double, const double,
		const int, const int, const int,
		const bool);
	static QString get_rgb_pixel_value(
		const ImageVariant*,
		const int,
		const double, const double,
		const int, const int, const int);
	static QString get_rgba_pixel_value(
		const ImageVariant*,
		const int,
		const double, const double,
		const int, const int, const int);
	static void draw_cross_out(QImage&);
};

#endif

