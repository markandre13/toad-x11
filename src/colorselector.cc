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

#include <toad/colorselector.hh>
#include <toad/figureeditor.hh>
#include <toad/colordialog.hh>
#include <toad/pushbutton.hh>

using namespace toad;

TColorSelector::TColorSelector(TWindow *parent,
                               const string &title,
                               TFigurePreferences *gedit):
  super(parent, title)
{
  dialogeditorhack = false;
  this->gedit = gedit;
  setSize(64, 32);

  w2 = getWidth() - 16;
  h = getHeight();
  border = min(h,w2) / 4 + 1;

  setBorder(0);
  setBackground(TColor::DIALOG);
  filled = false; 
  linecolor.set(0,0,0);
  fillcolor.set(255,255,255);

  pb1 = new TPushButton(this, "palette");
  pb1->bNoFocus = true;
  pb1->loadBitmap("memory://toad/colorpalette.png");
  pb1->setToolTip("Open Color Palette");
  pb1->setShape(w2,0,getWidth()-w2,getHeight()/2);
  connect(pb1->sigActivate, this, &TColorSelector::openColorPalette);

  pb2 = new TPushButton(this, "coloreditor");
  pb2->bNoFocus = true;
  pb2->loadBitmap("memory://toad/colordialog.png");
  pb2->setToolTip("Open Color Editor");
  pb2->setShape(w2,getHeight()/2,getWidth()-w2,getHeight()/2);
  connect(pb2->sigActivate, this, &TColorSelector::openColorDialog);

  ds = new TDropSiteColor(this, TRectangle(0,0,w2,h));
  connect_value(ds->sigDrop, this, &TColorSelector::dropColor, ds);
  if (gedit)                
    connect(gedit->sigChanged, this, &TColorSelector::preferencesChanged);
}

TColorSelector::~TColorSelector()
{
  if (gedit)
    disconnect(gedit->sigChanged, this);
}

void
TColorSelector::preferencesChanged()
{
  filled = gedit->filled;
  linecolor = gedit->linecolor;
  fillcolor = gedit->fillcolor;
  invalidateWindow();
}


void
TColorSelector::resize()
{
  w2 = getWidth() - 16;
  h = getHeight();
  border = min(h,w2) / 4 + 1;
  pb1->setShape(w2,0,getWidth()-w2,getHeight()/2);
  pb2->setShape(w2,getHeight()/2,getWidth()-w2,getHeight()/2);
  ds->setShape(0,0,w2,h);
}
 
void
TColorSelector::paint()
{
  TPen pen(this);
  
  pen.draw3DRectanglePC(0, 0, w2, h, false);

  pen.setColor(linecolor);
  pen.fillRectanglePC(2, 2, w2-4, h-4);

  if (filled) {
    pen.setColor(fillcolor);
    pen.fillRectanglePC(border, border, w2-border*2, h-border*2);
  } else {
    pen.setColor(255,255,255);
    pen.fillRectanglePC(border, border, w2-border*2, h-border*2);
#if 0
    pen.setColor(0,0,0);
    pen.drawLine(border+2, border+2, w2-border-2, h-border-2);
    pen.drawLine(border+2, h-border-2, w2-border-2, border+2);
#else
    pen.setColor(255,0,0);
    pen.drawLine(border+2, h-border-2-2, w2-border-4, border+2);
    pen.drawLine(border+2, h-border-2-1, w2-border-3, border+2);
    pen.drawLine(border+3, h-border-3, w2-border-2, border+2);
#endif
  }
  pen.draw3DRectanglePC(border, border, w2-border*2, h-border*2, filled);
}  
   
void
TColorSelector::mouseLDown(int x, int y, unsigned modifier)
{
  if (x<border ||
      y<border ||
      x>w2-border ||
      y>h-border)  
  {
    return;
  }

  if (!gedit) {
    invalidateWindow();
    return;
  }
  gedit->setLineColor(linecolor);
  if (!filled)
    gedit->setFillColor(fillcolor);
  else
    gedit->unsetFillColor();
}

void
TColorSelector::mouseMDown(int x, int y, unsigned modifier)
{
  if (x<border ||
      y<border ||
      x>w2-border ||
      y>h-border)  
  {
    startDrag(new TDnDColor(linecolor), modifier);
  } else {
    startDrag(new TDnDColor(fillcolor), modifier);
  }
}

void
TColorSelector::dropColor(const PDnDColor &drop)
{
  if (!gedit) {
    return;
  }

  if (drop->x<border ||
      drop->y<border ||
      drop->x>w2-border ||
      drop->y>h-border)  
  {
    linecolor.set(drop->rgb.r, drop->rgb.g, drop->rgb.b);
    if (gedit)
      gedit->setLineColor(linecolor);
  } else {
    fillcolor.set(drop->rgb.r, drop->rgb.g, drop->rgb.b);
    if (gedit && filled)
      gedit->setFillColor(fillcolor);
  }
  invalidateWindow();
}

void
TColorSelector::openColorDialog()
{
  TWindow *dlg = new TColorDialog(dialogeditorhack ? this : 0,
                                  "Color Editor");
  dlg->createWindow();
}

void
TColorSelector::openColorPalette()
{
}
