/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef TMessageBox
#define TMessageBox TMessageBox

#include <toad/window.hh>

namespace toad {

class TBitmap;
class TPushButton;

class TMessageBox: 
  public TWindow
{
  public:
    TMessageBox(TWindow *parent, 
                const string &title, 
                const string &text, 
                unsigned long type, 
                TBitmap *bitmap=NULL,
                EWindowPlacement placement=PLACE_PARENT_CENTER);
    unsigned getResult() const;
    
    void setCenterMode(EWindowPlacement p) {
      _placement = p;
    }

    static const unsigned ICON_EXCLAMATION = 0x1000;
    static const unsigned ICON_HAND        = 0x2000;
    static const unsigned ICON_STOP        = 0x3000;
    static const unsigned ICON_INFORMATION = 0x4000;
    static const unsigned ICON_QUESTION    = 0x5000;

    static const unsigned ACCEPT  = 1;
    static const unsigned ABORT   = 2;
    static const unsigned OK      = 4;
    static const unsigned RETRY   = 8;
    static const unsigned YES     = 16;
    static const unsigned NO      = 32;
    static const unsigned CANCEL  = 64;
#ifdef __WIN32__
#undef IGNORE
#endif
    static const unsigned IGNORE  = 128;

    static const unsigned ABORTRETRYIGNORE = 2+8+128;
    static const unsigned OKCANCEL         = 4+64;
    static const unsigned RETRYCANCEL      = 8+64;
    static const unsigned YESNO            = 16+32;
    static const unsigned YESNOCANCEL      = 16+32+64;
    
    static const unsigned BUTTON1 = 0x0100;
    static const unsigned BUTTON2 = 0x0200;
    static const unsigned BUTTON3 = 0x0300;
    static const unsigned BUTTON4 = 0x0400;
    static const unsigned BUTTON5 = 0x0500;
    static const unsigned BUTTON6 = 0x0600;
    static const unsigned BUTTON7 = 0x0700;
    static const unsigned BUTTON8 = 0x0800;

  private:
    EWindowPlacement _placement;

    void adjust();
    void paint();
    
    unsigned result;
    string text;
    ulong type;
    
    TCoord tx,ty;  // text position
    TCoord tw;     // text width
    TCoord iy;     // icon position

    void button(int id);
    TBitmap *bitmap;
};

unsigned messageBox(TWindow* parent, 
                    const string& title,
                    const string &text, 
                    ulong type=TMessageBox::ICON_INFORMATION|TMessageBox::OK,
                    TBitmap *bmp=NULL,
                    EWindowPlacement placement=PLACE_PARENT_CENTER);

} // namespace toad

#endif
