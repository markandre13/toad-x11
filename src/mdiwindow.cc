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

// the implementation of TMDIWindow, TMDIArea and TMDIShell is really
// nasty (or tricky)

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/scrollbar.hh>
#include <toad/mdiwindow.hh>
#include <toad/mdishell.hh>

namespace toad {

static const char* s_mdiarea="mdiarea";

class TMDIArea: 
  public TWindow
{
    typedef TWindow super;
  public:
    TMDIArea(TMDIWindow* w,const string &t=s_mdiarea)
      :super(w,t){setBorder(false); bFocusManager=true;}
    int x1,y1,x2,y2;    // virtual area (depends on child windows)
    void calculate_va();  // calculate virtual area
  private:
    void childNotify(TWindow*,EChildNotify);
};

} // namespace toad

using namespace toad;

/*****************************************************************************
 *                                                                           *
 * TMDIWindow                                                                *
 *                                                                           *
 *****************************************************************************/

/** 
 * \class toad::TMDIWindow
 *
 * An easy to handle Multiple Document Interface (MDI) implementations.
 *
 * It's Basicly  a small window manager which resides in a window of your
 * program.
 *
 * \note
 *   \li
 *     Windows added to TMDIWindow won't become direct children of
 *     TMDIWindow. The two windows between them are the MDI area and the
 *     shell window. But normaly you can ignore this behaviour.
 *   \li
 *     Theory tells it should be possible to nest TMDIWindows in TOAD but
 *     I've never tried it.
 *
 * \todo
 *   \li
 *     This is old and crude code. The new scrollbar code broke some of
 *     it's logic. There are some fixes, it works, but a clean concept is 
 *     missing. Sometimes is scrolles alone, sometime it crashes, sometimes
 *     it pretents that everything is alright.
 */

// constructor
//---------------------------------------------------------------------------
TMDIWindow::TMDIWindow(TWindow *p,const string &t)
  :TWindow(p,t)
{
  setBackground(TColor::DIALOG);
  area = new TMDIArea(this);
  area->setBackground(TColor::MDIAREA);
  hscroll=NULL;
  vscroll=NULL;
  beforeAddEnabled = true;
}

// resize
//---------------------------------------------------------------------------
void TMDIWindow::resize()
{
  area->calculate_va();
}

// signal handler
//---------------------------------------------------------------------------
void TMDIWindow::actHScroll(TScrollBar *sb)
{
  TInteractor *p = area->getFirstChild();
  while(p) {
    p->setSuppressMessages(true); // still needed?
    p->setPosition( p->getXPos() - sb->getValue(), p->getYPos());
    p->setSuppressMessages(false);
    p = getNextSibling(p);
  }
  area->calculate_va();
}

void TMDIWindow::actVScroll(TScrollBar *sb)
{
  TInteractor *p = area->getFirstChild();
  while(p) {
    p->setSuppressMessages(true);   // still needed?
    p->setPosition(p->getXPos(), p->getYPos()-sb->getValue());
    p->setSuppressMessages(false);
    p = getNextSibling(p);
  }
  area->calculate_va();
}

// vaChanged
//---------------------------------------------------------------------------
void TMDIWindow::vaChanged()
{
  int w = _w;
  int h = _h;
  TRectangle r;
  
  bool hf, vf;

  if (area->y1<0 || area->y2>h) {
    vf = true;
    if (!vscroll) {
      beforeAddEnabled = false;
      vscroll = new TScrollBar(this,"");
      vscroll->bNoFocus = true;
      beforeAddEnabled = true;
      CONNECT(vscroll->getModel()->sigChanged, this, actVScroll, vscroll);
    }
    vscroll->getShape(&r);
    w -= r.w;
  } else {
    vf = false;
  }
  
  if (area->x1<0 || area->x2>w) {
    hf = true;
    if (!hscroll) {
      beforeAddEnabled = false;
      hscroll = new TScrollBar(this,"");
      hscroll->bNoFocus = true;
      beforeAddEnabled = true;
      CONNECT(hscroll->getModel()->sigChanged, this,actHScroll, hscroll);
    }
    TRectangle r;
    hscroll->getShape(&r);
    h -= r.h;
  } else {
    hf = false;
  }
  
  if ( !vf && (area->y1<0 || area->y2>h) ) {
    vf = true;
    TRectangle r;
    if (!vscroll) {
      beforeAddEnabled = false;
      vscroll = new TScrollBar(this,"");
      vscroll->bNoFocus = true;
      beforeAddEnabled = true;
      CONNECT(vscroll->getModel()->sigChanged, this,actVScroll, vscroll);
    }
    vscroll->getShape(&r);
    w -= r.w;
  }

  if (area->x2<w)
    area->x2=w;
    
  if (area->y2<h)
    area->y2=h;
    
  area->setShape(0,0,w,h);

  if (vf) {
    if (vscroll) {
      vscroll->getModel()->setRangeProperties(0,h, area->y1,area->y2-1);
      if (vscroll) {
        vscroll->setShape(w,-1,TSIZE_PREVIOUS,h+1);
        if (!vscroll->isRealized()) vscroll->createWindow();
      }
    }
  } else {
    if (vscroll) {
      sendMessageDeleteWindow(vscroll);
      vscroll=NULL;
    }
  }

  if (hf) {
    if (hscroll) {
      hscroll->getModel()->setRangeProperties(0,w, area->x1,area->x2-1);
      if (hscroll) {
        hscroll->setShape(-1,h,w+1,TSIZE_PREVIOUS);
        if (!hscroll->isRealized()) hscroll->createWindow();
      }
    }
  } else {
    if (hscroll) {
      sendMessageDeleteWindow(hscroll);
      hscroll=NULL;
    }
  }
//  printf("TMDIWindow::vaChanged done\n");
}

// childNotify
//---------------------------------------------------------------------------
void TMDIWindow::beforeAdd(TInteractor **wnd)
{
  *wnd = new TMDIShell(area);
}

void TMDIWindow::childNotify(TWindow *wnd,EChildNotify type)
{
  switch(type) {
    case TCHILD_REMOVE:
      if (wnd==vscroll)
        vscroll=NULL;
      if (wnd==hscroll)
        hscroll=NULL;
      break;
    default:;
  }
}

/*****************************************************************************
 *                                                                           *
 * TMDIArea                                                                  *
 *                                                                           *
 *****************************************************************************/

// childNotify
//---------------------------------------------------------------------------
void TMDIArea::childNotify(TWindow *wnd,EChildNotify type)
{
  switch(type) {
    case TCHILD_CREATE:
      wnd->setFocus();
      calculate_va();
      break;
    case TCHILD_DESTROY:
#ifdef DONT_NEED_THIS_ANYMORE
      if(isPartOfFocus(wnd)) {
        TWindow *ptr;
        ptr=getFirstChild();
        while(ptr) {
          if (/* !IsPartOfFocus(ptr) && ptr->_bFocus*/ && ptr->isRealized())
          {
            ptr->setFocus();
            return;
          }
          ptr=getNextSibling(ptr);
        }
        /* SetFocus(NULL); */
      }
#endif
      calculate_va();
      break;
    case TCHILD_POSITION:
    case TCHILD_RESIZE:
      calculate_va();
      break;
    default:;
  }
}

// calculate_va
//---------------------------------------------------------------------------
void TMDIArea::calculate_va()
{
  int nx1,ny1,nx2,ny2;
  nx1=ny1=nx2=ny2=0;
  
  TInteractor *ptr=getFirstChild();
  while(ptr) {
    if (ptr->isRealized()) {
      TRectangle shape;
      ptr->getShape(&shape);
      if (shape.x < nx1)
        nx1=shape.x;
      if (shape.x+shape.w > nx2)
        nx2=shape.x+shape.w;
      if (shape.y < ny1)
        ny1=shape.y;
      if (shape.y+shape.h > ny2)
        ny2=shape.y+shape.h;
    }
    ptr=getNextSibling(ptr);
  }
//  if (nx1!=x1 || ny1!=y1 || nx2!=x2 || ny2!=y2)
  {
    x1=nx1; y1=ny1; x2=nx2; y2=ny2;
    static_cast<TMDIWindow*>(getParent())->vaChanged();
  }
}

