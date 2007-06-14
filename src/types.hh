/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_TYPES_HH
#define _TOAD_TYPES_HH

#include <iostream>
#include <vector>

typedef unsigned long ulong;
typedef unsigned char ubyte;
typedef unsigned char byte;

namespace toad {

using namespace std;

// typedef float TCoord; // cocoa
typedef double TCoord; // cairo
inline TCoord abs(toad::TCoord x) { return x<0.0 ? -x : x; }

struct TPoint {
  TPoint() {x=y=0;}
  TPoint(TCoord a, TCoord b):x(a),y(b){}
  TCoord x,y;
  void set(TCoord a, TCoord b) { x=a;y=b; }
};

inline ostream& operator<<(ostream &s, const TPoint& p) {
  return s<<'('<<p.x<<','<<p.y<<')';
}

struct TRectangle {
  TCoord x, y, w, h;
  TRectangle(){set(0,0,0,0);};
  TRectangle(TCoord x,TCoord y,TCoord w,TCoord h){ set(x,y,w,h); }
  TRectangle(const TPoint &p1, const TPoint &p2){ set(p1,p2); }
  void set(TCoord a,TCoord b,TCoord c,TCoord d);
  void set(const TPoint&, const TPoint&);
  void operator =(const TRectangle &r){ x=r.x;y=r.y;w=r.w;h=r.h; }
  void operator =(const TRectangle *r){ x=r->x;y=r->y;w=r->w;h=r->h; }
  bool isInside(TCoord px, TCoord py) const {
    return (x<=px && px<=x+w && y<=py && py<=y+h);
  }
  bool intersects(const TRectangle &r) const;
};

inline ostream& operator<<(ostream &s, const TRectangle& r) {
  return s<<'('<<r.x<<','<<r.y<<','<<r.w<<','<<r.h<<')';
}

class TPolygon: 
  public std::vector<TPoint>
{
  public:
    void addPoint(const TPoint &p) { push_back(p); }
    void addPoint(TCoord x, TCoord y) { push_back(TPoint(x,y)); }
    bool isInside(TCoord x, TCoord y) const;
    bool getShape(TRectangle *r) const;
};

} // namespace toad

#endif
