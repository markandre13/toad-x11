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
TFPolyline::paint(TPenBase &pen, EPaintType)
{
  pen.setLineColor(line_color);
  pen.drawLines(polygon);
}

double 
TFPolyline::distance(int mx, int my)
{
  TPolygon::const_iterator p(polygon.begin()), e(polygon.end());
  int x1,y1,x2,y2;
  double min = OUT_OF_RANGE, d;

  assert(p!=e);
  x2=p->x;
  y2=p->y;
  ++p;
  assert(p!=e);
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

/**
 * A variation of our super class mouseLDown, which accepts a minimum of
 * 2 points instead of 3.
 */
unsigned 
TFPolyline::mouseLDown(TFigureEditor *editor, int mx, int my, unsigned m)
{
  if (editor->state == TFigureEditor::STATE_CREATE &&
      m & MK_DOUBLE) 
  {
    if (polygon.size()<3)
      return STOP|DELETE;
    polygon.erase(--polygon.end());
    return STOP;
  }
  return super::mouseLDown(editor, mx, my, m);
}
