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
#include <toad/radiobutton.hh>

using namespace toad;

/**
 * @ingroup control
 * \class toad::TRadioButton
 * 
 * See <code>TRadioStateModel</code> for code examples.
 * \sa toad::TRadioState
 */

TRadioButton::TRadioButton(TWindow *parent, 
                           const string &title, 
                           TRadioStateModel *state)
  :super(parent, title, state)
{
  setBackground(TColor::DIALOG);
  setBorder(false);
}
           
void
TRadioButton::paint()
{
  TPen pen(this);
  // draw text
  int y = ( (getHeight()-pen.getHeight()) >> 1 );

  pen.setColor(255,255,255);
  pen.fillCirclePC(1,1,11,11);
  pen.setColor(128,128,128);
  pen.drawCirclePC(0,0,11,11);
  pen.drawCirclePC(1,1,10,10);
  pen.setColor(  0,  0,  0);
  pen.drawArcPC(1,1,11,11, 90, 180);
  if (isDown())
    pen.fillCirclePC(3,3, 6,6);
    
  pen.drawString(20,y, getLabel());
  
  if (isFocus()) {
    pen.setLineStyle(TPen::DOT);
    pen.drawRectanglePC(18,0,getWidth()-18,getHeight());
  }
}
