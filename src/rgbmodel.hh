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

#ifndef _TOAD_RGBMODEL_HH
#define _TOAD_RGBMODEL_HH

#include <toad/model.hh>
#include <toad/boundedrangemodel.hh>
#include <toad/textarea.hh>
// #include <toad/textmodel.hh>

namespace toad {

class TRGBModel:
  public TModel
{
  public:
    TRGBModel();
    TRGBModel(const TRGBModel &rgb) { _init(); *this = rgb; }
    TRGBModel& operator=(const TRGBModel &rgb) { *this = rgb; return *this; }
    TRGBModel& operator=(const TRGB &rgb) { 
      set(rgb.r, rgb.g, rgb.b);
      return *this; 
    }
    operator TRGB() const { TRGB c(r, g, b); return c; }
    TBoundedRangeModel r, g, b;
    
    void set(int r, int g, int b) {
      if (this->r == r && this->g == g && this->b == b)
        return;
      lock = true;
      this->r = r;
      this->g = g;
      this->b = b;
      lock = false;
      changed();
    }
    
  protected:
    bool lock;
    void _init();
    void changed();
};

TTextModel * createTextModel(TRGBModel * m);

} // namespace toad

#endif
