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

void
TFPolygon::getShape(TRectangle &r)
{
  TPoint p1, p2;

  TPolygon::const_iterator p(polygon.begin()), e(polygon.end());
  
  if (p==e)
    return;

  p1.x = p2.x = p->x;
  p1.y = p2.y = p->y;

  ++p;
  while(p!=e) {
    if (p->x < p1.x)
      p1.x = p->x;
    if (p->x > p2.x)
      p2.x = p->x;
    if (p->y < p1.y)
      p1.y = p->y;
    if (p->y > p2.y)
      p2.y = p->y;
    ++p;
  }

  r.set(p1,p2);
}

void 
TFPolygon::paint(TPenBase &pen, EPaintType)
{
  pen.setLineColor(line_color);
  if (!filled) {
    pen.drawPolygon(polygon);
  } else {
    pen.setFillColor(fill_color);
    pen.fillPolygon(polygon);
  }
}

double 
TFPolygon::distance(int mx, int my)
{
  if (filled && polygon.isInside(mx, my))
    return INSIDE;

  TPolygon::const_iterator p(polygon.begin()), e(polygon.end());
  int x1,y1,x2,y2;
  double min = OUT_OF_RANGE, d;

  assert(p!=e);
  --e;
  assert(p!=e);
  x2=e->x;
  y2=e->y;
  ++e;
  while(p!=e) {
    x1=x2;
    y1=y2;
    x2=p->x;
    y2=p->y;
    d = distance2Line(mx,my, x1,y1, x2,y2);
    if (d<min)
      min = d;
    ++p;
  }
  return min;
}

void 
TFPolygon::translate(int dx, int dy)
{
  TPolygon::iterator p(polygon.begin()), e(polygon.end());
  while(p!=e) {
    p->x+=dx;
    p->y+=dy;
    ++p;
  }
}

bool 
TFPolygon::getHandle(unsigned handle, TPoint &p)
{
  if (handle >= polygon.size())
    return false;
  p = polygon[handle];
  return true;
}

void 
TFPolygon::translateHandle(unsigned handle, int x, int y)
{
  TPoint p(x, y);
  polygon[handle]=p;
}

// polygon creation
//---------------------------------------------------------------------------
unsigned 
TFPolygon::mouseLDown(TFigureEditor *editor, int mx, int my, unsigned m)
{
  switch(editor->state) {
    case TFigureEditor::STATE_START_CREATE:
      polygon.addPoint(mx, my);
      editor->setMouseMoveMessages(TWindow::TMMM_ALL);
    case TFigureEditor::STATE_CREATE:
      if (m & MK_DOUBLE) {
        if (polygon.size()<4)
          return STOP|DELETE;
        polygon.erase(--polygon.end());
        return STOP;
      }
      polygon.addPoint(mx, my);
      editor->invalidateFigure(this);
      break;
    default:
      break;
  }
  return CONTINUE;
}

unsigned 
TFPolygon::mouseMove(TFigureEditor *editor, int mx, int my, unsigned)
{
  TPolygon::iterator p(polygon.end());
  --p;
  editor->invalidateFigure(this);
  p->set(mx, my);
  editor->invalidateFigure(this);
  return CONTINUE;
}

// storage
//---------------------------------------------------------------------------
void
TFPolygon::store(TOutObjectStream &out) const
{
  super::store(out);
  TPolygon::const_iterator p(polygon.begin()), e(polygon.end());
  while(p!=e) {
    out.indent();
    out << p->x << ' ' << p->y;
    ++p;
  }
}

bool
TFPolygon::restore(TInObjectStream &in)
{
  static bool flag;
  static int x;
  int y;

  if (in.what == ATV_START)
    flag = false;

  if (::restore(in, &y)) {
    if (flag) {
      polygon.addPoint(x, y);
    } else {
      x = y;
    }
    flag = !flag;
    return true;
  }

  if (super::restore(in))
    return true;
  ATV_FAILED(in)
  return false;
}
