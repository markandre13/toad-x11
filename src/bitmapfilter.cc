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

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/region.hh>
#include <toad/bitmapfilter.hh>

#include <map>

using namespace toad;

/**
 * \class toad::TBitmapFilter
 * The base class for the PNG, BMP, JPEG and other bitmap import/export
 * filters.
 * <P>
 * When you don't want to write or modify a bitmap filter you won't even
 * have to think about this class.
 */
TBitmapFilter::TBitmapFilter()
{
  color = NULL;
  index = NULL;
  next = NULL;
  mycolor=myindex=false;
}

bool TBitmapFilter::isIndex()
{
  return index!=NULL;
}

void TBitmapFilter::createBuffer(int width,int height,EBitmapType type)
{
  #ifdef SECURE
  if (color || index) {
    printf("toad: warning: TBitmapFilter::createBuffer buffer already exists\n");
    if (color) delete color;
    if (index) delete index;
  }
  #endif
  w = width;
  h = height;
  switch(type) {
    case TBITMAP_INDEXED:
      color = new TRGB[256];
      index = new unsigned char[w*h];
//      cout << __FILE__ << ":" << __LINE__ << " color=" << color << endl;
//      cout << __FILE__ << ":" << __LINE__ << " index=" << index << endl;
      break;
    case TBITMAP_TRUECOLOR:
    default:
      color = new TRGB[w*h];
//      cout << __FILE__ << ":" << __LINE__ << " color=" << color << endl;
//      cout << __FILE__ << ":" << __LINE__ << " index=" << index << endl;
      index = NULL;
      break;
  }
  myindex=mycolor=true;
}

void TBitmapFilter::setBuffer(int width, int height, TRGB *c, unsigned char *i)
{
  w = width;
  h = height;
  color = c;
  index = i;
  myindex=mycolor=false;
}

void TBitmapFilter::getBuffer(int *width, int *height, TRGB **c, unsigned char **i)
{
  *width = w;
  *height = h;
  *c = color;
  *i = index;
  myindex=mycolor=false;
}

void TBitmapFilter::deleteBuffer()
{
  if (color && mycolor) {
//    cout << __FILE__ << ":" << __LINE__ << " delete color " << color << endl;
    delete[] color;
  }
  if (index && myindex) {
//    cout << __FILE__ << ":" << __LINE__ << " delete index " << index << endl;
    delete[] index;
  }
  color = NULL;
  index = NULL;
}

// methods for indexed mode
//----------------------------------------------------------------------------
/**
  Set palette color <VAR>index</VAR> to <VAR>c</VAR> when buffer is of
  type TBITMAP_INDEXED.
*/
void TBitmapFilter::setIndexColor(int i, TRGB &c)
{
  #ifdef SECURE
  if (!index) {
    printf("TBitmapFilter::setIndexColor: not an indexed bitmap\n");
    return;
  }
  if (i<0 || i>255) {
    printf("TBitmapFilter::setIndexColor: index is out of range\n");
    return;
  }
  #endif
  color[i]=c;
}

/**
  Set pixel at position (<VAR>x</VAR>,<VAR>y</VAR>) to palette entry
  <VAR>i</VAR>.
*/
void 
TBitmapFilter::setIndexPixel(int x,int y,int i)
{
  #ifdef SECURE
  if (!index) {
    printf("toad: TBitmapFilter::SetIndexPixel: not an indexed bitmap\n");
    return;
  }
  if (x<0 || x>=w || y<0 || y>=h) {
    printf("toad: TBitmapFilter::SetIndexPixel: coordinates out of range\n");
    return;
  }
  #endif
  index[x+y*w]=i;
}

int 
TBitmapFilter::getIndexPixel(int x,int y)
{
  #ifdef SECURE
  if (!index) {
    printf("toad: TBitmapFilter::GetIndexPixel: not an indexed bitmap\n");
    return 0;
  }
  if (x<0 || x>=w || y<0 || y>=h) {
    printf("toad: TBitmapFilter::GetIndexPixel: coordinates out of range\n");
    return 0;
  };
  #endif
  return index[x+y*w];
}

bool 
TBitmapFilter::getIndexColor(int i,TRGB *c)
{
  #ifdef SECURE
  if (!index) {
    printf("TBitmapFilter::GetIndexColor: not an indexed bitmap\n");
    return false;
  }
  if (i<0 || i>255) {
    printf("TBitmapFilter::GetIndexColor: index is out of range\n");
    return false;
  }
  #endif
  *c = color[i];
  return true;
}

// methods for true color mode
//----------------------------------------------------------------------------
void 
TBitmapFilter::setColorPixel(int x,int y,TRGB &c)
{
  if (index) {
    printf("toad: TBitmapFilter::SetColorPixel: not a truecolor bitmap\n");
    return;
  }
  if (x<0 || x>=w || y<0 || y>=h) return;
  color[x+y*w]=c;
}

bool 
TBitmapFilter::getColorPixel(int x,int y,TRGB *c)
{
  if (!color || x<0 || x>=w || y<0 || y>=h) return false;
  if (index) {
    // *c=color[index[x+y*w]];
    printf("toad: TBitmapFilter::GetColorPixel: not a truecolor bitmap\n");
  } else {
    *c=color[x+y*w];
  }
  return true;
}

/**
  Reduces a true color bitmap to an indexed bitmap. 
*/

struct TCompare
{
  bool operator()(const TRGB &a, const TRGB &b) const {
    return (a.r +(a.g<<8) + (a.b<<16)) < (b.r + (b.g<<8) + (b.b<<16));
  }
};
typedef map<TRGB,short,TCompare> TColorMap;

bool 
TBitmapFilter::convertToIndexed(int *palette_size)
{
  TColorMap cm;
  TColorMap::iterator p;
  TRGB c;

  if (index) {
    *palette_size = 256;
    return true;
  }

//  printf("ConvertToIndexed\n");

  unsigned char *nindex = new unsigned char[w*h];
  TRGB *ncolor = new TRGB[256];

  int x,y;
  unsigned last=0;
  unsigned short ndx;
  unsigned char *pp = nindex;
  for(y=0; y<h; y++) {
    for(x=0; x<w; x++) {
      getColorPixel(x,y,&c);
      p = cm.find(c);
      if (p==cm.end()) {
        if (cm.size()>=256) {
          setError("'%s' has more than 256 colors");
          delete ncolor;
          delete nindex;
//          printf("failed\n");
          return false;
        }
        cm[c]=last;
        ndx = last;
        ncolor[last]=c;
        last++;
      } else {
        ndx = (*p).second;
      }
      *pp = ndx;
      pp++;
    }
  }
  if (mycolor)
    delete color;
  color = ncolor;
  index = nindex;
  if (palette_size)
    *palette_size = last;
  myindex=mycolor=true;

//  printf("converted to indexed bitmap\n");

  return true;
}

void 
TBitmapFilter::setError(const char *txt)
{
  errortxt=txt;
}

