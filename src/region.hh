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

#ifndef TRegion
#define TRegion TRegion

#include <toad/os.hh>

namespace toad {

class TRectangle;

#ifdef __WIN32__
#undef IN
#undef OUT
#undef PART
#endif

class TRegion
{
  public:
    enum EInside {
      IN, OUT, PART
    };

    TRegion();
    TRegion(const TRegion&);
    ~TRegion();

    void addRect(const TRectangle&);
    void addRect(int x,int y,int w,int h);
    void operator=(const TRegion&);       // assign
    void operator&=(const TRegion&);      // union
    void operator|=(const TRegion&);      // intersect
    void operator-=(const TRegion&);      // subtract
    void operator^=(const TRegion&);      // xor
    void operator&=(const TRectangle&);   // union
    void operator|=(const TRectangle&);   // intersect
    void operator-=(const TRectangle&);   // subtract
    void operator^=(const TRectangle&);   // xor
    void translate(int dx,int dy);
    void clear();
    void getBoundary(TRectangle*) const;  // getExtents
    void getClipBox(TRectangle *r) const { getBoundary(r); }
    long getNumRects() const;
    bool getRect(long, TRectangle*) const;
    
    bool isEmpty() const;
    bool isEqual(const TRegion &r) const;
    bool isInside(int x, int y) const;
    EInside isInside(const TRectangle &r) const;

    #ifdef __X11__
    _TOAD_REGION x11region;
    #endif
};

} // namespace toad

#endif
