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
#include <toad/rowcolumn.hh>

namespace toad {

TRowColumn::TRowColumn(TWindow *parent, const string &title):
  TWindow(parent, title)
{
  nOrientation = TS_HORIZONTAL;
}

void TRowColumn::adjust()
{
  resize();
  TInteractor *ptr = getFirstChild();
  if (nOrientation==TS_VERTICAL && ptr) { 
    do {
      ptr->setSize(getWidth(), TSIZE_PREVIOUS);
      ptr=getNextSibling(ptr);
    }while(ptr);
  }
}

void TRowColumn::resize()
{
  int x,y,max,total;
  x=y=max=total=0;
  TRectangle shape;
  
  TInteractor* ptr = getFirstChild();
  if(!ptr)
    return;

  if (nOrientation == TS_HORIZONTAL)
  { //+---------------+
    //| TS_HORIZONTAL |
    //+---------------+
    do {
      ptr->getShape(&shape);
      if ( x!=0 && x+shape.w>getWidth() )   // start a new line for next child if neccesary
      {
        total+=max;
        y+=max;
        x=max=0;
      }
      ptr->setPosition(x,y);                // place child
      x+=shape.w;                           // one step right
      if (shape.h > max)                    // determine the maximal height of the current line
        max=shape.h;
      ptr=getNextSibling(ptr);
    }while(ptr);
    setSize(getWidth(), total+max);
  }
  else
  { //+-------------+
    //| TS_VERTICAL |
    //+-------------+
    do
    {
      ptr->setPosition(x,y);  // place child
      ptr->getShape(&shape);
      y += shape.h;
      if (shape.w > max)
        max = shape.w;
      ptr=getNextSibling(ptr);
    }while(ptr);
    setSize(total+max, y);
  }
}

} // namespace toad
