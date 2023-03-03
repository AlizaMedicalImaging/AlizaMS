#ifndef A_TESTGL_H
#define A_TESTGL_H

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

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int, int) override;
};

#endif

