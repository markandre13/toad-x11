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

#include <toad/toad.hh>

#include <toad/combobox.hh>

#include <toad/buttonbase.hh>
#include <toad/scrollbar.hh>

using namespace toad;

class TComboBox::TComboButton:
  public TButtonBase
{
  public:
    TComboButton(TWindow *parent, const string &title)
    :TButtonBase(parent,title){};

  protected:
    void paint();
    void mouseLDown(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void mouseEnter(int,int,unsigned);
    void mouseLeave(int,int,unsigned);
};   

TComboBox::TComboBox(TWindow * parent, const string &title):
  super(parent, title)
{
  setSize(320, 20);

  btn = new TComboButton(this,"combobox.combobutton");
  connect(btn->sigArm, this, &TComboBox::button);
  connect(btn->sigDisarm, this, &TComboBox::button);
  
  table = new TTable(this, "table");
  connect(table->sigSelection, this, &TComboBox::selected);

  table->bExplicitCreate = true;
  table->bPopup = true;
  table->bSaveUnder = true;
  // table->_faked_focus = true;
}

void
TComboBox::paint()
{
  TTableSelectionModel *s = table->getSelectionModel();
  TTableSelectionModel::iterator p(s->begin());
  if (p==s->end())
    return;
  TPen pen(this);
  table->getRenderer()->renderItem(
    pen,
    p.getX(), p.getY(),
    getWidth(), getHeight(),
    false,
    false);
}

void
TComboBox::resize()
{
  btn->setShape(
    getWidth()-TScrollBar::getFixedSize()+1,-1,
    TScrollBar::getFixedSize(),getHeight()+2
  );
}

void
TComboBox::button()
{
  if (btn->isDown()) {
    table->setSize(getWidth(), (getDefaultFont().getHeight()+1)*8);
    placeWindow(table, PLACE_PULLDOWN, this);
    table->setMapped(true);
    if (table->isRealized()) {
      table->raiseWindow();
    } else {
      table->createWindow();
    }
  } else {
    table->setMapped(false);
  }
}

void
TComboBox::selected()
{
  btn->setDown(false);
  invalidateWindow();
  sigSelection();
}

//----------------

void
TComboBox::TComboButton::paint()
{
  TPen g(this);
  drawShadow(g, bDown);
  TPoint p[4];
  const int d=6;
  int n=bDown?1:0;

  int max = _h>_w ? _h : _w;
  int x = (_w-max)>>1;
  int y = (_h-max)>>1;

  p[0].set(x+n+(max>>1) , y+n+_h-d);
  p[1].set(x+n+max-d  , y+n+d-1);   
  p[2].set(x+n+d      , y+n+d-1);   

  g.setColor(TColor::BTNTEXT);
  g.fillPolygon(p,3);
  g.fillRectanglePC(n+d-2, n+_h-d+1, _w-d-d+4,2);
}

void
TComboBox::TComboButton::mouseLDown(int,int,unsigned)
{
  setDown(!isDown());
  if (isDown()) {
    sigArm();
  } else {
    sigDisarm();
  }
}

void
TComboBox::TComboButton::mouseLUp(int,int,unsigned)
{
}

void
TComboBox::TComboButton::mouseEnter(int,int,unsigned)
{
}

void
TComboBox::TComboButton::mouseLeave(int,int,unsigned)
{
}
