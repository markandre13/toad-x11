#ifndef MATH_MATRIX4_H
#define MATH_MATRIX4_H

/*

  GLT OpenGL C++ Toolkit (LGPL)
  Copyright (C) 2000-2002 Nigel Stewart  
  Email: nigels@nigels.com   
  WWW:   http://www.nigels.com/glt/

  Minor adjustments for usage within MakeTree
  Copyright (C) 2010 Mark-André Hopf

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*! \file 
    \brief   4x4 Matrix 
	\ingroup Math
*/

#include <iosfwd>

// #include <math/real.h>
#include "vector3.h"

////////////////////////// Matrix /////////////////////////////////

/*! \class   Matrix 
    \brief   4x4 Matrix
	\ingroup Math
	\author  Nigel Stewart, RMIT (nigels@nigels.com)
	\todo    Nice pictures and explanation for 4x4 transformation matrices.
*/

//class UnMatrix;

class Matrix
{
	friend Matrix matrixScale(const float sf);
	friend Matrix matrixScale(const Vector sf);
	friend Matrix matrixTranslate(const Vector trans);
	friend Matrix matrixTranslate(const real x,const real y,const real z);
	friend Matrix matrixRotate(const Vector axis,const float angle);
	friend Matrix matrixRotate(const float azimuth,const float elevation);
	friend Matrix matrixOrient(const Vector &x,const Vector &y,const Vector &z);
	friend Matrix matrixOrient(const Vector &direction,const Vector &up);

	friend std::ostream &operator<<(std::ostream &os,const Matrix &m);
	friend std::istream &operator>>(std::istream &is,      Matrix &m);

public:

	/// Default constructor
	Matrix();
	/// Copy constructor
	Matrix(const Matrix &matrix);
	/// Construct from array 
	Matrix(const float *matrix);
	/// Construct from array 
	Matrix(const double *matrix);
	/// Construct from OpenGL GL_MODELVIEW_MATRIX or GL_PROJECTION_MATRIX
	Matrix(const unsigned int glMatrix);
	/// Construct from string
//	Matrix(const std::string &str);

	/// Assignment operator
	Matrix &operator=(const Matrix &);

	/// Matrix multiplication
	Matrix  operator*(const Matrix &) const;
	/// In-place matrix multiplication
	Matrix &operator*=(const Matrix &);

	/// Matrix transformation of 3D vector
	Vector operator*(const Vector &) const;

	/// Reset to identity matrix
	void reset();
	/// Reset to identity matrix
	void identity();

	/// Is this matrix identity?
	bool isIdentity() const;

	/// Access i'th element of matrix
	      float &operator[](const int i);
	/// Access i'th element of matrix
	const float &operator[](const int i) const;

	/// Access as array
	operator float * ();
	/// Access as array
	operator const float * () const;

	/// Equality operator
	bool operator==(const Matrix &) const;
	/// Not-equal operator
	bool operator!=(const Matrix &) const;

	/// Calculate matrix inverse
	Matrix inverse() const;
	/// Calculate matrix transpose
	Matrix transpose() const;
	/// Calculate unmatrix
//	UnMatrix unmatrix() const;
	/// Calculate matrix determinant
	float det() const;

	/// Mult current OpenGL matrix 
	void glMultMatrix() const;
	/// Load current OpenGL matrix
	void glLoadMatrix() const;

	/// Write matrix in Povray format
//	std::ostream &writePov(std::ostream &os) const;

private:

	float _matrix[16];
	static float _identity[16];

	inline void set(const int col,const int row,const float val) 
	{ 
		_matrix[col*4+row] = val;
	}

	inline float get(const int col,const int row) const
	{
		return _matrix[col*4+row];
	}

	inline float &element(const int col,const int row) 
	{
		return _matrix[col*4+row];
	}

	// From Mesa-2.2\src\glu\project.c

	static void invertMatrixGeneral( const float *m, float *out );
	static void invertMatrix( const float *m, float *out );

	// From Graphics Gems GEMSI\MATINV.C

	float 
	det3x3
	( 
		const float a1, 
		const float a2, 
		const float a3, 
		const float b1, 
		const float b2, 
		const float b3, 
		const float c1, 
		const float c2, 
		const float c3 
	) const;

	float
	det2x2
	( 
		const float a, 
		const float b, 
		const float c, 
		const float d
	) const;
};

Matrix matrixScale(const float sf);
Matrix matrixScale(const Vector sf);
Matrix matrixTranslate(const Vector trans);
Matrix matrixTranslate(const real x,const real y,const real z);
Matrix matrixRotate(const Vector axis,const float angle);
Matrix matrixRotate(const float azimuth,const float elevation);
Matrix matrixOrient(const Vector &x,const Vector &y,const Vector &z);
Matrix matrixOrient(const Vector &direction,const Vector &up);

#endif
