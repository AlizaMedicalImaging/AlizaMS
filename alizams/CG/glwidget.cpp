//#define ALWAYS_SHOW_GL_ERROR

#include "structures.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include "glwidget-qt5.h"
#else
#include <QGLContext>
#include "glwidget-qt4.h"
#endif
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include "commonutils.h"
#include <cmath>
#include <iostream>
#include "shaders.h"
#include "luts.h"
#include "itkSpatialOrientation.h"
#include "data/cube_data.h"
#include "data/letters_data.h"
#include "data/letteri_data.h"
#include "data/letterp_data.h"
#include "data/lettera_data.h"
#include "data/letterr_data.h"
#include "data/letterl_data.h"

#define FBO_SIZE__0  512
#define FBO_SIZE__1 1024

#ifndef DISABLE_SIMDMATH
using namespace Vectormath::SSE;
#else
using namespace Vectormath::Scalar;
#endif

ShaderObj::ShaderObj()
{
	texture_handle   = new GLuint[TEXTURES_SIZE];
	location_sampler = new GLuint[TEXTURES_SIZE];
	for (int x = 0; x < TEXTURES_SIZE; x++) texture_handle[x] = 0;
	for (int x = 0; x < TEXTURES_SIZE; x++) location_sampler[x] = 0;
}

ShaderObj::~ShaderObj()
{
	delete [] texture_handle;
	delete [] location_sampler;
}

qMeshData::qMeshData() : shader(NULL)
{
	K            = new float[12];
	vboid        = new GLuint[VBOIDS_SIZE];
	get_shadows  = new int[MAX_SHADOWS_];
	cast_shadows = new int[MAX_SHADOWS_];
	textures     = new GLuint[TEXTURES_SIZE];
	tex_units    = new GLuint[TEXTURES_SIZE];
	for (int x = 0; x < 12; x++) K[x] = 0;
	for (int x = 0; x < VBOIDS_SIZE;   x++) vboid[x] = 0;
	for (int x = 0; x < MAX_SHADOWS_;  x++) get_shadows[x] = 0;
	for (int x = 0; x < MAX_SHADOWS_;  x++) cast_shadows[x] = 0;
	for (int x = 0; x < TEXTURES_SIZE; x++) textures[x] = 0;
	for (int x = 0; x < TEXTURES_SIZE; x++) tex_units[x] = 0;
}

qMeshData::~qMeshData()
{
	delete [] K;
	delete [] vboid;
	delete [] get_shadows;
	delete [] cast_shadows;
	delete [] textures;
	delete [] tex_units;
}

CollisionObject::CollisionObject() :
	collision_object(NULL), id(0), shape(NULL), mesh_data(NULL) {}

CollisionObject::~CollisionObject() {}

void CollisionObject::set_shape(btCollisionShape * s)
{
	shape = s;
}

btCollisionShape * CollisionObject::get_shape()
{
	return shape;
}

void CollisionObject::set_collision_object(btCollisionObject * o)
{
	collision_object = o;
}

btCollisionObject * CollisionObject::get_collision_object()
{
	return collision_object;
}

unsigned int CollisionObject::get_id() const
{
	return id;
}

void CollisionObject::set_id(unsigned int i)
{
	id = i;
}

void CollisionObject::set_mesh_data(qMeshData * q)
{
	mesh_data = q;
}

qMeshData * CollisionObject::get_mesh_data()
{
	return mesh_data;
}

static long long GLWidget_count_vbos = 0;
static bool GLWidget_max_vbos_65535  = false;

static QList<ImageVariant*> * selected_images__ = NULL;

const float color_spectro0[] = {(float)0x10/(float)0xff,(float)0x10/(float)0xff,(float)0xf0/(float)0xff,1.0f};
const float color_spectro1[] = {(float)0x10/(float)0xff,(float)0xe8/(float)0xff,(float)0x10/(float)0xff,1.0f};
const float color_cube[]     = {0.1f,0.1f,0.1f,0.2f,0.2f,0.2f};
const float color_letters[]  = {0.5f,0.5f,0.5f,0.8f,0.8f,0.8f};
//0x90,0xad,0xc6
//0x30,0x0d,0x0d

// to clear pointers
static std::vector<ShaderObj*> shaders;
static std::vector<GLuint*>    vboids; // size 2
static std::vector<GLuint>     vaoids;
static std::vector<GLuint*>    textures;
static std::vector<qMeshData*> qmeshes;

//static btAlignedObjectArray<CollisionObject*>  collision_objects;

struct  MyRayResultCallback : public btCollisionWorld::AllHitsRayResultCallback
{
	MyRayResultCallback(const btVector3 & rayFrom,const btVector3 & rayTo)
		: btCollisionWorld::AllHitsRayResultCallback(rayFrom, rayTo) {}
	virtual ~MyRayResultCallback() {}
	bool needsCollision(btBroadphaseProxy * proxy0) const
	{
		btCollisionObject * b = static_cast<btCollisionObject *>(proxy0->m_clientObject);
		if (b->getUserPointer())
		{
			//int * p = static_cast<int*>(b->getUserPointer());
			//if ((p[0]==) || (p[0]==)) return true;
			return true;
		}
		return false;
	}    
};

struct  MyClosestRayResultCallback0 : public btCollisionWorld::ClosestRayResultCallback
{
	MyClosestRayResultCallback0 (const btVector3 & rayFrom,const btVector3 & rayTo)
		: btCollisionWorld::ClosestRayResultCallback(rayFrom, rayTo) {}
	virtual ~MyClosestRayResultCallback0() {}
	bool needsCollision(btBroadphaseProxy * proxy0) const
	{
		btCollisionObject * b = static_cast<btCollisionObject *>(proxy0->m_clientObject);
		if (static_cast<int*>(b->getUserPointer()))
		{
			//if (static_cast<int*>(b->getUserPointer())[0] ==)
			return true;
		}
		return false;
	}    
};

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
GLWidget::GLWidget(QWidget * p, Qt::WindowFlags f) : QOpenGLWidget(p, f)
{
#ifdef USE_SET_GL_FORMAT
#ifndef USE_SET_DEFAULT_GL_FORMAT
	QSurfaceFormat format;
#ifndef __arm__
	format.setRenderableType(QSurfaceFormat::OpenGL);
#endif
#ifdef USE_CORE_3_2_PROFILE
#ifdef USE_GL_MAJOR_3_MINOR_2
	format.setVersion(3, 2); // may be required sometimes, e.g. Intel on Linux
#endif
	format.setProfile(QSurfaceFormat::CoreProfile);
#endif
	format.setRedBufferSize(8);
	format.setGreenBufferSize(8);
	format.setBlueBufferSize(8);
	format.setAlphaBufferSize(8);
	format.setDepthBufferSize(24);
	//format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	//format.setSwapInterval(0);
	//format.setSamples(4);
	setFormat(format);
#endif
#endif
	setMinimumSize(64,64);
	setFocusPolicy(Qt::WheelFocus);
	init_();
}
#else
GLWidget::GLWidget(QWidget * p) : QGLWidget(p)
{
	setMinimumSize(64,64);
	setFocusPolicy(Qt::WheelFocus);
	init_();
}

GLWidget::GLWidget(const QGLFormat & frm, QWidget * p) : QGLWidget(frm,p)
{
	setMinimumSize(64,64);
	setFocusPolicy(Qt::WheelFocus);
	init_();
}
#endif

void GLWidget::initializeGL()
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	initializeOpenGLFunctions();
#if 0
	const QSurfaceFormat & f = format();
	std::cout << "Major " << f.majorVersion() << std::endl;
	std::cout << "Minor " << f.minorVersion() << std::endl;
	std::cout << "Red buffer size "     << f.redBufferSize()     << std::endl;
	std::cout << "Green buffer size "   << f.greenBufferSize()   << std::endl;
	std::cout << "Blue buffer size "    << f.blueBufferSize()    << std::endl;
	std::cout << "Alpha buffer size "   << f.alphaBufferSize()   << std::endl;
	std::cout << "Stencil buffer size " << f.stencilBufferSize() << std::endl;
	std::cout << "Depth buffer size "   << f.depthBufferSize()   << std::endl;
	const int profile = (int)f.profile();
	switch (profile)
	{
	case 0:
		std::cout << "QSurfaceFormat::NoProfile" << std::endl;
		break;
	case 1:
		std::cout << "QSurfaceFormat::CoreProfile" << std::endl;
		break;
	case 2:
		std::cout << "QSurfaceFormat::CompatibilityProfile" << std::endl;
		break;
	default:
		std::cout << "OpenGL profile ??" << std::endl;
		break;
	}
	const int render = (int)f.renderableType();
	switch (render)
	{
	case 0x0:
		std::cout << "QSurfaceFormat::DefaultRenderableType" << std::endl;
		break;
	case 0x1:
		std::cout << "QSurfaceFormat::OpenGL" << std::endl;
		break;
	case 0x2:
		std::cout << "QSurfaceFormat::OpenGLES" << std::endl;
		break;
	case 0x4:
		std::cout << "QSurfaceFormat::OpenVG" << std::endl;
		break;
	default:
		std::cout << "Renderable type ??" << std::endl;
		break;
	}
	const int swap = (int)f.swapBehavior();
	switch (swap)
	{
	case 0:
		std::cout << "QSurfaceFormat::DefaultSwapBehavior" << std::endl;
		break;
	case 1:
		std::cout << "QSurfaceFormat::SingleBuffer" << std::endl;
		break;
	case 2:
		std::cout << "QSurfaceFormat::DoubleBuffer" << std::endl;
		break;
	case 3:
		std::cout << "QSurfaceFormat::TripleBuffer" << std::endl;
		break;
	default:
		std::cout << "Swap behavior ??" << std::endl;
		break;
	}
	std::cout << "Swap interval " << f.swapInterval() << std::endl;
	std::cout << "Samples " << f.samples() << std::endl;
#endif
	const QOpenGLContext * c = context();
	if (!c)
	{
		std::cout << "QOpenGLContext is NULL " << std::endl;
		return;
	}
	else
	{
		if (!c->isValid())
		{
			std::cout << "QOpenGLContext is invalid" << std::endl;
			return;
		}
		else
		{
#ifdef USE_CORE_3_2_PROFILE
			QOpenGLFunctions_3_2_Core * funcs = c->versionFunctions<QOpenGLFunctions_3_2_Core>();
#else
			QOpenGLFunctions_3_0 * funcs = c->versionFunctions<QOpenGLFunctions_3_0>();
#endif
			if (!funcs)
			{
				std::cout << "Could not obtain required OpenGL context version" << std::endl;
				return;
			}
		}
	}
#else // Qt4
	const QGLContext * c = context();
	if (!c->isValid())
	{
		std::cout << "GLContext is invalid" << std::endl;
		return;
	}
#if 0
	QGLFormat f = c->format();
	const int profile = (int)f.profile();
	switch (profile)
	{
	case 0:
		std::cout << "QGLFormat::NoProfile" << std::endl;
		break;
	case 1:
		std::cout << "QGLFormat::CoreProfile" << std::endl;
		break;
	case 2:
		std::cout << "QGLFormat::CompatibilityProfile" << std::endl;
		break;
	default:
		std::cout << "OpenGL profile ??" << std::endl;
		break;
	}
	std::cout << "Direct rendering " << f.directRendering() << std::endl;
	std::cout << "Depth " << f.depthBufferSize() << std::endl;
#endif
#endif
	update_clear_color();
	if (!opengl_init_done)
		init_opengl(this->size().width(), this->size().height());
}

void GLWidget::paintGL()
{
	paint__();
}

void GLWidget::resizeGL(int width, int height)
{
	resize__(width,height);
}

void GLWidget::mousePressEvent(QMouseEvent * e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		lastPos = e->pos();
	}
	else if (e->buttons() & Qt::MidButton)
	{
		lastPanPos = e->pos();
	}
	else if (e->buttons() & Qt::RightButton)
	{
		lastPosScale = e->pos();
#if 0
		send_ray0(e->x(),e->y());
		updateGL();
#endif
	}
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
	if (e->buttons() & Qt::RightButton)
	{
		lastPosScale = e->pos();
	}
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
	bool update_ = false;
	if (e->buttons() & Qt::LeftButton)
	{
		set_win_old_position(lastPos.x(), lastPos.y());
		set_win_new_position(e->x(), e->y());
		lastPos = e->pos();
		update_ = true;
	}
	else if (e->buttons() & Qt::MidButton)
	{
		const float deltax = e->x() - lastPanPos.x();
		const float deltay = e->y() - lastPanPos.y();
		set_pan_delta(deltax, deltay);
		lastPanPos = e->pos();
		update_ = true;
	}
	else if (e->buttons() & Qt::RightButton)
	{
		const int dy = lastPosScale.y() - e->y();
		const float  tmp0 = ortho_size - (float)dy;
		const double tmp1 = position_z - (double)dy;
		if (tmp0 < 0.001f) ortho_size = 0.001f;
		else ortho_size = tmp0;
		if (tmp1 < 0.001) position_z = 0.001;
		else position_z = tmp1;
		lastPosScale = e->pos();
		update_ = true;
	}
	if (update_) updateGL();
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
	double incr = 4.0;
	if (Qt::ControlModifier==QApplication::keyboardModifiers())
	{
		incr = 0.1;
	}
	else if (Qt::ShiftModifier==QApplication::keyboardModifiers())
	{
		incr = 25.0;
	}
	else { ;; }
	if (e->delta() > 0)
	{
		ortho_size += (float)incr;
		position_z += incr;
	}
	else
	{
		ortho_size -= (float)incr;
		position_z -= incr;
	}
	if (ortho_size < 0.001f) ortho_size = 0.001f;
	if (position_z < 0.001)  position_z = 0.001;
	updateGL();
}

void GLWidget::keyPressEvent(QKeyEvent * e)
{
	bool update_ = false;
	switch (e->key())
	{
	case Qt::Key_Plus:
		{
			zoom_in();
		}
		break;
	case Qt::Key_Minus:
		{
			zoom_out();
		}
		break;
	case Qt::Key_Right:
		{
			set_pan_delta(1,0);
			update_ = true;
		}
		break;
	case Qt::Key_Left:
		{
			set_pan_delta(-1,0);
			update_ = true;
		}
		break;
	case Qt::Key_Up:
		{
			set_pan_delta(0,-1);
			update_ = true;
		}
		break;
	case Qt::Key_Down:
		{
			set_pan_delta(0,1);
			update_ = true;
		}
		break;
	case Qt::Key_G:
		{
			if (Qt::ShiftModifier == QApplication::keyboardModifiers())
				get_screen(true);
			else
				get_screen(false);
		}
		break;
	case Qt::Key_W:
		{
			set_wireframe(!wireframe);
		}
		break;
	default:
		break;
	}
	if (update_) updateGL();
}

void GLWidget::set_ortho(bool t)
{
	ortho_proj = t;
	updateGL();
}

void GLWidget::set_fov(double i)
{
	fov = (float)i;
	updateGL();
}

void GLWidget::set_far(double i)
{
	far_plane = (float)i;
	updateGL();
}

void GLWidget::set_draw_frames_3d(bool t)
{
	draw_frames_3d = t;
	if (view == 0) updateGL();
}

void GLWidget::set_display_contours(bool t)
{
	display_contours = t;
	if (view == 0) updateGL();
}

void GLWidget::zoom_in()
{
	double incr = 4.0;
	if (Qt::ControlModifier==QApplication::keyboardModifiers())
	{
		incr = 0.1;
	}
	else if (Qt::ShiftModifier==QApplication::keyboardModifiers())
	{
		incr = 25.0;
	}
	ortho_size -= (float)incr;
	position_z -= incr;
	if (ortho_size < 0.001f) ortho_size = 0.001f;
	if (position_z < 0.001)  position_z = 0.001;
	updateGL();
}

void GLWidget::zoom_out()
{
	double incr = 4.0;
	if (Qt::ControlModifier==QApplication::keyboardModifiers())
	{
		incr = 0.1;
	}
	else if (Qt::ShiftModifier==QApplication::keyboardModifiers())
	{
		incr = 25.0;
	}
	ortho_size += (float)incr;
	position_z += incr;
	if (ortho_size < 0.001f) ortho_size = 0.001f;
	if (position_z < 0.001)  position_z = 0.001;
	updateGL();
}

void GLWidget::update_clear_color()
{
	QColor color0 = qApp->palette().color(QPalette::Window);
	set_clear_color((float)color0.redF(),(float)color0.greenF(),(float)color0.blueF());
}

void GLWidget::set_wireframe(bool t)
{
	wireframe = t;
	updateGL();
}

void GLWidget::set_skip_draw(bool t)
{
	if (!t) updateGL();
}

void GLWidget::set_alpha(double a)
{
	alpha = (float)a;
	updateGL();
}

void GLWidget::set_brightness(double a)
{
	brightness = (float)a;
	updateGL();
}

void GLWidget::set_cube(bool t)
{
	show_cube = t;
	updateGL();
}

void GLWidget::get_screen(bool white_bg)
{
	const float r = clear_color_r;
	const float g = clear_color_g;
	const float b = clear_color_b;
	if (white_bg)
	{
		set_clear_color(1.0, 1.0, 1.0);
		updateGL();
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QImage p = grabFramebuffer();
#else
	QImage p = grabFrameBuffer(false);
#endif
	if (white_bg)
	{
		set_clear_color(r, g, b);
		updateGL();
	}
	if (p.isNull()) return;
	const QString saved_dir = CommonUtils::get_screenshot_dir();
	const QString d = CommonUtils::get_screenshot_name(saved_dir);
	const QString f = QFileDialog::getSaveFileName(
		NULL,
		QString("Save PNG file"),
		d,
		QString("All Files (*)"),
		(QString*)NULL
		//,QFileDialog::DontUseNativeDialog
		);
	if (f.isEmpty()) return;
	QString f__;
	QFileInfo fi(f);
	CommonUtils::set_screenshot_dir(
		QDir::toNativeSeparators(fi.absolutePath()));
	QString ext = fi.suffix();
	if (ext.isEmpty())
	{
		f__ = QDir::toNativeSeparators(f + QString(".png"));
	}
	else if ((ext.toUpper() != QString("PNG")))
	{
		f__ = QDir::toNativeSeparators(
			fi.absolutePath() +
			QDir::separator() +
			fi.baseName() +
			QString(".png"));
	}
	else
	{
		f__ = QDir::toNativeSeparators(f);
	}
	if (!p.save(f__, "PNG"))
	{
		QMessageBox mbox;
		mbox.setWindowModality(Qt::ApplicationModal);
		mbox.addButton(QMessageBox::Close);
		mbox.setIcon(QMessageBox::Warning);
		mbox.setText(QString("Could not save file"));
		mbox.exec();
	}
}

void GLWidget::set_contours_width(float f)
{
	contours_width = (f < 0.1) ? 2.0f : f;
	if (view == 0) updateGL();
}

void GLWidget::init_()
{
	skip_draw = false;
	no_opengl3 = true;
	max_tex_size = 0;
	max_3d_texture_size = 0;
	opengl_init_done = false;
	gallium = false;
	fsquad_shader.program = 0;
	zero_shader.program = 0;
	raycast_shader.program = 0;
	raycast_color_shader.program = 0;
	raycast_shader_bb.program = 0;
	raycast_color_shader_bb.program = 0;
	raycast_shader_sigm.program = 0;
	raycast_color_shader_sigm.program = 0;
	raycast_shader_bb_sigm.program = 0;
	raycast_color_shader_bb_sigm.program = 0;
	c3d_shader_clamp.program = 0;
	c3d_shader_gradient_clamp.program = 0;
	c3d_shader_bb_clamp.program = 0;
	c3d_shader_gradient_bb_clamp.program = 0;
	c3d_shader.program = 0;
	c3d_shader_gradient.program = 0;
	c3d_shader_bb.program = 0;
	c3d_shader_gradient_bb.program = 0;
	c3d_shader_clamp_sigm.program = 0;
	c3d_shader_gradient_clamp_sigm.program = 0;
	c3d_shader_bb_clamp_sigm.program = 0;
	c3d_shader_gradient_bb_clamp_sigm.program = 0;
	c3d_shader_sigm.program = 0;
	c3d_shader_gradient_sigm.program = 0;
	c3d_shader_bb_sigm.program = 0;
	c3d_shader_gradient_bb_sigm.program = 0;
	c3d_shader_clamp_vbo = new GLuint[2];
	c3d_shader_gradient_clamp_vbo = new GLuint[2];
	c3d_shader_bb_clamp_vbo = new GLuint[2];
	c3d_shader_gradient_bb_clamp_vbo = new GLuint[2];
	c3d_shader_vbo = new GLuint[2];
	c3d_shader_gradient_vbo = new GLuint[2];
	c3d_shader_bb_vbo = new GLuint[2];
	c3d_shader_gradient_bb_vbo = new GLuint[2];
	c3d_shader_clamp_sigm_vbo = new GLuint[2];
	c3d_shader_gradient_clamp_sigm_vbo = new GLuint[2];
	c3d_shader_bb_clamp_sigm_vbo = new GLuint[2];
	c3d_shader_gradient_bb_clamp_sigm_vbo = new GLuint[2];
	c3d_shader_sigm_vbo = new GLuint[2];
	c3d_shader_gradient_sigm_vbo = new GLuint[2];
	c3d_shader_bb_sigm_vbo = new GLuint[2];
	c3d_shader_gradient_bb_sigm_vbo = new GLuint[2];
	raycastcube0 = new GLuint[2];
	raycastcube0_vao = 0;
	raycastcube1 = new GLuint[2];
	raycastcube1_vao = 0;
	raycastcube2 = new GLuint[2];
	raycastcube2_vao = 0;
	raycast_shader_vao = new GLuint[3];
	raycast_color_shader_vao = new GLuint[3];
	raycast_shader_bb_vao = new GLuint[3];
	raycast_color_shader_bb_vao = new GLuint[3];
	raycast_shader_sigm_vao = new GLuint[3];
	raycast_color_shader_sigm_vao = new GLuint[3];
	raycast_shader_bb_sigm_vao = new GLuint[3];
	raycast_color_shader_bb_sigm_vao = new GLuint[3];
	raycast_shader_vbo0 = 0;
	raycast_shader_vbo1 = 0;
	raycast_shader_vbo2 = 0;
	raycast_color_shader_vbo0 = 0;
	raycast_color_shader_vbo1 = 0;
	raycast_color_shader_vbo2 = 0;
	raycast_shader_bb_vbo0 = 0;
	raycast_shader_bb_vbo1 = 0;
	raycast_shader_bb_vbo2 = 0;
	raycast_color_shader_bb_vbo0 = 0;
	raycast_color_shader_bb_vbo1 = 0;
	raycast_color_shader_bb_vbo2 = 0;
	raycast_shader_sigm_vbo0 = 0;
	raycast_shader_sigm_vbo1 = 0;
	raycast_shader_sigm_vbo2 = 0;
	raycast_color_shader_sigm_vbo0 = 0;
	raycast_color_shader_sigm_vbo1 = 0;
	raycast_color_shader_sigm_vbo2 = 0;
	raycast_shader_bb_sigm_vbo0 = 0;
	raycast_shader_bb_sigm_vbo1 = 0;
	raycast_shader_bb_sigm_vbo2 = 0;
	raycast_color_shader_bb_sigm_vbo0 = 0;
	raycast_color_shader_bb_sigm_vbo1 = 0;
	raycast_color_shader_bb_sigm_vbo2 = 0;
	c3d_shader_clamp_vao = 0;
	c3d_shader_gradient_clamp_vao = 0;
	c3d_shader_bb_clamp_vao = 0;
	c3d_shader_gradient_bb_clamp_vao = 0;
	c3d_shader_vao = 0;
	c3d_shader_gradient_vao = 0;
	c3d_shader_bb_vao = 0;
	c3d_shader_gradient_bb_vao = 0;
	c3d_shader_clamp_sigm_vao = 0;
	c3d_shader_gradient_clamp_sigm_vao = 0;
	c3d_shader_bb_clamp_sigm_vao = 0;
	c3d_shader_gradient_bb_clamp_sigm_vao = 0;
	c3d_shader_sigm_vao = 0;
	c3d_shader_gradient_sigm_vao = 0;
	c3d_shader_bb_sigm_vao = 0;
	c3d_shader_gradient_bb_sigm_vao = 0;
	frame_shader.program = 0;
	simple_tex_shader.program = 0;
	sphere0_shader.program = 0;
	color_shader.program = 0;
	orientcube_shader.program = 0;
	mesh_shader.program = 0;
	view = 0;
	axis = 2;
	gradient1  = 0;
	gradient2  = 0;
	gradient3  = 0;
	gradient4  = 0;
	gradient5  = 0;
	gradient6  = 0;
	gradient7  = 0;
	x_rotation = 0;
	y_rotation = 0;
	z_rotation = 0;
	ortho_size  = (float)SCENE_ORTHO_SIZE;
	ortho_proj = true;
	position_z  = (float)SCENE_POS_Z;
	fov = (float)SCENE_FOV;
	far_plane = (float)SCENE_FAR_PLANE;
	alpha = (float)SCENE_ALPHA;
	brightness = 1.0f;
	m_collisionWorld = NULL;
	draw_frames_3d = false;
	display_contours = true;
	old_win_pos_x  = 0;
	old_win_pos_y  = 0;
	new_win_pos_x  = 0;
	new_win_pos_y  = 0;
	rect_selection = false;
	show_cube = true;
	wireframe = false;
	contours_width = 2.0f;
	pan_x  = 0;
	pan_y  = 0;
	clear_color_r = 0.9f;
	clear_color_g = 0.9f;
	clear_color_b = 0.9f;
	camera = new Camera;
	camera->reset();
}

GLWidget::~GLWidget()
{
	selected_images__ = NULL; 
}

void GLWidget::close_()
{
	if (no_opengl3) return;
	if (!opengl_init_done) return;
	makeCurrent();
	if (gradient1>0) {glDeleteTextures(1,&gradient1);gradient1=0;}
	if (gradient2>0) {glDeleteTextures(1,&gradient2);gradient2=0;}
	if (gradient3>0) {glDeleteTextures(1,&gradient3);gradient3=0;}
	if (gradient4>0) {glDeleteTextures(1,&gradient4);gradient4=0;}
	if (gradient5>0) {glDeleteTextures(1,&gradient5);gradient5=0;}
	if (gradient6>0) {glDeleteTextures(1,&gradient6);gradient6=0;}
	if (gradient7>0) {glDeleteTextures(1,&gradient7);gradient7=0;}
	for (unsigned int x = 0; x < shaders.size(); x++)
	{
		if (shaders.at(x)->program != 0)
		{
			glDetachShader(shaders[x]->program, shaders[x]->vshader);
			glDetachShader(shaders[x]->program, shaders[x]->fshader);
			glDeleteShader(shaders[x]->vshader);
			glDeleteShader(shaders[x]->fshader);
			glDeleteProgram(shaders[x]->program);
			shaders[x]->program = 0;
		}
	}
	shaders.clear();
	for (unsigned int x = 0; x < qmeshes.size(); x++)
	{
		if (qmeshes[x]) delete qmeshes[x];
	}
	qmeshes.clear();
	for (unsigned int x = 0; x < vboids.size(); x++)
	{
		glDeleteBuffers(2, vboids[x]); // size 2
		increment_count_vbos(-2);
		delete[] vboids[x];
	}
	vboids.clear();
	for (unsigned int x = 0; x < vaoids.size(); x++)
	{
		glDeleteVertexArrays(1, &(vaoids[x]));
	}
	vaoids.clear();
	glDeleteVertexArrays(3, raycast_shader_vao);
	delete [] raycast_shader_vao;
	glDeleteVertexArrays(3, raycast_color_shader_vao);
	delete [] raycast_color_shader_vao;
	glDeleteVertexArrays(3, raycast_shader_bb_vao);
	delete [] raycast_shader_bb_vao;
	glDeleteVertexArrays(3, raycast_color_shader_bb_vao);
	delete [] raycast_color_shader_bb_vao;
	glDeleteVertexArrays(3, raycast_shader_sigm_vao);
	delete [] raycast_shader_sigm_vao;
	glDeleteVertexArrays(3, raycast_color_shader_sigm_vao);
	delete [] raycast_color_shader_sigm_vao;
	glDeleteVertexArrays(3, raycast_shader_bb_sigm_vao);
	delete [] raycast_shader_bb_sigm_vao;
	glDeleteVertexArrays(3, raycast_color_shader_bb_sigm_vao);
	delete [] raycast_color_shader_bb_sigm_vao;
	glDeleteBuffers(1, &raycast_shader_vbo0);
	glDeleteBuffers(1, &raycast_shader_vbo1);
	glDeleteBuffers(1, &raycast_shader_vbo2);
	glDeleteBuffers(1, &raycast_color_shader_vbo0);
	glDeleteBuffers(1, &raycast_color_shader_vbo1);
	glDeleteBuffers(1, &raycast_color_shader_vbo2);
	glDeleteBuffers(1, &raycast_shader_bb_vbo0);
	glDeleteBuffers(1, &raycast_shader_bb_vbo1);
	glDeleteBuffers(1, &raycast_shader_bb_vbo2);
	glDeleteBuffers(1, &raycast_color_shader_bb_vbo0);
	glDeleteBuffers(1, &raycast_color_shader_bb_vbo1);
	glDeleteBuffers(1, &raycast_color_shader_bb_vbo2);
	glDeleteBuffers(1, &raycast_shader_sigm_vbo0);
	glDeleteBuffers(1, &raycast_shader_sigm_vbo1);
	glDeleteBuffers(1, &raycast_shader_sigm_vbo2);
	glDeleteBuffers(1, &raycast_color_shader_sigm_vbo0);
	glDeleteBuffers(1, &raycast_color_shader_sigm_vbo1);
	glDeleteBuffers(1, &raycast_color_shader_sigm_vbo2);
	glDeleteBuffers(1, &raycast_shader_bb_sigm_vbo0);
	glDeleteBuffers(1, &raycast_shader_bb_sigm_vbo1);
	glDeleteBuffers(1, &raycast_shader_bb_sigm_vbo2);
	glDeleteBuffers(1, &raycast_color_shader_bb_sigm_vbo0);
	glDeleteBuffers(1, &raycast_color_shader_bb_sigm_vbo1);
	glDeleteBuffers(1, &raycast_color_shader_bb_sigm_vbo2);
	increment_count_vbos(-24);
	for (unsigned int x = 0; x < textures.size(); x++)
	{
		glDeleteTextures(1, textures[x]);
	}
	textures.clear();
	free_fbos0(&framebuffer,    &fbo_tex,      &fbo_depth);
	free_fbos0(&cubebuffer,     &cube_tex,     &cube_depth);
	free_fbos1(&backfacebuffer, &backface_tex, &backface_depth);
	free_fbos1(&frontfacebuffer,&frontface_tex,&frontface_depth);
	glDeleteVertexArrays(1, &scene_vao);
	glDeleteBuffers(1, &scene_vbo);
	increment_count_vbos(-1);
	glDeleteVertexArrays(1, &frames_vao);
	glDeleteBuffers(1, &frames_vbo);
	increment_count_vbos(-1);
	glDeleteVertexArrays(1, &origin_vao);
	glDeleteBuffers(1, &origin_vbo);
	increment_count_vbos(-1);
	if (camera)
	{
		delete camera;
		camera = NULL;
	}
	opengl_init_done = false;
#if 0
	std::cout
		<< "Num VBOs at exit "
		<< get_count_vbos()
		<< std::endl;
#endif
}

void GLWidget::init_opengl(int w, int h)
{
	srand(time(NULL));
	if (opengl_init_done) return;
	opengl_init_done = true;
	for (int i = 0; i < 16; i++) mparams[i] = 0.0f;
	for (int i = 0; i <  6; i++) sparams[i] = 0.0f;
	win_w = w;
	win_h = h;
	opengl_info.clear();
	//
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		return;
	}
	if (glewIsSupported("GL_VERSION_3_0"))
	{
		no_opengl3 = false;
	}
	else
	{
		no_opengl3 = true;
		opengl_info.append(QString("\nOpenGL modules disabled"));
		return;
	}
#endif
	//
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	const QString vendor_str =
		QVariant(reinterpret_cast<const char*>(
			glGetString(GL_VENDOR))).toString();
	opengl_info.append(vendor_str + QString("\n"));
	const QString renderer_str  =
			QVariant(reinterpret_cast<const char*>(
				glGetString(GL_RENDERER))).toString();
	opengl_info.append(renderer_str);
	opengl_info.append(QString("\nOpenGL "));
	opengl_info.append(QVariant(reinterpret_cast<const char*>(
		glGetString(GL_VERSION))).toString());
	opengl_info.append(QString("\nGLSL "));
	opengl_info.append(QVariant(reinterpret_cast<const char*>(
		glGetString(GL_SHADING_LANGUAGE_VERSION))).toString());
	opengl_info.append(QString(" "));
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
	//
	opengl_info.append(
		QString("\n") +
		QVariant(max_tex_size).toString() +
		QString(", ") +
		QVariant(max_3d_texture_size).toString());
	//
	if (renderer_str.contains(QString("Gallium"),Qt::CaseInsensitive))
	{
		gallium = true;
	}
	//
	//
	//
	if (major >= 3) no_opengl3 = false;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	if (no_opengl3)
	{
		opengl_info.append(QString("\nOpenGL modules disabled"));
		return;
	}
#endif
	camera->set_heading(0.0f);
	camera->set_pitch(0.0f);
	camera->set_position(0.0f,0.0f,(float)SCENE_POS_Z);
	//
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
 	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
#if 0
	GLint max_samples_;
	glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &max_samples_);
	std::cout << "GL_MAX_INTEGER_SAMPLES " << max_samples_ << std::endl;

	GLfloat sample_coverage_;
	glGetFloatv(GL_SAMPLE_COVERAGE_VALUE, &sample_coverage_);
	std::cout << "GL_SAMPLE_COVERAGE_VALUE " << sample_coverage_ << std::endl;

	GLint samples_;
	glGetIntegerv(GL_SAMPLES, &samples_);
	std::cout << "GL_SAMPLES " << samples_ << std::endl;
#endif
	create_program(c3d_vs, c3d_fs, &c3d_shader);
	c3d_shader.location_mvp        = glGetUniformLocation(c3d_shader.program, "mvp");
	c3d_shader.location_sampler[0] = glGetUniformLocation(c3d_shader.program, "sampler0");
	c3d_shader.position_handle     = glGetAttribLocation (c3d_shader.program, "v_position");
	c3d_shader.texture_handle[0]   = glGetAttribLocation (c3d_shader.program, "v_texcoord0");
	c3d_shader.location_mparams    = glGetUniformLocation(c3d_shader.program, "mparams");
	shaders.push_back(&c3d_shader);
	generate_vao1(
		&c3d_shader_vao,
		c3d_shader_vbo,
		&(c3d_shader.position_handle),
		&(c3d_shader.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_bb, &c3d_shader_bb);
	c3d_shader_bb.location_mvp        = glGetUniformLocation(c3d_shader_bb.program, "mvp");
	c3d_shader_bb.location_sampler[0] = glGetUniformLocation(c3d_shader_bb.program, "sampler0");
	c3d_shader_bb.position_handle     = glGetAttribLocation (c3d_shader_bb.program, "v_position");
	c3d_shader_bb.texture_handle[0]   = glGetAttribLocation (c3d_shader_bb.program, "v_texcoord0");
	c3d_shader_bb.location_mparams    = glGetUniformLocation(c3d_shader_bb.program, "mparams");
	shaders.push_back(&c3d_shader_bb);
	generate_vao1(
		&c3d_shader_bb_vao,
		c3d_shader_bb_vbo,
		&(c3d_shader_bb.position_handle),
		&(c3d_shader_bb.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_clamp, &c3d_shader_clamp);
	c3d_shader_clamp.location_mvp        = glGetUniformLocation(c3d_shader_clamp.program, "mvp");
	c3d_shader_clamp.location_sampler[0] = glGetUniformLocation(c3d_shader_clamp.program, "sampler0");
	c3d_shader_clamp.position_handle     = glGetAttribLocation (c3d_shader_clamp.program, "v_position");
	c3d_shader_clamp.texture_handle[0]   = glGetAttribLocation (c3d_shader_clamp.program, "v_texcoord0");
	c3d_shader_clamp.location_mparams    = glGetUniformLocation(c3d_shader_clamp.program, "mparams");
	shaders.push_back(&c3d_shader_clamp);
	generate_vao1(
		&c3d_shader_clamp_vao,
		c3d_shader_clamp_vbo,
		&(c3d_shader_clamp.position_handle),
		&(c3d_shader_clamp.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_bb_clamp, &c3d_shader_bb_clamp);
	c3d_shader_bb_clamp.location_mvp        = glGetUniformLocation(c3d_shader_bb_clamp.program, "mvp");
	c3d_shader_bb_clamp.location_sampler[0] = glGetUniformLocation(c3d_shader_bb_clamp.program, "sampler0");
	c3d_shader_bb_clamp.position_handle     = glGetAttribLocation (c3d_shader_bb_clamp.program, "v_position");
	c3d_shader_bb_clamp.texture_handle[0]   = glGetAttribLocation (c3d_shader_bb_clamp.program, "v_texcoord0");
	c3d_shader_bb_clamp.location_mparams    = glGetUniformLocation(c3d_shader_bb_clamp.program, "mparams");
	shaders.push_back(&c3d_shader_bb_clamp);
	generate_vao1(
		&c3d_shader_bb_clamp_vao,
		c3d_shader_bb_clamp_vbo,
		&(c3d_shader_bb_clamp.position_handle),
		&(c3d_shader_bb_clamp.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient, &c3d_shader_gradient);
	c3d_shader_gradient.location_mvp        = glGetUniformLocation(c3d_shader_gradient.program, "mvp");
	c3d_shader_gradient.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient.program, "sampler0");
	c3d_shader_gradient.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient.program, "sampler1");
	c3d_shader_gradient.position_handle     = glGetAttribLocation (c3d_shader_gradient.program, "v_position");
	c3d_shader_gradient.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient.program, "v_texcoord0");
	c3d_shader_gradient.location_mparams    = glGetUniformLocation(c3d_shader_gradient.program, "mparams");
	shaders.push_back(&c3d_shader_gradient);
	generate_vao1(
		&c3d_shader_gradient_vao,
		c3d_shader_gradient_vbo,
		&(c3d_shader_gradient.position_handle),
		&(c3d_shader_gradient.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient_bb, &c3d_shader_gradient_bb);
	c3d_shader_gradient_bb.location_mvp        = glGetUniformLocation(c3d_shader_gradient_bb.program, "mvp");
	c3d_shader_gradient_bb.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient_bb.program, "sampler0");
	c3d_shader_gradient_bb.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient_bb.program, "sampler1");
	c3d_shader_gradient_bb.position_handle     = glGetAttribLocation (c3d_shader_gradient_bb.program, "v_position");
	c3d_shader_gradient_bb.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient_bb.program, "v_texcoord0");
	c3d_shader_gradient_bb.location_mparams    = glGetUniformLocation(c3d_shader_gradient_bb.program, "mparams");
	shaders.push_back(&c3d_shader_gradient_bb);
	generate_vao1(
		&c3d_shader_gradient_bb_vao,
		c3d_shader_gradient_bb_vbo,
		&(c3d_shader_gradient_bb.position_handle),
		&(c3d_shader_gradient_bb.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient_clamp, &c3d_shader_gradient_clamp);
	c3d_shader_gradient_clamp.location_mvp        = glGetUniformLocation(c3d_shader_gradient_clamp.program, "mvp");
	c3d_shader_gradient_clamp.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient_clamp.program, "sampler0");
	c3d_shader_gradient_clamp.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient_clamp.program, "sampler1");
	c3d_shader_gradient_clamp.position_handle     = glGetAttribLocation (c3d_shader_gradient_clamp.program, "v_position");
	c3d_shader_gradient_clamp.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient_clamp.program, "v_texcoord0");
	c3d_shader_gradient_clamp.location_mparams    = glGetUniformLocation(c3d_shader_gradient_clamp.program, "mparams");
	shaders.push_back(&c3d_shader_gradient_clamp);
	generate_vao1(
		&c3d_shader_gradient_clamp_vao,
		c3d_shader_gradient_clamp_vbo,
		&(c3d_shader_gradient_clamp.position_handle),
		&(c3d_shader_gradient_clamp.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient_bb_clamp, &c3d_shader_gradient_bb_clamp);
	c3d_shader_gradient_bb_clamp.location_mvp        = glGetUniformLocation(c3d_shader_gradient_bb_clamp.program, "mvp");
	c3d_shader_gradient_bb_clamp.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient_bb_clamp.program, "sampler0");
	c3d_shader_gradient_bb_clamp.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient_bb_clamp.program, "sampler1");
	c3d_shader_gradient_bb_clamp.position_handle     = glGetAttribLocation (c3d_shader_gradient_bb_clamp.program, "v_position");
	c3d_shader_gradient_bb_clamp.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient_bb_clamp.program, "v_texcoord0");
	c3d_shader_gradient_bb_clamp.location_mparams    = glGetUniformLocation(c3d_shader_gradient_bb_clamp.program, "mparams");
	shaders.push_back(&c3d_shader_gradient_bb_clamp);
	generate_vao1(
		&c3d_shader_gradient_bb_clamp_vao,
		c3d_shader_gradient_bb_clamp_vbo,
		&(c3d_shader_gradient_bb_clamp.position_handle),
		&(c3d_shader_gradient_bb_clamp.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_sigm, &c3d_shader_sigm);
	c3d_shader_sigm.location_mvp        = glGetUniformLocation(c3d_shader_sigm.program, "mvp");
	c3d_shader_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_sigm.program, "sampler0");
	c3d_shader_sigm.position_handle     = glGetAttribLocation (c3d_shader_sigm.program, "v_position");
	c3d_shader_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_sigm.program, "v_texcoord0");
	c3d_shader_sigm.location_mparams    = glGetUniformLocation(c3d_shader_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_sigm);
	generate_vao1(
		&c3d_shader_sigm_vao,
		c3d_shader_sigm_vbo,
		&(c3d_shader_sigm.position_handle),
		&(c3d_shader_sigm.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_bb_sigm, &c3d_shader_bb_sigm);
	c3d_shader_bb_sigm.location_mvp        = glGetUniformLocation(c3d_shader_bb_sigm.program, "mvp");
	c3d_shader_bb_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_bb_sigm.program, "sampler0");
	c3d_shader_bb_sigm.position_handle     = glGetAttribLocation (c3d_shader_bb_sigm.program, "v_position");
	c3d_shader_bb_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_bb_sigm.program, "v_texcoord0");
	c3d_shader_bb_sigm.location_mparams    = glGetUniformLocation(c3d_shader_bb_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_bb_sigm);
	generate_vao1(
		&c3d_shader_bb_sigm_vao,
		c3d_shader_bb_sigm_vbo,
		&(c3d_shader_bb_sigm.position_handle),
		&(c3d_shader_bb_sigm.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_clamp_sigm, &c3d_shader_clamp_sigm);
	c3d_shader_clamp_sigm.location_mvp        = glGetUniformLocation(c3d_shader_clamp_sigm.program, "mvp");
	c3d_shader_clamp_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_clamp_sigm.program, "sampler0");
	c3d_shader_clamp_sigm.position_handle     = glGetAttribLocation (c3d_shader_clamp_sigm.program, "v_position");
	c3d_shader_clamp_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_clamp_sigm.program, "v_texcoord0");
	c3d_shader_clamp_sigm.location_mparams    = glGetUniformLocation(c3d_shader_clamp_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_clamp_sigm);
	generate_vao1(
		&c3d_shader_clamp_sigm_vao,
		c3d_shader_clamp_sigm_vbo,
		&(c3d_shader_clamp_sigm.position_handle),
		&(c3d_shader_clamp_sigm.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_bb_clamp_sigm, &c3d_shader_bb_clamp_sigm);
	c3d_shader_bb_clamp_sigm.location_mvp        = glGetUniformLocation(c3d_shader_bb_clamp_sigm.program, "mvp");
	c3d_shader_bb_clamp_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_bb_clamp_sigm.program, "sampler0");
	c3d_shader_bb_clamp_sigm.position_handle     = glGetAttribLocation (c3d_shader_bb_clamp_sigm.program, "v_position");
	c3d_shader_bb_clamp_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_bb_clamp_sigm.program, "v_texcoord0");
	c3d_shader_bb_clamp_sigm.location_mparams    = glGetUniformLocation(c3d_shader_bb_clamp_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_bb_clamp_sigm);
	generate_vao1(
		&c3d_shader_bb_clamp_sigm_vao,
		c3d_shader_bb_clamp_sigm_vbo,
		&(c3d_shader_bb_clamp_sigm.position_handle),
		&(c3d_shader_bb_clamp_sigm.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient_sigm, &c3d_shader_gradient_sigm);
	c3d_shader_gradient_sigm.location_mvp        = glGetUniformLocation(c3d_shader_gradient_sigm.program, "mvp");
	c3d_shader_gradient_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient_sigm.program, "sampler0");
	c3d_shader_gradient_sigm.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient_sigm.program, "sampler1");
	c3d_shader_gradient_sigm.position_handle     = glGetAttribLocation (c3d_shader_gradient_sigm.program, "v_position");
	c3d_shader_gradient_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient_sigm.program, "v_texcoord0");
	c3d_shader_gradient_sigm.location_mparams    = glGetUniformLocation(c3d_shader_gradient_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_gradient_sigm);
	generate_vao1(
		&c3d_shader_gradient_sigm_vao,
		c3d_shader_gradient_sigm_vbo,
		&(c3d_shader_gradient_sigm.position_handle),
		&(c3d_shader_gradient_sigm.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient_bb_sigm, &c3d_shader_gradient_bb_sigm);
	c3d_shader_gradient_bb_sigm.location_mvp        = glGetUniformLocation(c3d_shader_gradient_bb_sigm.program, "mvp");
	c3d_shader_gradient_bb_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient_bb_sigm.program, "sampler0");
	c3d_shader_gradient_bb_sigm.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient_bb_sigm.program, "sampler1");
	c3d_shader_gradient_bb_sigm.position_handle     = glGetAttribLocation (c3d_shader_gradient_bb_sigm.program, "v_position");
	c3d_shader_gradient_bb_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient_bb_sigm.program, "v_texcoord0");
	c3d_shader_gradient_bb_sigm.location_mparams    = glGetUniformLocation(c3d_shader_gradient_bb_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_gradient_bb_sigm);
	generate_vao1(
		&c3d_shader_gradient_bb_sigm_vao,
		c3d_shader_gradient_bb_sigm_vbo,
		&(c3d_shader_gradient_bb_sigm.position_handle),
		&(c3d_shader_gradient_bb_sigm.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient_clamp_sigm, &c3d_shader_gradient_clamp_sigm);
	c3d_shader_gradient_clamp_sigm.location_mvp        = glGetUniformLocation(c3d_shader_gradient_clamp_sigm.program, "mvp");
	c3d_shader_gradient_clamp_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient_clamp_sigm.program, "sampler0");
	c3d_shader_gradient_clamp_sigm.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient_clamp_sigm.program, "sampler1");
	c3d_shader_gradient_clamp_sigm.position_handle     = glGetAttribLocation (c3d_shader_gradient_clamp_sigm.program, "v_position");
	c3d_shader_gradient_clamp_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient_clamp_sigm.program, "v_texcoord0");
	c3d_shader_gradient_clamp_sigm.location_mparams    = glGetUniformLocation(c3d_shader_gradient_clamp_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_gradient_clamp_sigm);
		generate_vao1(
		&c3d_shader_gradient_clamp_sigm_vao,
		c3d_shader_gradient_clamp_sigm_vbo,
		&(c3d_shader_gradient_clamp_sigm.position_handle),
		&(c3d_shader_gradient_clamp_sigm.texture_handle[0]));
	//
	create_program(c3d_vs, c3d_fs_gradient_bb_clamp_sigm, &c3d_shader_gradient_bb_clamp_sigm);
	c3d_shader_gradient_bb_clamp_sigm.location_mvp        = glGetUniformLocation(c3d_shader_gradient_bb_clamp_sigm.program, "mvp");
	c3d_shader_gradient_bb_clamp_sigm.location_sampler[0] = glGetUniformLocation(c3d_shader_gradient_bb_clamp_sigm.program, "sampler0");
	c3d_shader_gradient_bb_clamp_sigm.location_sampler[1] = glGetUniformLocation(c3d_shader_gradient_bb_clamp_sigm.program, "sampler1");
	c3d_shader_gradient_bb_clamp_sigm.position_handle     = glGetAttribLocation (c3d_shader_gradient_bb_clamp_sigm.program, "v_position");
	c3d_shader_gradient_bb_clamp_sigm.texture_handle[0]   = glGetAttribLocation (c3d_shader_gradient_bb_clamp_sigm.program, "v_texcoord0");
	c3d_shader_gradient_bb_clamp_sigm.location_mparams    = glGetUniformLocation(c3d_shader_gradient_bb_clamp_sigm.program, "mparams");
	shaders.push_back(&c3d_shader_gradient_bb_clamp_sigm);
		generate_vao1(
		&c3d_shader_gradient_bb_clamp_sigm_vao,
		c3d_shader_gradient_bb_clamp_sigm_vbo,
		&(c3d_shader_gradient_bb_clamp_sigm.position_handle),
		&(c3d_shader_gradient_bb_clamp_sigm.texture_handle[0]));
	//
	create_program(frame_vs, frame_fs, &frame_shader);
	frame_shader.location_mvp        = glGetUniformLocation(frame_shader.program, "mvp");
	frame_shader.location_K          = glGetUniformLocation(frame_shader.program, "K");
	frame_shader.position_handle     = glGetAttribLocation (frame_shader.program, "v_position");
	shaders.push_back(&frame_shader);
	//
	create_program(color_vs, orientcube_fs, &orientcube_shader);
	orientcube_shader.position_handle         = glGetAttribLocation (orientcube_shader.program, "v_position");
	orientcube_shader.normal_handle           = glGetAttribLocation (orientcube_shader.program, "v_normal");
	orientcube_shader.location_K              = glGetUniformLocation(orientcube_shader.program, "K");
	orientcube_shader.location_mvp            = glGetUniformLocation(orientcube_shader.program, "mvp");
	orientcube_shader.location_sparams        = glGetUniformLocation(orientcube_shader.program, "sparams");
	shaders.push_back(&orientcube_shader);
	//
	/*
	create_program(simple_tex_vs, simple_tex_fs, &simple_tex_shader);
	simple_tex_shader.location_mvp        = glGetUniformLocation(simple_tex_shader.program, "mvp");
	simple_tex_shader.location_sampler[0] = glGetUniformLocation(simple_tex_shader.program, "sampler0");
	simple_tex_shader.position_handle     = glGetAttribLocation (simple_tex_shader.program, "v_position");
	simple_tex_shader.texture_handle[0]   = glGetAttribLocation (simple_tex_shader.program, "v_texcoord0");
	shaders.push_back(&simple_tex_shader);
	//
	create_program(TBNf_vs0, TBNf_fs0, &sphere0_shader);    
	sphere0_shader.position_handle         = glGetAttribLocation (sphere0_shader.program, "v_position");
	sphere0_shader.normal_handle           = glGetAttribLocation (sphere0_shader.program, "v_normal");
	sphere0_shader.texture_handle[0]       = glGetAttribLocation (sphere0_shader.program, "v_texcoord0");
	sphere0_shader.tangent_handle          = glGetAttribLocation (sphere0_shader.program, "v_tangent");
	sphere0_shader.location_shininess      = glGetUniformLocation(sphere0_shader.program, "shininess");
	sphere0_shader.location_mvp            = glGetUniformLocation(sphere0_shader.program, "mvp");
	sphere0_shader.location_modeling       = glGetUniformLocation(sphere0_shader.program, "modeling");
	sphere0_shader.location_modeling_inv_t = glGetUniformLocation(sphere0_shader.program, "modeling_inv_t");
	sphere0_shader.location_sparams        = glGetUniformLocation(sphere0_shader.program, "sparams");
	sphere0_shader.location_sampler[0]     = glGetUniformLocation(sphere0_shader.program, "sampler0");
	sphere0_shader.location_sampler[1]     = glGetUniformLocation(sphere0_shader.program, "sampler1");  
	shaders.push_back(&sphere0_shader);
	//
	create_program(color_vs, color_fs, &color_shader);
	color_shader.position_handle         = glGetAttribLocation (color_shader.program, "v_position");
	color_shader.normal_handle           = glGetAttribLocation (color_shader.program, "v_normal");
	color_shader.location_K              = glGetUniformLocation(color_shader.program, "K");
	color_shader.location_shininess      = glGetUniformLocation(color_shader.program, "shininess");
	color_shader.location_mvp            = glGetUniformLocation(color_shader.program, "mvp");
	color_shader.location_modeling       = glGetUniformLocation(color_shader.program, "modeling");
	color_shader.location_modeling_inv_t = glGetUniformLocation(color_shader.program, "modeling_inv_t");
	color_shader.location_sparams        = glGetUniformLocation(color_shader.program, "sparams");
	shaders.push_back(&color_shader);
	*/
	//
	create_program(color_vs, mesh_fs, &mesh_shader);
	mesh_shader.position_handle         = glGetAttribLocation (mesh_shader.program, "v_position");
	mesh_shader.normal_handle           = glGetAttribLocation (mesh_shader.program, "v_normal");
	mesh_shader.location_K              = glGetUniformLocation(mesh_shader.program, "K");
	mesh_shader.location_mvp            = glGetUniformLocation(mesh_shader.program, "mvp");
	mesh_shader.location_sparams        = glGetUniformLocation(mesh_shader.program, "sparams");
	shaders.push_back(&mesh_shader);
	//
	gen_lut_tex(default_lut,default_lut_size,&gradient1);
	gen_lut_tex(black_rainbow_lut,black_rainbow_size,&gradient2);
	gen_lut_tex(syngo_lut,syngo_lut_size,&gradient3);
	gen_lut_tex(hot_iron,hot_iron_size,&gradient4);
	gen_lut_tex(hot_metal_blue,hot_metal_blue_size,&gradient5);
	gen_lut_tex(pet_dicom_lut,pet_dicom_lut_size,&gradient6);
	gen_lut_tex(pet20_dicom_lut,pet20_dicom_lut_size,&gradient7);
	//
	bool ok = create_fbos0(FBO_SIZE__0, FBO_SIZE__0,
			&framebuffer,
			&fbo_tex,
			&fbo_depth);
	if (!ok) { std::cout << "create_fbos0() failed"<< std::endl; }
	create_program(fsquad_vs, fsquad_fs, &fsquad_shader);
	fsquad_shader.location_sampler[0] = glGetUniformLocation(fsquad_shader.program, "sampler0");
	fsquad_shader.position_handle     = glGetAttribLocation (fsquad_shader.program, "v_position");
	shaders.push_back(&fsquad_shader);
	generate_screen_quad(&scene_vbo, &scene_vao, &(fsquad_shader.position_handle));
	//
	{
		float * tmp99 = new float[12];
		for (int x = 0; x < 12; x++) tmp99[x] = 0.0f;
		glGenVertexArrays(1, &frames_vao);
		glBindVertexArray(frames_vao);
		glGenBuffers(1, &frames_vbo);
		increment_count_vbos(1);
		glBindBuffer(GL_ARRAY_BUFFER, frames_vbo);
		glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), tmp99, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(frame_shader.position_handle);
		glBindVertexArray(0);
		//
		glGenVertexArrays(1, &origin_vao);
		glBindVertexArray(origin_vao);
		glGenBuffers(1, &origin_vbo);
		increment_count_vbos(1);
		glBindBuffer(GL_ARRAY_BUFFER, origin_vbo);
		glBufferData(GL_ARRAY_BUFFER, 3*sizeof(GLfloat), tmp99, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(frame_shader.position_handle);
		glBindVertexArray(0);
		delete [] tmp99;
	}
	//
	create_program(zero_vs, zero_fs, &zero_shader);
	zero_shader.location_mvp        = glGetUniformLocation(zero_shader.program, "mvp");
	zero_shader.position_handle     = glGetAttribLocation (zero_shader.program, "v_position");
	zero_shader.color_handle        = glGetAttribLocation (zero_shader.program, "v_color");
	shaders.push_back(&zero_shader);
	generate_raycastcube0_vao(
		&raycastcube0_vao, raycastcube0,
		&(zero_shader.position_handle), &(zero_shader.color_handle));
	generate_raycastcube1_vao(
		&raycastcube1_vao, raycastcube1,
		&(zero_shader.position_handle), &(zero_shader.color_handle));
	generate_raycastcube2_vao(
		&raycastcube2_vao, raycastcube2,
		&(zero_shader.position_handle), &(zero_shader.color_handle));
	ok = create_fbos1(FBO_SIZE__1, FBO_SIZE__1,
			&backfacebuffer,
			&backface_tex,
			&backface_depth);
	if (!ok) { std::cout << "create_fbos1() failed (1)"<< std::endl; }
	ok = create_fbos1(FBO_SIZE__1, FBO_SIZE__1,
			&frontfacebuffer,
			&frontface_tex,
			&frontface_depth);
	if (!ok) { std::cout << "create_fbos1() failed (2)"<< std::endl; }
	create_program(raycast_vs, raycast_fs_bb, &raycast_shader_bb);
	raycast_shader_bb.location_mvp        = glGetUniformLocation(raycast_shader_bb.program, "mvp");
	raycast_shader_bb.position_handle     = glGetAttribLocation (raycast_shader_bb.program, "v_position");
	raycast_shader_bb.location_sampler[0] = glGetUniformLocation(raycast_shader_bb.program, "sampler0");
	raycast_shader_bb.location_sampler[1] = glGetUniformLocation(raycast_shader_bb.program, "sampler1");
	raycast_shader_bb.location_sampler[2] = glGetUniformLocation(raycast_shader_bb.program, "sampler2");
	raycast_shader_bb.location_mparams    = glGetUniformLocation(raycast_shader_bb.program, "mparams");
	shaders.push_back(&raycast_shader_bb);
	generate_raycast_shader_vao(
		raycast_shader_bb_vao,
		&raycast_shader_bb_vbo0, &raycast_shader_bb_vbo1, &raycast_shader_bb_vbo2,
		&(raycast_shader_bb.position_handle));
	//
	create_program(raycast_vs, raycast_color_fs_bb, &raycast_color_shader_bb);
	raycast_color_shader_bb.location_mvp        = glGetUniformLocation(raycast_color_shader_bb.program, "mvp");
	raycast_color_shader_bb.position_handle     = glGetAttribLocation (raycast_color_shader_bb.program, "v_position");
	raycast_color_shader_bb.location_sampler[0] = glGetUniformLocation(raycast_color_shader_bb.program, "sampler0");
	raycast_color_shader_bb.location_sampler[1] = glGetUniformLocation(raycast_color_shader_bb.program, "sampler1");
	raycast_color_shader_bb.location_sampler[2] = glGetUniformLocation(raycast_color_shader_bb.program, "sampler2");
	raycast_color_shader_bb.location_sampler[3] = glGetUniformLocation(raycast_color_shader_bb.program, "sampler3");
	raycast_color_shader_bb.location_mparams    = glGetUniformLocation(raycast_color_shader_bb.program, "mparams");
	shaders.push_back(&raycast_color_shader_bb);
	generate_raycast_shader_vao(
		raycast_color_shader_bb_vao,
		&raycast_color_shader_bb_vbo0, &raycast_color_shader_bb_vbo1, &raycast_color_shader_bb_vbo2,
		&(raycast_color_shader_bb.position_handle));
	//
	create_program(raycast_vs, raycast_fs, &raycast_shader);
	raycast_shader.location_mvp        = glGetUniformLocation(raycast_shader.program, "mvp");
	raycast_shader.position_handle     = glGetAttribLocation (raycast_shader.program, "v_position");
	raycast_shader.location_sampler[0] = glGetUniformLocation(raycast_shader.program, "sampler0");
	raycast_shader.location_sampler[1] = glGetUniformLocation(raycast_shader.program, "sampler1");
	raycast_shader.location_sampler[2] = glGetUniformLocation(raycast_shader.program, "sampler2");
	raycast_shader.location_mparams    = glGetUniformLocation(raycast_shader.program, "mparams");
	shaders.push_back(&raycast_shader);
	generate_raycast_shader_vao(
		raycast_shader_vao,
		&raycast_shader_vbo0, &raycast_shader_vbo1, &raycast_shader_vbo2,
		&(raycast_shader.position_handle));
	//
	create_program(raycast_vs, raycast_color_fs, &raycast_color_shader);
	raycast_color_shader.location_mvp        = glGetUniformLocation(raycast_color_shader.program, "mvp");
	raycast_color_shader.position_handle     = glGetAttribLocation (raycast_color_shader.program, "v_position");
	raycast_color_shader.location_sampler[0] = glGetUniformLocation(raycast_color_shader.program, "sampler0");
	raycast_color_shader.location_sampler[1] = glGetUniformLocation(raycast_color_shader.program, "sampler1");
	raycast_color_shader.location_sampler[2] = glGetUniformLocation(raycast_color_shader.program, "sampler2");
	raycast_color_shader.location_sampler[3] = glGetUniformLocation(raycast_color_shader.program, "sampler3");
	raycast_color_shader.location_mparams    = glGetUniformLocation(raycast_color_shader.program, "mparams");
	shaders.push_back(&raycast_color_shader);
	generate_raycast_shader_vao(
		raycast_color_shader_vao,
		&raycast_color_shader_vbo0, &raycast_color_shader_vbo1, &raycast_color_shader_vbo2,
		&(raycast_color_shader.position_handle));
	//
	create_program(raycast_vs, raycast_fs_bb_sigm, &raycast_shader_bb_sigm);
	raycast_shader_bb_sigm.location_mvp        = glGetUniformLocation(raycast_shader_bb_sigm.program, "mvp");
	raycast_shader_bb_sigm.position_handle     = glGetAttribLocation (raycast_shader_bb_sigm.program, "v_position");
	raycast_shader_bb_sigm.location_sampler[0] = glGetUniformLocation(raycast_shader_bb_sigm.program, "sampler0");
	raycast_shader_bb_sigm.location_sampler[1] = glGetUniformLocation(raycast_shader_bb_sigm.program, "sampler1");
	raycast_shader_bb_sigm.location_sampler[2] = glGetUniformLocation(raycast_shader_bb_sigm.program, "sampler2");
	raycast_shader_bb_sigm.location_mparams    = glGetUniformLocation(raycast_shader_bb_sigm.program, "mparams");
	shaders.push_back(&raycast_shader_bb_sigm);
	generate_raycast_shader_vao(
		raycast_shader_bb_sigm_vao,
		&raycast_shader_bb_sigm_vbo0, &raycast_shader_bb_sigm_vbo1, &raycast_shader_bb_sigm_vbo2,
		&(raycast_shader_bb_sigm.position_handle));
	//
	create_program(raycast_vs, raycast_color_fs_bb_sigm, &raycast_color_shader_bb_sigm);
	raycast_color_shader_bb_sigm.location_mvp        = glGetUniformLocation(raycast_color_shader_bb_sigm.program, "mvp");
	raycast_color_shader_bb_sigm.position_handle     = glGetAttribLocation (raycast_color_shader_bb_sigm.program, "v_position");
	raycast_color_shader_bb_sigm.location_sampler[0] = glGetUniformLocation(raycast_color_shader_bb_sigm.program, "sampler0");
	raycast_color_shader_bb_sigm.location_sampler[1] = glGetUniformLocation(raycast_color_shader_bb_sigm.program, "sampler1");
	raycast_color_shader_bb_sigm.location_sampler[2] = glGetUniformLocation(raycast_color_shader_bb_sigm.program, "sampler2");
	raycast_color_shader_bb_sigm.location_sampler[3] = glGetUniformLocation(raycast_color_shader_bb_sigm.program, "sampler3");
	raycast_color_shader_bb_sigm.location_mparams    = glGetUniformLocation(raycast_color_shader_bb_sigm.program, "mparams");
	shaders.push_back(&raycast_color_shader_bb_sigm);
	generate_raycast_shader_vao(
		raycast_color_shader_bb_sigm_vao,
		&raycast_color_shader_bb_sigm_vbo0, &raycast_color_shader_bb_sigm_vbo1, &raycast_color_shader_bb_sigm_vbo2,
		&(raycast_color_shader_bb_sigm.position_handle));
	//
	create_program(raycast_vs, raycast_fs_sigm, &raycast_shader_sigm);
	raycast_shader_sigm.location_mvp        = glGetUniformLocation(raycast_shader_sigm.program, "mvp");
	raycast_shader_sigm.position_handle     = glGetAttribLocation (raycast_shader_sigm.program, "v_position");
	raycast_shader_sigm.location_sampler[0] = glGetUniformLocation(raycast_shader_sigm.program, "sampler0");
	raycast_shader_sigm.location_sampler[1] = glGetUniformLocation(raycast_shader_sigm.program, "sampler1");
	raycast_shader_sigm.location_sampler[2] = glGetUniformLocation(raycast_shader_sigm.program, "sampler2");
	raycast_shader_sigm.location_mparams    = glGetUniformLocation(raycast_shader_sigm.program, "mparams");
	shaders.push_back(&raycast_shader_sigm);
	generate_raycast_shader_vao(
		raycast_shader_sigm_vao,
		&raycast_shader_sigm_vbo0, &raycast_shader_sigm_vbo1, &raycast_shader_sigm_vbo2,
		&(raycast_shader_sigm.position_handle));
	//
	create_program(raycast_vs, raycast_color_fs_sigm, &raycast_color_shader_sigm);
	raycast_color_shader_sigm.location_mvp        = glGetUniformLocation(raycast_color_shader_sigm.program, "mvp");
	raycast_color_shader_sigm.position_handle     = glGetAttribLocation (raycast_color_shader_sigm.program, "v_position");
	raycast_color_shader_sigm.location_sampler[0] = glGetUniformLocation(raycast_color_shader_sigm.program, "sampler0");
	raycast_color_shader_sigm.location_sampler[1] = glGetUniformLocation(raycast_color_shader_sigm.program, "sampler1");
	raycast_color_shader_sigm.location_sampler[2] = glGetUniformLocation(raycast_color_shader_sigm.program, "sampler2");
	raycast_color_shader_sigm.location_sampler[3] = glGetUniformLocation(raycast_color_shader_sigm.program, "sampler3");
	raycast_color_shader_sigm.location_mparams    = glGetUniformLocation(raycast_color_shader_sigm.program, "mparams");
	shaders.push_back(&raycast_color_shader_sigm);
	generate_raycast_shader_vao(
		raycast_color_shader_sigm_vao,
		&raycast_color_shader_sigm_vbo0, &raycast_color_shader_sigm_vbo1, &raycast_color_shader_sigm_vbo2,
		&(raycast_color_shader_sigm.position_handle));
	//
	////////////////////////////
	// orient. cube
	ok = create_fbos0(
		256,
		256,
		&cubebuffer,
		&cube_tex,
		&cube_depth);
	if (!ok)
	{
		std::cout << "create_fbos0() failed (cube)"<< std::endl;
	}
	//
	cube = new qMeshData;
	GLuint * vboid000 = new GLuint[4];
	GLuint vaoid000 = 0;
	makeModelVBO_ArraysT(
		vboid000,
		&vaoid000,
		&(orientcube_shader.position_handle),
		&(orientcube_shader.normal_handle),
		NULL,
		NULL,
		p_vertices_cube,
		p_normals_cube,
		NULL,
		NULL,
		p_faces_cube,
		faces_size_cube,
		GL_STATIC_DRAW,
		10.0f);
	cube->faces_size = faces_size_cube/12;
	cube->vboid[0] = vboid000[0];
	cube->vboid[1] = vboid000[1];
	cube->vaoid    = vaoid000;
	cube->shader   = &orientcube_shader;
	qmeshes.push_back(cube);
	//
	letters = new qMeshData;
	GLuint * vboid001 = new GLuint[4];
	GLuint vaoid001 = 0;
	makeModelVBO_ArraysT(
		vboid001,
		&vaoid001,
		&(orientcube_shader.position_handle),
		&(orientcube_shader.normal_handle),
		NULL,
		NULL,
		p_vertices_letters,
		p_normals_letters,
		NULL,
		NULL,
		p_faces_letters,
		faces_size_letters,
		GL_STATIC_DRAW,
		10.0f);
	letters->faces_size = faces_size_letters/12;
	letters->vboid[0] = vboid001[0];
	letters->vboid[1] = vboid001[1];
	letters->vaoid    = vaoid001;
	letters->shader = &orientcube_shader;
	qmeshes.push_back(letters);
	//
	letteri = new qMeshData;
	GLuint * vboid002 = new GLuint[4];
	GLuint vaoid002 = 0;
	makeModelVBO_ArraysT(
		vboid002,
		&vaoid002,
		&(orientcube_shader.position_handle),
		&(orientcube_shader.normal_handle),
		NULL,
		NULL,
		p_vertices_letteri,
		p_normals_letteri,
		NULL,
		NULL,
		p_faces_letteri,
		faces_size_letteri,
		GL_STATIC_DRAW,
		10.0f);
	letteri->faces_size = faces_size_letteri/12;
	letteri->vboid[0] = vboid002[0];
	letteri->vboid[1] = vboid002[1];
	letteri->vaoid = vaoid002;
	letteri->shader = &orientcube_shader;
	qmeshes.push_back(letteri);
	//
	lettera = new qMeshData;
	GLuint * vboid003 = new GLuint[4];
	GLuint vaoid003 = 0;
	makeModelVBO_ArraysT(
		vboid003,
		&vaoid003,
		&(orientcube_shader.position_handle),
		&(orientcube_shader.normal_handle),
		NULL,
		NULL,
		p_vertices_lettera,
		p_normals_lettera,
		NULL,
		NULL,
		p_faces_lettera,
		faces_size_lettera,
		GL_STATIC_DRAW,
		10.0f);
	lettera->faces_size = faces_size_lettera/12;
	lettera->vboid[0] = vboid003[0];
	lettera->vboid[1] = vboid003[1];
	lettera->vaoid = vaoid003;
	lettera->shader = &orientcube_shader;
	qmeshes.push_back(lettera);
	//
	letterp = new qMeshData;
	GLuint * vboid004 = new GLuint[4];
	GLuint vaoid004 = 0;
	makeModelVBO_ArraysT(
		vboid004,
		&vaoid004,
		&(orientcube_shader.position_handle),
		&(orientcube_shader.normal_handle),
		NULL,
		NULL,
		p_vertices_letterp,
		p_normals_letterp,
		NULL,
		NULL,
		p_faces_letterp,
		faces_size_letterp,
		GL_STATIC_DRAW,
		10.0f);
	letterp->faces_size = faces_size_letterp/12;
	letterp->vboid[0] = vboid004[0];
	letterp->vboid[1] = vboid004[1];
	letterp->vaoid    = vaoid004;
	letterp->shader = &orientcube_shader;
	qmeshes.push_back(letterp);
	//
	letterr = new qMeshData;
	GLuint * vboid005 = new GLuint[4];
	GLuint vaoid005 = 0;
	makeModelVBO_ArraysT(
		vboid005,
		&vaoid005,
		&(orientcube_shader.position_handle),
		&(orientcube_shader.normal_handle),
		NULL,
		NULL,
		p_vertices_letterr,
		p_normals_letterr,
		NULL,
		NULL,
		p_faces_letterr,
		faces_size_letterr,
		GL_STATIC_DRAW,
		10.0f);
	letterr->faces_size = faces_size_letterr/12;
	letterr->vboid[0] = vboid005[0];
	letterr->vboid[1] = vboid005[1];
	letterr->vaoid    = vaoid005;
	letterr->shader = &orientcube_shader;
	qmeshes.push_back(letterr);
	//
	letterl = new qMeshData;
	GLuint * vboid006 = new GLuint[4];
	GLuint vaoid006 = 0;
	makeModelVBO_ArraysT(
		vboid006,
		&vaoid006,
		&(orientcube_shader.position_handle),
		&(orientcube_shader.normal_handle),
		NULL,
		NULL,
		p_vertices_letterl,
		p_normals_letterl,
		NULL,
		NULL,
		p_faces_letterl,
		faces_size_letterl,
		GL_STATIC_DRAW,
		10.0f);
	letterl->faces_size = faces_size_letterl/12;
	letterl->vboid[0] = vboid006[0];
	letterl->vboid[1] = vboid006[1];
	letterl->vaoid    = vaoid006;
	letterl->shader = &orientcube_shader;
	qmeshes.push_back(letterl);
	//
	checkGLerror("OpenGL error after init\n");
}

void GLWidget::init_physics(btCollisionWorld * w)
{
	m_collisionWorld = w;
}

QString GLWidget::get_system_info() const
{
	return opengl_info;
}

void GLWidget::draw_3d_tex1(
	GLuint * vao,
	GLuint * vbo,
	const float * v,
	const float * t)
{
	glBindVertexArray(*vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(float), v, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(float), t, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void GLWidget::draw_frame2(const GLfloat * v)
{
	glBindVertexArray(frames_vao);
	glBindBuffer(GL_ARRAY_BUFFER, frames_vbo);
	glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
}

void GLWidget::paint__()
{
	static unsigned long int count_frames = 0;
	if (no_opengl3) return;
#if 1
	glClearColor(clear_color_r, clear_color_g, clear_color_b, 1.0f);
#else
	glClearColor(0.0f, 1.0f, 0.0f, 0.5f);
#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (skip_draw) return;
	if (!selected_images__) return;
	count_frames += 1;
	switch(view)
	{
	case 0:
		paint_volume();
		break;
	case 1:
		paint_raycaster();
		break;
	default:
		break;
	}
#ifdef ALWAYS_SHOW_GL_ERROR
	checkGLerror(" GLWidget::paint__()\n");
#endif
}

void GLWidget::paint_raycaster()
{
	if (!selected_images__) return;
	if (selected_images__->empty()) return;
	if (!selected_images__->at(0)) return;
	const DisplayInterface * di = selected_images__->at(0)->di;
	if (!di->opengl_ok) return;
	if ((!selected_images__->at(0)->equi &&
		!selected_images__->at(0)->di->hide_orientation) &&
		selected_images__->at(0)->di->slices_generated)
	{
		// TODO warning "Non-uniform is not visible"
		return;
	}
	bool warn1 = false;
	bool warn2 = false;
	if (
		(display_contours && !selected_images__->at(0)->di->rois.empty()) ||
		!selected_images__->at(0)->di->spectroscopy_slices.empty())
	{
		warn1 = true;
	}
	if (!selected_images__->at(0)->equi ||
		di->skip_texture ||
		(di->idimz < 2) ||
		!selected_images__->at(0)->di->slices_generated)
	{
		if (warn1)
		{
			// TODO "Some objects are not visible"
		}
		return;
	}
	if (selected_images__->size() > 1)
	{
		warn2 = true;
	}
	//
	const float x__ = di->idimx*di->ix_spacing*0.5f;
	const float y__ = di->idimy*di->iy_spacing*0.5f;
	const float z__ = di->idimz*di->iz_spacing*0.5f;
	const unsigned int orientation = selected_images__->at(0)->orientation;
	const bool force_skip_cube = update_raycast_shader_vbo(
		orientation,
		x__, y__, z__,
		raycastcube0, raycastcube1, raycastcube2,
		true);
	//
	const float asp = (win_h>0) ? (float)win_w/(float)win_h : 1;
	const float fold_win_pos_x =  2.0f * (((float)old_win_pos_x/(float)win_w) - 0.5f);
	const float fold_win_pos_y = -2.0f * (((float)old_win_pos_y/(float)win_h) - 0.5f);
	const float fnew_win_pos_x =  2.0f * (((float)new_win_pos_x/(float)win_w) - 0.5f);
	const float fnew_win_pos_y = -2.0f * (((float)new_win_pos_y/(float)win_h) - 0.5f);
	float dx__ = 0.0f, dy__ = 0.0f;
	//
	// orient. cube
	const bool show_cube_tmp = show_cube && !di->hide_orientation && !force_skip_cube;
	if (show_cube_tmp)
	{
		render_orient_cube1(fold_win_pos_x,fold_win_pos_y,fnew_win_pos_x,fnew_win_pos_y);
	}
	//
	if (ortho_proj)
	{
		camera->orthographic(-ortho_size*asp, ortho_size*asp,
						-ortho_size, ortho_size,
						-far_plane, far_plane);
		const float d_ortho = ortho_size / (float)SCENE_ORTHO_SIZE;
		dx__ = pan_x*d_ortho;
		dy__ = pan_y*d_ortho;
	}
	else
	{
		camera->perspective(CAMERA_D2R*fov,asp,0.01f,(float)far_plane);
		const float d_persp = fabs(position_z) / (float)SCENE_POS_Z;
		dx__ = pan_x*d_persp;
		dy__ = pan_y*d_persp;
	}
	camera->set_trackball_pan_matrix(
		0.8f,
		fold_win_pos_x,fold_win_pos_y,
		fnew_win_pos_x,fnew_win_pos_y,
		(di->center_x - di->default_center_x),
		(di->center_y - di->default_center_y),
		(di->center_z - di->default_center_z),
		0.0f, 0.0f, position_z,
		0.0f, 1.0f, 0.0f,
		dx__, dy__);
	//
	new_win_pos_x = old_win_pos_x; new_win_pos_y = old_win_pos_y;
	//
	const Matrix4 mv_aos  = camera->m_view;
	const Matrix4 mvp_aos = camera->m_projection * mv_aos;
	ALIGN16(float mvp_aos_ptr[16]);
	camera->matrix4_to_float(mvp_aos,mvp_aos_ptr);
	ALIGN16(float mv_aos_ptr[16]);
	camera->matrix4_to_float(mv_aos,mv_aos_ptr);
	//
	// [0]
	mparams[0]  = (float)di->dimx; // X dim
	mparams[1]  = (float)(di->window_center - di->window_width*0.5);
	mparams[2]  = (float)(di->window_center + di->window_width*0.5);
	mparams[3]  = (float)di->window_width;
	// [1]
	mparams[4]  = (float)di->idimz; // Z dim
	mparams[5]  = (float)di->from_slice/(float)di->idimz; // z
	mparams[6]  = (float)di->to_slice/(float)di->idimz; // z
	mparams[7]  = (float)di->bb_y_min; // y
	// [2]
	mparams[8]  = (float)di->bb_y_max; // y
	mparams[9]  = (float)di->bb_x_min; // x
	mparams[10] = (float)di->bb_x_max; // x
	mparams[11] = (float)di->dimy; // Y dim
	// [3]
	mparams[12] = alpha; // premultiply alpha
	mparams[13] = brightness; // multiply color
	mparams[14] = (float)di->window_center;
	mparams[15] = 0.0f; // unused
	//
	glEnable(GL_CULL_FACE);
	//
	glUseProgram(zero_shader.program);
	//
	// backfacebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, backfacebuffer);
		glViewport(0, 0, FBO_SIZE__1, FBO_SIZE__1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backface_tex, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, backface_depth);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		glUniformMatrix4fv(zero_shader.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
		glBindVertexArray(raycastcube0_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);
		glBindVertexArray(raycastcube1_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(raycastcube2_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	// frontfacebuffer
	{
		glBindFramebuffer(GL_FRAMEBUFFER, frontfacebuffer);
		glViewport(0, 0, FBO_SIZE__1, FBO_SIZE__1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frontface_tex, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frontface_depth);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);
		glUniformMatrix4fv(zero_shader.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
		glBindVertexArray(raycastcube0_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);
		glBindVertexArray(raycastcube1_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(raycastcube2_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	glDisable(GL_CULL_FACE);
	//
	if (di->lut_function == 2)
	{
		if (di->selected_lut==0)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, FBO_SIZE__0, FBO_SIZE__0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, fbo_depth, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (rect_selection)
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_shader_bb_sigm_vbo0, &raycast_shader_bb_sigm_vbo1, &raycast_shader_bb_sigm_vbo2,
					false);

				glUseProgram(raycast_shader_bb_sigm.program);

				glUniform4fv(raycast_shader_bb_sigm.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_shader_bb_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_shader_bb_sigm.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_shader_bb_sigm.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_shader_bb_sigm.location_sampler[2], 0);

				glBindVertexArray(raycast_shader_bb_sigm_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_shader_bb_sigm_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_shader_bb_sigm_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			}
			else
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_shader_sigm_vbo0, &raycast_shader_sigm_vbo1, &raycast_shader_sigm_vbo2,
					false);

				glUseProgram(raycast_shader_sigm.program);

				glUniform4fv(raycast_shader_sigm.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_shader_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_shader_sigm.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_shader_sigm.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_shader_sigm.location_sampler[2], 0);

				glBindVertexArray(raycast_shader_sigm_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_shader_sigm_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_shader_sigm_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, FBO_SIZE__0, FBO_SIZE__0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, fbo_depth, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (rect_selection)
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_color_shader_bb_sigm_vbo0, &raycast_color_shader_bb_sigm_vbo1, &raycast_color_shader_bb_sigm_vbo2,
					false);

				glUseProgram(raycast_color_shader_bb_sigm.program);

				glUniform4fv(raycast_color_shader_bb_sigm.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_color_shader_bb_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_color_shader_bb_sigm.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_color_shader_bb_sigm.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_color_shader_bb_sigm.location_sampler[2], 0);

				glActiveTexture(GL_TEXTURE3);
				switch(di->selected_lut)
				{
				case  1: glBindTexture(GL_TEXTURE_1D, gradient1);  break;
				case  2: glBindTexture(GL_TEXTURE_1D, gradient2);  break;
				case  3: glBindTexture(GL_TEXTURE_1D, gradient3);  break;
				case  4: glBindTexture(GL_TEXTURE_1D, gradient4);  break;
				case  5: glBindTexture(GL_TEXTURE_1D, gradient5);  break;
				case  6: glBindTexture(GL_TEXTURE_1D, gradient6);  break;
				case  7: glBindTexture(GL_TEXTURE_1D, gradient7);  break;
				default: break;
				}
				glUniform1i(raycast_color_shader_bb_sigm.location_sampler[3], 3);

				glBindVertexArray(raycast_color_shader_bb_sigm_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_color_shader_bb_sigm_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_color_shader_bb_sigm_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
			else
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_color_shader_sigm_vbo0, &raycast_color_shader_sigm_vbo1, &raycast_color_shader_sigm_vbo2,
					false);

				glUseProgram(raycast_color_shader_sigm.program);

				glUniform4fv(raycast_color_shader_sigm.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_color_shader_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_color_shader_sigm.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_color_shader_sigm.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_color_shader_sigm.location_sampler[2], 0);

				glActiveTexture(GL_TEXTURE3);
				switch(di->selected_lut)
				{
				case  1: glBindTexture(GL_TEXTURE_1D, gradient1);  break;
				case  2: glBindTexture(GL_TEXTURE_1D, gradient2);  break;
				case  3: glBindTexture(GL_TEXTURE_1D, gradient3);  break;
				case  4: glBindTexture(GL_TEXTURE_1D, gradient4);  break;
				case  5: glBindTexture(GL_TEXTURE_1D, gradient5);  break;
				case  6: glBindTexture(GL_TEXTURE_1D, gradient6);  break;
				case  7: glBindTexture(GL_TEXTURE_1D, gradient7);  break;
				default: break;
				}
				glUniform1i(raycast_color_shader_sigm.location_sampler[3], 3);

				glBindVertexArray(raycast_color_shader_sigm_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_color_shader_sigm_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_color_shader_sigm_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
		}
	}
	else
	{
		if (di->selected_lut==0)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, FBO_SIZE__0, FBO_SIZE__0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, fbo_depth, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (rect_selection)
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_shader_bb_vbo0, &raycast_shader_bb_vbo1, &raycast_shader_bb_vbo2,
					false);

				glUseProgram(raycast_shader_bb.program);

				glUniform4fv(raycast_shader_bb.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_shader_bb.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_shader_bb.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_shader_bb.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_shader_bb.location_sampler[2], 0);

				glBindVertexArray(raycast_shader_bb_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_shader_bb_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_shader_bb_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
			else
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_shader_vbo0, &raycast_shader_vbo1, &raycast_shader_vbo2,
					false);

				glUseProgram(raycast_shader.program);

				glUniform4fv(raycast_shader.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_shader.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_shader.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_shader.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_shader.location_sampler[2], 0);

				glBindVertexArray(raycast_shader_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_shader_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_shader_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
			glViewport(0, 0, FBO_SIZE__0, FBO_SIZE__0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, fbo_depth, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (rect_selection)
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_color_shader_bb_vbo0, &raycast_color_shader_bb_vbo1, &raycast_color_shader_bb_vbo2,
					false);

				glUseProgram(raycast_color_shader_bb.program);

				glUniform4fv(raycast_color_shader_bb.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_color_shader_bb.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_color_shader_bb.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_color_shader_bb.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_color_shader_bb.location_sampler[2], 0);

				glActiveTexture(GL_TEXTURE3);
				switch(di->selected_lut)
				{
				case  1: glBindTexture(GL_TEXTURE_1D, gradient1);  break;
				case  2: glBindTexture(GL_TEXTURE_1D, gradient2);  break;
				case  3: glBindTexture(GL_TEXTURE_1D, gradient3);  break;
				case  4: glBindTexture(GL_TEXTURE_1D, gradient4);  break;
				case  5: glBindTexture(GL_TEXTURE_1D, gradient5);  break;
				case  6: glBindTexture(GL_TEXTURE_1D, gradient6);  break;
				case  7: glBindTexture(GL_TEXTURE_1D, gradient7);  break;
				default: break;
				}
				glUniform1i(raycast_color_shader_bb.location_sampler[3], 3);

				glBindVertexArray(raycast_color_shader_bb_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_color_shader_bb_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_color_shader_bb_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
			else
			{
				update_raycast_shader_vbo(
					orientation,
					x__, y__, z__,
					&raycast_color_shader_vbo0, &raycast_color_shader_vbo1, &raycast_color_shader_vbo2,
					false);

				glUseProgram(raycast_color_shader.program);

				glUniform4fv(raycast_color_shader.location_mparams, 16/4, mparams);
				glUniformMatrix4fv(raycast_color_shader.location_mvp, 1, GL_FALSE, mvp_aos_ptr);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, backface_tex);
				glUniform1i(raycast_color_shader.location_sampler[0], 2);

				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, frontface_tex);
				glUniform1i(raycast_color_shader.location_sampler[1], 5);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
				glUniform1i(raycast_color_shader.location_sampler[2], 0);

				glActiveTexture(GL_TEXTURE3);
				switch(di->selected_lut)
				{
				case  1: glBindTexture(GL_TEXTURE_1D, gradient1);  break;
				case  2: glBindTexture(GL_TEXTURE_1D, gradient2);  break;
				case  3: glBindTexture(GL_TEXTURE_1D, gradient3);  break;
				case  4: glBindTexture(GL_TEXTURE_1D, gradient4);  break;
				case  5: glBindTexture(GL_TEXTURE_1D, gradient5);  break;
				case  6: glBindTexture(GL_TEXTURE_1D, gradient6);  break;
				case  7: glBindTexture(GL_TEXTURE_1D, gradient7);  break;
				default: break;
				}
				glUniform1i(raycast_color_shader.location_sampler[3], 3);

				glBindVertexArray(raycast_color_shader_vao[0]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

				glBindVertexArray(raycast_color_shader_vao[1]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				glBindVertexArray(raycast_color_shader_vao[2]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
		}
	}
//////////////// final scene quad
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
#else
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
		glViewport(0, 0, win_w, win_h);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(fsquad_shader.program);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, fbo_tex);
		glUniform1i(fsquad_shader.location_sampler[0], 2);
		glBindVertexArray(scene_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
////////////////
	if (show_cube_tmp)
	{
		render_orient_cube2();
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
	}
////////////////
	if (warn1)
	{
		// TODO warning "Some objects are not visible"
	}
	if (warn2)
	{
		// TODO warning "Selected image only"
	}
}

void GLWidget::paint_volume()
{
	const int selected_images_size = selected_images__->size();
	if (selected_images_size <1) return;
	//
	const float fold_win_pos_x =  2.0f * (((float)old_win_pos_x/(float)win_w) - 0.5f);
	const float fold_win_pos_y = -2.0f * (((float)old_win_pos_y/(float)win_h) - 0.5f);
	const float fnew_win_pos_x =  2.0f * (((float)new_win_pos_x/(float)win_w) - 0.5f);
	const float fnew_win_pos_y = -2.0f * (((float)new_win_pos_y/(float)win_h) - 0.5f);
	unsigned int count = 0;
	unsigned int count_images = 0;
	const bool wireframe_ = wireframe;
	//
	if (show_cube)
	{
		render_orient_cube1(fold_win_pos_x,fold_win_pos_y,fnew_win_pos_x,fnew_win_pos_y);
	}
	//
	const float asp = (win_h>0) ? (float)win_w/(float)win_h : 1;
	float dx__ = 0.0f, dy__ = 0.0f;
	glViewport(0,0,win_w,win_h);
	if (ortho_proj)
	{
		camera->orthographic(-ortho_size*asp, ortho_size*asp,
						-ortho_size, ortho_size,
						-far_plane, far_plane);
		const float d_ortho = ortho_size / (float)SCENE_ORTHO_SIZE;
		dx__ = pan_x*d_ortho;
		dy__ = pan_y*d_ortho;
	}
	else
	{
		camera->perspective(CAMERA_D2R*fov,asp,0.01f,(float)far_plane);
		const float d_persp = fabs(position_z) / (float)SCENE_POS_Z;
		dx__ = pan_x*d_persp;
		dy__ = pan_y*d_persp;
	}
	float light_x, light_y, light_z;
	camera->set_trackball_pan_matrix2(
		0.8f,
		fold_win_pos_x,fold_win_pos_y,
		fnew_win_pos_x,fnew_win_pos_y,
		selected_images__->at(0)->di->center_x,
		selected_images__->at(0)->di->center_y,
		selected_images__->at(0)->di->center_z,
		0.0f,0.0f,position_z,
		0.0f,1.0f,0.0f,
		dx__, dy__,
		&light_x, &light_y, &light_z);
	//
	new_win_pos_x = old_win_pos_x; new_win_pos_y = old_win_pos_y;
	//
	const Matrix4 mv_aos  = camera->m_view;
	const Matrix4 mvp_aos = camera->m_projection * mv_aos;
	ALIGN16(float mvp_aos_ptr[16]);
	camera->matrix4_to_float(mvp_aos,mvp_aos_ptr);
	//
	for (int iii = 0; iii < selected_images_size; iii++)
	{
		if (!selected_images__->at(iii)) continue;
		if (!selected_images__->at(iii)->di->spectroscopy_slices.empty())
		{
			const DisplayInterface * di = selected_images__->at(iii)->di;
			glUseProgram(frame_shader.program);
			glUniformMatrix4fv(frame_shader.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
			glPointSize(2.0f);
			for (unsigned long x = 0; x < di->spectroscopy_slices.size(); x++)
			{
				glUniform4f(
					frame_shader.location_K,
					color_spectro0[0],
					color_spectro0[1],
					color_spectro0[2],
					color_spectro0[3]);
				if (di->spectroscopy_slices.at(x)->lsize > 0)
				{
					glBindVertexArray(di->spectroscopy_slices.at(x)->lvaoid);
					glDrawArrays(GL_LINES, 0, di->spectroscopy_slices.at(x)->lsize);
				}
				if (di->spectroscopy_ref == 1)
					glUniform4f(
						frame_shader.location_K,
						color_spectro1[0],
						color_spectro1[1],
						color_spectro1[2],
						color_spectro1[3]);
				if (di->spectroscopy_slices.at(x)->psize > 0)
				{
					glBindVertexArray(di->spectroscopy_slices.at(x)->pvaoid);
					glDrawArrays(GL_POINTS, 0, di->spectroscopy_slices.at(x)->psize);
				}
				if (di->spectroscopy_slices.at(x)->fvboid > 0)
				{
					glBindVertexArray(di->spectroscopy_slices.at(x)->fvaoid);
					glDrawArrays(GL_LINE_LOOP, 0, 4);
				}
			}
			count += 1;
		}
		//
		if (!selected_images__->at(iii)->di->trimeshes.empty())
		{
			if (wireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			// eye position and light direction
			sparams[0] = light_x;
			sparams[1] = light_y;
			sparams[2] = light_z;
			sparams[3] = sparams[0];
			sparams[4] = sparams[1];
			sparams[5] = sparams[2];
			glUseProgram(mesh_shader.program); // all meshes currently using 'mesh_shader'
			TriMeshes::const_iterator mi = selected_images__->at(iii)->di->trimeshes.begin();
			while (mi != selected_images__->at(iii)->di->trimeshes.end())
			{
				TriMesh * tm = mi.value();
				if (tm && tm->initialized && tm->visible && tm->qmesh)
				{
					d_mesh(tm->qmesh,
						NULL,
						NULL,
						mvp_aos_ptr,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						NULL,
						sparams,
						NULL);
				}
				++mi;
			}
			if (wireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		//
		if (display_contours && !selected_images__->at(iii)->di->rois.empty())
		{
			glPointSize(1.5f);
			glUseProgram(frame_shader.program);
			glUniformMatrix4fv(frame_shader.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
			for (int i = 0; i < selected_images__->at(iii)->di->rois.size(); i++)
			{
				if (selected_images__->at(iii)->di->rois.at(i).show)
				{
					const bool random_color = selected_images__->at(iii)->di->rois.at(i).random_color;
					if (!random_color)
					{
						const float color[] =
						{
							selected_images__->at(iii)->di->rois.at(i).color.r,
							selected_images__->at(iii)->di->rois.at(i).color.g,
							selected_images__->at(iii)->di->rois.at(i).color.b,
							1.0f
						};
						glUniform4fv(frame_shader.location_K, 1, color);
					}
					Contours::const_iterator it =
						selected_images__->at(iii)->di->rois.at(i).contours.constBegin();
					while (it != selected_images__->at(iii)->di->rois.at(i).contours.constEnd())
					{
						const Contour * c = it.value();
						if (c && c->vao_initialized)
						{
							if (random_color)
							{
								const float rcolor[] = { c->color.r, c->color.g, c->color.b, 1.0f };
								glUniform4fv(frame_shader.location_K, 1, rcolor);
							}
							glBindVertexArray(c->vaoid);
							switch (c->type)
							{
							case 1:  glDrawArrays(GL_LINE_LOOP,  0, c->dpoints.size()); break;
							case 2:  glDrawArrays(GL_LINE_STRIP, 0, c->dpoints.size()); break;
							case 3:  glDrawArrays(GL_LINE_STRIP, 0, c->dpoints.size()); break;
							default: glDrawArrays(GL_POINTS,     0, c->dpoints.size()); break;
							}
						}
						++it;
					}
				}
			}
			count += 1;
		}
	}
	//
	for (int iii = 0; iii < selected_images_size; iii++)
	{
		if (!selected_images__->at(iii)) continue;
		if (selected_images__->at(iii)->image_type >= 200) continue;
		//
		const DisplayInterface * di = selected_images__->at(iii)->di;
		if (di->skip_texture) continue;
		if ((di->from_slice < 0)||(di->from_slice >= di->idimz)) continue;
		if ((di->to_slice < 0)  ||(di->to_slice >=di->idimz))    continue;
		if ((int)di->image_slices.size() < di->idimz) continue;
		//
		const Vector3 direction_vector = Vector3(
			di->slices_direction_x,
			di->slices_direction_y,
			di->slices_direction_z);
		const float dotv = dot(direction_vector,camera->m_direction_vector);
		//
		// [0]
		mparams[0]  = 0.0f; // unused
		mparams[1]  = (float)(di->window_center - di->window_width*0.5);
		mparams[2]  = (float)(di->window_center + di->window_width*0.5);
		mparams[3]  = (float)di->window_width;
		// [1]
		mparams[4]  = 0.0f; // unused 
		mparams[5]  = 0.0f; // unused
		mparams[6]  = 0.0f; // unused
		mparams[7]  = (float)di->bb_y_min; // y
		// [2]
		mparams[8]  = (float)di->bb_y_max; // y
		mparams[9]  = (float)di->bb_x_min; // x
		mparams[10] = (float)di->bb_x_max; // x
		mparams[11] = 0.0f; // unused
		// [3]
		mparams[12] = alpha; // premultiply alpha
		mparams[13] = brightness; // multiply color
		mparams[14] = (float)di->window_center;
		mparams[15] = 0.0f; // unused
		//
		if (draw_frames_3d)
		{	
			glUseProgram(frame_shader.program);
			glUniformMatrix4fv(
				frame_shader.location_mvp,
				1, GL_FALSE, mvp_aos_ptr);
			glUniform4f(
				frame_shader.location_K,
				di->R,
				di->G,
				di->B,
				1.0f);
			if (dotv<0)
			{
				for (int x = di->from_slice; x <= di->to_slice; x++)
					draw_frame2(di->image_slices.at(x)->fv);
			}
			else
			{
				for (int x = di->to_slice; x >= di->from_slice; x--)
					draw_frame2(di->image_slices.at(x)->fv);
			}
			if (di->origin_ok)
			{
				glPointSize(3.0f);
				if (iii == 0)
					glUniform4f(frame_shader.location_K, 0.85f, 0.85f, 0.85f, 1.0f);
				glBindVertexArray(origin_vao);
				glBindBuffer(GL_ARRAY_BUFFER, origin_vbo);
				glBufferData(GL_ARRAY_BUFFER, 3*sizeof(GLfloat), di->origin, GL_DYNAMIC_DRAW);
				glDrawArrays(GL_POINTS, 0, 1);
			}
		}
		//
#if 0
		if (count_images > 10) break;
#endif
		//
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, di->cube_3dtex);
		if (di->selected_lut==0)
		{
			if (rect_selection)
			{
				if (di->lut_function == 2)
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_bb_clamp_sigm.program);
						glUniformMatrix4fv(c3d_shader_bb_clamp_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_bb_clamp_sigm.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_bb_clamp_sigm.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_bb_clamp_sigm_vao,
									c3d_shader_bb_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_bb_clamp_sigm_vao,
									c3d_shader_bb_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader_bb_sigm.program);
						glUniformMatrix4fv(c3d_shader_bb_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_bb_sigm.location_mparams, 16/4, mparams);
						glUniform1i(c3d_shader_bb_sigm.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_bb_sigm_vao,
									c3d_shader_bb_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_bb_sigm_vao,
									c3d_shader_bb_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
				else
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_bb_clamp.program);
						glUniformMatrix4fv(c3d_shader_bb_clamp.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_bb_clamp.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_bb_clamp.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_bb_clamp_vao,
									c3d_shader_bb_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_bb_clamp_vao,
									c3d_shader_bb_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader_bb.program);
						glUniformMatrix4fv(c3d_shader_bb.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_bb.location_mparams, 16/4, mparams);
						glUniform1i(c3d_shader_bb.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_bb_vao,
									c3d_shader_bb_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_bb_vao,
									c3d_shader_bb_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
			}
			else
			{
				if (di->lut_function == 2)
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_clamp_sigm.program);
						glUniformMatrix4fv(c3d_shader_clamp_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_clamp_sigm.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_clamp_sigm.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_clamp_sigm_vao,
									c3d_shader_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_clamp_sigm_vao,
									c3d_shader_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader_sigm.program);
						glUniformMatrix4fv(c3d_shader_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_sigm.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_sigm.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_sigm_vao,
									c3d_shader_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_sigm_vao,
									c3d_shader_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
				else
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_clamp.program);
						glUniformMatrix4fv(c3d_shader_clamp.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_clamp.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_clamp.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_clamp_vao,
									c3d_shader_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_clamp_vao,
									c3d_shader_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader.program);
						glUniformMatrix4fv(c3d_shader.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader.location_sampler[0], 0);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_vao,
									c3d_shader_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_vao,
									c3d_shader_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
			}
		}
		else
		{
			glActiveTexture(GL_TEXTURE3);
			switch(di->selected_lut)
			{
			case  1: glBindTexture(GL_TEXTURE_1D, gradient1);  break;
			case  2: glBindTexture(GL_TEXTURE_1D, gradient2);  break;
			case  3: glBindTexture(GL_TEXTURE_1D, gradient3);  break;
			case  4: glBindTexture(GL_TEXTURE_1D, gradient4);  break;
			case  5: glBindTexture(GL_TEXTURE_1D, gradient5);  break;
			case  6: glBindTexture(GL_TEXTURE_1D, gradient6);  break;
			case  7: glBindTexture(GL_TEXTURE_1D, gradient7);  break;
			default: break;
			}
			if (rect_selection)
			{
				if (di->lut_function == 2)
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_gradient_bb_clamp_sigm.program);
						glUniformMatrix4fv(c3d_shader_gradient_bb_clamp_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient_bb_clamp_sigm.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient_bb_clamp_sigm.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient_bb_clamp_sigm.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_clamp_sigm_vao,
									c3d_shader_gradient_bb_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_clamp_sigm_vao,
									c3d_shader_gradient_bb_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader_gradient_bb_sigm.program);
						glUniformMatrix4fv(c3d_shader_gradient_bb_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient_bb_sigm.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient_bb_sigm.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient_bb_sigm.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_sigm_vao,
									c3d_shader_gradient_bb_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_sigm_vao,
									c3d_shader_gradient_bb_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
				else
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_gradient_bb_clamp.program);
						glUniformMatrix4fv(c3d_shader_gradient_bb_clamp.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient_bb_clamp.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient_bb_clamp.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient_bb_clamp.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_clamp_vao,
									c3d_shader_gradient_bb_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_clamp_vao,
									c3d_shader_gradient_bb_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader_gradient_bb.program);
						glUniformMatrix4fv(c3d_shader_gradient_bb.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient_bb.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient_bb.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient_bb.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_vao,
									c3d_shader_gradient_bb_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_bb_vao,
									c3d_shader_gradient_bb_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
			}
			else
			{
				if (di->lut_function == 2)
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_gradient_clamp_sigm.program);
						glUniformMatrix4fv(c3d_shader_gradient_clamp_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient_clamp_sigm.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient_clamp_sigm.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient_clamp_sigm.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_clamp_sigm_vao,
									c3d_shader_gradient_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_clamp_sigm_vao,
									c3d_shader_gradient_clamp_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader_gradient_sigm.program);
						glUniformMatrix4fv(c3d_shader_gradient_sigm.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient_sigm.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient_sigm.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient_sigm.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_sigm_vao,
									c3d_shader_gradient_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_sigm_vao,
									c3d_shader_gradient_sigm_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
				else
				{
					if (!di->transparency)
					{
						glUseProgram(c3d_shader_gradient_clamp.program);
						glUniformMatrix4fv(c3d_shader_gradient_clamp.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient_clamp.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient_clamp.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient_clamp.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_clamp_vao,
									c3d_shader_gradient_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_clamp_vao,
									c3d_shader_gradient_clamp_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
					else
					{
						glUseProgram(c3d_shader_gradient.program);
						glUniformMatrix4fv(c3d_shader_gradient.location_mvp, 1, GL_FALSE, mvp_aos_ptr);
						glUniform4fv(c3d_shader_gradient.location_mparams, 16/4, mparams);	
						glUniform1i(c3d_shader_gradient.location_sampler[0], 0);
						glUniform1i(c3d_shader_gradient.location_sampler[1], 3);
						if (dotv<0)
						{
							for (int x = di->from_slice; x <= di->to_slice; x++)
								draw_3d_tex1(
									&c3d_shader_gradient_vao,
									c3d_shader_gradient_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
						else
						{
							for (int x = di->to_slice; x >= di->from_slice; x--)
								draw_3d_tex1(
									&c3d_shader_gradient_vao,
									c3d_shader_gradient_vbo,
									di->image_slices.at(x)->v,
									di->image_slices.at(x)->tc);
						}
					}
				}
			}
		}
		//
		if (!di->hide_orientation)
		{
			count += 1;
			count_images += 1;
		}
	}
	if ((count > 0) && show_cube)
	{
		render_orient_cube2();
	}
}

void GLWidget::resize__(int w, int h)
{
	win_w = w;
	win_h = h;
}

void GLWidget::gen_lut_tex(const unsigned char * lut, const int size, GLuint * tex)
{
	if (!lut||no_opengl3==true) return;
	glActiveTexture(GL_TEXTURE3);
	if (*tex > 0)
	{
		glDeleteTextures(1, tex);
		*tex = 0;
	}
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_1D, *tex);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//
	// GL_UNPACK_ALIGNMENT/GL_PACK_ALIGNMENT
	// 1 byte-alignment
	// 2 rows aligned to even-numbered bytes
	// 4 word-alignment
	// 8 rows start on double-word boundaries
	//               
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB8, size, 0, GL_RGB, GL_UNSIGNED_BYTE, lut);
	//
	glBindTexture(GL_TEXTURE_1D, 0);
	const int glerror = glGetError();
	if (glerror!=0)
	{
		std::cout
			<< "gen_lut_tex() : OpenGL Error "
			<< glerror
			<< std::endl;
	}
}

btVector3 GLWidget::get_ray_from(float x, float y)
{
	//std::cout << "get_ray_from(" << x <<  "," << y << ")" << std::endl;
	btVector3 ray_from;
	if (ortho_proj)
	{
		Point3 p         = Point3(x,(float)win_h-y,0.0f);
		Vector4 viewport = Vector4(0.0f,0.0f,(float)win_w,(float)win_h);
		Vector4 v        = camera->unproject(p,camera->m_view,camera->m_projection,viewport);
		ray_from         = btVector3(v.getX(),v.getY(),v.getZ());
	}
	else
	{
		ray_from = btVector3(camera->m_position.getX(),camera->m_position.getY(),camera->m_position.getZ());
	}
	return ray_from;
}

btVector3 GLWidget::get_ray_to(float x, float y)
{
	//std::cout << "get_ray_to(" << x << "," << y << ")" << std::endl;
	btVector3 ray_to;
	if (ortho_proj)
	{
		Point3 p         = Point3(x,(float)win_h-y,1.0f);
		Vector4 viewport = Vector4(0.0f,0.0f,(float)win_w,(float)win_h);
		Vector4 v        = camera->unproject(p,camera->m_view,camera->m_projection,viewport);
		ray_to           = btVector3(v.getX(),v.getY(),v.getZ());
	}
	else
	{
		Vector3 from       = Vector3(camera->m_position);
		Vector3 forward    = camera->m_direction_vector;
		Vector3 vertical   = camera->m_up_axis;
		Vector3 horizontal = normalize(cross(forward,vertical));
		forward *= far_plane;
		const double tanfov = tanf(0.5*CAMERA_D2R*fov);
		horizontal *= 2.0*far_plane*tanfov;
		vertical   *= 2.0*far_plane*tanfov;
		horizontal *= (float)win_w/(float)win_h;
		Vector3 dHor  = horizontal * 1.0f/(float)win_w;
		Vector3 dVert = vertical   * 1.0f/(float)win_h;
		Vector3 ray_to_center = from + forward;
		Vector3 res = ray_to_center - 0.5f*horizontal + 0.5f*vertical;
		res += x*dHor;
		res -= y*dVert;
		ray_to = btVector3(res.getX(),res.getY(),res.getZ());
	}
	return ray_to;
}

void GLWidget::send_ray0(float x, float y)
{
	if (!m_collisionWorld) return;
	//std::cout << "send_ray0(" << x << "," << y << ")" << std::endl;
	btVector3 from = get_ray_from(x,y);
	btVector3 to   = get_ray_to(x,y);
	MyClosestRayResultCallback0 rayResult(from,to);
	m_collisionWorld->rayTest(from, to, rayResult);
	if (rayResult.hasHit())
	{
		//std::cout << "   rayResult.hasHit()" << std::endl;
	}
}

void GLWidget::set_left()
{
	m_left = 1;
}

void GLWidget::set_right()
{
	m_right = 1;
}

void GLWidget::set_forward()
{
	m_forw = 1;
}

void GLWidget::set_backward()
{
	m_back = 1;
}

void GLWidget::set_win_old_position(int x, int y)
{
	old_win_pos_x = x;
	old_win_pos_y = y;
}

void GLWidget::set_win_new_position(int x, int y)
{
	new_win_pos_x = x;
	new_win_pos_y = y;
}

void GLWidget::set_pan_delta(int dx, int dy)
{
	pan_x += dx;
	pan_y += dy;
}

void GLWidget::set_selected_images_ptr(QList<ImageVariant*> * i)
{
	selected_images__ = i;
}

void GLWidget::set_view_3d()
{
	view = 0;
}

void GLWidget::set_view_rc()
{
	view = 1;
}

void GLWidget::set_clear_color(float red, float green, float blue)
{
	clear_color_r = red;
	clear_color_g = green;
	clear_color_b = blue;
}

void GLWidget::set_show_cube(bool t)
{
	show_cube = t;
}

void GLWidget::render_orient_cube1(
	float fold_win_pos_x,
	float fold_win_pos_y,
	float fnew_win_pos_x,
	float fnew_win_pos_y)
{
	glViewport(0, 0, 256, 256);
	camera->orthographic(-20.0f, 20.0f,-20.0f, 20.0f,-50.0f, 50.0f);
	camera->set_trackball_matrix(
		0.8f,
		fold_win_pos_x,fold_win_pos_y,
		fnew_win_pos_x,fnew_win_pos_y,
		0.0f,0.0f,0.0f,
		0.0f,0.0f,30.0f,
		0.0f,1.0f,0.0f);
	const Matrix4 mv_cube_aos  = camera->m_view;
	const Matrix4 mvp_cube_aos = camera->m_projection * mv_cube_aos;
	ALIGN16(float mvp_cube_aos_ptr[16]);
	camera->matrix4_to_float(mvp_cube_aos,mvp_cube_aos_ptr);
	// modeling is identity
	// eye position and light direction
	sparams[0] =  camera->m_position.getX();
	sparams[1] =  camera->m_position.getY();
	sparams[2] =  camera->m_position.getZ();
	sparams[3] = sparams[0];
	sparams[4] = sparams[1];
	sparams[5] = sparams[2];
	//
	glBindFramebuffer(GL_FRAMEBUFFER, cubebuffer);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		cube_tex,
		0);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, cube_depth,
		0);
#if 1
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
#else
	glClearColor(0.0f, 0.0f, 1.0f, 0.5f);
#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	//
	glUseProgram(orientcube_shader.program);
	glUniform3fv(orientcube_shader.location_K, 2, color_cube);
	d_orientcube(cube,
				 NULL,
				 NULL,
				 mvp_cube_aos_ptr,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 sparams,
				 NULL);
	glUniform3fv(orientcube_shader.location_K, 2, color_letters);
	d_orientcube(letters,
				 NULL,
				 NULL,
				 mvp_cube_aos_ptr,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 sparams,
				 NULL);
	d_orientcube(letteri,
				 NULL,
				 NULL,
				 mvp_cube_aos_ptr,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 sparams,
				 NULL);
	d_orientcube(lettera,
				 NULL,
				 NULL,
				 mvp_cube_aos_ptr,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 sparams,
				 NULL);
	d_orientcube(letterp,
				 NULL,
				 NULL,
				 mvp_cube_aos_ptr,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 sparams,
				 NULL);
	d_orientcube(letterr,
				 NULL,
				 NULL,
				 mvp_cube_aos_ptr,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 sparams,
				 NULL);
	d_orientcube(letterl,
				 NULL,
				 NULL,
				 mvp_cube_aos_ptr,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 sparams,
				 NULL);
	//
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
#if 1
	glClearColor(clear_color_r, clear_color_g, clear_color_b, 1.0f);
#else
	glClearColor(0.7f, 0.6f, 0.3f, 0.5f);
#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
}

void GLWidget::render_orient_cube2() 
{
	const GLsizei cube_viewport_size =
		win_w < win_h ? win_w/9 : win_h/9; 
	glViewport(0,0,cube_viewport_size,cube_viewport_size);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(fsquad_shader.program);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, cube_tex);
	glUniform1i(fsquad_shader.location_sampler[0], 4);
	glBindVertexArray(scene_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_DEPTH_TEST);
}

void GLWidget::update_screen_size(double delta)
{
	if (delta > 0)
	{
		ortho_size = 0.5*(delta + delta*0.1);
		position_z = 1.5*delta;
	}
	else
	{
		ortho_size = (float)SCENE_ORTHO_SIZE;
		position_z = (float)SCENE_POS_Z;
	}
}

void GLWidget::update_far_plane(float x)
{
	if (x > 0)
	{
		far_plane = x;
	}
	else
	{
		far_plane = (float)SCENE_FAR_PLANE;
	}
}

void GLWidget::fit_to_screen(const ImageVariant * ivariant)
{
	if (!ivariant)
	{
		update_screen_size(-1);
		return;
	}
	if (ivariant->image_type==200)
	{
		double max_delta = 0;
		TriMeshes::const_iterator mi = ivariant->di->trimeshes.begin();
		while (mi != ivariant->di->trimeshes.end())
		{
			if (mi.value() && mi.value()->max_delta > max_delta)
				max_delta = mi.value()->max_delta;
			++mi;
		}
		update_screen_size(max_delta);
	}
	else if (ivariant->image_type==100)
	{
		double max_delta = 0;
		for (int x = 0; x < ivariant->di->rois.size(); x++)
		{
			if (ivariant->di->rois.at(x).max_delta > max_delta)
				max_delta = ivariant->di->rois.at(x).max_delta;
		}
		update_screen_size(max_delta);
	}
	else if (ivariant->image_type >= 0 && ivariant->image_type <= 30)
	{
		if (ivariant->di->skip_texture)
		{
			update_screen_size(-1);
			return;
		}
		const double delta_x = ivariant->di->idimx * ivariant->di->ix_spacing;
		const double delta_y = ivariant->di->idimy * ivariant->di->iy_spacing;
		const double delta_z = ivariant->di->idimz * ivariant->di->iz_spacing;
		const double tmp0 = (delta_x > delta_y) ? delta_x : delta_y;
		const double max_delta = (tmp0 > delta_z) ? tmp0 : delta_z;
		update_screen_size(max_delta);
	}
	else
	{
		update_screen_size(-1);
	}
}

long long GLWidget::get_count_vbos()
{
	return GLWidget_count_vbos;
}

void GLWidget::increment_count_vbos(long long x)
{
	GLWidget_count_vbos += x;
}

void GLWidget::set_max_vbos_65535(bool t)
{
	GLWidget_max_vbos_65535 = t;
}

bool GLWidget::get_max_vbos_65535()
{
	return GLWidget_max_vbos_65535;
}

void GLWidget::generate_vao1(GLuint * vao, GLuint * vbo, GLuint * attr_v, GLuint * attr_t)
{
	*vao = 0;
	vbo[0] = 0;
	vbo[1] = 0;
	GLfloat * v = new GLfloat[12];
	GLfloat * t = new GLfloat[12];
	for (int x = 0; x < 12; x++)
	{
		v[x] = 0.0f;
		t[x] = 0.0f;
	}
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
	glGenBuffers(2, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_v,3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_v);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 4*3*sizeof(GLfloat), t, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_t, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_t);
	glBindVertexArray(0);
	increment_count_vbos(2);
	vaoids.push_back(*vao);
	vboids.push_back(vbo);
	delete [] v;
	delete [] t;
}

void GLWidget::generate_raycastcube0_vao(
	GLuint * vao, GLuint * vbo,
	GLuint * attr_v, GLuint * attr_c)
{
	*vao = 0;
	vbo[0] = 0;
	vbo[1] = 0;
	const float x = 100.0f;
	const float y = 100.0f;
	const float z = 100.0f;
	GLfloat v[] = {
		-x,  y, z,
		-x, -y, z,
		x,  y, z,
		x, -y, z,
		x,  y, -z,
		x, -y, -z,
		-x,  y, -z,
		-x, -y, -z,
		-x,  y, z,
		-x, -y, z };
	GLfloat c[] = {
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f };
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
	glGenBuffers(2, vbo);
	increment_count_vbos(2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_v, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_v);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), c, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_c, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_c);
	glBindVertexArray(0);
	vaoids.push_back(*vao);
	vboids.push_back(vbo);
}

void GLWidget::generate_raycastcube1_vao(
	GLuint * vao, GLuint * vbo,
	GLuint * attr_v, GLuint * attr_c)
{
	*vao = 0;
	vbo[0] = 0;
	vbo[1] = 0;
	const float x = 100.0f;
	const float y = 100.0f;
	const float z = 100.0f;
	GLfloat v[] = {
		-x, y, -z,
		-x, y,  z,
		x, y, -z,
		x, y,  z };
	GLfloat c[] = {
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f };
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
	glGenBuffers(2, vbo);
	increment_count_vbos(2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_v, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_v);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), c, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_c, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_c);
	glBindVertexArray(0);
	vaoids.push_back(*vao);
	vboids.push_back(vbo);
}

void GLWidget::generate_raycastcube2_vao(
	GLuint * vao, GLuint * vbo,
	GLuint * attr_v, GLuint * attr_c)
{
	*vao = 0;
	vbo[0] = 0;
	vbo[1] = 0;
	const float x = 100.0f;
	const float y = 100.0f;
	const float z = 100.0f;
	GLfloat v[] = {
		-x, -y,  z,
		-x, -y, -z,
		x, -y,  z,
		x, -y, -z };
	GLfloat c[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f };
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
	glGenBuffers(2, vbo);
	increment_count_vbos(2);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), v, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_v, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_v);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), c, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_c, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_c);
	glBindVertexArray(0);
	vaoids.push_back(*vao);
	vboids.push_back(vbo);
}

void GLWidget::generate_raycast_shader_vao(
	GLuint * vao,
	GLuint * vbo0, GLuint * vbo1, GLuint * vbo2,
	GLuint * attr_v)
{
	vao[0] = 0;
	vao[1] = 1;
	vao[2] = 2;
	const float x = 100.0f;
	const float y = 100.0f;
	const float z = 100.0f;
	GLfloat v0[] = {
		-x,  y, z,
		-x, -y, z,
		x,  y, z,
		x, -y, z,
		x,  y, -z,
		x, -y, -z,
		-x,  y, -z,
		-x, -y, -z,
		-x,  y, z,
		-x, -y, z };
	GLfloat v1[] = {
		-x, y, -z,
		-x, y,  z,
		x, y, -z,
		x, y,  z };
	GLfloat v2[] = {
		-x, -y,  z,
		-x, -y, -z,
		x, -y,  z,
		x, -y, -z };
	//
	glGenVertexArrays(1, &(vao[0]));
	glBindVertexArray(vao[0]);
	glGenBuffers(1, vbo0);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo0);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), v0, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_v, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_v);
	glBindVertexArray(0);
	//
	glGenVertexArrays(1, &(vao[1]));
	glBindVertexArray(vao[1]);
	glGenBuffers(1, vbo1);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo1);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), v1, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_v, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_v);
	glBindVertexArray(0);
	//
	glGenVertexArrays(1, &(vao[2]));
	glBindVertexArray(vao[2]);
	glGenBuffers(1, vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo2);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), v2, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(*attr_v, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr_v);
	glBindVertexArray(0);
	//
	increment_count_vbos(3);
}

bool GLWidget::update_raycast_shader_vbo(
	unsigned int orientation,
	float x__, float y__, float z__,
	GLuint * raycastcube0, GLuint * raycastcube1, GLuint * raycastcube2,
	bool both)
{
	bool force_skip_cube = false;
	switch (orientation)
	{
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIP:
		raycast_cube_RIP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIP:
		raycast_cube_LIP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSP:
		raycast_cube_RSP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSP:
		raycast_cube_LSP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIA:
		raycast_cube_RIA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA:
		raycast_cube_LIA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSA:
		raycast_cube_RSA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSA:
		raycast_cube_LSA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRP:
		raycast_cube_IRP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILP:
		raycast_cube_ILP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRP:
		raycast_cube_SRP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLP:
		raycast_cube_SLP(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRA:
		raycast_cube_IRA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILA:
		raycast_cube_ILA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRA:
		raycast_cube_SRA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLA:
		raycast_cube_SLA(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI:
		raycast_cube_RPI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPI:
		raycast_cube_LPI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI:
		raycast_cube_RAI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAI:
		raycast_cube_LAI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPS:
		raycast_cube_RPS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPS:
		raycast_cube_LPS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS:
		raycast_cube_RAS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS:
		raycast_cube_LAS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRI:
		raycast_cube_PRI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLI:
		raycast_cube_PLI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARI:
		raycast_cube_ARI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALI:
		raycast_cube_ALI(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRS:
		raycast_cube_PRS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLS:
		raycast_cube_PLS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARS:
		raycast_cube_ARS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALS:
		raycast_cube_ALS(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPR:
		raycast_cube_IPR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPR:
		raycast_cube_SPR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAR:
		raycast_cube_IAR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAR:
		raycast_cube_SAR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPL:
		raycast_cube_IPL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPL:
		raycast_cube_SPL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAL:
		raycast_cube_IAL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAL:
		raycast_cube_SAL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIR:
		raycast_cube_PIR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSR:
		raycast_cube_PSR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIR:
		raycast_cube_AIR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASR:
		raycast_cube_ASR(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIL:
		raycast_cube_PIL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSL:
		raycast_cube_PSL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIL:
		raycast_cube_AIL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	case itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASL:
		raycast_cube_ASL(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	default:
		force_skip_cube = true;
		raycast_cube(x__,y__,z__,raycastcube0,raycastcube1,raycastcube2,both);
		break;
	}
	return force_skip_cube;
}

void GLWidget::generate_quad(
	GLfloat * v,
	GLfloat * t,
	GLfloat * n,
	GLfloat * tan,
	GLuint  * vboid)
{
	//  GL_TRIANGLE_STRIP
	//
	//    0----2
	//    |  / |
	//    | /  |
	//    1----3
	//
	if (!v) return;
	glBindBuffer(GL_ARRAY_BUFFER, vboid[0]);
	glBufferData(GL_ARRAY_BUFFER, (4*3)*sizeof(GLfloat), v, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	if (t)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboid[1]);
		glBufferData(GL_ARRAY_BUFFER, (4*3)*sizeof(GLfloat), t, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	if (n)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboid[2]);
		glBufferData(GL_ARRAY_BUFFER, (4*3)*sizeof(GLfloat), n, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	if (tan)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboid[3]);
		glBufferData(GL_ARRAY_BUFFER, (4*3)*sizeof(GLfloat), tan, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

GLuint GLWidget::load_shader(GLenum shaderType, const char * pSource)
{
	GLuint shader = glCreateShader(shaderType);
	if (shader)
	{
		glShaderSource(shader, 1, &pSource, NULL);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled)
		{
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen)
			{
				const unsigned int infoLen__ = infoLen;
				char * buf = new char[infoLen__];
				if (buf)
				{
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					printf("Could not compile shader %d:\n%s\n", shaderType, buf);
					delete [] buf;
				}
			}
			glDeleteShader(shader);
			shader = 0;
		}
	}
	return shader;
}

bool GLWidget::create_program(
	const char * vertex,
	const char * fragment,
	ShaderObj * so,
	bool)
{
	GLuint vertex_shader, pixel_shader, program;
	vertex_shader = load_shader(GL_VERTEX_SHADER, vertex);
	if (!vertex_shader) return false;
	pixel_shader = load_shader(GL_FRAGMENT_SHADER, fragment);
	if (!pixel_shader) return false;
	program = glCreateProgram();
	if (program)
	{
		glAttachShader(program, vertex_shader);
		glAttachShader(program, pixel_shader);
		glLinkProgram(program);
		GLint linkStatus = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus != GL_TRUE)
		{
			GLint bufLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
			if (bufLength)
			{
				const unsigned int bufLength__ = bufLength;
				char * buf;
				try { buf = new char[bufLength__]; }
				catch (std::bad_alloc&) { return false; }
				if (buf)
				{
					glGetProgramInfoLog(program, bufLength, NULL, buf);
					printf("Could not link program:\n%s\n", buf);
					delete [] buf;
				}
			}
			glDeleteProgram(program);
			return false;
		}
		so->program = program;
		so->vshader = vertex_shader;
		so->fshader = pixel_shader;
		return true;
	}
	return false;
}

void GLWidget::checkGLerror(const char * op)
{
	const int error__ = (int)glGetError();
	if (error__ == 0) return;
	QString e;
	switch(error__)
	{
	case 0x0500:
		e = QString("GL_INVALID_ENUM");
		break;
	case 0x0501:
		e = QString("GL_INVALID_VALUE");
		break;
	case 0x0502:
		e = QString("GL_INVALID_OPERATION");
		break;
	case 0x0503:
		e = QString("GL_STACK_OVERFLOW");
		break;
	case 0x0504:
		e = QString("GL_STACK_UNDERFLOW");
		break;
	case 0x0505:
		e = QString("GL_OUT_OF_MEMORY");
		break;
	case 0x0506:
		e = QString("GL_INVALID_FRAMEBUFFER_OPERATION");
		break;
	case 0x0507:
		e = QString("GL_CONTEXT_LOST");
		break;
	case 0x8031:
		e = QString("GL_TABLE_TOO_LARGE");
		break;
	default:
		e = QString("Error ") + QVariant(error__).toString();
		break;
	}
	std::cout << e.toStdString() << " after " << op << std::endl;
}

void GLWidget::makeModelVBO_ArraysT(
	GLuint * vboid,
	GLuint * vaoid,
	GLuint * v_attr,
	GLuint * n_attr,
	GLuint * t_attr,
	GLuint * ta_attr,
	const float * vertices,
	const float * normals,
	const float * textures,
	const float * tangents,
	const unsigned int * faces,
	const int faces_size,
	GLenum usage,
	float scale)
{
	float * v;
	try { v = new float[(faces_size/12)*3*3]; }
	catch(std::bad_alloc&) { return; }
	if (!v) return;

	float * n  = NULL; // initialized to avoid warning
	if (normals)
	{
		try { n  = new float[(faces_size/12)*3*3]; }
		catch(std::bad_alloc&) { return; }
		if (!n) return;
	}

	float * t  = NULL; // initialized to avoid warning
	if (textures)
	{
		try { t  = new float[(faces_size/12)*3*3]; }
		catch(std::bad_alloc&) { return; }
		if (!t) return;
	}

	float * ta = NULL; // initialized to avoid warning
	if (tangents)
	{
		try { ta = new float[(faces_size/12)*3*3]; }
		catch(std::bad_alloc&) { return; }
		if (!ta) return;
	}

	unsigned int idx = 0;

	for (int ind = 0; ind < faces_size; ind+=12 )
	{
		int c1  = *(faces + ind + 0);
		int t1  = *(faces + ind + 1);
		int n1  = *(faces + ind + 2);
		int ta1 = *(faces + ind + 3);

		int c2   = *(faces + ind + 4);
		int t2   = *(faces + ind + 5);
		int n2   = *(faces + ind + 6);
		int ta2  = *(faces + ind + 7);

		int c3   = *(faces + ind + 8);
		int t3   = *(faces + ind + 9);
		int n3   = *(faces + ind + 10);
		int ta3  = *(faces + ind + 11);

		if (textures)
		{
			t[idx + 0 ] =  *(textures + 0 + (t1-1)*3);
			t[idx + 1 ] =  *(textures + 1 + (t1-1)*3);
			t[idx + 2 ] =  *(textures + 2 + (t1-1)*3);
		}

		if (normals)
		{
			n[idx + 0 ] = *(normals + 0 + (n1-1)*3);
			n[idx + 1 ] = *(normals + 1 + (n1-1)*3);
			n[idx + 2 ] = *(normals + 2 + (n1-1)*3);
		}

		if (tangents)
		{
			ta[idx + 0 ] = *(tangents + 0 + (ta1-1)*3);
			ta[idx + 1 ] = *(tangents + 1 + (ta1-1)*3);
			ta[idx + 2 ] = *(tangents + 2 + (ta1-1)*3);
		}

		v[idx + 0 ] = *(vertices + 0 + (c1-1)*3) * scale;
		v[idx + 1 ] = *(vertices + 1 + (c1-1)*3) * scale;
		v[idx + 2 ] = *(vertices + 2 + (c1-1)*3) * scale;

		if (textures)
		{
			t[idx + 3 ] = *(textures + 0 + (t2-1)*3);
			t[idx + 4 ] = *(textures + 1 + (t2-1)*3);
			t[idx + 5 ] = *(textures + 2 + (t2-1)*3);
		}

		if (normals)
		{
			n[idx + 3] = *(normals + 0 + (n2-1)*3);
			n[idx + 4] = *(normals + 1 + (n2-1)*3);
			n[idx + 5] = *(normals + 2 + (n2-1)*3);
		}

		if (tangents)
		{
			ta[idx + 3 ] = *(tangents + 0 + (ta2-1)*3);
			ta[idx + 4 ] = *(tangents + 1 + (ta2-1)*3);
			ta[idx + 5 ] = *(tangents + 2 + (ta2-1)*3);
		}

		v[idx + 3] = *(vertices + 0 + (c2-1)*3) * scale;
		v[idx + 4] = *(vertices + 1 + (c2-1)*3) * scale;
		v[idx + 5] = *(vertices + 2 + (c2-1)*3) * scale;

		if (textures)
		{
			t[idx + 6] = *(textures + 0 + (t3-1)*3);
			t[idx + 7] = *(textures + 1 + (t3-1)*3);
			t[idx + 8] = *(textures + 2 + (t3-1)*3);
		}

		if (normals)
		{
			n[idx + 6] = *(normals + 0 + (n3-1)*3);
			n[idx + 7] = *(normals + 1 + (n3-1)*3);
			n[idx + 8] = *(normals + 2 + (n3-1)*3);
		}

		if (tangents)
		{
			ta[idx + 6 ] = *(tangents + 0 + (ta3-1)*3);
			ta[idx + 7 ] = *(tangents + 1 + (ta3-1)*3);
			ta[idx + 8 ] = *(tangents + 2 + (ta3-1)*3);
		}

		v[idx + 6] = *(vertices + 0 + (c3-1)*3) * scale;
		v[idx + 7] = *(vertices + 1 + (c3-1)*3) * scale;
		v[idx + 8] = *(vertices + 2 + (c3-1)*3) * scale;

		idx += 9;
	}

	vboid[0] = 0;
	vboid[1] = 0;
	vboid[2] = 0;
	vboid[3] = 0;

	unsigned int count_buffers = 1;

	if (normals)  count_buffers++;
	if (textures) count_buffers++;
	if (tangents) count_buffers++;

	glGenVertexArrays(1, vaoid);
	glBindVertexArray(*vaoid);
	glGenBuffers(count_buffers, vboid);

	glBindBuffer(GL_ARRAY_BUFFER, vboid[0]);
	glBufferData(GL_ARRAY_BUFFER, (faces_size/12)*3*3*sizeof(float), v, usage);
	glVertexAttribPointer(*v_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*v_attr);

	if (normals)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboid[1]);
		glBufferData(GL_ARRAY_BUFFER, (faces_size/12)*3*3*sizeof(float), n, usage);
		glVertexAttribPointer(*n_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(*n_attr);
	}

	if (textures)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboid[2]);
		glBufferData(GL_ARRAY_BUFFER, (faces_size/12)*3*3*sizeof(float), t, usage);
		glVertexAttribPointer(*t_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(*t_attr);
	}

	if (tangents)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboid[3]);
		glBufferData(GL_ARRAY_BUFFER, (faces_size/12)*3*3*sizeof(float), ta, usage);
		glVertexAttribPointer(*ta_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(*ta_attr);
	}
	glBindVertexArray(0);

	increment_count_vbos(count_buffers);
	vboids.push_back(vboid);
	vaoids.push_back(*vaoid);

	delete [] v;
	if (normals)  delete [] n;
	if (textures) delete [] t;
	if (tangents) delete [] ta;
}

void GLWidget::generate_screen_quad(GLuint * vbo, GLuint * vao, GLuint * attr)
{
// Full Screen Quad
//
//  NDC:
//  (-1,  1) top-left corner of the viewport.
//  (-1, -1) bottom-left corner of the viewport.
//  ( 1,  1) top-right corner of the viewport.
//  ( 1, -1) bottom-right corner of the viewport.
//
//
//  GL_TRIANGLE_STRIP
//
//    0----2
//    |  / |
//    | /  |
//    1----3
//
// vertex shader:
// out_position = float4(in_position, 0.0, 1.0);
// out_texcoord = 0.5*in_position.xy + 0.5;
//
//
//
	GLfloat v[] = {
		-1.0f,  1.0f,   // 0
		-1.0f, -1.0f,   // 1
		 1.0f,  1.0f,   // 2
		 1.0f, -1.0f }; // 3
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
	glGenBuffers(1, vbo);
	increment_count_vbos(1);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, (4*2)*sizeof(GLfloat), v, GL_STATIC_DRAW);
	glVertexAttribPointer(*attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(*attr);
	glBindVertexArray(0);
}

void GLWidget::free_fbos0(
	GLuint * framebuffer,
	GLuint * color_texture,
	GLuint * depth_texture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, 0, 0);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	glDeleteFramebuffers(1, framebuffer);
	glDeleteTextures(1, color_texture);
	glDeleteTextures(1, depth_texture);
}

bool GLWidget::create_fbos0(
	unsigned int w,
	unsigned int h,
	GLuint * framebuffer,
	GLuint * color_texture,
	GLuint * depth_texture,
	GLenum   format,
	GLenum   type,
	GLenum   target)
{
	glGenTextures(1, depth_texture);
	glBindTexture(target, *depth_texture);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(target,
				0, 
				GL_DEPTH_COMPONENT24, //GL_DEPTH_COMPONENT16
				w,
				h,
				0, 
				GL_DEPTH_COMPONENT, 
				GL_UNSIGNED_INT, //GL_UNSIGNED_SHORT,
				NULL);
	glGenTextures(1, color_texture);
	glBindTexture(target, *color_texture );
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(target, 0, format, w, h, 0, format, type, NULL);
	glGenFramebuffers(1, framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, *color_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  target, *depth_texture, 0);
	bool ok = false;
	switch(glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
	case GL_FRAMEBUFFER_COMPLETE:ok = true; break;
	case GL_FRAMEBUFFER_UNSUPPORTED : break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT : break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT : break;
	default: break;
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	return ok;
}

void GLWidget::free_fbos1(
	GLuint * framebuffer,
	GLuint * color_texture,
	GLuint * depth_rb)
{
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	glDeleteFramebuffers(1, framebuffer);
	glDeleteTextures(1, color_texture);
	glDeleteRenderbuffers(1, depth_rb);
}

bool GLWidget::create_fbos1(
	unsigned int   w,
	unsigned int   h,
	GLuint * framebuffer,
	GLuint * color_texture,
	GLuint * depth_rb,
	GLenum   format,
	GLenum   type,
	GLenum   target)
{
	glGenRenderbuffers(1, depth_rb);
	glBindRenderbuffer(GL_RENDERBUFFER, *depth_rb);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
	glGenFramebuffers(1, framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
	glGenTextures(1, color_texture);
	glBindTexture(target, *color_texture);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(target, 0, format, w, h, 0, format, type, NULL);
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, *color_texture, 0);
	bool ok = false;
	switch(glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
	case GL_FRAMEBUFFER_COMPLETE: ok = true; break;
	case GL_FRAMEBUFFER_UNSUPPORTED : break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT : break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT : break;
	default: break;
	}
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	return ok;
}

void GLWidget::raycast_cube(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca1[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca2[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RIP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_rip[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_rip[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_rip[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_rip, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_rip, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_rip, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_rip[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca1_rip[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca2_rip[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_rip, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_rip, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_rip, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LIP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_lip[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_lip[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_lip[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_lip, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_lip, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_lip, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_lip[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca1_lip[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca2_lip[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_lip, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_lip, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_lip, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RSP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{

	GLfloat va0_rsp[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_rsp[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_rsp[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_rsp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_rsp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_rsp, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_rsp[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca1_rsp[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca2_rsp[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_rsp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_rsp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_rsp, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LSP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{

	GLfloat va0_lsp[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_lsp[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_lsp[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_lsp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_lsp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_lsp, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_lsp[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca1_lsp[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca2_lsp[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_lsp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_lsp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_lsp, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RIA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ria[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_ria[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_ria[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ria, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ria, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ria, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ria[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca1_ria[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca2_ria[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ria, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ria, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ria, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LIA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_lia[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_lia[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_lia[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_lia, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_lia, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_lia, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_lia[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca1_lia[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca2_lia[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_lia, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_lia, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_lia, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RSA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_rsa[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_rsa[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_rsa[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_rsa, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_rsa, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_rsa, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_rsa[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca1_rsa[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca2_rsa[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_rsa, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_rsa, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_rsa, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LSA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_lsa[] = {
			-x,  z, y,
			-x, -z, y,
			 x,  z, y,
			 x, -z, y,
			 x,  z, -y,
			 x, -z, -y,
			-x,  z, -y,
			-x, -z, -y,
			-x,  z, y,
			-x, -z, y };
	GLfloat va1_lsa[] = {
			-x, z, -y,
			-x, z,  y,
			 x, z, -y,
			 x, z,  y };
	GLfloat va2_lsa[] = {
			-x, -z,  y,
			-x, -z, -y,
			 x, -z,  y,
			 x, -z, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_lsa, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_lsa, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_lsa, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_lsa[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca1_lsa[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca2_lsa[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_lsa, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_lsa, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_lsa, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_IRP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_irp[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_irp[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_irp[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_irp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_irp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_irp, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_irp[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca1_irp[] = {
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca2_irp[] = {
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f };

		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_irp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_irp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_irp, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ILP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ilp[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_ilp[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_ilp[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ilp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ilp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ilp, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ilp[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca1_ilp[] = {
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca2_ilp[] = {
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f };

		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ilp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ilp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ilp, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SRP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_srp[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_srp[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_srp[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_srp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_srp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_srp, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_srp[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca1_srp[] = {
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca2_srp[] = {
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f };

		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_srp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_srp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_srp, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SLP(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_slp[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_slp[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_slp[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_slp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_slp, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_slp, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_slp[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca1_slp[] = {
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca2_slp[] = {
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f };

		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_slp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_slp, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_slp, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_IRA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ira[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_ira[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_ira[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ira, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ira, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ira, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ira[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca1_ira[] = {
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca2_ira[] = {
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ira, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ira, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ira, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ILA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ila[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_ila[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_ila[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ila, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ila, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ila, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ila[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca1_ila[] = {
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca2_ila[] = {
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ila, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ila, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ila, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SRA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_sra[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_sra[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_sra[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_sra, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_sra, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_sra, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_sra[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca1_sra[] = {
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca2_sra[] = {
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_sra, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_sra, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_sra, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SLA(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_sla[] = {
			-y,  z, x,
			-y, -z, x,
			 y,  z, x,
			 y, -z, x,
			 y,  z, -x,
			 y, -z, -x,
			-y,  z, -x,
			-y, -z, -x,
			-y,  z, x,
			-y, -z, x };
	GLfloat va1_sla[] = {
			-y, z, -x,
			-y, z,  x,
			 y, z, -x,
			 y, z,  x };
	GLfloat va2_sla[] = {
			-y, -z,  x,
			-y, -z, -x,
			 y, -z,  x,
			 y, -z, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_sla, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_sla, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_sla, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_sla[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca1_sla[] = {
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca2_sla[] = {
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_sla, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_sla, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_sla, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RPI(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_rpi[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_rpi[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_rpi[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_rpi, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_rpi, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_rpi, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_rpi[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca1_rpi[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca2_rpi[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_rpi, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_rpi, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_rpi, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LPI(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_lpi[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_lpi[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_lpi[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_lpi, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_lpi, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_lpi, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_lpi[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca1_lpi[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca2_lpi[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_lpi, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_lpi, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_lpi, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RAI(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_rai[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_rai[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_rai[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_rai, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_rai, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_rai, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_rai[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca1_rai[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca2_rai[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_rai, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_rai, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_rai, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LAI(
	float x, float y , float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_lai[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_lai[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_lai[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_lai, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_lai, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_lai, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_lai[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca1_lai[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca2_lai[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_lai, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_lai, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_lai, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RPS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_rps[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_rps[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_rps[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_rps, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_rps, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_rps, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_rps[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca1_rps[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca2_rps[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_rps, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_rps, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_rps, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LPS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_lps[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_lps[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_lps[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_lps, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_lps, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_lps, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_lps[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca1_lps[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca2_lps[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_lps, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_lps, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_lps, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_RAS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ras[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_ras[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_ras[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ras, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ras, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ras, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ras[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca1_ras[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca2_ras[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ras, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ras, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ras, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_LAS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_las[] = {
			-x,  y, z,
			-x, -y, z,
			 x,  y, z,
			 x, -y, z,
			 x,  y, -z,
			 x, -y, -z,
			-x,  y, -z,
			-x, -y, -z,
			-x,  y, z,
			-x, -y, z };
	GLfloat va1_las[] = {
			-x, y, -z,
			-x, y,  z,
			 x, y, -z,
			 x, y,  z };
	GLfloat va2_las[] = {
			-x, -y,  z,
			-x, -y, -z,
			 x, -y,  z,
			 x, -y, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_las, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_las, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_las, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_las[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca1_las[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca2_las[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_las, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_las, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_las, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PRI(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_pri[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_pri[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_pri[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_pri, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_pri, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_pri, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_pri[] = {
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca1_pri[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca2_pri[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_pri, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_pri, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_pri, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PLI(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_pli[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_pli[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_pli[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_pli, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_pli, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_pli, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_pli[] = {
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca1_pli[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca2_pli[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_pli, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_pli, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_pli, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ARI(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ari[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_ari[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_ari[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ari, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ari, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ari, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ari[] = {
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca1_ari[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca2_ari[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ari, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ari, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ari, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ALI(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ali[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_ali[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_ali[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ali, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ali, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ali, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ali[] = {
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca1_ali[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca2_ali[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ali, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ali, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ali, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PRS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_prs[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_prs[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_prs[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_prs, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_prs, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_prs, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_prs[] = {
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca1_prs[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca2_prs[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_prs, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_prs, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_prs, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PLS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_pls[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_pls[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_pls[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_pls, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_pls, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_pls, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_pls[] = {
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca1_pls[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca2_pls[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_pls, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_pls, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_pls, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ARS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ars[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_ars[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_ars[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ars, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ars, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ars, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ars[] = {
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca1_ars[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca2_ars[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ars, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ars, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ars, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ALS(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_als[] = {
			-y,  x, z,
			-y, -x, z,
			 y,  x, z,
			 y, -x, z,
			 y,  x, -z,
			 y, -x, -z,
			-y,  x, -z,
			-y, -x, -z,
			-y,  x, z,
			-y, -x, z };
	GLfloat va1_als[] = {
			-y, x, -z,
			-y, x,  z,
			 y, x, -z,
			 y, x,  z };
	GLfloat va2_als[] = {
			-y, -x,  z,
			-y, -x, -z,
			 y, -x,  z,
			 y, -x, -z };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_als, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_als, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_als, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_als[] = {
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca1_als[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca2_als[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_als, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_als, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_als, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_IPR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ipr[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_ipr[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_ipr[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ipr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ipr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ipr, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ipr[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca1_ipr[] = {
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca2_ipr[] = {
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ipr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ipr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ipr, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SPR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_spr[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_spr[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_spr[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_spr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_spr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_spr, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_spr[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca1_spr[] = {
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca2_spr[] = {
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_spr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_spr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_spr, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_IAR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_iar[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_iar[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_iar[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_iar, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_iar, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_iar, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_iar[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca1_iar[] = {
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca2_iar[] = {
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_iar, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_iar, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_iar, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SAR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_sar[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_sar[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_sar[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_sar, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_sar, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_sar, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_sar[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca1_sar[] = {
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca2_sar[] = {
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_sar, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_sar, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_sar, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_IPL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ipl[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_ipl[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_ipl[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ipl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ipl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ipl, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ipl[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca1_ipl[] = {
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca2_ipl[] = {
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ipl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ipl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ipl, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SPL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_spl[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_spl[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_spl[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_spl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_spl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_spl, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_spl[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca1_spl[] = {
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca2_spl[] = {
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_spl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_spl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_spl, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_IAL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ial[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_ial[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_ial[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ial, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ial, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ial, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ial[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca1_ial[] = {
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca2_ial[] = {
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ial, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ial, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ial, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_SAL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_sal[] = {
			-z,  y, x,
			-z, -y, x,
			 z,  y, x,
			 z, -y, x,
			 z,  y, -x,
			 z, -y, -x,
			-z,  y, -x,
			-z, -y, -x,
			-z,  y, x,
			-z, -y, x };
	GLfloat va1_sal[] = {
			-z, y, -x,
			-z, y,  x,
			 z, y, -x,
			 z, y,  x };
	GLfloat va2_sal[] = {
			-z, -y,  x,
			-z, -y, -x,
			 z, -y,  x,
			 z, -y, -x };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_sal, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_sal, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_sal, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_sal[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca1_sal[] = {
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca2_sal[] = {
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_sal, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_sal, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_sal, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PIR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_pir[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_pir[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_pir[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_pir, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_pir, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_pir, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_pir[] = {
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca1_pir[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca2_pir[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_pir, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_pir, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_pir, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PSR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_psr[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_psr[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_psr[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_psr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_psr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_psr, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_psr[] = {
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca1_psr[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca2_psr[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_psr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_psr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_psr, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_AIR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_air[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_air[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_air[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_air, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_air, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_air, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_air[] = {
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca1_air[] = {
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca2_air[] = {
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_air, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_air, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_air, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ASR(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_asr[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_asr[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_asr[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_asr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_asr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_asr, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_asr[] = {
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca1_asr[] = {
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca2_asr[] = {
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_asr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_asr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_asr, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PIL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_pil[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_pil[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_pil[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_pil, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_pil, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_pil, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_pil[] = {
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f };
		GLfloat ca1_pil[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		GLfloat ca2_pil[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_pil, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_pil, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_pil, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_PSL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_psl[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_psl[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_psl[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_psl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_psl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_psl, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_psl[] = {
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 1.0f };
		GLfloat ca1_psl[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		GLfloat ca2_psl[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_psl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_psl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_psl, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_AIL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_ail[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_ail[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_ail[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_ail, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_ail, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_ail, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_ail[] = {
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f };
		GLfloat ca1_ail[] = {
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f };
		GLfloat ca2_ail[] = {
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_ail, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_ail, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_ail, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLWidget::raycast_cube_ASL(
	float x, float y, float z,
	GLuint * vboid0, GLuint * vboid1, GLuint * vboid2, bool b)
{
	GLfloat va0_asl[] = {
			-z,  x, y,
			-z, -x, y,
			 z,  x, y,
			 z, -x, y,
			 z,  x, -y,
			 z, -x, -y,
			-z,  x, -y,
			-z, -x, -y,
			-z,  x, y,
			-z, -x, y };
	GLfloat va1_asl[] = {
			-z, x, -y,
			-z, x,  y,
			 z, x, -y,
			 z, x,  y };
	GLfloat va2_asl[] = {
			-z, -x,  y,
			-z, -x, -y,
			 z, -x,  y,
			 z, -x, -y };
	glBindBuffer(GL_ARRAY_BUFFER, vboid0[0]);
	glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), va0_asl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid1[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va1_asl, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboid2[0]);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), va2_asl, GL_DYNAMIC_DRAW);
	if (b)
	{
		GLfloat ca0_asl[] = {
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f,
					1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f,
					1.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					1.0f, 1.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					0.0f, 0.0f, 1.0f };
		GLfloat ca1_asl[] = {
					1.0f, 1.0f, 1.0f,
					1.0f, 0.0f, 1.0f,
					1.0f, 1.0f, 0.0f,
					1.0f, 0.0f, 0.0f };
		GLfloat ca2_asl[] = {
					0.0f, 0.0f, 1.0f,
					0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f };
		glBindBuffer(GL_ARRAY_BUFFER, vboid0[1]);
		glBufferData(GL_ARRAY_BUFFER, 30*sizeof(GLfloat), ca0_asl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid1[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca1_asl, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, vboid2[1]);
		glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), ca2_asl, GL_DYNAMIC_DRAW);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#if 0
static GLubyte color_bound(const float i)
{
	if      (i < 0.0f) return (GLubyte)0;
	else if (i > 1.0f) return (GLubyte)255;
	return (GLubyte)(255 * i);
}
#endif

void GLWidget::generateAnisoTexture(
	GLuint * tex,
	const unsigned int h,
	const unsigned int w)
{
#if 0
	const float color[4] = { 0.4f, 0.4f, 0.7f, 1.0f };
	const float Ka = 0.5f;
	const float Kd = 0.4f;
	const float Ks = 0.8f;
	const float spec = 60.0f;
	const bool  use_Ka = true;
	const bool  use_Kd = true;
	const bool  use_Ks = true;
	const float p = 2.0f; // compensation for unusually uniform brightness
	GLubyte t[h][w][3];
	for (unsigned int y = 0; y < h; y++)
	{
		const float LdotT = 2.0f * y / (h - 1.0f) - 1.0f;
		const float LdotN = sqrt(1.0f - LdotT * LdotT);
		for (unsigned int x = 0; x < w; x++)
		{
			const float VdotT = 2.0f * x / (w - 1.0f) - 1.0f;
			const float VdotN = sqrt(1.0f - VdotT * VdotT);
			float       VdotR = LdotN * VdotN - LdotT * VdotT;
			if (VdotR < 0) VdotR = 0;
			for (unsigned int i = 0; i < 3; i++)
			{
				const float c =
					(use_Ka ? Ka : 0.0f) +
					(use_Kd ? color[i] * Kd * pow(LdotN, p) : 0.0f) +
					(use_Ks ? Ks * pow(VdotR, spec) : 0.0f);
				t[y][x][i] = color_bound(c);
			}
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
}

void GLWidget::d_orientcube(
	qMeshData  * s,
	void  * ,
	int   * ,
	float * mvp,
	float * ,
	float * ,
	float * ,
	float * ,
	float * ,
	float * ,
	float * ,
	float * sparams,
	void  * )
{
	glUniformMatrix4fv(s->shader->location_mvp, 1, GL_FALSE, mvp);
	glUniform3fv(s->shader->location_sparams, 2, sparams);
	glBindVertexArray(s->vaoid);
	glDrawArrays(GL_TRIANGLES, 0, s->faces_size*3);
}

void GLWidget::d_mesh(
	qMeshData * s,
	void  * ,
	int   * ,
	float * mvp,
	float * ,
	float * ,
	float * ,
	float * ,
	float * ,
	float * ,
	float * ,
	float * sparams,
	void  * )
{
	glUniformMatrix4fv(s->shader->location_mvp, 1, GL_FALSE, mvp);
	glUniform3fv(s->shader->location_sparams, 2, sparams);
	glUniform4fv(s->shader->location_K, 2, s->K);
	glBindVertexArray(s->vaoid);
	glDrawArrays(GL_TRIANGLES, 0, s->faces_size*3);
}
