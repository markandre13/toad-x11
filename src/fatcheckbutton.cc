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

#include <toad/fatcheckbutton.hh>

using namespace toad;

TFatCheckButton::TFatCheckButton(TWindow *p, const string &t):
  TButtonBase(p, t)
{
  _init();
}

void
TFatCheckButton::_init()
{
  model = 0;
  setModel(new TBoolModel());
}

void
TFatCheckButton::setModel(TBoolModel *m)
{
  if (model)
    disconnect(model->sigChanged, this);
  model = m;
  if (model)
    connect(model->sigChanged, this, &TFatCheckButton::valueChanged);
}

void TFatCheckButton::mouseLDown(int, int, unsigned)
{
  if (!isEnabled()) {
    return;
  }

  if (model) {
    setValue(!*model);
  }

  if (model && *model) {
    setBackground(TColor::SLIDER_FACE);
  } else {
    setBackground(TColor::BTNFACE);
  }
  
  setFocus();
  invalidateWindow();
}

void
TFatCheckButton::valueChanged()
{
  invalidateWindow();
}

void
TFatCheckButton::keyDown(TKey key, char* str, unsigned modifier)
{
  if (!modifier && (key==TK_RETURN || *str==' '))
    mouseLDown(0,0,0);
}

void
TFatCheckButton::paint()
{
  TPen pen(this);
  bool b = model ? *model : false;
  drawShadow(pen, b);
  drawLabel(pen, getLabel(), b);
}
