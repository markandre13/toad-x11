/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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

static TRGB sysrgb[TColor::MAX]=
{
                                  // IBM PC CGA     Amstrad CPC   HTML
  TRGB(  0/255.0,  0/255.0,  0/255.0),  //  0 black       0             Black  
  TRGB(128/255.0,  0/255.0,  0/255.0),  //  1 red         3             Maroon
  TRGB(  0/255.0,128/255.0,  0/255.0),  //  2 green       9             Green 
  TRGB(128/255.0,128/255.0,  0/255.0),  //  3 yellow      12            Olive 
  TRGB(  0/255.0,  0/255.0,128/255.0),  //  4 blue        1             Navy  
  TRGB(128/255.0,  0/255.0,128/255.0),  //  5 violett     4             Purple
  TRGB(  0/255.0,128/255.0,128/255.0),  //  6 cyan        10            Teal  
  TRGB(128/255.0,128/255.0,128/255.0),  //  7 gray        13            Gray  
  TRGB(192/255.0,192/255.0,192/255.0),  //  8 lightgray                 Silver
  TRGB(255/255.0,  0/255.0,  0/255.0),  //  9 lightred    6             Red   
  TRGB(  0/255.0,255/255.0,  0/255.0),  // 10 lightgreen  18            Lime  
  TRGB(255/255.0,255/255.0,  0/255.0),  // 11 yellow      24            Yellow
  TRGB(  0/255.0,  0/255.0,255/255.0),  // 12 lightblue   2             Blue   
  TRGB(255/255.0,  0/255.0,255/255.0),  // 13 magenta     8             Fuchsia
  TRGB(  0/255.0,255/255.0,255/255.0),  // 14 lightcyan   20            Aqua   
  TRGB(255/255.0,255/255.0,255/255.0),  // 15 white       26            White  

  // SystemColors
  TRGB(  0/255.0,  0/255.0,  0/255.0),  // BTNTEXT
  TRGB(150/255.0,150/255.0,150/255.0),  // BTNSHADOW
  TRGB(215/255.0,215/255.0,215/255.0),  // BTNFACE
  TRGB(255/255.0,255/255.0,255/255.0),  // BTNLIGHT
  TRGB(215/255.0,215/255.0,215/255.0),  // MENU
  TRGB(  0/255.0,  0/255.0,  0/255.0),  // MENUTEXT
  TRGB(255/255.0,255/255.0,255/255.0),  // TEXTEDIT
  TRGB(128/255.0,  0/255.0,  0/255.0),  // MDIAREA
  TRGB(  0/255.0,  0/255.0,128/255.0),  // CAPTION
  TRGB(255/255.0,255/255.0,255/255.0),  // CAPTIONTEXT
  TRGB(215/255.0,215/255.0,215/255.0),  // INACTIVECAPTION
  TRGB(  0/255.0,  0/255.0,  0/255.0),  // INACTIVECAPTIONTEXT
  TRGB(215/255.0,215/255.0,215/255.0),  // DIALOG
  TRGB(  0/255.0,  0/255.0,  0/255.0),  // DIALOGTEXT
  TRGB(127/255.0,127/255.0,191/255.0),  // SLIDER_FACE
  TRGB( 63/255.0, 63/255.0,127/255.0),  // SLIDER_SHADOW
  TRGB(191/255.0,191/255.0,255/255.0),  // SLIDER_LIGHT
  TRGB(137/255.0,137/255.0,215/255.0),  // SELECTED
  TRGB(127/255.0,127/255.0,191/255.0),  // SELECTED_2
  TRGB(  0/255.0,  0/255.0,  0/255.0),  // SELECTED_TEXT
  TRGB(222/255.0,222/255.0,222/255.0),  // SELECTED_GRAY
  TRGB(215/255.0,215/255.0,215/255.0),  // SELECTED_GRAY_2
  TRGB(255/255.0,255/255.0,255/255.0),  // TABLE_CELL
  TRGB(231/255.0,231/255.0,255/255.0),  // TABLE_CELL_2
  TRGB( 79/255.0,128/255.0,255/255.0)   // FIGURE_SELECTION
};

TColor::TColor()
{
  _Init();
  r = g = b = 0;
}

TColor::TColor(TCoord rn, TCoord gn, TCoord bn)
{
  _Init();
  r = rn; g = gn; b = bn;
}

TColor::TColor(EColor ec)
{
  _Init();
  const TRGB &c = sysrgb[ec];
  r = c.r; g = c.g; b = c.b;
}

void TColor::_Init()
{
}

TColor::~TColor()
{
}

const TRGB*
TColor::lookup(EColor n)
{
  if (n>=MAX)
    n = BLACK;
  return &sysrgb[n];
}
        
void TColor::set(EColor ec)
{
  const TRGB &c = sysrgb[ec];
  r = c.r; g = c.g; b = c.b;
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
  byte br(r*255.0), bg(g*255.0), bb(b*255.0);
  ::store(out, br);
  ::store(out, bg);
  ::store(out, bb);
}

bool 
TSerializableRGB::restore(TInObjectStream &in)
{
  if (::restore(in, 0, &r)) {
    r /= 255.0;
    return true;
  }
  if (::restore(in, 1, &g)) {
    g /= 255.0;
    return true;
  }
  if (::restore(in, 2, &b)) {
    b /= 255.0;
    return true;
  }
  if (super::restore(in))
    return true;
  ATV_FAILED(in)
  return false;
}
