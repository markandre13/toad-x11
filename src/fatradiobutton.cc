/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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
#include <toad/buttonbase.hh>
#include <toad/fatradiobutton.hh>

using namespace toad;

/**
 * @class toad::TFatRadioButton
 * @ingroup control
 * TFatRadioButton is a radio button with the look of a push button
 * (or the button of a real radio)<BR>
 * It's often used in toolbars of graphic editors.
 *
 */

TFatRadioButton::TFatRadioButton(TWindow *parent, 
                               const string &title, 
                               TRadioStateModel *state)
  :TRadioButtonBase(parent, title, state)
{
}

// paint
//---------------------------------------------------------------------------
void TFatRadioButton::paint()
{
  TPen pen(this);
  
  bool down = isDown();
  drawLabel(pen, getLabel(), down);
  drawShadow(pen, down);
}
