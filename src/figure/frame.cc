/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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
TFFrame::paint(TPenBase &pen, EPaintType type)
{
  TPoint p[3];
  pen.setColor(TColor::WHITE);
  TRectangle r(p1, p2);
  p[0].set(r.x      , r.y+r.h-1);
  p[1].set(r.x+r.w-1, r.y+r.h-1);
  p[2].set(r.x+r.w-1, r.y);
  pen.drawLines(p,3);

  p[0].set(r.x+1  , r.y+r.h-2);
  p[1].set(r.x+1  , r.y+1);
  p[2].set(r.x+r.w-2, r.y+1);
  pen.drawLines(p,3);
  pen.setColor(TColor::GRAY);
  pen.drawRectanglePC(r.x,r.y,r.w-1,r.h-1);

  if (!text.empty()) {
    pen.setFont(fontname);
    int fh = pen.getHeight();
    int tw = pen.getTextWidth(text);
    pen.setColor(TColor::DIALOG);
    pen.fillRectanglePC(r.x+5-1, r.y-fh/2, tw+2, fh);
    pen.setColor(line_color);
    pen.drawString(r.x+5, r.y-fh/2, text);
  }

  if (type==EDIT) {
    pen.setColor(line_color);
    int fh = TOADBase::getDefaultFont().getHeight();
    unsigned dx = pen.getTextWidth(text.substr(0, cx))+5;
    int yp = r.y-fh/2;
    pen.drawLine(r.x+dx,yp,r.x+dx,yp+pen.getHeight());
  }
}

void
TFFrame::getShape(TRectangle *r)
{
  PFont font = TPen::lookupFont(fontname);
  int a = font->getHeight()/2;
  TFRectangle::getShape(r);
  r->y-=a;
  r->h+=a;
}

TCoord
TFFrame::distance(TCoord mx, TCoord my)
{
//  cout << __PRETTY_FUNCTION__ << endl;
#if 1
  if (!text.empty()) {
    PFont font = TPen::lookupFont(fontname);
    TCoord fh = font->getHeight();
    TCoord tw = font->getTextWidth(text);
    TRectangle r(min(p1.x,p2.x)+5-1, min(p1.y,p2.y)-fh/2, tw+2, fh);
    if (r.isInside(mx, my))
      return INSIDE;
  }
#endif
  filled = false;
//cout << "is " << mx << "," << my
//     <<" in " << x << "," << y << "," << w << "," << h << endl;
  return TFRectangle::distance(mx,my);
}

unsigned
TFFrame::stop(TFigureEditor *editor)
{
  return STOP; // NOTHING;
}

unsigned
TFFrame::keyDown(TFigureEditor *editor, const TKeyEvent &ke)
{
  TKey key = ke.key();
  string txt = ke.str();
  unsigned m = ke.modifier();
//  cout << __PRETTY_FUNCTION__ << endl;
  if (key==TK_RETURN)
    return STOP;

//  int fh = TOADBase::DefaultFont().Height();
//  int tw = TOADBase::DefaultFont().TextWidth(text);
//  TRect r(x+5-1, y-fh/2, tw+2, fh);
  
  editor->invalidateFigure(this);
  unsigned result = TFText::keyDown(editor, ke);
//  r.w = TOADBase::DefaultFont().TextWidth(text)+2;
  editor->invalidateFigure(this);
  return result;
}

bool
TFFrame::getHandle(unsigned handle, TPoint *p)
{
//  cout << __PRETTY_FUNCTION__ << endl;
  return TFRectangle::getHandle(handle, p);
}

static bool flag;

unsigned 
TFFrame::mouseLDown(TFigureEditor *e, const TMouseEvent &me)
{
//  cout << __PRETTY_FUNCTION__ << endl;

  switch(e->state) {
    case TFigureEditor::STATE_START_CREATE:
cout << "start create frame " << this << endl;
flag = true;
      TFRectangle::mouseLDown(e,me);
      TFText::mouseLDown(e,me);
      break;
      
    case TFigureEditor::STATE_CREATE:
    case TFigureEditor::STATE_EDIT:
cout << "create/edit frame " << this << endl;
      if (distance(me.x,me.y)>RANGE) {
        e->invalidateFigure(this);
        cout << "stop" << endl;
        return STOP|REPEAT;
      }
      cout << "still in range" << endl;
      break;
      
    default:
      break;
  }
  return CONTINUE;
}

unsigned 
TFFrame::mouseMove(TFigureEditor *e, const TMouseEvent &me)
{
cout << "mouse move frame " << this << endl;
  if (flag)
    TFRectangle::mouseMove(e,me);
  return CONTINUE;
}

unsigned 
TFFrame::mouseLUp(TFigureEditor *e, const TMouseEvent &me)
{
cout << "mouse up frame " << this << endl;
  TFRectangle::mouseLUp(e,me);
flag = false;
  return CONTINUE;
}

void
TFFrame::calcSize()
{
//  cout << __PRETTY_FUNCTION__ << endl;
}
