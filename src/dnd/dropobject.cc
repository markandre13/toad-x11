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

#include <toad/dnd/dropobject.hh>

using namespace toad;

/**
 * \ingroup dnd
 * \class toad::TDnDObject
 * TDnDObject encapsulates objects for drag'n drop transfers.
 */

TDnDObject::TDnDObject()
{
}

TDnDObject::~TDnDObject()
{
}

void TDnDObject::flatten()
{
}

/**
 * Select type <VAR>mime</VAR> for the actions <VAR>actions</VAR>.
 */
void TDnDObject::setType(const string &mime, unsigned actions)
{
  TDnDTypeList::iterator p, e;
  p = typelist.begin();
  e = typelist.end();
  while(p!=e) {
    if ((*p)->mime==mime) {
      (*p)->actions = actions;
      return;
    }
    p++;
  }
  typelist.push_back(new TDnDType(mime, actions));
}

/**
 * Called during `dropRequest' this functions selects a type from
 * the typelist in <VAR>drop</VAR> and returns <VAR>true</VAR>.<BR>
 * When the type wasn't available it returns <VAR>false</VAR>.
 */
bool TDnDObject::select(TDnDObject &drop,
                        const string &major,
                        const string &minor)
{
  TDnDTypeList::iterator p, e;
  p = drop.typelist.begin();
  e = drop.typelist.end();
  while(p!=e) {
    if ((*p)->major==major &&
        (*p)->minor==minor)
    {
      (*p)->wanted = true;
      return true;
    }
    p++;
  }
  return false;
}
