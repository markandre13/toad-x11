/*
 * TPaint -- a simple bitmap editor
 * Copyright (C) 1996-2002 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <toad/toad.hh>
#include <toad/dnd/color.hh>
#include <toad/dialog.hh>
#include <toad/scrollbar.hh>
#include <toad/pushbutton.hh>

#include <toad/scrolledarea.hh>

#include <toad/io/urlstream.hh>
#include <toad/io/binstream.hh>
#include <fstream>

using namespace toad;

#include "Palette.hh"

extern TColor color[1];

//. While TRect is defined as
//.   x,y   : position
//.   w,h   : size
//. TArea is defined as
//.   x1,y1 : upper left position
//.   x2,y2 : lower right position
struct TArea
{
  int y1,y2,x1,x2;
};

class TColorTable: 
  public TScrolledArea
{
    static const int zoom=12; // size of a color field
    static const int w=8;     // number of color fields horizontal
    static const int h=16;    // number of color fields vertical

  public:
    TColorTable(TWindow *p, const string &t);
    
    TColor* table;      // the color table
    int table_size;     // number of entries in the table

    // sigColor is triggered, when the user selects a color
    // with the left pointer button; 'color' contains the
    // selected color
    //-----------------------------------------------------
    TColor color;
    TSignal sigColor;
    
  protected:
    void paint();
    void create();
    
    // select a color and trigger sigColor
    //-------------------------------------
    void mouseLDown(int,int,unsigned);

    // methods for drag and drop
    //----------------------------------------
    void mouseMDown(int,int,unsigned);
    void dropColor(PDnDColor);

    // the new TScrolledArea service methods?
    //----------------------------------------
    void window2Area(int sx,int sy,int *dx,int *dy);
    void window2Area(const TRectangle &in,TArea *out);
    void window2Window(int sx,int sy,int *dx,int *dy);
};

TColorTable::TColorTable(TWindow *p, const string &t):TScrolledArea(p,t)
{
  // the whole window is filled by paint, so theirs no need
  // wasting time painting a background
  //--------------------------------------------------------
  bNoBackground = true;
  
  TDropSiteColor *ds = new TDropSiteColor(this);
  CONNECT_VALUE(ds->sigDrop, this, dropColor, ds);
}

void TColorTable::create()
{
  // configure the TScrolledArea properties of the window
  //------------------------------------------------------
  bAlwaysVertical = true;
  setItemSize(zoom,zoom);
  setAreaSize(8,32);
  setVisibleAreaPos(0,0);
  setVisibleAreaSize(8,16);
}

void TColorTable::paint()
{
  TPen pen(this);
  pen.setColorMode(TColor::DITHER);

  TRectangle r;
  pen.getClipBox(&r);
  
  TArea a;
  int sx,sy;

  window2Area(r,&a);
  window2Window(r.x,r.y,&sx,&sy);

  int ndx = a.y1*area_w;

  for(int y=a.y1; y<=a.y2; y++) {
    for(int x=0; x<area_w; x++) {
      assert(ndx < table_size);
      pen.setColor(table[ndx]);
      pen.fillRectangle(
        (x-area_x)*item_w,
        sy,
        item_w,item_h);
      ndx++;
      if (ndx>=table_size)
        return;
    }
    sy+=item_h;
  }
}

void TColorTable::mouseLDown(int x,int y,unsigned)
{
  window2Area(x,y,&x,&y);
  
  int ndx = x+y*area_w;
  if (ndx>=table_size)
    return;
  color=table[ndx];
  sigColor();
}

void TColorTable::window2Area(int sx,int sy,int *dx,int *dy)
{
  *dx=(sx/item_w)+area_x;
  *dy=(sy/item_h)+area_y;
}

void TColorTable::window2Area(const TRectangle &in,TArea *out)
{
  out->y1 = in.y / item_h + area_y;
  out->y2 = (in.y+in.h) / item_h + area_y;
  out->x1 = in.x / item_w + area_x;
  out->x2 = (in.x+in.w) / item_w + area_x;
}

//. Translate window coordinate into upper left corner of the
//. corresponding area item
void TColorTable::window2Window(int sx,int sy,int *dx,int *dy)
{
  *dx=sx-(sx%item_w);
  *dy=sy-(sy%item_h);
}

// Drag & Drop
//-------------------------------------------------------------------
void TColorTable::mouseMDown(int x,int y, unsigned m)
{
  // Coord2Item
  x=(x/item_w)+area_x;
  y=(y/item_h)+area_y;
  
  int ndx = x+y*area_w;
  if (ndx>=table_size)
    return;
  
  startDrag(new TDnDColor(table[ndx]), m);
}

void TColorTable::dropColor(PDnDColor drop)
{
  // Coord2Item
#if 0
  printf("drop (%i,%i,%i) at %i,%i\n",
    drop->rgb.r, drop->rgb.g, drop->rgb.b,
    drop->x,drop->y);
#endif
  int x=(drop->x/item_w)+area_x;
  int y=(drop->y/item_h)+area_y;

  int ndx = x+y*area_w;
  if (ndx>=table_size)
    return;
  table[ndx].set(drop->rgb.r, drop->rgb.g, drop->rgb.b);
  invalidateWindow();
}

// TPalette
//-------------------------------------------------------------------
TPalette::TPalette(TWindow *p,const string &t)
  :super(p,t)
{
  setBackground(TColor::DIALOG);
  bFocusManager = true;
  setSize(150,250);

  TColorTable *ct = new TColorTable(this,"color table");
    palette = ct->table = new TColor[256];
    ct->table_size = 256;
    ct->setPosition(30,10);
    CONNECT(ct->sigColor, this, actColor, ct);
    
  TPushButton *pb;
  
  pb=new TPushButton(this,"Save");
    pb->setShape(10,220,50,20);
    CONNECT(pb->sigClicked, this, savePalette);
  pb=new TPushButton(this,"Load");
    pb->setShape(70,220,50,20);
    CONNECT(pb->sigClicked, this, loadPalette);
  
  loadPalette();
}

TPalette::~TPalette()
{
  if (palette)
    delete[] palette;
}

void TPalette::paint()
{
  TPen pen(this);
  pen.setColorMode(TColor::DITHER);
  pen.setColor(::color[0]);
  pen.fillRectangle(10,10,10,10);
}

void TPalette::actColor(TColorTable *ct)
{
  ::color[0]=ct->color;
  TPen pen(this);
  pen.setColorMode(TColor::DITHER);
  pen.setColor(::color[0]);
  pen.fillRectangle(10,10,10,10);
}

void TPalette::loadPalette()
{
  try {
    unsigned char c;
    unsigned long entries;
    string fn = TOADBase::getExecutablePath()+"standard.pal";
    iurlstream is;
    try {
      is.open(fn);
    } catch (exception &e) {
      is.open("memory://standard.pal");
    }
    TInBinStream in(&is);
    c = in.readByte();
    entries = in.readDWord();
      entries=256;
    TColor col;
    for(unsigned long i=0;i<entries;i++) {
      col.r = in.readByte();
      col.g = in.readByte();
      col.b = in.readByte();
      palette[i]=col;
    }
  } catch(exception &e) {
    messageBox(this, "Warning",
                     "Failed to load the color palette",
                     TMessageBox::OK | TMessageBox::ICON_INFORMATION);
  }
  if (getFirstChild()) {
    TWindow *w = dynamic_cast<TWindow*>(getFirstChild());
    if (w)
      w->invalidateWindow();
  }
}

void TPalette::savePalette()
{
  ofstream os("standard.pal", ofstream::out|ofstream::trunc /*, 0644 */);
  TOutBinStream out(&os);
  out.writeByte(1);         // version
  out.writeDWord(256);      // number of entries
  for(int i=0; i<256; i++) {
    out.writeByte(palette[i].r);
    out.writeByte(palette[i].g);
    out.writeByte(palette[i].b);
  }
}
