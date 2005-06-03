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
  table->selectionFollowsMouse = true;
  connect(table->sigClicked, this, &TComboBox::selected);
  connect(table->sigDoubleClicked, this, &TComboBox::selected);

  table->bExplicitCreate = true;
  table->bPopup = true;
  table->bSaveUnder = true;
  // table->_faked_focus = true;
}

void
TComboBox::paint()
{
//cout << "TComboBox '"<<getTitle()<<"': render" << endl;
  TPen pen(this);

  pen.draw3DRectanglePC(0,0, getWidth(), getHeight());
  if (!table->getAdapter()) {
    pen.setColor(TColor::DIALOG);
    pen.fillRectanglePC(2,2, getWidth()-4, getHeight()-4);
//cout << "  no table adapter" << endl;
    return;
  }
#if 1
  // checks which should already be handled in TTable...
  int col, row;
  TSingleSelectionModel *sm = 0;
  if (table->getSelectionModel())
    sm = dynamic_cast<TSingleSelectionModel*>(table->getSelectionModel());
  if (sm) {
    col = sm->getCol();
    row = sm->getRow();
  } else {
    // this is a hack, the right way would be to find the first selected
    // entry but the current selection model API doesn't provide such an
    // interface...
    col = table->getLastSelectionCol();
    row = table->getLastSelectionRow();
  }

  if (table->getAdapter()->getRows() <= row) {
    cerr << "TComboBox '"<<getTitle()<<"': tables selection model was out of renderer range" << endl;
    pen.setColor(128,64,64);
    pen.fillRectanglePC(2,2, getWidth()-4, getHeight()-4);
    return;
  }
#endif
  pen.translate(2, 2);
  TTableEvent te;
  te.col = col;
  te.row = row;
  te.w = btn->getXPos()-2;
  te.h = getHeight()-4;
  te.cursor = false;
  te.selected = false;
  te.focus = true;
  if (!sm || !sm->isEmpty()) {
    table->getAdapter()->renderItem(pen, te);
  }

  if (isFocus()) {
    pen.setColor(0,0,0);
    pen.setLineStyle(TPen::SOLID);
    pen.setLineWidth(1);
    pen.drawRectanglePC(0,0, getWidth()-4-btn->getWidth(), getHeight()-4);
  }
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
TComboBox::focus(bool b)
{
  // in case we're loosing the focus, close the combobox
  if (b==false && table->isMapped()) {
    btn->setDown(false);
  }
  invalidateWindow();
}

void
TComboBox::button()
{
  if (btn->isDown()) {
    if (!isFocus())
      setFocus();
    table->setSize(getWidth(), (getDefaultFont().getHeight()+1)*8);
    placeWindow(table, PLACE_PULLDOWN, this);
    table->setMapped(true);
    if (table->isRealized()) {
      table->raiseWindow();
    } else {
      table->createWindow();
    }
    table->setFocus();
    grabPopupMouse();
  } else {
    table->setMapped(false);
    ungrabMouse();
  }
}

void
TComboBox::mouseEvent(TMouseEvent &me)
{
  switch(me.type) {
    case TMouseEvent::LDOWN:
      btn->setDown(!btn->isDown());
      setFocus();
      break;
    case TMouseEvent::ROLL_UP:
      table->keyDown(TK_UP, const_cast<char*>(""), 0);
      table->keyUp(TK_UP, const_cast<char*>(""), 0);
      invalidateWindow();
      sigSelection();
      break;
    case TMouseEvent::ROLL_DOWN:
      table->keyDown(TK_DOWN, const_cast<char*>(""), 0);
      table->keyUp(TK_DOWN, const_cast<char*>(""), 0);
      invalidateWindow();
      sigSelection();
      break;
  }
}

void
TComboBox::keyDown(TKey key, char *string, unsigned modifier)
{
  // in case the combobox is open and the table popup window
  // is mapped (visible), delegate all keyboard events to the
  // table
  if (table->isMapped()) {
    // but not the SHIFT keys, as table interprets 'em as
    // select.. which I don't think is a good idea in respect
    // to the backward focus traversal using shift+tab keys...
    if (key==TK_SHIFT_L || key==TK_SHIFT_R)
      return;
    if (key==TK_ESCAPE) {
      btn->setDown(false);
      return;
    }
    table->keyDown(key, string, modifier);
    return;
  }

  switch(key) {
    case TK_PAGE_DOWN:
    case TK_SPACE:
      btn->setDown(true);
      break;
    case TK_DOWN:
    case TK_UP:
      table->keyDown(key, string, modifier);
      table->keyDown(TK_RETURN, string, modifier);
      // table->selectAtCursor();
      invalidateWindow();
      break;
  }
}

void
TComboBox::closeRequest()
{
  btn->setDown(false);
}

void
TComboBox::selected()
{
  table->selectAtCursor();
  btn->setDown(false);
  invalidateWindow();
  sigSelection();
}

void
TComboBox::_rendererChanged()
{
//  cerr << "TComboBox '"<<getTitle()<<"': the tables renderer triggered sigChanged" << endl;
  invalidateWindow();
}

void
TComboBox::_selectionChanged()
{
//  cerr << "TComboBox '"<<getTitle()<<"': the tables selection model triggered sigChanged()" << endl;
  invalidateWindow();
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
