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

#include <toad/figure.hh>
#include <toad/figureeditor.hh>
#include <toad/labelowner.hh>

// avoid problems on AIX, IRIX, ...
#define exception c_exception
#include <cmath>
#undef exception

using namespace toad;

TFWindow::TFWindow()
{
  window = NULL;
  taborder = 0;
  removeable = false;
}

void 
TFWindow::paint(TPenBase&, EPaintType)
{
}

double
TFWindow::distance(TCoord x, TCoord y)
{
  filled = true;
  return super::distance(x,y);
}

void
TFWindow::translate(TCoord dx, TCoord dy)
{
  if (window==NULL) {
    cerr << "toad warning: TFWindow.window==NULL : " << title << endl;
    return;
  }
  super::translate(dx, dy);
  TRectangle r;
  getShape(&r);
  window->setShape(r);
}

void
TFWindow::translateHandle(unsigned handle, TCoord mx, TCoord my, unsigned m)
{
  if (window==NULL) {
    cerr << "toad warning: TFWindow.window==NULL : " << title << endl;
    return;
  }
  super::translateHandle(handle, mx, my, m);
  TRectangle r;
  getShape(&r);
  window->setShape(r);
}

void 
TFWindow::store(TOutObjectStream &out) const
{
  if (window==NULL)
    cerr << "toad warning: TFWindow.window==NULL : " << title << endl;
    
  TRectangle r(p1,p2);
  ::store(out, "x", r.x);
  ::store(out, "y", r.y);
  ::store(out, "w", r.w);
  ::store(out, "h", r.h);
  ::store(out, "title", title);

  if (window) {
    TLabelOwner *lo = dynamic_cast<TLabelOwner*>(window);
    if (lo) {
      ::store(out, "label", lo->_label);
    }
  }
  
  ::store(out, "taborder", taborder);
}

/**
 * Restore TFWindow from stream.
 *
 * The data fetched during restore will be copied by `arrangeHelper(...)'
 * in `dialog.cc' to the window.
 */
bool
TFWindow::restore(TInObjectStream &in)
{
  static TRectangle r;
  
  if (::finished(in)) {
    setShape(r.x, r.y, r.w, r.h);
    return true;
  }
  
  if (
    ::restore(in, "x", &r.x) ||
    ::restore(in, "y", &r.y) ||
    ::restore(in, "w", &r.w) ||
    ::restore(in, "h", &r.h) ||
    ::restore(in, "title", &title) ||
    ::restore(in, "label", &label) ||
    ::restore(in, "taborder", &taborder) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}
