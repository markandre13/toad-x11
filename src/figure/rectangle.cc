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

void TFRectangle::getShape(TRectangle &r)
{
  r.set(p1,p2);
}

void 
TFRectangle::paint(TPenBase &pen, EPaintType)
{
  pen.setLineColor(line_color);
  if (!filled) {
    pen.drawRectangle(p1,p2);
  } else {
    pen.setFillColor(fill_color);
    pen.fillRectangle(p1,p2);
  }
}

double 
TFRectangle::distance(int mx, int my)
{
  if (filled && TRectangle(p1, p2).isInside(mx, my))
    return INSIDE;

  int x1,y1,x2,y2;
  double min = OUT_OF_RANGE, d;
  
  for(int i=0; i<4; i++) {
    switch(i) {
    case 0:
      x1=p1.x; y1=p1.y; x2=p2.x; y2=p1.y;
      break;
    case 1:
      x1=p2.x; y1=p2.y;
      break;
    case 2:
      x2=p1.x; y2=p2.y;
      break;
    case 3:
      x1=p1.x; y1=p1.y;
      break;
    }
    d = distance2Line(mx,my, x1,y1, x2,y2);
    if (d<min)
      min = d;
  }
  return min;
}

void 
TFRectangle::translate(int dx, int dy)
{
  p1.x+=dx;
  p1.y+=dy;
  p2.x+=dx;
  p2.y+=dy;
}

bool 
TFRectangle::getHandle(unsigned handle, TPoint &p)
{
  switch(handle) {
    case 0:
      p = p1;
      return true;
    case 1:
      p.x = p2.x;
      p.y = p1.y;
      return true;
    case 2:
      p = p2;
      return true;
    case 3:
      p.x = p1.x;
      p.y = p2.y;
      return true;
  }
  return false;
}

void 
TFRectangle::translateHandle(unsigned handle, int x, int y)
{
  switch(handle) {
    case 0:
      p1.x = x;
      p1.y = y;
      break;
    case 1:
      p2.x = x;
      p1.y = y;
      break;
    case 2:
      p2.x = x;
      p2.y = y;
      break;
    case 3:
      p1.x = x;
      p2.y = y;
      break;
  }
}

unsigned 
TFRectangle::mouseLDown(TFigureEditor *editor, int mx, int my, unsigned)
{
  switch(editor->state) {
    case TFigureEditor::STATE_START_CREATE:
      p1.x = p2.x = mx;
      p1.y = p2.y = my;
      editor->invalidateFigure(this);
      break;
    default:
      break;
  }
  return CONTINUE;
}

unsigned 
TFRectangle::mouseMove(TFigureEditor *editor, int mx, int my, unsigned)
{
  switch(editor->state) {
    case TFigureEditor::STATE_CREATE:
      editor->invalidateFigure(this);
      p2.x = mx;
      p2.y = my;
      editor->invalidateFigure(this);
      break;
    default:
      break;
  }
  return CONTINUE;
}

unsigned 
TFRectangle::mouseLUp(TFigureEditor *editor, int mx, int my, unsigned)
{
  switch(editor->state) {
    case TFigureEditor::STATE_CREATE:
      mouseMove(editor,mx,my,0);
      if (p1.x==p2.x && p1.y==p2.y)
        return STOP|DELETE;
      return STOP;
    default:
      break;
  }
  return CONTINUE;
}

void
TFRectangle::store(TOutObjectStream &out) const
{
  super::store(out);
  TRectangle r(p1,p2);
  ::store(out, "x", r.x);
  ::store(out, "y", r.y);
  ::store(out, "w", r.w);
  ::store(out, "h", r.h);
}

bool
TFRectangle::restore(TInObjectStream &in)
{
  #warning "TFRectangle::restore: uses static variable"
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
    super::restore(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}
