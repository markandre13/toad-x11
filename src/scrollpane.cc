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
#include <toad/scrollpane.hh>

#define DBM(X)

/**
 * \class toad::TScrollPane
 */

using namespace toad;

// constructor
//-------------------------------------
TScrollPane::TScrollPane(TWindow *p, const string &t):
  TWindow(p,t)
{
  vscroll = hscroll = 0;
  uix = uiy = 1;
  resetScrollPane();
}

void
TScrollPane::resetScrollPane()
{
  lx = ly = 0;
  if (vscroll)
    vscroll->setValue(0);
  if (hscroll)
    hscroll->setValue(0);
}

void
TScrollPane::_scrolled()
{
  int dx, dy;
  dx = dy = 0;
  if (hscroll) {
    int n = hscroll->getValue();
    dx = lx - n;
    lx = n;
  }
  if (vscroll) {
    int n = vscroll->getValue();  
    dy = ly - n;
    ly = n;
  }
  scrollRectangle(visible, dx, dy, true);
  
  if (visible.x) {
    TRectangle r(0,visible.y,visible.x,visible.h);
    scrollRectangle(r, 0, dy, true);
  }
  
  if (visible.y) {
    TRectangle r(visible.x,0,visible.w,visible.y);
    scrollRectangle(r, dx, 0, true);
  } 

  scrolled(dx, dy);
}

void
TScrollPane::scrolled(int dx, int dy)
{
}

/**
 * Configure the scrollbars according the window and pane size.
 */
void
TScrollPane::doLayout()
{
  visible.set(0,0,getWidth(), getHeight());
  adjustPane(); // adjust 'visible'

  bool need_hscroll = false;
  bool need_vscroll = false;

  if (pane.w > visible.w || pane.x < 0) {
    need_hscroll = true;  
    visible.h -= TScrollBar::getFixedSize()-1;
  }
   
  if (pane.h > visible.h || pane.y < 0) {
    need_vscroll = true;  
    visible.w -= TScrollBar::getFixedSize()-1;
  }
   
  if (!need_hscroll && pane.w > visible.w) {
    need_hscroll = true;
    visible.h -= TScrollBar::getFixedSize()-1;
  }
  
  DBM(cout
      << "doLayout:" << endl
      << "visible  = "<<visible<<endl
      << "pane     = "<<pane<<endl
      << "need h,v = "<<need_hscroll<<", "<<need_vscroll<<endl;)

  if (need_vscroll) {
    if (!vscroll) {  
      vscroll = new TScrollBar(this, "vertical");
      connect(vscroll->getModel()->sigChanged, this, &TScrollPane::_scrolled);
      vscroll->createWindow();
    }
    vscroll->bNoFocus=true;
    vscroll->setShape(
      visible.x+visible.w,
      visible.y -1,
      TScrollBar::getFixedSize(),
      visible.h +2);
    vscroll->setExtent(visible.h);
    vscroll->setMinimum(pane.y);
    if (pane.y+pane.h < visible.y+visible.h)
      vscroll->setMaximum(visible.y+visible.h-1);
    else    
      vscroll->setMaximum(pane.y+pane.h-1);
    vscroll->setMapped(true);  
    vscroll->setUnitIncrement(uiy);
  } else {
    if (vscroll) {
      vscroll->setMapped(false);
      vscroll->setValue(0);
    }
  }  
     
  if (need_hscroll) {
    if (!hscroll) {  
      hscroll = new TScrollBar(this, "horizontal");
      connect(hscroll->getModel()->sigChanged, this, &TScrollPane::_scrolled);
      hscroll->createWindow();
    }
    hscroll->bNoFocus=true;
    hscroll->setShape(
      visible.x -1,
      visible.y+visible.h,
      visible.w+2,
      TScrollBar::getFixedSize());
    hscroll->setExtent(visible.w);
    hscroll->setMinimum(pane.x);
    if (pane.x+pane.w < visible.x+visible.w)
      hscroll->setMaximum(visible.x+visible.w-1);
    else    
      hscroll->setMaximum(pane.x+pane.w-1);
    hscroll->setMapped(true);  
    hscroll->setUnitIncrement(uix);
  } else {
    if (hscroll) {
      hscroll->setMapped(false);
      hscroll->setValue(0);
    }
  }  
}

/**
 * draw the litle gray box between the two scrollbars (when we have
 * two of 'em)
 */
void
TScrollPane::paintCorner(TPen &pen)
{
  if (hscroll && vscroll &&
      hscroll->isMapped() &&
      vscroll->isMapped())
  {
    TRectangle r(getWidth() - TScrollBar::getFixedSize(),
                 getHeight()- TScrollBar::getFixedSize(),
                 TScrollBar::getFixedSize(), TScrollBar::getFixedSize());
    pen.setColor(TColor::LIGHTGRAY);
    pen.identity();
    pen|=r;
    pen.fillRectanglePC(r);
  }
}

void
TScrollPane::getPanePos(int *x, int *y, bool setall) const {
  if (setall) {
    *x = *y = 0;
  }
  if (hscroll) *x = hscroll->getValue();
  if (vscroll) *y = vscroll->getValue();
}

void
TScrollPane::setPanePos(int x, int y) {
  if (hscroll) hscroll->setValue(x);
  if (vscroll) vscroll->setValue(y);
}

void
TScrollPane::setUnitIncrement(int uix, int uiy)
{
  this->uix = uix;
  this->uiy = uiy;
  
  if (hscroll)
    hscroll->setUnitIncrement(uix);
  if (vscroll)
    vscroll->setUnitIncrement(uiy);
}
    
