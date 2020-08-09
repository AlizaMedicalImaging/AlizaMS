#ifndef ICONUTILS__H__
#define ICONUTILS__H__

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
