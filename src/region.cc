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
 * \class toad::TRegion
 *
 * \note
 * TRegion violates the good programming style by accessing Xlibs' internal
 * Region structure for performance reasons.
 * The file "toad/X11/region.h" was part of XFree86 3.2 (X11R6.1) but
 * seems to do well with X11R5 on SUN also.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <toad/toadbase.hh>
#include <toad/region.hh>
#include <toad/X11/region.h>

using namespace toad;

TRegion::TRegion()
{
  TOAD_XLIB_MTLOCK();
  x11region = XCreateRegion();
//cerr << "create x11region " << x11region << endl;
  TOAD_XLIB_MTUNLOCK();
}

TRegion::TRegion(const TRegion &r)
{
  x11region = XCreateRegion();
//cerr << "create x11region " << x11region << endl;
  XUnionRegion(x11region, r.x11region, x11region);
}

void
TRegion::operator=(const TRegion &r)
{
  clear();
  XUnionRegion(x11region, r.x11region, x11region);
}

TRegion::~TRegion()
{
  TOAD_XLIB_MTLOCK();
//err << "destroying X11 region " << x11region << endl;
  XDestroyRegion(x11region);
  TOAD_XLIB_MTUNLOCK();
}

/**
 * Clear region.
 */
void 
TRegion::clear()
{
  static const TRegion empty;
  XIntersectRegion(x11region, empty.x11region, x11region);
}

/**
 * Move region by the specified vector.
 */
void
TRegion::translate(int dx, int dy)
{
  TOAD_XLIB_MTLOCK();
  XOffsetRegion(x11region, dx, dy);
  TOAD_XLIB_MTUNLOCK();
}

/**
 * Return the smallest rectangle containing the region.
 *
 * This can be used to speed up TWindow::paint implementations.
 */
void
TRegion::getBoundary(TRectangle* r) const
{
  XRectangle rect;
  TOAD_XLIB_MTLOCK();
  XClipBox(x11region, &rect);
  TOAD_XLIB_MTUNLOCK();
  
  r->x = rect.x;
  r->y = rect.y;
  r->w = rect.width;
  r->h = rect.height;
}

/**
 * Return the number of rectangles in the region.
 *
 * \sa getRect
 */
long
TRegion::getNumRects() const
{
  return x11region->numRects;
}

/**
 * Returns a single rectangle.
 *
 * \sa getNumRects
 */
bool
TRegion::getRect(long n, TRectangle *r) const
{
  if ( n<0 || n >= x11region->numRects)
    return false;
  r->x = x11region->rects[n].x1;
  r->y = x11region->rects[n].y1;
  r->w = x11region->rects[n].x2 - x11region->rects[n].x1;
  r->h = x11region->rects[n].y2 - x11region->rects[n].y1;
  return true;
}

void
TRegion::operator&=(const TRegion &r)
{
  // avoid segfault in Xlib during XDestroyRegion
  if (x11region==r.x11region) {
    return;
  }
  XIntersectRegion(x11region, r.x11region, x11region);
}

void
TRegion::operator|=(const TRegion &r)
{
  if (x11region==r.x11region)
    return;
  XUnionRegion(x11region, r.x11region, x11region);
}

void
TRegion::operator-=(const TRegion &r)
{
  if (x11region==r.x11region)
    return;
  XSubtractRegion(x11region, r.x11region, x11region);
}

void
TRegion::operator^=(const TRegion &r)
{
  if (x11region==r.x11region)
    return;
  XXorRegion(x11region, r.x11region, x11region);
}

void
TRegion::operator&=(const TRectangle &rect)
{
  TRegion r;
  r|=rect;
  XIntersectRegion(x11region, r.x11region, x11region);
}

void
TRegion::operator|=(const TRectangle &r)
{
  XRectangle rect;
  rect.x=r.x;
  rect.y=r.y;
  rect.width=r.w;
  rect.height=r.h;
  XUnionRectWithRegion(&rect, x11region, x11region);
}

void
TRegion::operator-=(const TRectangle &rect)
{
  TRegion r;
  r|=rect;
  XSubtractRegion(x11region, r.x11region, x11region);
}

void
TRegion::operator^=(const TRectangle &rect)
{
  TRegion r;
  r|=rect;
  XXorRegion(x11region, r.x11region, x11region);
}

/**
 * add a rectangle to the region
 */
void
TRegion::addRect(const TRectangle &r)
{
  addRect(r.x,r.y,r.w,r.h);
};

void
TRegion::addRect(int x,int y,int w,int h)
{
  if (w<0 || h<0) {
    cerr << "TRegion::addRect: warning illegal size\n";
    return;
  }
  XRectangle rect;
  rect.x=x;
  rect.y=y;
  rect.width=w;
  rect.height=h;
  XUnionRectWithRegion(&rect, x11region, x11region);
};

bool
TRegion::isEmpty() const
{
  return XEmptyRegion(x11region);
}

bool
TRegion::isEqual(const TRegion &r) const
{
  return XEqualRegion(r.x11region, x11region);
}

bool
TRegion::isInside(int x, int y) const
{
  return XPointInRegion(x11region, x, y);
}

TRegion::EInside
TRegion::isInside(const TRectangle &r) const
{
  switch(XRectInRegion(x11region, r.x, r.y, r.w, r.h)) {
    case RectangleIn:
      return IN;
    case RectangleOut:
      return OUT;
    case RectanglePart:
      return PART;
  }
  return OUT; // suppress compiler warning
}
