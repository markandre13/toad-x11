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

#ifndef _TOAD_ARROWBUTTON_HH
#define _TOAD_ARROWBUTTON_HH

#include <toad/pushbutton.hh>
#include <toad/simpletimer.hh>

namespace toad {

class TArrowButton: 
  public TPushButton, TSimpleTimer
{
  public:
    enum EArrowType {
      ARROW_DOWN=1, ARROW_UP, ARROW_RIGHT, ARROW_LEFT
    };
    TArrowButton(TWindow *parent, const string &title, EArrowType d)
      :TPushButton(parent, title)
    { direction=d;bNoFocus=true; }
    void setType(EArrowType type) { direction = type; }
      
  protected:
    void mouseLDown(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void tick();
    void paint();

  private:
    int delay;
    EArrowType direction;
};

} // namespace toad

#endif
