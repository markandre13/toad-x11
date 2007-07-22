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
#include <X11/Xatom.h>
#endif

#include <cstring>

#define _TOAD_PRIVATE

#include <toad/toad.hh>

#define DBM(A)

namespace toad {

typedef unsigned long ulong;

#ifdef __X11__

static void determine_color_mode();

struct TDirectPixel
{
  ulong red_base, green_base, blue_base;
  ulong red_max, green_max, blue_max;
};

static TDirectPixel direct_pixel;

Colormap x11colormap;

#endif

// InitColor
//---------------------------------------------------------------------------
void TOADBase::initColor()
{
#ifdef __X11__
  determine_color_mode();
#endif
}

// _SetPen
//---------------------------------------------------------------------------
#ifdef __X11__
void
TColor::_setPen(TPen *pen, _TOAD_GC &gc)
{
  if (!gc) {
    gc = XCreateGC(x11display, pen->x11drawable, 0, 0);
    if (pen->region)
      XSetRegion(x11display, gc, pen->region->x11region);
  }
  XSetForeground(x11display, gc, _getPixel(*this));
}

static inline ulong
color2byte(TCoord c)
{
  if (c<=0.0)
    return 0;
  if (c>=1.0)
    return 255;
  return (ulong)(c * 255.0);
}

ulong
TColor::_getPixel(const TRGB &rgb)
{
  return
    ((color2byte(rgb.r)>>direct_pixel.red_max)<<direct_pixel.red_base) |
    ((color2byte(rgb.g)>>direct_pixel.green_max)<<direct_pixel.green_base) |
    ((color2byte(rgb.b)>>direct_pixel.blue_max)<<direct_pixel.blue_base);
}

ulong
TColor::_getPixel(const TRGB24 &rgb)
{
  return
    ((rgb.r>>direct_pixel.red_max)<<direct_pixel.red_base) |
    ((rgb.g>>direct_pixel.green_max)<<direct_pixel.green_base) |
    ((rgb.b>>direct_pixel.blue_max)<<direct_pixel.blue_base);
}

void PrintVisualInfo(const XVisualInfo &xvi)
{
#if 1
  cout << "visual of depth " << xvi.depth
       << " and bits per rgb " << xvi.bits_per_rgb << endl;

  switch(xvi.c_class) {
    case StaticGray:
      cout << "StaticGray" << endl;
      break;
    case GrayScale:
      cout << "GrayScale" << endl;
      break;
    case StaticColor:
      cout << "StaticColor" << endl;
      break;
    case PseudoColor:
      cout << "PseudoColor" << endl;
      break;
    case TrueColor:
      cout << "TrueColor" << endl;
      break;
    case DirectColor:
      cout << "DirectColor" << endl;
      break;
  }

  switch(xvi.c_class) {
    case TrueColor:
    case DirectColor:
      cout << "R: " << direct_pixel.red_base << "," << direct_pixel.red_max << endl;
      cout << "G: " << direct_pixel.green_base << "," << direct_pixel.green_max << endl;
      cout << "B: " << direct_pixel.blue_base << "," << direct_pixel.blue_max << endl;
      break;
  }
#endif
}

inline void pixel_helper(ulong mask, ulong &base, ulong &max)
{       
  base=0; max=0;
  while((mask&1)==0) {
    base++;
    mask>>=1;
  }
  while((mask&1)!=0) {
    max++;
    mask>>=1; 
  }
  max = 8-max;
}

/**
 * Set 'color_mode' from the visual class.
 *
 * \li StaticGray, GrayScale: Isn't supported yet.
 * \li PseudoColor, DirectColor: Indexed.
 * \li StaticColor, TrueColor: RGB
 */
void 
determine_color_mode()
{
  Visual *visual = XDefaultVisual(x11display, DefaultScreen(x11display));

  x11colormap = DefaultColormap(x11display,
                                DefaultScreen(x11display));
  x11visual = visual;
  x11depth  = DefaultDepth(x11display,
                           DefaultScreen(x11display));

  switch(visual->c_class) {
    case StaticGray:
    case GrayScale:
      cerr << "toad: gray color visuals aren't supported" << endl;
      exit(1);
    case PseudoColor:
    case DirectColor:
      cerr << "toad: pseudo color aren't supported" << endl;
      exit(1);
    case StaticColor:
    case TrueColor:
      pixel_helper(visual->red_mask, direct_pixel.red_base, direct_pixel.red_max);
      pixel_helper(visual->green_mask, direct_pixel.green_base, direct_pixel.green_max);
      pixel_helper(visual->blue_mask, direct_pixel.blue_base, direct_pixel.blue_max);
      break;
  }

  // The initial values of the colormap entries are undefined for the visual
  // classes GrayScale, PseudoColor, and DirectColor!

//  PrintVisualInfo(visual);
}
#endif

} // namespace toad

