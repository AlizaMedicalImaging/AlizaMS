#ifndef GLWIDGET_QT4CORE_H
#define GLWIDGET_QT4CORE_H

////////////////////////////////////////////
//
//
//
//
#define USE_SET_GL_FORMAT
#define USE_CORE_3_2_PROFILE
#define USE_GL_MAJOR_3_MINOR_2
//
//
//
//
////////////////////////////////////////////

#include "glew/include/GL/glew.h"
#include <QtGlobal>
#include <QGLWidget>
#include "camera.h"
#include <QPoint>
#include <QModelIndex>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QColor>
#include <QString>

#define SCENE_ORTHO_SIZE 150.0f
#define SCENE_POS_Z 400.0f
#define SCENE_FOV 45.0f
#define SCENE_FAR_PLANE 10000.0f
#define SCENE_ALPHA 1.0f
#define VBOIDS_SIZE 4
#define TEXTURES_SIZE 8

class ImageVariant;

class ShaderObj
{
public:
	ShaderObj();
	~ShaderObj();
	GLuint program;
	GLuint vshader;
	GLuint fshader;
	GLuint position_handle;
	GLuint normal_handle;
	GLuint tangent_handle;
	GLuint random_handle;
	GLuint color_handle;
	GLuint time_handle;
	GLuint velocity_handle;
	GLuint location_mvp;
	GLuint location_mv;
	GLuint location_modeling;
	GLuint location_modeling_inv_t;
	GLuint location_K;
	GLuint location_shininess;
	GLuint location_mparams;
	GLuint location_fparams;
	GLuint location_sparams;
	GLuint location_iparams;
	GLuint location_coparams;
	GLuint location_pparams;
	GLuint * texture_handle;
	GLuint * location_sampler;
};

class qMeshData
{
public:
	qMeshData();
	~qMeshData();
	float * K;
	float   shininess;
	unsigned int faces_size;
	GLuint * vboid;
	GLuint   vaoid;
	int    * get_shadows;
	int    * cast_shadows;
	GLuint * textures;
	GLuint * tex_units;
	ShaderObj * shader;
};

#include "btBulletCollisionCommon.h"
VECTORMATH_ALIGNED_PRE class CollisionObject
{
public:
	VECTORMATH_ALIGNED16_NEW();

	CollisionObject();
	~CollisionObject();
	void set_shape(btCollisionShape * s);
	btCollisionShape * get_shape();
	void set_collision_object(btCollisionObject * o);
	btCollisionObject * get_collision_object();
	unsigned int get_id() const;
	void set_id(unsigned int i);
	void set_mesh_data(qMeshData * q);
	qMeshData * get_mesh_data();
	btCollisionObject * collision_object;
	unsigned int id;
	btCollisionShape  * shape;
	qMeshData * mesh_data;
} VECTORMATH_ALIGNED_POST;

VECTORMATH_ALIGNED_PRE class GLWidget : public QGLWidget
{
Q_OBJECT
public:
	VECTORMATH_ALIGNED16_NEW();

	GLWidget();
	GLWidget(const QGLFormat&);
	~GLWidget();
	void init_();
	void close_();
	void zoom_in();
	void zoom_out();
	void update_clear_color();
	void set_wireframe(bool);
	void get_screen(bool);
	static long long get_count_vbos();
	static void increment_count_vbos(long long);
	static void set_max_vbos_65535(bool);
	static bool get_max_vbos_65535();
	bool skip_draw;
	int  max_tex_size;
	int  max_3d_texture_size;
	bool opengl_init_done;
	void set_left();
	void set_right();
	void set_forward();
	void set_backward();
	QString get_system_info() const;
	float mparams[16];
	float sparams[9];
	btCollisionWorld * m_collisionWorld;
	void init_physics(btCollisionWorld*);
	void send_ray0(float x, float y);
	btVector3 get_ray_to(float x, float y);
	btVector3 get_ray_from(float x, float y);
	void  init_opengl(int,int);
	void  paint__();
	void  paint_volume();
	void  paint_raycaster();
	void  resize__(int, int);
	int   normalize_angle(int);
	int   x_rotation, y_rotation, z_rotation;
	float ortho_size;
	double position_z;
	void draw_3d_tex1(
		GLuint*,
		GLuint*,
		const GLfloat*,
		const GLfloat*);
	void draw_frame2(const GLfloat*);
	int win_w, win_h;
	Camera * camera;
	bool   no_opengl3;
	short  view;
	short  axis;
	GLuint gradient1;
	GLuint gradient2;
	GLuint gradient3;
	GLuint gradient4;
	GLuint gradient5;
	GLuint gradient6;
	GLuint gradient7;
	void gen_lut_tex(const unsigned char*, const int, GLuint*);
	bool ortho_proj;
	float fov;
	float far_plane;
	float alpha;
	float brightness;
	bool  draw_frames_3d;
	bool  display_contours;
	QString bullet_info;
	QString opengl_info;
	short m_left;
	short m_right;
	short m_forw;
	short m_back;
	//
	int old_win_pos_x;
	int old_win_pos_y;
	int new_win_pos_x;
	int new_win_pos_y;
	void set_win_old_position(int, int);
	void set_win_new_position(int, int);
	//
	void set_clear_color(float, float, float);
	void set_show_cube(bool);
	void render_orient_cube1(float, float, float, float);
	void render_orient_cube2();
	//
	int pan_x;
	int pan_y;
	void set_pan_delta(int, int);
	void update_screen_size(double);
	void update_far_plane(float);
	void fit_to_screen(const ImageVariant*);
	//
	void set_selected_images_ptr(QList<ImageVariant*>*);
	void set_view_3d();
	void set_view_rc();
	bool rect_selection;
	bool show_cube;
	bool wireframe;
	float contours_width;
	//
	float  clear_color_r;
	float  clear_color_g;
	float  clear_color_b;
	GLuint framebuffer;
	GLuint fbo_tex;
	GLuint fbo_depth;
	GLuint backfacebuffer;
	GLuint backface_tex;
	GLuint backface_depth;
	GLuint frontfacebuffer;
	GLuint frontface_tex;
	GLuint frontface_depth;
	GLuint scene_vbo;
	GLuint scene_vao;
	GLuint frames_vbo;
	GLuint frames_vao;
	GLuint origin_vbo;
	GLuint origin_vao;
	ShaderObj fsquad_shader;
	ShaderObj zero_shader;
	ShaderObj raycast_shader;
	ShaderObj raycast_color_shader;
	ShaderObj raycast_shader_bb;
	ShaderObj raycast_color_shader_bb;
	ShaderObj raycast_shader_sigm;
	ShaderObj raycast_color_shader_sigm;
	ShaderObj raycast_shader_bb_sigm;
	ShaderObj raycast_color_shader_bb_sigm;
	ShaderObj c3d_shader_clamp;
	ShaderObj c3d_shader_gradient_clamp;
	ShaderObj c3d_shader_bb_clamp;
	ShaderObj c3d_shader_gradient_bb_clamp;
	ShaderObj c3d_shader;
	ShaderObj c3d_shader_gradient;
	ShaderObj c3d_shader_bb;
	ShaderObj c3d_shader_gradient_bb;
	ShaderObj c3d_shader_clamp_sigm;
	ShaderObj c3d_shader_gradient_clamp_sigm;
	ShaderObj c3d_shader_bb_clamp_sigm;
	ShaderObj c3d_shader_gradient_bb_clamp_sigm;
	ShaderObj c3d_shader_sigm;
	ShaderObj c3d_shader_gradient_sigm;
	ShaderObj c3d_shader_bb_sigm;
	ShaderObj c3d_shader_gradient_bb_sigm;
	GLuint * c3d_shader_clamp_vbo;
	GLuint * c3d_shader_gradient_clamp_vbo;
	GLuint * c3d_shader_bb_clamp_vbo;
	GLuint * c3d_shader_gradient_bb_clamp_vbo;
	GLuint * c3d_shader_vbo;
	GLuint * c3d_shader_gradient_vbo;
	GLuint * c3d_shader_bb_vbo;
	GLuint * c3d_shader_gradient_bb_vbo;
	GLuint * c3d_shader_clamp_sigm_vbo;
	GLuint * c3d_shader_gradient_clamp_sigm_vbo;
	GLuint * c3d_shader_bb_clamp_sigm_vbo;
	GLuint * c3d_shader_gradient_bb_clamp_sigm_vbo;
	GLuint * c3d_shader_sigm_vbo;
	GLuint * c3d_shader_gradient_sigm_vbo;
	GLuint * c3d_shader_bb_sigm_vbo;
	GLuint * c3d_shader_gradient_bb_sigm_vbo;
	GLuint raycast_shader_vao;
	GLuint raycast_color_shader_vao;
	GLuint raycast_shader_bb_vao;
	GLuint raycast_color_shader_bb_vao;
	GLuint raycast_shader_sigm_vao;
	GLuint raycast_color_shader_sigm_vao;
	GLuint raycast_shader_bb_sigm_vao;
	GLuint raycast_color_shader_bb_sigm_vao;
	GLuint c3d_shader_clamp_vao;
	GLuint c3d_shader_gradient_clamp_vao;
	GLuint c3d_shader_bb_clamp_vao;
	GLuint c3d_shader_gradient_bb_clamp_vao;
	GLuint c3d_shader_vao;
	GLuint c3d_shader_gradient_vao;
	GLuint c3d_shader_bb_vao;
	GLuint c3d_shader_gradient_bb_vao;
	GLuint c3d_shader_clamp_sigm_vao;
	GLuint c3d_shader_gradient_clamp_sigm_vao;
	GLuint c3d_shader_bb_clamp_sigm_vao;
	GLuint c3d_shader_gradient_bb_clamp_sigm_vao;
	GLuint c3d_shader_sigm_vao;
	GLuint c3d_shader_gradient_sigm_vao;
	GLuint c3d_shader_bb_sigm_vao;
	GLuint c3d_shader_gradient_bb_sigm_vao;
	ShaderObj frame_shader;
	ShaderObj simple_tex_shader;
	ShaderObj sphere0_shader;
	ShaderObj color_shader;
	ShaderObj orientcube_shader;
	ShaderObj mesh_shader;
	GLuint * raycastcube0;
	GLuint raycastcube0_vao;
	GLuint cubebuffer;
	GLuint cube_tex;
	GLuint cube_depth;
	qMeshData * cube;
	qMeshData * letters;
	qMeshData * letteri;
	qMeshData * lettera;
	qMeshData * letterp;
	qMeshData * letterr;
	qMeshData * letterl;

	void generate_vao1(GLuint*, GLuint*, GLuint*, GLuint*);
	void generate_quad(
		GLfloat*, GLfloat*, GLfloat*, GLfloat*, GLuint*);
	GLuint load_shader(GLenum, const char*);
	bool create_program(
		const char*, const char*, ShaderObj*, bool=false);
	void checkGLerror(const char*);
	void makeModelVBO_ArraysT(
		GLuint*, GLuint*,
		GLuint*, GLuint*,GLuint*, GLuint*,
		const float*, const float*, const float*, const float*,
		const unsigned int*, const int,
		GLenum=GL_STATIC_DRAW,
		float=1.0f);
	void generate_screen_quad(GLuint*, GLuint*, GLuint*);
	void free_fbos0(
		GLuint * framebuffer,
		GLuint * color_texture,
		GLuint * depth_texture);
	bool create_fbos0(unsigned int w, unsigned int h,
		GLuint * framebuffer,
		GLuint * color_texture,
		GLuint * depth_texture,
		GLenum   format = GL_RGBA,
		GLenum   type   = GL_UNSIGNED_BYTE,
		GLenum   target = GL_TEXTURE_2D);
	void free_fbos1(
		GLuint * framebuffer,
		GLuint * color_texture,
		GLuint * depth_rb);
	bool create_fbos1(unsigned int w, unsigned int h,
		GLuint * framebuffer,
		GLuint * color_texture,
		GLuint * depth_rb,
		GLenum   format = GL_RGBA,
		GLenum   type   = GL_UNSIGNED_BYTE,
		GLenum   target = GL_TEXTURE_2D);
	void generate_raycastcube_vao(GLuint*, GLuint*, GLuint*, GLuint*);
	void generate_raycast_shader_vao(GLuint*, GLuint*, GLuint*);
	bool update_raycast_shader_vbo(
		unsigned int, float, float, float, GLuint*, bool);
	void raycast_cube(float, float, float, GLuint*, bool);
	void raycast_cube_RIP(float, float, float, GLuint*, bool);
	void raycast_cube_LIP(float, float, float, GLuint*, bool);
	void raycast_cube_RSP(float, float, float, GLuint*, bool);
	void raycast_cube_LSP(float, float, float, GLuint*, bool);
	void raycast_cube_RIA(float, float, float, GLuint*, bool);
	void raycast_cube_LIA(float, float, float, GLuint*, bool);
	void raycast_cube_RSA(float, float, float, GLuint*, bool);
	void raycast_cube_LSA(float, float, float, GLuint*, bool);
	void raycast_cube_IRP(float, float, float, GLuint*, bool);
	void raycast_cube_ILP(float, float, float, GLuint*, bool);
	void raycast_cube_SRP(float, float, float, GLuint*, bool);
	void raycast_cube_SLP(float, float, float, GLuint*, bool);
	void raycast_cube_IRA(float, float, float, GLuint*, bool);
	void raycast_cube_ILA(float, float, float, GLuint*, bool);
	void raycast_cube_SRA(float, float, float, GLuint*, bool);
	void raycast_cube_SLA(float, float, float, GLuint*, bool);
	void raycast_cube_RPI(float, float, float, GLuint*, bool);
	void raycast_cube_LPI(float, float, float, GLuint*, bool);
	void raycast_cube_RAI(float, float, float, GLuint*, bool);
	void raycast_cube_LAI(float, float, float, GLuint*, bool);
	void raycast_cube_RPS(float, float, float, GLuint*, bool);
	void raycast_cube_LPS(float, float, float, GLuint*, bool);
	void raycast_cube_RAS(float, float, float, GLuint*, bool);
	void raycast_cube_LAS(float, float, float, GLuint*, bool);
	void raycast_cube_PRI(float, float, float, GLuint*, bool);
	void raycast_cube_PLI(float, float, float, GLuint*, bool);
	void raycast_cube_ARI(float, float, float, GLuint*, bool);
	void raycast_cube_ALI(float, float, float, GLuint*, bool);
	void raycast_cube_PRS(float, float, float, GLuint*, bool);
	void raycast_cube_PLS(float, float, float, GLuint*, bool);
	void raycast_cube_ARS(float, float, float, GLuint*, bool);
	void raycast_cube_ALS(float, float, float, GLuint*, bool);
	void raycast_cube_IPR(float, float, float, GLuint*, bool);
	void raycast_cube_SPR(float, float, float, GLuint*, bool);
	void raycast_cube_IAR(float, float, float, GLuint*, bool);
	void raycast_cube_SAR(float, float, float, GLuint*, bool);
	void raycast_cube_IPL(float, float, float, GLuint*, bool);
	void raycast_cube_SPL(float, float, float, GLuint*, bool);
	void raycast_cube_IAL(float, float, float, GLuint*, bool);
	void raycast_cube_SAL(float, float, float, GLuint*, bool);
	void raycast_cube_PIR(float, float, float, GLuint*, bool);
	void raycast_cube_PSR(float, float, float, GLuint*, bool);
	void raycast_cube_AIR(float, float, float, GLuint*, bool);
	void raycast_cube_ASR(float, float, float, GLuint*, bool);
	void raycast_cube_PIL(float, float, float, GLuint*, bool);
	void raycast_cube_PSL(float, float, float, GLuint*, bool);
	void raycast_cube_AIL(float, float, float, GLuint*, bool);
	void raycast_cube_ASL(float, float, float, GLuint*, bool);
	void generateAnisoTexture(GLuint*, const unsigned int, const unsigned int);
	void d_orientcube(
		qMeshData  * ,
			void  * ,
			int   * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			void  *);
	void d_mesh(
		qMeshData  * ,
			void  * ,
			int   * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			float * ,
			void  *);

public slots:
	void set_ortho(bool);
	void set_fov(double);
	void set_far(double);
	void set_draw_frames_3d(bool);
	void set_display_contours(bool);
	void set_skip_draw(bool);
	void set_alpha(double);
	void set_brightness(double);
	void set_cube(bool);
	void set_contours_width(float);

signals:
	void opengl3_not_available();

protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int, int) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	void wheelEvent(QWheelEvent*) override;
	void keyPressEvent(QKeyEvent*) override;
	QPoint lastPos;
	QPoint lastPanPos;
	QPoint lastPosScale;

private:
	void disable_gl_in_settings();
} VECTORMATH_ALIGNED_POST;

#endif
