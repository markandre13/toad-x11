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

#define DBM_FEEL(A)

#include <toad/menubar.hh>
#include <toad/toad.hh>
#include <toad/form.hh>
#include <toad/mdiwindow.hh>
#include <toad/pushbutton.hh>
#include <toad/textfield.hh>
#include <toad/action.hh>

#include <iostream>
#include <fstream>

using namespace toad;

/**
 * \ingroup control
 * \class toad::TMenuBar
 *
 */

TMenuBar::TMenuBar(TWindow *p, const string& t):
  super(p, t)
{
  vertical = false; // i'm a horizontal menuhelper
  setLayout(new TMenuLayout()); // i require a layout
}

TMenuBar::~TMenuBar()
{
}
