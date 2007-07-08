/*
 * TPaint -- a simple bitmap editor
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <toad/toad.hh>
#include <toad/dragndrop.hh>
#include <toad/dialog.hh>
#include <toad/pushbutton.hh>
#include <toad/textfield.hh>
#include <toad/scrollbar.hh>

#include "ColorEditor.hh"

extern TColor color[1];

TColorEditor::TColorEditor(TWindow *p,const string &t)
  :super(p,t)
{
  #warning "had to modify bShell to enabled DnD in MDI child"
  /*
    the problem is that the DnD information isn't moved forward to the
    top level window when TColorEditor loses it's bShellFlag
  */
  bShell=false;

  TDropSiteColor *dsc = new TDropSiteColor(this, TRectangle(70,40,80,80));
  CONNECT_VALUE(dsc->sigDrop, this,dropColor, dsc);

  bFocusManager = true;
  setBackground(TColor::DIALOG);

  r.setRangeProperties(0,1,0,256);
  g.setRangeProperties(0,1,0,256);
  b.setRangeProperties(0,1,0,256);

  // setSize(220,160);

  TPushButton *btn;
  btn = new TPushButton(this,"apply");
    CONNECT(btn->sigClicked, this, sigApply);

  TScrollBar *wnd;
  TTextField *tf;

  wnd = new TScrollBar(this, "red.sb", &r);
    CONNECT(wnd->getModel()->sigChanged, this, changed);

  wnd = new TScrollBar(this, "green.sb", &g);
    CONNECT(wnd->getModel()->sigChanged, this, changed);

  wnd = new TScrollBar(this, "blue.sb", &b);
    CONNECT(wnd->getModel()->sigChanged, this, changed);

  tf = new TTextField(this,"red.tf", &r);
  tf = new TTextField(this,"green.tf", &g);
  tf = new TTextField(this,"blue.tf", &b);
    
  loadLayout("dialog#TColorEditor");
}

void TColorEditor::mouseMDown(int,int,unsigned m)
{
  startDrag(new TDnDColor(color), m);
}

void TColorEditor::dropColor(PDnDColor dc)
{
cerr << "enter: dropColor" << endl;
  color = dc->rgb;
  r = color.r;
  g = color.g;
  b = color.b;
  TPen pen(this);

  pen.setColorMode(TColor::DITHER);
  pen.setColor(color.r,color.g,color.b);
  pen.fillRectangle(70,40,80,80);
cerr << "leave: dropColor" << endl;
}

void TColorEditor::paint()
{
  super::paint();
  TPen pen(this);

  pen.setColorMode(TColor::DITHER125);
  pen.setColor(color.r,color.g,color.b);
  pen.fillRectangle(70,40,80,80);
}

void TColorEditor::changed()
{
  color.r = r;
  color.g = g;
  color.b = b;
  invalidateWindow(false);
}

void TColorEditor::sigApply()
{
  color.r = r;
  color.g = g;
  color.b = b;
  ::color[0]=color;
}
