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
#include <toad/figureeditor.hh>
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
  removeable = true;
  mat = 0;
}

TFigure::~TFigure()
{
}

TColoredFigure::TColoredFigure()
{
  filled = false;
  line_color.set(0,0,0);
  fill_color.set(0,0,0);
  line_style = TPen::SOLID;
  line_width = 0;
}

void
TFigure::setFromPreferences(TFigurePreferences *preferences)
{
}

void
TColoredFigure::setFromPreferences(TFigurePreferences *preferences)
{
  switch(preferences->reason) {
    case TFigurePreferences::ALLCHANGED:
      line_width = preferences->linewidth;
      line_style = preferences->linestyle;
      line_color = preferences->linecolor;
      fill_color = preferences->fillcolor;
      filled     = preferences->filled;
      break;
    case TFigurePreferences::LINECOLOR:
      line_color = preferences->linecolor;
      break;
    case TFigurePreferences::FILLCOLOR:
      fill_color = preferences->fillcolor;
      filled     = preferences->filled;
      break;
    case TFigurePreferences::UNSETFILLCOLOR:
      filled     = preferences->filled;
      break;
    case TFigurePreferences::LINEWIDTH:
      line_width = preferences->linewidth;
      break;
    case TFigurePreferences::LINESTYLE:
      line_style = preferences->linestyle;
      break;
  }
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
  pen.setLineColor(TColor::FIGURE_SELECTION);
  pen.setFillColor(TColor::WHITE);
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
    getShape(&r);
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
bool
TFigure::getHandle(unsigned n, TPoint &p)
{
  return false;
}

/**
 * Set handle <I>handle</I> to position (x,y).
 */
void
TFigure::translateHandle(unsigned handle, int x, int y)
{
}

bool
TFigure::startInPlace()
{
  return false;
}

unsigned
TFigure::stop(TFigureEditor*) 
{ 
  return NOTHING; 
}

unsigned 
TFigure::keyDown(TFigureEditor*, TKey, char*, unsigned)
{ 
  return CONTINUE; 
}

void
TFigure::startCreate()
{
}

unsigned
TFigure::mouseLDown(TFigureEditor*, int, int, unsigned)
{
  return STOP;
}

unsigned
TFigure::mouseMove(TFigureEditor*, int, int, unsigned)
{
  return CONTINUE;
}

unsigned 
TFigure::mouseLUp(TFigureEditor*, int, int, unsigned)
{
  return CONTINUE;
}

unsigned
TFigure::mouseRDown(TFigureEditor*, int, int, unsigned)
{
  return CONTINUE;
}

namespace {

struct TStylePair {
  TPen::ELineStyle code;
  const char *name;
};

TStylePair sp[] = {
  { TPen::SOLID, "solid" },
  { TPen::DASH,  "dash" },
  { TPen::DOT, "dot" },
  { TPen::DASHDOT, "dashdot" },
  { TPen::DASHDOTDOT, "dashdotdot" }
};

} // namespace

void
TFigure::store(TOutObjectStream &out) const
{
  if (mat) {
    ::store(out, "trans", mat);
  }
}

void
TColoredFigure::store(TOutObjectStream &out) const
{
  super::store(out);
  ::store(out, "linecolor", line_color);
  if (filled) {
    ::store(out, "fillcolor", fill_color);
  }
  if (line_width>0) {
    ::store(out, "linewidth", line_width);
  }
  if (line_style!=TPen::SOLID) {
    for(unsigned i=0; i<5; ++i) {
      if (sp[i].code == line_style) {
        ::store(out, "linestyle", sp[i].name);
        break;
      }
    }
  }
}

bool
TFigure::restore(TInObjectStream &in)
{
  if (
    ::restorePtr(in, "trans", &mat) ||
    finished(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}

bool
TColoredFigure::restore(TInObjectStream &in)
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
  string style;
  if (::restore(in, "linestyle", &style)) {
    for(unsigned i=0; i<5; ++i) {
      if (style == sp[i].name) {
        line_style = sp[i].code;
        return true;
      }
    }
    return false;
  }
  
  if (
    ::restore(in, "linecolor", &line_color) ||
    ::restore(in, "linewidth", &line_width) ||
    super::restore(in)
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
