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

#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#define _TOAD_PRIVATE

#include <toad/toad.hh>
#include <vector>
#include <cmath>

using namespace toad;

#warning "translation in _dx,_dy is ignored"

static void curve(TPolygon&,double,double,double,double,double,double,double,double);
static void fcurve(TDPolygon&,double,double,double,double,double,double,double,double);

static TPolygon lst;

void
TPen::drawBezier(int x1,int y1, 
                 int x2,int y2, 
                 int x3,int y3, 
                 int x4,int y4)
{
  lst.erase(lst.begin(), lst.end());
  lst.push_back(TPoint(x1, y1));
  curve(lst, x1,y1, x2,y2, x3,y3, x4,y4);
  #warning "obsure code"
  drawLines(&*lst.begin(), lst.size());
}

void
TPen::drawBezier(double x1,double y1, 
                 double x2,double y2, 
                 double x3,double y3, 
                 double x4,double y4)
{
  lst.erase(lst.begin(), lst.end());
  lst.push_back(TPoint((int)x1, (int)y1));
  curve(lst, x1,y1, x2,y2, x3,y3, x4,y4);
  #warning "obscure code"
  drawLines(&*lst.begin(), lst.size());
}

void
TPen::drawPolyBezier(const TPoint *p, int n)
{
  lst.erase(lst.begin(), lst.end());
  lst.push_back(p[0]);
  n-=3;
  int i=0;
  while(i<=n) {
    curve(lst,
          p[i].x, p[i].y,
          p[i+1].x, p[i+1].y,
          p[i+2].x, p[i+2].y,
          p[i+3].x, p[i+3].y);
    i+=3;
  }
  #warning "obscure code"
  drawLines(&*lst.begin(), lst.size());
}

void
TPen::drawPolyBezier(const TDPoint *p, int n)
{
  lst.erase(lst.begin(), lst.end());
  lst.push_back(TPoint(p[0].x, p[0].y));
  n-=3;
  int i=0;
  while(i<=n) {
    curve(lst,
          p[i].x, p[i].y,
          p[i+1].x, p[i+1].y,
          p[i+2].x, p[i+2].y,
          p[i+3].x, p[i+3].y);
    i+=3;
  }
  #warning "obscure code"
  drawLines(&*lst.begin(), lst.size());
}

void
TPen::fillPolyBezier(const TPoint *p, int n)
{
  lst.erase(lst.begin(), lst.end());
  lst.push_back(p[0]);
  n-=3;
  int i=0;
  while(i<=n) {
    curve(lst,
          p[i].x, p[i].y,
          p[i+1].x, p[i+1].y,
          p[i+2].x, p[i+2].y,
          p[i+3].x, p[i+3].y);
    i+=3;
  }
  #warning "obscure code"
  fillPolygon(&*lst.begin(), lst.size());
}

void
TPen::fillPolyBezier(const TDPoint *p, int n)
{
  lst.erase(lst.begin(), lst.end());
  lst.push_back(TPoint(p[0].x, p[0].y));
  n-=3;
  int i=0;
  while(i<=n) {
    curve(lst,
          p[i].x, p[i].y,
          p[i+1].x, p[i+1].y,
          p[i+2].x, p[i+2].y,
          p[i+3].x, p[i+3].y);
    i+=3;
  }
  #warning "obscure code"
  fillPolygon(&*lst.begin(), lst.size());
}

void
TPen::drawPolyBezier(const TPolygon &polygon)
{
  unsigned n = polygon.size();
  if (n<4)
    return;
  
  TPolygon::const_iterator p(polygon.begin());

  lst.erase(lst.begin(), lst.end());
  lst.push_back(*p);
  
  while(n>=4) {
    curve(lst,
          p->x, p->y,
          (p+1)->x, (p+1)->y,
          (p+2)->x, (p+2)->y,
          (p+3)->x, (p+3)->y);
    p+=3;
    n-=3;
  }
  #warning "obscure code"
  drawLines(&*lst.begin(), lst.size());
}

void
TPen::drawPolyBezier(const TDPolygon &)
{
  #warning "not implemented: TPen::drawPolyBezier(const TDPolygon &)"
}

void
TPen::fillPolyBezier(const TPolygon &polygon)
{
  unsigned n = polygon.size();
  if (n<4)
    return;
  
  TPolygon::const_iterator p(polygon.begin());

  lst.erase(lst.begin(), lst.end());
  lst.push_back(*p);
  
  while(n>=4) {
    curve(lst,
          p->x, p->y,
          (p+1)->x, (p+1)->y,
          (p+2)->x, (p+2)->y,
          (p+3)->x, (p+3)->y);
    p+=3;
    n-=3;
  }
  fillPolygon(&*lst.begin(), lst.size());
}

void
TPen::fillPolyBezier(const TDPolygon &)
{
  #warning "not implemented: TPen::fillPolyBezier(const TDPolygon &)"
}

/**
 * This method converts a TDPolygon into a TDPolygon bezier curve and
 * offers an opportunity for more sophisticated operations on bezier
 * curves. E.g. calculating the curves bounding box, rotation, etc.
 */
void 
TPenBase::poly2Bezier(const TPoint* src, int n, TPolygon &dst)
{
  dst.erase(dst.begin(), dst.end());
  dst.push_back(TPoint(src[0].x, src[0].y));
  n-=3;
  int i=0;
  while(i<=n) {
    curve(dst,
          src[i].x,   src[i].y,
          src[i+1].x, src[i+1].y,
          src[i+2].x, src[i+2].y,
          src[i+3].x, src[i+3].y);
    i+=3;
  }
}

void 
TPenBase::poly2Bezier(const TPolygon &src, TPolygon &dst)
{
  dst.erase(dst.begin(), dst.end());
  dst.addPoint(src[0]);
  unsigned n = src.size();
  n-=3;
  unsigned i=0;
  while(i<=n) {
    curve(dst,
          src[i].x,   src[i].y,
          src[i+1].x, src[i+1].y,
          src[i+2].x, src[i+2].y,
          src[i+3].x, src[i+3].y);
    i+=3;
  }
}

void 
TPenBase::poly2Bezier(const TDPoint* src, int n, TDPolygon &dst)
{
  dst.erase(dst.begin(), dst.end());
  dst.push_back(TDPoint(src[0].x, src[0].y));
  n-=3;
  int i=0;
  while(i<=n) {
    fcurve(dst,
          src[i].x,   src[i].y,
          src[i+1].x, src[i+1].y,
          src[i+2].x, src[i+2].y,
          src[i+3].x, src[i+3].y);
    i+=3;
  }
}

// This is almost the same algorithm as used by the Fresco Toolkit but I've 
// removed a bug and improved it's speed [MAH]

inline double
mid(double a, double b)
{
  return (a + b) / 2.0;
}

static void curve(
  TPolygon &poly,
  double x0, double y0, 
  double x1, double y1,
  double x2, double y2,
  double x3, double y3)
{
  double vx0 = x1-x0;
  double vx1 = x2-x1;
  double vx2 = x3-x2;
  double vy0 = y1-y0;
  double vy1 = y2-y1;
  double vy2 = y3-y2;
  double vx3 = x2-x0;
  double vx4 = x3-x0;
  double vy3 = y2-y0;
  double vy4 = y3-y0;

  double w0 = vx0 * vy1 - vy0 * vx1;
  double w1 = vx1 * vy2 - vy1 * vx2;
  double w2 = vx3 * vy4 - vy3 * vx4;
  double w3 = vx0 * vy4 - vy0 * vx4;

  if (fabs(w0)+fabs(w1)+fabs(w2)+fabs(w3)<80.0) {
    poly.push_back(TPoint((int)x0, (int)y0));
    poly.push_back(TPoint((int)x1, (int)y1));
    poly.push_back(TPoint((int)x2, (int)y2));
    poly.push_back(TPoint((int)x3, (int)y3));
  } else {
    double xx  = mid(x1, x2);
    double yy  = mid(y1, y2);
    double x11 = mid(x0, x1);
    double y11 = mid(y0, y1);
    double x22 = mid(x2, x3);
    double y22 = mid(y2, y3);
    double x12 = mid(x11, xx);
    double y12 = mid(y11, yy);
    double x21 = mid(xx, x22);
    double y21 = mid(yy, y22);
    double cx  = mid(x12, x21);
    double cy  = mid(y12, y21);
    curve(poly, x0, y0, x11, y11, x12, y12, cx, cy);
    curve(poly, cx, cy, x21, y21, x22, y22, x3, y3);
  }
}

static void fcurve(
  TDPolygon &poly,
  double x0, double y0, 
  double x1, double y1,
  double x2, double y2,
  double x3, double y3)
{
  double vx0 = x1-x0;
  double vx1 = x2-x1;
  double vx2 = x3-x2;
  double vy0 = y1-y0;
  double vy1 = y2-y1;
  double vy2 = y3-y2;
  double vx3 = x2-x0;
  double vx4 = x3-x0;
  double vy3 = y2-y0;
  double vy4 = y3-y0;

  double w0 = vx0 * vy1 - vy0 * vx1;
  double w1 = vx1 * vy2 - vy1 * vx2;
  double w2 = vx3 * vy4 - vy3 * vx4;
  double w3 = vx0 * vy4 - vy0 * vx4;

  if (fabs(w0)+fabs(w1)+fabs(w2)+fabs(w3)<80.0) {
    poly.push_back(TDPoint(x0, y0));
    poly.push_back(TDPoint(x1, y1));
    poly.push_back(TDPoint(x2, y2));
    poly.push_back(TDPoint(x3, y3));
  } else {
    double xx  = mid(x1, x2);
    double yy  = mid(y1, y2);
    double x11 = mid(x0, x1);
    double y11 = mid(y0, y1);
    double x22 = mid(x2, x3);
    double y22 = mid(y2, y3);
    double x12 = mid(x11, xx);
    double y12 = mid(y11, yy);
    double x21 = mid(xx, x22);
    double y21 = mid(yy, y22);
    double cx  = mid(x12, x21);
    double cy  = mid(y12, y21);
    fcurve(poly, x0, y0, x11, y11, x12, y12, cx, cy);
    fcurve(poly, cx, cy, x21, y21, x22, y22, x3, y3);
  }
}
