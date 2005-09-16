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

#ifndef TDnDImage
#define TDnDImage TDnDImage

#include <toad/dragndrop.hh>

namespace toad {

class TDnDImage;
typedef GSmartPointer<TDnDImage> PDnDImage;

/**
 * @ingroup dnd
 */
class TDnDImage:
  public TDnDObject
{
  public:
/*  dragging will be available after the rewrite of TBitmap:
    TDnDImage(TBitmap *bmp);
    void flatten();
*/
    static bool select(TDnDObject&);
    static PDnDImage convertData(TDnDObject&);

    PBitmap bmp;
};

} // namespace toad

#endif
