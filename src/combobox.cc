/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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
      :TButtonBase(parent,title)
    {
      setBorder(false);
    };

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
  setBorder(false);

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
  TPen pen(this);

  pen.draw3DRectanglePC(0,0, getWidth(), getHeight());
#if 1
#if 1
  if (!table->getRenderer()) {
    pen.setColor(TColor::DIALOG);
    pen.fillRectanglePC(2,2, getWidth()-4, getHeight()-4);
    return;
  }
  pen.translate(2, 2);
  table->getRenderer()->renderItem(
    pen,
    table->getLastSelectionCol(), table->getLastSelectionRow(),
    btn->getXPos()-2, getHeight()-4,
    false,
    false,
    isFocus());
#endif
#else
  TAbstractTableSelectionModel *s = table->getSelectionModel();
  TTableSelectionModel::iterator p(s->begin());
  if (p==s->end())
    return;
  pen.translate(2, 2);
  table->getRenderer()->renderItem(
    pen,
    p.getX(), p.getY(),
    btn->getXPos()-2, getHeight()-4,
    false,
    false,
    isFocus());
#endif
}

void
TComboBox::resize()
{
  btn->setShape(
    getWidth()-TScrollBar::getFixedSize()+1 -2, 2,
    TScrollBar::getFixedSize(), getHeight()-4
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
  TPen pen(this);
  drawShadow(pen, bDown, true);

  int n=bDown?1:0;
  int cx = (getWidth() >> 1)+n;
  int cy = (getHeight() >> 1)+n - 1;

  TPoint p[3];
  p[0].set(cx-4,cy-2);
  p[1].set(cx+4,cy-2);
  p[2].set(cx,  cy+2);

  pen.setColor(TColor::BTNTEXT);
  pen.fillPolygon(p,3);
  
  pen.drawRectanglePC(cx-4, cy+4, 9, 2);
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
