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

#include <toad/figure/line.hh>
#include <toad/figureeditor.hh>

using namespace toad;

TFLine::TFLine()
{
  p1.x = p1.y = p2.x = p1.y = 0;
}

TFLine::TFLine(int x1, int y1, int x2, int y2)
{
  p1.x = x1; p1.y = y1;
  p2.x = x2; p2.y = y2;
}

void 
TFLine::paint(TPenBase &pen, EPaintType)
{
  pen.setLineColor(line_color);
  pen.drawLine(p1, p2);
}

void 
TFLine::getShape(TRectangle &r)
{
  r.set(p1, p2);
}

double 
TFLine::distance(int mx, int my)
{
  return distance2Line(mx, my, p1.x, p1.y, p2.x, p2.y);
}

bool
TFLine::getHandle(unsigned handle, TPoint& p)
{
  switch(handle) {
    case 0:
      p=p1;
      return true;
    case 1:
      p=p2;
      return true;
  }
  return false;
}

void 
TFLine::translate(int dx, int dy)
{
  p1.x+=dx;
  p1.y+=dy;
  
  p2.x+=dx;
  p2.y+=dy;
}

void 
TFLine::translateHandle(unsigned h, int x, int y)
{
  switch(h) {
    case 0:
      p1.set(x,y);
      break;
    case 1:
      p2.set(x,y);
      break;
  }
}

unsigned
TFLine::mouseLDown(TFigureEditor *e, int x, int y, unsigned)
{
  if (e->state == TFigureEditor::STATE_START_CREATE) {
    e->invalidateFigure(this);
    p1.set(x,y);
    p2.set(x,y);
    e->invalidateFigure(this);
    return CONTINUE;
  }
  return NOTHING;
}

unsigned
TFLine::mouseMove(TFigureEditor *e, int x, int y, unsigned)
{
  if (e->state == TFigureEditor::STATE_CREATE) {
    e->invalidateFigure(this);
    p2.set(x,y);
    e->invalidateFigure(this);
    return CONTINUE;
  }
  return NOTHING;
}

unsigned
TFLine::mouseLUp(TFigureEditor *e, int x, int y, unsigned)
{
  if (e->state == TFigureEditor::STATE_CREATE) {
    p2.set(x,y);
    if (p1.x==p2.x && p1.y==p2.y)
      return STOP|DELETE;
    return STOP;
  }
  return NOTHING;
}

void 
TFLine::store(TOutObjectStream &out) const
{
  ::store(out, "x1", p1.x);
  ::store(out, "y1", p1.y);
  ::store(out, "x2", p2.x);
  ::store(out, "y2", p2.y);
  ::store(out, "linecolor", line_color);
}

bool 
TFLine::restore(TInObjectStream &in)
{
  if (
    ::restore(in, "linecolor", &line_color) ||
    ::restore(in, "x1", &p1.x) ||
    ::restore(in, "y1", &p1.y) ||
    ::restore(in, "x2", &p2.x) ||
    ::restore(in, "y2", &p2.y) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}
