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

/*
 *
 *   FYI: THE TBITMAP CLASS WILL BE REWRITTEN IN THE FUTURE
 *
 */

#ifndef TBitmap
#define TBitmap TBitmap

#include <toad/pointer.hh>

namespace toad {

class TPen;
class TBitmapFilter;
class TFileDialog;

enum EBitmapType
{
  TBITMAP_INDEXED,
  TBITMAP_TRUECOLOR,
  TBITMAP_SERVER
};

enum EBitmapDither
{ 
  TBITMAP_SUBSTITUTE = 0, 
  TBITMAP_ORDERED,
  TBITMAP_FLOYD_STEINBERG
};

// beware, this class is pure alpha code:
class TBitmapMask
{
  public:
    TBitmapMask(int w, int h);
    ~TBitmapMask();
    
    void clearAll();
    void setPixel(int,int,bool);
    bool getPixel(int,int) const;
    
    int _w,_h;
    byte *data;
};

class TBitmap: 
  public TOADBase, public TSmartObject
{
  friend class TPen;
  friend class TWindow;

  protected:
    enum EBitmapMode
    {
      TBITMAP_SHOW,
      TBITMAP_EDIT
    };

    int zoom;

  public:
    TBitmap();
    TBitmap(int w,int h, EBitmapType type=TBITMAP_TRUECOLOR);
    virtual ~TBitmap();
    
    static bool addFilter(TBitmapFilter*);

    // these two methods are located in 'filedialog.cc'
    //--------------------------------------------------
    static bool getInputFilter(TFileDialog&);
    static bool getOutputFilter(TFileDialog&);

    static void open();
    static void close();
    
    void setDither(EBitmapDither);
    void setZoom(int z);
    void setPixel(int x,int y,short r,short g,short b);
    bool getPixel(int x,int y,short *r,short *g,short *b);
    bool getPixel(int x,int y,TRGB*);
    void load(const string &url);
    void load(istream&);
    bool save(const string &url, void* xtra=NULL);
    bool save(ostream&, void *xtra=NULL);
    void update();

    void drawBitmap(const TPen*,int,int,int,int,int,int,int) const;
    void drawBitmap(const TPen*,int,int) const;
    void drawBitmap(const TPen*,int,int, int,int,int,int) const;
    int width,height;
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // these two methods a pure alpha stuff:
    void clearMask();
    void setMask(const TBitmapMask&);

  protected:
    EBitmapMode mode;
    EBitmapDither dither;
    unsigned long pixmap;   // server side pixmap
    unsigned long mask;     // server side mask
    int pix_width, pix_height;
    void pCopyToLine(int x1,int x2,int y,int *line);
    void copy_bitmap_to_pixmap_and_delete_it();

    TRGB *color;        // color table
    unsigned char *index; // NULL for true color
    TRGB& pGetColor(int,int);
    TRGB& pGetColor(int);
    
    bool modified;
};

typedef GSmartPointer<TBitmap> PBitmap;

class TAlterBitmap: public TBitmap
{
  friend class TPen;
  friend class TWindow;

  public:
    TAlterBitmap()
    {
      mode = TBITMAP_EDIT;
    }
    
    TAlterBitmap(int w,int h, EBitmapType type=TBITMAP_TRUECOLOR)
    :TBitmap(w,h,type)
    {
      mode = TBITMAP_EDIT;
    }
};

} // namespace toad

#endif
