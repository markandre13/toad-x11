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

#ifndef TFont
#define TFont TFont

#include <toad/os.hh>
#include <toad/config.h>
#include <toad/toadbase.hh>
#include <toad/matrix2d.hh>
#include <toad/pointer.hh>
#include <fontconfig/fontconfig.h>

#if ( FC_VERSION < 20200 )
  #warning "Fontconfig 2.2.0 or higher, ie. from the CVS at freedesktop.org, is required."
#endif

// #include <toad/tablemodels.hh>

#ifdef HAVE_LIBXFT    
typedef struct _XftFont XftFont;
#endif

#ifdef HAVE_LIBXUTF8
#include <libXutf8/Xutf8.h>
#endif

namespace toad {

class TFont:
  public TSmartObject, public TOADBase
{
  friend class TPen;
  
  public:
    TFont();
    TFont(const string &fontname);
    void setFont(const string &fontname);
    virtual ~TFont();

  public:
    enum ERenderType {
      RENDER_X11,
      RENDER_FREETYPE
    };
    static ERenderType default_rendertype;
    static FcConfig* getFcConfig();
    static string default_font;
//  private:
    string id;
#ifdef __X11__
    double x11scale;          // only used for rotated fonts

#ifdef TOAD_OLD_FONTCODE
    _TOAD_FONTSTRUCT x11fs;
    _TOAD_FONT       x11font; // only used for rotated fonts
#endif

#ifdef HAVE_LIBXUTF8
    XUtf8FontStruct *xutf8font;   // horizontal
    XUtf8FontStruct *xutf8font_r; // rotated
#endif

    void createX11Font(TMatrix2D*);

#ifdef HAVE_LIBXFT    
    XftFont * xftfont;
    int ascent, descent;
    void createXftFont(TMatrix2D*);
#endif
#endif

    void init();
    void clear();

    string fontname; // Fontconfig font name
    ERenderType rendertype;

  public:
    void setRenderType(ERenderType rt) { rendertype=rt; clear(); }
    ERenderType getRenderType() const { return rendertype; }
    
    void createFont(TMatrix2D*);

#ifdef __X11__    
#ifdef TOAD_OLD_FONTCODE
    _TOAD_FONT getX11Font() const;
#endif
    
#ifdef HAVE_LIBXFT
    XftFont * getXftFont() const {
      return xftfont;
    }
#endif    
#endif

    int getTextWidth(const string&) const;
    int getTextWidth(const char*) const;
    int getTextWidth(const char*,int len) const;
    int getTextWidth(const unsigned char *s) const { return getTextWidth((const char*)s); }
    int getTextWidth(const unsigned char *s,int len) const { return getTextWidth((const char*)s,len); }
    int getAscent() const;
    int getDescent() const;
    int getHeight() const {return getAscent()+getDescent();}
};

typedef GSmartPointer<TFont> PFont;

extern PFont default_font;
extern PFont bold_font;

} // namespace toad

#endif
