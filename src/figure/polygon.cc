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
TFPolygon::getShape(TRectangle *r)
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

  r->set(p1,p2);
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
TFPolygon::getHandle(unsigned handle, TPoint *p)
{
  if (handle >= polygon.size())
    return false;
  *p = polygon[handle];
  return true;
}

void
TFPolygon::translateHandle(unsigned handle, int x, int y, unsigned m)
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
      polygon.addPoint(mx, my);
      editor->setMouseMoveMessages(TWindow::TMMM_ALL);
      break;
    case TFigureEditor::STATE_CREATE: {
      if (m & MK_DOUBLE) {
        if (polygon.size()<4)
          return STOP|DELETE;
        polygon.erase(--polygon.end());
        return STOP;
      }
      TPolygon::iterator p(polygon.end()-2);
      if (p->x != mx || p->y != my) {
        polygon.addPoint(mx, my);
        editor->invalidateFigure(this);
      }
    } break;
    default:
      break;
  }
  return CONTINUE;
}

unsigned 
TFPolygon::mouseMove(TFigureEditor *editor, int mx, int my, unsigned)
{
  TPolygon::iterator p(--polygon.end());
  editor->invalidateFigure(this);
  p->set(mx, my);
  editor->invalidateFigure(this);
  return CONTINUE;
}

unsigned 
TFPolygon::keyDown(TFigureEditor *editor, TKey key, char *str, unsigned)
{
  editor->invalidateFigure(this);
  switch(key) {
    case TK_BACKSPACE:
    case TK_DELETE:
      if (polygon.size()<=1)
        return STOP|DELETE;
      *(polygon.end()-2) = *(polygon.end()-1);
      polygon.erase(polygon.end()-1);
      break;
    case TK_RETURN:
      return STOP;
  }
  editor->invalidateFigure(this);
  return CONTINUE;
}

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

unsigned
TFPolygon::mouseRDown(TFigureEditor *editor, int x, int y, unsigned modifier)
{
  if (editor->state != TFigureEditor::STATE_NONE)
    return NOTHING;

  unsigned i=0;
  bool found=false;
  for(TPolygon::iterator p=polygon.begin();
      p!=polygon.end();
      ++p, ++i)
  {
    if (p->x-editor->fuzziness<=x && x<=p->x+editor->fuzziness && 
        p->y-editor->fuzziness<=y && y<=p->y+editor->fuzziness) 
    {
      // cerr << "found handle " << i << endl;
      found = true;
      break;
    }
  }
  
  if (!found && polygon.size()<=2)
    return NOTHING;

  TInteractor *dummy = new TInteractor(0, "dummy interactor");
//cerr << "create tree " << dummy << endl;
  TAction *action;
  if (!found) {
    action = new TAction(dummy, "add point", TAction::ALWAYS);
    TCLOSURE4(
      action->sigActivate,
      figure, this,
      edit, editor,
      _x, x,
      _y, y,
      edit->invalidateFigure(figure);
      figure->insertPointNear(_x, _y);
      edit->invalidateFigure(figure);
    )
  } else {
    action = new TAction(dummy, "delete point", TAction::ALWAYS);
    TCLOSURE3(
      action->sigActivate,
      figure, this,
      edit, editor,
      _i, i,
      edit->invalidateFigure(figure);
      figure->deletePoint(_i);
      edit->invalidateFigure(figure);
    )
  }
  TMyPopupMenu *menu;
  menu = new TMyPopupMenu(editor, "popup");
  menu->tree = dummy;
  menu->setScopeInteractor(dummy);
  menu->open(x, y, modifier);
  return NOTHING;
}

void
TFPolygon::insertPointNear(int x, int y)
{
  unsigned i=0;
  double min;

  for(unsigned j=0; j < polygon.size(); j++) {
    double d;
    if (j+1<polygon.size()) {
      d = distance2Line(x, y,
                        polygon[j  ].x, polygon[j  ].y,
                        polygon[j+1].x, polygon[j+1].y);
    } else {
      d = distance2Line(x, y,
                        polygon[j  ].x, polygon[j  ].y,
                        polygon[0].x, polygon[0].y);
    }
    if (j==0) {
      min = d;
    } else {
      if (d<min) {
        min = d;
        i = j;
      }
    }
  }

//cout << "i=" << i << ", polygon.size()=" << polygon.size() << endl;
  if (i+1<polygon.size()) {
    polygon.insert(polygon.begin()+i+1, TPoint(x,y));
  } else {
    polygon.insert(polygon.end(), TPoint(x,y));
  }
}

void
TFPolygon::deletePoint(unsigned i)
{
  if (polygon.size()<=2)
    return;

  polygon.erase(polygon.begin()+i);
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

  if (in.attribute.empty() && ::restore(in, &y)) {
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
