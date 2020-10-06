#include "filepath.h"

const char * FilePath::getPath(const QString & p)
{
#ifdef _WIN32
	return p.toUtf8().constData();
#else
	return p.toLocal8Bit().constData();
#endif
}
