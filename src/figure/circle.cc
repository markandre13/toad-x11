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

// avoid problems on AIX, IRIX, ...
#define exception c_exception
#include <cmath>
#undef exception

// missing in mingw
#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi */
#endif

using namespace toad;

void 
TFCircle::paint(TPenBase &pen, EPaintType)
{
  pen.setLineColor(line_color);
  if (!filled) {
    pen.drawCircle(p1,p2);
  } else {
    pen.setFillColor(fill_color);
    pen.fillCircle(p1,p2);
  }
}

double 
TFCircle::distance(int mx, int my)
{
  TRectangle r;
  getShape(&r);
  double rx = 0.5*(r.w);
  double ry = 0.5*(r.h);
  double cx = (double)r.x+rx;
  double cy = (double)r.y+ry;
  double dx = (double)mx - cx;
  double dy = (double)my - cy;
  
  double phi = atan( (dy*rx) / (dx*ry) );
  if (dx<0.0)
    phi=phi+M_PI;
  double ex = rx*cos(phi);
  double ey = ry*sin(phi);
  if (filled) {
    double d = sqrt(dx*dx+dy*dy)-sqrt(ex*ex+ey*ey);
    if (d<0.0)
      return INSIDE;
    return d;
  }
  dx -= ex;
  dy -= ey;
  return sqrt(dx*dx+dy*dy);
}
