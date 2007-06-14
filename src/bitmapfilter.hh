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

// base filter class for import/export of TBitmap data
//-----------------------------------------------------

#ifndef _TOAD_BITMAPFILTER_HH
#define _TOAD_BITMAPFILTER_HH 1

#include <toad/config.h>
#include <toad/bitmap.hh>

namespace toad {

#ifdef __WIN32__
#undef OK
#undef WRONG
#undef ERROR
#endif

class TBitmapFilter
{
  private:
    TBitmapFilter(const TBitmapFilter&) { }
  public:
    TBitmapFilter();
    virtual ~TBitmapFilter() {};

    virtual const char* getName()=0;
    virtual const char* getExt()=0;
    virtual int editSpecific(){return 0;}

    enum EResult {
      OK, WRONG, ERROR
    };

    virtual bool save(ostream&)=0;
    virtual EResult load(istream&)=0;

    void deleteBuffer();
    void createBuffer(int w,int h,EBitmapType);
    void setBuffer(int w,int h,TRGB24*,unsigned char*);
    void getBuffer(int *w,int *h,TRGB24**,unsigned char**);
    bool isIndex();
    
    // index
    void setIndexColor(int index, TRGB24 &c);
    bool getIndexColor(int index, TRGB24 *c);
    void setIndexPixel(int x,int y,int index);
    // void setIndexLine(int y, byte*);
    // void setIndexLine(int y, ushort*);
    // void setIndexLine(int y, ulong*);
    int getIndexPixel(int x,int y);
    
    // true color
    void setColorPixel(int x,int y,TRGB24&);
    // void SetColorLine(int y, TRGB*);
    bool getColorPixel(int x,int y,TRGB24*);
    
    void setError(const char *txt);
    const char *errortxt;

  protected:
    int w,h;

  private:
    TRGB24 *color;
    unsigned char *index;
    bool mycolor:1;
    bool myindex:1;
    
  public:
    TBitmapFilter *next;
};

} // namespace toad

#endif
