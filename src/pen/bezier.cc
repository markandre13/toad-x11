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

// XDrawLines is broken for pixmaps with coordinates <0
// verified on MacOS X, ...
#if 0
#define PIXMAP_FIX_001(xp, n) \
  if (bmp) { \
    for(unsigned i=0; i<n; ++i) { \
      if (xp[i].x < 0) \
        xp[i].x--; \
    } \
  }
#else
#define PIXMAP_FIX_001(xp, n)
#endif

using namespace toad;

#ifdef __X11__
struct TXPoints
{
  private:
    XPoint *p;
    unsigned n, a;
  public:
    TXPoints() {
      p = 0;
      n = a = 0;
    }
    void clear() {
      n = 0;
    }
    void push(int x, int y) {
      if (n>=a) {
        a+=1024;
        p = (XPoint*)realloc(p, a*sizeof(XPoint));
      }
      p[n].x = x;
      p[n].y = y;
      ++n;
    }
    void push(double x, double y) {
      push((int)lround(x), (int)lround(y));
    }
    unsigned size() const { return n; }
    XPoint* ptr() const { return p; }
};
static void xcurve(TXPoints&,double,double,double,double,double,double,double,double);
static TXPoints lst;
static void curve(TPolygon&,double,double,double,double,double,double,double,double);
#endif

void
TPen::drawBezier(TCoord x1, TCoord y1, 
                 TCoord x2, TCoord y2, 
                 TCoord x3, TCoord y3, 
                 TCoord x4, TCoord y4)
{
#ifdef __X11__
  lst.clear();

  if (mat) {
    mat->map(x1, y1, &x1, &y1);
    mat->map(x2, y2, &x2, &y2);
    mat->map(x3, y3, &x3, &y3);
    mat->map(x4, y4, &x4, &y4);
  }

  lst.push(x1, y1);
  xcurve(lst, x1,y1, x2,y2, x3,y3, x4,y4);
  PIXMAP_FIX_001(lst.ptr(), lst.size())
  XDrawLines(x11display, x11drawable, o_gc, lst.ptr(), lst.size(), CoordModeOrigin);
#endif

#ifdef __COCOA__
  assert(false);
#endif
}

#ifdef __X11__
static bool
points2list(TMatrix2D *mat, const TPoint *p, size_t n)
{
  if (n<4)
    return false;

  lst.clear();
  TCoord x, y;
  if (!mat) {
    lst.push(p[0].x, p[0].y);
  } else {
    mat->map(p[0].x, p[0].y, &x, &y);
    lst.push(x, y);
  }

  while(n>=4) {
    if (!mat) {
      xcurve(lst,
             p->x, p->y,
             (p+1)->x, (p+1)->y,
             (p+2)->x, (p+2)->y,
             (p+3)->x, (p+3)->y);
      p+=3;
    } else {
      TCoord px[4], py[4];
      for(int j=1; j<4; ++j) {
        ++p;
        mat->map(p->x, p->y, &px[j], &py[j]);
      }
      xcurve(lst,
             x,     y,
             px[1], py[1],
             px[2], py[2],
             px[3], py[3]);
      x=px[3]; y=py[3];
    }
    n-=3;
  }
  return true;
}
#endif

void
TPen::drawBezier(const TPoint *p, size_t n)
{
#ifdef __X11__
  if (!points2list(mat, p, n))
    return;
  PIXMAP_FIX_001(lst.ptr(), lst.size())
  XDrawLines(x11display, x11drawable, o_gc, lst.ptr(), lst.size(), CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (n<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  NSPoint np[n];
  NSPoint *q = np;
  const TPoint *e = p+n;
  for(; p!=e; ++p, ++q) 
  {
    q->x = p->x;
    q->y = p->y;
  }
  [path appendBezierPathWithPoints: np count:n];
  [path closePath];
  [path stroke];
#endif
}

void
TPen::fillBezier(const TPoint *p, size_t n)
{
#ifdef __X11__
  if (!points2list(mat, p, n))
    return;

  XPoint *d = lst.ptr();
  n = lst.size();
  if (!outline) {
    XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc,
      d, n, Nonconvex, CoordModeOrigin);
  }
  XDrawLine(x11display, x11drawable, o_gc,
     d[0].x,d[0].y,
     d[n-1].x,d[n-1].y);
  PIXMAP_FIX_001(d, n)
  lst.push(d[0].x, d[0].y);
  XDrawLines(x11display, x11drawable, o_gc,
      lst.ptr(), lst.size(), CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (n<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  NSPoint np[n];
  NSPoint *q = np;
  const TPoint *e = p+n;
  for(; p!=e; ++p, ++q) 
  {
    q->x = p->x;
    q->y = p->y;
  }
  [path appendBezierPathWithPoints: np count:n];
  [path closePath];
  [path fill];
#endif
}

#ifdef __X11__
static bool
polygon2list(TMatrix2D *mat, const TPolygon &polygon)
{
  size_t n = polygon.size();
  if (n<4)
    return false;

  lst.clear();
  TCoord x, y;
  if (!mat) {
    lst.push(polygon[0].x, polygon[0].y);
  } else {
    mat->map(polygon[0].x, polygon[0].y, &x, &y);
    lst.push(x, y);
  }

  TPolygon::const_iterator p(polygon.begin());
  while(n>=4) {
    if (!mat) {
      xcurve(lst,
             p->x, p->y,
             (p+1)->x, (p+1)->y,
             (p+2)->x, (p+2)->y,
             (p+3)->x, (p+3)->y);
      p+=3;
    } else {
      TCoord px[4], py[4];
      for(int j=1; j<4; ++j) {
        ++p;
        mat->map(p->x, p->y, &px[j], &py[j]);
      }
      xcurve(lst,
             x,     y,
             px[1], py[1],
             px[2], py[2],
             px[3], py[3]);
      x=px[3]; y=py[3];
    }
    n-=3;
  }
  return true;
}
#endif

void
TPen::drawBezier(const TPolygon &polygon)
{
#ifdef __X11__
  if (!polygon2list(mat, polygon))
    return;
  PIXMAP_FIX_001(lst.ptr(), lst.size())
  XDrawLines(x11display, x11drawable, o_gc, lst.ptr(), lst.size(), CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (polygon.size()<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  NSPoint np[3];
  
  np[0].x = polygon[0].x;
  np[0].y = polygon[0].y;
  [path moveToPoint: np[0]];
  
  TPolygon::const_iterator p = polygon.begin();
  while(true) {
    ++p;
    if (p==polygon.end())
      break;
    np[1].x = p->x;
    np[1].y = p->y;
    ++p;
    if (p==polygon.end())
      break;
    np[2].x = p->x;
    np[2].y = p->y;
    ++p;
    if (p==polygon.end())
      break;
    np[0].x = p->x;
    np[0].y = p->y;
    [path curveToPoint: np[0] controlPoint1: np[1] controlPoint2: np[2]];
  }
  // [path appendBezierPathWithPoints: np count:polygon.size()];
  // [path closePath];
  [path stroke];
#endif
}

void
TPen::fillBezier(const TPolygon &polygon)
{
#ifdef __X11__
  if (!polygon2list(mat, polygon))
    return;

  XPoint *d = lst.ptr();
  size_t n = lst.size();
  if (!outline) {
    XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc,
      d, n, Nonconvex, CoordModeOrigin);
  }
  PIXMAP_FIX_001(d, n)
  lst.push(d[0].x, d[0].y);
  XDrawLines(x11display, x11drawable, o_gc,
      lst.ptr(), lst.size(), CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (polygon.size()<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  NSPoint np[polygon.size()];
  NSPoint *q = np;
  for(TPolygon::const_iterator p = polygon.begin();
      p != polygon.end();
      ++p, ++q)
  {
    q->x = p->x;
    q->y = p->y;
  }
  [path appendBezierPathWithPoints: np count:polygon.size()];
  [path closePath];
  [path fill];
#endif
}

#ifdef __X11__
inline double
mid(double a, double b)
{
  return (a + b) / 2.0;
}

#define WEIGHT 4.0

static void xcurve(
  TXPoints &poly,
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

  if (fabs(w0)+fabs(w1)+fabs(w2)+fabs(w3)<WEIGHT) {
    poly.push(x0, y0);
    poly.push(x1, y1);
    poly.push(x2, y2);
    poly.push(x3, y3);
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
    xcurve(poly, x0, y0, x11, y11, x12, y12, cx, cy);
    xcurve(poly, cx, cy, x21, y21, x22, y22, x3, y3);
  }
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

  if (fabs(w0)+fabs(w1)+fabs(w2)+fabs(w3)<WEIGHT) {
    poly.push_back(TPoint(lround(x0), lround(y0)));
    poly.push_back(TPoint(lround(x1), lround(y1)));
    poly.push_back(TPoint(lround(x2), lround(y2)));
    poly.push_back(TPoint(lround(x3), lround(y3)));
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
#endif
