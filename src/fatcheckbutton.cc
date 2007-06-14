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

#include <toad/fatcheckbutton.hh>

using namespace toad;

TFatCheckButton::TFatCheckButton(TWindow *p, const string &t, TBoolModel *m):
  TButtonBase(p, t)
{
  _init(m);
}

void
TFatCheckButton::_init(TBoolModel *m)
{
  bNoBackground = true;
  model = 0;
  if (!m)
    m = new TBoolModel();
  setModel(m);
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

void TFatCheckButton::mouseLDown(const TMouseEvent&)
{
  if (!isEnabled()) {
    return;
  }

  if (model) {
    setValue(!*model);
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
TFatCheckButton::keyDown(const TKeyEvent &ke)
{
  if (!ke.modifier() && (ke.key()==TK_RETURN || ke.str()==" ")) {
    if (!isEnabled()) {
      return;
    }

    if (model) {
      setValue(!*model);
    }
    setFocus();
    invalidateWindow();
  }
}

void
TFatCheckButton::paint()
{
  TPen pen(this);
  bool b = model ? *model : false;
  if (b) {
    pen.setColor(TColor::SLIDER_FACE);
  } else {
    pen.setColor(TColor::BTNFACE);
  }
  pen.fillRectangle(0, 0, getWidth(), getHeight());
  drawShadow(pen, b);
  drawLabel(pen, getLabel(), b);
}
