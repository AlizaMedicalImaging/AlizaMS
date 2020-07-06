#ifndef TESTGL____H__
#define TESTGL____H__

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QOpenGLWidget>
#else
#include "glew/include/GL/glew.h"
#include <QGLWidget>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
class TestGL : public QOpenGLWidget
#else
class TestGL : public QGLWidget
#endif
{
public:
	TestGL();
	~TestGL() {}
	bool opengl_init_done;
	bool no_opengl3;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	void updateGL() { update(); }
#endif
protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int, int);
};

#endif // TESTGL____H__
