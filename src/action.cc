/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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
 * An action adds itself to TAction::actions where it can be found by
 * menubars, popup menus and other widgets which can present the
 * various actions to the user.
 *
 * \todo
 *   \li
 *     Currently the actions look is controlled by the menubar which is
 *     wrong.
 */

TAction::TAction(TInteractor *parent, const string &id)
  :TInteractor(parent)
{
  title = id;
  focus = true;
  enabled = true;
  bitmap = 0;
  type = BUTTON;
  actions.push_back(this);
}

TAction::~TAction()
{
  actions.erase(this);
}

void 
TAction::domainFocus(bool b)
{
//  cout << ( b ? "received" : "lost") << " domain focus for " << getTitle() << endl;
  bool oldstate = focus && enabled;
  focus = b;
  if ( (focus && enabled) != oldstate ) {
    sigChanged();
  }
}

void 
TAction::setEnabled(bool b)
{
//  cout << ( b ? "received" : "lost") << " domain focus for " << getTitle() << endl;
  bool oldstate = focus && enabled;
  enabled = b;
  if ( (focus && enabled) != oldstate ) {
    sigChanged();
  }
}

bool 
TAction::isEnabled() const
{
  return sigActivate.isConnected() && focus && enabled;
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
