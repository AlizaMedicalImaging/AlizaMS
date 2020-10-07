#include "filepath.h"
#include <QDir>

const char * FilePath::getPath(const QString & p)
{
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
	return (QDir::toNativeSeparators(p)).toUtf8().constData();
#else
#ifdef _WIN32
	return (QDir::toNativeSeparators(p)).toLocal8Bit().constData();
#else
	return p.toLocal8Bit().constData();
#endif
#endif
}
