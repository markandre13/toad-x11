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

#ifndef TFilterGIF
#define TFilterGIF TFilterGIF

#include <toad/bitmapfilter.hh>

namespace toad {

class TFilterGIF: 
  public TBitmapFilter
{
  public:
    bool save(ostream&);
    EResult load(istream&);

    const char* getName(){return "CompuServe GIF";}
    const char* getExt(){return "*.gif";}
    int editSpecific();

    struct TColormap;
  private:
    struct TImage
    {
      unsigned height;
      unsigned len;
      bool interlace;
      TColormap *colormap;
    };
    char buffer[10];
    enum EType {GIF_TYPE_NONE, GIF_TYPE_87A, GIF_TYPE_89A} type;
    unsigned width, height;
    unsigned color_resolution;
    unsigned background;
    unsigned aspect_ratio;
    unsigned image_count;

    bool has_global_colormap;
};

} // namespace toad

#endif
