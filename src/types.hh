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

struct TPoint {
  TPoint() {x=y=0;}
  TPoint(int a, int b):x(a),y(b){}
  int x,y;
  void set(int a,int b) { x=a;y=b; }
};

inline ostream& operator<<(ostream &s, const TPoint& p) {
  return s<<'('<<p.x<<','<<p.y<<')';
}

struct TDPoint {
  TDPoint() {x=y=0;}
  TDPoint(double a, double b):x(a),y(b){}
  double x,y;
  void set(double a,double b) { x=a;y=b; }
};

inline ostream& operator<<(ostream &s, const TDPoint& p) {
  return s<<'('<<p.x<<','<<p.y<<')';
}

struct TRectangle {
  int x,y,w,h;
  TRectangle(){set(0,0,0,0);};
  TRectangle(int x,int y,int w,int h){ set(x,y,w,h); }
  TRectangle(const TPoint &p1, const TPoint &p2){ set(p1,p2); }
  void set(int a,int b,int c,int d);
  void set(const TPoint&, const TPoint&);
  void operator =(const TRectangle &r){ x=r.x;y=r.y;w=r.w;h=r.h; }
  void operator =(const TRectangle *r){ x=r->x;y=r->y;w=r->w;h=r->h; }
  bool isInside(int px,int py) const {
    return (x<=px && px<x+w && y<=py && py<y+h);
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
    void addPoint(int x, int y) { push_back(TPoint(x,y)); }
    bool isInside(int x, int y) const;
};

class TDPolygon: 
  public std::vector<TDPoint>
{
  public:
    void addPoint(const TDPoint &p) { push_back(p); }
    void addPoint(double x, double y) { push_back(TDPoint(x,y)); }
};

} // namespace toad

#endif
