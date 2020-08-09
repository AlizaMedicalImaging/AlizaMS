#include "testgl.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QOpenGLContext>
#include <QSurfaceFormat>
#endif

TestGL::TestGL()
{
	no_opengl3 = true;
	opengl_init_done = false;
	setMinimumSize(128,128);
}

void TestGL::initializeGL()
{
	if (opengl_init_done) return;
	opengl_init_done = true;
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	GLenum err = glewInit();
	if (GLEW_OK != err) return;
	if (glewIsSupported("GL_VERSION_3_0")) no_opengl3 = false;
#else
	QOpenGLContext * c = QOpenGLContext::currentContext();
	if (c && c->isValid())
	{
		const QSurfaceFormat & f = c->format();
		if (f.majorVersion() >= 3) no_opengl3 = false;
	}
#endif
}

void TestGL::paintGL()
{
}

void TestGL::resizeGL(int, int)
{
}

