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

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/pushbutton.hh>

#include <toad/mdishell.hh>

namespace toad {

class TMDIShell::TTitleBar: 
  public TWindow
{
  public:
    TTitleBar(TMDIShell *p,const string &t="");
  protected:
    void create();
    void mouseLDown(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void mouseMove(int,int,unsigned);
    void paint();
  private:
    int _x,_y;
    bool bMoving;
    int ox,oy;
    TMDIShell *shell;
    void cmdButton();
};

class TXButton: 
  public TPushButton
{
  public:
    TXButton(TWindow* p,const string &t):TPushButton(p,t){bNoFocus = true;};
    void paint();
};

} // namespace toad

using namespace toad;

/*****************************************************************************
 *                                                                           *
 * TMDIShell                                                                 *
 *                                                                           *
 *****************************************************************************/
TMDIShell::TMDIShell(TWindow *p,const string &t):TWindow(p,t)
{
  bFocusManager = true;
  tb = NULL;    // IT'S A MUST OR YOU'LL CRASH BECAUSE OF 'childNotify'
  tb = new TTitleBar(this);
  tb->setShape(-1,-1, _w+2,18);
  wnd = NULL;
}

void
TMDIShell::childNotify(TWindow *child, EChildNotify type)
{
  switch(type) {
    case TCHILD_ADD:
      if (tb==NULL || child==tb)
        return;
      wnd = child;
//      wnd->bShell = true;
      // break;
    case TCHILD_TITLE:
      if (tb) { // this..
        setTitle(child->getTitle());
        tb->invalidateWindow();
      }
      break;
    case TCHILD_FOCUS:
      if (tb) {
        tb->invalidateWindow();
      }
      break;
    case TCHILD_BEFORE_CREATE:
      // This is the last and best moment to unset the bShell flag.
      // Otherwise 'wnd' would get managed by the real window manager
      // and not by this TMDIShell object. (If! bShell was true.)
      if (wnd && wnd==child) {
        wnd->bShell = false;
        setPosition(wnd->getXPos(), wnd->getYPos());
      }
      break;
    case TCHILD_POSITION:
      if (wnd && wnd==child) {
        setPosition(wnd->getXPos(), wnd->getYPos());
      }
      break;
    case TCHILD_RESIZE:
      resize();
      break;
    case TCHILD_REMOVE:
      sendMessageDeleteWindow(this);
      break;
    default:;
  }
}

void
TMDIShell::created()
{
  resize();
}

void
TMDIShell::resize()
{
  TInteractor *w;
  TRectangle shape;
  w=getFirstChild();
  while(w) {  // this loop assumes only 2 children!!!
    if (w!=tb) {
      tb->setShape(-1,-1,_w+2,18);
      w->getShape(&shape);
      w->setSuppressMessages(true);
      w->setPosition(-1,16);
      w->setSuppressMessages(false);
      setShape(TPOS_PREVIOUS,TPOS_PREVIOUS,shape.w, 17+shape.h );
      break;
    }
    w = getNextSibling(w);
  }
}

void
TMDIShell::focus(bool)
{
  tb->invalidateWindow();
}

void
TMDIShell::closeRequest()
{
  destroyWindow();
  sendMessageDeleteWindow(this);
}

/*****************************************************************************
 *                                                                           *
 * TTitleBar                                                                 *
 *                                                                           *
 *****************************************************************************/

TMDIShell::TTitleBar::TTitleBar(TMDIShell *p, const string &t)
  :TWindow(p,t)
{
  shell = p;
  setBackground(TColor::INACTIVECAPTION);
  setMouseMoveMessages(TMMM_LBUTTON);
}

void
TMDIShell::TTitleBar::create()
{
  TXButton *w;
  w = new TXButton(this, "titlebar.xbutton");
    CONNECT(w->sigActivate, this, cmdButton);
    w->setBorder(false);
    w->setPosition(1,1);
    w->setSize(_h-2,_h-2);
  bMoving=false;
}

void
TMDIShell::TTitleBar::cmdButton()
{
  getParent()->closeRequest();
}

void
TMDIShell::TTitleBar::paint()
{
  TPen pen(this);
  pen.setFont(toad::bold_font); // hack !!!
  if (getParent()->isFocus()) {
    setBackground(TColor::CAPTION);
    pen.setColor(TColor::CAPTIONTEXT);
  } else {
    setBackground(TColor::INACTIVECAPTION);
    pen.setColor(TColor::INACTIVECAPTIONTEXT);
  }
  clearWindow();
  pen.clrClipBox();
  int x = (_w-pen.getTextWidth(getParent()->getTitle())) >> 1;
  int y = (_h-pen.getHeight()) >> 1;
  pen.drawString(x, y, getParent()->getTitle());
}

void
TMDIShell::TTitleBar::mouseLDown(int x,int y,unsigned)
{
  _x=x; _y=y;
  shell->setFocus();
  shell->raiseWindow();
}

void
TMDIShell::TTitleBar::mouseMove(int mx,int my,unsigned)
{
  TRectangle shape;
  TPen pen(getParent()->getParent());
  pen.setClipChildren(false);
  pen.setMode(TPen::INVERT);
  pen.setLineWidth(3);

  getParent()->getShape(&shape);
  int x=mx-_x+shape.x;
  int y=my-_y+shape.y;

  if (!bMoving) {
    // XGrabServer
    grabMouse(TMMM_PREVIOUS,getParent()->getParent());
    bMoving = true;
    lockPaintQueue();
  } else {
    pen.drawRectanglePC(ox,oy,shape.w,shape.h);
  }
  pen.drawRectanglePC(x,y,shape.w,shape.h);
  ox=x; oy=y;
}

void
TMDIShell::TTitleBar::mouseLUp(int,int,unsigned)
{
  if (bMoving) {
    unlockPaintQueue();
    ungrabMouse();
    // XGrabServer
    TPen pen(getParent()->getParent());
    pen.setClipChildren(false);
    pen.setMode(TPen::INVERT);
    pen.setLineWidth(3);
    TRectangle shape;
    getParent()->getShape(&shape);
    pen.drawRectanglePC(ox,oy,shape.w,shape.h);
    bMoving = false;

    getParent()->setPosition(ox,oy);
  }
}

/*****************************************************************************
 *                                                                           *
 * TXButton                                                                  *
 *                                                                           *
 *****************************************************************************/
void
TXButton::paint()
{
  TPen pen(this);
  drawShadow(pen, bDown && bInside);
  int n=bDown && bInside?1:0;

  pen.setColor(TColor::BTNTEXT);
  pen.drawLine(3+n,3+n, _w-4+n,_h-4+n);
  pen.drawLine(3+n,4+n, _w-5+n,_h-4+n);
  pen.drawLine(4+n,3+n, _w-4+n,_h-5+n);
  pen.drawLine(_w-4+n,3+n, 3+n,_h-4+n);
  pen.drawLine(_w-5+n,3+n, 3+n,_h-5+n);
  pen.drawLine(_w-4+n,4+n, 4+n,_h-4+n);
}
