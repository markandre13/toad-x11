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

#include <toad/figure.hh>
#include <toad/figuremodel.hh>
#include <cmath>

#include <toad/dialog.hh>
#include <toad/menuhelper.hh>

using namespace toad;

/**
 * \defgroup figure Figures
 *
 * Figures are 2 dimensional graphic primitives.
 */

/**
 * \class toad::TFigure
 *
 * TFigure (TFigure in later releases), provides a 2 dimensional figure
 * which is painted by TFigureWindow and can be manipulated by TFigureEditor.
 *
 * Most of the virtual methods provided by TFigure are to support the
 * TFigureEditor class.
 *
 *
 * \li
 *   paint: this is the most important method
 *
 * Methods used during object creation:
 *
 * \li
 *   paint
 * \li
 *   startCreate: this is the first method called when TFigureEditor
 *   starts to create a gadget
 * \li
 *   mouseLDown, mouseMove, mouseLUp: these methods are called after
 *   'startCreate' and shall handle the creation prozess of the figure.
 *   The return value can be a combination of NOTHING, CONTINUE, STOP, REPEAT
 *   or DELETE.
 */

#if 0
const double TFigure::OUT_OF_RANGE;
const double TFigure::RANGE;
const double TFigure::INSIDE;
#endif

void
TFigure::initialize()
{
  TObjectStore& serialize(toad::getDefaultStore());
  serialize.registerObject(new TSerializableRGB());
  serialize.registerObject(new TDialogLayout());
  serialize.registerObject(new TMenuLayout());
  serialize.registerObject(new TMenuEntry());
  serialize.registerObject(new TMenuSeparator());
  serialize.registerObject(new TMatrix2D());
  serialize.registerObject(new TFText());
  serialize.registerObject(new TFFrame());
  serialize.registerObject(new TFLine());
  serialize.registerObject(new TFRectangle());
  serialize.registerObject(new TFCircle());
  serialize.registerObject(new TFPolygon());
  serialize.registerObject(new TFPolyline());
  serialize.registerObject(new TFBezier());
  serialize.registerObject(new TFBezierline());
  serialize.registerObject(new TFGroup());
  serialize.registerObject(new TFWindow());
  serialize.registerObject(new TFigureModel());
}

void
TFigure::terminate()
{
  toad::getDefaultStore().unregisterAll();
}

TInObjectStream TFigure::serialize;

TFigure::TFigure()
{
  filled = false;
  removeable = true;
  mat = 0;
}

TFigure::~TFigure()
{
}

void
TFigure::setFont(const string&)
{
}

void
TFigure::setFromPreferences(TFigurePreferences*)
{
}

/**
 * Called from the gadget editor when the gadget is selected.
 *
 * The default behaviour is to draw small rectangles at the positions
 * delivered by <I>getHandle</I> or at the corners of the rectangle
 * delivered by <I>getShape</I> when <I>getHandle</I> returns `false'
 * for handle 0.
 */
void
TFigure::paintSelection(TPenBase &pen)
{
  pen.setColor(0,0,0);
  unsigned h=0;
  TPoint pt;
  while(true) {
    if ( !getHandle(h, pt) )
      break;
    int x, y;
    if (pen.mat) {
      pen.mat->map(pt.x, pt.y, &x, &y);
      pen.push();
      pen.identity();
    } else {
      x = pt.x;
      y = pt.y;
    }
    pen.fillRectanglePC(x-2,y-2,5,5);
    if (pen.mat)
      pen.pop();
    h++;
  }
  if (h==0) {
    TRectangle r;
    getShape(r);
    int x, y;
    for(int i=0; i<4; ++i) {
      switch(i) {
        case 0: x = r.x;       y = r.y;       break;
        case 1: x = r.x+r.w-1; y = r.y;       break;
        case 2: x = r.x+r.w-1; y = r.y+r.h-1; break;
        case 3: x = r.x;       y = r.y+r.h-1; break;
      }
      if (pen.mat) {
        pen.mat->map(x, y, &x, &y);
        pen.push();
        pen.identity();
      }
      pen.fillRectanglePC(x-2,y-2,5,5);
      if (pen.mat)
        pen.pop();
    }
  }
}

/**
 * Return <I>true</I> and the position of handle <I>n</I> in <I>p</I> or
 * <I>false</I> when there's no handle <I>n</I>.
 *
 * The first handle is 0.
 */
bool TFigure::getHandle(unsigned n, TPoint &p)
{
  return false;
}

/**
 * Set handle <I>handle</I> to position (x,y).
 */
void TFigure::translateHandle(unsigned handle, int x, int y)
{
}

void
TFigure::store(TOutObjectStream &out) const
{
  if (mat) {
    ::store(out, "trans", mat);
  }
  ::store(out, "linecolor", line_color);
  if (filled) {
    ::store(out, "fillcolor", fill_color);
  }
}

bool
TFigure::restore(TInObjectStream &in)
{
  bool b;
  if (in.what==ATV_START) {
    filled = false;
    return true;
  }
  if (::restore(in, "filled", &b))
    return true;
  if (::restore(in, "fillcolor", &fill_color)) {
    filled = true;
    return true;
  }
  if (
    ::restorePtr(in, "trans", &mat) ||
    ::restore(in, "linecolor", &line_color) ||
    finished(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}

/**
 * Returns the distance of point (x,y) to the line (x1,y1)-(x2,y2).
 */
double TFigure::distance2Line(int x, int y, int x1, int y1, int x2, int y2)
{
  double bx = x2 - x1;
  double by = y2 - y1;
  double ax = x-x1;
  double ay = y-y1;
  double lb = bx*bx+by*by;
  double t = (bx * ax + by * ay ) / lb;
  if (t<0.0 || t>1.0)
    return OUT_OF_RANGE;
  return fabs(by * ax - bx * ay) / sqrt(lb);
}
