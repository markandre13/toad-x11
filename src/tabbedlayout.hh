/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#ifndef _TOAD_TABBEDLAYOUT_HH
#define _TOAD_TABBEDLAYOUT_HH

#include <toad/layout.hh>

namespace toad {

class TTabbedLayout:
  public TLayout
{
  public:
    TTabbedLayout() { tab = 0; current = 0;}
  private:
    int width;
    
    struct TTab:
      public TRectangle
    {
      TWindow *window;
    };
    
    TTab *tab;
    unsigned ntabs;
    
    TWindow *current;

    bool mouseEvent(TMouseEvent&);
    void arrange();
    void paint();

    void paintTab(TPen &pen, TTab &tab, bool filled);
  
    TCloneable* clone() const { return new TTabbedLayout(*this); }
    const char * name() const { return "toad::TTabbedLayout"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
};

} // namespace toad

#endif
