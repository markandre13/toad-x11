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

#include <toad/figure.hh>
#include <toad/figureeditor.hh>

using namespace toad;

TFGroup::~TFGroup()
{
}

void 
TFGroup::calcSize()
{
  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  if (p==e)
    return;
  TRectangle r;
  (*p)->getShape(r);
  p1.x=r.x;
  p1.y=r.y;
  p2.x=r.x+r.w-1;
  p2.y=r.y+r.h-1;
  p++;
  while(p!=e) {
    (*p)->getShape(r);
    if (r.x < p1.x)
      p1.x = r.x;
    if (r.y < p1.y)
      p1.y = r.y;
    int ix2 = r.x + r.w-1;
    int iy2 = r.y + r.h-1;
    if (p2.x < ix2)
      p2.x=ix2;
    if (p2.y < iy2)
      p2.y=iy2;
    p++;
  }
}

void 
TFGroup::paint(TPenBase &pen, EPaintType)
{
  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  while(p!=e) {
    (*p)->paint(pen, NORMAL);
    p++;
  }
}

bool 
TFGroup::getHandle(unsigned n, TPoint &p)
{
cerr << "group, query handle " << n << endl;
if (super::getHandle(n, p)) {
  cerr << "  ok" << endl;
  return true;
} else {
  cerr << "  no" << endl;
}
  return false;
}

double
TFGroup::distance(int mx, int my)
{
  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  if (p==e)
    return OUT_OF_RANGE;
  double d = (*p)->distance(mx, my);
  p++;
  while(p!=e) {
    double td = (*p)->distance(mx, my);
    if (td<d)
      d=td;
    p++;
  }
  return d;
}

void
TFGroup::translate(int dx, int dy)
{
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
  gadgets.store(out);
}

bool
TFGroup::restore(TInObjectStream &in)
{
  if (in.what == ATV_FINISHED) {
    calcSize();
  }
  if (gadgets.restore(in))
    return true;
  ATV_FAILED(in);
  return false;
}
