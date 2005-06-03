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

#ifndef TColor
#define TColor TColor

#include <toad/os.hh>
#include <toad/pointer.hh>

#include <toad/io/serializable.hh>

#define RGB_PALETTE_MAX 126

namespace toad {

struct TRGB
{
    byte r,g,b;
    TRGB() {
    }
    TRGB(byte ir, byte ig, byte ib) {
      r=ir; g=ig; b=ib;
    }

    bool operator ==(const TRGB &c) const {
      return (r==c.r && g==c.g && b==c.b);
    }
    bool operator !=(const TRGB &c) const {
      return (r!=c.r || g!=c.g || b!=c.b);
    }
    void operator() (byte rn,byte gn,byte bn) {
      r=rn;g=gn;b=bn;
    }
    void set(byte rn,byte gn,byte bn) {
      r=rn;g=gn;b=bn;
    }
};

struct TSerializableRGB:
  public TRGB, public TSerializable
{
    typedef TSerializable super;

    TSerializableRGB() {}

    TSerializableRGB(byte ir, byte ig, byte ib) {
      r=ir; g=ig; b=ib;
    }
    TSerializableRGB& operator= (const TRGB &c) {
      r=c.r; g=c.g; b=c.b;
      return *this;
    }
/*
    bool operator ==(const TRGB &c) const {
      return (r==c.r && g==c.g && b==c.b);
    }
    bool operator !=(const TRGB &c) const {
      return (r!=c.r || g!=c.g || b!=c.b);
    }
    void operator() (byte rn,byte gn,byte bn) {
      r=rn;g=gn;b=bn;
    }
    void set(byte rn,byte gn,byte bn) {
      r=rn;g=gn;b=bn;
    }
*/
    SERIALIZABLE_INTERFACE(toad::, TSerializableRGB)
};

class TColor
  :public TSerializableRGB
{
    friend class TPen;
    friend class TWindow;
    friend class TBitmap;

#ifdef __X11__
    class TData:
      public TSmartObject
    {
      public:
        TData();
        ~TData();
        bool pixel_is_valid;
        ulong xpixel;
        ulong /* Pixmap */ pm;
    };
#endif

  public:
    enum EColor16 {
      BLACK=0,
      RED=1,
      GREEN=2,
      YELLOW=3,
      BLUE=4,
      MAGENTA=5,
      CYAN=6,
      GREY=7, GRAY=7,
      LIGHTGREY=8, LIGHTGRAY=8,
      LIGHTRED,
      LIGHTGREEN,
      LIGHTYELLOW,
      LIGHTBLUE,
      LIGHTMAGENTA,
      LIGHTCYAN,
      WHITE,
      COLOR16_MAX
    };

    enum ESystemColor {
      BTNTEXT=0,            // buttonbase
      BTNSHADOW,
      BTNFACE,
      BTNLIGHT,
      MENU,                 // menubar
      MENUTEXT,
      TEXTEDIT,
      MDIAREA,              // mdi stuff
      CAPTION,
      CAPTIONTEXT,
      INACTIVECAPTION,
      INACTIVECAPTIONTEXT,
      DIALOG,               // dialog
      DIALOGTEXT,
      SLIDER_FACE,          // scrollbar slider
      SLIDER_SHADOW,
      SLIDER_LIGHT,
      SELECTED,             // background for selected text when focus
      SELECTED_2,
      SELECTED_TEXT,        // color for selected text
      SELECTED_GRAY,        // background for selected text when no focus
      SELECTED_GRAY_2,
      TABLE_CELL,           // table cell color
      TABLE_CELL_2,
      FIGURE_SELECTION,     // used by figureeditor to mark selections
      MAX
    };

    enum EDitherMode {
      NEAREST = 0,
      DITHER16 = 1,
      DITHER27 = 2,
      DITHER125 = 3,
      DITHER = 3
    };

    TColor();
    TColor(byte r, byte g, byte b);
    TColor(EColor16);
    TColor(ESystemColor);
    ~TColor();

    void set(const TRGB &c) {
      r = c.r;
      g = c.g;
      b = c.b;
#ifdef __X11__
      _data = NULL;
#endif
#ifdef __WIN32__
      colorref = RGB(r, g, b);
#endif
    }
    void set(byte cr, byte cg, byte cb) {
      r = cr; g = cg; b = cb;
#ifdef __X11__
      _data = NULL;
#endif
#ifdef __WIN32__
      colorref = RGB(r, g, b);
#endif
    }
    void set(EColor16);
    void set(ESystemColor);

    TColor& operator =(const TColor &c) {
      r = c.r;
      g = c.g;
      b = c.b;
#ifdef __X11__
      _data = c._data;
#endif
#ifdef __WIN32__
      colorref = RGB(r, g, b);
#endif
      return *this;
    }

    TColor& operator =(const TRGB &c) {
      r = c.r;
      g = c.g;
      b = c.b;
#ifdef __X11__
      _data = NULL;
#endif
#ifdef __WIN32__
      colorref = RGB(r, g, b);
#endif
      return *this;
    }


  private:
    void _Init();
    static const TColor& _palette(int i);

#ifdef __X11__
    // for pen, window and bitmaps
    //-----------------------------
    void _setPen(TPen*, _TOAD_GC &gc);
    static ulong _getPixel(const TRGB&);
    ulong _getPixel() { return _getPixel(*this); }
    static ulong _getPixelAt(const TRGB&, int x,int y);
    ulong _getPixelAt(int x, int y) { return _getPixelAt(*this,x,y); }
    
    static bool _shouldNotDither();
    typedef GSmartPointer<TData> PData;
    PData _data;
#endif

#ifdef __WIN32__
    public:
    COLORREF colorref;
#endif


};

} // namespace toad

bool restore(atv::TInObjectStream &p, toad::TSerializableRGB *value);
// bool restore(TInObjectStream &p, const char *name, TRGB **value);


#endif
