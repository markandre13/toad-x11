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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>

using namespace toad;

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


Cursor TCursor::X11Cursor(TCursor::EType type)
{
  static bool init = false;
  if (!init) {
    for(int i=0; i<TCursor::_MAX; i++)
      cursor[i]=0;
    init=true;
  }

  if (!cursor[type]) {
    cursor[type] = XCreateFontCursor(TOADBase::x11display, xtype[type]);
  }
  
  return cursor[type];
}

void TWindow::setCursor(TCursor::EType type)
{
  if (_cursor==type)
    return;
  Cursor cursor = TCursor::X11Cursor(type);
  if (x11window) {
    if (type!=TCursor::DEFAULT)
      XDefineCursor(x11display, x11window, cursor);
    else
      XUndefineCursor(x11display, x11window);
  }
  _cursor = type;
}

