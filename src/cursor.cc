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

#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#endif

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>

using namespace toad;

#ifdef __X11__

static unsigned int xtype[TCursor::_MAX] = {
  XC_top_left_arrow,
  XC_crosshair,
  XC_hand2,
  XC_xterm,
  XC_watch,
  XC_fleur,
  XC_top_side,
  XC_top_right_corner,
  XC_top_left_corner,
  XC_bottom_side,
  XC_bottom_right_corner,
  XC_bottom_left_corner,
  XC_left_side,
  XC_right_side,
  XC_question_arrow,
  XC_exchange,
  XC_sizing,
  XC_pirate,
  XC_mouse,
  XC_pencil,
  XC_spraycan,
  XC_sb_h_double_arrow,
  XC_sb_v_double_arrow,
  XC_target,
  XC_dot,
  XC_circle
};

/**
 * \class toad::TCursor
 * Predefined cursor types.
 */

static Cursor cursor[TCursor::_MAX];

Cursor
TCursor::X11Cursor(TCursor::EType type)
{
  static bool init = false;
  if (!init) {
    for(int i=0; i<TCursor::_MAX; i++)
      ::cursor[i]=0;
    init=true;
  }

  if (!::cursor[type] && type!=PARENT) {
    ::cursor[type] = XCreateFontCursor(toad::x11display, xtype[type]);
  }
  
  return ::cursor[type];
}
#endif

void
TWindow::setCursor(TCursor::EType type)
{
#ifdef __X11__
  Cursor cursor = TCursor::X11Cursor(type);
  if (cursor == _cursor)
    return;
  _cursor = cursor;
  if (x11window)
    XDefineCursor(x11display, x11window, _cursor);
#endif
}

void
TWindow::setCursor(const TCursor *c)
{
#ifdef __X11__
  if (c) {
    if (_cursor == c->cursor)
      return;
    _cursor = c->cursor;
  } else {
    if (_cursor == 0)
      return;
    _cursor = 0;
  }
  if (x11window)
    XDefineCursor(x11display, x11window, _cursor);
#endif
}

TCursor::TCursor(const char shape[32][32+1], unsigned ox, unsigned oy)
{
#ifdef __X11__
  XColor fc, bc;
  fc.red = fc.green = fc.blue = 0xFFFF;
  fc.flags = DoRed|DoGreen|DoBlue;
  bc.red = bc.green = bc.blue = 0x0000;
  fc.flags = DoRed|DoGreen|DoBlue;

  GC gc0, gc1;
  gc0 = None;

  Pixmap pm_icon;
  Pixmap pm_mask;
  pm_icon = XCreatePixmap(x11display, DefaultRootWindow(x11display),
            32, 32, 1);
  pm_mask = XCreatePixmap(x11display, DefaultRootWindow(x11display),
            32, 32, 1);

  XGCValues gv;
  gv.foreground=0;
  gc0 = XCreateGC(x11display, pm_icon, GCForeground, &gv);
  gv.foreground=1;
  gc1 = XCreateGC(x11display, pm_icon, GCForeground, &gv);

  for(int y=0; y<32; y++) {
    for(int x=0; x<32; x++) {
      if (shape[y][x]==' ') {
        XDrawPoint(x11display, pm_mask, gc0, x,y);
      } else {
        XDrawPoint(x11display, pm_mask, gc1, x,y);
      }
      if (shape[y][x]=='.') {
        XDrawPoint(x11display, pm_icon, gc0, x,y);
      } else {
        XDrawPoint(x11display, pm_icon, gc1, x,y);
      }
    }
  }
  cursor = XCreatePixmapCursor(x11display, 
                               pm_icon, pm_mask,
                               &fc, &bc,
                               ox,oy);
  XFreePixmap(x11display, pm_icon);
  XFreePixmap(x11display, pm_mask);
  
  XFreeGC(x11display, gc0);
  XFreeGC(x11display, gc1);
#endif
}

TCursor::~TCursor()
{
#ifdef __X11__
  if (cursor)
    XFreeCursor(x11display, cursor);
#endif
}
