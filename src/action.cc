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

#include <toad/toad.hh>
#include <toad/action.hh>
#include <toad/menubar.hh>

using namespace toad;

TActionStorage TAction::actions;

TAbstractChoice::~TAbstractChoice()
{
}

/**
 * \class toad::TAction
 *
 * \li 
 *   Locates a TMenuBar in the tree a class TMenuBar::Add with itself
 *   as an argument.
 * \todo
 *   \li
 *     Currently the actions look is controlled by the menubar which is
 *     wrong.
 *   \li
 *     The action shouldn't attach itself to the menubar but to some kind
 *     of storage. The menubar shall query this storage when needed.
 */

TAction::TAction(TWindow *parent, const string &id)
  :TInteractor(parent)
{
  title = id;
  enabled = true;
  bitmap = 0;
  type = BUTTON;
  actions.push_back(this);
}

TAction::~TAction()
{
  actions.erase(this);
}

void TAction::domainFocus(bool b)
{
//  cout << ( b ? "received" : "lost") << " domain focus for " << getTitle() << endl;
  if (enabled!=b) {
    enabled = b;
    sigChanged();
  }
}

bool 
TAction::isEnabled() const
{
  return sigActivate.isConnected() && enabled;
}


unsigned
TAction::getSize() const
{
  return 1;
}

/**
 * Returns the ID associated with this action.
 *
 * \param idx A value between 0 and Size().
 */
const string& 
TAction::getID(unsigned idx) const
{
  return title;
}
