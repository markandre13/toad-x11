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

typedef unsigned char byte;
typedef unsigned long ulong;

#ifdef __X11__

static void determine_color_mode();

static void complete_palette125();
static void create_translation_table125();
static bool allocate_exact_colors();
static void allocate_fuzzy_colors();

void dither16_8x8(byte r, byte g, byte b, Pixmap pm);
void dither27_8x8(byte r, byte g, byte b, Pixmap pm);
void dither125_8x8(byte r, byte g, byte b, Pixmap pm);

static unsigned long get_pixel(byte r, byte g, byte b);
static void dither(byte r, byte g, byte b, Pixmap pm);
static unsigned long dither_pixel(byte r, byte g, byte b, int x, int y);

struct TDirectPixel
{
  ulong red_base, green_base, blue_base;
  ulong red_max, green_max, blue_max;
};

static TDirectPixel direct_pixel;

static ulong color_mode;
static const ulong _DITHER = 1; // use dithering
static const ulong _DIRECT = 2; // calculate pixel values with TDirectPixel

bool TColor::_shouldNotDither()
{
  return !(color_mode & _DITHER);
}

Colormap toad::x11colormap;
static unsigned long pixel[RGB_PALETTE_MAX];
static int nPaletteSize;
static Pixmap empty_pm = 0;

// InitColor
//---------------------------------------------------------------------------
void TOADBase::initColor()
{
  determine_color_mode();
  
  if (color_mode & _DITHER) {
    complete_palette125();
    create_translation_table125();
    if (!allocate_exact_colors())
      allocate_fuzzy_colors();
  }
}

static unsigned color_limit = 126;

void TOADBase::setColorLimit(unsigned n)
{
  if (n<=16)
    n=16;
  else if (n<=28)
    n=28;
  else if (n<=126)
    n=126;

  color_limit = n;
}

// _SetPen
//---------------------------------------------------------------------------
TColor::TData::TData()
{
  pixel_is_valid = false;
  pm = 0;
}

TColor::TData::~TData()
{
  if (pm) {
    if (!empty_pm)
      empty_pm = pm;
    else
      XFreePixmap(x11display, pm);
  }
}


void
TColor::_setPen(TPen *pen, _TOAD_GC &gc)
{
  if (!gc) {
    gc = XCreateGC(x11display, pen->x11drawable,0,0);
    if (pen->region)
      XSetRegion(x11display, gc, pen->region->x11region);
  }
  if (_data==NULL)
    _data = new TData();
  if ( !(color_mode&_DITHER) ) {
    XSetForeground(x11display, gc, _getPixel(*this));
    return;
  }
  switch(pen->cmode) {
    case TColor::NEAREST:
      XSetFillStyle(x11display, gc, FillSolid);
      XSetForeground(x11display, gc, get_pixel(r,g,b));
      break;
      
      //+-------------------------------------------------+
      //| dither (r,g,b) with the system colors in pixmap |
      //+-------------------------------------------------+
    case TColor::DITHER16:
    case TColor::DITHER27:
    case TColor::DITHER125:
      if (!_data->pm) {
        if (empty_pm) {
          _data->pm=empty_pm;
          empty_pm=0;
        } else {
          _data->pm = XCreatePixmap(x11display, 
                             RootWindow(x11display, x11screen),
                             8,8, 
                             DefaultDepth(x11display, x11screen) );
        }
      }
      dither(r,g,b,_data->pm);
      XSetTile(x11display, gc, _data->pm);
      XSetFillStyle(x11display, gc, FillTiled);
      break;
  }
}

ulong
TColor::_getPixel(const TRGB &rgb)
{
  if (color_mode & _DIRECT) {
    return
      ((rgb.r>>direct_pixel.red_max)<<direct_pixel.red_base) |
      ((rgb.g>>direct_pixel.green_max)<<direct_pixel.green_base) |
      ((rgb.b>>direct_pixel.blue_max)<<direct_pixel.blue_base);
  }
  return get_pixel(rgb.r, rgb.g, rgb.b);
}

ulong TColor::_getPixelAt(const TRGB &rgb, int x, int y)
{
  if (color_mode & _DIRECT) {
    return
      ((rgb.r>>direct_pixel.red_max)<<direct_pixel.red_base) |
      ((rgb.g>>direct_pixel.green_max)<<direct_pixel.green_base) |
      ((rgb.b>>direct_pixel.blue_max)<<direct_pixel.blue_base);
  }
  if ( !(color_mode & _DITHER) )
    return _getPixel(rgb);
  else
    return dither_pixel(rgb.r,rgb.g,rgb.b, x,y);
}

#endif

/****************************************************************************
 *                                                                          *
 *                             Data Structures                              *
 *                                                                          *
 ****************************************************************************/

//  `palette' contains all colors TOAD will try to allocate when the display
// is something like an 8 bit indexed visual.
//  The first 16 colors are the well known colors of the IBM PC CGA and are
// compatible with "A Standard Default color Space for the Internet" as
// mentioned in HTML 4.0.
//  The colors up to 27 define a 2:2:2 RGB space and the colors up to
// 125 a 5:5:5 RGB space.
//  All color palettes 16,28 and 126 can be used for dithering.
//---------------------------------------------------------------------------
static TColor palette[RGB_PALETTE_MAX]=
{                       // IBM PC CGA     Amstrad CPC   HTML
  TColor(  0,  0,  0),  //  0 black       0             Black
  TColor(128,  0,  0),  //  1 red         3             Maroon
  TColor(  0,128,  0),  //  2 green       9             Green
  TColor(128,128,  0),  //  3 yellow      12            Olive
  TColor(  0,  0,128),  //  4 blue        1             Navy
  TColor(128,  0,128),  //  5 violett     4             Purple
  TColor(  0,128,128),  //  6 cyan        10            Teal
  TColor(128,128,128),  //  7 gray        13            Gray
  TColor(192,192,192),  //  8 lightgray                 Silver
  TColor(255,  0,  0),  //  9 lightred    6             Red
  TColor(  0,255,  0),  // 10 lightgreen  18            Lime
  TColor(255,255,  0),  // 11 yellow      24            Yellow
  TColor(  0,  0,255),  // 12 lightblue   2             Blue
  TColor(255,  0,255),  // 13 magenta     8             Fuchsia
  TColor(  0,255,255),  // 14 lightcyan   20            Aqua
  TColor(255,255,255),  // 15 white       26            White
  
  TColor(128,  0,255),  // 16             5
  TColor(255,  0,128),  // 17             7
  TColor(  0,128,255),  // 18             11
  TColor(128,128,255),  // 19             14
  TColor(255,128,  0),  // 20             15
  TColor(255,128,128),  // 21             16
  TColor(255,128,255),  // 22             17
  TColor(  0,255,128),  // 23             19
  TColor(128,255,  0),  // 24             21
  TColor(128,255,128),  // 25             22
  TColor(128,255,255),  // 26             23
  TColor(255,255,128)   // 27             25
  
  // ...
};

const TColor& TColor::_palette(int n)
{
  return palette[n];
}

#ifdef __X11__

// color translation tables
//---------------------------------------------------------------------------
// the `palette' above is not very efficient to access, so the following
// translations tables are used to speed things up

// color translation table for the 28 color mode
int order27[27]=
{
  0,4,12,1,5,16,9,17,13,2,6,18,3,7,19,20,21,17,10,23,14,24,25,26,11,27,15
};

// color translation table for the 126 color mode
int order125[125];

// dither tables
//---------------------------------------------------------------------------
int dither_m4x4[16]=
{ 
   0, 8, 2,10,
  12, 4,14, 6,
   3,11, 1, 9,
  15, 7,13, 5
};

int dither_m8x8[64]=
{ 
  0,32, 8,40, 3,35,11,43,
 48,16,56,24,51,19,59,27,
 12,44, 4,36,15,47, 7,39,
 60,28,52,20,63,31,55,23,
 2,34,10,42, 1,33, 9,41,
 50,18,58,26,49,17,57,25,
 14,46, 6,38,13,45, 5,37,
 62,30,54,22,61,29,53,21
};


/****************************************************************************
 *                                                                          *
 *                                Functions                                 *
 *                                                                          *
 ****************************************************************************/

// add the missing entries to the color palette (28 to 125)
//---------------------------------------------------------------------------
void complete_palette125()
{
  int p=28;
  for(int g=0; g<=256; g+=64) {
    for(int r=0; r<=256; r+=64) {
      for(int b=0; b<=256; b+=64) {
        if (r==64 || r==192 || g==64 || g==192 || b==64 || b==192) {
          if (r==256) r=255;
          if (g==256) g=255;
          if (b==256) b=255;
          palette[p++].set(r,g,b);
        }
      }
    }
  }
}

// create the translation table for the 126 color mode
//---------------------------------------------------------------------------
void create_translation_table125()
{
  int p=0;
  int p1=0;
  int p2=28;
  for(int g=0; g<=100; g+=25) {
    for(int r=0; r<=20; r+=5) {
      for(int b=0; b<=4; b+=1) {
        if (b==1 || b==3 || r==5 || r==15 || g==25 || g==75) {
          order125[p]=p2;
          p2++;
        } else {
          order125[p]=order27[p1];
          p1++;
        }
        p++;
      }
    }
  }
}

// allocate colors for indexed color mode
//---------------------------------------------------------------------------
bool allocate_exact_colors()
{
  nPaletteSize = color_limit;

  XColor xcolor;
  int i;
  for(i=0; i<nPaletteSize; i++) {
    xcolor.red  = palette[i].r + (palette[i].r<<8);
    xcolor.green= palette[i].g + (palette[i].g<<8);
    xcolor.blue = palette[i].b + (palette[i].b<<8);
    if (XAllocColor(x11display, x11colormap, &xcolor)==0)
      break;
    pixel[i] = xcolor.pixel;
  }
  if (i<=125) {
    if (i<=27) {
      if (i<=15) {
        DBM(printf("toad: less than 16 colors available\n");)
        nPaletteSize=i;
        return false;
      } else {
        nPaletteSize=16;
        XFreeColors(x11display, x11colormap, pixel+16,i-16+1,0);
      }
    } else {
#if 0
      // use 28 colors
      nPaletteSize=28;
      XFreeColors(x11display, x11colormap, pixel+28,i-28+1,0);
#else
      DBM(printf("toad: %d colors available\n", i);)
      nPaletteSize=i;
      return false;
#endif
    }
  }
  DBM(printf("toad: got %i colors\n", nPaletteSize);)
  return true;
}

// determine `color_mode' and set `color_map'
//---------------------------------------------------------------------------

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
      cerr << "toad: gray scale visuals aren't supported yet" << endl;
      color_mode = _DITHER;
      break;
    case PseudoColor:
    case DirectColor:
      color_mode = _DITHER;
      break;
    case StaticColor:
    case TrueColor:
      pixel_helper(visual->red_mask, direct_pixel.red_base, direct_pixel.red_max);
      pixel_helper(visual->green_mask, direct_pixel.green_base, direct_pixel.green_max);
      pixel_helper(visual->blue_mask, direct_pixel.blue_base, direct_pixel.blue_max);
      color_mode = _DIRECT;
      break;
  }

  // The initial values of the colormap entries are undefined for the visual
  // classes GrayScale, PseudoColor, and DirectColor!

//  PrintVisualInfo(visual);
}

/*****************************************************************************
 *                                                                           *
 * Smart PseudoColor Color Allocation                                        *
 *                                                                           *
 *****************************************************************************/
struct CStat
{
  int cmindex;  // index in color map
  int pindex;   // index in palette
  double dist;  // distance
};

static void MakeStat(CStat stat[], int nPaletteSize, XColor ctable[], bool public_color[], int n);
static void SortStat(CStat stat[], int nPaletteSize);

/**
 * Allocate colors. When there aren't enough colors, try to find similar
 * colors in the current color palette.
 */
void
allocate_fuzzy_colors()
{
  // get list of public color cells
  //--------------------------------
  int i;
  XColor xcolor;
  nPaletteSize = 126;
  
  // get a list of all current colors
  //----------------------------------
  Visual *v = DefaultVisual(x11display, x11screen);
  
  XColor *ctable = new XColor[v->map_entries];
  for (i=0; i<v->map_entries; i++)
    ctable[i].pixel = i;
  XQueryColors(x11display, x11colormap, ctable, v->map_entries);

  // get a list of all public colors
  //---------------------------------
  ulong tmp_pixel[v->map_entries];
  bool public_color[v->map_entries];
  int tmp_npixel=0;
  for (i=0; i<v->map_entries; i++) {
    xcolor = ctable[i];
    if (XAllocColor(x11display, x11colormap, &xcolor)==0) {
      public_color[i]=false;
    } else {
      public_color[i]=true;
      tmp_pixel[tmp_npixel++]=xcolor.pixel;
    }
  }
  XFreeColors(x11display, x11colormap, tmp_pixel, tmp_npixel, 0);
  
  // find the best matches
  //-----------------------
  CStat stat[nPaletteSize];
  MakeStat(stat,nPaletteSize, ctable,public_color,v->map_entries);

  // free colors this application has allocated before
  //---------------------------------------------------
  //  XFreeColors(x11display, x11colormap,pixel,nAllocatedColors-1,0);

  // some color are free now, allocate the worst matches
  //-----------------------------------------------------
  bool bAllocated[nPaletteSize];
  for(i=0; i<nPaletteSize; i++)
    bAllocated[i]=false;

  SortStat(stat,nPaletteSize);
  for(i=0; i<nPaletteSize; i++) {
    int k = stat[i].pindex;
    xcolor.red  = palette[k].r + (palette[k].r<<8);
    xcolor.green= palette[k].g + (palette[k].g<<8);
    xcolor.blue = palette[k].b + (palette[k].b<<8);
    if (XAllocColor(x11display, x11colormap, &xcolor)==0)
      break;
    pixel[stat[i].pindex] = xcolor.pixel;
    bAllocated[stat[i].pindex] = true;
  }
            
  // colormap has been modified, redo the statistic
  //------------------------------------------------
  MakeStat(stat,nPaletteSize, ctable,public_color,v->map_entries);
      
  // allocate the rest
  //-------------------
  for(i=0; i<nPaletteSize; i++) {
    if (bAllocated[i])
      continue;
    xcolor.red  = ctable[stat[i].cmindex].red;
    xcolor.green= ctable[stat[i].cmindex].green;
    xcolor.blue = ctable[stat[i].cmindex].blue;
    if (XAllocColor(x11display, x11colormap, &xcolor)==0) {
      cerr << "toad: couldn't allocate existing read-only color" << endl;
    }
    pixel[stat[i].pindex] = xcolor.pixel;
    bAllocated[stat[i].pindex] = true;
  }
  
  delete ctable;
}

void MakeStat(CStat stat[],
              int nPaletteSize,
              XColor ctable[],
              bool public_color[],
              int n)
{
  int i,j;
  for(i=0; i<nPaletteSize; i++) {
    stat[i].dist=-1.0;
    stat[i].pindex = i;
  }

  for(i=0; i<nPaletteSize; i++) {
    double pr,pg,pb, r,g,b;
    double md; int mi;
    pr = palette[i].r;
    pg = palette[i].g;
    pb = palette[i].b;
    
    for(j=0; j<n; j++) {
      r = (pr-ctable[j].red/256.0)  ;
      g = (pg-ctable[j].green/256.0);
      b = (pb-ctable[j].blue/256.0) ;
      double d = r*r+g*g+b*b;
      if (j==0 || d<md) {
        md=d;
        mi=j;
      }
    }
    stat[i].dist = md;
    stat[i].cmindex = mi;
  }
}     

void SortStat(CStat stat[], int nPaletteSize)
{
  int i,j;
  CStat akku;

  for(i=nPaletteSize-1; i>0; i--) {
    for(j=0; j<i; j++) {
      if (stat[j].dist<stat[j+1].dist) {
        akku = stat[j];
        stat[j] = stat[j+1];
        stat[j+1] = akku;
      }
    }
  }
}

/*****************************************************************************
 *                                                                           *
 * 16 Color Dithering                                                        *
 *                                                                           *
 *****************************************************************************/

struct _rgb
{
  unsigned char r,g,b;
} rgb[64];

static void 
reduce(byte &r, byte &ir)
{
  if (r==255) {
    r=64;
    ir=2;
  } else {
    r++;
    ir = (r &0x80) ? 2 : 1;
    r &= 64+32+16+8+4;
    r>>=1;
  }
  if (r==0 && ir==2) {
    r=64; ir=1;
  }
}

static void 
setrgb(int i, char f, byte v)
{
  switch(f) {
    case 'r':
      rgb[i].r=v;
      break;
    case 'g':
      rgb[i].g=v;
      break;
    case 'b':
      rgb[i].b=v;
      break;
  }
}

static void 
fill(byte &r, byte &ir, byte &i2max, char f)
{
  int i,j;

  if (ir==1) {
    for(i=0; i<64; i++)
      setrgb(i,f,0);
    i=i2max;
    while(i<64 && r>0) {
      setrgb(i,f,1);
      r--;
      i++;
    }
    if(r>0) {
      r>>=1;
      for(i=0; i<r; i++)
        setrgb(i,f,2);
    }
  }

  if (ir==2) {
    for(i=0; i<r; i++)
      setrgb(i,f,2);
    if (r<i2max) {
      for(j=(i2max-r)>>1;j>0;j--) {
        setrgb(i,f,2);
        i++;
      }
      while(i<i2max) {
        setrgb(i,f,0);
        i++;
      }
    }
    for(;i<64; i++)
      setrgb(i,f,1);
  }
}

void 
dither16_8x8(byte r, byte g, byte b, Pixmap pm)
{
  static int Dithertable[64]=
  {
     0,36,32, 4,18,54,50,22,
     2,38,34, 6,16,52,48,20,
     9,45,41,13,27,63,59,31,
    11,47,43,15,25,61,57,29,
     1,37,33, 5,19,55,51,23,
     3,39,35, 7,17,53,49,21,
     8,44,40,12,26,62,58,30,
    10,46,42,14,24,60,56,28
  };

  byte m[64];

  int i,j;

  // reduce colors r,g,b and set intensity values ir,ig,ib
  // each (color,intensity) pair goes through:
  // (0,1),(2,1),(4,1),..,(64,1),(2,2),..,(64,2)
  byte ir,ig,ib;
  reduce(r,ir);
  reduce(g,ig);
  reduce(b,ib);

  // set i2max to maximal color value with intensity 2
  byte i2max=0;
  if (ir==2) i2max=r;
  if (ig==2 && i2max<g) i2max=g;
  if (ib==2 && i2max<b) i2max=b;

  fill(r,ir,i2max,'r');
  fill(g,ig,i2max,'g');
  fill(b,ib,i2max,'b');

  // convert rgb table to colortable
  byte pen;
  for(i=0; i<64; i++) {
    switch(rgb[i].r) {
      case 0:
        switch(rgb[i].g) {
          case 0:
            switch(rgb[i].b) {
              case 0:   // 000
                pen= 0; break;
              case 1:   // 001
                pen= 4; break;
              case 2:   // 002
                pen=12; break;
            }
            break;
          case 1:
            switch(rgb[i].b) {
              case 0:   // 010
                pen= 2; break;
              case 1:   // 011
                pen= 6; break;
            }
            break;
          case 2:
            switch(rgb[i].b) {
              case 0:   // 020
                pen=10; break;
              case 2:   // 022
                pen=14; break;
            }
            break;
        }
        break;
      case 1:
        switch(rgb[i].g) {
          case 0:
            switch(rgb[i].b) {
              case 0:   // 100
                pen= 1; break;
              case 1:   // 101
                pen=5; break;
            }
            break;
          case 1:
            switch(rgb[i].b) {
              case 0:   // 110
                pen= 3; break;
              case 1:   // 111
                pen= 7; break;
            }
            break;
        }
        break;
      case 2:
        switch(rgb[i].g)
        {
          case 0:
            switch(rgb[i].b) {
              case 0:   // 200
                pen= 9; break;
              case 2:   // 202
                pen=13; break;
            }
            break;
          case 2:
            switch(rgb[i].b) {
              case 0:   // 220
                pen=11; break;
              case 2:   // 222
                pen=15; break;
            }
            break;
        }
        break;
    }
    m[Dithertable[i]]=pen;
  }

  // the following code makes dithering really slow and even images won't help
  {
    GC ink=XCreateGC(x11display,DefaultRootWindow(x11display),0,0);
    int p=0;
    for(i=0; i<8; i++) {
      for(j=0; j<8; j++) {
        XSetForeground(x11display, ink, pixel[m[p++]]);
        XDrawPoint(x11display, pm, ink, j,7-i);
      }
    }
    XFreeGC(x11display,ink);
  }
}

/*****************************************************************************
 *                                                                           *
 * 27 Color Dithering                                                        *
 *                                                                           *
 *****************************************************************************/
void 
dither27_8x8(byte r, byte g, byte b, Pixmap pm)
{
  int i,j;

  unsigned m[64];
  memset(m,0,sizeof(m));
    
  int c;
  
  // red
  c=(r+1)/2;
  j=0;
  for(i=1; i<=c; i++) {
    m[j]+=3;
    j++;
    if (j>=64) j=0;   // j=(j+1)%64;
  }

  // green
  c=(g+1)/2;
  j=0;
  for(i=1; i<=c; i++) {
    m[j]+=9;
    j++;
    if (j>=64) j=0;
  }

  // blue
  c=(b+1)/2;
  j=0;
  for(i=1; i<=c; i++) {
    m[j]+=1;
    j++;
    if (j>=64) j=0;
  }

  // the following code makes dithering really slow and even images won't help
  GC ink=XCreateGC(x11display, DefaultRootWindow(x11display),0,0);
  int p=0;
  for(i=0; i<8; i++) {
    for(j=0; j<8; j++) {
      XSetForeground(x11display, ink, pixel[order27[m[dither_m8x8[p++]]]]);
      XDrawPoint(x11display, pm, ink, j,7-i);
    }
  }
  XFreeGC(x11display, ink);
}


/*****************************************************************************
 *                                                                           *
 * 125 Color Dithering                                                       *
 *                                                                           *
 *****************************************************************************/
void 
dither125_8x8(byte r, byte g, byte b, Pixmap pm)
{
  int i,j;

#if 0
  unsigned m[64];
  
  // clear memory
  for(i=0;i<64;i++)
    m[i]=0;
    
  int c;
  
  // red
  c = r==255?256:r;
  j=0;
  for(i=1; i<=c; i++)
  {
    m[j]+=5;
    j++;
    if (j>=64) j=0;
  }

  // green
  c = g==255?256:g;
  j=0;
  for(i=1; i<=c; i++)
  {
    m[j]+=25;
    j++;
    if (j>=64) j=0;
  }

  // blue
  c = b==255?256:b;
  j=0;
  for(i=1; i<=c; i++)
  {
    m[j]+=1;
    j++;
    if (j>=64) j=0;
  }
#endif
  // the following code makes dithering really slow and even images won't help
  GC ink=XCreateGC(x11display,DefaultRootWindow(x11display),0,0);
  for(i=0; i<8; i++) {
    for(j=0; j<8; j++) {
      XSetForeground(x11display, ink, dither_pixel(r,g,b,j,i));
      XDrawPoint(x11display, pm, ink, j,7-i);
    }
  }
  XFreeGC(x11display, ink);
}

/*****************************************************************************
 *                                                                           *
 * Mid-Level Dithering Functions                                             *
 *                                                                           *
 *****************************************************************************/

// dither color to pixmap
//---------------------------------------------------------------------------
void 
dither(byte r, byte g, byte b, Pixmap pm)
{
  if (nPaletteSize<28) {
    dither16_8x8(r,g,b,pm);
    return;
  }
  if (nPaletteSize<126)
  {
    dither27_8x8(r,g,b,pm);
    return;
  }
  dither125_8x8(r,g,b,pm);
}

// get the nearest system color to (r,g,b)
//---------------------------------------------------------------------------
unsigned long 
get_pixel(byte r, byte g, byte b)
{
  if (nPaletteSize==126) {
    return pixel[order125[
      (((int)r+32)>>6)*5+
      (((int)g+32)>>6)*25+
      (((int)b+32)>>6)
    ]];
  }

  int nearest = 0;
  double dr,dg,db, d1,d2;
  d2=256.0*256.0*257.0;
  for(int i=0; i<nPaletteSize; i++) {
    dr = 0.30 * (static_cast<double>(palette[i].r)-static_cast<double>(r));
    dg = 0.59 * (static_cast<double>(palette[i].g)-static_cast<double>(g));
    db = 0.11 * (static_cast<double>(palette[i].b)-static_cast<double>(b));
    d1 = dr*dr + dg*dg + db*db;
    if (d1<d2) {
      d2=d1;
      nearest = i;
    }
  }
  
  return pixel[nearest];
}


// get pixel color with dithering
//---------------------------------------------------------------------------
unsigned long 
dither_pixel(byte r, byte g, byte b, int x, int y)
{
  // make sure x and y are positive
  //--------------------------------
  while(x<0) x+=256;
  while(y<0) y+=256;

  int p=dither_m8x8[(x&7)+((y&7)<<3)];
  int c = 0;
  switch(nPaletteSize) {
    case 28:
      if (b) c += (b+128-p)>>7;
      if (r) c += ((r+128-p)>>7)*3;
      if (g) c += ((g+128-p)>>7)*9;
      return pixel[order27[c]];
    case 126:
      if (b) c += (b+64-p)>>6;
      if (r) c += ((r+64-p)>>6)*5;
      if (g) c += ((g+64-p)>>6)*25;
      return pixel[order125[c]];
      break;
    default: // doing this with 16 colors is complicated => not implemented
      return get_pixel(r,g,b);
  }
}

#endif

} // namespace toad

