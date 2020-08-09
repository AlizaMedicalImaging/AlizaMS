#include "filepath.h"

QString FilePath::getPath(const QString & p)
{
	const QString s(p);
	return s.toLocal8Bit();
}
