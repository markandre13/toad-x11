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
#include <toad/figuremodel.hh>
#include <toad/figureeditor.hh>
#include <cmath>

#include <toad/dialog.hh>
#include <toad/menuhelper.hh>
#include <toad/springlayout.hh>

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

#if 1
TCoord toad::TFigure::OUT_OF_RANGE = HUGE_VAL;
TCoord toad::TFigure::RANGE = 5.0;
TCoord toad::TFigure::INSIDE = -1.0;
#endif

// this class is for backward compability

namespace {

class TFPolyline:
  public TFLine
{
  public:
    const char * getClassName() const { return "toad::TFPolyline"; }
};

} // namespace

void
TFigure::initialize()
{
  TObjectStore& serialize(toad::getDefaultStore());
  serialize.registerObject(new TSerializableRGB());
  serialize.registerObject(new TDialogLayout());
  serialize.registerObject(new TMenuLayout());
  serialize.registerObject(new TSpringLayout());
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
  serialize.registerObject(new TFImage());
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
  mat = cmat = 0;
}

TFigure::TFigure(const TFigure &f)
{
  removeable = f.removeable;
  mat = cmat = 0;
  if (f.mat)
    mat = new TMatrix2D(*f.mat);
  if (f.cmat)
    cmat = new TMatrix2D(*f.cmat);
}

TFigure::~TFigure()
{
  if (mat)
    delete mat;
  if (cmat)
    delete cmat;
}

/**
 * This method is experimental.
 */
bool
TFigure::editEvent(TFigureEditEvent &ee)
{
  switch(ee.type) {
    case TFigureEditEvent::TRANSLATE:
      translate(ee.x, ee.y);
      break;
    case TFigureEditEvent::START_IN_PLACE:
      return startInPlace();
  }
  return true;
}

/**
 * This one is a crude hack to speedup selection in drawings with
 * many beziers. I still need to work it out.
 */
double
TFigure::_distance(TFigureEditor *fe, TCoord x, TCoord y)
{
  return distance(x, y);
}

double
TFigure::distance(TCoord x, TCoord y)
{
  TRectangle r;
  getShape(&r);
  return r.isInside(x, y) ? INSIDE : OUT_OF_RANGE;
}
    
void
TFigure::translate(TCoord dx, TCoord dy)
{
}


TColoredFigure::TColoredFigure()
{
  filled = false;
  closed = false;
  line_color.set(0,0,0);
  fill_color.set(0,0,0);
  alpha = 255;
  line_style = TPen::SOLID;
  line_width = 0;
}

void
TFigure::setAttributes(const TFigureAttributes *preferences)
{
}

void
TFigure::getAttributes(TFigureAttributes *preferences) const
{
}

void
TColoredFigure::setAttributes(const TFigureAttributes *preferences)
{
int a;
  switch(preferences->reason) {
    case TFigureAttributes::ALLCHANGED:
      line_width = preferences->linewidth;
      line_style = preferences->linestyle;
      line_color = preferences->linecolor;
      fill_color = preferences->fillcolor;
      filled     = preferences->filled;
      alpha = preferences->alpha;
      break;
    case TFigureAttributes::LINECOLOR:
      line_color = preferences->linecolor;
      break;
    case TFigureAttributes::FILLCOLOR:
      fill_color = preferences->fillcolor;
      filled     = preferences->filled;
      break;
    case TFigureAttributes::UNSETFILLCOLOR:
      filled     = preferences->filled;
      break;
    case TFigureAttributes::LINEWIDTH:
      line_width = preferences->linewidth;
      break;
    case TFigureAttributes::LINESTYLE:
      line_style = preferences->linestyle;
      break;
    case TFigureAttributes::ALPHA:
      alpha = preferences->alpha;
      break;
  }
}

void
TColoredFigure::getAttributes(TFigureAttributes *preferences) const
{
  preferences->linewidth = line_width;
  preferences->linestyle = line_style;
  preferences->linecolor = line_color;
  preferences->fillcolor = fill_color;
  preferences->filled = filled;
  preferences->alpha  = alpha;
}

/**
 * Called from the gadget editor when the gadget is selected.
 *
 * The default behaviour is to draw small rectangles at the positions
 * delivered by <I>getHandle</I> or at the corners of the rectangle
 * delivered by <I>getShape</I> when <I>getHandle</I> returns `false'
 * for handle 0.
 *
 * When 'handle' is positive, it indicates that the handle of the same
 * number is currently manipulated.
 *
 * \param pen
 *   A pen to draw the figures selection.
 * \param handle
 *   Handle which is currently manipulated or <0, in case of none.
 */
void
TFigure::paintSelection(TPenBase &pen, int handle)
{
  pen.setLineColor(TColor::FIGURE_SELECTION);
  pen.setFillColor(TColor::WHITE);

  const TMatrix2D *m0 = pen.getMatrix();
  if (m0) {
    pen.push();
    pen.identity();
  }

  unsigned h=0;
  TPoint pt;
  while(true) {
    if ( !getHandle(h, &pt) )
      break;
    TCoord x, y;
    if (m0) {
      m0->map(pt.x, pt.y, &x, &y);
    } else {
      x = pt.x;
      y = pt.y;
    }
    pen.setLineWidth(1);
    if (handle!=h) {
      pen.fillRectanglePC(x-2,y-2,5,5);
    } else {
      pen.setFillColor(TColor::FIGURE_SELECTION);
      pen.fillRectanglePC(x-2,y-2,5,5);
      pen.setFillColor(TColor::WHITE);
    }
    h++;
  }
  
  // no handles found, use the figures shape instead
  if (h==0) {
    TRectangle r;
    getShape(&r);
    TCoord x, y;
    for(int i=0; i<4; ++i) {
      switch(i) {
        case 0: x = r.x;       y = r.y;       break;
        case 1: x = r.x+r.w-1; y = r.y;       break;
        case 2: x = r.x+r.w-1; y = r.y+r.h-1; break;
        case 3: x = r.x;       y = r.y+r.h-1; break;
      }
      if (m0)
        m0->map(x, y, &x, &y);
      pen.setLineWidth(1);
      pen.fillRectanglePC(x-2,y-2,5,5);
    }
  }

  if (m0)
    pen.pop();
}

/**
 * Return <I>true</I> and the position of handle <I>n</I> in <I>p</I> or
 * <I>false</I> when there's no handle <I>n</I>.
 *
 * The first handle is 0.
 */
bool
TFigure::getHandle(unsigned n, TPoint *p)
{
  return false;
}

/**
 * This method was added to support TFGroup so that it knows when handle
 * modification begins and when it ends.
 *
 * \return
 *   Returns 'true' when code manipulating the handle must also manipulate
 *   the figure's mat and cmat attributes.
 */
bool
TFigure::startTranslateHandle()
{
  return true;
}

void
TFigure::endTranslateHandle()
{
}

/**
 * Set handle <I>handle</I> to position (x,y).
 */
void
TFigure::translateHandle(unsigned handle, TCoord x, TCoord y, unsigned)
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
TFigure::keyDown(TFigureEditor*, const TKeyEvent&)
{ 
  return CONTINUE; 
}

void
TFigure::startCreate()
{
}

unsigned
TFigure::mouseLDown(TFigureEditor*, const TMouseEvent&)
{
  return STOP;
}

unsigned
TFigure::mouseMove(TFigureEditor*, const TMouseEvent&)
{
  return CONTINUE;
}

unsigned 
TFigure::mouseLUp(TFigureEditor*, const TMouseEvent&)
{
  return CONTINUE;
}

unsigned
TFigure::mouseRDown(TFigureEditor*, const TMouseEvent&)
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
  if (cmat) {
    ::store(out, "coord-trans", cmat);
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
  if (alpha!=255) {
    ::store(out, "alpha", alpha);
  }
}

bool
TFigure::restore(TInObjectStream &in)
{
  if (
    ::restorePtr(in, "trans", &mat) ||
    ::restorePtr(in, "coord-trans", &cmat) ||
    TSerializable::restore(in)
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

  int a;
  if (::restore(in, "alpha", &a)) {
    alpha = a;
    return true;
  }
  
  if (
    ::restore(in, "linecolor", &line_color) ||
    ::restore(in, "linewidth", &line_width) ||
    ::restore(in, "alpha", &alpha) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}

/**
 * Returns the distance of point (x,y) to the line (x1,y1)-(x2,y2).
 */
TCoord 
TFigure::distance2Line(TCoord x, TCoord y, TCoord x1, TCoord y1, TCoord x2, TCoord y2)
{
  TCoord bx = x2 - x1;
  TCoord by = y2 - y1;
  TCoord ax = x-x1;
  TCoord ay = y-y1;
  if (bx==0.0 && by==0.0) {
    return sqrt(ax*ax+ay*ay);
  }
  TCoord lb = bx*bx+by*by;
  TCoord t = (bx * ax + by * ay ) / lb;
  if (t<0.0 || t>1.0)
    return OUT_OF_RANGE;
  return fabs(by * ax - bx * ay) / sqrt(lb);
}
