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

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/checkbox.hh>

using namespace toad;

/**
 * \ingroup control
 * \class toad::TCheckBox
 * A checkbox is a small control that can be toggled on and off.
 * \note
 *   Don't confuse this with Motifs XmCheckBox.
 */

#define FENSTER95

TCheckBox::TCheckBox(TWindow *parent, const string &title, TBoolModel *m)
  :super(parent,title)
{
  _init(m);
}

TCheckBox::~TCheckBox()
{
}

void
TCheckBox::_init(TBoolModel *m)
{
  setBackground(TColor::DIALOG);
  setBorder(false);
  if (!m)
    m = new TBoolModel();
  setModel(m);
}

void
TCheckBox::setModel(TBoolModel *m)
{
  if (model)
    disconnect(model->sigChanged, this);
  model = m;
  connect(model->sigChanged, this, &TCheckBox::valueChanged);
}

void
TCheckBox::valueChanged()
{
  invalidateWindow();
}

void 
TCheckBox::paint()
{
  TPen pen(this);
  int height;

  pen.setColor(255,255,255);
  if (isEnabled()) {
    pen.fillRectanglePC(2,2,9,9);
  }
  pen.draw3DRectanglePC(0,0,13,13);

  if (isEnabled()) {
    pen.setColor(0,0,0);
    height = pen.drawTextWidth(20,0, getLabel(),_w-20);
  } else {
    pen.setColor(TColor::BTNLIGHT);
    pen.drawTextWidth(21,1, getLabel(),_w-20);
    pen.setColor(TColor::BTNSHADOW);
    height = pen.drawTextWidth(20,0, getLabel(),_w-20);
  }

  if (model->getValue()) {
    pen.drawLine(3,3,9,9);
    pen.drawLine(3,4,8,9);
    pen.drawLine(4,3,9,8);
    pen.drawLine(9,3,3,9);
    pen.drawLine(8,3,3,8);
    pen.drawLine(9,4,4,9);
  }
    
  if (isFocus()) {
    pen.setLineStyle(TPen::DOT);
    pen.drawRectanglePC(18,0,_w-18,height);
  } else {
    pen.setColor(TColor::DIALOG);
    pen.drawRectanglePC(18,0,_w-18,height);
  }
}

void 
TCheckBox::mouseLDown(int,int,unsigned)
{
  if (!isEnabled())
    return;
  model->toggleValue();
  setFocus();
}

void
TCheckBox::keyDown(TKey key, char* str, unsigned modifier)
{
  if (!modifier && (key==TK_RETURN || *str==' ')) {
    model->toggleValue();
    setFocus();
  }
}
