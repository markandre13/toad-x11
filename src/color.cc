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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstring>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <iostream>

using namespace toad;

/**
 * \class toad::TColor
 * Contains TOADs color management. See 
 * TPen.SetColor(red,green,blue) how to use colors.
 */

TColor sysrgb[TColor::MAX]=
{
  TColor(  0,  0,  0),  // BTNTEXT
  TColor(128,128,128),  // BTNSHADOW
  TColor(192,192,192),  // BTNFACE
  TColor(255,255,255),  // BTNLIGHT
  TColor(192,192,192),  // MENU
  TColor(  0,  0,  0),  // MENUTEXT
  TColor(255,255,255),  // TEXTEDIT
  TColor(128,  0,  0),  // MDIAREA
  TColor(  0,  0,128),  // CAPTION
  TColor(255,255,255),  // CAPTIONTEXT
  TColor(192,192,192),  // INACTIVECAPTION
  TColor(  0,  0,  0),  // INACTIVECAPTIONTEXT
  TColor(192,192,192),  // DIALOG
  TColor(  0,  0,  0),  // DIALOGTEXT
  TColor(127,127,191),  // SLIDER_FACE
  TColor( 63, 63,127),  // SLIDER_SHADOW
  TColor(191,191,255),  // SLIDER_LIGHT
  TColor(127,127,191),  // SELECTED
  TColor(  0,  0,  0),  // SELECTED_TEXT
};

TColor::TColor()
{
  _Init();
  r = g = b = 0;
}

TColor::TColor(byte rn, byte gn, byte bn)
{
  _Init();
  r = rn; g = gn; b = bn;
}

TColor::TColor(TColor::EColor16 c16)
{
  _Init();
  const TColor &c = _palette(c16);
  r = c.r; g = c.g; b = c.b;
}

TColor::TColor(ESystemColor sc)
{
  _Init();
  const TColor &c = sysrgb[sc];
  r = c.r; g = c.g; b = c.b;
}

void TColor::_Init()
{
}

TColor::~TColor()
{
}

void TColor::set(TColor::EColor16 c16)
{
  const TColor &c = _palette(c16);
  r = c.r; g = c.g; b = c.b;
  _data = NULL;
}

void TColor::set(ESystemColor sc)
{
  const TColor &c = sysrgb[sc];
  r = c.r; g = c.g; b = c.b;
  _data = NULL;
}

bool
restore(atv::TInObjectStream &p, toad::TRGB *value)
{
  if (p.what != ATV_GROUP)
    return false;
//  if (!p.type.empty())
//    return false;
  p.setInterpreter(value);
  return true;
}

#if 0
/**
 * explicit type
 */
bool
restore(atv::TInObjectStream &p, const char *name, TRGB **value)
{
  if (p.what != ATV_GROUP)
    return false;
  if (p.attribute != name)
    return false;
//  if (p.type.empty())
//    return false;

  TSerializable *s = p.clone(p.type);
  
  if (!s) {
    return false;
  }

  *value = dynamic_cast<TRGB*>(s);
  if (!*value) {
    p.err << "TRGB isn't of type TRGB";
    delete s;
    return false;
  }
  p.setInterpreter(*value);
  return true;
}
#endif

void
TRGB::store(TOutObjectStream &out) const
{
  ::store(out, r);
  ::store(out, g);
  ::store(out, b);
}

bool 
TRGB::restore(TInObjectStream &in)
{
  if (
    ::restore(in, 0, &r) ||
    ::restore(in, 1, &g) ||
    ::restore(in, 2, &b) ||
    super::restore(in)
    ) return true;
  ATV_FAILED(in)
  return false;
}
