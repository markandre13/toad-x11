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

#include <toad/figurewindow.hh>
#include <toad/figure.hh>
#include <toad/io/binstream.hh>

using namespace toad;

TFigureModel::TFigureModel()
{
//  cerr << "new TGadgetmodel " << this << endl;
}

TFigureModel::TFigureModel(const TFigureModel &m)
{
//  cerr << "copy constructed TFigureModel " << this << " from " << &m << endl;
}

TFigureModel::~TFigureModel()
{
  type = DELETE;
  sigChanged();
  clear();
}

void
TFigureModel::erase(const iterator &p)
{
  type = MODIFIED;
  sigChanged();
  storage.erase(p.p);
}

void
TFigureModel::erase(const iterator &p, const iterator &e)
{
  type = MODIFIED;
  sigChanged();
  storage.erase(p.p, e.p);
}

void
TFigureModel::insert(const iterator &p, TFigure *g)
{
  type = MODIFIED;
  sigChanged();
  storage.insert(p.p, g);
}

void
TFigureModel::insert(const iterator &at, const iterator &from, const iterator &to)
{
  type = MODIFIED;
  sigChanged();
  storage.insert(at.p, from.p, to.p);
}

/**
 * remove all gadgets
 */
void
TFigureModel::clear()
{
  type = MODIFIED;
  sigChanged();
//  cerr << "clear TFigureModel " << this << endl;
  TStorage::iterator p, e;
  p = storage.begin();
  e = storage.end();
  while(p!=e) {
    delete *p;
    ++p;
  }
  storage.erase(storage.begin(), storage.end());
}

void
TFigureModel::store(TOutObjectStream &out) const
{
  TStorage::const_iterator p, e;
  p = storage.begin();
  e = storage.end();
  while(p!=e) {
    ::store(out, *p);
    ++p;
  }
}

bool
TFigureModel::restore(TInObjectStream &in)
{
  switch(in.what) {
    case ATV_START:
      clear();
      return true;
    case ATV_GROUP: {
        TSerializable *s = in.clone(in.type);
        if (!s)
          return false;
//        cerr << "okay got " << s->name() << endl;
        TFigure *g = dynamic_cast<TFigure*>(s);
        if (!g) {
//          cerr << "  wasn't a gadget" << endl;
          delete s;
          return false;
        }
//        cerr << "adding new gadget to TFigureModel " << this << endl;
        storage.push_back(g);
//        cerr << "new storage size is " << storage.size() << endl;
        in.setInterpreter(s);
        return true;
      }
      break;
    case ATV_FINISHED:
//      cerr << "restored TFigureModel of size " << storage.size() << endl;
      type = MODIFIED;
      sigChanged();
      return true;
  }
  return false;
}

//--------------------------------------------------

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
