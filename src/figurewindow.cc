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

#include <toad/figurewindow.hh>
#include <toad/figure.hh>
#include <toad/io/binstream.hh>

using namespace toad;

/**
 * \class toad::TFigureWindow
 * Gadgets are 2dimensional graphical 
 * objects in TOAD and <I>TFigureWindow</I> stores and paints them.
 */

TFigureWindow::TFigureWindow(TWindow *p, const string& t):
  super(p,t)
{
  gadgets = new TFigureModel();
}

void
TFigureWindow::paint()
{
  TPen pen(this);
  print(pen);
}

void
TFigureWindow::print(TPenBase &pen)
{
  TFigureModel::iterator p = gadgets->begin();
  while(p!=gadgets->end()) {
    (*p)->paint(pen, TFigure::NORMAL);
    ++p;
  }
}

void
TFigureWindow::store(TOutObjectStream &out) const
{
  ::store(out, gadgets);
}

bool
TFigureWindow::restore(TInObjectStream &in)
{
#if 1
  TSerializable *s;

  // ::restorePtr(in, &s);
  s = in.restore();
  
  TFigureModel *m = dynamic_cast<TFigureModel *>(s);
  if (!m) {
    cerr << "wasn't a TFigureModel" << endl;
    return false;
  }
//  cerr << "setting new TFigureModel" << endl;
//  cerr << "  size of new model: " << gadgets->storage.size() << endl;
//  cerr << "  size of next model: " << m->storage.size() << endl;
  gadgets = m;
//  cerr << "  size of new model: " << gadgets->storage.size() << endl;
#else
  TFigureModel *m;
  ::restorePtr(in, &m);
  gadgets = m;
#endif
  invalidateWindow();
  return true;
}
