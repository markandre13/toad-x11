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
#include <cstdio>
#include <cstring>

#include <toad/toad.hh>
#include <toad/bitmap.hh>
#include <toad/bitmapfilter.hh>
#include <toad/io/urlstream.hh>
#include <toad/io/binstream.hh>
#include <toad/filter_bmp.hh>

using namespace toad;

TFilterBMP::EResult 
TFilterBMP::load(istream &is)
{
  TInBinStream file(&is);

  TBMPFileInfo bfi;
  ulong dummy;

  // load file header 
  //------------------
  if (!file.compareString("BM",2)) {
    setError("'%s' is not a bitmap file");
    return WRONG;
  }
    
  file.readDWord();
  file.readDWord();
  bfi.bitmap_offset = file.readDWord();

  // bitmap info || bitmap core info
  bfi.color_offset = file.tellRead();

  dummy = file.readDWord();
  bfi.color_offset += dummy;    // add size of bitmap info
  bfi.color_count  = (bfi.bitmap_offset - bfi.color_offset) / 4;
  if (dummy!=40) {
    setError("Can't load OS/2 1.2 Bitmap Files");
    return ERROR;
  }

  bfi.width           = file.readDWord();
  bfi.height          = file.readDWord();
  bfi.planes          = file.readWord();
  bfi.bits_per_pixel  = file.readWord();
  bfi.compression     = file.readDWord();
  bfi.image_size      = file.readDWord();
  if (bfi.planes!=1 || bfi.compression!=0) {
    setError("Can't load compressed bitmap files or files with more than "
             "one plane.");
    return ERROR;
  }

  if (( bfi.bits_per_pixel!=1 && bfi.bits_per_pixel!=4
     && bfi.bits_per_pixel!=8 && bfi.bits_per_pixel!=24))
  {
    setError("Bits per pixel resolution must be 1,4,8 or 24");
    return ERROR;
  }

  // load color palette 
  //--------------------
  TRGB f;
  if (bfi.bits_per_pixel!=24) {
    createBuffer(bfi.width,bfi.height,TBITMAP_INDEXED);
    file.seekRead(bfi.color_offset);
    for(unsigned i=0; i<bfi.color_count; i++) {
      f.b = file.readByte();
      f.g = file.readByte();
      f.r = file.readByte();
      file.readByte();
      setIndexColor(i,f);
    }
  } else {
    createBuffer(bfi.width, bfi.height, TBITMAP_TRUECOLOR);
  }
  
  // load bitmap data 
  //------------------
  file.seekRead(bfi.bitmap_offset);

  int i;
  unsigned n,c,x;
  int y;
  unsigned char b;

  switch(bfi.bits_per_pixel) {
    case 1:
      for(y=bfi.height-1; y>=0; y--) {
        c = 0;
        for(x=0; x<bfi.width; x+=8) {
          b = file.readByte();
          c++;
          for(i=0; i<8; i++) {
            n=b&1;
            b>>=1;
            setIndexPixel(x+7-i,y, n);
          }
        }
        c=4-(c%4); 
        if (c<4) 
          for(i=c; i>0; i--) 
            file.readByte();
      }
      break;

    case 4:
      for(y=bfi.height-1; y>=0; y--) {
        c = 0;
        for(x=0; x<bfi.width; x+=2) {
          c++;
          b = file.readByte();
          n=b & 15;
          setIndexPixel(x+1,y,n);
          n=(b>>4) & 15;
          setIndexPixel(x,y,n);
        }
        c=4-(c%4); 
        if (c<4) 
          for(i=c; i>0; i--) 
            file.readByte();
      }
      break;
    case 8:
      for(y=bfi.height-1; y>=0; y--) {
        c = 0;
        for(x=0; x<bfi.width; x++) {
          c++;
          setIndexPixel(x,y,file.readByte());
        }
        c=4-(c%4); 
        if (c<4) 
          for(i=c; i>0; i--) 
            file.readByte();
      }
      break;
    case 24:
      unsigned char c;
      for(y=bfi.height-1; y>=0; y--) {
        c=0;
        for(x=0; x<bfi.width; x++) {
          f.b = file.readByte();
          f.g = file.readByte();
          f.r = file.readByte();
          c+=3;
          setColorPixel(x,y, f);
        }
        c=4-(c%4); 
        if (c<4) 
          for(i=c; i>0; i--) 
            file.readByte();
      }
      break;
  }
  return OK;
}

bool TFilterBMP::save(ostream &os)
{
  TOutBinStream file(&os);

  file.writeString("BM");                       // type           (0)
  file.writeDWord(14+40+w*h*3UL);               // size           (2)
  file.writeDWord(0);                           // reserved       (6)
  file.writeDWord(14+40);                       // bitmap offset  (10)

  // bitmap info header
  //--------------------
  file.writeDWord(40);                          // size           (14)
  file.writeDWord(w);
  file.writeDWord(h);
  file.writeWord(1);                            // planes
  file.writeWord(24);                           // bit per pixel
  file.writeDWord(0);                           // compression
  file.writeDWord(w*h*3UL);                     // size of image
  file.writeDWord(0);
  file.writeDWord(0);
  file.writeDWord(0);
  file.writeDWord(0);

  short c;
  TRGB f;
  for(int y=h-1; y>=0; y--) {
    c=0;
    for(int x=0; x<w; x++) {
      if (isIndex())
        getIndexColor(getIndexPixel(x,y), &f);
      else
        getColorPixel(x,y, &f);
      file.writeByte(f.b);
      file.writeByte(f.g);
      file.writeByte(f.r);
      c+=3;
    }
    c=4-(c%4);
    if (c<4)
      for(int x=c; x>0; x--)
        file.writeByte(0);
  }
  return true;
}

