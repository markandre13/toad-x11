/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-Andr� Hopf <mhopf@mark13.de>
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

namespace toad {

TFatCheckButton::TFatCheckButton(TWindow *p, const string &t):
  TButtonBase(p, t)
{
  _init();
}

void TFatCheckButton::_init()
{
  _data = false;
}

void TFatCheckButton::mouseLDown(int, int, unsigned)
{
  if (!isEnabled())
    return;
  
  _data = !_data;
  
  if (_data)
    setBackground(TColor::SLIDER_FACE);
  else
    setBackground(TColor::BTNFACE);
  
  setFocus();
  invalidateWindow();
  sigValueChanged();
}

void TFatCheckButton::keyDown(TKey key, char* str, unsigned modifier)
{
  if (!modifier && (key==TK_RETURN || *str==' '))
    mouseLDown(0,0,0);
}

void TFatCheckButton::paint()
{
  TPen pen(this);
  drawShadow(pen, _data);
  drawLabel(pen, getLabel(), _data);
}

} // namespace toad
