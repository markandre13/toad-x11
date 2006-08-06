/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#include <cmath>
#include <set>
#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#define _TOAD_PRIVATE

#include <toad/os.hh>
#include <toad/config.h>
#include <toad/toadbase.hh>
#include <toad/font.hh>
#include <toad/pen.hh>
#include <toad/utf8.hh>

#ifdef HAVE_LIBXFT

typedef struct _XftFont XftFont;

#ifdef _XFT_NO_COMPAT_
#undef _XFT_NO_COMPAT_
#endif

#include <X11/Xft/Xft.h>
#endif

#ifndef HAVE_LIBXFT
#define XftDraw void
#endif

#ifdef HAVE_LIBXUTF8
#include "xutf8/Xutf8.h"
#endif


using namespace toad;

static TFontManager* fontmanager = NULL;

TFontManager::~TFontManager()
{
}

bool 
TFontManager::setDefaultByName(const string &engine)
{
  if (engine=="x11") {
    fontmanager = new TFontManagerX11;
  } else
  if (engine=="freetype") {
    fontmanager = new TFontManagerFT;
  } else {
    return false;
  }
  return true;
}

TFontManager* TFontManager::getDefault()
{
  return fontmanager;
}

TFont TFont::default_font("arial,helvetica,sans-serif:size=12");

TFont::~TFont()
{
  if (font)
    FcPatternDestroy(font);
  if (corefont) {
    assert(fontmanager!=NULL);
    fontmanager->freeCoreFont(this);
  }
}

void
TFont::setFont(const string &fontname)
{
  if (font)
    FcPatternDestroy(font);
  font = FcNameParse((FcChar8*)fontname.c_str());
}

const char*
TFont::getFont() const
{
  return (char*)FcNameUnparse(font);
}

void
TFont::setFamily(const string &family)
{
  FcPatternAddString(font, FC_FAMILY, (FcChar8*)family.c_str());
}

const char*
TFont::getFamily() const
{
  char *s;
  FcPatternGetString(font, FC_FAMILY, 0, (FcChar8**)&s);
  return s;
}

void
TFont::setSize(double size)
{
  FcPatternAddDouble(font, FC_SIZE, size);
}

double
TFont::getSize() const
{
  double d = 0.0;
  FcPatternGetDouble(font, FC_SIZE, 0, &d);
  return d;
}

void
TFont::setWeight(int weight)
{
  FcPatternAddInteger(font, FC_WEIGHT, weight);
}

int
TFont::getWeight() const
{
}

void
TFont::setSlant(int slant)
{
   FcPatternAddInteger(font, FC_SLANT, slant);
}

int
TFont::getSlant() const
{
  int i;
  FcPatternGetInteger(font, FC_SLANT, 0, &i);
  return i;
}

