/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <toad/matrix2d.hh>
#include <cmath>

// missing in mingw
#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi */
#endif

using namespace toad;

TMatrix2D::TMatrix2D()
{
  identity();
  next = 0;  
}

TMatrix2D::TMatrix2D(const TMatrix2D &m)
{
  *this = m;
  next = 0;  
}

TMatrix2D&
TMatrix2D::operator=(const TMatrix2D &m)
{
  a11 = m.a11;
  a12 = m.a12;
  a21 = m.a21;
  a22 = m.a22;
  tx  = m.tx;
  ty  = m.ty;
  _identity = m._identity;
  return *this;
}

/**
 * Reset matrix to 'no transformation'.
 *
 * \pre
    / a11 a12  tx \      / 1.0 0.0 0.0 \
   |  a21 a22  ty  | := |  0.0 1.0 0.0  |
    \ 0.0 0.0 1.0 /      \ 0.0 0.0 1.0 /
   \endpre
 */
void
TMatrix2D::identity()
{
  a11 = a22 = 1.0;
  a21 = a12 = tx = ty = 0.0;
  _identity = true;
}
 
/**
 * PostScript alike rotation of the coordinate system with the rotation
 * matrix multiplied after the current matrix.
 *
 * \pre
  M' := M * R
 
   / a11 a12  tx \       / a11 a12  tx \     / r11 r12 0.0 \
  |  a21 a22  ty  | :=  |  a21 a22  ty  | * |  r21 r22 0.0  |
   \ 0.0 0.0 1.0 /       \ 0.0 0.0 1.0 /     \ 0.0 0.0 1.0 / 
   \endpre
 */
void
TMatrix2D::rotate(double degree)
{
  double a = degree / 360.0 * 2.0 * M_PI;
  double r11, r12, r21, r22;
  r11 = r22 = cos(a);
  r21 = sin(a);
  r12 = -r21;  

  double n11 = a11 * r11 + a12 * r21;
  double n21 = a21 * r11 + a22 * r21;

  double n12 = a11 * r12 + a12 * r22;
  double n22 = a21 * r12 + a22 * r22;

  double ntx = tx;
  double nty = ty;
  
  a11 = n11;
  a21 = n21;
  a12 = n12;
  a22 = n22;
  tx = ntx; 
  ty = nty; 
  
  _identity = false;
}

/**
 * PostScript alike tranlation of the coordinate system with the
 * translation matrix multiplied after the current matrix.
 *
 * \pre
  M' := M * T
 
   / a11 a12  tx \      / a11 a12  tx \     / 1.0 0.0   x \
  |  a21 a22  ty  | := |  a21 a22  ty  | * |  0.0 1.0   y  |
   \ 0.0 0.0 1.0 /      \ 0.0 0.0 1.0 /     \ 0.0 0.0 1.0 / 
   \endpre
 */
void
TMatrix2D::translate(double x, double y)
{
  tx += a11 * x + a12 * y;
  ty += a21 * x + a22 * y;
}

/**
 * Rotate coordinate system around point (x, y).
 *
 * \pre
   M' := -T * R * T * M
   \endpre
 */
void
TMatrix2D::rotateAt(double x, double y, double degree)
{
  double a = degree / 360.0 * 2.0 * M_PI;
  double r11, r12, r21, r22;
  r11 = r22 = cos(a);
  r21 = sin(a);
  r12 = -r21;  

  double n11 = a11 * r11 + a21 * r12;
  double n21 = a11 * r21 + a21 * r22;
  double n12 = a12 * r11 + a22 * r12;
  double n22 = a12 * r21 + a22 * r22;

  double ntx = r11 * (tx-x) + r12 * (ty-y) + x;
  double nty = r21 * (ty-y) + r22 * (ty-y) + x;
  
  a11 = n11;
  a21 = n21;
  a12 = n12;
  a22 = n22;
  tx = ntx; 
  ty = nty; 
  
  _identity = false;
}

/**
 *
 */
void
TMatrix2D::scale(double xfactor, double yfactor)
{
  a11 *= xfactor;
  a12 *= xfactor;
  a21 *= yfactor;
  a22 *= yfactor;
  
  _identity = false;
}

void
TMatrix2D::shear(double, double)
{
}

/**
 *
 * \pre
   / a11 a12  tx \       / a11 a12  tx \     / r11 r12  rx \
  |  a21 a22  ty  | :=  |  a21 a22  ty  | * |  r21 r22  ry  |
   \ 0.0 0.0 1.0 /       \ 0.0 0.0 1.0 /     \ 0.0 0.0 1.0 / 
   \endpre
 */
void
TMatrix2D::multiply(const TMatrix2D *m)
{
  double n11 = a11 * m->a11 + a12 * m->a21;
  double n21 = a21 * m->a11 + a22 * m->a21;

  double n12 = a11 * m->a12 + a12 * m->a22;
  double n22 = a21 * m->a12 + a22 * m->a22;
#if 1
  // toad 0.64.0
  double ntx = a11 * m->tx + a12 * m->ty + tx;
  double nty = a21 * m->tx + a22 * m->ty + ty;
#else  
  // cairo
  double ntx =  tx    * m->a11 + ty    * m->a12 + m->tx;
  double nty =  tx    * m->a21 + ty    * m->a22 + b->ty;
#endif  
  a11 = n11;
  a21 = n21;
  a12 = n12;
  a22 = n22;
  tx = ntx; 
  ty = nty; 
  
  _identity = false;
}
 
/**
 * \pre
   / outY \      / a11 a12  tx \     / inX \
  |  outX  | := |  a21 a22  ty  | * |  inY  |
   \ 1.0  /      \ 0.0 0.0 1.0 /     \ 1.0 / 
   \endpre
 */
void
TMatrix2D::map(int inX, int inY, short int *outX, short int *outY) const
{
  double x, y;
  x = inX; y=inY;
  *outX = static_cast<short int>(lround(a11 * x + a12 * y + tx));
  *outY = static_cast<short int>(lround(a21 * x + a22 * y + ty));
}

void
TMatrix2D::map(int inX, int inY, int *outX, int *outY) const
{
  double x, y;
  x = inX; y=inY;
  *outX = static_cast<int>(lround(a11 * x + a12 * y + tx));
  *outY = static_cast<int>(lround(a21 * x + a22 * y + ty));
}

void
TMatrix2D::map(int inX, int inY, long *outX, long *outY) const
{
  double x, y;
  x = inX; y=inY;
  *outX = lround(a11 * x + a12 * y + tx);
  *outY = lround(a21 * x + a22 * y + ty);
}

void
TMatrix2D::map(int inX, int inY, double *outX, double *outY) const
{
  double x, y;
  x = inX; y=inY;
  *outX = a11 * x + a12 * y + tx;
  *outY = a21 * x + a22 * y + ty;
}

void
TMatrix2D::map(double inX, double inY, double *outX, double *outY) const
{
  double x, y;
  x = inX; y=inY;
  *outX = a11 * x + a12 * y + tx;
  *outY = a21 * x + a22 * y + ty;
}

/**
 * Invert the matrix.
 *
 * \pre
          -1
   A' := A 
   \endpre
 */
void
TMatrix2D::invert()
{
  double d = 1.0 / (a11 * a22 - a12 * a21);
  double n11 = d * a22;
  double n21 = d * -a21;
  double n12 = d * -a12;
  double n22 = d * a11;
  double nx  = d * (a12 * ty - a22 * tx);
  double ny  = d * (a21 * tx - a11 * ty);
  
  a11 = n11;
  a21 = n21;
  a12 = n12;
  a22 = n22;
  tx  = nx;
  ty  = ny;
  _identity = false;
}

void
TMatrix2D::store(TOutObjectStream &out) const
{
  ::store(out, a11);
  ::store(out, a21);
  ::store(out, a12);
  ::store(out, a22);
  ::store(out, tx);
  ::store(out, ty);
}

bool
TMatrix2D::restore(TInObjectStream &in)
{
  _identity = false;
  if (
    ::restore(in, 0, &a11) ||
    ::restore(in, 1, &a21) ||
    ::restore(in, 2, &a12) ||
    ::restore(in, 3, &a22) ||
    ::restore(in, 4, &tx) ||
    ::restore(in, 5, &ty) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in);
  return false;
}
