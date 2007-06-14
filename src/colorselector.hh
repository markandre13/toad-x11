/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_COLORSELECTOR_HH
#define _TOAD_COLORSELECTOR_HH 1

#include <toad/dnd/color.hh>
#include <toad/figureeditor.hh>

namespace toad {

class TPushButton;

class TColorSelector:
  public TWindow
{
    typedef TWindow super;

    PFigureAttributes gedit;
    TRGB linecolor;
    TRGB fillcolor;
    bool filled;
    int border, w2, h;
    TPushButton *pb1, *pb2;
    TDropSiteColor *ds;

  public:
    TColorSelector(TWindow *parent, 
                   const string &title, 
                   TFigureAttributes *gedit = 0);
    ~TColorSelector();

    bool dialogeditorhack;
  protected:
    void resize();
    void paint();
    void mouseLDown(const TMouseEvent&);
    void mouseMDown(const TMouseEvent&);

    void dropColor(const PDnDColor&);
    void openColorDialog();
    void openColorPalette();
    
    void preferencesChanged();
};

} // namespace toad

#endif
