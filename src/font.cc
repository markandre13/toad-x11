/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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
#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#define _TOAD_PRIVATE

#include <toad/config.h>
#include <toad/toadbase.hh>
#include <toad/font.hh>
#include <toad/pen.hh>

#ifdef HAVE_LIBXFT

#ifdef _XFT_NO_COMPAT_
#undef _XFT_NO_COMPAT_
#endif

#include <X11/Xft/Xft.h>

#endif

#ifndef HAVE_LIBXFT
#define XftDraw void
#endif

#ifndef FC_WEIGHT_BOOK                       
#define FC_WEIGHT_BOOK 75            
#endif
#ifndef FC_DUAL
#define FC_DUAL 90
#endif

using namespace toad;

TFont::ERenderType TFont::default_rendertype = TFont::RENDER_X11;
string TFont::default_font("arial,helvetica,sans-serif");

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

static bool X11ConfigBuildFonts(FcConfig *config);
static string d2s(double d);

TFont::TFont()
{
  init();
  createFont(0);
}

/**
 *
 * Possible fontnames:
 *
 * \li Times
 * \li Times-12
 * \li Times:size=12
 * \li Times,serif:size=12
 * \li Times:size=12:dpi=100
 * \li Times-12:bold
 * \li Times-12:bold:italic
 * \li Times:bold:italic:pixelsize=18
 */
TFont::TFont(const string &fontname)
{
  init();
  this->fontname = fontname;
  createFont(0);
}

void
TFont::setFont(const string &fontname)
{
  this->fontname = fontname;
  createFont(0);
}

TFont::~TFont() {
  clear();
}

static FcConfig *fc_x11fonts = 0;

void
TFont::init()
{
  if (!fc_x11fonts) {
    fc_x11fonts = FcInitLoadConfig();
    if (fc_x11fonts) {
      if (!X11ConfigBuildFonts(fc_x11fonts)) {
        cerr << "failed to scan available X11 fonts" << endl;
        FcConfigDestroy(fc_x11fonts);
        fc_x11fonts = 0;
      }
    }
  }
  x11scale = 1.0;
#ifdef TOAD_OLD_FONTCODE
  x11font = 0;
  x11fs = 0;
#endif
#ifdef HAVE_LIBXUTF8
  xutf8font = 0;
  xutf8font_r = 0;
#endif
#ifdef HAVE_LIBXFT
  xftfont = 0;
#endif

  fontname = default_font;
  setRenderType(default_rendertype);
}

void
TFont::clear()
{
#ifdef TOAD_OLD_FONTCODE
  if (x11font) {
    XUnloadFont(x11display, x11font);
    x11font = 0;
  }
  if (x11fs) {
    XUnloadFont(x11display, x11fs->fid);
    XFreeFontInfo(NULL, x11fs, 0);
    x11fs = 0;
  }
#endif
#ifdef HAVE_LIBXUTF8
  if (xutf8font) {
    XFreeUtf8FontStruct(x11display, xutf8font);
    xutf8font = 0;
  }
  if (xutf8font_r) {
    XFreeUtf8FontStruct(x11display, xutf8font_r);
    xutf8font = 0;
  }
#endif
#ifdef HAVE_LIBXFT
  if (xftfont) {
    XftFontClose(x11display, xftfont);
    xftfont = 0;
  }
#endif
}

int
TFont::getAscent() const
{
#ifdef TOAD_OLD_FONTCODE
  if (x11fs) {
    return x11scale * x11fs->ascent;
  }
#endif
#ifdef HAVE_LIBXUTF8
  if (xutf8font) {
    return xutf8font->ascent;
  }
#endif
#ifdef HAVE_LIBXFT
  if (xftfont)
    return x11scale * xftfont->ascent;
#endif
cerr << __PRETTY_FUNCTION__ << ": empty font" << endl;
  return 0;
}

int
TFont::getDescent() const
{
#ifdef TOAD_OLD_FONTCODE
  if (x11fs) {
    return x11scale * x11fs->descent;
  }
#endif
#ifdef HAVE_LIBXUTF8
  if (xutf8font) {
    return xutf8font->descent;
  }
#endif
#ifdef HAVE_LIBXFT
  if (xftfont)
    return x11scale * xftfont->descent;
#endif
cerr << __PRETTY_FUNCTION__ << ": empty font" << endl;
  return 0;
}

int
TFont::getTextWidth(const string &str) const
{
  return getTextWidth(str.c_str(),str.size());
}
 
int
TFont::getTextWidth(const char *str) const
{
  return getTextWidth(str,strlen(str));
}
 
int
TFont::getTextWidth(const char *str, int len) const
{
  if (len==0)
    return 0;
#ifdef TOAD_OLD_FONTCODE
  if (x11fs) {
    return x11scale * XTextWidth(x11fs,str,len);
  }
#endif
#ifdef HAVE_LIBXUTF8
  if (xutf8font) {
    return x11scale * XUtf8TextWidth(xutf8font,str,len);
  }
#endif
#ifdef HAVE_LIBXFT
  if (xftfont) {
    XGlyphInfo gi;
    XftTextExtentsUtf8(x11display, xftfont, (const XftChar8*)str, len, &gi);
    if (str[len-1]==' ') {
      XGlyphInfo gi2;
      XftTextExtentsUtf8(x11display, xftfont, (const XftChar8*)"  ", 2, &gi2);
      return (gi.width+gi2.width)*x11scale;
    }
    return gi.width*x11scale;
  }
#endif
  return 0;
}

void
TFont::createFont(TMatrix2D *mat)
{
  switch(rendertype) {
    case RENDER_X11:
      createX11Font(mat);
      break;
    case RENDER_FREETYPE:
#ifdef HAVE_LIBXFT
      createXftFont(mat);
#else
      cerr << "fatal: no Xft available" << endl;
#endif
      break;
  }
}

#ifdef TOAD_OLD_FONTCODE
Font
TFont::getX11Font() const
{
  if (x11font)
    return x11font;
  if (x11fs)
    return x11fs->fid;
  return 0;
}
#endif

static bool dummy;

static int
dummyhandler(Display *, XErrorEvent *)
{
  dummy = true;
}

void
TFont::createX11Font(TMatrix2D *mat)
{
  FcPattern *pattern = FcNameParse( (FcChar8 *)fontname.c_str());

  // check that the required transformation matches the current font
  string newid;
  if (mat && !mat->isIdentity()) {
    double d;
    FcPatternGetDouble(pattern, FC_SIZE, 0, &d);
    newid = "[";
    newid += d2s(mat->a11 * d);
    newid += d2s(mat->a12 * d);
    newid += d2s(mat->a21 * d);
    newid += d2s(mat->a22 * d);
    newid += "]";

    if (newid != id) {
#ifdef TOAD_OLD_FONTCODE
      if (x11font) {  
        // we have an rotated font, but not the one we need
        XUnloadFont(x11display, x11font);
        x11font = 0;
      }
#endif
#ifdef HAVE_LIBXUTF8
      if (xutf8font_r) {
        XFreeUtf8FontStruct(x11display, xutf8font_r);
        xutf8font_r = 0;
      }
#endif
      id = newid;
    } else {
#ifdef TOAD_OLD_FONTCODE
      if (x11font) {
        // we have an rotated font and it's the one we need
        return;
      }
#endif
#ifdef HAVE_LIBXUTF8
      if (xutf8font_r) {
        return;
      }
#endif
    }
  } else {
    // no rotation
#ifdef HAVE_LIBXUTF8
    if (xutf8font_r) {
      XFreeUtf8FontStruct(x11display, xutf8font_r);
      xutf8font_r = 0;
    }
#endif
#ifdef TOAD_OLD_FONTCODE
    if (x11font) {
      XUnloadFont(x11display, x11font);
      x11font = 0;
    }
    if (x11fs) {
      // no rotated font is required and we have the normal font
      return;
    }
#endif
#ifdef HAVE_LIBXUTF8
    if (xutf8font) {
      return;
    }
#endif
  }

  // Execute substitutions
  FcConfigSubstitute(fc_x11fonts, pattern, FcMatchPattern);
  // Perform default substitutions in a pattern
  FcDefaultSubstitute(pattern);
  
  FcResult result;
  FcPattern *found = FcFontMatch(fc_x11fonts, pattern, &result);

  FcChar8 *filename;
  FcPatternGetString(found, FC_FILE, 0, &filename);
  TX11FontName xfn;
  xfn.setXLFD((char*)filename);

  FcBool scaleable;
  FcPatternGetBool(found, FC_SCALABLE, 0, &scaleable);
  if (scaleable) {
    double size, dpi;
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

#ifdef TOAD_OLD_FONTCODE
  XFontStruct *new_fs = NULL;
  if (!x11fs) {
#endif
#ifdef HAVE_LIBXUTF8
  XUtf8FontStruct *xutf8font_new = 0;
  if (!xutf8font) {
#endif
  
    // In case someone is scaling, font size may become bigger
    // than the ones actually used on the screen, so we set a
    // limit here at 2304pt (as an unwanted side effect this is
    // also the largest unscaled font size now...)
    unsigned points;
    sscanf(xfn.points.c_str(), "%u", &points);
    if (points>23040) {
      double pt = points;
      x11scale = pt / 23040.0;
      xfn.points = "23040";
    }

    // finally load the font
#ifdef TOAD_OLD_FONTCODE
    new_fs = XLoadQueryFont(x11display, xfn.getXLFD().c_str());
    if (!new_fs) {
      cerr << "error while loading font '" << fontname << "':\n"
           << "  failed to load X11 font structure '" << xfn.getXLFD() << "'\n";
      return;
    }
#endif
#ifdef HAVE_LIBXUTF8
    xutf8font_new = XCreateUtf8FontStruct(x11display, xfn.getXLFD().c_str());
    if (!xutf8font_new) {
      cerr << "error while loading font '" << fontname << "':\n"
           << "  failed to load Utf8 font structure '" << xfn.getXLFD() << "'\n";
    }
  }
#endif
  
  // In case a matrix is available, we need to load a 2nd font,
  // which will be used for output. The untransformed font will
  // then be used to get text extents.
#ifdef TOAD_OLD_FONTCODE
  Font new_font = 0;
#endif
#ifdef HAVE_LIBXUTF8
  XUtf8FontStruct *xutf8font_r_new = 0;
#endif
  if (mat && !mat->isIdentity()) {
    xfn.points = id;
    xfn.pixels = "0";

    // Some matrizes, ie. small ones, cause XLoadFont to fail, so we
    // need to temporary install an error handler or the default error
    // handler would call 'exit(..)'. the XSync below is done because
    // XLoadFont is assynchron.
    XErrorHandler oldhandler;
    oldhandler = XSetErrorHandler(dummyhandler);
    dummy = false;
#ifdef TOAD_OLD_FONTCODE
    new_font = XLoadFont(x11display, xfn.getXLFD().c_str());
#endif
#ifdef HAVE_LIBXUTF8
    xutf8font_r_new = XCreateUtf8FontStruct(x11display, xfn.getXLFD().c_str());
#endif
    XSync(x11display, False);
    XSetErrorHandler(oldhandler);
    if (dummy) {
      cerr << "error while loading font '" << fontname << "':\n"
           << "  failed to load X11 font '" << xfn.getXLFD() << "'\n";
#ifdef TOAD_OLD_FONTCODE
      if (new_fs) {
        XUnloadFont(x11display, new_fs->fid);
        XFreeFontInfo(NULL,new_fs,0);
      }
#endif
#ifdef HAVE_LIBXUTF8
      if (xutf8font_r_new) {
        XFreeUtf8FontStruct(x11display, xutf8font_r_new);
      }
#endif
      return;
    }
  }
  
  // set new font
#ifdef TOAD_OLD_FONTCODE
  if (new_font) {
    assert(x11font==0);
    x11font = new_font;
  }
  if (new_fs) {
    assert(x11fs==NULL);
    x11fs = new_fs;
  }
#endif
#ifdef HAVE_LIBXUTF8
  if (xutf8font_new) {
    assert(xutf8font==0);
    xutf8font = xutf8font_new;
  }
  if (xutf8font_r_new) {
    assert(xutf8font_r==0);
    xutf8font_r = xutf8font_r_new;
  }
#endif
}

#ifdef HAVE_LIBXFT
void
TFont::createXftFont(TMatrix2D *mat)
{
  if (xftfont) {
    XftFontClose(x11display, xftfont);
    xftfont = 0;
  }
  XftPattern *pattern = XftNameParse(fontname.c_str());

  // fontconfig-devel.txt sais that the default dpi is 75 but my
  // version took about 100 dpi, so force it to 75 dpi here:
  double dpi;
  if (XftPatternGetDouble(pattern, XFT_DPI, 0, &dpi)==XftResultNoMatch) {
    XftPatternAddDouble(pattern, XFT_DPI, 75.0);
  }

  if (mat && !mat->isIdentity()) {
    double x1, y1, x2, y2;
    TMatrix2D m(*mat);
    m.invert();
    m.map(0.0, 0.0, &x1, &y1);
    m.map(0.0, 1.0, &x2, &y2);
    x1-=x2;
    y1-=y2;
    x11scale = sqrt(x1*x1+y1*y1);
    XftMatrix xftmat;
    XftMatrixInit(&xftmat);
    xftmat.xx = mat->a11;
    xftmat.yx = mat->a12;
    xftmat.xy = mat->a21;
    xftmat.yy = mat->a22;
    XftPatternAddMatrix(pattern, XFT_MATRIX, &xftmat);
  }

  XftResult result;
  XftPattern *found;
  found = XftFontMatch(x11display, x11screen, pattern, &result);

  XftFont *new_font = XftFontOpenPattern(x11display, found);
  if (new_font) {
    clear();
    xftfont = new_font;
  }

  XftPatternDestroy(pattern);
}
#endif

#if 0

static void
drawString16(int x, int y,
             XChar2b *text,
             unsigned len )
{
  font.createFont(pen.mat);

  switch(font.getRenderType()) {
    case TFont::RENDER_X11:
      if (!font.getX11Font())
        return;
      XSetFont(x11display, pen.o_gc, font.getX11Font());
      y+=font.getAscent();
      if (!pen.mat) {
        XDrawString16(x11display, pen.x11drawable, pen.o_gc, x,y, text, len );
cerr << __LINE__ << endl;
      } else {
#if 1
        pen.mat->map(x, y, &x, &y);
        XDrawString16(x11display, pen.x11drawable, pen.o_gc, x,y, text, len );
#else
        int x2, y2;
        string::const_iterator p(text.begin()), e(text.end());
        while(p!=e) {
          XChar2b buffer[2];
          buffer[0]=*p;
          buffer[1]=0;
          pen.mat->map(x, y, &x2, &y2);
          XDrawString16(x11display, pen.x11drawable, pen.o_gc, x2,y2, buffer, 1);
          x+=font.getTextWidth(buffer);
          ++p;
        }
#endif
      }
      break;
#ifdef HAVE_LIBXFT
    case TFont::RENDER_FREETYPE: {
      y+=font.getAscent();
      if (pen.mat)
        pen.mat->map(x, y, &x, &y);
      XftColor color;
      color.color.red = 0;
      color.color.green = 0;
      color.color.blue = 0;
      color.color.alpha = 0xffff;
for(int i=0; i<len; ++i) {
  int a = text[i].byte1;
  text[i].byte1 = text[i].byte2;
  text[i].byte2 = a;
}
      XftDrawString16(xftdraw, &color, font.getXftFont(), x,y, (XftChar16*)text, len);
      } break;
#endif
  }
}


#endif


/*********************************************************************
 *                                                                   *
 * Create Fontconfig configuration with X11 server fonts             *
 *                                                                   *
 *********************************************************************/

extern "C" {
extern void 
FcConfigSetFonts(FcConfig *config,
                 FcFontSet *fonts,
                 FcSetName set);

extern FcFontSet *
FcConfigGetFonts (FcConfig  *config,
                  FcSetName set);
}

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

width_t widths[] = {
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

/**
 * 
 */
bool
X11ConfigBuildFonts(FcConfig *config)
{
  bool result = false;

  FcFontSet *fs = FcConfigGetFonts(config, FcSetSystem);
  if (!fs)
    fs = FcFontSetCreate();
  
  FcPattern *font;
/*
  Display *x11display;
  if ((x11display = XOpenDisplay(""))==NULL) {
    cerr << "Couldn't open X11 display\n";
    return result;
  }
*/
  int count;
  char **fl;
  TX11FontName xfn;
  
  // unicode 0x00-0xFF
  fl = XListFonts(x11display, "*-iso8859-1", 8192, &count);
  // unicode 0x0000 - 0xFFFF
  //  fl = XListFonts(x11display, "*-iso10646-1", 8192, &count);
  if (count>0)
    result=true;
  for(int i=0; i<count; ++i) {
    xfn.setXLFD(fl[i]);
    font = FcPatternCreate();
    FcPatternAddString(font, FC_RASTERIZER, (FcChar8*)"X11");
    FcPatternAddString(font, FC_FILE, (FcChar8*)fl[i]);
#if 0
    FcPatternAddString(font, FC_LANG, (FcChar8*)
      "aa|af|ar|ast|ava|ay|be|bg|bi|bin|br|bs|ca|ce|ch|co|cs|cy|da|de|el|"
      "en|eo|es|et|eu|fi|fj|fo|fr|fur|fy|gd|gl|gn|gv|he|ho|hr|hu|ia|ibo|id|"
      "ie|ik|io|is|it|ki|kl|kum|la|lb|lez|lt|lv|mg|mh|mt|nb|nl|nn|no|ny|oc|"
      "om|os|pl|pt|rm|ru|se|sel|sh|sk|sl|sma|smj|smn|so|sq|sr|sv|sw|tn|tr|"
      "ts|ug|uk|ur|vo|vot|wa|wen|wo|xh|yap|yi|zu");
#endif
    FcPatternAddString(font, FC_FOUNDRY, (FcChar8*)xfn.vendor.c_str());

    if (!xfn.family.empty()) {
      bool flag = true;
      for(unsigned i=0; i<xfn.family.size(); ++i) {
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

#if 0
    FcPatternAddString(font, FC_STYLE, (FcChar8*)"Regular");
#endif
#if 0
    // style overwrites weight and slant

    string style = xfn.weight + ' ' + xfn.set_width + ' ' + slant;
    FcPatternAddString(font, FC_STYLE, (FcChar8*)style.c_str());
/*
  weight  width     slant     
  Regular
  Regular           Italic
  Regular           Oblique
  Bold              Italic
  Plain
  Roman
  Medium            Italic
  Bold              Italic
  Normal
  Demi
  Regular Condensed Italic
  Bold    Condensed Italic
  Bold              Oblique
  Demi              Oblique
  Demi Bold         Italic
  Regular Condensed
  Book              Oblique

  no style or weight -> "Medium"
  no style or slant  -> "Roman"
  no pixel size      -> 12pt, 75dpi, scale=1
*/
#endif

    FcCharSet *charset = FcCharSetCreate();
    FcPatternAddCharset(font, FC_CHARSET, charset);
    FcFontSetAdd(fs, font);
  }
  XFreeFontNames(fl);

  FcConfigSetFonts(config, fs, FcSetSystem);

#if 0
  if (fs) {
    int j;
    for(j=0; j<fs->nfont; j++) {
      FcPatternPrint(fs->fonts[j]);
    }
  }
  cout << "----------------------------------------------" << endl;
#endif
  return result;
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
           hdpi=="0" && 
           vdpi=="0" &&
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
