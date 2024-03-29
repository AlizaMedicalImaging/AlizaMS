/*
   Copyright (C) 2006, 2007 Sony Computer Entertainment Inc.
   All rights reserved.

   Redistribution and use in source and binary forms,
   with or without modification, are permitted provided that the
   following conditions are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the Sony Computer Entertainment Inc nor the names
  of its contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef VECTORMATH_SSE_MATRIX_HPP
#define VECTORMATH_SSE_MATRIX_HPP

namespace Vectormath
{
namespace SSE
{

// ========================================================
// Matrix3
// ========================================================

inline Matrix3::Matrix3(const Matrix3 & mat)
{
  mCol0 = mat.mCol0;
  mCol1 = mat.mCol1;
  mCol2 = mat.mCol2;
}

inline Matrix3::Matrix3(float scalar)
{
  mCol0 = Vector3(scalar);
  mCol1 = Vector3(scalar);
  mCol2 = Vector3(scalar);
}

inline Matrix3::Matrix3(const FloatInVec & scalar)
{
  mCol0 = Vector3(scalar);
  mCol1 = Vector3(scalar);
  mCol2 = Vector3(scalar);
}

inline Matrix3::Matrix3(const Quat & unitQuat)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 xyzw_2 = _mm_add_ps(unitQuat.get128(), unitQuat.get128());
  const __m128 wwww = _mm_shuffle_ps(unitQuat.get128(), unitQuat.get128(), _MM_SHUFFLE(3, 3, 3, 3));
  const __m128 yzxw = _mm_shuffle_ps(unitQuat.get128(), unitQuat.get128(), _MM_SHUFFLE(3, 0, 2, 1));
  const __m128 zxyw = _mm_shuffle_ps(unitQuat.get128(), unitQuat.get128(), _MM_SHUFFLE(3, 1, 0, 2));
  const __m128 yzxw_2 = _mm_shuffle_ps(xyzw_2, xyzw_2, _MM_SHUFFLE(3, 0, 2, 1));
  const __m128 zxyw_2 = _mm_shuffle_ps(xyzw_2, xyzw_2, _MM_SHUFFLE(3, 1, 0, 2));
  __m128 tmp0, tmp1, tmp2;
  tmp0 = _mm_mul_ps(yzxw_2, wwww);                               //tmp0=2yw, 2zw, 2xw, 2w2
  tmp1 = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_mul_ps(yzxw, yzxw_2));//tmp1=1 - 2y2, 1 - 2z2, 1 - 2x2, 1 - 2w2
  tmp2 = _mm_mul_ps(yzxw, xyzw_2);                               //tmp2=2xy, 2yz, 2xz, 2w2
  tmp0 = _mm_add_ps(_mm_mul_ps(zxyw, xyzw_2), tmp0);             //tmp0=2yw + 2zx, 2zw + 2xy, 2xw + 2yz, 2w2 + 2w2
  tmp1 = _mm_sub_ps(tmp1, _mm_mul_ps(zxyw, zxyw_2));             //tmp1=1 - 2y2 - 2z2, 1 - 2z2 - 2x2, 1 - 2x2 - 2y2, 1 - 2w2 - 2w2
  tmp2 = _mm_sub_ps(tmp2, _mm_mul_ps(zxyw_2, wwww));             //tmp2=2xy - 2zw, 2yz - 2xw, 2xz - 2yw, 2w2 -2w2
  const __m128 tmp3 = sseSelect(tmp0, tmp1, select_x);
  const __m128 tmp4 = sseSelect(tmp1, tmp2, select_x);
  const __m128 tmp5 = sseSelect(tmp2, tmp0, select_x);
  mCol0 = Vector3(sseSelect(tmp3, tmp2, select_z));
  mCol1 = Vector3(sseSelect(tmp4, tmp0, select_z));
  mCol2 = Vector3(sseSelect(tmp5, tmp1, select_z));
}

inline Matrix3::Matrix3(const Vector3 & _col0, const Vector3 & _col1, const Vector3 & _col2)
{
  mCol0 = _col0;
  mCol1 = _col1;
  mCol2 = _col2;
}

inline Matrix3 & Matrix3::setCol0(const Vector3 & _col0)
{
  mCol0 = _col0;
  return *this;
}

inline Matrix3 & Matrix3::setCol1(const Vector3 & _col1)
{
  mCol1 = _col1;
  return *this;
}

inline Matrix3 & Matrix3::setCol2(const Vector3 & _col2)
{
  mCol2 = _col2;
  return *this;
}

inline Matrix3 & Matrix3::setCol(int col, const Vector3 & vec)
{
  *(&mCol0 + col) = vec;
  return *this;
}

inline Matrix3 & Matrix3::setRow(int row, const Vector3 & vec)
{
  mCol0.setElem(row, vec.getElem(0));
  mCol1.setElem(row, vec.getElem(1));
  mCol2.setElem(row, vec.getElem(2));
  return *this;
}

inline Matrix3 & Matrix3::setElem(int col, int row, float val)
{
  (*this)[col].setElem(row, val);
  return *this;
}

inline Matrix3 & Matrix3::setElem(int col, int row, const FloatInVec & val)
{
  Vector3 tmpV3_0 = this->getCol(col);
  tmpV3_0.setElem(row, val);
  this->setCol(col, tmpV3_0);
  return *this;
}

inline const FloatInVec Matrix3::getElem(int col, int row) const
{
  return this->getCol(col).getElem(row);
}

inline const Vector3 Matrix3::getCol0() const
{
  return mCol0;
}

inline const Vector3 Matrix3::getCol1() const
{
  return mCol1;
}

inline const Vector3 Matrix3::getCol2() const
{
  return mCol2;
}

inline const Vector3 Matrix3::getCol(int col) const
{
  return *(&mCol0 + col);
}

inline const Vector3 Matrix3::getRow(int row) const
{
  return Vector3(mCol0.getElem(row), mCol1.getElem(row), mCol2.getElem(row));
}

inline Vector3 & Matrix3::operator[](int col)
{
  return *(&mCol0 + col);
}

inline const Vector3 Matrix3::operator[](int col) const
{
  return *(&mCol0 + col);
}

inline Matrix3 & Matrix3::operator = (const Matrix3 & mat)
{
  mCol0 = mat.mCol0;
  mCol1 = mat.mCol1;
  mCol2 = mat.mCol2;
  return *this;
}

inline const Matrix3 transpose(const Matrix3 & mat)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  __m128 tmp0, tmp1, res0, res1, res2;
  tmp0 = _mm_unpacklo_ps(mat.getCol0().get128(), mat.getCol2().get128());
  tmp1 = _mm_unpackhi_ps(mat.getCol0().get128(), mat.getCol2().get128());
  res0 = _mm_unpacklo_ps(tmp0, mat.getCol1().get128());
  res1 = _mm_shuffle_ps(tmp0, tmp0, _MM_SHUFFLE(0, 3, 2, 2));
  res1 = sseSelect(res1, mat.getCol1().get128(), select_y);
  res2 = _mm_shuffle_ps(tmp1, tmp1, _MM_SHUFFLE(0, 1, 1, 0));
  res2 = sseSelect(res2, sseSplat(mat.getCol1().get128(), 2), select_y);
  return Matrix3(Vector3(res0), Vector3(res1), Vector3(res2));
}

inline const Matrix3 inverse(const Matrix3 & mat)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 tmp2 = sseVecCross(mat.getCol0().get128(), mat.getCol1().get128());
  const __m128 tmp0 = sseVecCross(mat.getCol1().get128(), mat.getCol2().get128());
  const __m128 tmp1 = sseVecCross(mat.getCol2().get128(), mat.getCol0().get128());
  __m128 dot = sseVecDot3(tmp2, mat.getCol2().get128());
  dot = sseSplat(dot, 0);
  const __m128 invdet = _mm_rcp_ps(dot);
  const __m128 tmp3 = _mm_unpacklo_ps(tmp0, tmp2);
  const __m128 tmp4 = _mm_unpackhi_ps(tmp0, tmp2);
  __m128 inv0, inv1, inv2;
  inv0 = _mm_unpacklo_ps(tmp3, tmp1);
  inv1 = _mm_shuffle_ps(tmp3, tmp3, _MM_SHUFFLE(0, 3, 2, 2));
  inv1 = sseSelect(inv1, tmp1, select_y);
  inv2 = _mm_shuffle_ps(tmp4, tmp4, _MM_SHUFFLE(0, 1, 1, 0));
  inv2 = sseSelect(inv2, sseSplat(tmp1, 2), select_y);
  inv0 = _mm_mul_ps(inv0, invdet);
  inv1 = _mm_mul_ps(inv1, invdet);
  inv2 = _mm_mul_ps(inv2, invdet);
  return Matrix3(Vector3(inv0), Vector3(inv1), Vector3(inv2));
}

inline const FloatInVec determinant(const Matrix3 & mat)
{
  return dot(mat.getCol2(), cross(mat.getCol0(), mat.getCol1()));
}

inline const Matrix3 Matrix3::operator + (const Matrix3 & mat) const
{
  return Matrix3(
    (mCol0 + mat.mCol0),
    (mCol1 + mat.mCol1),
    (mCol2 + mat.mCol2));
}

inline const Matrix3 Matrix3::operator - (const Matrix3 & mat) const
{
  return Matrix3((mCol0 - mat.mCol0), (mCol1 - mat.mCol1), (mCol2 - mat.mCol2));
}

inline Matrix3 & Matrix3::operator += (const Matrix3 & mat)
{
  *this = *this + mat;
  return *this;
}

inline Matrix3 & Matrix3::operator -= (const Matrix3 & mat)
{
  *this = *this - mat;
  return *this;
}

inline const Matrix3 Matrix3::operator - () const
{
  return Matrix3((-mCol0), (-mCol1), (-mCol2));
}

inline const Matrix3 absPerElem(const Matrix3 & mat)
{
  return Matrix3(absPerElem(mat.getCol0()), absPerElem(mat.getCol1()), absPerElem(mat.getCol2()));
}

inline const Matrix3 Matrix3::operator * (float scalar) const
{
  return *this * FloatInVec(scalar);
}

inline const Matrix3 Matrix3::operator * (const FloatInVec & scalar) const
{
  return Matrix3((mCol0 * scalar), (mCol1 * scalar), (mCol2 * scalar));
}

inline Matrix3 & Matrix3::operator *= (float scalar)
{
  return *this *= FloatInVec(scalar);
}

inline Matrix3 & Matrix3::operator *= (const FloatInVec & scalar)
{
  *this = *this * scalar;
  return *this;
}

inline const Matrix3 operator * (float scalar, const Matrix3 & mat)
{
  return FloatInVec(scalar) * mat;
}

inline const Matrix3 operator * (const FloatInVec & scalar, const Matrix3 & mat)
{
  return mat * scalar;
}

inline const Vector3 Matrix3::operator * (const Vector3 & vec) const
{
  const __m128 xxxx = sseSplat(vec.get128(), 0);
  const __m128 yyyy = sseSplat(vec.get128(), 1);
  const __m128 zzzz = sseSplat(vec.get128(), 2);
  __m128 res = _mm_mul_ps(mCol0.get128(), xxxx);
  res = sseMAdd(mCol1.get128(), yyyy, res);
  res = sseMAdd(mCol2.get128(), zzzz, res);
  return Vector3(res);
}

inline const Matrix3 Matrix3::operator * (const Matrix3 & mat) const
{
  return Matrix3((*this * mat.mCol0), (*this * mat.mCol1), (*this * mat.mCol2));
}

inline Matrix3 & Matrix3::operator *= (const Matrix3 & mat)
{
  *this = *this * mat;
  return *this;
}

inline const Matrix3 mulPerElem(const Matrix3 & mat0, const Matrix3 & mat1)
{
  return Matrix3(
    mulPerElem(mat0.getCol0(), mat1.getCol0()),
    mulPerElem(mat0.getCol1(), mat1.getCol1()),
    mulPerElem(mat0.getCol2(), mat1.getCol2()));
}

inline const Matrix3 Matrix3::identity()
{
  return Matrix3(Vector3::xAxis(), Vector3::yAxis(), Vector3::zAxis());
}

inline const Matrix3 Matrix3::rotationX(float radians)
{
  return rotationX(FloatInVec(radians));
}

inline const Matrix3 Matrix3::rotationX(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  const __m128 zero = _mm_setzero_ps();
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 select_z = _mm_load_ps(sz);
  __m128 s, c, res1, res2;
  sseSinfCosf(radians.get128(), &s, &c);
  res1 = sseSelect(zero, c, select_y);
  res1 = sseSelect(res1, s, select_z);
  res2 = sseSelect(zero, sseNegatef(s), select_y);
  res2 = sseSelect(res2, c, select_z);
  return Matrix3(Vector3::xAxis(), Vector3(res1), Vector3(res2));
}

inline const Matrix3 Matrix3::rotationY(float radians)
{
  return rotationY(FloatInVec(radians));
}

inline const Matrix3 Matrix3::rotationY(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  const __m128 zero = _mm_setzero_ps();
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_z = _mm_load_ps(sz);
  __m128 s, c, res0, res2;
  sseSinfCosf(radians.get128(), &s, &c);
  res0 = sseSelect(zero, c, select_x);
  res0 = sseSelect(res0, sseNegatef(s), select_z);
  res2 = sseSelect(zero, s, select_x);
  res2 = sseSelect(res2, c, select_z);
  return Matrix3(Vector3(res0), Vector3::yAxis(), Vector3(res2));
}

inline const Matrix3 Matrix3::rotationZ(float radians)
{
  return rotationZ(FloatInVec(radians));
}

inline const Matrix3 Matrix3::rotationZ(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 zero = _mm_setzero_ps();
  __m128 s, c, res0, res1;
  sseSinfCosf(radians.get128(), &s, &c);
  res0 = sseSelect(zero, c, select_x);
  res0 = sseSelect(res0, s, select_y);
  res1 = sseSelect(zero, sseNegatef(s), select_x);
  res1 = sseSelect(res1, c, select_y);
  return Matrix3(Vector3(res0), Vector3(res1), Vector3::zAxis());
}

inline const Matrix3 Matrix3::rotationZYX(const Vector3 & radiansXYZ)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sxyz[4]) = { ffffffff, ffffffff, ffffffff, 0.0f };
  const __m128 select_xyz = _mm_load_ps(sxyz);
  const __m128 angles = Vector4(radiansXYZ, 0.0f).get128();
  __m128 s, c;
  sseSinfCosf(angles, &s, &c);
  const __m128 negS = sseNegatef(s);
  const __m128 Z0 = _mm_unpackhi_ps(c, s);
  __m128 Z1 = _mm_unpackhi_ps(negS, c);
  Z1 = _mm_and_ps(Z1, select_xyz);
  const __m128 Y0 = _mm_shuffle_ps(c, negS, _MM_SHUFFLE(0, 1, 1, 1));
  const __m128 Y1 = _mm_shuffle_ps(s, c, _MM_SHUFFLE(0, 1, 1, 1));
  const __m128 X0 = sseSplat(s, 0);
  const __m128 X1 = sseSplat(c, 0);
  const __m128 tmp = _mm_mul_ps(Z0, Y1);
  return Matrix3(
    Vector3(_mm_mul_ps(Z0, Y0)),
    Vector3(sseMAdd(Z1, X1, _mm_mul_ps(tmp, X0))),
    Vector3(sseMSub(Z1, X0, _mm_mul_ps(tmp, X1))));
}

inline const Matrix3 Matrix3::rotation(float radians, const Vector3 & unitVec)
{
  return rotation(FloatInVec(radians), unitVec);
}

inline const Matrix3 Matrix3::rotation(const FloatInVec & radians, const Vector3 & unitVec)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 axis = unitVec.get128();
  __m128 s, c, tmp0, tmp1, tmp2;
  sseSinfCosf(radians.get128(), &s, &c);
  const __m128 xxxx = sseSplat(axis, 0);
  const __m128 yyyy = sseSplat(axis, 1);
  const __m128 zzzz = sseSplat(axis, 2);
  const __m128 oneMinusC = _mm_sub_ps(_mm_set1_ps(1.0f), c);
  const __m128 axisS = _mm_mul_ps(axis, s);
  const __m128 negAxisS = sseNegatef(axisS);
  tmp0 = _mm_shuffle_ps(axisS, axisS, _MM_SHUFFLE(0, 0, 2, 0));
  tmp0 = sseSelect(tmp0, sseSplat(negAxisS, 1), select_z);
  tmp1 = sseSelect(sseSplat(axisS, 0), sseSplat(negAxisS, 2), select_x);
  tmp2 = _mm_shuffle_ps(axisS, axisS, _MM_SHUFFLE(0, 0, 0, 1));
  tmp2 = sseSelect(tmp2, sseSplat(negAxisS, 0), select_y);
  tmp0 = sseSelect(tmp0, c, select_x);
  tmp1 = sseSelect(tmp1, c, select_y);
  tmp2 = sseSelect(tmp2, c, select_z);
  return Matrix3(
    Vector3(sseMAdd(_mm_mul_ps(axis, xxxx), oneMinusC, tmp0)),
    Vector3(sseMAdd(_mm_mul_ps(axis, yyyy), oneMinusC, tmp1)),
    Vector3(sseMAdd(_mm_mul_ps(axis, zzzz), oneMinusC, tmp2)));
}

inline const Matrix3 Matrix3::rotation(const Quat & unitQuat)
{
  return Matrix3(unitQuat);
}

inline const Matrix3 Matrix3::scale(const Vector3 & scaleVec)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  const __m128 zero = _mm_setzero_ps();
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  return Matrix3(
    Vector3(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sx))),
    Vector3(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sy))),
    Vector3(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sz))));
}

inline const Matrix3 appendScale(const Matrix3 & mat, const Vector3 & scaleVec)
{
  return Matrix3(
    (mat.getCol0() * scaleVec.getX()),
    (mat.getCol1() * scaleVec.getY()),
    (mat.getCol2() * scaleVec.getZ()));
}

inline const Matrix3 prependScale(const Vector3 & scaleVec, const Matrix3 & mat)
{
  return Matrix3(
    mulPerElem(mat.getCol0(), scaleVec),
    mulPerElem(mat.getCol1(), scaleVec),
    mulPerElem(mat.getCol2(), scaleVec));
}

inline const Matrix3 select(const Matrix3 & mat0, const Matrix3 & mat1, bool select1)
{
  return Matrix3(
    select(mat0.getCol0(), mat1.getCol0(), select1),
    select(mat0.getCol1(), mat1.getCol1(), select1),
    select(mat0.getCol2(), mat1.getCol2(), select1));
}

inline const Matrix3 select(const Matrix3 & mat0, const Matrix3 & mat1, const BoolInVec & select1)
{
  return Matrix3(
    select(mat0.getCol0(), mat1.getCol0(), select1),
    select(mat0.getCol1(), mat1.getCol1(), select1),
    select(mat0.getCol2(), mat1.getCol2(), select1));
}

// ========================================================
// Matrix4
// ========================================================

inline Matrix4::Matrix4(const Matrix4 & mat)
{
  mCol0 = mat.mCol0;
  mCol1 = mat.mCol1;
  mCol2 = mat.mCol2;
  mCol3 = mat.mCol3;
}

inline Matrix4::Matrix4(float scalar)
{
  mCol0 = Vector4(scalar);
  mCol1 = Vector4(scalar);
  mCol2 = Vector4(scalar);
  mCol3 = Vector4(scalar);
}

inline Matrix4::Matrix4(const FloatInVec & scalar)
{
  mCol0 = Vector4(scalar);
  mCol1 = Vector4(scalar);
  mCol2 = Vector4(scalar);
  mCol3 = Vector4(scalar);
}

inline Matrix4::Matrix4(const Transform3 & mat)
{
  mCol0 = Vector4(mat.getCol0(), 0.0f);
  mCol1 = Vector4(mat.getCol1(), 0.0f);
  mCol2 = Vector4(mat.getCol2(), 0.0f);
  mCol3 = Vector4(mat.getCol3(), 1.0f);
}

inline Matrix4::Matrix4(const Vector4 & _col0, const Vector4 & _col1, const Vector4 & _col2, const Vector4 & _col3)
{
  mCol0 = _col0;
  mCol1 = _col1;
  mCol2 = _col2;
  mCol3 = _col3;
}

inline Matrix4::Matrix4(const Matrix3 & mat, const Vector3 & translateVec)
{
  mCol0 = Vector4(mat.getCol0(), 0.0f);
  mCol1 = Vector4(mat.getCol1(), 0.0f);
  mCol2 = Vector4(mat.getCol2(), 0.0f);
  mCol3 = Vector4(translateVec,  1.0f);
}

inline Matrix4::Matrix4(const Quat & unitQuat, const Vector3 & translateVec)
{
  Matrix3 mat = Matrix3(unitQuat);
  mCol0 = Vector4(mat.getCol0(), 0.0f);
  mCol1 = Vector4(mat.getCol1(), 0.0f);
  mCol2 = Vector4(mat.getCol2(), 0.0f);
  mCol3 = Vector4(translateVec,  1.0f);
}

inline Matrix4 & Matrix4::setCol0(const Vector4 & _col0)
{
  mCol0 = _col0;
  return *this;
}

inline Matrix4 & Matrix4::setCol1(const Vector4 & _col1)
{
  mCol1 = _col1;
  return *this;
}

inline Matrix4 & Matrix4::setCol2(const Vector4 & _col2)
{
  mCol2 = _col2;
  return *this;
}

inline Matrix4 & Matrix4::setCol3(const Vector4 & _col3)
{
  mCol3 = _col3;
  return *this;
}

inline Matrix4 & Matrix4::setCol(int col, const Vector4 & vec)
{
  *(&mCol0 + col) = vec;
  return *this;
}

inline Matrix4 & Matrix4::setRow(int row, const Vector4 & vec)
{
  mCol0.setElem(row, vec.getElem(0));
  mCol1.setElem(row, vec.getElem(1));
  mCol2.setElem(row, vec.getElem(2));
  mCol3.setElem(row, vec.getElem(3));
  return *this;
}

inline Matrix4 & Matrix4::setElem(int col, int row, float val)
{
  (*this)[col].setElem(row, val);
  return *this;
}

inline Matrix4 & Matrix4::setElem(int col, int row, const FloatInVec & val)
{
  Vector4 tmpV3_0 = this->getCol(col);
  tmpV3_0.setElem(row, val);
  this->setCol(col, tmpV3_0);
  return *this;
}

inline const FloatInVec Matrix4::getElem(int col, int row) const
{
  return this->getCol(col).getElem(row);
}

inline const Vector4 Matrix4::getCol0() const
{
  return mCol0;
}

inline const Vector4 Matrix4::getCol1() const
{
  return mCol1;
}

inline const Vector4 Matrix4::getCol2() const
{
  return mCol2;
}

inline const Vector4 Matrix4::getCol3() const
{
  return mCol3;
}

inline const Vector4 Matrix4::getCol(int col) const
{
  return *(&mCol0 + col);
}

inline const Vector4 Matrix4::getRow(int row) const
{
  return Vector4(mCol0.getElem(row), mCol1.getElem(row), mCol2.getElem(row), mCol3.getElem(row));
}

inline Vector4 & Matrix4::operator[](int col)
{
  return *(&mCol0 + col);
}

inline const Vector4 Matrix4::operator[](int col) const
{
  return *(&mCol0 + col);
}

inline Matrix4 & Matrix4::operator = (const Matrix4 & mat)
{
  mCol0 = mat.mCol0;
  mCol1 = mat.mCol1;
  mCol2 = mat.mCol2;
  mCol3 = mat.mCol3;
  return *this;
}

inline const Matrix4 transpose(const Matrix4 & mat)
{
  const __m128 tmp0 = _mm_unpacklo_ps(mat.getCol0().get128(), mat.getCol2().get128());
  const __m128 tmp1 = _mm_unpacklo_ps(mat.getCol1().get128(), mat.getCol3().get128());
  const __m128 tmp2 = _mm_unpackhi_ps(mat.getCol0().get128(), mat.getCol2().get128());
  const __m128 tmp3 = _mm_unpackhi_ps(mat.getCol1().get128(), mat.getCol3().get128());
  const __m128 res0 = _mm_unpacklo_ps(tmp0, tmp1);
  const __m128 res1 = _mm_unpackhi_ps(tmp0, tmp1);
  const __m128 res2 = _mm_unpacklo_ps(tmp2, tmp3);
  const __m128 res3 = _mm_unpackhi_ps(tmp2, tmp3);
  return Matrix4(Vector4(res0), Vector4(res1), Vector4(res2), Vector4(res3));
}

inline const Matrix4 inverse(const Matrix4 & mat)
{
  static const float f0x80000000 = bit_cast_uint2float(0x80000000U);
  VECTORMATH_ALIGNED(const float PNPN[4]) = { 0.0f, f0x80000000, 0.0f, f0x80000000 };
  VECTORMATH_ALIGNED(const float NPNP[4]) = { f0x80000000, 0.0f, f0x80000000, 0.0f };
  VECTORMATH_ALIGNED(const float one) = 1.0f;
  const __m128 Sign_PNPN = _mm_load_ps(PNPN);
  const __m128 Sign_NPNP = _mm_load_ps(NPNP);
  __m128 Va, Vb, Vc;
  __m128 r1, r2, r3, tt, tt2;
  __m128 sum, Det, RDet;
  __m128 mtL1, mtL2, mtL3, mtL4;
  __m128 _L1 = mat.getCol0().get128();
  __m128 _L2 = mat.getCol1().get128();
  __m128 _L3 = mat.getCol2().get128();
  __m128 _L4 = mat.getCol3().get128();

  // Calculating the minterms for the first line.
  tt = _L4;
  tt2 = sseRor(_L3, 1);
  Vc = _mm_mul_ps(tt2, sseRor(tt, 0)); // V3'.V4
  Va = _mm_mul_ps(tt2, sseRor(tt, 2)); // V3'.V4"
  Vb = _mm_mul_ps(tt2, sseRor(tt, 3)); // V3'.V4^

  r1 = _mm_sub_ps(sseRor(Va, 1), sseRor(Vc, 2)); // V3".V4^ - V3^.V4"
  r2 = _mm_sub_ps(sseRor(Vb, 2), sseRor(Vb, 0)); // V3^.V4' - V3'.V4^
  r3 = _mm_sub_ps(sseRor(Va, 0), sseRor(Vc, 1)); // V3'.V4" - V3".V4'

  tt = _L2;
  Va = sseRor(tt, 1);
  sum = _mm_mul_ps(Va, r1);
  Vb = sseRor(tt, 2);
  sum = _mm_add_ps(sum, _mm_mul_ps(Vb, r2));
  Vc = sseRor(tt, 3);
  sum = _mm_add_ps(sum, _mm_mul_ps(Vc, r3));

  // Calculating the determinant.
  Det = _mm_mul_ps(sum, _L1);
  Det = _mm_add_ps(Det, _mm_movehl_ps(Det, Det));

  mtL1 = _mm_xor_ps(sum, Sign_PNPN);

  // Calculating the minterms of the second line (using previous results).
  tt = sseRor(_L1, 1);
  sum = _mm_mul_ps(tt, r1);
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r2));
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r3));
  mtL2 = _mm_xor_ps(sum, Sign_NPNP);

  // Testing the determinant.
  Det = _mm_sub_ss(Det, _mm_shuffle_ps(Det, Det, 1));

  // Calculating the minterms of the third line.
  tt = sseRor(_L1, 1);
  Va = _mm_mul_ps(tt, Vb);  // V1'.V2"
  Vb = _mm_mul_ps(tt, Vc);  // V1'.V2^
  Vc = _mm_mul_ps(tt, _L2); // V1'.V2

  r1 = _mm_sub_ps(sseRor(Va, 1), sseRor(Vc, 2)); // V1".V2^ - V1^.V2"
  r2 = _mm_sub_ps(sseRor(Vb, 2), sseRor(Vb, 0)); // V1^.V2' - V1'.V2^
  r3 = _mm_sub_ps(sseRor(Va, 0), sseRor(Vc, 1)); // V1'.V2" - V1".V2'

  tt = sseRor(_L4, 1);
  sum = _mm_mul_ps(tt, r1);
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r2));
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r3));
  mtL3 = _mm_xor_ps(sum, Sign_PNPN);

  // Dividing is FASTER than rcp_nr! (Because rcp_nr causes many register-memory RWs).
  //
  // __m128 c =_mm_div_ss(__m128 a, __m128 b __m128) :
  // divides the first component of b by a, the other components are copied from a.
  //
  // TODO: find out why in original code X1_YZ0_W1[4] = { 1.0f, 0.0f, 0.0f, 1.0f }
  // was used for _mm_load_ss((float*)&X1_YZ0_W1) instead of just pointer to 1.0f which does the same,
  // _mm_load_ss(float * p) sets _m128 --> [*p] [0] [0] [0].
  RDet = _mm_div_ss(_mm_load_ss(&one), Det);
  RDet = _mm_shuffle_ps(RDet, RDet, 0x0);

  // Devide the first 12 minterms with the determinant.
  mtL1 = _mm_mul_ps(mtL1, RDet);
  mtL2 = _mm_mul_ps(mtL2, RDet);
  mtL3 = _mm_mul_ps(mtL3, RDet);

  // Calculate the minterms of the forth line and devide by the determinant.
  tt = sseRor(_L3, 1);
  sum = _mm_mul_ps(tt, r1);
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r2));
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r3));
  mtL4 = _mm_xor_ps(sum, Sign_NPNP);
  mtL4 = _mm_mul_ps(mtL4, RDet);

  // Now we just have to transpose the minterms matrix.
  const __m128 trns0 = _mm_unpacklo_ps(mtL1, mtL2);
  const __m128 trns1 = _mm_unpacklo_ps(mtL3, mtL4);
  const __m128 trns2 = _mm_unpackhi_ps(mtL1, mtL2);
  const __m128 trns3 = _mm_unpackhi_ps(mtL3, mtL4);
  _L1 = _mm_movelh_ps(trns0, trns1);
  _L2 = _mm_movehl_ps(trns1, trns0);
  _L3 = _mm_movelh_ps(trns2, trns3);
  _L4 = _mm_movehl_ps(trns3, trns2);

  return Matrix4(Vector4(_L1), Vector4(_L2), Vector4(_L3), Vector4(_L4));
}

inline const Matrix4 affineInverse(const Matrix4 & mat)
{
  Transform3 affineMat;
  affineMat.setCol0(mat.getCol0().getXYZ());
  affineMat.setCol1(mat.getCol1().getXYZ());
  affineMat.setCol2(mat.getCol2().getXYZ());
  affineMat.setCol3(mat.getCol3().getXYZ());
  return Matrix4(inverse(affineMat));
}

inline const Matrix4 orthoInverse(const Matrix4 & mat)
{
  Transform3 affineMat;
  affineMat.setCol0(mat.getCol0().getXYZ());
  affineMat.setCol1(mat.getCol1().getXYZ());
  affineMat.setCol2(mat.getCol2().getXYZ());
  affineMat.setCol3(mat.getCol3().getXYZ());
  return Matrix4(orthoInverse(affineMat));
}

inline const FloatInVec determinant(const Matrix4 & mat)
{
  __m128 Va, Vb, Vc;
  __m128 r1, r2, r3, tt, tt2;
  __m128 sum, Det;
  const __m128 _L1 = mat.getCol0().get128();
  const __m128 _L2 = mat.getCol1().get128();
  const __m128 _L3 = mat.getCol2().get128();
  const __m128 _L4 = mat.getCol3().get128();

  // Calculating the minterms for the first line.
  tt = _L4;
  tt2 = sseRor(_L3, 1);
  Vc = _mm_mul_ps(tt2, sseRor(tt, 0)); // V3'.V4
  Va = _mm_mul_ps(tt2, sseRor(tt, 2)); // V3'.V4"
  Vb = _mm_mul_ps(tt2, sseRor(tt, 3)); // V3'.V4^

  r1 = _mm_sub_ps(sseRor(Va, 1), sseRor(Vc, 2)); // V3".V4^ - V3^.V4"
  r2 = _mm_sub_ps(sseRor(Vb, 2), sseRor(Vb, 0)); // V3^.V4' - V3'.V4^
  r3 = _mm_sub_ps(sseRor(Va, 0), sseRor(Vc, 1)); // V3'.V4" - V3".V4'

  tt = _L2;
  Va = sseRor(tt, 1);
  sum = _mm_mul_ps(Va, r1);
  Vb = sseRor(tt, 2);
  sum = _mm_add_ps(sum, _mm_mul_ps(Vb, r2));
  Vc = sseRor(tt, 3);
  sum = _mm_add_ps(sum, _mm_mul_ps(Vc, r3));

  // Calculating the determinant.
  Det = _mm_mul_ps(sum, _L1);
  Det = _mm_add_ps(Det, _mm_movehl_ps(Det, Det));

  // Calculating the minterms of the second line (using previous results).
  tt = sseRor(_L1, 1);
  sum = _mm_mul_ps(tt, r1);
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r2));
  tt = sseRor(tt, 1);
  sum = _mm_add_ps(sum, _mm_mul_ps(tt, r3));

  // Testing the determinant.
  Det = _mm_sub_ss(Det, _mm_shuffle_ps(Det, Det, 1));
  return FloatInVec(Det, 0);
}

inline const Matrix4 Matrix4::operator + (const Matrix4 & mat) const
{
  return Matrix4(
    (mCol0 + mat.mCol0),
    (mCol1 + mat.mCol1),
    (mCol2 + mat.mCol2),
    (mCol3 + mat.mCol3));
}

inline const Matrix4 Matrix4::operator - (const Matrix4 & mat) const
{
  return Matrix4(
    (mCol0 - mat.mCol0),
    (mCol1 - mat.mCol1),
    (mCol2 - mat.mCol2),
    (mCol3 - mat.mCol3));
}

inline Matrix4 & Matrix4::operator += (const Matrix4 & mat)
{
  *this = *this + mat;
  return *this;
}

inline Matrix4 & Matrix4::operator -= (const Matrix4 & mat)
{
  *this = *this - mat;
  return *this;
}

inline const Matrix4 Matrix4::operator - () const
{
  return Matrix4((-mCol0), (-mCol1), (-mCol2), (-mCol3));
}

inline const Matrix4 absPerElem(const Matrix4 & mat)
{
  return Matrix4(
    absPerElem(mat.getCol0()),
    absPerElem(mat.getCol1()),
    absPerElem(mat.getCol2()),
    absPerElem(mat.getCol3()));
}

inline const Matrix4 Matrix4::operator * (float scalar) const
{
  return *this * FloatInVec(scalar);
}

inline const Matrix4 Matrix4::operator * (const FloatInVec & scalar) const
{
  return Matrix4((mCol0 * scalar), (mCol1 * scalar), (mCol2 * scalar), (mCol3 * scalar));
}

inline Matrix4 & Matrix4::operator *= (float scalar)
{
  return *this *= FloatInVec(scalar);
}

inline Matrix4 & Matrix4::operator *= (const FloatInVec & scalar)
{
  *this = *this * scalar;
  return *this;
}

inline const Matrix4 operator * (float scalar, const Matrix4 & mat)
{
  return FloatInVec(scalar) * mat;
}

inline const Matrix4 operator * (const FloatInVec & scalar, const Matrix4 & mat)
{
  return mat * scalar;
}

inline const Vector4 Matrix4::operator * (const Vector4 & vec) const
{
  return Vector4(
  _mm_add_ps(
    _mm_add_ps(_mm_mul_ps(mCol0.get128(), _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(0, 0, 0, 0))), _mm_mul_ps(mCol1.get128(), _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(1, 1, 1, 1)))),
    _mm_add_ps(_mm_mul_ps(mCol2.get128(), _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(2, 2, 2, 2))), _mm_mul_ps(mCol3.get128(), _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(3, 3, 3, 3))))));
}

inline const Vector4 Matrix4::operator * (const Vector3 & vec) const
{
  return Vector4(
  _mm_add_ps(
    _mm_add_ps(_mm_mul_ps(mCol0.get128(), _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(0, 0, 0, 0))), _mm_mul_ps(mCol1.get128(), _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(1, 1, 1, 1)))),
    _mm_mul_ps(mCol2.get128(), _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(2, 2, 2, 2)))));
}

inline const Vector4 Matrix4::operator * (const Point3 & pnt) const
{
  return Vector4(
  _mm_add_ps(
    _mm_add_ps(_mm_mul_ps(mCol0.get128(), _mm_shuffle_ps(pnt.get128(), pnt.get128(), _MM_SHUFFLE(0, 0, 0, 0))), _mm_mul_ps(mCol1.get128(), _mm_shuffle_ps(pnt.get128(), pnt.get128(), _MM_SHUFFLE(1, 1, 1, 1)))),
    _mm_add_ps(_mm_mul_ps(mCol2.get128(), _mm_shuffle_ps(pnt.get128(), pnt.get128(), _MM_SHUFFLE(2, 2, 2, 2))), mCol3.get128())));
}

inline const Matrix4 Matrix4::operator * (const Matrix4 & mat) const
{
  return Matrix4((*this * mat.mCol0), (*this * mat.mCol1), (*this * mat.mCol2), (*this * mat.mCol3));
}

inline Matrix4 & Matrix4::operator *= (const Matrix4 & mat)
{
  *this = *this * mat;
  return *this;
}

inline const Matrix4 Matrix4::operator * (const Transform3 & tfrm) const
{
  return Matrix4(
    (*this * tfrm.getCol0()),
    (*this * tfrm.getCol1()),
    (*this * tfrm.getCol2()),
    (*this * Point3(tfrm.getCol3())));
}

inline Matrix4 & Matrix4::operator *= (const Transform3 & tfrm)
{
  *this = *this * tfrm;
  return *this;
}

inline const Matrix4 mulPerElem(const Matrix4 & mat0, const Matrix4 & mat1)
{
  return Matrix4(
    mulPerElem(mat0.getCol0(), mat1.getCol0()),
    mulPerElem(mat0.getCol1(), mat1.getCol1()),
    mulPerElem(mat0.getCol2(), mat1.getCol2()),
    mulPerElem(mat0.getCol3(), mat1.getCol3()));
}

inline const Matrix4 Matrix4::identity()
{
  return Matrix4(Vector4::xAxis(), Vector4::yAxis(), Vector4::zAxis(), Vector4::wAxis());
}

inline Matrix4 & Matrix4::setUpper3x3(const Matrix3 & mat3)
{
  mCol0.setXYZ(mat3.getCol0());
  mCol1.setXYZ(mat3.getCol1());
  mCol2.setXYZ(mat3.getCol2());
  return *this;
}

inline const Matrix3 Matrix4::getUpper3x3() const
{
  return Matrix3(mCol0.getXYZ(), mCol1.getXYZ(), mCol2.getXYZ());
}

inline Matrix4 & Matrix4::setTranslation(const Vector3 & translateVec)
{
  mCol3.setXYZ(translateVec);
  return *this;
}

inline const Vector3 Matrix4::getTranslation() const
{
  return mCol3.getXYZ();
}

inline const Matrix4 Matrix4::rotationX(float radians)
{
  return rotationX(FloatInVec(radians));
}

inline const Matrix4 Matrix4::rotationX(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 zero = _mm_setzero_ps();
  __m128 s, c, res1, res2;
  sseSinfCosf(radians.get128(), &s, &c);
  res1 = sseSelect(zero, c, select_y);
  res1 = sseSelect(res1, s, select_z);
  res2 = sseSelect(zero, sseNegatef(s), select_y);
  res2 = sseSelect(res2, c, select_z);
  return Matrix4(Vector4::xAxis(), Vector4(res1), Vector4(res2), Vector4::wAxis());
}

inline const Matrix4 Matrix4::rotationY(float radians)
{
  return rotationY(FloatInVec(radians));
}

inline const Matrix4 Matrix4::rotationY(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 zero = _mm_setzero_ps();
  __m128 s, c, res0, res2;
  sseSinfCosf(radians.get128(), &s, &c);
  res0 = sseSelect(zero, c, select_x);
  res0 = sseSelect(res0, sseNegatef(s), select_z);
  res2 = sseSelect(zero, s, select_x);
  res2 = sseSelect(res2, c, select_z);
  return Matrix4(Vector4(res0), Vector4::yAxis(), Vector4(res2), Vector4::wAxis());
}

inline const Matrix4 Matrix4::rotationZ(float radians)
{
  return rotationZ(FloatInVec(radians));
}

inline const Matrix4 Matrix4::rotationZ(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 zero = _mm_setzero_ps();
  __m128 s, c, res0, res1;
  sseSinfCosf(radians.get128(), &s, &c);
  res0 = sseSelect(zero, c, select_x);
  res0 = sseSelect(res0, s, select_y);
  res1 = sseSelect(zero, sseNegatef(s), select_x);
  res1 = sseSelect(res1, c, select_y);
  return Matrix4(Vector4(res0), Vector4(res1), Vector4::zAxis(), Vector4::wAxis());
}

inline const Matrix4 Matrix4::rotationZYX(const Vector3 & radiansXYZ)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sxyz[4]) = { ffffffff, ffffffff, ffffffff, 0.0f };
  __m128 s, c;
  const __m128 angles = Vector4(radiansXYZ, 0.0f).get128();
  sseSinfCosf(angles, &s, &c);
  const __m128 negS = sseNegatef(s);
  const __m128 Z0 = _mm_unpackhi_ps(c, s);
  __m128 Z1 = _mm_unpackhi_ps(negS, c);
  Z1 = _mm_and_ps(Z1, _mm_load_ps(sxyz));
  const __m128 Y0 = _mm_shuffle_ps(c, negS, _MM_SHUFFLE(0, 1, 1, 1));
  const __m128 Y1 = _mm_shuffle_ps(s, c, _MM_SHUFFLE(0, 1, 1, 1));
  const __m128 X0 = sseSplat(s, 0);
  const __m128 X1 = sseSplat(c, 0);
  const __m128 tmp = _mm_mul_ps(Z0, Y1);
  return Matrix4(
    Vector4(_mm_mul_ps(Z0, Y0)),
    Vector4(sseMAdd(Z1, X1, _mm_mul_ps(tmp, X0))),
    Vector4(sseMSub(Z1, X0, _mm_mul_ps(tmp, X1))),
    Vector4::wAxis());
}

inline const Matrix4 Matrix4::rotation(float radians, const Vector3 & unitVec)
{
  return rotation(FloatInVec(radians), unitVec);
}

inline const Matrix4 Matrix4::rotation(const FloatInVec & radians, const Vector3 & unitVec)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  VECTORMATH_ALIGNED(const float sxyz[4]) = { ffffffff, ffffffff, ffffffff, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 select_xyz = _mm_load_ps(sxyz);
  __m128 axis = unitVec.get128();
  __m128 s, c, tmp0, tmp1, tmp2;
  sseSinfCosf(radians.get128(), &s, &c);
  const __m128 xxxx = sseSplat(axis, 0);
  const __m128 yyyy = sseSplat(axis, 1);
  const __m128 zzzz = sseSplat(axis, 2);
  const __m128 oneMinusC = _mm_sub_ps(_mm_set1_ps(1.0f), c);
  const __m128 axisS = _mm_mul_ps(axis, s);
  const __m128 negAxisS = sseNegatef(axisS);
  tmp0 = _mm_shuffle_ps(axisS, axisS, _MM_SHUFFLE(0, 0, 2, 0));
  tmp0 = sseSelect(tmp0, sseSplat(negAxisS, 1), select_z);
  tmp1 = sseSelect(sseSplat(axisS, 0), sseSplat(negAxisS, 2), select_x);
  tmp2 = _mm_shuffle_ps(axisS, axisS, _MM_SHUFFLE(0, 0, 0, 1));
  tmp2 = sseSelect(tmp2, sseSplat(negAxisS, 0), select_y);
  tmp0 = sseSelect(tmp0, c, select_x);
  tmp1 = sseSelect(tmp1, c, select_y);
  tmp2 = sseSelect(tmp2, c, select_z);
  axis = _mm_and_ps(axis, select_xyz);
  tmp0 = _mm_and_ps(tmp0, select_xyz);
  tmp1 = _mm_and_ps(tmp1, select_xyz);
  tmp2 = _mm_and_ps(tmp2, select_xyz);
  return Matrix4(
    Vector4(sseMAdd(_mm_mul_ps(axis, xxxx), oneMinusC, tmp0)),
    Vector4(sseMAdd(_mm_mul_ps(axis, yyyy), oneMinusC, tmp1)),
    Vector4(sseMAdd(_mm_mul_ps(axis, zzzz), oneMinusC, tmp2)),
    Vector4::wAxis());
}

inline const Matrix4 Matrix4::rotation(const Quat & unitQuat)
{
  return Matrix4(Transform3::rotation(unitQuat));
}

inline const Matrix4 Matrix4::scale(const Vector3 & scaleVec)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 zero = _mm_setzero_ps();
  return Matrix4(
    Vector4(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sx))),
    Vector4(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sy))),
    Vector4(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sz))),
    Vector4::wAxis());
}

inline const Matrix4 appendScale(const Matrix4 & mat, const Vector3 & scaleVec)
{
  return Matrix4(
    (mat.getCol0() * scaleVec.getX()),
    (mat.getCol1() * scaleVec.getY()),
    (mat.getCol2() * scaleVec.getZ()),
    mat.getCol3());
}

inline const Matrix4 prependScale(const Vector3 & scaleVec, const Matrix4 & mat)
{
  const Vector4 scale4 = Vector4(scaleVec, 1.0f);
  return Matrix4(
    mulPerElem(mat.getCol0(), scale4),
    mulPerElem(mat.getCol1(), scale4),
    mulPerElem(mat.getCol2(), scale4),
    mulPerElem(mat.getCol3(), scale4));
}

inline const Matrix4 Matrix4::translation(const Vector3 & translateVec)
{
  return Matrix4(Vector4::xAxis(), Vector4::yAxis(), Vector4::zAxis(), Vector4(translateVec, 1.0f));
}

inline const Matrix4 Matrix4::lookAt(const Point3 & eyePos, const Point3 & lookAtPos, const Vector3 & upVec)
{
  Vector3 v3Y = normalize(upVec);
  const Vector3 v3Z = normalize((eyePos - lookAtPos));
  const Vector3 v3X = normalize(cross(v3Y, v3Z));
  v3Y = cross(v3Z, v3X);
  const Matrix4 m4EyeFrame = Matrix4(Vector4(v3X), Vector4(v3Y), Vector4(v3Z), Vector4(eyePos));
  return orthoInverse(m4EyeFrame);
}

inline const Matrix4 Matrix4::perspective(float fovyRadians, float aspect, float zNear, float zFar)
{
  static const float VECTORMATH_PI_OVER_2 = 1.570796327f;
  const __m128 zero = _mm_setzero_ps();
  const float f = tanf(VECTORMATH_PI_OVER_2 - fovyRadians * 0.5f);
  const float rangeInv = 1.0f / (zNear - zFar);
  SSEFloat tmp;
  tmp.m128 = zero;
  tmp.f[0] = f / aspect;
  const __m128 col0 = tmp.m128;
  tmp.m128 = zero;
  tmp.f[1] = f;
  const __m128 col1 = tmp.m128;
  tmp.m128 = zero;
  tmp.f[2] = (zNear + zFar) * rangeInv;
  tmp.f[3] = -1.0f;
  const __m128 col2 = tmp.m128;
  tmp.m128 = zero;
  tmp.f[2] = zNear * zFar * rangeInv * 2.0f;
  const __m128 col3 = tmp.m128;
  return Matrix4(Vector4(col0), Vector4(col1), Vector4(col2), Vector4(col3));
}

inline const Matrix4 Matrix4::frustum(float left, float right, float bottom, float top, float zNear, float zFar)
{
  /* function implementation based on code from STIDC SDK:           */
  /* --------------------------------------------------------------  */
  /* PLEASE DO NOT MODIFY THIS SECTION                               */
  /* This prolog section is automatically generated.                 */
  /*                                                                 */
  /* (C)Copyright                                                    */
  /* Sony Computer Entertainment, Inc.,                              */
  /* Toshiba Corporation,                                            */
  /* International Business Machines Corporation,                    */
  /* 2001,2002.                                                      */
  /* S/T/I Confidential Information                                  */
  /* --------------------------------------------------------------  */
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  VECTORMATH_ALIGNED(const float sw[4]) = { 0.0f, 0.0f, 0.0f, ffffffff };
  const __m128 zero = _mm_setzero_ps();
  SSEFloat l, f, r, n, b, t;
  l.f[0] = left;
  f.f[0] = zFar;
  r.f[0] = right;
  n.f[0] = zNear;
  b.f[0] = bottom;
  t.f[0] = top;
  __m128 lbf = _mm_unpacklo_ps(l.m128, f.m128);
  __m128 rtn = _mm_unpacklo_ps(r.m128, n.m128);
  lbf = _mm_unpacklo_ps(lbf, b.m128);
  rtn = _mm_unpacklo_ps(rtn, t.m128);
  const __m128 diff = _mm_sub_ps(rtn, lbf);
  const __m128 sum = _mm_add_ps(rtn, lbf);
  const __m128 inv_diff = _mm_rcp_ps(diff);
  __m128 near2 = sseSplat(n.m128, 0);
  near2 = _mm_add_ps(near2, near2);
  const __m128 diagonal = _mm_mul_ps(near2, inv_diff);
  const __m128 column = _mm_mul_ps(sum, inv_diff);
  return Matrix4(
    Vector4(sseSelect(zero, diagonal, _mm_load_ps(sx))),
    Vector4(sseSelect(zero, diagonal, _mm_load_ps(sy))),
    Vector4(sseSelect(column, _mm_set1_ps(-1.0f), _mm_load_ps(sw))),
    Vector4(sseSelect(zero, _mm_mul_ps(diagonal, sseSplat(f.m128, 0)), _mm_load_ps(sz))));
}

inline const Matrix4 Matrix4::orthographic(float left, float right, float bottom, float top, float zNear, float zFar)
{
  /* function implementation based on code from STIDC SDK:           */
  /* --------------------------------------------------------------  */
  /* PLEASE DO NOT MODIFY THIS SECTION                               */
  /* This prolog section is automatically generated.                 */
  /*                                                                 */
  /* (C)Copyright                                                    */
  /* Sony Computer Entertainment, Inc.,                              */
  /* Toshiba Corporation,                                            */
  /* International Business Machines Corporation,                    */
  /* 2001,2002.                                                      */
  /* S/T/I Confidential Information                                  */
  /* --------------------------------------------------------------  */
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  VECTORMATH_ALIGNED(const float sw[4]) = { 0.0f, 0.0f, 0.0f, ffffffff };
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 zero = _mm_setzero_ps();
  SSEFloat l, f, r, n, b, t;
  l.f[0] = left;
  f.f[0] = zFar;
  r.f[0] = right;
  n.f[0] = zNear;
  b.f[0] = bottom;
  t.f[0] = top;
  __m128 lbf = _mm_unpacklo_ps(l.m128, f.m128);
  __m128 rtn = _mm_unpacklo_ps(r.m128, n.m128);
  lbf = _mm_unpacklo_ps(lbf, b.m128);
  rtn = _mm_unpacklo_ps(rtn, t.m128);
  const __m128 diff = _mm_sub_ps(rtn, lbf);
  const __m128 sum = _mm_add_ps(rtn, lbf);
  const __m128 inv_diff = _mm_rcp_ps(diff);
  const __m128 neg_inv_diff = sseNegatef(inv_diff);
  const __m128 diagonal = _mm_add_ps(inv_diff, inv_diff);
  const __m128 column = _mm_mul_ps(sum, sseSelect(neg_inv_diff, inv_diff, select_z));
  return Matrix4(
    Vector4(sseSelect(zero, diagonal, _mm_load_ps(sx))),
    Vector4(sseSelect(zero, diagonal, _mm_load_ps(sy))),
    Vector4(sseSelect(zero, diagonal, select_z)),
    Vector4(sseSelect(column, _mm_set1_ps(1.0f), _mm_load_ps(sw))));
}

inline const Matrix4 select(const Matrix4 & mat0, const Matrix4 & mat1, bool select1)
{
  return Matrix4(
    select(mat0.getCol0(), mat1.getCol0(), select1),
    select(mat0.getCol1(), mat1.getCol1(), select1),
    select(mat0.getCol2(), mat1.getCol2(), select1),
    select(mat0.getCol3(), mat1.getCol3(), select1));
}

inline const Matrix4 select(const Matrix4 & mat0, const Matrix4 & mat1, const BoolInVec & select1)
{
  return Matrix4(
    select(mat0.getCol0(), mat1.getCol0(), select1),
    select(mat0.getCol1(), mat1.getCol1(), select1),
    select(mat0.getCol2(), mat1.getCol2(), select1),
    select(mat0.getCol3(), mat1.getCol3(), select1));
}

// ========================================================
// Transform3
// ========================================================

inline Transform3::Transform3(const Transform3 & tfrm)
{
  mCol0 = tfrm.mCol0;
  mCol1 = tfrm.mCol1;
  mCol2 = tfrm.mCol2;
  mCol3 = tfrm.mCol3;
}

inline Transform3::Transform3(float scalar)
{
  mCol0 = Vector3(scalar);
  mCol1 = Vector3(scalar);
  mCol2 = Vector3(scalar);
  mCol3 = Vector3(scalar);
}

inline Transform3::Transform3(const FloatInVec & scalar)
{
  mCol0 = Vector3(scalar);
  mCol1 = Vector3(scalar);
  mCol2 = Vector3(scalar);
  mCol3 = Vector3(scalar);
}

inline Transform3::Transform3(const Vector3 & _col0, const Vector3 & _col1, const Vector3 & _col2, const Vector3 & _col3)
{
  mCol0 = _col0;
  mCol1 = _col1;
  mCol2 = _col2;
  mCol3 = _col3;
}

inline Transform3::Transform3(const Matrix3 & tfrm, const Vector3 & translateVec)
{
  this->setUpper3x3(tfrm);
  this->setTranslation(translateVec);
}

inline Transform3::Transform3(const Quat & unitQuat, const Vector3 & translateVec)
{
  this->setUpper3x3(Matrix3(unitQuat));
  this->setTranslation(translateVec);
}

inline Transform3 & Transform3::setCol0(const Vector3 & _col0)
{
  mCol0 = _col0;
  return *this;
}

inline Transform3 & Transform3::setCol1(const Vector3 & _col1)
{
  mCol1 = _col1;
  return *this;
}

inline Transform3 & Transform3::setCol2(const Vector3 & _col2)
{
  mCol2 = _col2;
  return *this;
}

inline Transform3 & Transform3::setCol3(const Vector3 & _col3)
{
  mCol3 = _col3;
  return *this;
}

inline Transform3 & Transform3::setCol(int col, const Vector3 & vec)
{
  *(&mCol0 + col) = vec;
  return *this;
}

inline Transform3 & Transform3::setRow(int row, const Vector4 & vec)
{
  mCol0.setElem(row, vec.getElem(0));
  mCol1.setElem(row, vec.getElem(1));
  mCol2.setElem(row, vec.getElem(2));
  mCol3.setElem(row, vec.getElem(3));
  return *this;
}

inline Transform3 & Transform3::setElem(int col, int row, float val)
{
  (*this)[col].setElem(row, val);
  return *this;
}

inline Transform3 & Transform3::setElem(int col, int row, const FloatInVec & val)
{
  Vector3 tmpV3_0 = this->getCol(col);
  tmpV3_0.setElem(row, val);
  this->setCol(col, tmpV3_0);
  return *this;
}

inline const FloatInVec Transform3::getElem(int col, int row) const
{
  return this->getCol(col).getElem(row);
}

inline const Vector3 Transform3::getCol0() const
{
  return mCol0;
}

inline const Vector3 Transform3::getCol1() const
{
  return mCol1;
}

inline const Vector3 Transform3::getCol2() const
{
  return mCol2;
}

inline const Vector3 Transform3::getCol3() const
{
  return mCol3;
}

inline const Vector3 Transform3::getCol(int col) const
{
  return *(&mCol0 + col);
}

inline const Vector4 Transform3::getRow(int row) const
{
  return Vector4(mCol0.getElem(row), mCol1.getElem(row), mCol2.getElem(row), mCol3.getElem(row));
}

inline Vector3 & Transform3::operator[](int col)
{
  return *(&mCol0 + col);
}

inline const Vector3 Transform3::operator[](int col) const
{
  return *(&mCol0 + col);
}

inline Transform3 & Transform3::operator = (const Transform3 & tfrm)
{
  mCol0 = tfrm.mCol0;
  mCol1 = tfrm.mCol1;
  mCol2 = tfrm.mCol2;
  mCol3 = tfrm.mCol3;
  return *this;
}

inline const Transform3 inverse(const Transform3 & tfrm)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 tmp2 = sseVecCross(tfrm.getCol0().get128(), tfrm.getCol1().get128());
  const __m128 tmp0 = sseVecCross(tfrm.getCol1().get128(), tfrm.getCol2().get128());
  const __m128 tmp1 = sseVecCross(tfrm.getCol2().get128(), tfrm.getCol0().get128());
  __m128 inv0, inv1, inv2, inv3, dot;
  inv3 = sseNegatef(tfrm.getCol3().get128());
  dot = sseVecDot3(tmp2, tfrm.getCol2().get128());
  dot = sseSplat(dot, 0);
  const __m128 invdet = _mm_rcp_ps(dot);
  const __m128 tmp3 = _mm_unpacklo_ps(tmp0, tmp2);
  const __m128 tmp4 = _mm_unpackhi_ps(tmp0, tmp2);
  inv0 = _mm_unpacklo_ps(tmp3, tmp1);
  const __m128 xxxx = sseSplat(inv3, 0);
  inv1 = _mm_shuffle_ps(tmp3, tmp3, _MM_SHUFFLE(0, 3, 2, 2));
  inv1 = sseSelect(inv1, tmp1, select_y);
  inv2 = _mm_shuffle_ps(tmp4, tmp4, _MM_SHUFFLE(0, 1, 1, 0));
  inv2 = sseSelect(inv2, sseSplat(tmp1, 2), select_y);
  const __m128 yyyy = sseSplat(inv3, 1);
  const __m128 zzzz = sseSplat(inv3, 2);
  inv3 = _mm_mul_ps(inv0, xxxx);
  inv3 = sseMAdd(inv1, yyyy, inv3);
  inv3 = sseMAdd(inv2, zzzz, inv3);
  inv0 = _mm_mul_ps(inv0, invdet);
  inv1 = _mm_mul_ps(inv1, invdet);
  inv2 = _mm_mul_ps(inv2, invdet);
  inv3 = _mm_mul_ps(inv3, invdet);
  return Transform3(Vector3(inv0), Vector3(inv1), Vector3(inv2), Vector3(inv3));
}

inline const Transform3 orthoInverse(const Transform3 & tfrm)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 tmp0 = _mm_unpacklo_ps(tfrm.getCol0().get128(), tfrm.getCol2().get128());
  const __m128 tmp1 = _mm_unpackhi_ps(tfrm.getCol0().get128(), tfrm.getCol2().get128());
  __m128 inv0, inv1, inv2, inv3;
  inv3 = sseNegatef(tfrm.getCol3().get128());
  inv0 = _mm_unpacklo_ps(tmp0, tfrm.getCol1().get128());
  const __m128 xxxx = sseSplat(inv3, 0);
  inv1 = _mm_shuffle_ps(tmp0, tmp0, _MM_SHUFFLE(0, 3, 2, 2));
  inv1 = sseSelect(inv1, tfrm.getCol1().get128(), select_y);
  inv2 = _mm_shuffle_ps(tmp1, tmp1, _MM_SHUFFLE(0, 1, 1, 0));
  inv2 = sseSelect(inv2, sseSplat(tfrm.getCol1().get128(), 2), select_y);
  const __m128 yyyy = sseSplat(inv3, 1);
  const __m128 zzzz = sseSplat(inv3, 2);
  inv3 = _mm_mul_ps(inv0, xxxx);
  inv3 = sseMAdd(inv1, yyyy, inv3);
  inv3 = sseMAdd(inv2, zzzz, inv3);
  return Transform3(Vector3(inv0), Vector3(inv1), Vector3(inv2), Vector3(inv3));
}

inline const Transform3 absPerElem(const Transform3 & tfrm)
{
  return Transform3(
    absPerElem(tfrm.getCol0()),
    absPerElem(tfrm.getCol1()),
    absPerElem(tfrm.getCol2()),
    absPerElem(tfrm.getCol3()));
}

inline const Vector3 Transform3::operator * (const Vector3 & vec) const
{
  const __m128 xxxx = sseSplat(vec.get128(), 0);
  const __m128 yyyy = sseSplat(vec.get128(), 1);
  const __m128 zzzz = sseSplat(vec.get128(), 2);
  __m128 res = _mm_mul_ps(mCol0.get128(), xxxx);
  res = sseMAdd(mCol1.get128(), yyyy, res);
  res = sseMAdd(mCol2.get128(), zzzz, res);
  return Vector3(res);
}

inline const Point3 Transform3::operator * (const Point3 & pnt) const
{
  const __m128 xxxx = sseSplat(pnt.get128(), 0);
  const __m128 yyyy = sseSplat(pnt.get128(), 1);
  const __m128 zzzz = sseSplat(pnt.get128(), 2);
  __m128 tmp0 = _mm_mul_ps(mCol0.get128(), xxxx);
  __m128 tmp1 = _mm_mul_ps(mCol1.get128(), yyyy);
  tmp0 = sseMAdd(mCol2.get128(), zzzz, tmp0);
  tmp1 = _mm_add_ps(mCol3.get128(), tmp1);
  const __m128 res = _mm_add_ps(tmp0, tmp1);
  return Point3(res);
}

inline const Transform3 Transform3::operator * (const Transform3 & tfrm) const
{
  return Transform3(
    (*this * tfrm.mCol0),
    (*this * tfrm.mCol1),
    (*this * tfrm.mCol2),
    Vector3((*this * Point3(tfrm.mCol3))));
}

inline Transform3 & Transform3::operator *= (const Transform3 & tfrm)
{
  *this = *this * tfrm;
  return *this;
}

inline const Transform3 mulPerElem(const Transform3 & tfrm0, const Transform3 & tfrm1)
{
  return Transform3(
    mulPerElem(tfrm0.getCol0(), tfrm1.getCol0()),
    mulPerElem(tfrm0.getCol1(), tfrm1.getCol1()),
    mulPerElem(tfrm0.getCol2(), tfrm1.getCol2()),
    mulPerElem(tfrm0.getCol3(), tfrm1.getCol3()));
}

inline const Transform3 Transform3::identity()
{
  return Transform3(
    Vector3::xAxis(),
    Vector3::yAxis(),
    Vector3::zAxis(),
    Vector3(0.0f));
}

inline Transform3 & Transform3::setUpper3x3(const Matrix3 & tfrm)
{
  mCol0 = tfrm.getCol0();
  mCol1 = tfrm.getCol1();
  mCol2 = tfrm.getCol2();
  return *this;
}

inline const Matrix3 Transform3::getUpper3x3() const
{
  return Matrix3(mCol0, mCol1, mCol2);
}

inline Transform3 & Transform3::setTranslation(const Vector3 & translateVec)
{
  mCol3 = translateVec;
  return *this;
}

inline const Vector3 Transform3::getTranslation() const
{
  return mCol3;
}

inline const Transform3 Transform3::rotationX(float radians)
{
  return rotationX(FloatInVec(radians));
}

inline const Transform3 Transform3::rotationX(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 zero = _mm_setzero_ps();
  __m128 s, c, res1, res2;
  sseSinfCosf(radians.get128(), &s, &c);
  res1 = sseSelect(zero, c, select_y);
  res1 = sseSelect(res1, s, select_z);
  res2 = sseSelect(zero, sseNegatef(s), select_y);
  res2 = sseSelect(res2, c, select_z);
  return Transform3(Vector3::xAxis(), Vector3(res1), Vector3(res2), Vector3(_mm_setzero_ps()));
}

inline const Transform3 Transform3::rotationY(float radians)
{
  return rotationY(FloatInVec(radians));
}

inline const Transform3 Transform3::rotationY(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 zero = _mm_setzero_ps();
  __m128 s, c, res0, res2;
  sseSinfCosf(radians.get128(), &s, &c);
  res0 = sseSelect(zero, c, select_x);
  res0 = sseSelect(res0, sseNegatef(s), select_z);
  res2 = sseSelect(zero, s, select_x);
  res2 = sseSelect(res2, c, select_z);
  return Transform3(Vector3(res0), Vector3::yAxis(), Vector3(res2), Vector3(0.0f));
}

inline const Transform3 Transform3::rotationZ(float radians)
{
  return rotationZ(FloatInVec(radians));
}

inline const Transform3 Transform3::rotationZ(const FloatInVec & radians)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 zero = _mm_setzero_ps();
  __m128 s, c, res0, res1;
  sseSinfCosf(radians.get128(), &s, &c);
  res0 = sseSelect(zero, c, select_x);
  res0 = sseSelect(res0, s, select_y);
  res1 = sseSelect(zero, sseNegatef(s), select_x);
  res1 = sseSelect(res1, c, select_y);
  return Transform3(Vector3(res0), Vector3(res1), Vector3::zAxis(), Vector3(0.0f));
}

inline const Transform3 Transform3::rotationZYX(const Vector3 & radiansXYZ)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sxyz[4]) = { ffffffff, ffffffff, ffffffff, 0.0f };
  const __m128 angles = Vector4(radiansXYZ, 0.0f).get128();
  __m128 s, c;
  sseSinfCosf(angles, &s, &c);
  const __m128 negS = sseNegatef(s);
  const __m128 Z0 = _mm_unpackhi_ps(c, s);
  __m128 Z1 = _mm_unpackhi_ps(negS, c);
  Z1 = _mm_and_ps(Z1, _mm_load_ps(sxyz));
  const __m128 Y0 = _mm_shuffle_ps(c, negS, _MM_SHUFFLE(0, 1, 1, 1));
  const __m128 Y1 = _mm_shuffle_ps(s, c, _MM_SHUFFLE(0, 1, 1, 1));
  const __m128 X0 = sseSplat(s, 0);
  const __m128 X1 = sseSplat(c, 0);
  const __m128 tmp = _mm_mul_ps(Z0, Y1);
  return Transform3(
    Vector3(_mm_mul_ps(Z0, Y0)),
    Vector3(sseMAdd(Z1, X1, _mm_mul_ps(tmp, X0))),
    Vector3(sseMSub(Z1, X0, _mm_mul_ps(tmp, X1))),
    Vector3(0.0f));
}

inline const Transform3 Transform3::rotation(float radians, const Vector3 & unitVec)
{
  return rotation(FloatInVec(radians), unitVec);
}

inline const Transform3 Transform3::rotation(const FloatInVec & radians, const Vector3 & unitVec)
{
  return Transform3(Matrix3::rotation(radians, unitVec), Vector3(0.0f));
}

inline const Transform3 Transform3::rotation(const Quat & unitQuat)
{
  return Transform3(Matrix3(unitQuat), Vector3(0.0f));
}

inline const Transform3 Transform3::scale(const Vector3 & scaleVec)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  const __m128 zero = _mm_setzero_ps();
  return Transform3(
    Vector3(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sx))),
    Vector3(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sy))),
    Vector3(sseSelect(zero, scaleVec.get128(), _mm_load_ps(sz))),
    Vector3(0.0f));
}

inline const Transform3 appendScale(const Transform3 & tfrm, const Vector3 & scaleVec)
{
  return Transform3(
    (tfrm.getCol0() * scaleVec.getX()),
    (tfrm.getCol1() * scaleVec.getY()),
    (tfrm.getCol2() * scaleVec.getZ()),
    tfrm.getCol3());
}

inline const Transform3 prependScale(const Vector3 & scaleVec, const Transform3 & tfrm)
{
  return Transform3(
    mulPerElem(tfrm.getCol0(), scaleVec),
    mulPerElem(tfrm.getCol1(), scaleVec),
    mulPerElem(tfrm.getCol2(), scaleVec),
    mulPerElem(tfrm.getCol3(), scaleVec));
}

inline const Transform3 Transform3::translation(const Vector3 & translateVec)
{
  return Transform3(Vector3::xAxis(), Vector3::yAxis(), Vector3::zAxis(), translateVec);
}

inline const Transform3 select(const Transform3 & tfrm0, const Transform3 & tfrm1, bool select1)
{
  return Transform3(
    select(tfrm0.getCol0(), tfrm1.getCol0(), select1),
    select(tfrm0.getCol1(), tfrm1.getCol1(), select1),
    select(tfrm0.getCol2(), tfrm1.getCol2(), select1),
    select(tfrm0.getCol3(), tfrm1.getCol3(), select1));
}

inline const Transform3 select(const Transform3 & tfrm0, const Transform3 & tfrm1, const BoolInVec & select1)
{
  return Transform3(
    select(tfrm0.getCol0(), tfrm1.getCol0(), select1),
    select(tfrm0.getCol1(), tfrm1.getCol1(), select1),
    select(tfrm0.getCol2(), tfrm1.getCol2(), select1),
    select(tfrm0.getCol3(), tfrm1.getCol3(), select1));
}

// ========================================================
// Quat
// ========================================================

inline Quat::Quat(const Matrix3 & tfrm)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  VECTORMATH_ALIGNED(const float sw[4]) = { 0.0f, 0.0f, 0.0f, ffffffff };
  const __m128 select_x = _mm_load_ps(sx);
  const __m128 select_y = _mm_load_ps(sy);
  const __m128 select_z = _mm_load_ps(sz);
  const __m128 select_w = _mm_load_ps(sw);
  const __m128 col0 = tfrm.getCol0().get128();
  const __m128 col1 = tfrm.getCol1().get128();
  const __m128 col2 = tfrm.getCol2().get128();
  __m128 res;
  __m128 xx_yy, xx_yy_zz_xx, yy_zz_xx_yy, zz_xx_yy_zz;
  __m128 zy_xz_yx, yz_zx_xy;
  __m128 res0, res1, res2, res3;

  /* four cases: */
  /* trace > 0 */
  /* else */
  /*  xx largest diagonal element */
  /*  yy largest diagonal element */
  /*  zz largest diagonal element */
  /* compute quaternion for each case */

  xx_yy = sseSelect(col0, col1, select_y);
  xx_yy_zz_xx = _mm_shuffle_ps(xx_yy, xx_yy, _MM_SHUFFLE(0, 0, 1, 0));
  xx_yy_zz_xx = sseSelect(xx_yy_zz_xx, col2, select_z);
  yy_zz_xx_yy = _mm_shuffle_ps(xx_yy_zz_xx, xx_yy_zz_xx, _MM_SHUFFLE(1, 0, 2, 1));
  zz_xx_yy_zz = _mm_shuffle_ps(xx_yy_zz_xx, xx_yy_zz_xx, _MM_SHUFFLE(2, 1, 0, 2));

  const __m128 diagSum  = _mm_add_ps(_mm_add_ps(xx_yy_zz_xx, yy_zz_xx_yy), zz_xx_yy_zz);
  const __m128 diagDiff = _mm_sub_ps(_mm_sub_ps(xx_yy_zz_xx, yy_zz_xx_yy), zz_xx_yy_zz);
  const __m128 radicand = _mm_add_ps(sseSelect(diagDiff, diagSum, select_w), _mm_set1_ps(1.0f));
  const __m128 invSqrt = sseNewtonrapsonRSqrtf(radicand);

  zy_xz_yx = sseSelect(col0, col1, select_z);                             // zy_xz_yx = 00 01 12 03
  zy_xz_yx = _mm_shuffle_ps(zy_xz_yx, zy_xz_yx, _MM_SHUFFLE(0, 1, 2, 2)); // zy_xz_yx = 12 12 01 00
  zy_xz_yx = sseSelect(zy_xz_yx, sseSplat(col2, 0), select_y);            // zy_xz_yx = 12 20 01 00
  yz_zx_xy = sseSelect(col0, col1, select_x);                             // yz_zx_xy = 10 01 02 03
  yz_zx_xy = _mm_shuffle_ps(yz_zx_xy, yz_zx_xy, _MM_SHUFFLE(0, 0, 2, 0)); // yz_zx_xy = 10 02 10 10
  yz_zx_xy = sseSelect(yz_zx_xy, sseSplat(col2, 1), select_x);            // yz_zx_xy = 21 02 10 10

  const __m128 sum = _mm_add_ps(zy_xz_yx, yz_zx_xy);
  const __m128 diff = _mm_sub_ps(zy_xz_yx, yz_zx_xy);
  const __m128 scale = _mm_mul_ps(invSqrt, _mm_set1_ps(0.5f));

  res0 = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(0, 1, 2, 0));
  res0 = sseSelect(res0, sseSplat(diff, 0), select_w);
  res1 = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(0, 0, 0, 2));
  res1 = sseSelect(res1, sseSplat(diff, 1), select_w);
  res2 = _mm_shuffle_ps(sum, sum, _MM_SHUFFLE(0, 0, 0, 1));
  res2 = sseSelect(res2, sseSplat(diff, 2), select_w);
  res3 = diff;
  res0 = sseSelect(res0, radicand, select_x);
  res1 = sseSelect(res1, radicand, select_y);
  res2 = sseSelect(res2, radicand, select_z);
  res3 = sseSelect(res3, radicand, select_w);
  res0 = _mm_mul_ps(res0, sseSplat(scale, 0));
  res1 = _mm_mul_ps(res1, sseSplat(scale, 1));
  res2 = _mm_mul_ps(res2, sseSplat(scale, 2));
  res3 = _mm_mul_ps(res3, sseSplat(scale, 3));

  /* determine case and select answer */
  const __m128 xx = sseSplat(col0, 0);
  const __m128 yy = sseSplat(col1, 1);
  const __m128 zz = sseSplat(col2, 2);
  res = sseSelect(res0, res1, _mm_cmpgt_ps(yy, xx));
  res = sseSelect(res, res2, _mm_and_ps(_mm_cmpgt_ps(zz, xx), _mm_cmpgt_ps(zz, yy)));
  res = sseSelect(res, res3, _mm_cmpgt_ps(sseSplat(diagSum, 0), _mm_setzero_ps()));
  mVec128 = res;
}

// ========================================================
// Misc free functions
// ========================================================

inline const Matrix3 outer(const Vector3 & tfrm0, const Vector3 & tfrm1)
{
  return Matrix3((tfrm0 * tfrm1.getX()), (tfrm0 * tfrm1.getY()), (tfrm0 * tfrm1.getZ()));
}

inline const Matrix4 outer(const Vector4 & tfrm0, const Vector4 & tfrm1)
{
  return Matrix4(
    (tfrm0 * tfrm1.getX()),
    (tfrm0 * tfrm1.getY()),
    (tfrm0 * tfrm1.getZ()),
    (tfrm0 * tfrm1.getW()));
}

inline const Vector3 rowMul(const Vector3 & vec, const Matrix3 & mat)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  const __m128 select_y = _mm_load_ps(sy);
  __m128 mcol0, mcol1, mcol2, res;
  const __m128 tmp0 = _mm_unpacklo_ps(mat.getCol0().get128(), mat.getCol2().get128());
  const __m128 tmp1 = _mm_unpackhi_ps(mat.getCol0().get128(), mat.getCol2().get128());
  const __m128 xxxx = sseSplat(vec.get128(), 0);
  mcol0 = _mm_unpacklo_ps(tmp0, mat.getCol1().get128());
  mcol1 = _mm_shuffle_ps(tmp0, tmp0, _MM_SHUFFLE(0, 3, 2, 2));
  mcol1 = sseSelect(mcol1, mat.getCol1().get128(), select_y);
  mcol2 = _mm_shuffle_ps(tmp1, tmp1, _MM_SHUFFLE(0, 1, 1, 0));
  mcol2 = sseSelect(mcol2, sseSplat(mat.getCol1().get128(), 2), select_y);
  const __m128 yyyy = sseSplat(vec.get128(), 1);
  res = _mm_mul_ps(mcol0, xxxx);
  const __m128 zzzz = sseSplat(vec.get128(), 2);
  res = sseMAdd(mcol1, yyyy, res);
  res = sseMAdd(mcol2, zzzz, res);
  return Vector3(res);
}

inline const Matrix3 crossMatrix(const Vector3 & vec)
{
  static const float ffffffff = bit_cast_uint2float(0xffffffffU);
  VECTORMATH_ALIGNED(const float sx[4]) = { ffffffff, 0.0f, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sy[4]) = { 0.0f, ffffffff, 0.0f, 0.0f };
  VECTORMATH_ALIGNED(const float sz[4]) = { 0.0f, 0.0f, ffffffff, 0.0f };
  VECTORMATH_ALIGNED(const float fx[4]) = { 0.0f, ffffffff, ffffffff, ffffffff };
  VECTORMATH_ALIGNED(const float fy[4]) = { ffffffff, 0.0f, ffffffff, ffffffff };
  VECTORMATH_ALIGNED(const float fz[4]) = { ffffffff, ffffffff, 0.0f, ffffffff };
  const __m128 neg = sseNegatef(vec.get128());
  __m128 res0, res1, res2;
  res0 = _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(0, 2, 2, 0));
  res0 = sseSelect(res0, sseSplat(neg, 1), _mm_load_ps(sz));
  res1 = sseSelect(sseSplat(vec.get128(), 0), sseSplat(neg, 2), _mm_load_ps(sx));
  res2 = _mm_shuffle_ps(vec.get128(), vec.get128(), _MM_SHUFFLE(0, 0, 1, 1));
  res2 = sseSelect(res2, sseSplat(neg, 0), _mm_load_ps(sy));
  res0 = _mm_and_ps(res0, _mm_load_ps(fx));
  res1 = _mm_and_ps(res1, _mm_load_ps(fy));
  res2 = _mm_and_ps(res2, _mm_load_ps(fz));
  return Matrix3(Vector3(res0), Vector3(res1), Vector3(res2));
}

inline const Matrix3 crossMatrixMul(const Vector3 & vec, const Matrix3 & mat)
{
  return Matrix3(cross(vec, mat.getCol0()), cross(vec, mat.getCol1()), cross(vec, mat.getCol2()));
}

} // namespace SSE
} // namespace Vectormath

#endif // VECTORMATH_SSE_MATRIX_HPP
