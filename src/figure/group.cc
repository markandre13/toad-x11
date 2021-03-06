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

using namespace toad;

TFGroup::TFGroup()
{
  #warning "FUNCTIONAL OVERKILL, USE A MODIFIED FLAG INSTEAD AND NOT FIGUREMODEL"
  connect(gadgets.sigChanged, this, &TFGroup::modelChanged);
}

TFGroup::TFGroup(const TFGroup &g):
  gadgets(g.gadgets)
{
  if (g.mat && !mat)
    mat = new TMatrix2D(*g.mat);
  calcSize();
  connect(gadgets.sigChanged, this, &TFGroup::modelChanged);
}

/**
 * Adds a transformation matrix to the group in case one of the
 * groups figures contains a transformation matrix.
 *
 * This is to avoid unwanted calls to 'translate'.
 */
void
TFGroup::modelChanged()
{
  if (mat)
    return;

  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  while(p!=e) {
    if ( (*p)->mat ) {
      mat = new TMatrix2D();
      return;
    }
    ++p;
  }
}

TFGroup::~TFGroup()
{
  disconnect(gadgets.sigChanged, this);
}

/**
 * Calculate the size.
 *
 */
void 
TFGroup::calcSize()
{
  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  if (p==e)
    return;
  TRectangle r;
  TMatrix2D m;
  bool first = true;
  
  while(p!=e) {
    TFigure *f = *p;
    f->getShape(&r);
#if 0
    if (mat)
      m = *mat;
    else
#endif
      m.identity();
    if (f->mat)
      m.multiply(f->mat);
    for(int i=0; i<4; ++i) {
      int x, y;
      switch(i) {
        case 0:
          m.map(r.x, r.y, &x, &y);
          break;
        case 1:
          m.map(r.x+r.w, r.y, &x, &y);
          break;
        case 2:
          m.map(r.x+r.w, r.y+r.h, &x, &y);
          break;
        case 3:
          m.map(r.x, r.y+r.h, &x, &y);
          break;
      }
      if (first) {
        p1.x = p2.x = x;
        p1.y = p2.y = y;
        first = false;
      } else {
        if (p1.x>x)
          p1.x=x;
        if (p2.x<x)
          p2.x=x;
        if (p1.y>y) 
          p1.y=y;
        if (p2.y<y)
          p2.y=y;
      }
    }
    ++p;
  }
}

void 
TFGroup::paint(TPenBase &pen, EPaintType)
{
  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  while(p!=e) {
    if ((*p)->mat) {
      pen.push();
      pen.multiply((*p)->mat);
      (*p)->paint(pen, NORMAL);
      pen.pop();
    } else {
      (*p)->paint(pen, NORMAL);
    }
    p++;
  }
}


bool 
TFGroup::getHandle(unsigned n, TPoint *p)
{
#if 1
  return super::getHandle(n, p);
#else
cerr << "group, query handle " << n << endl;
if (super::getHandle(n, p)) {
  cerr << "  ok" << endl;
  return true;
} else {
  cerr << "  no" << endl;
}
  return false;
#endif
}

static TMatrix2D lastmatrix, lastimatrix;
static TPoint lp1, lp2, lpi1, lpi2;

/*
  pen.multiply(obj->mat);
  pen.draw(obj->x, ...)
*/

bool
TFGroup::startTranslateHandle()
{
  // in case there's no matrix yet, add one and set upper left corner to
  // (0, 0)
  if (!mat) {
    TMatrix2D *mat = new TMatrix2D;
    mat->translate(p1.x, p1.y);
    translate(-p1.x, -p1.y);
    this->mat = mat;
  } else {
    // there is a matrix, move object to (0,0)
    // ... no code yet ...
  }
  lp1 = p1;
  lp2 = p2;
  lastmatrix = *mat;
  lastimatrix = lastmatrix;
  lastimatrix.invert();
  lastimatrix.map(p1.x, p1.y, &lpi1.x, &lpi1.y);
  lastimatrix.map(p2.x, p2.y, &lpi2.x, &lpi2.y);
  return false;
}

void
TFGroup::endTranslateHandle()
{
}

void
TFGroup::translateHandle(unsigned handle, TCoord x, TCoord y, unsigned m)
{
  TCoord w0, w1, h0, h1;
#if 1
//cout << "TFGroup::translateHandle("<<handle<<", "<<x<<", "<<y<<",m)"<<endl;
  switch(handle) {
    case 2:
      lastimatrix.map(x, y, &x, &y);
      w0 = lp2.x - lp1.x; // old width
      w1 = x - lp1.x;     // new width
      h0 = lp2.y - lp1.y;
      h1 = y - lp1.y;
      if (mat)
        delete mat;
      mat = new TMatrix2D(lastmatrix);
//      cerr << "scale by " << (1.0 / w0 * w1) << endl;
      mat->scale(w1/w0, h1/h0);
      mat->translate(p1.x*w1/w0, p1.y*h1/h0);
      break;
  }
#else
cerr << "TFGroup::translateHandle isn't implemented yet" << endl;
#if 0

  cerr << __FUNCTION__ << endl;
  switch(handle) {
    case 0:
      break;
    case 1:
      break;
    case 2:
cerr << "  p2.x = " << p2.x << ", x = " << x << endl;
      w0 = p2.x - p1.x;
      w1 = x - p1.x;
      h0 = p2.y - p1.y;
      h1 = y - p1.y;
      if (!mat)
        mat = new TMatrix2D();
      cerr << "scale by " << (w1/w0) ", " << (h1/h0) << endl;
      mat->scale(w1/w0, h1/h0);
      
      // double h0 = p2.y - p1.y;
#if 0
      calcSize();
#endif
#if 0
      p2.x = x;
      p2.y = y;
#endif
cerr << "  p2.x = " << p2.x << ", x = " << x << endl;
      break;
    case 3:
      break;
  }
#endif
#endif
}

double
TFGroup::_distance(TFigureEditor *fe, TCoord mx, TCoord my)
{
  TCoord d = OUT_OF_RANGE;
  for (TFigureModel::iterator p = gadgets.begin();
       p != gadgets.end();
       ++p)
  {
    TCoord td;
    if ( (*p)->mat) {
      TCoord x, y;
      TMatrix2D m(*(*p)->mat);
      m.invert();
      m.map(mx, my, &x, &y);
      td = (*p)->_distance(fe, x, y);
    } else {
      td = (*p)->_distance(fe, mx, my);
    }
    if (td<d)
      d=td;
  }
  return d;
}

bool
TFGroup::editEvent(TFigureEditEvent &ee)
{
  switch(ee.type) {
    case TFigureEditEvent::TRANSLATE:
      translate(ee.x, ee.y);
      break;
    default:
      ;
  }
}

void
TFGroup::translate(TCoord dx, TCoord dy)
{
  if (mat) {
    mat->translate(dx, dy);
    return;
  }
  p1.x+=dx;
  p1.y+=dy;
  p2.x+=dx;
  p2.y+=dy;
  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  while(p!=e) {
    (*p)->translate(dx,dy);
    p++;
  }
}

void
TFGroup::store(TOutObjectStream &out) const
{
  if (mat) {
    ::store(out, "trans", mat);
  }
  gadgets.store(out);
}

bool
TFGroup::restore(TInObjectStream &in)
{
  if (in.what == ATV_FINISHED) {
    calcSize();
    return true;
  }
  if (::restorePtr(in, "trans", &mat)) {
    return true;
  }
  if (gadgets.restore(in))
    return true;
  ATV_FAILED(in);
  return false;
}
