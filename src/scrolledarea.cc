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

#include <toad/toad.hh>
#include <toad/scrollbar.hh>
#include <toad/region.hh>
#include <toad/scrolledarea.hh>

/**
 * \class toad::TScrolledArea
 *
 * <B>Note</B>: This is going to become obsolete in the future.
 * <OL>
 *   <LI> size of an item
 *   <LI> size of the area by count of items
 *   <LI> position of the upper left corner by count of items
 * </OL>
 */

namespace toad {

// constructor
//-------------------------------------
TScrolledArea::TScrolledArea(TWindow *p, const string &t):
  TWindow(p,t)
{
  setArea(0,0);
  hscroll = vscroll = NULL;
  bAlwaysVertical = bAlwaysHorizontal = false;
}

// SetArea
//-------------------------------------
void 
TScrolledArea::setArea(int aw, int ah, int x, int y, int iw, int ih)
{
  area_w=aw; area_h=ah;
  area_x=x;  area_y=y;
  item_w=iw; item_h=ih;
  visi_w = visi_h = 0;
  
  if (isRealized()) {
    pArrangeSB();
    invalidateWindow();
  }
}

void TScrolledArea::setItemSize(int w,int h)
{
  item_w = w;
  item_h = h;
}

void TScrolledArea::setVisibleAreaPos(int x,int y)
{
  area_x = x;
  area_y = y;
}

void TScrolledArea::setAreaSize(int w,int h)
{
  area_w = w;
  area_h = h;
}

void TScrolledArea::setVisibleAreaSize(int w,int h)
{
  #warning "this is a hack"
  setSize(w*item_w+TScrollBar::getFixedSize(),h*item_h);
}

// actVScroll
//-------------------------------------
void TScrolledArea::actVScroll()
{
  int ny = vscroll->getValue();   // new position
  int dy = ny - area_y;           // movement
  area_y = ny;                    // set new position
  if (abs(dy)<visi_h)             // part of the area is still visible
  {
    scrollWindow(0,-dy*item_h);         // generates 'paint' event
  } else {
    invalidateWindow();
  }
}

// actHScroll
//-------------------------------------
void TScrolledArea::actHScroll()
{
  int nx = hscroll->getValue();   // new position
  int dx = nx - area_x;           // movement
  area_x = nx;                    // set new position
  if (abs(dx)<visi_w)             // part of the area is still visible
  {
    scrollWindow(-dx*item_w,0);   // generates 'paint' event
  } else {
    invalidateWindow();
  }
}

// pArrangeSB
//-------------------------------------
void TScrolledArea::pArrangeSB()
{
  // calculate visible area
  //------------------------
  visi_w = getWidth() / item_w;
  visi_h = getHeight() / item_h;
  
  // test which scrollbars are neccessary
  //--------------------------------------
  bool need_h, need_v;
  need_h = need_v = false;

  if (bAlwaysVertical || visi_h<area_h) // need vertical scrollbar
  {
    need_v = true;
    visi_w = (getWidth()-TScrollBar::getFixedSize()) / item_w;
  }
  
  if (bAlwaysHorizontal || visi_w < area_w) // need horizontal scrollbar
  {
    need_h = true;
    visi_h = (getHeight()-TScrollBar::getFixedSize()) / item_h;
  }
  
  // after adding only a horizontal scrollbar a vertical may be needed now
  if (!need_v && visi_h < area_h) {
    need_v = true;
    visi_w = (getWidth()-TScrollBar::getFixedSize()) / item_w;
  }
  
  // ensure that most of the area is visible
  //-----------------------------------------
  if (area_y+visi_h > area_h) {
    area_y = area_h - visi_h;
    if (area_y < 0)
      area_y = 0;
  }

  if (area_x+visi_w > area_w) {
    area_x = area_w - visi_w;
    if (area_x < 0)
      area_x = 0;
  }

  // create and destroy scrollbars
  //-------------------------------
  // NOTE: since a window is wasting about ?bytes of server memory,
  //       i believe it's a good idea to destroy them when not needed
  if (need_v) {
    if (!vscroll) {
      vscroll = new TScrollBar(this,"");
      vscroll->bNoFocus = true;
      CONNECT(vscroll->getModel()->sigChanged, this,actVScroll);
    } else {
      need_v = false;
    }
  } else {
    if (vscroll) {
      delete vscroll;
      vscroll = NULL;
    }
  }

  if (need_h) {
    if (!hscroll) {
      hscroll = new TScrollBar(this,"");
      hscroll->bNoFocus = true;
      CONNECT(hscroll->getModel()->sigChanged, this,actHScroll);
    } else {
      need_h = false;
    }
  } else {
    if (hscroll) {
      delete hscroll;
      hscroll = NULL;
    }
  }

  // arrange scrollbars
  //--------------------
  if (vscroll) {
    vscroll->setShape(
      getWidth()-TScrollBar::getFixedSize(),
      0,
      TSIZE_PREVIOUS,
      (hscroll ? getHeight()-TScrollBar::getFixedSize() : getHeight())
    );
    vscroll->getModel()->setRangeProperties(area_y, visi_h, 0,area_h-1);
  }

  if (hscroll) {
    hscroll->setShape(
      0,
      getHeight()-TScrollBar::getFixedSize(),
      vscroll ? getWidth()-TScrollBar::getFixedSize() : getWidth(),
      TSIZE_PREVIOUS
    );
    hscroll->getModel()->setRangeProperties(area_x, visi_w, 0,area_w-1);
  }
  
  // create new scrollbars
  //-----------------------
  if (isRealized() && need_v)
    vscroll->createWindow();
    
  if (isRealized() && need_h)
    hscroll->createWindow();
}

// resize
//--------
void TScrolledArea::resize()
{
  pArrangeSB();
}

void TScrolledArea::clipPen(TPen &pen)
{
  if (vscroll && hscroll) {
    TRectangle r(0,0,
      getWidth()-TScrollBar::getFixedSize(),
      getHeight()-TScrollBar::getFixedSize() );
    if (bNoBackground) {
      TPen pen2(this);
      pen.setColor(TColor::DIALOG);
      pen.fillRectangle(getWidth()-TScrollBar::getFixedSize(),
                        getHeight()-TScrollBar::getFixedSize(),
                        TScrollBar::getFixedSize(),
                        TScrollBar::getFixedSize() );
    } 
    pen&=r;
  }
}

} // namespace toad
