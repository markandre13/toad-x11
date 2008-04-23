/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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
/**
 * \file operation.cc
 * \todo
 *   \li
 *     Xft fonts are too big
 *   \li
 *     Xft fonts are always black
 *   \li
 *     better rendering of rotated X11 fonts
 *   \li
 *     rotate bitmaps
 *   \li
 *     rotate arcs
 *   \li
 *     improve quality of rotated arcs
 *   \li
 *     implement shear
 *   \li
 *     make TMatrix2D map virtual in for more complicated transformation
 *   \li
 *     make TMatrix2D serializable
 *   \li
 *     allow multiplication of TMatrix2Ds
 *   \li
 *     (re-)add integer based translations
 */

#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif


#include <assert.h>
#include <cstring>
#include <cmath>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/matrix2d.hh>
#include <toad/window.hh>
#include <toad/region.hh>
#include <toad/bitmap.hh>
#include <iostream>

#ifdef __X11__
#ifdef HAVE_LIBXFT

#ifdef _XFT_NO_COMPAT_
#undef _XFT_NO_COMPAT_
#endif

#include <X11/Xft/Xft.h>

#ifdef FC_VERSION
#define HAVE_FONTCONFIG
#endif

#endif
#endif

using namespace toad;

// Adjust (width, height) to pixel coordinates for XDraw... operations.
#define XDRAW_PIXEL_COORD(w,h) \
  if (!w || !h) \
    return; \
  if (w<0) { \
    w=-w; \
    x-=w-1; \
  } \
  if (h<0) { \
    h=-h; \
    y-=h-1; \
  } \
  w--; \
  h--;

// Adjust (width, height) to raster coordinates for XDraw... operations.
#define XDRAW_RASTER_COORD(w,h) \
  if (w<0) { \
    w=-w; \
    x-=w; \
  } \
  if (h<0) { \
    h=-h; \
    y-=h; \
  }

namespace {

#ifdef __X11__
inline void
tpoint2xpoint(const TPoint *in, XPoint *out, int n, const TMatrix2D *mat) {
  const TPoint *sp = in;
  const TPoint *se = in+n;
  XPoint *dp = out;
  if (!mat) {
    while(sp!=se) {
      dp->x = sp->x;
      dp->y = sp->y;
      ++dp;
      ++sp;
    }
  } else {
    while(sp!=se) {
      mat->map(sp->x, sp->y, &dp->x, &dp->y);
      ++dp;
      ++sp;
    }
  }
}

inline void
polygon2xpoint(const TPolygon &in, XPoint *out, const TMatrix2D *mat) {
  TPolygon::const_iterator sp(in.begin()), se(in.end());
  XPoint *dp = out;
  if (!mat) {
    while(sp!=se) {
      dp->x = sp->x;
      dp->y = sp->y;
      ++dp;
      ++sp;
    }
  } else {
    while(sp!=se) {
      mat->map(sp->x, sp->y, &dp->x, &dp->y);
      ++dp;
      ++sp;
    }
  }
}
#endif

#ifdef __WIN32__
inline void
tpoint2wpoint(const TPoint *in, POINT *out, int n, const TMatrix2D *mat) {
  const TPoint *sp = in;
  const TPoint *se = in+n;
  POINT *dp = out;
  if (!mat) {
    while(sp!=se) {
      dp->x = sp->x;
      dp->y = sp->y;
      ++dp;
      ++sp;
    }
  } else {
    while(sp!=se) {
      mat->map(sp->x, sp->y, &dp->x, &dp->y);
      ++dp;
      ++sp;
    }
  }
}

inline void
polygon2wpoint(const TPolygon &in, POINT *out, const TMatrix2D *mat) {
  TPolygon::const_iterator sp(in.begin()), se(in.end());
  POINT *dp = out;
  if (!mat) {
    while(sp!=se) {
      dp->x = sp->x;
      dp->y = sp->y;
      ++dp;
      ++sp;
    }
  } else {
    while(sp!=se) {
      mat->map(sp->x, sp->y, &dp->x, &dp->y);
      ++dp;
      ++sp;
    }
  }
}
#endif

} // namespace

void
TPen::identity()
{
#ifdef __X11__
  if (mat) {
    mat->identity();
    _setLineAttributes();
  }
#endif

#ifdef __COCOA__
  NSAffineTransform* xform = [NSAffineTransform transform];
  for(mstack_t::const_iterator p = mstack.begin();
      p != mstack.end();
      ++p)
  {
    [xform appendTransform: *p];
  }
  [xform invert];
  [xform concat];
  [mstack.back() appendTransform: xform];
#endif
}

void
TPen::translate(TCoord dx, TCoord dy)
{
#ifdef __X11__
  if (!mat) {
    if (dx==0.0 && dy==0.0)
      return;
    mat = new TMatrix2D();
  }
  mat->translate(dx, dy);
#endif

#ifdef __COCOA__
  NSAffineTransform* xform = [NSAffineTransform transform];
  [xform translateXBy: dx yBy: dy];
  [xform concat];
  [mstack.back() appendTransform: xform];
#endif
}

void
TPen::rotate(double radians)
{
#ifdef __X11__
  if (!mat)
    mat = new TMatrix2D();
  mat->rotate(radians);
#endif

#ifdef __COCOA__
  NSAffineTransform* xform = [NSAffineTransform transform];
  [xform rotateByRadians: radians];
  [xform concat];
  [mstack.back() appendTransform: xform];
#endif
}

void
TPen::scale(TCoord sx, TCoord sy)
{
#ifdef __X11__
  if (!mat)
    mat = new TMatrix2D();
  mat->scale(sx, sy);
  _setLineAttributes();
#endif

#ifdef __COCOA__
  NSAffineTransform* xform = [NSAffineTransform transform];
  [xform scaleXBy: sx yBy: sy];
  [xform concat];
  [mstack.back() appendTransform: xform];
#endif
}

#if 0
void
TPen::shear(double, double)
{
}
#endif

void
TPen::multiply(const TMatrix2D *m)
{
#ifdef __X11__
  assert(m!=0);
  if (!mat) {
    mat = new TMatrix2D(*m);
  } else {
    mat->multiply(m);
    _setLineAttributes();
  }
#endif

#ifdef __COCOA__
  NSAffineTransform* xform = [NSAffineTransform transform];
  NSAffineTransformStruct m0;
  m0.m11 = m->a11;
  m0.m12 = m->a12;
  m0.m21 = m->a21;
  m0.m22 = m->a22;
  m0.tX  = m->tx; 
  m0.tY  = m->ty; 
  [xform setTransformStruct: m0];
  [xform concat];
  [mstack.back() appendTransform: xform];
#endif
}

void
TPen::setMatrix(TCoord a11, TCoord a21, TCoord a12, TCoord a22, TCoord tx, TCoord ty)
{
#ifdef __X11__
  if (!mat)
    mat = new TMatrix2D();
  mat->set(a11, a21, a12, a22, tx, ty);
  _setLineAttributes();
#endif

#ifdef __COCOA__
  identity();
  NSAffineTransform* xform = [NSAffineTransform transform];
  NSAffineTransformStruct m0;
  m0.m11 = a11;
  m0.m12 = a12;
  m0.m21 = a21;
  m0.m22 = a22;
  m0.tX  = tx; 
  m0.tY  = ty; 
  [xform setTransformStruct: m0];
  [xform concat];
  [mstack.back() appendTransform: xform];
#endif
}

#warning "TPen push and pop use a list..."

void
TPen::push()
{
#ifdef __X11__
  if (mat) {
    TMatrix2D *mnew;
    mnew = new TMatrix2D(*mat);
    mnew->next = mat;
    mat = mnew;
  }
#endif

#ifdef __COCOA__
  mstack.push_back([NSAffineTransform transform]);
#endif
}

void
TPen::pop()
{
#ifdef __X11__
  if (mat) {
    TMatrix2D *mold = mat;
    mat = mat->next;
    delete mold;
    _setLineAttributes();
  }
#endif

#ifdef __COCOA__
  [mstack.back() invert];
  [mstack.back() concat];
  mstack.pop_back();
#endif
}

void
TPen::popAll()
{
#ifdef __X11__
  if (!mat)
    return;
  while(mat) {
    TMatrix2D *mold = mat;
    mat = mat->next;
    delete mold;
  }
  _setLineAttributes();
#endif

#ifdef __COCOA__
  cerr << "TPen::popAll is not implemented\n";
#endif
}

// point
//----------------------------------------------------------------------------
void
TPen::drawPoint(TCoord x, TCoord y)
{
#ifdef __X11__
  if (!mat) {
    XDrawPoint(x11display, x11drawable, o_gc, x, y);
  } else {
    short sx, sy;
    mat->map(x, y, &sx, &sy);
    XDrawPoint(x11display, x11drawable, o_gc, sx, sy);
  }
#endif

#ifdef __WIN32__
  activateW32();
  if (!mat) {
    ::SetPixel(w32hdc, x, y, o_color.colorref);
  } else {
    int sx, sy;
    mat->map(x, y, &sx, &sy);
    ::SetPixel(w32hdc, x, y, o_color.colorref);
  }
#endif
}

// line
//----------------------------------------------------------------------------
void
TPen::drawLines(const TPoint *s, size_t n)
{
#ifdef __X11__
  XPoint xp[n];
  tpoint2xpoint(s, xp, n, mat);
  PIXMAP_FIX_001(xp, n)
  XDrawLines(x11display, x11drawable, o_gc, xp, n, CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (n<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  [path moveToPoint: NSMakePoint(s[0].x+0.5, s[0].y+0.5)];
  for(size_t i=1; i<n; ++i)
    [path lineToPoint: NSMakePoint(s[i].x+0.5, s[i].y+0.5)];
  [path stroke];
#endif

#ifdef __WIN32__
  activateW32();
  POINT wp[n+1];
  tpoint2wpoint(s, wp, n, mat);
  
  ::MoveToEx(w32hdc, wp[0].x, wp[0].y, NULL);
  for(int i=1; i<n; ++i)
    ::LineTo(w32hdc, wp[i].x, wp[i].y);
  ::LineTo(w32hdc, wp[n-1].x+1, wp[n-1].y);
/*
  wp[n].x = wp[n-1].x;
  wp[n].y = wp[n-1].y;
  ::Polyline(w32hdc, wp, n+0);
*/
#endif
}

void
TPen::drawLines(const TPolygon &polygon)
{
#ifdef __X11__
  size_t n = polygon.size();
  XPoint xp[n];
  polygon2xpoint(polygon, xp, mat);
  PIXMAP_FIX_001(xp, n)
  XDrawLines(x11display, x11drawable, o_gc, xp, n, CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (polygon.size()<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  [path moveToPoint: NSMakePoint(polygon[0].x, polygon[0].y)];
  for(size_t i=1; i<polygon.size(); ++i)
    [path lineToPoint: NSMakePoint(polygon[i].x, polygon[i].y)];
  [path stroke];
#endif

#ifdef __WIN32__
  activateW32();
  size_t n = polygon.size();
  POINT pts[n];
  polygon2wpoint(polygon, pts, mat);
  ::MoveToEx(w32hdc, pts[0].x, pts[0].y, NULL);
  for(int i=1; i<n; ++i)
    ::LineTo(w32hdc, pts[i].x, pts[i].y);
  ::LineTo(w32hdc, pts[n-1].x+1, pts[n-1].y);
#endif
}

// rectangle
//----------------------------------------------------------------------------
void
TPen::vdrawRectangle(TCoord x, TCoord y, TCoord w, TCoord h)
{
#ifdef __X11__
  XDRAW_RASTER_COORD(w,h)
  if (!mat) {
    if (w==0 || h==0) {
      XDrawLine(x11display, x11drawable, o_gc, x, y, x+w,y+h);
      return;
    }
    XDrawRectangle(x11display, x11drawable, o_gc, x, y, w, h);
  } else {
    XPoint xp[5];
    XPoint *p = xp;
    mat->map(x, y, &p->x, &p->y); ++p;
    mat->map(x+w, y, &p->x, &p->y); ++p;
    mat->map(x+w, y+h, &p->x, &p->y); ++p;
    mat->map(x, y+h, &p->x, &p->y); ++p;
    p->x = xp->x;
    p->y = xp->y;
    PIXMAP_FIX_001(xp, 5)
    XDrawLines(x11display, x11drawable, o_gc, xp, 5, CoordModeOrigin);
  }
#endif

#ifdef __COCOA__
  NSRect r = NSMakeRect(x+0.5,y+0.5,w,h);
  [NSBezierPath strokeRect: r];
#endif

#ifdef __WIN32__
  activateW32();
  XDRAW_RASTER_COORD(w,h)
  POINT pts[5];
  if (!mat) {
    if (w==0 || h==0) {
      // XDrawLine(x11display, x11drawable, o_gc, x, y, x+w,y+h);
      return;
    }
    pts[0].x = x;   pts[0].y = y;
    pts[1].x = x+w; pts[1].y = y;
    pts[2].x = x+w; pts[2].y = y+h;
    pts[3].x = x;   pts[3].y = y+h;
    pts[4].x = x;   pts[4].y = y;
  } else {
    POINT pts[5];
    POINT *p = pts;
    mat->map(x, y, &p->x, &p->y); ++p;
    mat->map(x+w, y, &p->x, &p->y); ++p;
    mat->map(x+w, y+h, &p->x, &p->y); ++p;
    mat->map(x, y+h, &p->x, &p->y); ++p;
    p->x = pts->x;
    p->y = pts->y;
  }
//  ::Polyline(w32hdc, pts, 5);
  ::MoveToEx(w32hdc, pts[0].x, pts[0].y, NULL);
  for(int i=1; i<5; ++i)
    ::LineTo(w32hdc, pts[i].x, pts[i].y);
  ::LineTo(w32hdc, pts[5-1].x+1, pts[5-1].y);
#endif
}

void
TPen::vfillRectangle(TCoord x, TCoord y, TCoord w, TCoord h)
{
  XDRAW_RASTER_COORD(w,h)
#ifdef __X11__
  if (!mat) {
    if (!outline) {
      if (two_colors) {
        XFillRectangle(x11display, x11drawable, f_gc, x, y,w,h);
        XDrawRectangle(x11display, x11drawable, o_gc, x, y,w,h);
      } else {
        XFillRectangle(x11display, x11drawable, o_gc, x,y,w+1,h+1);
      }
    } else {
      XDrawRectangle(x11display, x11drawable, o_gc, x, y,w,h);
    }
  } else {
    XPoint pts[5];
    XPoint *p = pts;
    mat->map(x, y, &p->x, &p->y); ++p;
    mat->map(x+w, y, &p->x, &p->y); ++p;
    mat->map(x+w, y+h, &p->x, &p->y); ++p;
    mat->map(x, y+h, &p->x, &p->y); ++p;
    p->x = pts->x;
    p->y = pts->y;
    if (!outline) {
      XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc,
                   pts, 4, Nonconvex, CoordModeOrigin);
    }
    PIXMAP_FIX_001(pts, 5)
    XDrawLines(x11display, x11drawable, o_gc, pts, 5, CoordModeOrigin);
  }
#endif

#ifdef __COCOA__
  NSRect r = NSMakeRect(x+0.5,y+0.5,w,h);
  [NSBezierPath fillRect: r];
#endif

#ifdef __WIN32__
  activateW32();
  if (!mat) {
    ::Rectangle(w32hdc, x, y, x+w+1, y+h+1);
  } else {
    POINT pts[5];
    POINT *p = pts;
    mat->map(x, y, &p->x, &p->y); ++p;
    mat->map(x+w, y, &p->x, &p->y); ++p;
    mat->map(x+w, y+h, &p->x, &p->y); ++p;
    mat->map(x, y+h, &p->x, &p->y); ++p;
    p->x = pts->x;
    p->y = pts->y;
    ::Polygon(w32hdc, pts, 5);
  }
#endif
}

// circle
//----------------------------------------------------------------------------
/*****************************************************************
The Graphic Gem III
ed. by David Kirk, Academic Press, 1992

Van Aken, Jerry, and Simar, Ray, A Parametric Elliptical Arc Algorithm,
p. 164-172, code: p. 478-479

http://www.acm.org/pubs/tog/GraphicsGems/gemsiii/parelarc.c

Plot a series of points along a PI/2-radian arc of an ellipse.
The arc is specified in terms of a control polygon (a triangle)
with vertices P, Q and K.  The arc begins at P, ends at Q, and is
completely contained within the control polygon.  The draw_point 
function plots a single pixel at display coordinates (x,y).

Entry:
  xP, yP, xQ, yQ, xK, yK -- coordinates of P, Q and K.  These
    are 32-bit fixed-point values with 16 bits of fraction.  
  m -- nonnegative integer that controls spacing between points.
    The angular increment between points is 1/2^m radians.
Exit:
  The number of points plotted is 1 + floor((PI/2)*2^m).
*****************************************************************/

#define PIV2  102944     /* fixed point PI/2 */
#define TWOPI 411775     /* fixed point 2*PI */
#define HALF  32768      /* fixed point 1/2 */ 
typedef long FIX;        /* 32-bit fixed point, 16-bit fraction */

void
map2(const TMatrix2D *m, long &x, long &y)
{
  double ox, oy;
  ox = m->a11 * (double)x + m->a12 * (double)y + m->tx;
  oy = m->a21 * (double)x + m->a22 * (double)y + m->ty;
  x = (long)ox;
  y = (long)oy;
}

#ifdef __X11__
XPoint *
qtr_elips(const TPen *pen, XPoint *p, long xP, long yP, long xQ, long yQ, long xK, long yK, int m)
{
  int i;
  FIX vx, ux, vy, uy, w, xJ, yJ;
  if (pen->mat) {
    map2(pen->mat, xP, yP);
    map2(pen->mat, xQ, yQ);
    map2(pen->mat, xK, yK);
  }
  xP<<=16;   
  yP<<=16;   
  xQ<<=16;   
  yQ<<=16;   
  xK<<=16;   
  yK<<=16;   
  vx = xK - xQ;                 /* displacements from center */
  ux = xK - xP;
  vy = yK - yQ;
  uy = yK - yP;
  xJ = xP - vx + HALF;          /* center of ellipse J */
  yJ = yP - vy + HALF;
  ux -= (w = ux >> (2*m + 3));  /* cancel 2nd-order error */
  ux -= (w >>= (2*m + 4));      /* cancel 4th-order error */
  ux -= w >> (2*m + 3);         /* cancel 6th-order error */
  ux += vx >> (m + 1);          /* cancel 1st-order error */
  uy -= (w = uy >> (2*m + 3));  /* cancel 2nd-order error */
  uy -= (w >>= (2*m + 4));      /* cancel 4th-order error */
  uy -= w >> (2*m + 3);         /* cancel 6th-order error */
  uy += vy >> (m + 1);          /* cancel 1st-order error */
  for (i = (PIV2 << m) >> 16; i >= 0; --i) {
    p->x = (xJ + vx) >> 16;
    p->y = (yJ + vy) >> 16;
    ++p;
    ux -= vx >> m;
    vx += ux >> m;
    uy -= vy >> m;
    vy += uy >> m;
  }
  return p;
}
#endif

void
TPen::vdrawCircle(TCoord x, TCoord y, TCoord w, TCoord h)
{
#ifdef __X11__
  if (!mat) {
    XDRAW_RASTER_COORD(w,h)
    if (w==0 || h==0) {
      XDrawLine(x11display, x11drawable, o_gc, x, y, x+w,y+h);
      return;
    }
    // hmm, seem my X server ignores w,h>=800...
    XDrawArc(x11display, x11drawable, o_gc, x,y,w,h, 0,360*64);
  } else {
    TCoord dw, dh;
    mat->map(w,h, &dw, &dh);
    unsigned long m = lround(pow(max(dw, dh), 0.25));
    ++m;
    if (m<3) m = 3;
    if (m>14) m = 14; // maximum will be 4*102948 points
    unsigned long n = ( ((PIV2<<m) >> 16)+1 )*4;
    XPoint pts[n];
    XPoint *p = pts;
  
    p = qtr_elips(this, p,  x+w/2, y     ,  x+w  , y+h/2,  x+w, y  ,  m);
    p = qtr_elips(this, p,  x+w  , y+h/2 ,  x+w/2, y+h  ,  x+w, y+h,  m);
    p = qtr_elips(this, p,  x+w/2, y+h   ,  x    , y+h/2,  x  , y+h,  m);
    p = qtr_elips(this, p,  x    , y+h/2 ,  x+w/2, y    ,  x  , y  ,  m);
    PIXMAP_FIX_001(pts, n)
    XDrawLines(x11display, x11drawable, o_gc, pts, n, CoordModeOrigin);
  }
#endif

#ifdef __COCOA__
  NSRect r = NSMakeRect(x+0.5,y+0.5,w,h);
  NSBezierPath *path = [NSBezierPath bezierPathWithOvalInRect: r];
  [path stroke];
#endif
}

void
TPen::vfillCircle(TCoord x, TCoord y, TCoord w, TCoord h)
{
#ifdef __X11__
  XDRAW_PIXEL_COORD(w,h)
  if (!mat) {
    if (!outline)
      XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x, y,w,h, 0,360*64);
    XDrawArc(x11display, x11drawable, o_gc, x, y,w,h, 0,360*64);
  } else {
    unsigned long m = lround(pow(max(w, h), 0.25));
    ++m;
    if (m<3) m = 3;
    if (m>14) m = 14; // maximum will be 4*102948 points
  
    int n = ( ((PIV2<<m) >> 16)+1 )*4;
    XPoint pts[n];
    XPoint *p = pts;
  
    p = qtr_elips(this, p,  x+w/2, y     ,  x+w  , y+h/2,  x+w, y  ,  m);
    p = qtr_elips(this, p,  x+w  , y+h/2 ,  x+w/2, y+h  ,  x+w, y+h,  m);
    p = qtr_elips(this, p,  x+w/2, y+h   ,  x    , y+h/2,  x  , y+h,  m);
    p = qtr_elips(this, p,  x    , y+h/2 ,  x+w/2, y    ,  x  , y  ,  m);
    if (!outline) {
      XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
                   pts, n, Nonconvex, CoordModeOrigin);
    }
    PIXMAP_FIX_001(pts, n)
    XDrawLines(x11display, x11drawable, o_gc, pts, n, CoordModeOrigin);
  }
#endif

#ifdef __COCOA__
  NSRect r = NSMakeRect(x+0.5,y+0.5,w,h);
  NSBezierPath *path = [NSBezierPath bezierPathWithOvalInRect: r];
  [path fill];
#endif
}

// arc
//----------------------------------------------------------------------------
void
TPen::vdrawArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2)
{
#ifdef __X11__
  XDRAW_RASTER_COORD(w,h)
  if (w==0 || h==0) {
    XDrawLine(x11display, x11drawable, o_gc, x, y, x+w,y+h);
    return;
  }
  XDrawArc(x11display, x11drawable, o_gc, x,y,w,h, (int)(r1*64.0),(int)(r2*64.0));
#endif
}

void
TPen::vfillArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2)
{
#ifdef __X11__
  XDRAW_RASTER_COORD(w,h)
  XDRAW_PIXEL_COORD(w,h)
  int i1=(int)(r1*64.0);
  int i2=(int)(r2*64.0);
  if (!outline) {
    XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x, y,w,h, i1,i2);
  }
  XDrawArc(x11display, x11drawable, o_gc, x,y,w,h, i1,i2);
#endif
}

#if 0
void
TPenBase::drawArcPC(int x, int y, int w, int h, double r1, double r2)
{
#ifdef __X11__
  XDRAW_PIXEL_COORD(w,h)
  drawArc(x, y, w, h, r1, r2);
#endif
}

void
TPenBase::fillArcPC(int x, int y, int w, int h, double r1, double r2)
{
#ifdef __X11__
  XDRAW_PIXEL_COORD(w,h)
  fillArc(x, y, w, h, r1, r2);
#endif
}
#endif

// polygon
//----------------------------------------------------------------------------
void
TPen::drawPolygon(const TPoint points[], size_t n)
{
#ifdef __X11__
  XPoint pts[n+1];
  tpoint2xpoint(points, pts, n, mat);
  pts[n].x=pts[0].x;
  pts[n].y=pts[0].y;
  PIXMAP_FIX_001(pts, n+1)
  XDrawLines(x11display, x11drawable, o_gc, pts, n+1, CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (n<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  [path moveToPoint: NSMakePoint(points[0].x+0.5, points[0].y+0.5)];
  for(size_t i=1; i<n; ++i)
    [path lineToPoint: NSMakePoint(points[i].x+0.5, points[i].y+0.5)];
  [path closePath];
  [path stroke];   
#endif

#ifdef __WIN32__
  POINT pts[n+1];
  tpoint2wpoint(points, pts, n, mat);
  pts[n].x=pts[0].x;
  pts[n].y=pts[0].y;
//  ::Polyline(w32hdc, pts, n+1);
  ::MoveToEx(w32hdc, pts[0].x, pts[0].y, NULL);
  for(size_t i=1; i<=n; ++i)
    ::LineTo(w32hdc, pts[i].x, pts[i].y);
  ::LineTo(w32hdc, pts[n-1].x+1, pts[n-1].y);
#endif
}

void
TPen::fillPolygon(const TPoint s[], size_t n)
{
#ifdef __X11__
  XPoint d[n];
  tpoint2xpoint(s, d, n, mat);
  if (!outline) {
    XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
      d, n, Nonconvex, CoordModeOrigin);
  }
  XDrawLines(x11display, x11drawable, o_gc, 
      d, n, CoordModeOrigin);
  XDrawLine(x11display, x11drawable, o_gc,
     d[0].x,d[0].y,
     d[n-1].x,d[n-1].y);
  PIXMAP_FIX_001(d, n)
  XDrawLines(x11display, x11drawable, o_gc, 
      d, n, CoordModeOrigin);
#endif

#ifdef __COCOA__
  if (n<2)
    return;
  NSBezierPath *path = [NSBezierPath bezierPath];
  [path moveToPoint: NSMakePoint(s[0].x+0.5, s[0].y+0.5)];
  for(size_t i=1; i<n; ++i)
    [path lineToPoint: NSMakePoint(s[i].x+0.5, s[i].y+0.5)];
  [path closePath];
  [path fill];
#endif

#ifdef __WIN32__
  POINT d[n];
  tpoint2wpoint(s, d, n, mat);
  ::Polygon(w32hdc, d, n);
#endif
}

void
TPen::drawPolygon(const TPolygon &polygon)
{
#ifdef __X11__
  size_t n = polygon.size();
  XPoint d[n];
  polygon2xpoint(polygon, d, mat);
  XDrawLine(x11display, x11drawable, o_gc,
    d[0].x, d[0].y,
    d[n-1].x, d[n-1].y);
  PIXMAP_FIX_001(d, n)
  XDrawLines(x11display, x11drawable, o_gc, 
    d, n, CoordModeOrigin);
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
  [path stroke];
#endif

#ifdef __WIN32__
  size_t n = polygon.size();
  POINT pts[n+1];
  polygon2wpoint(polygon, pts, mat);
  pts[n].x=pts[0].x;
  pts[n].y=pts[0].y;
  // ::Polyline(w32hdc, pts, n+1);
  ::MoveToEx(w32hdc, pts[0].x, pts[0].y, NULL);
  for(size_t i=1; i<=n; ++i)
    ::LineTo(w32hdc, pts[i].x, pts[i].y);
  ::LineTo(w32hdc, pts[n-1].x+1, pts[n-1].y);
#endif
}

void
TPen::fillPolygon(const TPolygon &polygon)
{
#ifdef __X11__
  size_t n = polygon.size();
  XPoint d[n];
  polygon2xpoint(polygon, d, mat);

  if (!outline) {
    XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
      d, n, Nonconvex, CoordModeOrigin);
  }
  XDrawLine(x11display, x11drawable, o_gc,
    d[0].x, d[0].y,
    d[n-1].x, d[n-1].y);
  PIXMAP_FIX_001(d, n)
  XDrawLines(x11display, x11drawable, o_gc, 
    d, n, CoordModeOrigin);
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

#ifdef __WIN32__
  size_t n = polygon.size();
  POINT d[n];
  polygon2wpoint(polygon, d, mat);
  ::Polygon(w32hdc, d, n);
#endif
}

// bitmap
//----------------------------------------------------------------------------
void
TPen::vdrawBitmap(TCoord x, TCoord y, const TBitmap& bmp)
{
#ifdef __X11__
  if (mat) {
    mat->map(x, y, &x, &y);
  }
  bmp.drawBitmap(this, x, y);
#endif

#ifdef __COCOA__
  if (bmp.img==nil)
    return;
  NSAffineTransform* xform = [NSAffineTransform transform];
  [xform translateXBy: x yBy: y+bmp.height];
  [xform scaleXBy: 1.0 yBy: -1.0];
  [xform concat];
  [bmp.img drawAtPoint: NSMakePoint(0, 0)];
  [xform invert];
  [xform concat];
#endif
}

// text string
//----------------------------------------------------------------------------

void
TPen::vdrawString(TCoord x, TCoord y, const char *text, size_t len, bool transparent)
{
#ifdef __COCOA__
  char *t = 0;
  if (strlen(text)!=len) {
    t = strdup(text);
    t[len] = 0;
  }
//cerr<<"vdrawString("<<x<<","<<y<<",\""<<text<<"\","<<len<<","<<transparent<<")\n";
  if (!transparent) {
//cerr << "  not transparent" << endl;
    TRGB stroke2 = stroke, fill2 = fill;
    setColor(fill2.r, fill2.g, fill2.b); 
    fillRectanglePC(x,y,getTextWidth(t?t:text),getHeight());
    setLineColor(stroke2.r, stroke2.g, stroke2.b);
    setFillColor(fill2.r, fill2.g, fill2.b);
  }
  NSDictionary *textAttributes =
    [NSDictionary
      dictionaryWithObject:
        [NSColor colorWithDeviceRed: stroke.r
                              green: stroke.g
                              blue:  stroke.b
                              alpha: 1.0]
      forKey: NSForegroundColorAttributeName];
  [[NSString stringWithUTF8String: t?t:text]  
    drawAtPoint: NSMakePoint(x+0.5, y+0.5)    
    withAttributes: textAttributes];
/*
  [[NSString stringWithUTF8String: text]
    drawAtPoint: NSMakePoint(x+0.5, y+0.5)
    withAttributes: [[NSGraphicsContext currentContext] attributes]];
*/
  if (t)
    free(t);
#endif

#ifdef __X11__
  if (font && font->fontmanager)
    font->fontmanager->drawString(this, x, y, text, len, transparent);
#endif

#ifdef __WIN32__
  if (mat) {
    mat->map(x, y, &x, &y);
  }

  if (transparent) {  
    ::SetBkMode(w32hdc, TRANSPARENT);
  } else {
    ::SetBkMode(w32hdc, OPAQUE);
  }
  ::TextOut(w32hdc, x,y, text, len);
//  ::ExtTextOut(w32hdc, x, y, 0, 0, str, strlen, 0);
//  RECT r;
//  r.left = x;
//  r.top  = y;
//  r.right = 320;
//  r.bottom = 200;
//  ::DrawText(w32hdc, str, strlen, &r, 0);
#endif
}
