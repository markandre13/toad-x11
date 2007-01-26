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
#include <toad/debug.hh>
#include <toad/config.h>

#ifdef __X11__

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef HAVE_LIBXUTF8
#include "xutf8/Xutf8.h"
#endif

#endif

#define _TOAD_PRIVATE

#include <toad/font.hh>
#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/matrix2d.hh>

#include <iostream>
#include <map>
#include <set>

using namespace toad;

static FcConfig *fc_x11fonts = 0;
static FcFontSet *fc_x11fontset = 0;

FcConfig*
TFontManagerX11::getFcConfig()
{
  return fc_x11fonts;
}

FcFontSet*
TFontManagerX11::getFcFontSet()
{
  return fc_x11fontset;
}

// X Logical Font Description
struct TX11FontName {
  
  string vendor, family, weight, slant, set_width, adj_style,
         pixels, points, hdpi, vdpi, spacing, avg_width,
         registry, encoding;
  
  void setWildcards();
  void setXLFD(const char *name);
  string getXLFD() const;

  bool isScaleable() const;
};

static string d2s(double d);


static FcBool
FcPatternAddCharset(FcPattern *p, const char *object, FcCharSet *charset)
{
  FcValue v;
  v.type = FcTypeCharSet;
  v.u.c = charset;
  return FcPatternAdd(p, object, v, FcTrue);
} 

struct weight_t {
  const char *x11;
  int fc;
};

static weight_t weights[] = {
  { "thin",         FC_WEIGHT_THIN },
  { "extralight",   FC_WEIGHT_EXTRALIGHT },
  { "extra light",  FC_WEIGHT_EXTRALIGHT },
  { "ultralight",   FC_WEIGHT_ULTRALIGHT },
  { "ultra light",  FC_WEIGHT_ULTRALIGHT },
  { "light",        FC_WEIGHT_LIGHT },
  { "book",         FC_WEIGHT_BOOK },
  { "regular",      FC_WEIGHT_REGULAR },
  { "normal",       FC_WEIGHT_NORMAL },
  { "medium",       FC_WEIGHT_MEDIUM },
  { "demi",         FC_WEIGHT_DEMIBOLD },
  { "demibold",     FC_WEIGHT_DEMIBOLD },
  { "demi bold",    FC_WEIGHT_DEMIBOLD },
  { "semibold",     FC_WEIGHT_SEMIBOLD },
  { "semi bold",    FC_WEIGHT_SEMIBOLD },
  { "bold",         FC_WEIGHT_BOLD },
  { "extrabold",    FC_WEIGHT_EXTRABOLD },
  { "extra bold",   FC_WEIGHT_EXTRABOLD },
  { "ultrabold",    FC_WEIGHT_ULTRABOLD },
  { "ultra bold",   FC_WEIGHT_ULTRABOLD },
  { "ultrablack",   FC_WEIGHT_ULTRABOLD },
  { "ultra black",  FC_WEIGHT_ULTRABOLD },
  { "black",        FC_WEIGHT_BLACK  },
  { "heavy",        FC_WEIGHT_HEAVY }
};

struct slant_t {
  const char *x11;
  int fc;
  const char *text;
};

static slant_t slants[] = {
  { "", FC_SLANT_ROMAN, "roman" },
  { "r", FC_SLANT_ROMAN, "roman" },
  { "i", FC_SLANT_ITALIC, "italic" },
  { "o", FC_SLANT_OBLIQUE, "oblique" }
};

struct width_t {
  const char *x11;
  int fc;
};

static width_t widths[] = {
  { "condensed",        FC_WIDTH_CONDENSED },
  { "extended",         FC_WIDTH_EXPANDED },
  { "expanded",         FC_WIDTH_EXPANDED },
  { "narrow",           FC_WIDTH_CONDENSED },
  { "normal",           FC_WIDTH_NORMAL },
  { "semicondensed",    FC_WIDTH_SEMICONDENSED },
  { "semi condensed",   FC_WIDTH_SEMICONDENSED },
  { "extracondensed",   FC_WIDTH_EXTRACONDENSED },
  { "extra condensed",  FC_WIDTH_EXTRACONDENSED },
  { "ultracondensed",   FC_WIDTH_ULTRACONDENSED },
  { "ultra condensed",  FC_WIDTH_ULTRACONDENSED },
  { "semiexpanded",     FC_WIDTH_SEMIEXPANDED },
  { "semi expanded",    FC_WIDTH_SEMIEXPANDED },
  { "extraexpanded",    FC_WIDTH_EXTRAEXPANDED },
  { "extra expanded",   FC_WIDTH_EXTRAEXPANDED },
  { "ultraexpanded",    FC_WIDTH_ULTRAEXPANDED },
  { "ultra expanded",   FC_WIDTH_ULTRAEXPANDED }
};

struct spacing_t {
  const char *x11;
  int fc;
};

static spacing_t spacings[] = {
  { "c", FC_CHARCELL },
  { "d", FC_DUAL },
  { "m", FC_MONO },
  { "p", FC_PROPORTIONAL }
};

void
TFontManagerX11::init() const
{
  if (!fc_x11fonts) {
    toad::flush();
    fc_x11fonts = FcInitLoadConfig();
    if (fc_x11fonts) {
      if (!buildFontList(fc_x11fonts)) {
        cerr << "failed to scan available X11 fonts" << endl;
        FcConfigDestroy(fc_x11fonts);
        fc_x11fonts = 0;
      }
    }
  }
}

bool
TFontManagerX11::buildFontList(FcConfig *config)
{
  bool result = false;
  time_t starttime;
  if (debug_fontmanager_x11) {
    cerr << "building X11 fontconfig list..." << endl;
    starttime = time(NULL);
  }

  FcFontSet *fs = FcConfigGetFonts(config, FcSetSystem);
  if (!fs)
    fs = FcFontSetCreate();
  
  FcPattern *font;

  int count;
  char **fl;
  TX11FontName xfn;

  // there are fonts, which don't match XLFD format like 'fixed', '8x16' or 
  // 'kanji16'. we ignore these and hope they also exist with an XLFD name
  assert(toad::x11display!=NULL);
  fl = XListFonts(toad::x11display, "-*-*-*-*-*-*-*-*-*-*-*-*-*-*", 8192, &count);
  if (count>0)
    result=true;

  if (debug_fontmanager_x11) {
    cout << "found " << count << " fonts with XLFD names" << endl;
  }

  // create a list which ignores the different encodings
  map<string, set<string> > mymap;
  for(int i=0; i<count; ++i) {
    xfn.setXLFD(fl[i]);

    #warning "suboptimal code..."
    // setting these entries to "0" is required for sorting, but
    // in case no variant exists with "0" in all these fields, we
    // may use a bad filename
    xfn.pixels = "0";
    xfn.points = "0";
    xfn.hdpi = "0";
    xfn.vdpi = "0";
    xfn.avg_width = "0";

    string xlfd = xfn.getXLFD();
    
    size_t j = xlfd.rfind('-');
    j = xlfd.rfind('-', j-1);
    mymap[xlfd.substr(0,j)].insert(xlfd.substr(j+1));
  }
  
  if (debug_fontmanager_x11) {
    cout << "found " << mymap.size() << " unique fonts" << endl;
  }  

  // fill the freetype FontSet
  for(map<string, set<string> >::iterator p = mymap.begin();
      p != mymap.end();
      ++p)
  {
#if 0
    cerr << p->first << endl;
    for(set<string>::iterator q = p->second.begin();
        q!=p->second.end();
        ++q)
    {
      cerr << "  " << *q << endl;
    }
#endif
    string xlfd = p->first + "-";
    
    // prefer iso10646-1 over iso8859-1 over ...
    if (p->second.find("iso10646-1")!=p->second.end()) {
      xlfd += "iso10646-1";
    } else
    if (p->second.find("iso8859-1")!=p->second.end()) {
      xlfd += "iso8859-1";
    } else {
      // pick one at random...
      xlfd += *p->second.begin();
    }

    xfn.setXLFD(xlfd.c_str());

    // cout << xlfd << endl;

    font = FcPatternCreate();

    FcPatternAddString(font, FC_RASTERIZER, (FcChar8*)"X11");
    FcPatternAddString(font, FC_FILE, (FcChar8*)xlfd.c_str());

    FcPatternAddString(font, FC_FOUNDRY, (FcChar8*)xfn.vendor.c_str());

    if (!xfn.family.empty()) {
      bool flag = true;
      for(size_t i=0; i<xfn.family.size(); ++i) {
        if (flag)
          xfn.family[i] = toupper(xfn.family[i]);
        flag = (xfn.family[i]==' ');
      }
    }
    FcPatternAddString(font, FC_FAMILY, (FcChar8*)xfn.family.c_str());
    
    for(int j=0; j<sizeof(weights)/sizeof(weight_t); ++j) {
      if (xfn.weight == weights[j].x11) {
        FcPatternAddInteger(font, FC_WEIGHT, weights[j].fc);
        break;
      }
    }

    string slant;
    for(int j=0; j<sizeof(slants)/sizeof(slant_t); ++j) {
      if (xfn.slant == slants[j].x11) {
        FcPatternAddInteger(font, FC_SLANT, slants[j].fc);
        slant = slants[j].text;
        break;
      }
    }
    
    for(int j=0; j<sizeof(widths)/sizeof(width_t); ++j) {
      if (xfn.set_width == widths[j].x11) {
        FcPatternAddInteger(font, FC_WIDTH, widths[j].fc);
        break;
      }
    }
      
    for(int j=0; j<sizeof(spacings)/sizeof(spacing_t); ++j) {
      if (xfn.slant == spacings[j].x11) {
        FcPatternAddInteger(font, FC_SPACING, spacings[j].fc);
        break;
      }
    }

    if (xfn.isScaleable()) {
      FcPatternAddBool(font, FC_SCALABLE, FcTrue);
      FcPatternAddBool(font, FC_OUTLINE, FcTrue);
    } else {
      FcPatternAddBool(font, FC_SCALABLE, FcFalse);
      FcPatternAddBool(font, FC_OUTLINE, FcFalse);

      double px, pt, dpi;
      px = atof(xfn.pixels.c_str());
      pt = atof(xfn.points.c_str()) / 10.0;
      dpi = atof(xfn.hdpi.c_str());
      FcPatternAddDouble(font, FC_PIXEL_SIZE, px);
      FcPatternAddDouble(font, FC_SIZE, pt);
      FcPatternAddDouble(font, FC_DPI, dpi);
    }

    FcPatternAddInteger(font, FC_INDEX, 0);
    FcPatternAddInteger(font, FC_FONTVERSION, 0);

    FcCharSet *charset = FcCharSetCreate();
    FcPatternAddCharset(font, FC_CHARSET, charset);
    FcFontSetAdd(fs, font);
  }
  XFreeFontNames(fl);

//  FcConfigSetFonts(config, fs, FcSetSystem);
  fc_x11fontset = fs;

#if 0
  if (fs) {
    int j;
    for(j=0; j<fs->nfont; j++) {
      FcPatternPrint(fs->fonts[j]);
    }
  }
  cout << "----------------------------------------------" << endl;
#endif

  if (debug_fontmanager_x11) {
    cerr << "X11 fontlist created after " << (time(NULL) - starttime) << "s" << endl;
  }


  return result;
}

struct TX11Font
{
  TX11Font() {
    x11scale = 1.0;
    refcnt = 0;
    normal = 0;
#ifdef HAVE_LIBXUTF8
    xutf8font = NULL;
    xutf8font_r = NULL;
#endif
  }
  ~TX11Font() {
    if (normal) {
      normal->refcnt--;
    }
#ifdef HAVE_LIBXUTF8
    if (xutf8font) {
      XFreeUtf8FontStruct(toad::x11display, xutf8font);
    }
    if (xutf8font_r) {
      XFreeUtf8FontStruct(toad::x11display, xutf8font_r);
    }
#endif
  }

  string id;
  unsigned refcnt;
  TX11Font *normal;
  double x11scale;          // only used for rotated fonts

#ifndef HAVE_LIBXUTF8
  XFontSet x11fs;
  XFontSet x11fs_r;
#else
  XUtf8FontStruct *xutf8font;   // horizontal
  XUtf8FontStruct *xutf8font_r; // rotated
#endif
};

static map<string, TX11Font> cache;

static bool dummy;

static int
dummyhandler(Display *, XErrorEvent *)
{
  dummy = true;
}

void
TFontManagerX11::freeCoreFont(TFont *font)
{
  assert(font->fontmanager==this);
  if (font->corefont) {
    delete static_cast<TX11Font*>(font->corefont);
    font->corefont = 0;
  }
}

bool
TFontManagerX11::allocate(TFont *font, const TMatrix2D *mat)
{
#if 0
  if (font->corefont)
    return true;
#endif

  if (debug_fontmanager_x11) {
    cout << "allocate font '" << font->getFont() << "'" << endl;
  }

//cout << __PRETTY_FUNCTION__ << endl;
  if (!font->font) {
    cout << "error: font has no pattern" << endl;
    return false;
  }
  assert(font->fontmanager==this);
  init();
  TX11Font *x11 = 0;
  if (!font->corefont) {
    x11 = new TX11Font;
    font->corefont = x11;
  } else {
    x11 = static_cast<TX11Font*>(font->corefont);
  }

  // do we have an allocated font?
  string newid;
  if (mat && !mat->isIdentity()) {
//cout << "allocate rotated font **************************" << endl;
    double d = 12.0;
    FcPatternGetDouble(font->font, FC_SIZE, 0, &d);
    newid = "[";
    newid += d2s(mat->a11 * d);
    newid += d2s(mat->a12 * d);
    newid += d2s(mat->a21 * d);
    newid += d2s(mat->a22 * d);
    newid += "]";

    if (newid != x11->id) {
      // in case we have the wrong rotated font, free it
#ifdef HAVE_LIBXUTF8
      if (x11->xutf8font_r) {
        XFreeUtf8FontStruct(toad::x11display, x11->xutf8font_r);
        x11->xutf8font_r = 0;
      }
#endif
      x11->id = newid;
    } else {
      // we have the correct rotated font, we're done => return
#ifdef HAVE_LIBXUTF8
      if (x11->xutf8font_r) {
        return true;
      }
#endif
    }
  } else {
#ifdef HAVE_LIBXUTF8
    if (x11->xutf8font_r) {
      XFreeUtf8FontStruct(toad::x11display, x11->xutf8font_r);
      x11->xutf8font_r = 0;
    }
    if (x11->xutf8font) {
      return true;
    }
#endif
  }

cout << "allocate font of size " << x11->id << endl;

  // allocate the font
  FcPattern *pattern = FcPatternDuplicate(font->font);

  // Execute substitutions
  FcConfigSubstitute(fc_x11fonts, pattern, FcMatchPattern);
  // Perform default substitutions in a pattern
  FcDefaultSubstitute(pattern);
  
  FcResult result;
  FcPattern *found = FcFontSetMatch(fc_x11fonts,
                                    &fc_x11fontset, 1,
                                    pattern,
                                    &result);
//  FcPattern *found = FcFontMatch(fc_x11fonts, pattern, &result);

  FcChar8 *filename;
  FcPatternGetString(found, FC_FILE, 0, &filename);
  TX11FontName xfn;
  xfn.setXLFD((char*)filename);

  if (xfn.isScaleable()) {
    double size=12.0, dpi=100.0;
    FcPatternGetDouble(found, FC_SIZE, 0, &size);
    FcPatternGetDouble(found, FC_DPI, 0, &dpi);
    
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%i", (int)(size*10.0));
    xfn.points = buffer;
    snprintf(buffer, sizeof(buffer), "%i", (int)dpi);
    xfn.hdpi = buffer;
    xfn.vdpi = buffer;
    xfn.pixels="0";
  }

  FcPatternDestroy(pattern);

#ifdef HAVE_LIBXUTF8
  XUtf8FontStruct *xutf8font_new = 0;
  if (!x11->xutf8font) {
#endif
  
    // In case someone is scaling, font size may become bigger
    // than the ones actually used on the screen, so we set a
    // limit here at 2304pt (as an unwanted side effect this is
    // also the largest unscaled font size now...)
    unsigned points;
    sscanf(xfn.points.c_str(), "%u", &points);
    if (points>23040) {
      double pt = points;
      x11->x11scale = pt / 23040.0;
      xfn.points = "23040";
    } else {
      x11->x11scale = 1.0;
    }

    // finally load the font
#ifdef HAVE_LIBXUTF8
    XErrorHandler oldhandler;
    oldhandler = XSetErrorHandler(dummyhandler);
    dummy = false;
    xutf8font_new = XCreateUtf8FontStruct(toad::x11display, xfn.getXLFD().c_str());
    XSync(toad::x11display, False);
    XSetErrorHandler(oldhandler);
    if (dummy || !xutf8font_new) {
      cerr << "error while loading font: "
           << "  failed to load Utf8 font structure '" << xfn.getXLFD() << "'\n";
      return false;
    }
  }
#endif
  
  // In case a matrix is available, we need to load a 2nd font,
  // which will be used for output. The untransformed font will
  // then be used to get text extents.
#ifdef HAVE_LIBXUTF8
  XUtf8FontStruct *xutf8font_r_new = 0;
#endif
  if (mat && !mat->isIdentity()) {
    xfn.points = x11->id;
    xfn.pixels = "0";

    // Some matrizes, ie. small ones, cause XLoadFont to fail, so we
    // need to temporary install an error handler or the default error
    // handler would call 'exit(..)'. the XSync below is done because
    // XLoadFont is assynchron.
    XErrorHandler oldhandler;
    oldhandler = XSetErrorHandler(dummyhandler);
    dummy = false;
#ifdef HAVE_LIBXUTF8
    xutf8font_r_new = XCreateUtf8FontStruct(x11display, xfn.getXLFD().c_str());
#endif
    XSync(toad::x11display, False);
    XSetErrorHandler(oldhandler);
    if (dummy) {
      cerr << "error while loading font:\n"
           << "  failed to load X11 font '" << xfn.getXLFD() << "'\n";
#ifdef HAVE_LIBXUTF8
      if (xutf8font_r_new) {
        XFreeUtf8FontStruct(toad::x11display, xutf8font_r_new);
      }
#endif
      return false;
    }
  }
  
  // set new font
#ifdef HAVE_LIBXUTF8
  if (xutf8font_new) {
    assert(x11->xutf8font==0);
    x11->xutf8font = xutf8font_new;
  }
  if (xutf8font_r_new) {
    assert(x11->xutf8font_r==0);
//cout << "have rotated font" << endl;
    x11->xutf8font_r = xutf8font_r_new;
  }
//else cout << "no rotated font" << endl;
#endif
  return true;
}

void
TFontManagerX11::drawString(TPenBase *penbase, int x, int y, const char *str, size_t strlen, bool transparent)
{
  TPen *pen = dynamic_cast<TPen*>(penbase);
  assert(pen);
  assert(pen->font);

//  if (!pen->font->corefont)
//    return;
  if (!allocate(pen->font, pen->mat))
    return;
  TX11Font *x11 = static_cast<TX11Font*>(pen->font->corefont);
  // y+=getAscent(pen->font);
#ifdef HAVE_LIBXUTF8
  if (x11->xutf8font) {
    y += x11->x11scale * x11->xutf8font->ascent;
  }
#endif

  if (!pen->mat || pen->mat->isIdentity()) {
    if (pen->mat)
      pen->mat->map(x, y, &x, &y);
#ifdef HAVE_LIBXUTF8
    if (transparent) {
      XUtf8DrawString(toad::x11display, pen->x11drawable, x11->xutf8font, pen->o_gc, x, y, str, strlen);
    } else {
      int h = getHeight(pen->font);
    	XFillRectangle(toad::x11display, pen->x11drawable, 
                     pen->two_colors ? pen->f_gc : pen->o_gc,
    	               x, y - h,
    	               XUtf8TextWidth(x11->xutf8font, str, strlen), h);
      XUtf8DrawString(toad::x11display, pen->x11drawable, x11->xutf8font, pen->o_gc, x, y, str, strlen);
    }
#endif
  } else {
    // Draw rotated and downscaled font char by char, using the horizontal
    // unscaled font as a reference. This is much slower than a single
    // X*DrawString call but the precision is required, even for horizontal
    // and just scaled fonts.
#ifdef HAVE_LIBXUTF8
    assert(x11->xutf8font_r);
#endif
    int x2, y2;
    const char *p = str;
    int len=0;
    while(*p && len<strlen) {
      char buffer[5];
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
  
      int direction, fasc, fdesc;
#ifdef HAVE_LIBXUTF8
      pen->mat->map(x, y, &x2, &y2);
      if (transparent) {
        XUtf8DrawString(toad::x11display, pen->x11drawable, x11->xutf8font_r, pen->o_gc, x2, y2, buffer, clen);
      } else {
        int h = getHeight(pen->font);
      	XFillRectangle(toad::x11display, pen->x11drawable, 
                       pen->two_colors ? pen->f_gc : pen->o_gc,
    	                 x, y - h,
    	                 XUtf8TextWidth(x11->xutf8font, str, strlen), h);
        XUtf8DrawString(toad::x11display, pen->x11drawable, x11->xutf8font, pen->o_gc, x, y, buffer, clen);
      }
      x += x11->x11scale * XUtf8TextWidth(x11->xutf8font, buffer, clen);
#endif
//      x+=getTextWidth(pen->font, buffer, clen);
    }
  }
}

int 
TFontManagerX11::getHeight(TFont *font)
{
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TX11Font *x11 = static_cast<TX11Font*>(font->corefont);
#ifdef HAVE_LIBXUTF8
  if (x11->xutf8font) {
    return x11->x11scale * (x11->xutf8font->ascent + x11->xutf8font->descent);
  }
#endif
  return 0;
}

int 
TFontManagerX11::getAscent(TFont *font)
{
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TX11Font *x11 = static_cast<TX11Font*>(font->corefont);
#ifdef HAVE_LIBXUTF8
  if (x11->xutf8font) {
    return x11->x11scale * x11->xutf8font->ascent;
  }
#endif
  return 0;
}

int 
TFontManagerX11::getDescent(TFont *font)
{
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TX11Font *x11 = static_cast<TX11Font*>(font->corefont);
#ifdef HAVE_LIBXUTF8
  if (x11->xutf8font) {
    return x11->x11scale * x11->xutf8font->descent;
  }
#endif
  return 0;
}

int
TFontManagerX11::getTextWidth(TFont *font, const char *text, size_t n)
{
  if (n==0)
    return 0;
  if (!font->corefont && !allocate(font, 0))
    return 0;
  TX11Font *x11 = static_cast<TX11Font*>(font->corefont);
#ifdef HAVE_LIBXUTF8
  if (x11->xutf8font) {
    return x11->x11scale * XUtf8TextWidth(x11->xutf8font, text, n);
  }
#endif
  return 0;
}



/*********************************************************************
 *                                                                   *
 * TX11FontName definitions                                          *
 *                                                                   *
 *********************************************************************/

void
TX11FontName::setWildcards()
{
  vendor = family = weight = slant = set_width = adj_style =
  pixels = points = hdpi = vdpi = spacing = avg_width =
  registry = encoding = "*";
}

void
TX11FontName::setXLFD(const char *name)
{
  int l, r;
    
  l = 1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  vendor.assign(name+l, r-l);
    
  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  family.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  weight.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  slant.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  set_width.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  adj_style.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  pixels.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  points.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  hdpi.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  vdpi.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  spacing.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  avg_width.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  registry.assign(name+l, r-l);

  l=r+1;
  for(r=l; name[r]!=0 && name[r]!='-'; ++r);
  encoding.assign(name+l, r-l);
}

string
TX11FontName::getXLFD() const
{
  string name;
  name = "-" + vendor +
         "-" + family +
         "-" + weight +
         "-" + slant + 
         "-" + set_width + 
         "-" + adj_style +
         "-" + pixels +
         "-" + points +
         "-" + hdpi +
         "-" + vdpi +
         "-" + spacing +
         "-" + avg_width +
         "-" + registry +
         "-" + encoding;
  return name;
}

bool
TX11FontName::isScaleable() const
{
   return (pixels=="0" &&
           points=="0" && 
           avg_width=="0");
}

/*
 * X11R6 provides 2d transformations for characters
 *
 * + : plus
 * ~ : minus
 *                  / a b 0 \
 *  [ a b c d ] <-> | c d 0 |
 *                  \ 0 0 1 /
 */
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
