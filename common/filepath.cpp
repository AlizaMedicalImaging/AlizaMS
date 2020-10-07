#include "filepath.h"
#ifdef _MSC_VER
#include <QDir>
#endif

const char * FilePath::getPath(const QString & p)
{
#ifdef _MSC_VER
	return (QDir::toNativeSeparators(p)).toUtf8().constData();
#else
	return p.toLocal8Bit().constData();
#endif
}
