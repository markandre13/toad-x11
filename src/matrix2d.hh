/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2006 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_MATRIX2D_HH
#define _TOAD_MATRIX2D_HH

#include <toad/types.hh>
#include <toad/io/serializable.hh>

namespace toad {

class TMatrix2D:
  public TSerializable
{
    typedef TSerializable super;
    bool _identity;
  public:
    TMatrix2D();
    TMatrix2D(const TMatrix2D&);
    TMatrix2D& operator=(const TMatrix2D&);
    TMatrix2D& operator*=(const TMatrix2D &m) {
      multiply(&m);
      return *this;
    }
    TMatrix2D operator*(const TMatrix2D &m) const;
  
    TCoord a11, a12;
    TCoord a21, a22;
    TCoord tx, ty;

    void identity();
    bool isIdentity() const { return _identity; }
    void rotate(TCoord radiant);
    void rotateAt(TCoord x, TCoord y, TCoord radiant);
    void translate(TCoord, TCoord);

    void scale(TCoord, TCoord);
    void shear(TCoord, TCoord);
    
    void multiply(const TMatrix2D*);
    void invert();

    void set(double a11, double a21, double a12, double a22, double tx, double ty) {
      this->a11 = a11;
      this->a21 = a21;
      this->a12 = a12;
      this->a22 = a22;
      this->tx  = tx;
      this->ty  = ty;
    }
    
    void map(int inX, int inY, short int *outX, short int *outY) const;
    void map(int inX, int inY, int *outX, int *outY) const;
    void map(int inX, int inY, long *outX, long *outY) const;
    void map(int inX, int inY, double *outX, double *outY) const;
    void map(double inX, double inY, double *outX, double *outY) const;
 
    TMatrix2D *next;
    
    SERIALIZABLE_INTERFACE(toad::, TMatrix2D);
};

} // namespace toad

#endif
