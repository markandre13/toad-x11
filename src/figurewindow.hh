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

#ifndef _TOAD_FIGUREWINDOW_HH
#define _TOAD_FIGUREWINDOW_HH

#include <toad/figuremodel.hh>

namespace toad {

/**
 * \ingroup figure
 */
class TFigureWindow: 
  public TWindow
{
    typedef TWindow super;
  public:
    TFigureWindow(TWindow *p, const string& t);
    void store(TOutObjectStream&) const;
    virtual bool restore(TInObjectStream&);
    virtual void addGadget(TFigure *g) { gadgets->add(g); }
  protected:
    void paint();
  public:
    virtual void print(TPenBase&);
    void setModel(TFigureModel *m) {
      gadgets = m;
    }
    TFigureModel * getModel() const {
      return gadgets;
    }
  
    PFigureModel gadgets;
};

} // namespace toad

#endif
