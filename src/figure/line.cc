/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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

#include <math.h>
#include <toad/figure.hh>
#include <toad/figureeditor.hh>

using namespace toad;

#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi */
#endif

TFLine::TFLine()
{
  arrowmode = NONE;
  arrowtype = SIMPLE;
  arrowheight = 8 * 96;
  arrowwidth = 4 * 96;
}

void
TFLine::setFromPreferences(TFigurePreferences *preferences)
{
  switch(preferences->reason) {
    case TFigurePreferences::ALLCHANGED:
      arrowmode = preferences->arrowmode;
      arrowtype = preferences->arrowtype;
      break;
    case TFigurePreferences::LINEWIDTH:
      break;
    case TFigurePreferences::LINESTYLE:
      break;
    case TFigurePreferences::ARROWMODE:
      arrowmode = preferences->arrowmode;
      break;
    case TFigurePreferences::ARROWSTYLE:
      arrowtype = preferences->arrowtype;
      break;
  }
}

void
TFLine::drawArrow(TPenBase &pen, 
                  const TPoint &p1, const TPoint &p2,
                  const TRGB &line, const TRGB &fill,
                  int w, int h,
                  EArrowType type)
{
  double d = atan2((double)p2.y - p1.y, 
                   (double)p2.x - p1.x);
  
  double height = h;
  double width  = 0.5 * w;
  
  TPoint p0;
  
  TPoint p[4];
  
  p[0] = p1;
  
  p0.x = static_cast<int>(cos(d) * height + p1.x);
  p0.y = static_cast<int>(sin(d) * height + p1.y);

  double r = 90.0 / 360.0 * (2.0 * M_PI);
  p[1].x = p0.x + static_cast<int>(cos(d-r) * width);
  p[1].y = p0.y + static_cast<int>(sin(d-r) * width);
  p[3].x = p0.x + static_cast<int>(cos(d+r) * width);
  p[3].y = p0.y + static_cast<int>(sin(d+r) * width);
  
  pen.setLineColor(line);
  switch(type) {
    case SIMPLE:
      pen.drawLine(p[0], p[1]);
      pen.drawLine(p[0], p[3]);
      break;
    case EMPTY:
      p[2] = p0;
      pen.setFillColor(fill);
      pen.fillPolygon(p, 4);
      break;
    case FILLED:
      p[2] = p0;
      pen.setFillColor(line);
      pen.fillPolygon(p, 4);
      break;
    case EMPTY_CONCAVE:
      height -= height / 4;
      p[2].x = static_cast<int>(cos(d) * height + p1.x);
      p[2].y = static_cast<int>(sin(d) * height + p1.y);
      pen.setFillColor(fill);
      pen.fillPolygon(p, 4);
      break;
    case FILLED_CONCAVE:
      height -= height / 4;
      p[2].x = static_cast<int>(cos(d) * height + p1.x);
      p[2].y = static_cast<int>(sin(d) * height + p1.y);
      pen.setFillColor(line);
      pen.fillPolygon(p, 4);
      break;
    case EMPTY_CONVEX:
      height += height / 4;
      p[2].x = static_cast<int>(cos(d) * height + p1.x);
      p[2].y = static_cast<int>(sin(d) * height + p1.y);
      pen.setFillColor(fill);
      pen.fillPolygon(p, 4);
      break;
    case FILLED_CONVEX:
      height += height / 4;
      p[2].x = static_cast<int>(cos(d) * height + p1.x);
      p[2].y = static_cast<int>(sin(d) * height + p1.y);
      pen.setFillColor(line);
      pen.fillPolygon(p, 4);
      break;
  }
}

void 
TFLine::paint(TPenBase &pen, EPaintType)
{
  pen.drawLines(polygon);

  if (arrowmode == HEAD || arrowmode == BOTH)
    drawArrow(pen, polygon[polygon.size()-1], polygon[polygon.size()-2], line_color, fill_color, arrowwidth, arrowheight, arrowtype);
  if (arrowmode == TAIL || arrowmode == BOTH)
    drawArrow(pen, polygon[0], polygon[1], line_color, fill_color, arrowwidth, arrowheight, arrowtype);
}

double 
TFLine::distance(int mx, int my)
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
TFLine::mouseLDown(TFigureEditor *editor, int mx, int my, unsigned m)
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
