/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

/**
 * \file operation.cc
 * \todo
 *   \li
 *     rotate fonts
 *   \li
 *     rotate bitmaps
 *   \li
 *     rotate arcs
 *   \li
 *     improve quality of rotated arcs
 *   \li
 *     implement shear
 *   \li
 *     move TMatrix2D definition into a separate header file
 *   \li
 *     make TMatrix2D map virtual in for more complicated transformation
 *   \li
 *     add TPenBase::push(TMatrix2D*)
 *   \li
 *     make TMatrix2D serializable
 *   \li
 *     allow multiplication of TMatrix2Ds
 *   \li
 *     (re-)add integer based translations
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <assert.h>
#include <cstring>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/region.hh>
#include <iostream>

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

class toad::TMatrix2D
{
  public:
    TMatrix2D();
  
    double a11, a12;
    double a21, a22;
    double tx, ty;

    void identity();
    void rotate(double);
    void translate(double, double);
    void scale(double, double);
    void shear(double, double);
    void set(double a11, double a12, double a21, double a22, double tx, double ty) {
      this->a11 = a11;
      this->a12 = a12;
      this->a21 = a21;
      this->a22 = a22;
      this->tx  = tx;
      this->ty  = ty;
    }
    
    void map(int inX, int inY, short int *outX, short int *outY) const;
 
    TMatrix2D *next;
};

namespace {

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

} // namespace

TMatrix2D::TMatrix2D()
{
  identity();
  next = 0;  
}

void
TMatrix2D::identity()
{
  a11 = a22 = 1.0;
  a21 = a12 = tx = ty = 0.0;
}
 
/**
 * \pre
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
}

/**
 * \pre
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
 
void
TMatrix2D::scale(double xfactor, double yfactor)
{
  a11 *= xfactor;
  a12 *= xfactor;
  a21 *= yfactor;
  a22 *= yfactor;
}

void
TMatrix2D::shear(double, double)
{
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
  *outX = a11 * x + a12 * y + tx;
  *outY = a21 * x + a22 * y + ty;
}

void
TPen::identity()
{
  if (mat)
    mat->identity();
}

void
TPen::translate(double dx, double dy)
{
  if (!mat) {
    if (dx==0.0 && dy==0.0)
      return;
    mat = new TMatrix2D();
  }
  mat->translate(dx, dy);
}

void
TPen::rotate(double degree)
{
  if (!mat)
    mat = new TMatrix2D();
  mat->rotate(degree);
}

void
TPen::scale(double xfactor, double yfactor)
{
  if (!mat)
    mat = new TMatrix2D();
  mat->scale(xfactor, yfactor);
}

void
TPen::shear(double, double)
{
}

void
TPen::setMatrix(double a11, double a12, double a21, double a22, double tx, double ty)
{
  if (!mat)
    mat = new TMatrix2D();
  mat->set(a11, a12, a21, a22, tx, ty);
}

void
TPen::push()
{
  if (mat) {
    TMatrix2D *mnew = new TMatrix2D(*mat);
    mnew->next = mat;
    mat = mnew;
  }
}

void
TPen::pop()
{
  if (mat) {
    TMatrix2D *mold = mat;
    mat = mat->next;
    delete mold;
  }
}

void
TPen::popAll()
{
  while(mat) {
    TMatrix2D *mold = mat;
    mat = mat->next;
    delete mold;
  }
}

// point
//----------------------------------------------------------------------------
void
TPen::drawPoint(int x, int y) const
{
  if (!mat) {
    XDrawPoint(x11display, x11drawable, o_gc, x, y);
  } else {
    short sx, sy;
    mat->map(x, y, &sx, &sy);
    XDrawPoint(x11display, x11drawable, o_gc, sx, sy);
  }
}

// line
//----------------------------------------------------------------------------
void
TPen::vdrawLine(int x1, int y1, int x2, int y2) const
{
  if (!mat) {
    XDrawLine(x11display, x11drawable, o_gc, x1,y1, x2, y2);
  } else {
    short sx1, sy1, sx2, sy2;
    mat->map(x1, y1, &sx1, &sy1);
    mat->map(x2, y2, &sx2, &sy2);
    XDrawLine(x11display, x11drawable, o_gc, sx1,sy1, sx2, sy2);
  }
}



void
TPen::drawLines(const TPoint *s, int n) const
{
  XPoint xp[n];
  tpoint2xpoint(s, xp, n, mat);
  XDrawLines(x11display, x11drawable, o_gc, xp, n, CoordModeOrigin);
}

void
TPen::drawLines(const TPolygon &polygon) const
{
  unsigned n = polygon.size();
  XPoint xp[n];
  polygon2xpoint(polygon, xp, mat);
  XDrawLines(x11display, x11drawable, o_gc, xp, n, CoordModeOrigin);
}

// rectangle
//----------------------------------------------------------------------------
void
TPen::vdrawRectangle(int x, int y, int w, int h) const
{
  XDRAW_RASTER_COORD(w,h)
  if (!mat) {
    if (w==0 || h==0) {
      XDrawLine(x11display, x11drawable, o_gc, x, y, x+w,y+h);
      return;
    }
    XDrawRectangle(x11display, x11drawable, o_gc, x, y, w, h);
  } else {
    XPoint pts[5];
    XPoint *p = pts;
    mat->map(x, y, &p->x, &p->y); ++p;
    mat->map(x+w, y, &p->x, &p->y); ++p;
    mat->map(x+w, y+h, &p->x, &p->y); ++p;
    mat->map(x, y+h, &p->x, &p->y); ++p;
    p->x = pts->x;
    p->y = pts->y;
    XDrawLines(x11display, x11drawable, o_gc, pts, 5, CoordModeOrigin);
  }
}

void
TPen::vfillRectangle(int x, int y, int w, int h) const
{
  XDRAW_RASTER_COORD(w,h)
  if (!mat) {
    if (two_colors) {
      XFillRectangle(x11display, x11drawable, f_gc, x, y,w,h);
      XDrawRectangle(x11display, x11drawable, o_gc, x, y,w,h);
    } else {
      XFillRectangle(x11display, x11drawable, o_gc, x,y,w+1,h+1);
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
    XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc,
                 pts, 4, Nonconvex, CoordModeOrigin);
    XDrawLines(x11display, x11drawable, o_gc, pts, 5, CoordModeOrigin);
  }
}

void
TPenBase::drawRectanglePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h);
  drawRectangle(x,y,w,h);
}

void
TPenBase::fillRectanglePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  fillRectangle(x, y, w, h);
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

XPoint *
qtr_elips(const TPen *pen, XPoint *p, long xP, long yP, long xQ, long yQ, long xK, long yK, int m)
{
  int i, x, y;
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

void
TPen::vdrawCircle(int x, int y, int w, int h) const
{
  if (!mat) {
    XDRAW_RASTER_COORD(w,h)
    if (w==0 || h==0) {
      XDrawLine(x11display, x11drawable, o_gc, x, y, x+w,y+h);
      return;
    }
    // hmm, seem my X server ignores w,h>=800...
    XDrawArc(x11display, x11drawable, o_gc, x,y,w,h, 0,360*64);
  } else {
    int m = pow(max(w, h), 0.25);
    ++m;
    if (m<3) m = 3;
    if (m>16) m = 15;
  
    int n = ( ((PIV2<<m) >> 16)+1 )*4;
    XPoint pts[n];
    XPoint *p = pts;
  
    p = qtr_elips(this, p,  x+w/2, y     ,  x+w  , y+h/2,  x+w, y  ,  m);
    p = qtr_elips(this, p,  x+w  , y+h/2 ,  x+w/2, y+h  ,  x+w, y+h,  m);
    p = qtr_elips(this, p,  x+w/2, y+h   ,  x    , y+h/2,  x  , y+h,  m);
    p = qtr_elips(this, p,  x    , y+h/2 ,  x+w/2, y    ,  x  , y  ,  m);
    XDrawLines(x11display, x11drawable, o_gc, pts, n, CoordModeOrigin);
  }
}

void
TPen::vfillCircle(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  if (!mat) {
    XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x, y,w,h, 0,360*64);
    XDrawArc(x11display, x11drawable, o_gc, x, y,w,h, 0,360*64);
  } else {
    int m = pow(max(w, h), 0.25);
    ++m;
    if (m<3) m = 3;
    if (m>16) m = 15;
  
    int n = ( ((PIV2<<m) >> 16)+1 )*4;
    XPoint pts[n];
    XPoint *p = pts;
  
    p = qtr_elips(this, p,  x+w/2, y     ,  x+w  , y+h/2,  x+w, y  ,  m);
    p = qtr_elips(this, p,  x+w  , y+h/2 ,  x+w/2, y+h  ,  x+w, y+h,  m);
    p = qtr_elips(this, p,  x+w/2, y+h   ,  x    , y+h/2,  x  , y+h,  m);
    p = qtr_elips(this, p,  x    , y+h/2 ,  x+w/2, y    ,  x  , y  ,  m);
    
    XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
                 pts, n, Nonconvex, CoordModeOrigin);
    XDrawLines(x11display, x11drawable, o_gc, pts, n, CoordModeOrigin);
  }
}

void
TPenBase::drawCirclePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  drawCircle(x, y, w, h);
}

void
TPenBase::fillCirclePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  fillCircle(x, y, w, h);
}

// arc
//----------------------------------------------------------------------------
void
TPen::vdrawArc(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_RASTER_COORD(w,h)
  if (w==0 || h==0) {
    XDrawLine(x11display, x11drawable, o_gc, x, y, x+w,y+h);
    return;
  }
  XDrawArc(x11display, x11drawable, o_gc, x,y,w,h, (int)(r1*64.0),(int)(r2*64.0));
}

void
TPen::vfillArc(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_RASTER_COORD(w,h)
  XDRAW_PIXEL_COORD(w,h)
  int i1=(int)(r1*64.0);
  int i2=(int)(r2*64.0);
  XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x, y,w,h, i1,i2);
  XDrawArc(x11display, x11drawable, o_gc, x,y,w,h, i1,i2);
}

void
TPenBase::drawArcPC(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_PIXEL_COORD(w,h)
  drawArc(x, y, w, h, r1, r2);
}

void
TPenBase::fillArcPC(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_PIXEL_COORD(w,h)
  fillArc(x, y, w, h, r1, r2);
}

// polygon
//----------------------------------------------------------------------------
void
TPen::drawPolygon(const TPoint points[], int n) const
{
  drawLines(points, n);
  drawLine(points[0].x,points[0].y,points[n-1].x,points[n-1].y);
}

void
TPen::fillPolygon(const TPoint s[], int n) const
{
  XPoint d[n];
  tpoint2xpoint(s, d, n, mat);
  XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
    d, n, Nonconvex, CoordModeOrigin);
  XDrawLines(x11display, x11drawable, o_gc, 
      d, n, CoordModeOrigin);
  XDrawLine(x11display, x11drawable, o_gc,
     s[0].x,s[0].y,
     s[n-1].x,s[n-1].y);
}

void
TPen::drawPolygon(const TPolygon &polygon) const
{
  unsigned n = polygon.size();
  XPoint d[n];
  polygon2xpoint(polygon, d, mat);

  XDrawLines(x11display, x11drawable, o_gc, 
    d, n, CoordModeOrigin);
  XDrawLine(x11display, x11drawable, o_gc,
    d[0].x, d[0].y,
    d[n-1].x, d[n-1].y);
}

void
TPen::fillPolygon(const TPolygon &polygon) const
{
  unsigned n = polygon.size();
  XPoint d[n];
  polygon2xpoint(polygon, d, mat);

  XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
    d, n, Nonconvex, CoordModeOrigin);
  XDrawLines(x11display, x11drawable, o_gc, 
    d, n, CoordModeOrigin);
  XDrawLine(x11display, x11drawable, o_gc,
    d[0].x, d[0].y,
    d[n-1].x, d[n-1].y);
}

// bitmap
//----------------------------------------------------------------------------
void
TPen::drawBitmap(int x, int y, const TBitmap* bmp) const
{
  if (mat) {
    x+=mat->tx;
    y+=mat->ty;
  }
  bmp->drawBitmap(this, x, y);
}

void
TPen::drawBitmap(int x, int y, const TBitmap& bmp) const
{
  if (mat) {
    x+=mat->tx;
    y+=mat->ty;
  }
  bmp.drawBitmap(this, x, y);
}

void
TPen::drawBitmap(int x, int y, const TBitmap* bmp, int ax, int ay, int aw, int ah) const
{
  if (mat) {
    x+=mat->tx;
    y+=mat->ty;
    ax+=mat->tx;
    ay+=mat->ty;
  }
  bmp->drawBitmap(this, x, y, ax,ay,aw,ah);
}

void
TPen::drawBitmap(int x, int y, const TBitmap& bmp, int ax, int ay, int aw, int ah) const
{
  if (mat) {
    x+=mat->tx;
    y+=mat->ty;
    ax+=mat->tx;
    ay+=mat->ty;
  }
  bmp.drawBitmap(this, x, y, ax,ay,aw,ah);
}

// 3D rectangle
//----------------------------------------------------------------------------
/**
 * This is a special function for widgets.
 */
void
TPen::vdraw3DRectangle(int x, int y, int w, int h, bool inset) const
{
  TColor saved_color = o_color;
  
  ++w;
  ++h;
  
  TPen *t = const_cast<TPen*>(this);
  
  TPoint p[3];
  if (inset)
    t->setColor(255,255,255);
  else
    t->setColor(0,0,0);
  p[0].set(x+1  ,y+h-1);
  p[1].set(x+w-1,y+h-1);
  p[2].set(x+w-1,y);
  drawLines(p,3);

  if (inset)  
    t->setColor(192,192,192);
  else
    t->setColor(128,128,128);
  p[0].set(x+2  ,y+h-2);
  p[1].set(x+w-2,y+h-2);
  p[2].set(x+w-2,y+1);
  drawLines(p,3);

  if (inset)
    t->setColor(128,128,128);
  else
    t->setColor(192,192,192);
  p[0].set(x    ,y+h-1);
  p[1].set(x    ,y);
  p[2].set(x+w-1,y);
  drawLines(p,3);
  
  if (inset)
    t->setColor(0,0,0);
  else
    t->setColor(255,255,255);
  p[0].set(x+1  ,y+h-2);
  p[1].set(x+1  ,y+1);
  p[2].set(x+w-2,y+1);
  drawLines(p,3);
  
  t->setColor(saved_color);
}

void
TPenBase::draw3DRectanglePC(int x, int y, int w, int h, bool inset) const
{
  XDRAW_PIXEL_COORD(w,h)
  vdraw3DRectangle(x, y, w, h, inset);
}

// text string
//----------------------------------------------------------------------------
int
TPen::getTextWidth(const string &str) const
{
  return font->getTextWidth(str.c_str());
}

int
TPen::getTextWidth(const char *str) const
{
  return font->getTextWidth(str);
}

/**
 * Width of 'str' when printed with the current font.
 */
int
TPen::getTextWidth(const char *str, int len) const
{
  return font->getTextWidth(str,len);
}

/**
 * Ascent of the current font.
 */
int
TPen::getAscent() const
{
  return font->getAscent();
}

/**
 * Descent of the current font.
 */
int
TPen::getDescent() const
{
  return font->getDescent();
}

/**
 * Height of the current font.
 */
int
TPen::getHeight() const
{
  return font->getHeight();
}

/**
 * Draw string `str'. <VAR>x</VAR>, <VAR>y</VAR> is the upper left
 * coordinate of the string.<BR>
 * DrawString is a little bit slower than FillString.
 */
void
TPen::drawString(int x,int y, const string &str) const
{
  TPen::drawString(x,y,str.c_str(),(int)str.size());
}

void
TPen::drawString(int x,int y, const char *str, int strlen) const
{
  if (!str)
    return;
  if (mat) {
    x+=mat->tx;
    y+=mat->ty;
  }
  XDrawString(x11display, x11drawable, o_gc, x,y+getAscent(), str, strlen);
}

/**
 * FillString will fill the background with the current back color when
 * drawing the string.<BR>
 * The back color can be set with SetBackColor.<BR>
 * Please note that FillString doesn't support color dithering and will
 * use the nearest color TOAD was able to allocate.<BR>
 * Maybe i'm going to rename this method into `PrintString' since
 * `FillString' is really a very idiotic name.
 */
void
TPen::fillString(int x,int y, const string &str) const
{
  TPen::fillString(x,y,str.c_str(),(int)str.size());
}

void
TPen::fillString(int x,int y, const char *str, int strlen) const
{
  if (!str)
    return;
  if (mat) {
    x+=mat->tx;
    y+=mat->ty;
  }
  XDrawImageString(x11display, x11drawable, o_gc, x,y+getAscent(), str, strlen);
}

/**
 * Draw string 'str' in multiple lines, reduce spaces between words to one 
 * an break lines to fit width. 'str' can contain '\n'.
 */
int
TPen::drawTextWidth(int x,int y,const string &str, unsigned width) const
{
  const char* text=str.c_str();
  
  unsigned i;

  if (mat) {
    x+=mat->tx;
    y+=mat->ty;
  }
  
  // 1st step: count words and lines
  unsigned word_count, min_lines;
  font->count_words_and_lines(text, &word_count, &min_lines);
  if (!word_count) return 0;
  
  // 2nd step: create a word list
  TFont::TWord* word = font->make_wordlist(text, word_count);
  
  // 3rd step: output
  unsigned blank_width = getTextWidth(" ",1);
  unsigned line_len = 0;
  unsigned word_of_line = 1;
  
  for(i=0; i<word_count; i++)
    {
      if ((line_len+word[i].len>width && i!=0) || word[i].linefeeds)
  {
    if (word[i].linefeeds)
      y+=getHeight()*word[i].linefeeds;
    else
      y+=getHeight();
    line_len = 0;
    word_of_line = 0;
  }
      drawString(x+line_len,y, word[i].pos, word[i].bytes);
      line_len+=word[i].len+blank_width;
      word_of_line++;
    }
  
  delete[] word;
  return y+getHeight();
}
