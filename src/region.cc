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

/**
 * \class toad::TRegion
 *
 * \note
 * TRegion violates the good programming style by accessing Xlibs' internal
 * Region structure for performance reasons.
 * The file "toad/X11/region.h" was part of XFree86 3.2 (X11R6.1) but
 * seems to do well with X11R5 on SUN also.
 */

#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#endif

#include <toad/toadbase.hh>
#include <toad/region.hh>

#ifdef __X11__
#include <toad/X11/region.h>
#endif

using namespace toad;

namespace toad {
  bool debug_region = false;
}

TRegion::TRegion()
{
#ifdef __X11__
  x11region = XCreateRegion();
  if (debug_region)
    cerr << "create x11region " << x11region << " [" << this << "]\n";
#endif
}

TRegion::TRegion(const TRegion &r)
{
#ifdef __X11__
  x11region = XCreateRegion();
  if (debug_region)
    cerr << "create x11region " << x11region << " [" << this << "]\n";
  XUnionRegion(x11region, r.x11region, x11region);
#endif
}

void
TRegion::operator=(const TRegion &r)
{
#ifdef __X11__
  clear();
  XUnionRegion(x11region, r.x11region, x11region);
#endif
}

TRegion::~TRegion()
{
#ifdef __X11__
  if (debug_region)
    cerr << "destroying X11 region " << x11region << " [" << this << "]\n";
  XDestroyRegion(x11region);
#endif
}

/**
 * Clear region.
 */
void 
TRegion::clear()
{
#ifdef __X11__
  static const TRegion empty;
  XIntersectRegion(x11region, empty.x11region, x11region);
#endif
}

/**
 * Move region by the specified vector.
 */
void
TRegion::translate(int dx, int dy)
{
#ifdef __X11__
  XOffsetRegion(x11region, dx, dy);
#endif
}

/**
 * Return the smallest rectangle containing the region.
 *
 * This can be used to speed up TWindow::paint implementations.
 */
void
TRegion::getBoundary(TRectangle* r) const
{
#ifdef __X11__
  XRectangle rect;
  XClipBox(x11region, &rect);
  
  r->x = rect.x;
  r->y = rect.y;
  r->w = rect.width;
  r->h = rect.height;
#endif
}

/**
 * Return the number of rectangles in the region.
 *
 * \sa getRect
 */
long
TRegion::getNumRects() const
{
#ifdef __X11__
  return x11region->numRects;
#endif

#ifdef __WIN32__
  return 0;
#endif
}

/**
 * Returns a single rectangle.
 *
 * \sa getNumRects
 */
bool
TRegion::getRect(long n, TRectangle *r) const
{
#ifdef __X11__
  if ( n<0 || n >= x11region->numRects)
    return false;
  r->x = x11region->rects[n].x1;
  r->y = x11region->rects[n].y1;
  r->w = x11region->rects[n].x2 - x11region->rects[n].x1;
  r->h = x11region->rects[n].y2 - x11region->rects[n].y1;
#endif
  return true;
}

void
TRegion::operator&=(const TRegion &r)
{
#ifdef __X11__
  // avoid segfault in Xlib during XDestroyRegion
  if (x11region==r.x11region) {
    return;
  }
  XIntersectRegion(x11region, r.x11region, x11region);
#endif
}

void
TRegion::operator|=(const TRegion &r)
{
#ifdef __X11__
  if (x11region==r.x11region)
    return;
  XUnionRegion(x11region, r.x11region, x11region);
#endif
}

void
TRegion::operator-=(const TRegion &r)
{
#ifdef __X11__
  if (x11region==r.x11region)
    return;
  XSubtractRegion(x11region, r.x11region, x11region);
#endif
}

void
TRegion::operator^=(const TRegion &r)
{
#ifdef __X11__
  if (x11region==r.x11region)
    return;
  XXorRegion(x11region, r.x11region, x11region);
#endif
}

void
TRegion::operator&=(const TRectangle &rect)
{
#ifdef __X11__
  TRegion r;
  r|=rect;
  XIntersectRegion(x11region, r.x11region, x11region);
#endif
}

void
TRegion::operator|=(const TRectangle &r)
{
#ifdef __X11__
  XRectangle rect;
  rect.x=r.x;
  rect.y=r.y;
  rect.width=r.w;
  rect.height=r.h;
  XUnionRectWithRegion(&rect, x11region, x11region);
#endif
}

void
TRegion::operator-=(const TRectangle &rect)
{
#ifdef __X11__
  TRegion r;
  r|=rect;
  XSubtractRegion(x11region, r.x11region, x11region);
#endif
}

void
TRegion::operator^=(const TRectangle &rect)
{
#ifdef __X11__
  TRegion r;
  r|=rect;
  XXorRegion(x11region, r.x11region, x11region);
#endif
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
#ifdef __X11__
  XRectangle rect;
  rect.x=x;
  rect.y=y;
  rect.width=w;
  rect.height=h;
  XUnionRectWithRegion(&rect, x11region, x11region);
#endif
};

bool
TRegion::isEmpty() const
{
#ifdef __X11__
  return XEmptyRegion(x11region);
#endif
}

bool
TRegion::isEqual(const TRegion &r) const
{
#ifdef __X11__
  return XEqualRegion(r.x11region, x11region);
#endif
}

bool
TRegion::isInside(int x, int y) const
{
#ifdef __X11__
  return XPointInRegion(x11region, x, y);
#endif
}

TRegion::EInside
TRegion::isInside(const TRectangle &r) const
{
#ifdef __X11__
  switch(XRectInRegion(x11region, r.x, r.y, r.w, r.h)) {
    case RectangleIn:
      return IN;
    case RectangleOut:
      return OUT;
    case RectanglePart:
      return PART;
  }
#endif
  return OUT; // suppress compiler warning
}
