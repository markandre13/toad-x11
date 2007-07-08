/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <toad/toad.hh>

using namespace toad;

class TMyWindow:
  public TWindow
{
  public:
    TMyWindow(TWindow *p, const string &t);
    void mouseLDown(int,int,unsigned);
    
    bool state;
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMyWindow wnd(NULL, "Double Click");
    toad::mainLoop();
  } toad::terminate();
  return 0;
}

TMyWindow::TMyWindow(TWindow *p, const string &t):
  TWindow(p,t)
{
  bStaticFrame = true;
  state = false;
}

void
TMyWindow::mouseLDown(int,int,unsigned modifier)
{
  if (modifier & MK_DOUBLE) {
    state=!state;
    if (state)
      setBackground(TColor::BLACK);
    else
      setBackground(TColor::RED);
    invalidateWindow();
  }
}
