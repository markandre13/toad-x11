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

#ifndef TFilterBMP
#define TFilterBMP TFilterBMP

namespace toad {

/**
 * Filter for BMP files
 */
class TFilterBMP: public TBitmapFilter
{
  public:
    bool save(ostream&);
    EResult load(istream&);
    const char* getName(){return "PC-DOS Bitmap";}
    const char* getExt(){return "*.bmp";}
  private:
    struct TBMPFileInfo
    {
      ulong bitmap_offset;
      ulong color_offset;
      unsigned color_count;
      ulong width;
      ulong height;
      unsigned planes;
      unsigned bits_per_pixel;
      ulong compression;
      ulong image_size;
    };
};

}

#endif
