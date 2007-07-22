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


#include <toad/os.hh>

#ifdef __X11__

#include <toad/config.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define _TOAD_PRIVATE

#include <toad/font.hh>
#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/matrix2d.hh>
#include <toad/region.hh>
#include <toad/utf8.hh>

#include <iostream>
#include <map>
#include <set>
#include <cmath>

typedef struct _XftFont XftFont;

#ifdef _XFT_NO_COMPAT_
#undef _XFT_NO_COMPAT_
#endif

#include <X11/Xft/Xft.h>

#define XftDraw void



using namespace toad;

FcConfig*
TFontManagerFT::getFcConfig()
{
  cerr << __FILE__ << ":" << __LINE__ << " not implemented" << endl;
  return 0;
}

FcFontSet*
TFontManagerFT::getFcFontSet()
{
  cerr << __FILE__ << ":" << __LINE__ << " not implemented" << endl;
  return 0;
}

static string d2s(double d);

void
TFontManagerFT::init() const
{
}

bool
TFontManagerFT::buildFontList(FcConfig *config)
{
}

static bool dummy;

static int
dummyhandler(Display *, XErrorEvent *)
{
  dummy = true;
}

struct TFTFont {
  XftFont *xftfont, *xftfont_r;
  double x11scale;
  string id;

  TFTFont() {
    x11scale = 1.0;
    xftfont = 0;
    xftfont_r = 0;
  }
  ~TFTFont() {
    clear();
  }
  void clear();
};

void
TFTFont::clear()
{
  if (xftfont) {  
    XftFontClose(toad::x11display, xftfont);
    xftfont = 0;
  }
  if (xftfont_r) {
    XftFontClose(toad::x11display, xftfont_r);
    xftfont_r = 0;
  }
}

void
TFontManagerFT::freeCoreFont(TFont *font)
{
  assert(font->fontmanager==this);
  if (font->corefont) {
    delete static_cast<TFTFont*>(font->corefont);
    font->corefont = 0;
  }
}

bool
TFontManagerFT::allocate(TFont *font, const TMatrix2D *mat)
{
#if 0
  if (font->corefont)
    return true;
#endif
//cout << __PRETTY_FUNCTION__ << endl;
  if (!font->font) {
    cout << "error: font has no pattern" << endl;
    return false;
  }
  assert(font->fontmanager==this);
  init();
  TFTFont *ft = 0;
  if (!font->corefont) {
    ft = new TFTFont;
    font->corefont = ft;
  } else {
    ft = static_cast<TFTFont*>(font->corefont);
  }

  XftPattern *pattern;
  string newid;
  if (mat && !mat->isIdentity()) {
    double d=12.0;
    pattern = FcPatternDuplicate(font->font);
    XftPatternGetDouble(pattern, FC_SIZE, 0, &d);
    newid = "[";
    newid += d2s(mat->a11 * d);
    newid += d2s(mat->a12 * d);
    newid += d2s(mat->a21 * d);
    newid += d2s(mat->a22 * d);
    newid += "]";
    if (newid != ft->id) {
      if (ft->xftfont) {
        XftFontClose(toad::x11display, ft->xftfont);
        ft->xftfont = 0;
      }
      ft->id = newid;
    } else {
      XftPatternDestroy(pattern);
      return true;
    }
  } else {
    if (ft->xftfont) {
      return true;
    }
    pattern = FcPatternDuplicate(font->font);
  }

  // fontconfig-devel.txt sais that the default dpi is 75 but my
  // version took about 100 dpi, so force it to 75 dpi here:
  double dpi;
  if (XftPatternGetDouble(pattern, XFT_DPI, 0, &dpi)==XftResultNoMatch) {
    XftPatternAddDouble(pattern, XFT_DPI, 75.0);
  }

  XftResult result;
  XftPattern *found;

  XftFont *new_font = 0;
  XftFont *new_font_r = 0;

  if (mat && !mat->isIdentity()) {

    // set x11scale to the fonts scaling factor which we need as
    // XftTextExtentsUtf8 will deliver the font actually used but
    // we need to deliver the unscaled dimensions
    double x1, y1, x2, y2;
    TMatrix2D m(*mat);
    m.invert();
    m.map(0.0, 0.0, &x1, &y1);
    m.map(0.0, 1.0, &x2, &y2);
    x1-=x2;
    y1-=y2;
    ft->x11scale = sqrt(x1*x1+y1*y1);

    // ascent and descent are wrong for rotated fonts, so we retrieve
    // these values here for an unrotated font.
    found = XftFontMatch(x11display, x11screen, pattern, &result);
    new_font = XftFontOpenPattern(x11display, found);

    XftMatrix xftmat;
    XftMatrixInit(&xftmat);
    xftmat.xx = mat->a11;
    xftmat.yx = mat->a12;
    xftmat.xy = mat->a21;
    xftmat.yy = mat->a22;
    XftPatternAddMatrix(pattern, XFT_MATRIX, &xftmat);

    found = XftFontMatch(x11display, x11screen, pattern, &result);
    new_font_r = XftFontOpenPattern(x11display, found);
  } else {
    ft->x11scale = 1.0;
    found = XftFontMatch(x11display, x11screen, pattern, &result);
    new_font = XftFontOpenPattern(x11display, found);
  }
   
  if (new_font) {
    ft->clear();
    ft->xftfont = new_font;
    ft->xftfont_r = new_font_r;
  }
   
  XftPatternDestroy(pattern);


  return true;
}

void
TFontManagerFT::drawString(TPenBase *penbase, int x, int y, const char *str, size_t strlen, bool transparent)
{
//cout << __PRETTY_FUNCTION__ << endl;
  TPen *pen = dynamic_cast<TPen*>(penbase);
  assert(pen);
  assert(pen->font);

//  if (!pen->font->corefont)
//    return;
  if (!allocate(pen->font, pen->mat))
    return;
  TFTFont *ft = static_cast<TFTFont*>(pen->font->corefont);

  if (!transparent /* && pen->using_bitmap*/) {
    bool b = pen->two_colors;
    GC gc;
    if (b) {
      pen->two_colors = false;
      gc = pen->o_gc; pen->o_gc = pen->f_gc;
    }
    pen->fillRectanglePC(x,y,pen->getTextWidth(str,strlen),pen->getHeight());
    if (b) {
      pen->two_colors = true;
      pen->o_gc = gc;
    }
  }

  y+=ft->xftfont->ascent;

  XftColor color;
  color.color.red   = pen->o_color.r * 0xFFFF;
  color.color.green = pen->o_color.g * 0xFFFF;
  color.color.blue  = pen->o_color.b * 0xFFFF;
  color.color.alpha = 0xffff;

  if (!pen->xftdraw) {
    pen->xftdraw = XftDrawCreate(toad::x11display, 
                                 pen->x11drawable, 
                                 toad::x11visual, 
                                 toad::x11colormap);
  }   
  if (pen->region)
    XftDrawSetClip(pen->xftdraw, pen->region->x11region);
  else if (pen->wnd && pen->wnd->getUpdateRegion())
    XftDrawSetClip(pen->xftdraw, pen->wnd->getUpdateRegion()->x11region);

 if (!pen->mat || pen->mat->isIdentity()) {
    if (pen->mat)
      pen->mat->map(x, y, &x, &y);
    XftDrawStringUtf8(pen->xftdraw, &color, ft->xftfont, x,y, (XftChar8*)str, strlen);
  } else {
    // Draw rotated and downscaled font char by char, using the horizontal
    // unscaled font as a reference. This is much slower than a single
    // X*DrawString call but the precision is required, even for horizontal
    // and just scaled fonts.
    int x2, y2;
    const char *p = str;
    int len=0;
    while(*p && len<strlen) {
      char buffer[6];
      unsigned clen=1;
      buffer[0]=*p;   
      ++p;
      ++len;
  
      while( ((unsigned char)*p & 0xC0) == 0x80) {
        buffer[clen]=*p;
        ++len;
        ++p;  
        ++clen;
      }
      buffer[clen]=0;
  
      int direction, fasc, fdesc;
  
      pen->mat->map(x, y, &x2, &y2);
      XftDrawStringUtf8(pen->xftdraw, &color, ft->xftfont_r, x2,y2, (XftChar8*)buffer, clen);
      x+=pen->font->getTextWidth(buffer, clen);
    }
  }
}

int 
TFontManagerFT::getHeight(TFont *font)
{
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TFTFont *ft = static_cast<TFTFont*>(font->corefont);
  if (ft->xftfont) {
    return ft->xftfont->ascent + ft->xftfont->descent;
  }
  return 0;
}

int 
TFontManagerFT::getAscent(TFont *font)
{
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TFTFont *ft = static_cast<TFTFont*>(font->corefont);
  if (ft->xftfont) {
    return ft->xftfont->ascent;
  }
  return 0;
}

int 
TFontManagerFT::getDescent(TFont *font)
{
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TFTFont *ft = static_cast<TFTFont*>(font->corefont);
  if (ft->xftfont) {
    return ft->xftfont->descent;
  }
  return 0;
}

int
TFontManagerFT::getTextWidth(TFont *font, const char *str, size_t len)
{
  if (len==0)
    return 0;
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TFTFont *ft = static_cast<TFTFont*>(font->corefont);
  if (ft->xftfont) {
    XGlyphInfo gi;
    if (ft->xftfont->ascent + ft->xftfont->descent<=64) {
      XftTextExtentsUtf8(toad::x11display, ft->xftfont, (XftChar8*)str, len, &gi);
      return gi.xOff;
    }
    int w = 0;
    for(int i=0; i<len;) {
      XGlyphInfo gi;
      int n = utf8bytecount(str, i, 1);
      XftTextExtentsUtf8(toad::x11display, ft->xftfont, (XftChar8*)str+i, n, &gi);
      w+=gi.xOff;
      i+=n;
    }
    return w;
  }
  return 0;
}

string
d2s(double d)
{
  string s;
  
  if (d<0.0) {
    s+='~';
    d=-d;
  } else {
    s+='+';
  }
  
  double a = 1.0;
  while(a<=d) a*=10.0;

  bool b;

  a/=10.0;

  if (a>=1.0) {
    while(a>=1.0) {
      int digit=0; 
      while(d>=a) {
        digit++;
        d-=a;
      }
      a/=10.0;
      s+='0'+digit;
    }
  } else {
    s+='0';
  }
  
  if (d>=0.0001) {
    s+='.';
    while(d>=0.0001) {
      int digit=0; 
      while(d>=a) {
        digit++;
        d-=a;
      }
      a/=10.0;
      s+='0'+digit;
    }
  }

  return s;
}

#endif
