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
#include <toad/figureeditor.hh>

#include <toad/action.hh>
#include <toad/popupmenu.hh>

using namespace toad;

void
TFBezierline::paintSelectionLines(TPenBase &pen)
{
    pen.setColor(TColor::FIGURE_SELECTION);
    pen.setLineStyle(TPen::SOLID);
    TPolygon::const_iterator p(polygon.begin());
    unsigned n = polygon.size();
    if (pen.getMatrix()) {
      const TMatrix2D *mat = pen.getMatrix();
      pen.push();
      pen.identity();
      pen.setLineWidth(1);
      while(true) {
        if (n>=2) {
          int x0, y0, x1, y1;
          mat->map(p->x, p->y, &x0, &y0);
          mat->map((p+1)->x, (p+1)->y, &x1, &y1);
          pen.drawLine(x0, y0, x1, y1);
        }
        if (n>=4) {
          int x0, y0, x1, y1;
          mat->map((p+2)->x, (p+2)->y, &x0, &y0);
          mat->map((p+3)->x, (p+3)->y, &x1, &y1);
          pen.drawLine(x0, y0, x1, y1);
        } else {
          break;
        }
        p+=3;
        n-=3;
      }
      pen.pop();
    } else {
      pen.setLineWidth(1);
      while(true) {
        if (n>=2) {
          pen.drawLine(p->x, p->y,
                       (p+1)->x, (p+1)->y);
        }
        if (n>=4) {
          pen.drawLine((p+2)->x, (p+2)->y,
                       (p+3)->x, (p+3)->y);
        } else {
          break;
        }
        p+=3;
        n-=3;
      }
    }
}

void
TFBezierline::paint(TPenBase &pen, EPaintType type)
{
  if (type==EDIT || type==SELECT) {
    paintSelectionLines(pen);
  }

  pen.setAlpha(alpha);
  pen.setColor(line_color);
  pen.setLineStyle(line_style);
  pen.setLineWidth(line_width);
  pen.drawBezier(polygon);

  if (arrowmode == NONE) {
    pen.setAlpha(255);
    return;
  }
  pen.setLineStyle(TPen::SOLID);

  int aw = arrowwidth * line_width;
  int ah = arrowheight * line_width;

  if (arrowmode == HEAD || arrowmode == BOTH)
    drawArrow(pen, polygon[polygon.size()-1], polygon[polygon.size()-2], line_color, fill_color, aw, ah, arrowtype);
  if (arrowmode == TAIL || arrowmode == BOTH)
    drawArrow(pen, polygon[0], polygon[1], line_color, fill_color, aw, ah, arrowtype);
  pen.setAlpha(255);
}

/**
 * Like TFigure::paintSelection but with slightly different behaviour
 * to ease bezier editing.
 */
void
TFBezierline::paintSelection(TPenBase &pen, int handle)
{
  _paintSelection(pen, handle, false);
}

void
TFBezier::paintSelection(TPenBase &pen, int handle)
{
  _paintSelection(pen, handle, true);
}

void
TFBezierline::_paintSelection(TPenBase &pen, int handle, bool filled)
{
  pen.setLineColor(TColor::FIGURE_SELECTION);
  pen.setFillColor(TColor::WHITE);
  unsigned h=0;
  TPoint pt;   
  while(true) {
    if ( !getHandle(h, &pt) )
      break;
    int x, y;
    if (pen.getMatrix()) {
      pen.getMatrix()->map(pt.x, pt.y, &x, &y);
      pen.push();
      pen.identity();
    } else {
      x = pt.x;
      y = pt.y;
    }
    if (handle!=h) {
      if ((h%3)==0) {
        pen.fillRectanglePC(x-2,y-2,5,5);
      } else {
        bool b = false;
        if ( (handle%3)==0) {
          b = h+1==handle || h-1==handle;
          if (filled) {
            if (handle==0) {
              if (polygon.size()-2==h)
                b = true;
            }
            if (handle==polygon.size()-1) {
              if (1==h)
                b = true;
            }
          }
        }
        if (b) {
          pen.setFillColor(TColor::FIGURE_SELECTION);
          pen.fillCirclePC(x-2,y-2,6,6);
          pen.setFillColor(TColor::WHITE);
        } else {
          pen.fillCirclePC(x-2,y-2,6,6);
        }
      }
    } else {
      pen.setFillColor(TColor::FIGURE_SELECTION);
      if ((h%3)==0) {
        pen.fillRectanglePC(x-2,y-2,5,5);
      } else {
        pen.fillCirclePC(x-2,y-2,6,6);
      }
      pen.setFillColor(TColor::WHITE); 
    }
    if (pen.getMatrix())
      pen.pop();
    h++;
  }
}    

double
TFBezierline::_distance(TFigureEditor *fe, TCoord x, TCoord y)
{
  if (!polygon.isInside(x, y)) {
    TCoord x1,y1,x2,y2;
    double min = OUT_OF_RANGE;
    TPolygon::const_iterator p(polygon.begin());
    x2=p->x;
    y2=p->y;
    ++p;
    while(p!=polygon.end()) {
      x1=x2;
      y1=y2;
      x2=p->x;
      y2=p->y;
      double d = distance2Line(x,y, x1,y1, x2,y2);
      if (d<min)
        min = d;
      ++p;
    }
    if (min > 0.5*fe->fuzziness*TFigure::RANGE)
      return OUT_OF_RANGE;
  }

  TPolygon p2;
  TPenBase::poly2Bezier(polygon, p2);
  
  TPolygon::const_iterator p(p2.begin()), e(p2.end());
  TCoord x1,y1,x2,y2;
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
    d = distance2Line(x,y, x1,y1, x2,y2);
    if (d<min)
      min = d;
    ++p;
  }
  return min;
}

bool flag0 = false;

unsigned
TFBezierline::mouseLDown(TFigureEditor *editor, const TMouseEvent &me)
{
cout << "mouseLDown, polygon.size()=" << polygon.size() << endl;
  if (polygon.size()>3) {
    TPolygon::iterator p(polygon.end());
    --p;
#if 1
    if (p->x == me.x && p->y == me.y) {
      --p;
      if (p->x == me.x && p->y == me.y) {
        flag0 = true;
        cout << "flag 0" << endl;
      }
    }
#endif
  }

  if (polygon.size()==0)
    polygon.addPoint(me.x, me.y);
  polygon.addPoint(me.x, me.y);
    // editor->getWindow()->setMouseMoveMessages(TWindow::TMMM_ALL);
  return CONTINUE;
}

unsigned
TFBezierline::mouseMove(TFigureEditor *editor, const TMouseEvent &me)
{
cout << "mouseMove" << endl;
  if (polygon.size()<2)
    return CONTINUE;
//cout << polygon.size() % 4 << ":" << (polygon.size() % 4 == 2) << ":" << (polygon.size() % 4 == 0) << endl;

  if (polygon.size() % 3 == 2) {
    editor->invalidateFigure(this);
    TPolygon::iterator p(polygon.end());
    --p;
    p->set(me.x, me.y);
    if (!flag0 && polygon.size()>=3) {
      --p;
      TPolygon::iterator p0 = p;
      --p;
      p->set( p0->x - (me.x - p0->x),
              p0->y - (me.y - p0->y) );
    }
    editor->invalidateFigure(this);
  }
  if (polygon.size() % 3 == 1) {
    editor->invalidateFigure(this);
    TPolygon::iterator p(polygon.end());
    --p;
    p->set(me.x, me.y);
    --p;
    p->set(me.x, me.y);
    editor->invalidateFigure(this);
  }
  return CONTINUE;
}

unsigned
TFBezierline::mouseLUp(TFigureEditor *editor, const TMouseEvent &me)
{
cout << "mouseLUp" << endl;
  flag0 = false;
//  if (polygon.size()==2) {
cout << "add 2,3" << endl;
    polygon.addPoint(me.x, me.y);
    polygon.addPoint(me.x, me.y);
    editor->invalidateFigure(this);
//  }
  return CONTINUE;
}

void 
TFBezierline::translateHandle(unsigned handle, TCoord x, TCoord y, unsigned modifier)
{
  _translateHandle(handle, x, y, modifier, false);
}

void
TFBezier::translateHandle(unsigned handle, TCoord x, TCoord y, unsigned m)
{
  _translateHandle(handle, x, y, m, true);
}

void 
TFBezierline::_translateHandle(unsigned h0, TCoord x, TCoord y, unsigned m, bool filled)
{
  if ((h0%3)==0) {
    TCoord dx = x - polygon[h0].x;
    TCoord dy = y - polygon[h0].y;
    polygon[h0].x = x;
    polygon[h0].y = y;
    if (filled && h0==0 && polygon.size()>3) {
      h0=polygon.size()-1;
      polygon[h0].x = x;
      polygon[h0].y = y;
      --h0;
      polygon[h0].x += dx;
      polygon[h0].y += dy;
      polygon[1].x += dx;
      polygon[1].y += dy;
    } else {
      if (h0>0) {
        polygon[h0-1].x += dx;
        polygon[h0-1].y += dy;
      }
      if (h0+1<polygon.size()) {
        polygon[h0+1].x += dx;
        polygon[h0+1].y += dy;
      }
    }
  } else {
    unsigned h1, h2;
    bool b = true;
    if ( (h0%3)==1 ) {
      if (h0>2) {
        h1 = h0-1;
        h2 = h0-2;
      } else {
        if (filled && polygon.size()>=7) {
          h1 = polygon.size()-1;
          h2 = h1-1;
        } else {
          b = false;
        }
      }
    } else {
      if (h0<polygon.size()-3) {
        h1 = h0+1;
        h2 = h0+2;
      } else {
        if (filled && polygon.size()>=7) {
          h1=0;
          h2=1;
        } else {
          b = false;
        }
      }
    }
    
    polygon[h0].x=x;
    polygon[h0].y=y;

    if (b) {    
      if (m & MK_SHIFT) {
        // symmetric
        polygon[h2].x = polygon[h1].x + polygon[h1].x - polygon[h0].x;
        polygon[h2].y = polygon[h1].y + polygon[h1].y - polygon[h0].y;
      } else
      if (m & MK_CONTROL) {
        // on a line
        double dy = polygon[h1].y - polygon[h2].y;
        double dx = polygon[h1].x - polygon[h2].x;
        double d = atan2((double)polygon[h1].y - polygon[h0].y,
                         (double)polygon[h1].x - polygon[h0].x);
        double n = sqrt(dx*dx+dy*dy);
        polygon[h2].x = polygon[h1].x + cos(d)*n;
        polygon[h2].y = polygon[h1].y + sin(d)*n;
      }
    }
  }
}

namespace {

class TMyPopupMenu:
  public TPopupMenu
{
  public:
    TMyPopupMenu(TWindow *p, const string &t): TPopupMenu(p, t)
    {
//cerr << "create menu " << this << endl;
    }
    ~TMyPopupMenu() {
//cerr << "delete tree " << tree << endl;
      delete tree;
    }
    
    void closeRequest() {
      TPopupMenu::closeRequest();
//cerr << "delete menu " << this << endl;
      delete this;
    }
    
    TInteractor *tree;
};

}

unsigned
TFBezierline::mouseRDown(TFigureEditor *editor, const TMouseEvent &me)
{
//  cerr << "TFBezierline::mouseRDown" << endl;
//cerr << " at (" << x << ", " << y << ")\n";
//cerr << " 1s point at (" << polygon[0].x << ", " << polygon[0].y << ")\n";

//cerr << "editor->fuzziness = " << editor->fuzziness << endl;
  unsigned i=0;
  bool found=false;
  for(TPolygon::iterator p=polygon.begin();
      p!=polygon.end();
      ++p, ++i)
  {
    if (p->x-editor->fuzziness<=me.x && me.x<=p->x+editor->fuzziness && 
        p->y-editor->fuzziness<=me.y && me.y<=p->y+editor->fuzziness) 
    {
      // cerr << "found handle " << i << endl;
      found = true;
      break;
    }
  }
  
  if (found && (i%3)!=0)
    return NOTHING;

  TInteractor *dummy = new TInteractor(0, "dummy interactor");
//cerr << "create tree " << dummy << endl;
  TAction *action;
  if (!found) {
    action = new TAction(dummy, "add point", TAction::ALWAYS);
    TCLOSURE4(
      action->sigClicked,
      figure, this,
      edit, editor,
      _x, me.x,
      _y, me.y,
      edit->invalidateFigure(figure);
      figure->insertPointNear(_x, _y);
      edit->invalidateFigure(figure);
    )
    action = new TAction(dummy, "split");
  } else {
    action = new TAction(dummy, "delete point", TAction::ALWAYS);
    TCLOSURE3(
      action->sigClicked,
      figure, this,
      edit, editor,
      _i, i,
      if (figure->polygon.size()<=4) {
        edit->deleteFigure(figure);
      } else {
        edit->invalidateFigure(figure);
        figure->deletePoint(_i);
      }
      edit->invalidateFigure(figure);
    )
    action = new TAction(dummy, "split");
  }
  // action = new TAction(dummy, "sharp edge");
  // action = new TAction(dummy, "no edge");
  TMyPopupMenu *menu;
  menu = new TMyPopupMenu(editor, "popup");
  menu->tree = dummy;
  menu->setScopeInteractor(dummy);
  menu->open(me.x, me.y, me.modifier());
  return NOTHING;
}

namespace {

inline double
mid(double a, double b)
{
  return (a + b) / 2.0;
}

inline double
distance(double x, double y, double x1, double y1)
{
  double ax = x-x1;
  double ay = y-y1;
  return sqrt(ax*ax+ay*ay);
}

double
bezpoint(
  double px, double py,
  double x0, double y0,
  double x1, double y1,
  double x2, double y2,
  double x3, double y3,
  double min=0.0, double max=1.0,
  double *dist = 0)
{
  double vx0 = x1-x0;
  double vx1 = x2-x1;
  double vx2 = x3-x2;
  double vy0 = y1-y0;
  double vy1 = y2-y1;
  double vy2 = y3-y2;

  double w0 = vx0 * vy1 - vy0 * vx1;
  double w1 = vx1 * vy2 - vy1 * vx2;
  
  double vx3 = x2 - x0;
  double vx4 = x3 - x0;
  double vy3 = y2 - y0;
  double vy4 = y3 - y0;
  
  double w2 = vx3 * vy4 - vy3 * vx4;
  double w3 = vx0 * vy4 - vy0 * vx4;

  if (fabs(w0)+fabs(w1)+fabs(w2)+fabs(w3)<1.0) {
    double mind, d, f;
    mind = distance(px, py, x0, y0);
    f = 0.0;
    d = distance(px, py, x1, y1);
    if (d<mind) {
      mind = d;  
      f = 1.0;   
    }
    d = distance(px, py, x2, y2);
    if (d<mind) {
      mind = d;  
      f = 2.0;   
    }
    d = distance(px, py, x3, y3);
    if (d<mind) {
      mind = d;  
      f = 3.0;   
    }

    if (dist)
      *dist = mind;
    return min + (max-min)*f/3.0;
  }
   
  double xx  = mid(x1, x2);
  double yy  = mid(y1, y2);
  double x11 = mid(x0, x1);
  double y11 = mid(y0, y1);
  double x22 = mid(x2, x3);
  double y22 = mid(y2, y3);
  double x12 = mid(x11, xx);
  double y12 = mid(y11, yy);
  double x21 = mid(xx, x22);
  double y21 = mid(yy, y22);
  double cx  = mid(x12, x21);
  double cy  = mid(y12, y21);
  double d1, d2, t1, t2;
  t1 = bezpoint(px, py, x0, y0, x11, y11, x12, y12, cx, cy, min, min+(max-min)/2.0, &d1);
  t2 = bezpoint(px, py, cx, cy, x21, y21, x22, y22, x3, y3, min+(max-min)/2.0, max, &d2);
  if (dist) {
    *dist = (d1<d2) ? d1 : d2;
  }
  return (d1<d2) ? t1 : t2;
}

} // namespace

/**
 * Insert an additional point near the point given by x, y.
 */
void
TFBezierline::insertPointNear(TCoord x, TCoord y)
{
//  cerr << "add point near " << x << ", " << y << endl;

  unsigned i=0;
  double f, min;

  for(unsigned j=0; j+3 <= polygon.size(); j+=3) {
    double u, d;
    u = bezpoint(x, y,
                 polygon[j  ].x, polygon[j  ].y,
                 polygon[j+1].x, polygon[j+1].y,
                 polygon[j+2].x, polygon[j+2].y,
                 polygon[j+3].x, polygon[j+3].y,
                 0.0, 1.0, &d);
    if (j==0) {
      i = j;
      f = u;
      min = d;
    } else {
      if (d<min) {
        min = d;
        f = u;
        i = j;
      }
    }
  }

  double x0 = f*(polygon[i+1].x-polygon[i+0].x) + polygon[i+0].x;
  double y0 = f*(polygon[i+1].y-polygon[i+0].y) + polygon[i+0].y;
  double x1 = f*(polygon[i+2].x-polygon[i+1].x) + polygon[i+1].x;
  double y1 = f*(polygon[i+2].y-polygon[i+1].y) + polygon[i+1].y;
  double x2 = f*(polygon[i+3].x-polygon[i+2].x) + polygon[i+2].x;
  double y2 = f*(polygon[i+3].y-polygon[i+2].y) + polygon[i+2].y;

  double x3 = f*(x1-x0) + x0;
  double y3 = f*(y1-y0) + y0;
  double x4 = f*(x2-x1) + x1;
  double y4 = f*(y2-y1) + y1;

  double x5 = f*(x4-x3) + x3;
  double y5 = f*(y4-y3) + y3;

  polygon[i+1].set(x0,y0);
  polygon.insert(polygon.begin()+i+2, TPoint(x3,y3));
  polygon.insert(polygon.begin()+i+3, TPoint(x5,y5));
  polygon.insert(polygon.begin()+i+4, TPoint(x4,y4));
  polygon[i+5].set(x2,y2);
}

void
TFBezierline::deletePoint(unsigned i)
{
  // don't delete curve handles
  if ((i%3)!=0)
    return;
  if (polygon.size()<=4)
    return;
    
  if (i==0) {
    polygon.erase(polygon.begin(), polygon.begin()+3);
  } else 
  if (i==polygon.size()-1) {
    polygon.erase(polygon.end()-3, polygon.end());
  } else {
    polygon.erase(polygon.begin()+i-1, polygon.begin()+i+2);
  }
}

/*
 * TFBezier is derived from TFBezierline
 */

void 
TFBezier::store(TOutObjectStream &out) const
{
  // skip the line super classes TFBezierLine -> TFLine -> TFPolygon -> ...
  // with their arrow parameters
  TFPolygon::store(out);
}

void
TFBezier::setAttributes(const TFigureAttributes *attr)
{
  // skip the line super classes TFBezierLine -> TFLine -> TFPolygon -> ...
  // with their different handling of the 'filled' flag
  TFPolygon::setAttributes(attr);
}

void
TFBezier::paint(TPenBase &pen, EPaintType type)
{
  pen.setAlpha(alpha);
  pen.setLineColor(line_color);
  pen.setLineStyle(line_style);
  pen.setLineWidth(line_width);
  
  if (!filled) {
    pen.drawBezier(polygon);
  } else {
    pen.setFillColor(fill_color);
    pen.fillBezier(polygon);
  }

  pen.setAlpha(255);  
  
  if (type==EDIT || type==SELECT) {
    paintSelectionLines(pen);
    return;
    pen.setLineStyle(TPen::SOLID);
    pen.setLineWidth(0);
    pen.setColor(TColor::FIGURE_SELECTION);
//    pen.setLineStyle(TPen::DOT);
    TPolygon::const_iterator p(polygon.begin());
    unsigned n = polygon.size();
    while(true) {
      if (n>=2) {
        pen.drawLine(p->x, p->y,
                     (p+1)->x, (p+1)->y);
      }
      if (n>=4) {
        pen.drawLine((p+2)->x, (p+2)->y,
                     (p+3)->x, (p+3)->y);
      } else {
        break;
      }
      p+=3;
      n-=3;
    }
//    pen.setLineStyle(TPen::SOLID);
  }
}

double
TFBezier::_distance(TFigureEditor *fe, TCoord x, TCoord y)
{
  if (!polygon.isInside(x, y))
    return OUT_OF_RANGE;

  TPolygon p2;
  TPenBase::poly2Bezier(polygon, p2);
  if (filled && p2.isInside(x, y))
    return INSIDE;
  
  TPolygon::const_iterator p(p2.begin()), e(p2.end());
  TCoord x1,y1,x2,y2;
  TCoord min = OUT_OF_RANGE, d;
  assert(p!=e);
  --e;
  assert(p!=e);
  x2=p->x;
  y2=p->y;
  ++e;
  assert(p!=e);
  while(p!=e) {
    x1=x2;
    y1=y2;
    x2=p->x;
    y2=p->y;
    d = distance2Line(x,y, x1,y1, x2,y2);
    if (d<min)
      min = d;
    ++p;
  }
  return min;
}

unsigned
TFBezier::mouseLDown(TFigureEditor *editor, const TMouseEvent &me)
{
  switch(editor->state) {
    case TFigureEditor::STATE_START_CREATE:
      polygon.addPoint(me.x, me.y);
      editor->setAllMouseMoveEvents(true);
    case TFigureEditor::STATE_CREATE: {
      if (false /*m & MK_DOUBLE*/) {
//cerr << "end at: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
        if (polygon.size()<=4) {
          return STOP|DELETE;
        }
        editor->invalidateFigure(this);
        if ((polygon.size()%3)==1) {
          TPolygon::iterator e(polygon.end());
          e-=3;
          polygon.erase(e, polygon.end());
        } else
        if ((polygon.size()%3)==2) {
          TPolygon::iterator e(polygon.end());
          --e;
          polygon.erase(e, polygon.end());
        }
        if (polygon.size()<4)
          return STOP|DELETE;

        // join first and last point
        TPolygon::iterator e(polygon.end()), p(polygon.begin());
        --e;
        *e = *p;

//cerr << "end with: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
        return STOP;
      }
      if ((polygon.size()%3)==2) {
//cerr << "add point 2 and 3" << endl;
        polygon.addPoint(me.x, me.y);
      }
      polygon.addPoint(me.x, me.y);
//cerr << "after add: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
      editor->invalidateFigure(this);
      } break;
    default:
      break;
  }
  return CONTINUE;
}
