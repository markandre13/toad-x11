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

#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

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
  TColor(150,150,150),  // BTNSHADOW
  TColor(215,215,215),  // BTNFACE
  TColor(255,255,255),  // BTNLIGHT
  TColor(215,215,215),  // MENU
  TColor(  0,  0,  0),  // MENUTEXT
  TColor(255,255,255),  // TEXTEDIT
  TColor(128,  0,  0),  // MDIAREA
  TColor(  0,  0,128),  // CAPTION
  TColor(255,255,255),  // CAPTIONTEXT
  TColor(215,215,215),  // INACTIVECAPTION
  TColor(  0,  0,  0),  // INACTIVECAPTIONTEXT
  TColor(215,215,215),  // DIALOG
  TColor(  0,  0,  0),  // DIALOGTEXT
  TColor(127,127,191),  // SLIDER_FACE
  TColor( 63, 63,127),  // SLIDER_SHADOW
  TColor(191,191,255),  // SLIDER_LIGHT
  TColor(137,137,215),  // SELECTED
  TColor(127,127,191),  // SELECTED_2
  TColor(  0,  0,  0),  // SELECTED_TEXT
  TColor(222,222,222),  // SELECTED_GRAY
  TColor(215,215,215),  // SELECTED_GRAY_2
  TColor(255,255,255),  // TABLE_CELL
  TColor(231,231,255),  // TABLE_CELL_2
  TColor( 79,128,255),  // FIGURE_SELECTION
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
#ifdef __X11__
  _data = NULL;
#endif
#ifdef __WIN32__
  colorref = RGB(r, g, b);
#endif
}

void TColor::set(ESystemColor sc)
{
  const TColor &c = sysrgb[sc];
  r = c.r; g = c.g; b = c.b;
#ifdef __X11__
  _data = NULL;
#endif
#ifdef __WIN32__
  colorref = RGB(r, g, b);
#endif
}

bool
restore(atv::TInObjectStream &p, toad::TSerializableRGB *value)
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
TSerializableRGB::store(TOutObjectStream &out) const
{
  ::store(out, r);
  ::store(out, g);
  ::store(out, b);
}

bool 
TSerializableRGB::restore(TInObjectStream &in)
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
