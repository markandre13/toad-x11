/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

int TFText::cx = 0;

void 
TFText::calcSize()
{
  int w=0, h=0;
  unsigned l,r;
  l = 0;
  while(true) {
    PFont font = TPen::lookupFont(fontname);
    h+=font->getHeight();
    r = text.find('\n', l);
    int wl = font->getTextWidth(text.substr(l,r==string::npos ? r : r-l));
    if (wl>w)
      w=wl;
    if (r==string::npos)
      break;
    l = r+1;
  }
  p2.x=p1.x+w+1;
  p2.y=p1.y+h+1;
}

void 
TFText::paint(TPenBase &pen, EPaintType type)
{
  pen.setFont(fontname);
  pen.setLineColor(line_color);
#if 0
  if (filled) {
    pen.setFillColor(fill_color);
    TPoint q1, q2;
    if (pen.mat) {
      pen.mat->map(p1.x, p1.y, &q1.x, &q1.y);
      pen.mat->map(p2.x, p2.y, &q2.x, &q2.y);
      pen.push();
      pen.identity();
    }
    TRectangle r(q1, q2);
    pen.fillRectangle(r.x-2, r.y-2, r.w+4, r.h+4);
    if (pen.mat) {
      pen.pop();
    }
  }
#endif  
  unsigned l, r;
  int yp = p1.y;
  l = 0;
  while(true) {
    r = text.find('\n', l);
    pen.drawString(p1.x,yp, text.substr(l,r==string::npos ? r : r-l));
    if (type==EDIT && l<=cx && cx<=r) {
      unsigned dx = pen.getTextWidth(text.substr(l, cx-l));
      pen.drawLine(p1.x+dx,yp,p1.x+dx,yp+pen.getHeight());
    }
    if (r==string::npos)
      break;
    l = r+1;
    yp+=pen.getHeight();
  }
}

double 
TFText::distance(int mx, int my)
{
TRectangle r(p1, p2);
// cerr << "mouse at (" << mx << ", " << my << "), text " << r << endl;

  if (TRectangle(p1, p2).isInside(mx, my))
    return INSIDE;
  return super::distance(mx,my);
}

bool
TFText::getHandle(unsigned, TPoint*)
{
  return false;
}

bool 
TFText::startInPlace()
{
  cx = 0;
  return true;
}

void 
TFText::startCreate()
{
  cx = 0;
}

unsigned 
TFText::stop(TFigureEditor*)
{
  if (text.empty())
    return STOP|DELETE;
  return STOP;
}

unsigned 
TFText::keyDown(TFigureEditor *editor, TKey key, char *str, unsigned)
{
  editor->invalidateFigure(this);
  switch(key) {
    case TK_LEFT:
      if (cx>0) {
        cx--;
#ifdef TOAD_OLD_FONTCODE
        if (toad::_x11corefontmethod!=TOAD_X11CFM_DEPRECATED)
#endif
        while( ((unsigned char)text[cx] & 0xC0) == 0x80)
          --cx;
      }
      break;
    case TK_RIGHT:
      if (cx<text.size()) {
        cx++;
#ifdef TOAD_OLD_FONTCODE
        if (toad::_x11corefontmethod!=TOAD_X11CFM_DEPRECATED)
#endif
        while( ((unsigned char)text[cx] & 0xC0) == 0x80)
          cx++;
      }
      break;
    case TK_UP:
      break;
    case TK_DOWN:
      break;
    case TK_HOME:
      if (cx==0 || text[cx-1]=='\n')
        break;
      cx = text.rfind('\n', cx-1);
      if (cx==string::npos)
        cx=0;
      else
        cx++;
      break;
    case TK_END:
      cx = text.find('\n', cx);
      if (cx==string::npos)
        cx=text.size();
      break;
    case TK_BACKSPACE:
      if (cx>0) {
        cx--;
#ifdef TOAD_OLD_FONTCODE
        if (toad::_x11corefontmethod!=TOAD_X11CFM_DEPRECATED)
#endif
        while( ((unsigned char)text[cx] & 0xC0) == 0x80)
          --cx;
      } else {
        break;
      }
    case TK_DELETE:
      text.erase(cx,1);
#ifdef TOAD_OLD_FONTCODE
      if (toad::_x11corefontmethod!=TOAD_X11CFM_DEPRECATED)
#endif
      while( ((unsigned char)text[cx] & 0xC0) == 0x80) {
        text.erase(cx,1);
      }
      break;
    case TK_RETURN:
      text.insert(cx, 1, '\n');
      cx++;
      break;
    default:
      if ((unsigned char)str[0]>=32 || str[1]!=0) {
        text.insert(cx, str);
        cx+=strlen(str);
      }
  }
  calcSize();
  editor->invalidateFigure(this);
  return CONTINUE;
}

unsigned 
TFText::mouseLDown(TFigureEditor *editor, int x, int y, unsigned)
{
  switch(editor->state) {
    case TFigureEditor::STATE_START_CREATE:
      cx = 0;
      p1.x = x;
      p1.y = y;
      calcSize();
      editor->invalidateFigure(this);
      startInPlace();
      break;
      
    case TFigureEditor::STATE_CREATE:
    case TFigureEditor::STATE_EDIT:
      if (distance(x,y)>RANGE) {
        editor->invalidateFigure(this);
        if (text.empty())
          return STOP|DELETE|REPEAT;
        return STOP|REPEAT;
      }
      break;
      
    default:
      break;
  }
  return CONTINUE;
}

unsigned
TFText::mouseMove(TFigureEditor*, int x, int y, unsigned)
{
  return CONTINUE;
}

unsigned 
TFText::mouseLUp(TFigureEditor*, int, int, unsigned)
{
  return CONTINUE;
}

void
TFText::setAttributes(const TFigureAttributes *preferences)
{
  super::setAttributes(preferences);
  switch(preferences->reason) {
    case TFigureAttributes::ALLCHANGED:
    case TFigureAttributes::FONTNAME:
      fontname = preferences->fontname;
      calcSize();
      break;
  }
}

void
TFText::getAttributes(TFigureAttributes *preferences) const
{
  super::getAttributes(preferences);
  preferences->fontname = fontname;
}

void 
TFText::store(TOutObjectStream &out) const
{
  super::store(out);
  ::store(out, "fontname", fontname);
  ::store(out, "text", text);
}

bool
TFText::restore(TInObjectStream &in)
{
  if (
    ::restore(in, "text", &text) ||
    ::restore(in, "fontname", &fontname) ||
    super::restore(in)
  ) {
    if (in.what == ATV_FINISHED) {
      calcSize();
    }
    return true;
  }
  ATV_FAILED(in)
  return false;
}
