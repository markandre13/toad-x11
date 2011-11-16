#include "matrix4.h"

/*! \file 
	\ingroup Math
*/

#include <cmath>
#include <cassert>
#include <string>
#include <iostream>
#include <iomanip>
using namespace std;

#include <GL/gl.h>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

float Matrix::_identity[16] = 
{
   1.0, 0.0, 0.0, 0.0,
   0.0, 1.0, 0.0, 0.0,
   0.0, 0.0, 1.0, 0.0,
   0.0, 0.0, 0.0, 1.0
};

Matrix::Matrix()
{
	reset();
}

Matrix::Matrix(const Matrix &matrix)
{
	(*this) = matrix;
}

Matrix::Matrix(const float *matrix)
{
        real  *i = _matrix;
	const float *j =  matrix;
	for (int k=0; k<16; i++,j++,k++)
		*i = *j;
}

Matrix::Matrix(const double *matrix)
{
	real   *i = _matrix;
	const double *j =  matrix;
	for (int k=0; k<16; i++,j++,k++)
		*i = *j;
}

Matrix::Matrix(const unsigned int glMatrix)
{
	switch (glMatrix)
	{
	case GL_MODELVIEW_MATRIX:
		glGetFloatv(GL_MODELVIEW_MATRIX,_matrix);
		break;
	case GL_PROJECTION_MATRIX:
		glGetFloatv(GL_PROJECTION_MATRIX,_matrix);
		break;
	default:
		break;
	}
}

Matrix &
Matrix::operator=(const Matrix &m)
{
  memcpy(_matrix,m._matrix,sizeof(_matrix));
  return *this;
}

Matrix 
Matrix::operator *(const Matrix &m) const
{
#ifdef __SSE__
  Matrix r;
  __m128 a_line, b_line, r_line;
  for (int i=0; i<16; i+=4) {
    a_line = _mm_load_ps(m._matrix);
    b_line = _mm_set1_ps(_matrix[i]);
    r_line = _mm_mul_ps(a_line, b_line);
    for (int j=1; j<4; j++) {
      a_line = _mm_load_ps(&m._matrix[j<<2]);
      b_line = _mm_set1_ps(_matrix[i+j]);
      r_line = _mm_add_ps(_mm_mul_ps(a_line, b_line), r_line);
    }
    _mm_store_ps(&r._matrix[i], r_line);
  }
  return r;
#else
  Matrix prod;
  for (int c=0;c<4;c++)
    for (int r=0;r<4;r++)
      prod.set(c,r,
	get(c,0)*m.get(0,r) +
	get(c,1)*m.get(1,r) +
	get(c,2)*m.get(2,r) +
	get(c,3)*m.get(3,r));
  return prod;
#endif
}

Matrix &
Matrix::operator*=(const Matrix &m)
{
	// Potential optimisation here to
	// skip a temporary copy

	return (*this) = (*this)*m;
}

Vector    
Matrix::operator*(const Vector &v) const
{
	float prod[4] = { 0,0,0,0 };

	for (int r=0;r<4;r++)
	{
		for (int c=0;c<3;c++)
			prod[r] += v[c]*get(c,r);

		prod[r] += get(3,r);
	}

	float div = 1.0 / prod[3];

	return Vector(prod[0]*div,prod[1]*div,prod[2]*div);
}

void 
Matrix::reset()
{
	memcpy(_matrix,_identity,16*sizeof(float));
}

void 
Matrix::identity()
{
	reset();
}

bool 
Matrix::isIdentity() const
{
	return !memcmp(_matrix,_identity,sizeof(_matrix));
}

const float & 
Matrix::operator[](const int i) const
{
	assert(i>=0 && i<16);
	return _matrix[i];
}

float & 
Matrix::operator[](const int i)
{
	assert(i>=0 && i<16);
	return _matrix[i];
}

bool 
Matrix::operator==(const Matrix &m) const
{
	return !memcmp(_matrix,m._matrix,sizeof(_matrix));
}

bool 
Matrix::operator!=(const Matrix &m) const
{
	return memcmp(_matrix,m._matrix,sizeof(_matrix))!=0;
}

Matrix::operator float *()
{
	return _matrix;
}

Matrix::operator const float *() const
{
	return _matrix;
}

void
Matrix::glMultMatrix() const
{
	glMultMatrixf(_matrix);
}

void 
Matrix::glLoadMatrix() const
{
	glLoadMatrixf(_matrix);
}


/*! 
	\brief		Uniform scale transformation matrix
	\ingroup	Math
*/

Matrix matrixScale(const float sf)
{
	Matrix scale;

	scale.set(0,0,sf);
	scale.set(1,1,sf);
	scale.set(2,2,sf);
	
	return scale;
}

/*! 
	\brief		Non-uniform scale transformation matrix
	\ingroup	Math
*/

Matrix matrixScale(const Vector sf)
{
	Matrix scale;

	scale.set(0,0,sf[0]);
	scale.set(1,1,sf[1]);
	scale.set(2,2,sf[2]);
	
	return scale;
}

/*! 
	\brief		Translation transformation matrix
	\ingroup	Math
*/

Matrix matrixTranslate(const Vector trans)
{
	Matrix translate;

	translate.set(3,0,trans[0]);
	translate.set(3,1,trans[1]);
	translate.set(3,2,trans[2]);

	return translate;
}

/*! 
	\brief		Translation transformation matrix
	\ingroup	Math
*/

Matrix matrixTranslate(const real x,const real y,const real z)
{
	Matrix translate;

	translate.set(3,0,x);
	translate.set(3,1,y);
	translate.set(3,2,z);

	return translate;
}

/*! 
	\brief		Axis rotation transformation matrix
	\ingroup	Math
	\param      axis	Axis of rotation
	\param		angle	Angle in degrees
*/

Matrix matrixRotate(const Vector axis, const float angle)
{
	Matrix rotate;

	// Page 466, Graphics Gems

	float s = sin(angle*M_PI_DEG);
	float c = cos(angle*M_PI_DEG);
	float t = 1 - c;

	Vector ax = axis/sqrt(axis.norm());

	float x = ax[0];
	float y = ax[1];
	float z = ax[2];

	rotate.set(0,0,t*x*x+c);
	rotate.set(1,0,t*y*x+s*z);
	rotate.set(2,0,t*z*x-s*y);

	rotate.set(0,1,t*x*y-s*z);
	rotate.set(1,1,t*y*y+c);
	rotate.set(2,1,t*z*y+s*x);

	rotate.set(0,2,t*x*z+s*y);
	rotate.set(1,2,t*y*z-s*x);
	rotate.set(2,2,t*z*z+c);

	return rotate;
}

/*! 
	\brief		Rotation transformation matrix
	\ingroup	Math
	\param      azimuth		East-West degrees
	\param		elevation	North-South degrees
*/

Matrix matrixRotate(const float azimuth, const float elevation)
{
	Matrix rotate;

	float ca = cos(azimuth*M_PI_DEG);
	float sa = sin(azimuth*M_PI_DEG);
	float cb = cos(elevation*M_PI_DEG);
	float sb = sin(elevation*M_PI_DEG);

	rotate.set(0,0,cb);
	rotate.set(1,0,0);
	rotate.set(2,0,-sb);

	rotate.set(0,1,-sa*sb);
	rotate.set(1,1,ca);
	rotate.set(2,1,-sa*cb);

	rotate.set(0,2,ca*sb);
	rotate.set(1,2,sa);
	rotate.set(2,2,ca*cb);

	return rotate;
}

/*! 
	\brief		Orientation transformation matrix
	\ingroup	Math
	\param      x	New orientation for +x
	\param		y	New orientation for +y
	\param		z	New orientation for +z
*/

Matrix matrixOrient(const Vector &x,const Vector &y,const Vector &z)
{
	Matrix orient;

	orient.set(0,0,x.x());
	orient.set(0,1,x.y());
	orient.set(0,2,x.z());

	orient.set(1,0,y.x());
	orient.set(1,1,y.y());
	orient.set(1,2,y.z());

	orient.set(2,0,z.x());
	orient.set(2,1,z.y());
	orient.set(2,2,z.z());

	return orient;
}

/*! 
	\brief		Orientation transformation matrix
	\ingroup	Math
	\param      direction	New orientation for +z
	\param      up          New orientation for +y
*/

Matrix matrixOrient(const Vector &direction,const Vector &up)
{
	assert(direction.norm()>0.0);
	assert(up.norm()>0.0);

	Vector d(direction);
	d.normalize();

	Vector u(up);
	u.normalize();

	return matrixOrient(xProduct(u,d),u,d);
}

/*! 
	\brief		Write a matrix to a text output stream
	\ingroup	Math
*/

std::ostream &
operator<<(std::ostream &os,const Matrix &m)
{
	for (int r=0;r<4;r++)
		for (int c=0;c<4;c++)
			os << setw(10) << setfill(' ') << m.get(c,r) << ( c==3 ? '\n' : '\t');

	return os;
}

/*! 
	\brief		Read a matrix from a text output stream
	\ingroup	Math
*/

std::istream &
operator>>(std::istream &is,Matrix &m)
{
	for (int r=0;r<4;r++)
		for (int c=0;c<4;c++)
		{
			float tmp;
			is >> tmp;
			m.set(c,r,tmp);
		}

	return is;
}

Matrix 
Matrix::inverse() const
{
	Matrix inv;
	invertMatrix(_matrix,inv._matrix);
	return inv;
}

Matrix 
Matrix::transpose() const
{
	Matrix tmp;
	
	for (int i=0;i<4;i++)
		for (int j=0;j<4;j++)
			tmp.set(j,i,get(i,j));
	
	return tmp;
}

//
// Compute the inverse of a 4x4 matrix.  Contributed by scotter@lafn.org
//

/* NB. OpenGL Matrices are COLUMN major. */
#define MAT(m,r,c) (m)[(c)*4+(r)]

/* Here's some shorthand converting standard (row,column) to index. */
#define m11 MAT(m,0,0)
#define m12 MAT(m,0,1)
#define m13 MAT(m,0,2)
#define m14 MAT(m,0,3)
#define m21 MAT(m,1,0)
#define m22 MAT(m,1,1)
#define m23 MAT(m,1,2)
#define m24 MAT(m,1,3)
#define m31 MAT(m,2,0)
#define m32 MAT(m,2,1)
#define m33 MAT(m,2,2)
#define m34 MAT(m,2,3)
#define m41 MAT(m,3,0)
#define m42 MAT(m,3,1)
#define m43 MAT(m,3,2)
#define m44 MAT(m,3,3)

void 
Matrix::invertMatrixGeneral( const float *m, float *out )
{
   float det;
   float d12, d13, d23, d24, d34, d41;
   float tmp[16]; /* Allow out == in. */

   /* Inverse = adjoint / det. (See linear algebra texts.)*/

   /* pre-compute 2x2 dets for last two rows when computing */
   /* cofactors of first two rows. */
   d12 = (m31*m42-m41*m32);
   d13 = (m31*m43-m41*m33);
   d23 = (m32*m43-m42*m33);
   d24 = (m32*m44-m42*m34);
   d34 = (m33*m44-m43*m34);
   d41 = (m34*m41-m44*m31);

   tmp[0] =  (m22 * d34 - m23 * d24 + m24 * d23);
   tmp[1] = -(m21 * d34 + m23 * d41 + m24 * d13);
   tmp[2] =  (m21 * d24 + m22 * d41 + m24 * d12);
   tmp[3] = -(m21 * d23 - m22 * d13 + m23 * d12);

   /* Compute determinant as early as possible using these cofactors. */
   det = m11 * tmp[0] + m12 * tmp[1] + m13 * tmp[2] + m14 * tmp[3];

   /* Run singularity test. */
   if (det == 0.0) {
      /* printf("invert_matrix: Warning: Singular matrix.\n"); */
	  memcpy(out,_identity,16*sizeof(float));
   }
   else {
      float invDet = 1.0 / det;
      /* Compute rest of inverse. */
      tmp[0] *= invDet;
      tmp[1] *= invDet;
      tmp[2] *= invDet;
      tmp[3] *= invDet;

      tmp[4] = -(m12 * d34 - m13 * d24 + m14 * d23) * invDet;
      tmp[5] =  (m11 * d34 + m13 * d41 + m14 * d13) * invDet;
      tmp[6] = -(m11 * d24 + m12 * d41 + m14 * d12) * invDet;
      tmp[7] =  (m11 * d23 - m12 * d13 + m13 * d12) * invDet;

      /* Pre-compute 2x2 dets for first two rows when computing */
      /* cofactors of last two rows. */
      d12 = m11*m22-m21*m12;
      d13 = m11*m23-m21*m13;
      d23 = m12*m23-m22*m13;
      d24 = m12*m24-m22*m14;
      d34 = m13*m24-m23*m14;
      d41 = m14*m21-m24*m11;

      tmp[8] =  (m42 * d34 - m43 * d24 + m44 * d23) * invDet;
      tmp[9] = -(m41 * d34 + m43 * d41 + m44 * d13) * invDet;
      tmp[10] =  (m41 * d24 + m42 * d41 + m44 * d12) * invDet;
      tmp[11] = -(m41 * d23 - m42 * d13 + m43 * d12) * invDet;
      tmp[12] = -(m32 * d34 - m33 * d24 + m34 * d23) * invDet;
      tmp[13] =  (m31 * d34 + m33 * d41 + m34 * d13) * invDet;
      tmp[14] = -(m31 * d24 + m32 * d41 + m34 * d12) * invDet;
      tmp[15] =  (m31 * d23 - m32 * d13 + m33 * d12) * invDet;

      memcpy(out, tmp, 16*sizeof(float));
   }
}


//
// Invert matrix m.  This algorithm contributed by Stephane Rehel
// <rehel@worldnet.fr>
//

void 
Matrix::invertMatrix( const float *m, float *out )
{
   float det;
   float tmp[16]; /* Allow out == in. */

   if( m41 != 0. || m42 != 0. || m43 != 0. || m44 != 1. ) {
      invertMatrixGeneral(m, out);
      return;
   }

   /* Inverse = adjoint / det. (See linear algebra texts.)*/

   tmp[0]= m22 * m33 - m23 * m32;
   tmp[1]= m23 * m31 - m21 * m33;
   tmp[2]= m21 * m32 - m22 * m31;

   /* Compute determinant as early as possible using these cofactors. */
   det= m11 * tmp[0] + m12 * tmp[1] + m13 * tmp[2];

   /* Run singularity test. */
   if (det == 0.0) {
      /* printf("invert_matrix: Warning: Singular matrix.\n"); */
      memcpy( out, _identity, 16*sizeof(float) );
   }
   else {
      float d12, d13, d23, d24, d34, d41;
      register float im11, im12, im13, im14;

      det= 1. / det;

      /* Compute rest of inverse. */
      tmp[0] *= det;
      tmp[1] *= det;
      tmp[2] *= det;
      tmp[3]  = 0.;

      im11= m11 * det;
      im12= m12 * det;
      im13= m13 * det;
      im14= m14 * det;
      tmp[4] = im13 * m32 - im12 * m33;
      tmp[5] = im11 * m33 - im13 * m31;
      tmp[6] = im12 * m31 - im11 * m32;
      tmp[7] = 0.;

      /* Pre-compute 2x2 dets for first two rows when computing */
      /* cofactors of last two rows. */
      d12 = im11*m22 - m21*im12;
      d13 = im11*m23 - m21*im13;
      d23 = im12*m23 - m22*im13;
      d24 = im12*m24 - m22*im14;
      d34 = im13*m24 - m23*im14;
      d41 = im14*m21 - m24*im11;

      tmp[8] =  d23;
      tmp[9] = -d13;
      tmp[10] = d12;
      tmp[11] = 0.;

      tmp[12] = -(m32 * d34 - m33 * d24 + m34 * d23);
      tmp[13] =  (m31 * d34 + m33 * d41 + m34 * d13);
      tmp[14] = -(m31 * d24 + m32 * d41 + m34 * d12);
      tmp[15] =  1.;

      memcpy(out, tmp, 16*sizeof(float));
  }
}
