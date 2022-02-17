#include <cstdlib>
#include "camera.h"

#ifdef DISABLE_SIMDMATH
using namespace Vectormath::Scalar;
#else
using namespace Vectormath::SSE;
#endif

Camera::Camera()
{
	reset();
}

Camera::~Camera() {}

#ifdef DISABLE_SIMDMATH
void Camera::matrix4_to_float(const Matrix4 & m_aos, float * m)
{
	m[ 0] = m_aos.getElem(0,0);
	m[ 1] = m_aos.getElem(0,1);
	m[ 2] = m_aos.getElem(0,2);
	m[ 3] = m_aos.getElem(0,3);
	m[ 4] = m_aos.getElem(1,0);
	m[ 5] = m_aos.getElem(1,1);
	m[ 6] = m_aos.getElem(1,2);
	m[ 7] = m_aos.getElem(1,3);
	m[ 8] = m_aos.getElem(2,0);
	m[ 9] = m_aos.getElem(2,1);
	m[10] = m_aos.getElem(2,2);
	m[11] = m_aos.getElem(2,3);
	m[12] = m_aos.getElem(3,0);
	m[13] = m_aos.getElem(3,1);
	m[14] = m_aos.getElem(3,2);
	m[15] = m_aos.getElem(3,3);
}
#endif

void Camera::matrix3_to_float(const Matrix3 & m_aos, float * m)
{
	m[0] = m_aos.getElem(0,0);
	m[1] = m_aos.getElem(0,1);
	m[2] = m_aos.getElem(0,2);
	m[3] = m_aos.getElem(1,0);
	m[4] = m_aos.getElem(1,1);
	m[5] = m_aos.getElem(1,2);
	m[6] = m_aos.getElem(2,0);
	m[7] = m_aos.getElem(2,1);
	m[8] = m_aos.getElem(2,2);
}

void Camera::float_to_matrix4(const float * m, Matrix4 & m_aos)
{
	m_aos.setCol(0, Vector4(m[ 0], m[ 1], m[ 2], m[ 3]));
	m_aos.setCol(1, Vector4(m[ 4], m[ 5], m[ 6], m[ 7]));
	m_aos.setCol(2, Vector4(m[ 8], m[ 9], m[10], m[11]));
	m_aos.setCol(3, Vector4(m[12], m[13], m[14], m[15]));
}

void Camera::float_to_matrix3(const float * m, Matrix3 & m_aos)
{
	m_aos.setCol(0, Vector3(m[0], m[1], m[2]));
	m_aos.setCol(1, Vector3(m[3], m[4], m[5]));
	m_aos.setCol(2, Vector3(m[6], m[7], m[8]));
}

void Camera::reset()
{
	m_position = Point3(0.0f,0.0f,1.0f);
	m_direction_vector = Vector3(0.0f,0.0f,-1.0f);
	m_up_axis = Vector3(0.0f,1.0f,0.0f);
	m_view = Matrix4::identity();
	m_projection = Matrix4::identity();
	m_inv_view = Matrix4::identity();
	m_trackball = Quat::identity();
#ifdef ENABLE_INVTRANS
	m_inv_view = Matrix4::identity();
#endif
	m_heading = 0.0f;
	m_pitch = 0.0f;
	m_forward_vel = 0.0f;
	m_side_vel = 0.0f;
	for (unsigned int x = 0; x < MAX_SHADOWS; x++)
	{
		m_light_mvp_scaled_biased[x] = Matrix4::identity();
		m_light_modelview[x] = Matrix4::identity();
		m_light_projection[x] = Matrix4::identity();
	}
	m_scale_and_bias.setElem(0,0,0.5f);
	m_scale_and_bias.setElem(0,1,0.0f);
	m_scale_and_bias.setElem(0,2,0.0f);
	m_scale_and_bias.setElem(0,3,0.0f);
	m_scale_and_bias.setElem(1,0,0.0f);
	m_scale_and_bias.setElem(1,1,0.5f);
	m_scale_and_bias.setElem(1,2,0.0f);
	m_scale_and_bias.setElem(1,3,0.0f);
	m_scale_and_bias.setElem(2,0,0.0f);
	m_scale_and_bias.setElem(2,1,0.0f);
	m_scale_and_bias.setElem(2,2,0.5f);
	m_scale_and_bias.setElem(2,3,0.0f);
	m_scale_and_bias.setElem(3,0,0.5f);
	m_scale_and_bias.setElem(3,1,0.5f);
	m_scale_and_bias.setElem(3,2,0.5f);
	m_scale_and_bias.setElem(3,3,1.0f);
}

void Camera::look_at(
	float camera_positionX,
	float camera_positionY,
	float camera_positionZ,
	float camera_targetX,
	float camera_targetY,
	float camera_targetZ,
	float upAxisX,
	float upAxisY,
	float upAxisZ,
	bool calculate_direction)
{
	// calculate direction, position, up axis, view matrix
	m_position =
		Point3(camera_positionX, camera_positionY, camera_positionZ);
	const Point3 camera_target =
		Point3(camera_targetX, camera_targetY, camera_targetZ);
	m_up_axis = Vector3(upAxisX, upAxisY, upAxisZ);
	m_view = Matrix4::lookAt(m_position, camera_target, m_up_axis);

	if (calculate_direction)
		m_direction_vector = normalize(camera_target - m_position);

#ifdef ENABLE_INVTRANS
	// calculate inverse of view matrix
	const Matrix3 mbasis = m_view.getUpper3x3();
	const Matrix3 mbasis_inverse = inverse(mbasis);
	m_inv_view = Matrix4::identity();
	m_inv_view.setUpper3x3(mbasis_inverse);
#endif
}

void Camera::perspective(
	float _fov_radians,
	float _aspect,
	float _near,
	float _far)
{
	m_projection =
		Matrix4::perspective(_fov_radians, _aspect, _near, _far);
}

void Camera::orthographic(
	float _left,
	float _right,
	float _bottom,
	float _top,
	float _near,
	float _far)
{
	m_projection =
		Matrix4::orthographic(
			_left,
			_right,
			_bottom,
			_top,
			_near,
			_far);
}

void Camera::project(
	Point3 & pos,
	float * viewport,
	float * winX,
	float * winY,
	float * winZ)
{
	const Matrix4 m = m_projection*m_view;
	const Vector4 v =
		m.getCol0()*pos.getX()+
		m.getCol1()*pos.getY()+
		m.getCol2()*pos.getZ()+
		m.getCol3();
	*winX = viewport[0] + viewport[2]*(v.getX()+1.0f)/2.0f;
	*winY = viewport[1] + viewport[3]*(v.getY()+1.0f)/2.0f;
	*winZ = (v.getZ()+1.0f)/2.0f;
}

void Camera::project_compat(
	Point3 & pos_aos,
	int * viewport,
	float * win_coord0,
	float * win_coord1)
{
	// reimplentation of gluProject,
	// works well with perspective projection
	Vector4 pos = Vector4(
		pos_aos.getX(),
		pos_aos.getY(),
		pos_aos.getZ(),
		1.0f);
	Vector4 tmp0 = m_view*pos;
	Vector4 tmp1 =
		m_projection.getCol0()*tmp0.getX() +
		m_projection.getCol1()*tmp0.getY() +
		m_projection.getCol2()*tmp0.getZ() +
		m_projection.getCol3()*tmp0.getW();
	if (tmp0.getZ() == 0.0f) return;
	float tmp3 = -1.0f/tmp0.getZ();
	tmp1[0] *= tmp3;
	tmp1[1] *= tmp3;
	// !! normalized
	*win_coord0 =
		((tmp1.getX()*0.5f+0.5f)*viewport[2]+viewport[0])
			/viewport[2];
	*win_coord1 =
		((tmp1.getY()*0.5f+0.5f)*viewport[3]+viewport[1])
			/viewport[3];
	//
	// This is only correct when glDepthRange(0.0, 1.0)
	// tmp1[2] *= tmp3;
	// *win_coord2=(1.0f+tmp1[2])*0.5f;
}

Vector4 Camera::unproject(
	Point3  & pos,
	Matrix4 & modelview,
	Matrix4 & projection,
	Vector4 & viewport)
{
	// caution : correct Y might be (height-Y)
	const Matrix4 mvp_inv = inverse(projection*modelview);
	Vector4 tmp;
	tmp[0] = 2.0f*(pos.getX() - viewport.getX())/viewport.getZ() - 1.0f;
	tmp[1] = 2.0f*(pos.getY() - viewport.getY())/viewport.getW() - 1.0f;
	tmp[2] = 2.0f*pos.getZ() - 1.0f;
	tmp[3] = 1.0f;
	return (mvp_inv*tmp);
}

float Camera::project_to_sphere(float radius, float x, float y)
{
	float result;
	const float tmp0 = sqrt(x*x + y*y);
	// inside sphere or on hyperbola
	if (tmp0 < radius * 0.70710678118654752440f)
	{
		result = sqrt(radius*radius - tmp0*tmp0);
	}
	else
	{
		const float tmp1 = radius / 1.41421356237309504880f;
		result = tmp1*tmp1 / tmp0;
	}
	return result;
}

void Camera::set_trackball(
	float radius,
	float p0_x, float p0_y,
	float p1_x, float p1_y)
{
	if ((p0_x!=p1_x)||(p0_y!=p1_y))
	{
		const Vector3 p0 = Vector3(
			p0_x, p0_y,project_to_sphere(radius,p0_x, p0_y));
		const Vector3 p1 = Vector3(
			p1_x, p1_y,project_to_sphere(radius,p1_x, p1_y));
		const Vector3 axis = normalize(cross(p0,p1));
		const Vector3 d = p0 - p1;
		float t = length(d) / (2.0f*radius);
		if (t >  1.0f) t =  1.0f;
		if (t < -1.0f) t = -1.0f;
		const float phi = 2.0f * asin(t);
		const Quat q = Quat(-axis*sin(phi/2.0f), cos(phi/2.0f));
		m_trackball *= q;
		m_trackball = normalize(m_trackball);
	}
}

void Camera::set_trackball_matrix(
	float radius, float p0_x, float p0_y,float p1_x, float p1_y,
	float center_x, float center_y, float center_z,
	float pos_x, float pos_y, float pos_z,
	float up_x, float up_y, float up_z)
{
	Vector3 pos = Vector3(pos_x, pos_y, pos_z);
	Vector3 up_axis = Vector3(up_x, up_y, up_z);
	const Vector3 center = Vector3(center_x, center_y, center_z);
	if ((p0_x!=p1_x)||(p0_y!=p1_y))
	{
		const Vector3 p0 = Vector3(
			p0_x, p0_y,project_to_sphere(radius,p0_x, p0_y));
		const Vector3 p1 = Vector3(
			p1_x, p1_y,project_to_sphere(radius,p1_x, p1_y));
		const Vector3 axis = normalize(cross(p0,p1));
		const Vector3 d = p0 - p1;
		float t = length(d) / (2.0f*radius);
		if (t >  1.0f) t =  1.0f;
		if (t < -1.0f) t = -1.0f;
		const float phi = 2.0f * asin(t);
		const Quat q = Quat(-axis*sin(phi/2.0f), cos(phi/2.0f));
		m_trackball *= q;
		m_trackball = normalize(m_trackball);
	}
	pos = center + rotate(m_trackball,pos);
	up_axis = rotate(m_trackball,up_axis);
	look_at(pos.getX(),pos.getY(),pos.getZ(),
			center_x, center_y, center_z,
			up_axis.getX(),up_axis.getY(),up_axis.getZ());
}

void Camera::set_trackball_pan_matrix(
	float radius, float p0_x, float p0_y,float p1_x, float p1_y,
	float center_x, float center_y, float center_z,
	float pos_x, float pos_y, float pos_z,
	float up_x, float up_y, float up_z,
	float dx, float dy)
{
	Vector3 pos = Vector3(pos_x, pos_y, pos_z);
	Vector3 up_axis = Vector3(up_x, up_y, up_z);
	Vector3 center = Vector3(center_x, center_y, center_z);
	if ((p0_x!=p1_x)||(p0_y!=p1_y))
	{
		const Vector3 p0 = Vector3(
			p0_x, p0_y,project_to_sphere(radius,p0_x, p0_y));
		const Vector3 p1 = Vector3(
			p1_x, p1_y,project_to_sphere(radius,p1_x, p1_y));
		const Vector3 axis = normalize(cross(p0,p1));
		const Vector3 d = p0 - p1;
		float t = length(d) / (2.0f*radius);
		if (t >  1.0f) t =  1.0f;
		if (t < -1.0f) t = -1.0f;
		const float phi = 2.0f * asin(t);
		const Quat q = Quat(-axis*sin(phi/2.0f), cos(phi/2.0f));
		m_trackball *= q;
		m_trackball = normalize(m_trackball);
	}
	up_axis = rotate(m_trackball,up_axis);
	pos = rotate(m_trackball,pos);
	const Vector3 right = cross(normalize(pos),up_axis);
	center = center + right*dx + up_axis*dy;
	pos = center + pos;
	look_at(pos.getX(),pos.getY(),pos.getZ(),
			center.getX(), center.getY(), center.getZ(),
			up_axis.getX(),up_axis.getY(),up_axis.getZ());
}

void Camera::set_trackball_pan_matrix2(
	float radius, float p0_x, float p0_y,float p1_x, float p1_y,
	float center_x, float center_y, float center_z,
	float pos_x, float pos_y, float pos_z,
	float up_x, float up_y, float up_z,
	float dx, float dy,
	float * light_x, float * light_y, float * light_z)
{
	Vector3 pos = Vector3(pos_x, pos_y, pos_z);
	Vector3 up_axis = Vector3(up_x, up_y, up_z);
	Vector3 center = Vector3(center_x, center_y, center_z);
	if ((p0_x!=p1_x)||(p0_y!=p1_y))
	{
		const Vector3 p0 = Vector3(
			p0_x, p0_y,project_to_sphere(radius,p0_x, p0_y));
		const Vector3 p1 = Vector3(
			p1_x, p1_y,project_to_sphere(radius,p1_x, p1_y));
		const Vector3 axis = normalize(cross(p0,p1));
		const Vector3 d = p0 - p1;
		float t = length(d) / (2.0f*radius);
		if (t >  1.0f) t =  1.0f;
		if (t < -1.0f) t = -1.0f;
		const float phi = 2.0f * asin(t);
		const Quat q = Quat(-axis*sin(phi/2.0f), cos(phi/2.0f));
		m_trackball *= q;
		m_trackball = normalize(m_trackball);
	}
	up_axis = rotate(m_trackball,up_axis);
	pos = rotate(m_trackball,pos);
	const Vector3 right = cross(normalize(pos),up_axis);
	center = center + right*dx + up_axis*dy;
	pos = center + pos;
	const Vector3 light = pos - center;
	*light_x = light.getX();
	*light_y = light.getY();
	*light_z = light.getZ();
	look_at(pos.getX(),pos.getY(),pos.getZ(),
			center.getX(), center.getY(), center.getZ(),
			up_axis.getX(),up_axis.getY(),up_axis.getZ());
}

void Camera::change_heading(float radians)
{
	m_heading += radians;
	if (m_heading > 6.283185307179586476925286766559f)
	{
		m_heading -= 6.283185307179586476925286766559f;
	}
	else if (m_heading < -6.283185307179586476925286766559f)
	{
		m_heading += 6.283185307179586476925286766559f;
	}
}

void Camera::set_heading(float radians)
{
	m_heading = radians;
	if (m_heading > 6.283185307179586476925286766559f)
	{
		m_heading -= 6.283185307179586476925286766559f;
	}
	else if (m_heading < -6.283185307179586476925286766559f)
	{
		m_heading += 6.283185307179586476925286766559f;
	}
}

void Camera::change_pitch(float radians)
{
	m_pitch += radians;
	if (m_pitch > 6.283185307179586476925286766559f)
	{
		m_pitch -= 6.283185307179586476925286766559f;
	}
	else if (m_pitch < -6.283185307179586476925286766559f)
	{
		m_pitch += 6.283185307179586476925286766559f;
	}
}

void Camera::set_pitch(float radians)
{
	m_pitch = radians;
	if (m_pitch > 6.283185307179586476925286766559f)
	{
		m_pitch -= 6.283185307179586476925286766559f;
	}
	else if (m_pitch < -6.283185307179586476925286766559f)
	{
		m_pitch += 6.283185307179586476925286766559f;
	}
}

void Camera::set_velocity(float vel)
{
	m_forward_vel = vel;
}

void Camera::set_side_velocity(float side_vel)
{
	m_side_vel = side_vel;
}

void Camera::set_viewer_matrix(float elapsed_time)
{
	// calculate direction
	const Vector3 axis_head = Vector3(0.0f,1.0f,0.0f);
	const Matrix4 mhead = mhead.rotation(m_heading,axis_head);

	const Vector3 axis_pitch = Vector3(1.0f,0.0f,0.0f);
	const Matrix4 mpitch = mpitch.rotation(m_pitch,axis_pitch);

	const Matrix4 m_get_direction = mhead * mpitch;
	m_direction_vector = Vector3(
		m_get_direction.getElem(2,0),
		m_get_direction.getElem(2,1),
		-m_get_direction.getElem(2,2));
	m_direction_vector = normalize(m_direction_vector);

	// calculate position, up axis, view matrix
	m_up_axis = m_get_direction.getUpper3x3().getRow(1);
	m_position += m_forward_vel*elapsed_time*m_direction_vector;
	m_position[0] +=
		-1.0f*m_direction_vector.getZ()*m_side_vel*elapsed_time;
	m_position[1] += 0.0f;
	m_position[2] += m_direction_vector.getX()*m_side_vel*elapsed_time;

	const Matrix4 mres = mpitch * mhead;

	Matrix4 mtransl = Matrix4::identity();
	Vector4 c3 = Vector4(
		-m_position.getX(),
		-m_position.getY(),
		-m_position.getZ(),
		1.0f);
	mtransl.setCol(3, c3);

	m_view = mres * mtransl;

#ifdef ENABLE_INVTRANS
	// calculate inverse of view matrix
	const Matrix3 mbasis = m_view.getUpper3x3();
	const Matrix3 mbasis_inverse = inverse(mbasis);
	m_inv_view = Matrix4::identity();
	m_inv_view.setUpper3x3(mbasis_inverse);
#endif
}

void Camera::set_position(float x, float y, float z)
{
	m_position = Point3(x,y,z);
}

// proj_matrix_type is
// 0 - ortho
// 1 - frustum
// 2 - perspective
void Camera::calculate_light_matrices(
	unsigned short id,
	float * light_params,
	int proj_matrix_type)
{
	// calculate light projection and modelview matrices
	// and their scaled and biased product
	if (!light_params) return;
	const float eye_x = light_params[0];
	const float eye_y = light_params[1];
	const float eye_z = light_params[2];
	const float target_x = light_params[3];
	const float target_y = light_params[4];
	const float target_z = light_params[5];
	const float light_up_x = light_params[6];
	const float light_up_y = light_params[7];
	const float light_up_z = light_params[8];
	const float light_box_hw = light_params[9];
	const float light_box_hh = light_params[10];
	const float _near = light_params[11];
	const float _far = light_params[12];
	const float _fov_radians = light_params[13];
	const float _aspect = light_params[14];

	const Point3 target = Point3(target_x, target_y, target_z);
	const Point3 eye = Point3(eye_x, eye_y, eye_z);

	if (proj_matrix_type == 0) // orthographic
	{
		m_light_projection[id] =
			Matrix4::orthographic(
				-light_box_hw,
				light_box_hw,
				-light_box_hh,
				light_box_hh,
				_near,
				_far);
	}
	else if (proj_matrix_type == 1) // frustum
	{
		m_light_projection[id] =
			Matrix4::frustum(
				-light_box_hw,
				light_box_hw,
				-light_box_hh,
				light_box_hh,
				_near,
				_far);
	}
	else if (proj_matrix_type == 2) // perspective
	{
		m_light_projection[id] =
			Matrix4::perspective(
				_fov_radians,
				_aspect,
				_near,
				_far);
	}
	else
	{
		m_light_projection[id] = Matrix4::identity();
	}

	const Vector3 light_up_axis =
		Vector3(light_up_x,light_up_y,light_up_z);
	m_light_modelview[id] =
		Matrix4::lookAt(eye,target,light_up_axis);
	m_light_mvp_scaled_biased[id] =
		m_scale_and_bias*m_light_projection[id]*m_light_modelview[id];
}

void Camera::calculate_projective_matrix(
	unsigned short id,
	float * projective_matrix,
	float * modeling)
{
	// calculate projective matrix =
	// light mvp scaled and biased * object modeling matrix
#if 1
	// to silence Coverity warning
	Matrix4 modeling_aos = Matrix4::identity();
#else
	Matrix4 modeling_aos;
#endif
	float_to_matrix4(modeling, modeling_aos);
	const Matrix4 projective_matrix_aos =
		m_light_mvp_scaled_biased[id]*modeling_aos;
	matrix4_to_float(projective_matrix_aos, projective_matrix);
}
