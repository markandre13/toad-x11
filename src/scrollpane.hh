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

#ifndef _TOAD_SCROLLPANE_HH
#define _TOAD_SCROLLPANE_HH

#include <toad/window.hh>

namespace toad {

class TScrollBar;

class TScrollPane:
  public TWindow
{
  public:
    TScrollPane(TWindow *p, const string &t);
    
  protected:
    //! the pane (rename this one!)
    TRectangle visible;
    
    void resetScrollPane();
    
    virtual void scrolled(int x, int y);
    
    void doLayout();
    virtual void adjustPane() = 0;
    
    void getPanePos(int *x, int *y, bool setall=true) const;
    void setPanePos(int x, int y);
    void setUnitIncrement(int uix, int uiy);

    // pane location and size in pixels
    TRectangle pane;
    
    void paintCorner(TPen&);

  private:
    //! unit increment as set by setUnitIncrement
    int uix, uiy;
  
    //! last scrollbar position (so we know how much to scroll)
    int lx, ly;

    TScrollBar *vscroll, *hscroll;

    void _scrolled();
};

} // namespace toad

#endif
