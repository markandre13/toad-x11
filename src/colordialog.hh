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

#ifndef _TOAD_COLORDIALOG_HH
#define _TOAD_COLORDIALOG_HH

#include <toad/dialog.hh>
#include <toad/rgbmodel.hh>

namespace toad {

class TColorDialog:
  public TDialog
{
    typedef TDialog super;
    TRGB *color;
    TRGB origcolor;
    TBoundedRangeModel hue, saturation, value;
    TRGBModel rgb;
    bool lock;

    void _init();
    void createBitmaps();
    void rgb2hsv();
    void hsv2rgb();

  public:
    TColorDialog(TWindow *parent, const string &title);
    TColorDialog(TWindow *parent, const string &title, TRGB *color);
    ~TColorDialog();
    void paint();
    void mouseLDown(int, int, unsigned);
    void mouseMove(int, int, unsigned);
    
    void done(bool apply);
    
    TBitmap *bmp1 , *bmp2;
};

} // namespace toad

#endif
