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

#ifndef _TOAD_POPUPMENU_HH
#define _TOAD_POPUPMENU_HH

#include <toad/menuhelper.hh>

namespace toad {

class TPopupMenu:
  public TMenuHelper   
{
    class TMenuFilter;
    TMenuFilter *flt;
  public:
    TPopupMenu(TWindow *parent, const string &title);
    ~TPopupMenu();
    void open(TMouseEvent &event);
    void open(int x, int y, unsigned modifier);
    void addFilter();

    //! the x,y and modifier values given to the last 'open' method call
    int x, y;
    unsigned modifier;
};

} // namespace toad

#endif
