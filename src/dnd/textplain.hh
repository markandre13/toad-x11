/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef TDnDTextPlain
#define TDnDTextPlain TDnDTextPlain

#include <toad/connect.hh>
#include <toad/dragndrop.hh>

namespace toad {

class TDnDTextPlain;
typedef GSmartPointer<TDnDTextPlain> PDnDTextPlain;

/**
 * @ingroup dnd
 */
class TDnDTextPlain:
  public TDnDObject
{
  public:
    TDnDTextPlain(const string&);
  
    static bool select(TDnDObject&);
    void flatten();
    static PDnDTextPlain convertData(TDnDObject&);
    
    string text;
};

/**
 * @ingroup dnd
 */
class TDropSiteTextPlain:
  public TDropSite
{
    typedef TDropSite super;
  public:
    TDropSiteTextPlain(TWindow *p):super(p) {};
    TDropSiteTextPlain(TWindow *p, const TRectangle &r):super(p,r) {};
    TSignal sigDrop;
    TDnDTextPlain* getValue() { return value; }
  protected:
    PDnDTextPlain value;
    void dropRequest(TDnDObject&);
    void drop(TDnDObject&);
};

} // namespace toad

#endif
