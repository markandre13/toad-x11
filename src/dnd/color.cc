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

#include <toad/dnd/color.hh>

using namespace toad;

// application/x-toad-color
//---------------------------------------------------------------------------
TDnDColor::TDnDColor(const TRGB &color)
{
  rgb = color;
  setType("application/x-toad-color", ACTION_COPY);
}

TDnDColor::~TDnDColor()
{
}

bool TDnDColor::select(TDnDObject &drop)
{
  return TDnDObject::select(drop, "application", "x-toad-color");
}

// Store
void TDnDColor::flatten()
{
  flatdata.erase();
  flatdata+=(char)rgb.r;
  flatdata+=(char)rgb.g;
  flatdata+=(char)rgb.b;
}

// Restore
PDnDColor TDnDColor::convertData(TDnDObject &drop)
{
  TDnDColor *result;
  result = dynamic_cast<TDnDColor*>(&drop);
  if (result || !drop.type)
    return result;

  if (drop.type->major!="application" ||
      drop.type->minor!="x-toad-color" ||
      drop.flatdata.size()<3) {
    return result;
  }

  TRGB rgb;
  rgb.r = drop.flatdata[0];
  rgb.g = drop.flatdata[1];
  rgb.b = drop.flatdata[2];
  result = new TDnDColor(rgb);
  result->x = drop.x;
  result->y = drop.y;
  return result;
}


void TDropSiteColor::dropRequest(TDnDObject &drop)
{
  if (TDnDColor::select(drop)) {
    drop.action = ACTION_COPY;
    return;
  }
  drop.action = ACTION_NONE;
}

void TDropSiteColor::drop(TDnDObject &drop)
{
  value = TDnDColor::convertData(drop);
  if (value) {
    sigDrop();
    value = NULL;
  }
}
