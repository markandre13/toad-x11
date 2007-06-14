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

#include <toad/popupmenu.hh>

using namespace toad;

/**
 * \ingroup control
 * \class toad::TPopupMenu
 *
 */

class TPopupMenu::TMenuFilter:
  public TEventFilter
{ 
    TWindow *window;
    TPopupMenu *menu;
  public:
    TMenuFilter(TPopupMenu *menu, TWindow *window) {
      this->menu   = menu;
      this->window = window;
    }
  protected:
    bool mouseEvent(TMouseEvent &evt) {
      if (evt.window == window &&
          evt.type == TMouseEvent::RDOWN)
      {
        menu->open(evt);
        return true;
      }
      return false;
    }
};

TPopupMenu::TPopupMenu(TWindow *parent, const string &title):
  TMenuHelper(parent, title)
{
  flt = 0;
  vertical = true;
  bExplicitCreate = true;
  bPopup = true;
  setLayout(new TMenuLayout()); // i require a layout
}

TPopupMenu::~TPopupMenu()
{
  if (flt) {
    toad::removeEventFilter(flt);
  }
}

/**
 * Open the popup menu.
 */
void
TPopupMenu::open(TMouseEvent &event)
{
  x = event.x;
  y = event.y;
  modifier = event.modifier();
  placeWindow(this, PLACE_CORNER_MOUSE_POINTER, 0);
  createWindow();
  grabPopupMouse();
}

void
TPopupMenu::open(TCoord x, TCoord y, unsigned modifier)
{
  this->x = x;
  this->y = y;
  this->modifier = modifier;
  placeWindow(this, PLACE_CORNER_MOUSE_POINTER, 0);
  createWindow();
  grabPopupMouse();
}

/**
 * Add a event filter, which catches the right mouse button being pressed
 * inside the popup menu's parent window and invokes 'open()'.
 */
void
TPopupMenu::addFilter()
{
  if (flt)
    return;
  flt = new TMenuFilter(this, getParent());
  toad::insertEventFilter(flt, 0, KF_GLOBAL);
}
