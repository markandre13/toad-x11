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

#include "colorpalette.hh"

#include <toad/fatcheckbutton.hh>
#include <toad/pushbutton.hh>
#include <fstream>

using namespace toad;

TColorPalette::TColorPalette(TWindow *parent, const string &title,
                             TFigureAttributes *preferences):
  TWindow(parent, title)
{
//  bNoBackground = true;
  this->preferences = preferences;
  bStaticFrame = true;
  bParentlessAssistant = true;
  setBackground(TColor::DIALOG);
  setSize(320, 153);
  
  fw = fh = 12;
  
  int x, y, w, h;

  TPushButton *btn;
  TFatCheckButton *fcb;
  TTextField *tf;
  
  palettelistrenderer = new TPaletteListAdapter(&palettes);
  y = 4;
  w = getWidth();
  TComboBox *cb = new TComboBox(this, "palettes");
  h = cb->getHeight();
  w-= 4 + h + h + 4;
  cb->setToolTip("select color palette");
  cb->setAdapter(palettelistrenderer);
  cb->setShape(4,y,w,h);
  connect(cb->sigSelection, this, &TThis::paletteSelected, cb);
  
  btn = new TPushButton(this, "add palette");
  btn->setToolTip("create a new palette");
  btn->setShape(4+w+1,y+1,h-2,h-2);
  btn->setLabel("+");

  btn = new TPushButton(this, "delete palette");
  btn->setToolTip("delete current palette");
  btn->setShape(4+w+h+1,y+1,h-2,h-2);
  btn->setLabel("-");

  rectColors.set(4,y+h+4, getWidth()-4-4, getHeight() - 4 - h - 4 - 4 - h - 4);

  sb = new TScrollBar(this, "updown", &position);
  rectColors.w -= sb->getWidth()+2;
  sb->setShape(rectColors.x+rectColors.w, rectColors.y,
               TSIZE_PREVIOUS, rectColors.h);
  position.setValue(0);
  connect(position.sigChanged, this, &TWindow::invalidateWindow, true);
  
  tf = new TTextField(this, "colorname", &colorname);
  h = tf->getHeight();
  y = getHeight() - 4 - h;
  w = getWidth() - 4 - h - h - 4;
  tf->setShape(4+h,y,w-h,TSIZE_PREVIOUS);

  fcb = new TFatCheckButton(this, "linecolor", &doLineColor);
  fcb->setToolTip("set as line color");
  fcb->setLabel("L");
  fcb->setShape(4+w,y,h,h);
  TCLOSURE1(
    doLineColor.sigChanged,
    _this, this,
    if (_this->doLineColor)
      _this->assignLineColor();
  )
  
  fcb = new TFatCheckButton(this, "fillcolor", &doFillColor);
  fcb->setToolTip("set as fill color");
  fcb->setLabel("F");
  fcb->setShape(4+w+h,y,h,h);
  TCLOSURE1(
    doFillColor.sigChanged,
    _this, this,
    if (_this->doFillColor)
      _this->assignFillColor();
  )

  rectCurrent.set(4,y,h,h);

  TDropSiteColor *ds = new TDropSiteColor(this, rectColors);
  connect_value(ds->sigDrop, this, &TColorPalette::dropColor, ds);

  loadPalette("COPIC.txt");  
  loadPalette("palette.txt");
  loadPalette("CGA.txt");

  selectPalette(0);
}

void
TColorPalette::mouseEvent(const TMouseEvent &me)
{
  if (me.type == TMouseEvent::ROLL_UP ||
      me.type == TMouseEvent::ROLL_UP_END ||
      me.type == TMouseEvent::ROLL_DOWN ||
      me.type == TMouseEvent::ROLL_DOWN_END)
  {
    sb->mouseEvent(me);
    return;
  }
  TWindow::mouseEvent(me);
}

void
TColorPalette::selectPalette(int n)
{
  invalidateWindow();
  ppalette = 0;
  pcolor = 0;

  if (palettes.size()<=n)
    return;
  ppalette = palettes[n];
  if (!ppalette->rgb.empty()) {
    pcolor = &ppalette->rgb[0];
    colorname = pcolor->name;
  }
  int fields_per_row = rectColors.w / fw;
#if 1
  position.setExtent(rectColors.h / fh);
  position.setMinimum(0);
  position.setMaximum(ppalette->rgb.size()/fields_per_row);
  position.setValue(0);
#else
  position.setRangeProperties(0,                  // value
                              rectColors.h / fh,  // extent
                              0,                  // min
                              ppalette->rgb.size()/fields_per_row);
#endif
}

void
TColorPalette::paletteSelected(TComboBox *cb)
{
  unsigned i = cb->getSelectionModel()->getRow();
  selectPalette(i /*cb->getLastSelectionRow()*/);
}

bool
TColorPalette::loadPalette(const string &filename)
{
  ifstream in;
  in.open(filename.c_str());
  if (!in) {
    cerr << "failed to open color palette '" << filename << "'\n";
    return false;
  }
  TNamedPalette *np = new TNamedPalette();
  palettes.push_back(np);
  np->modified = false;
  np->filename = filename;
  np->name = filename;
  
  char buffer[256];
  char name[256];
  int r, g, b;
  in.getline(buffer, sizeof(buffer));
  while(in) {
    in.getline(buffer, sizeof(buffer));
    if (buffer[0]=='\0' || buffer[0]==';')
      continue;
    if (buffer[0]!='#')
      sscanf(buffer, "%i %i %i %[ -~]", &r, &g, &b, name); /*"*/
    else
      sscanf(buffer, "#%2x%2x%2x %[ -~]", &r, &g, &b, name); /*"*/
    np->rgb.push_back(TNamedColor(r/255.0, g/255.0, b/255.0, name));
  }
  npos = 0;
  palettelistrenderer->modelChanged();
  return true;
}

TColorPalette::TNamedColor *
TColorPalette::mouse2color(TCoord mx, TCoord my)
{
  if (!ppalette)
    return 0;

  TCoord x, y;
  x = rectColors.x;
  y = rectColors.y;
  
  int first = position * (rectColors.w / fw);
  if (first>=ppalette->rgb.size())
    return 0;
  
  for(TPalette::iterator p = ppalette->rgb.begin()+first;
      p!=ppalette->rgb.end();
      ++p)
  {
    if (x<=mx && mx<=x+fw && y<=my && my<=y+fh) {
      return &(*p);
    } 
    x+=fw;
    if (x+fw>=rectColors.x+rectColors.w) {
      x = rectColors.x;
      y+=fh;
      if (y+fh>=rectColors.y+rectColors.h) {
        break;
      }
    }
  }
  return 0;
}

void
TColorPalette::paint()
{
  if (!ppalette)
    return;

  TPen pen(this);
  if (pcolor!=0) {
    pen.setColor(*pcolor);
    pen.fillRectangle(rectCurrent);
  }
  
  int x, y;
  x = rectColors.x;
  y = rectColors.y;
  
  int first = position * (rectColors.w / fw);
  if (first>=ppalette->rgb.size())
    return;
  
  for(TPalette::iterator p = ppalette->rgb.begin()+first;
      p!=ppalette->rgb.end();
      ++p)
  {
    pen.setColor(*p);
    if (&(*p)!=pcolor) {
      pen.fillRectanglePC(x+1,y+1,fw-1,fh-1);
    } else {
      pen.fillRectanglePC(x,y,fw,fh);
      pen.setColor(0,0,0);
      pen.drawRectanglePC(x-1, y-1, fw+2, fh+2);
    }
    
    x+=fw;
    if (x+fw>=rectColors.x+rectColors.w) {
      x = rectColors.x;
      y+=fh;
      if (y+fh>=rectColors.y+rectColors.h) {
        break;
      }
    }
  }
}

void
TColorPalette::mouseLDown(const TMouseEvent &me)
{
  TNamedColor *color = mouse2color(me.x, me.y);
  if (!color)
    return;
  colorname = color->name;
  pcolor = color;
  if (doLineColor)
    assignLineColor();
  if (doFillColor)
    assignFillColor();
  invalidateWindow();
}

void
TColorPalette::mouseMDown(const TMouseEvent &me)
{
  TNamedColor *color = mouse2color(me.x, me.y);
  if (!color)
    return;  
  startDrag(new TDnDColor(*color), me.modifier());
}

void
TColorPalette::dropColor(const PDnDColor &drop)
{
  TNamedColor *color = mouse2color(drop->x, drop->y);
  if (!color)
    return;
//  *color = drop->rgb;
  color->set(drop->rgb.r, drop->rgb.g, drop->rgb.b);
  invalidateWindow();
}

void
TColorPalette::assignLineColor()
{
  if (preferences && pcolor)
    preferences->setLineColor(*pcolor);
}

void
TColorPalette::assignFillColor()
{
  if (preferences && pcolor)
    preferences->setFillColor(*pcolor);
}
