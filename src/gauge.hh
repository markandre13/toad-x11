/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_GAUGE_HH
#define _TOAD_GAUGE_HH

#include <toad/arrowbutton.hh>
#include <toad/integermodel.hh>

namespace toad {

class TGauge:
  public TControl
{
    PIntegerModel model;
    TArrowButton *up, *down;
    int unitIncrement;
    int down_y, down_val;
    
  public:
    TGauge(TWindow*, const string&, TIntegerModel *model=0);
    
    void setModel(TIntegerModel*);
    TIntegerModel* getModel() const { return model; }
    void setUnitIncrement(int i) { unitIncrement=i; }
    int getUnitIncrement() const { return unitIncrement; }

    void resize();
    void increment();
    void decrement();
    
    void modelChanged();
    
    void mouseEvent(TMouseEvent &me);
};

} // namespace toad

#endif
