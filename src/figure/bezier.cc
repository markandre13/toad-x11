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
TFBezier::paint(TPenBase &pen, EPaintType type)
{
  pen.setLineColor(line_color);
  if (!filled) {
    pen.drawPolyBezier(polygon);
  } else {
    pen.setFillColor(fill_color);
    pen.fillPolyBezier(polygon);
  }
  if (type==EDIT || type==SELECT) {
    pen.setColor(0,0,255);
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
TFBezier::distance(int x, int y)
{
  TPolygon p2;
  TPenBase::poly2Bezier(polygon, p2);
  if (filled && p2.isInside(x, y))
    return INSIDE;
  
  TPolygon::const_iterator p(p2.begin()), e(p2.end());
  int x1,y1,x2,y2;
  double min = OUT_OF_RANGE, d;
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
TFBezier::mouseLDown(TFigureEditor *editor, int mx, int my, unsigned m)
{
  switch(editor->state) {
    case TFigureEditor::STATE_START_CREATE:
      polygon.addPoint(mx, my);
      editor->setMouseMoveMessages(TWindow::TMMM_ALL);
    case TFigureEditor::STATE_CREATE: {
      if (m & MK_DOUBLE) {
//cerr << "end at: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
        if (polygon.size()<=4)
          return STOP|DELETE;
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
        if (polygon.size()>4) {
          TPolygon::iterator e(polygon.end()), p(polygon.begin());
          --e;
          *e = *p;
        }

//cerr << "end with: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
        return STOP;
      }
      if ((polygon.size()%3)==2) {
//cerr << "add point 2 and 3" << endl;
        polygon.addPoint(mx, my);
      }
      polygon.addPoint(mx, my);
//cerr << "after add: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
      editor->invalidateFigure(this);
      } break;
    default:
      break;
  }
  return CONTINUE;
}


void
TFBezierline::paint(TPenBase &pen, EPaintType type)
{
  if (type==EDIT || type==SELECT) {
    pen.setColor(0,0,255);
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

  pen.setColor(line_color);
  pen.drawPolyBezier(polygon);
}

double
TFBezierline::distance(int x, int y)
{
  TPolygon p2;
  TPenBase::poly2Bezier(polygon, p2);
  
  TPolygon::const_iterator p(p2.begin()), e(p2.end());
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
    d = distance2Line(x,y, x1,y1, x2,y2);
    if (d<min)
      min = d;
    ++p;
  }
  return min;
}

unsigned
TFBezierline::mouseLDown(TFigureEditor *editor, int mx, int my, unsigned m)
{
  switch(editor->state) {
    case TFigureEditor::STATE_START_CREATE:
      polygon.addPoint(mx, my);
      editor->setMouseMoveMessages(TWindow::TMMM_ALL);
    case TFigureEditor::STATE_CREATE: {
      if (m & MK_DOUBLE) {
//cerr << "end at: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
        if (polygon.size()<=4)
          return STOP|DELETE;
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
/*
        TPolygon::iterator e(polygon.end()), p(polygon.begin());
        --e;
        *e = *p;
*/
//cerr << "end with: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
        return STOP;
      }
      if ((polygon.size()%3)==2) {
//cerr << "add point 2 and 3" << endl;
        polygon.addPoint(mx, my);
      }
      polygon.addPoint(mx, my);
//cerr << "after add: n=" << polygon.size() << ", n%3=" << (polygon.size()%3) << endl;
      editor->invalidateFigure(this);
      } break;
    default:
      break;
  }
  return CONTINUE;
}

unsigned
TFBezierline::mouseMove(TFigureEditor *editor, int mx, int my, unsigned m)
{
  TPolygon::iterator p(polygon.end());
  --p;
  editor->invalidateFigure(this);
  unsigned n = polygon.size();
//cerr << "n=" << n << ", n%3=" << (n%3) << endl;
  if (n>3 && (n%3)==1) {
    p->set(mx, my);
    --p;
  }
  if (n>3 && (n%3)==2) {
    TPolygon::iterator p2 = polygon.end();
    p2-=2;
    TPolygon::iterator p1 = p2;
    --p1;
    p1->set( p2->x - (mx - p2->x),
             p2->y - (my - p2->y) );
  }
  p->set(mx, my);
  editor->invalidateFigure(this);
  return CONTINUE;
}
