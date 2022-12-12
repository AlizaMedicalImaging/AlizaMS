#ifndef A_ICONUTILS_H
#define A_ICONUTILS_H

class ImageVariant;

class IconUtils
{
public:
	IconUtils();
	~IconUtils();
	static void update_icon(ImageVariant*, const int);
	static void icon(ImageVariant*);
	static void kill_threads();
};

#endif
