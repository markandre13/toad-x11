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

#ifndef TDnDColor
#define TDnDColor TDnDColor

#include <toad/dragndrop.hh>

namespace toad {

class TDnDColor;
typedef GSmartPointer<TDnDColor> PDnDColor;

/**
 * @ingroup dnd
 */
class TDnDColor:
  public TDnDObject
{
  public:
    TDnDColor(const TRGB&);
    ~TDnDColor();
    static bool select(TDnDObject&);
    
    void flatten();
    static PDnDColor convertData(TDnDObject&);
    TRGB rgb;
};

/**
 * @ingroup dnd
 */
class TDropSiteColor:
  public TDropSite
{
    typedef TDropSite super;
  public:
    TDropSiteColor(TWindow *p):super(p) {};
    TDropSiteColor(TWindow *p, const TRectangle &r):super(p,r) {};
    TSignal sigDrop;
    TDnDColor* getValue() { return value; }
  protected:
    PDnDColor value;
    void dropRequest(TDnDObject&);
    void drop(TDnDObject&);
};

} // namespace toad

#endif
