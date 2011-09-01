/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2007 by Mark-André Hopf <mhopf@mark13.org>
 * Visit http://www.mark13.org/fischland/.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "fpath.hh"
#include <toad/action.hh>
#include <toad/popupmenu.hh>
#include <toad/figureeditor.hh>

#include <cmath>

void
TFPath::getShape(toad::TRectangle *r)
{
  polygon.getShape(r);
}

void
TFPath::translate(TCoord dx, TCoord dy)
{
  TPolygon::iterator p(polygon.begin()), e(polygon.end());
  while(p!=e) {
    p->x+=dx;
    p->y+=dy;
    ++p;
  }
}

bool 
TFPath::getHandle(unsigned handle, TPoint *p)
{
  if (handle >= polygon.size())
    return false;
  *p = polygon[handle];
  return true;
}

void
TFPath::translateHandle(unsigned handle, TCoord x, TCoord y, unsigned m)
{
  // 0 0  1 1 1  2 2 2  3 3 3
  // 0 1  2 3 4  5 6 7  8 9 10
//  cout << "handle: " << handle << endl;
  
//  cout << "corner: " << (handle+1)/3 << endl;
//  cout << "no    : " << (handle+1)%3 << endl;
  unsigned c = 3;
  unsigned i = (handle+1)/3;
  if (i<corner.size())
    c = corner[i];
//  cout << "i=" << i << ", c=" << c << endl;
  switch( (handle+1)%3 ) {
    case 0:
      if ( c == 4) {
        polygon[handle].x = x;
        polygon[handle].y = y;
        if (handle+2 <= polygon.size()) {
          polygon[handle+2].x = polygon[handle+1].x + (polygon[handle+1].x - polygon[handle].x);
          polygon[handle+2].y = polygon[handle+1].y + (polygon[handle+1].y - polygon[handle].y);
        }
      } else
      if ( c & 1 ) {
        polygon[handle].x = x;
        polygon[handle].y = y;
      } else {
        if (handle+2 < polygon.size()) {
          if ((c & 2) == 0) {
            polygon[handle+2].x = x;
            polygon[handle+2].y = y;
          } else {
            polygon[handle+2].x += x - polygon[handle].x;
            polygon[handle+2].y += y - polygon[handle].y;
          }
        }
        polygon[handle].x = x;
        polygon[handle].y = y;
        if (handle+1 < polygon.size()) {
          polygon[handle+1].x = x;
          polygon[handle+1].y = y;
        }
      }
      break;
    case 1: {
        TCoord dx = x - polygon[handle].x;
        TCoord dy = y - polygon[handle].y;
        if (handle>0) {
          polygon[handle-1].x += dx;
          polygon[handle-1].y += dy;
        } else 
        if (closed) {
          polygon.back().x = x;
          polygon.back().y = y;
          polygon[polygon.size()-2].x += dx;
          polygon[polygon.size()-2].y += dy;
        }
        polygon[handle].x = x;
        polygon[handle].y = y;
        if (handle+1<polygon.size()) {
          polygon[handle+1].x += dx;
          polygon[handle+1].y += dy;
        }
      } break;
    case 2:
      if ( c == 4) {
        polygon[handle].x = x;
        polygon[handle].y = y;
        if (handle>1) {
          polygon[handle-2].x = polygon[handle-1].x + (polygon[handle-1].x - polygon[handle].x);
          polygon[handle-2].y = polygon[handle-1].y + (polygon[handle-1].y - polygon[handle].y);
        }
      } else
      if ( c & 2 ) {
        polygon[handle].x = x;
        polygon[handle].y = y;
      } else {
        if (handle>1) {
          if ((c & 1) == 0) {
            polygon[handle-2].x = x;
            polygon[handle-2].y = y;
          } else {
            polygon[handle-2].x += polygon[handle].x - x;
            polygon[handle-2].y += polygon[handle].y - y;
          }
        }
        polygon[handle].x = x;
        polygon[handle].y = y;
        if (handle>0) {
          polygon[handle-1].x = x;
          polygon[handle-1].y = y;
        }
      }
      break;
  }
  
  TPoint p(x, y);
  polygon[handle]=p;
}

void
TFPath::paintSelection(TPenBase &pen, int handle)
{
  pen.setLineWidth(1);

  TMatrix2D _m0;
  const TMatrix2D *m0 = pen.getMatrix();
  if (m0 || cmat) {
    pen.push();
    pen.identity();
    if (cmat) {
      if (!m0) {
        m0 = cmat;
      } else {
        _m0 = *m0;
        _m0 *= *cmat;
        m0 = &_m0;
      }
    }
  }
  
  pen.setLineColor(TColor::FIGURE_SELECTION);
  pen.setFillColor(TColor::WHITE);

//  if (type==EDIT || type==SELECT) {
    for(TPolygon::size_type i=0; i<polygon.size(); i+=3) {
      // line before corner
      TCoord x0, y0, x1, y1;
      if (i>0) {
        x0 = polygon[i].x;
        y0 = polygon[i].y;
        x1 = polygon[i-1].x;
        y1 = polygon[i-1].y;
        if (m0) {
          m0->map(x0, y0, &x0, &y0);
          m0->map(x1, y1, &x1, &y1);
        }
        pen.drawLine(x0, y0, x1, y1);
        pen.fillCirclePC(x1-2,y1-2,6,6);
      }
/*
 else
      if (!closed && i+1<polygon.size()) {
        x0 = polygon[i].x;
        y0 = polygon[i].y;
        x1 = polygon[i].x - (polygon[i+1].x - polygon[i].x);
        y1 = polygon[i].y - (polygon[i+1].y - polygon[i].y);
        if (m0) {
          m0->map(x0, y0, &x0, &y0);
          m0->map(x1, y1, &x1, &y1);
        }
        pen.drawLine(x0, y0, x1, y1);
        pen.fillCirclePC(x1-2,y1-2,6,6);
      }
*/    
      // line after corner
      if (i+1<polygon.size()) {
        x0 = polygon[i].x;
        y0 = polygon[i].y;
        x1 = polygon[i+1].x;
        y1 = polygon[i+1].y;
        if (m0) {
          m0->map(x0, y0, &x0, &y0);
          m0->map(x1, y1, &x1, &y1);
        }
        pen.drawLine(x0, y0, x1, y1);
        pen.fillCirclePC(x1-2,y1-2,6,6);
      }
//    }
    for(TPolygon::size_type i=0; i<polygon.size(); i+=3) {
      TCoord x, y;
      if (m0) {
        m0->map(polygon[i].x, polygon[i].y, &x, &y);
      } else {
        x = polygon[i].x;
        y = polygon[i].y;
      }
      pen.fillRectanglePC(x-2,y-2,5,5);
    }
  }
  
  if (m0 || cmat) {
    pen.pop();
  }
}


void
TFPath::paint(TPenBase &pen, EPaintType type)
{
  pen.setAlpha(alpha);
  pen.setColor(line_color);
  pen.setLineStyle(line_style);
  pen.setLineWidth(line_width);

  if (!cmat) {
    if (!closed || !filled) {  
      pen.drawBezier(polygon);
//      pen.fillCircle(polygon[0].x-100, polygon[0].y-100, 200, 200);
    } else {
      pen.setFillColor(fill_color);
      pen.fillBezier(polygon);
//      pen.fillCircle(polygon[0].x-100, polygon[0].y-100, 200, 200);
    }
  } else {
    TPoint polygon2[polygon.size()];
    TPoint *p2 = polygon2;
    for(TPolygon::const_iterator p = polygon.begin();
        p != polygon.end();
        ++p, ++p2)
    {
      cmat->map(p->x, p->y, &p2->x, &p2->y);
    }
    if (!closed || !filled) {  
      pen.drawBezier(polygon2, polygon.size());
    } else {
      pen.setFillColor(fill_color);
      pen.fillBezier(polygon2, polygon.size());
    }
  }

  if (type!=EDIT && type!=SELECT)
    return;

  paintSelection(pen, -1);
}

double
TFPath::_distance(TFigureEditor *fe, TCoord x, TCoord y)
{
  if (!polygon.isInside(x, y)) {
    TCoord x1,y1,x2,y2;
    TCoord min = OUT_OF_RANGE;
    TPolygon::const_iterator p(polygon.begin());
    x2=p->x;
    y2=p->y;
    ++p;
    while(p!=polygon.end()) {
      x1=x2;
      y1=y2;
      x2=p->x;
      y2=p->y;
      TCoord d = distance2Line(x,y, x1,y1, x2,y2);
      if (d<min)
        min = d;
      ++p;
    }
    if (min > 0.5*fe->fuzziness*TFigure::RANGE)
      return OUT_OF_RANGE;
  }
   
  TPolygon p2;
  TPenBase::poly2Bezier(polygon, p2);
  if (closed && filled) {
    if (p2.isInside(x, y))
      return INSIDE;
  }
  
  TPolygon::const_iterator p(p2.begin()), e(p2.end());
  TCoord x1,y1,x2,y2;
  TCoord min = OUT_OF_RANGE, d;
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
TFPath::mouseRDown(TFigureEditor *editor, const TMouseEvent &me)
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
//      cerr << "found handle " << i << endl;
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
//cout << "distance between ("<<x<<","<<y<<")-("<<x1<<","<<y1<<") = " << sqrt(ax*ax+ay*ay) << endl;
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
TFPath::insertPointNear(TCoord x, TCoord y)
{
//  cerr << "add point near " << x << ", " << y << endl;

  unsigned i=0, j;
  TCoord f, min;

  for(j=0; j+3 <= polygon.size(); j+=3) {
    TCoord u, d;
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
       
  TCoord x0 = f*(polygon[i+1].x-polygon[i+0].x) + polygon[i+0].x;
  TCoord y0 = f*(polygon[i+1].y-polygon[i+0].y) + polygon[i+0].y;
  TCoord x1 = f*(polygon[i+2].x-polygon[i+1].x) + polygon[i+1].x;
  TCoord y1 = f*(polygon[i+2].y-polygon[i+1].y) + polygon[i+1].y;
  TCoord x2 = f*(polygon[i+3].x-polygon[i+2].x) + polygon[i+2].x;
  TCoord y2 = f*(polygon[i+3].y-polygon[i+2].y) + polygon[i+2].y;

  TCoord x3 = f*(x1-x0) + x0;
  TCoord y3 = f*(y1-y0) + y0;
  TCoord x4 = f*(x2-x1) + x1;
  TCoord y4 = f*(y2-y1) + y1;

  TCoord x5 = f*(x4-x3) + x3;
  TCoord y5 = f*(y4-y3) + y3;

  j = (i+1) / 3;
//  cout << "insert corner " << j << endl;
  if (j<=corner.size())
    corner.insert(corner.begin()+j, 4);

  polygon[i+1].set(x0,y0);
  polygon.insert(polygon.begin()+i+2, TPoint(x3,y3));
  polygon.insert(polygon.begin()+i+3, TPoint(x5,y5));
  polygon.insert(polygon.begin()+i+4, TPoint(x4,y4));
  polygon[i+5].set(x2,y2);
}

/**
 * Find a point on the path nearest to (inX, inY) and return the distance
 * to the path and the point in (outX, outY).
 */
TCoord
TFPath::findPointNear(TCoord inX, TCoord inY, TCoord *outX, TCoord *outY, TCoord *outF) const
{
  assert(polygon.size()>=4);
  unsigned i=0, j;
  TCoord f, f0, min;

  for(j=0; j+3 <= polygon.size(); j+=3) {
    TCoord u, d;
    u = bezpoint(inX, inY,
                 polygon[j  ].x, polygon[j  ].y,
                 polygon[j+1].x, polygon[j+1].y,
                 polygon[j+2].x, polygon[j+2].y,
                 polygon[j+3].x, polygon[j+3].y,
                 0.0, 1.0, &d);
//cout << "distance to ("<<inX<<","<<inY<<" is "<<d<<endl;
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
       
  TCoord x0 = f*(polygon[i+1].x-polygon[i+0].x) + polygon[i+0].x;
  TCoord y0 = f*(polygon[i+1].y-polygon[i+0].y) + polygon[i+0].y;
  TCoord x1 = f*(polygon[i+2].x-polygon[i+1].x) + polygon[i+1].x;
  TCoord y1 = f*(polygon[i+2].y-polygon[i+1].y) + polygon[i+1].y;
  TCoord x2 = f*(polygon[i+3].x-polygon[i+2].x) + polygon[i+2].x;
  TCoord y2 = f*(polygon[i+3].y-polygon[i+2].y) + polygon[i+2].y;

  TCoord x3 = f*(x1-x0) + x0;
  TCoord y3 = f*(y1-y0) + y0;
  TCoord x4 = f*(x2-x1) + x1;
  TCoord y4 = f*(y2-y1) + y1;

  *outX = f*(x4-x3) + x3;
  *outY = f*(y4-y3) + y3;
  if (outF) {
    *outF = f + (i/3);
//cout << "findPointNear: f="<<f<<", i="<<i<<", *outF="<<*outF<<endl;
  }

  return min;  
}
 
void
TFPath::deletePoint(unsigned i)
{
  // don't delete curve handles
  if ((i%3)!=0)
    return;
  if (polygon.size()<=4)
    return;

  unsigned j = (i+1) / 3;
//  cout << "delete corner " << j << endl;
  if (j<corner.size())
   corner.erase(corner.begin()+j);

  if (i==0) {
    polygon.erase(polygon.begin(), polygon.begin()+3);
  } else 
  if (i==polygon.size()-1) {
    polygon.erase(polygon.end()-3, polygon.end());
  } else {
    polygon.erase(polygon.begin()+i-1, polygon.begin()+i+2);
  }
}  

void
TFPath::store(TOutObjectStream &out) const
{
  TColoredFigure::store(out);
  ::store(out, "closed", closed);
  unsigned i = 0;
  for(TPolygon::const_iterator p = polygon.begin();
      p != polygon.end();
      ++p, ++i)
  {
    if (i==0 || i%3 == 2 ) {
      out.indent();
      unsigned c = 3;
      unsigned j = (i+1)/3;
      if (j<corner.size())
        c = corner[j];
      out << c;
    }
    out << ' ' << p->x << ' ' << p->y;
  }
}

// 0 1 2 3 4 5 6 7 8 9
// --- ----- ----- ----

bool
TFPath::restore(TInObjectStream &in)
{
  if (in.what == ATV_VALUE && in.attribute.empty() && in.type.empty()) {
//    cerr << "corner: " << in.value << endl;
    unsigned n;
    if (polygon.empty()) {
      n = 2;
    } else {
      n = 3;
    }
    corner.push_back(atoi(in.value.c_str()));
    
    in.setInterpreter(0);
    for(unsigned i=0; i<n; ++i) {
      if (!in.parse())
        break;
      if (in.what == ATV_FINISHED) {
        in.putback('}');
        break;
      }
      int x, y;
      x = atoi(in.value.c_str());
//      cerr << in.value << ", ";
      in.parse();
      y = atoi(in.value.c_str());
      polygon.addPoint(x, y);
//      cerr << in.value << ", ";
    }
//    cerr << endl;
    in.setInterpreter(this);
    return true;
  }
  bool b;
  if (::restore(in, "closed", &b)) {
    closed = b;
    return true;
  }
  if (TColoredFigure::restore(in))
    return true;
  ATV_FAILED(in)
  return false;
}
