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

#ifndef TScrolledArea
#define TScrolledArea TScrolledArea

#ifndef TScrollBar
#include <toad/scrollbar.hh>
#endif

namespace toad {

class TScrolledArea: 
  public TWindow
{
  public:
    TScrolledArea(TWindow *p, const string &t);
    void setArea(int aw, int ah, int x=0, int y=0, int iw=1, int ih=1);
    void setItemSize(int iw,int ih);
    void setVisibleAreaPos(int x,int y);
    void setVisibleAreaSize(int w,int h);
    void setAreaSize(int,int);
    void clipPen(TPen &pen);

    bool bAlwaysVertical:1;
    bool bAlwaysHorizontal:1;
    
//  protected:
    void resize();
    
    int item_w, item_h;   // size of each item
    int area_w, area_h;   // size of area by number of items
    int area_x, area_y;   // position of area by items
    int visi_w, visi_h;   // visible part of the area by number of items

//  private:
    TScrollBar *vscroll, *hscroll;
    void actVScroll();
    void actHScroll();
  public:
    void pArrangeSB();
    
};

} // namespace toad

#endif
