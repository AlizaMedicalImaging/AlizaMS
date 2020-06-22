#ifndef GLWIDGET_H
#define GLWIDGET_H

#include "camera.h"
#include <QOpenGLWidget>
//#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_0>
#include <QPoint>
#include <QModelIndex>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QColor>
#include "structures.h"
#include <QString>
#include "btBulletCollisionCommon.h"

#define SCENE_ORTHO_SIZE 150
#define SCENE_POS_Z 400
#define SCENE_FOV 45
#define SCENE_FAR_PLANE 10000
#define SCENE_ALPHA 0.55

#define VBOIDS_SIZE 4
#define TEXTURES_SIZE 8
#define MAX_SHADOWS_ 1

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
	int	   * get_shadows;
	int	   * cast_shadows;
	GLuint * textures;
	GLuint * tex_units;
	ShaderObj * shader;
};

#include "btBulletCollisionCommon.h"
ALIGN16_PRE class CollisionObject
{
public:

	MY_DECLARE_NEW()

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
	unsigned int		id;
	btCollisionShape  * shape;
	qMeshData		  * mesh_data;
} ALIGN16_POST;

ALIGN16_PRE
class GLWidget : public QOpenGLWidget, public QOpenGLFunctions_3_0
{

	Q_OBJECT

public:

	MY_DECLARE_NEW()

	GLWidget(QWidget(*)=NULL, Qt::WindowFlags=0);
	~GLWidget();
	void updateGL() { makeCurrent(); update(); }
	void init_();
	void close_();
	void zoom_in();
	void zoom_out();
	void update_clear_color();
	static long long get_count_vbos();
	static void increment_count_vbos(long long);
	static long long get_count_tex();
	static void increment_count_tex(long long);
	static void set_max_vbos_65535(bool);
	static bool get_max_vbos_65535();
	static void set_max_tex_65535(bool);
	static bool get_max_tex_65535();
	bool skip_draw;
	int  max_tex_size;
	int  max_3d_texture_size;
	bool opengl_init_done;
	bool gallium;
	void set_left();
	void set_right();
	void set_forward();
	void set_backward();
	QString get_system_info() const;
	void  close();
	float mparams[16];
	float sparams[9];
	btCollisionWorld * m_collisionWorld;
	void init_physics(btCollisionWorld*);
	void send_ray0(float x, float y);
	btVector3 get_ray_to(float x, float y);
	btVector3 get_ray_from(float x, float y);
	void  init_opengl(int,int);
	void  paint();
	void  paint_volume();
	void  paint_raycaster();
	void  resize(int, int);
	int   normalize_angle(int);
	int   x_rotation, y_rotation, z_rotation;
	float ortho_size;
	double position_z;
	void draw_3d_tex2(const GLfloat*,const GLfloat*,const ShaderObj&);
	void draw_frame(const GLuint*);
	void draw_frame2(const GLfloat*);
	void draw_lines(unsigned long, const GLuint*);
	void draw_points(unsigned long, const GLuint*);
	int win_w, win_h;
	Camera * camera;
	bool   no_opengl3;
	bool   trilinear;
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
#if 0
	void load_lut_tex(const QString&, GLuint*);
#endif
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
	void set_selected_images_ptr(Scene3DImages*);
	void set_view_3d();
	void set_view_rc();
	bool rect_selection;
	bool show_cube;
	bool gpu_shader4;
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
    GLuint frames_vbo;
    GLuint slice_v_vbo;
    GLuint slice_t_vbo;
	GLuint origin_vbo;
    ShaderObj fsquad_shader;
	ShaderObj zero_shader;
	ShaderObj raycast_shader;
	ShaderObj raycast_color_shader;
	ShaderObj raycast_shader_bb;
	ShaderObj raycast_color_shader_bb;
	ShaderObj c3d_shader_clamp;
	ShaderObj c3d_shader_gradient_clamp;
	ShaderObj c3d_shader_bb_clamp;
	ShaderObj c3d_shader_gradient_bb_clamp;
	ShaderObj c3d_shader;
	ShaderObj c3d_shader_gradient;
	ShaderObj c3d_shader_bb;
	ShaderObj c3d_shader_gradient_bb;
	ShaderObj raycast_shader_sigm;
	ShaderObj raycast_color_shader_sigm;
	ShaderObj raycast_shader_bb_sigm;
	ShaderObj raycast_color_shader_bb_sigm;
	ShaderObj c3d_shader_clamp_sigm;
	ShaderObj c3d_shader_gradient_clamp_sigm;
	ShaderObj c3d_shader_bb_clamp_sigm;
	ShaderObj c3d_shader_gradient_bb_clamp_sigm;
	ShaderObj c3d_shader_sigm;
	ShaderObj c3d_shader_gradient_sigm;
	ShaderObj c3d_shader_bb_sigm;
	ShaderObj c3d_shader_gradient_bb_sigm;
	ShaderObj frame_shader;
	ShaderObj simple_tex_shader;
	ShaderObj sphere0_shader;
	ShaderObj color_shader;
	ShaderObj orientcube_shader;
	ShaderObj mesh_shader;
	GLuint raycastcube0[2];
	GLuint raycastcube1[2];
	GLuint raycastcube2[2];
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

	void generate_quad(
		GLfloat*, GLfloat*, GLfloat*, GLfloat*, GLuint*);
	GLuint load_shader(GLenum, const char*);
	bool create_program(
		const char*, const char*, ShaderObj*, bool=false);
	void checkGLerror(const char*);
	void makeModelVBO_ArraysT(GLuint*,
		 const float*, const float*, const float*, const float*,
		 const unsigned int*, const int,
		 GLenum=GL_STATIC_DRAW,
		 float=1.0f);
	void generate_point_vbo(
		GLuint*,
		const float, const float, const float);
	void generate_screen_quad(unsigned int*);
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
	void raycast_cube(bool generate, float x, float y, float z,
		GLuint * vboid0, GLuint * vboid1, GLuint * vboid2);
  	void raycast_cube_RIP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LIP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_RSP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LSP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_RIA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LIA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_RSA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LSA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_IRP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ILP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SRP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SLP(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_IRA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ILA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SRA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SLA(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_RPI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LPI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_RAI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LAI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_RPS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LPS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_RAS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_LAS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PRI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PLI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ARI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ALI(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PRS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PLS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ARS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ALS(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_IPR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SPR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_IAR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SAR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_IPL(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SPL(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_IAL(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_SAL(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PIR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PSR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_AIR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ASR(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PIL(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_PSL(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_AIL(float, float, float, GLuint*, GLuint*, GLuint*);
	void raycast_cube_ASL(float, float, float, GLuint*, GLuint*, GLuint*);
	void generateAnisoTexture(GLuint*, const unsigned int, const unsigned int);
	void d_orientcube(
		void  * ,
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
	void get_screen();
	void set_contours_width(float);

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int, int);
	void mousePressEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void wheelEvent(QWheelEvent*);
	void keyPressEvent(QKeyEvent*);
	QPoint lastPos;
	QPoint lastPanPos;
	QPoint lastPosScale;
}
ALIGN16_POST
;

#endif
