/*
 * (C) mihail.isakov@googlemail.com 2008-2022
 */

#ifndef A_CAMERA_H
#define A_CAMERA_H

////////////////////////////////
//
//
#define CAMERA_MAX_SHADOWS 1
#define CAMERA_ENABLE_INVTRANS
//
//
////////////////////////////////

#define CAMERA_PI  3.1415926535897932384626433832795f
#define CAMERA_2PI 6.283185307179586476925286766559f
#define CAMERA_D2R 0.0174532925199432957692369f
#define CAMERA_R2D 57.2957795130823208767981548f

#ifdef DISABLE_SIMDMATH
#include "vectormath/scalar/vectormath.h"
#define VECTORMATH_ALIGNED(a) a
#define VECTORMATH_ALIGNED_PRE
#define VECTORMATH_ALIGNED_POST
#define VECTORMATH_ALIGNED16_NEW()
#else
#include "vectormath/sse/vectormath.h"
#endif

VECTORMATH_ALIGNED_PRE class Camera
{
public:
	VECTORMATH_ALIGNED16_NEW();

	Camera();
	~Camera();
#ifdef DISABLE_SIMDMATH
	Vectormath::Scalar::Point3 m_position;
	Vectormath::Scalar::Vector3 m_direction_vector;
	Vectormath::Scalar::Vector3 m_up_axis;
	Vectormath::Scalar::Matrix4 m_view;
	Vectormath::Scalar::Matrix4 m_projection;
	Vectormath::Scalar::Matrix4 m_inv_view;
	Vectormath::Scalar::Quat m_trackball;
	Vectormath::Scalar::Matrix4 m_light_mvp_scaled_biased[CAMERA_MAX_SHADOWS];
	Vectormath::Scalar::Matrix4 m_light_modelview[CAMERA_MAX_SHADOWS];
	Vectormath::Scalar::Matrix4 m_light_projection[CAMERA_MAX_SHADOWS];
	Vectormath::Scalar::Matrix4 m_scale_and_bias;
#else
	Vectormath::SSE::Point3 m_position;
	Vectormath::SSE::Vector3 m_direction_vector;
	Vectormath::SSE::Vector3 m_up_axis;
	Vectormath::SSE::Matrix4 m_view;
	Vectormath::SSE::Matrix4 m_projection;
	Vectormath::SSE::Matrix4 m_inv_view;
	Vectormath::SSE::Quat m_trackball;
	Vectormath::SSE::Matrix4 m_light_mvp_scaled_biased[CAMERA_MAX_SHADOWS];
	Vectormath::SSE::Matrix4 m_light_modelview[CAMERA_MAX_SHADOWS];
	Vectormath::SSE::Matrix4 m_light_projection[CAMERA_MAX_SHADOWS];
	Vectormath::SSE::Matrix4 m_scale_and_bias;
#endif
	float m_heading;
	float m_pitch;
	float m_forward_vel;
	float m_side_vel;
#ifdef DISABLE_SIMDMATH
	void matrix4_to_float(const Vectormath::Scalar::Matrix4&,float*);
	void matrix3_to_float(const Vectormath::Scalar::Matrix3&,float*);
	void float_to_matrix4(const float*,Vectormath::Scalar::Matrix4&);
	void float_to_matrix3(const float*,Vectormath::Scalar::Matrix3&);
#else
	inline void matrix4_to_float(
		const Vectormath::SSE::Matrix4 & m_,
		float * m)
	{
		_mm_storeu_ps((m   ), m_.getCol0().get128());
		_mm_storeu_ps((m+4 ), m_.getCol1().get128());
		_mm_storeu_ps((m+8 ), m_.getCol2().get128());
		_mm_storeu_ps((m+12), m_.getCol3().get128());
	}
	void matrix3_to_float(const Vectormath::SSE::Matrix3&, float*);
	void float_to_matrix4(const float*,Vectormath::SSE::Matrix4&);
	void float_to_matrix3(const float*,Vectormath::SSE::Matrix3&);
#endif
	void reset();
	void look_at(
		float,float,float,float,
		float,float,float,float,
		float,bool=true);
	void perspective(
		float,float,float,float);
	void orthographic(
		float,float,float,float,
		float,float);
#ifdef DISABLE_SIMDMATH
	void project(
		Vectormath::Scalar::Point3&,
		float*,float*,float*,float*);
	void project_compat(
		Vectormath::Scalar::Point3&,int*,float*,float*);
	Vectormath::Scalar::Vector4 unproject(
		Vectormath::Scalar::Point3&,
		Vectormath::Scalar::Matrix4&,
		Vectormath::Scalar::Matrix4&,
		Vectormath::Scalar::Vector4&);
#else
	void project(
		Vectormath::SSE::Point3&,
		float*,float*,float*,float*);
	void project_compat(
		Vectormath::SSE::Point3&,int*,float*,float*);
	Vectormath::SSE::Vector4 unproject(
		Vectormath::SSE::Point3&,
		Vectormath::SSE::Matrix4&,
		Vectormath::SSE::Matrix4&,
		Vectormath::SSE::Vector4&);
#endif
	float project_to_sphere(
		float,float,float);
	void set_trackball(
		float,float,float,float,float);
	void set_trackball_matrix(
		float,float,float,float, float,float,
		float,float,float,float,
		float,float,float,float);
	void set_trackball_pan_matrix(
		float,float,float,float, float,float,
		float,float,float,float,
		float,float,float,float,
		float,float);
	void set_trackball_pan_matrix2(
		float,float,float,float, float,float,
		float,float,float,float,
		float,float,float,float,
		float,float,
		float*,float*,float*);
	void change_heading(float);
	void set_heading(float);
	void change_pitch(float);
	void set_pitch(float);
	void set_velocity(float);
	void set_side_velocity(float);
	void set_viewer_matrix(float);
	void set_position(float,float,float);
	void calculate_light_matrices(
		unsigned short,float*,int);
	void calculate_projective_matrix(
		unsigned short,float*,float*);
} VECTORMATH_ALIGNED_POST;

#endif

