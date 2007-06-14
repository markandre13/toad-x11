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

#ifndef _TOAD_FLOATMODEL_HH
#define _TOAD_FLOATMODEL_HH 1

#include <toad/textmodel.hh>
#include <iostream>

namespace toad {

using namespace std;

class TFloatModel:
  public TModel
{
  protected:
    double minimum;
    double maximum;
    double value;
    double extent;
    
    bool adjusting;

    /** true, when the last setValue call had an out of range value */    
    bool out_of_range;

    virtual void changed();

  public:
    TFloatModel();
    
    double getExtent() const { return extent; }
    double getMaximum() const { return maximum; }
    double getMinimum() const { return minimum; }
    double getValue() const { return value; }
    // name inconsistence: isAdjusting() const would be right
    bool getValueIsAdjusting() const { return adjusting; }
    
    void setExtent(double extent);
    void setMaximum(double max);
    void setMinimum(double min);
    void setRangeProperties(double value, double extent, double min, double max, bool adjusting=false);
    void setValue(double value);
    void setValueIsAdjusting(bool b);
    
    bool wasOutOfRange() const { return out_of_range; }
    
    TFloatModel& operator=(const TFloatModel &m) {
      setValue(m.value);
      return *this;
    }
    TFloatModel& operator=(double v) {
      setValue(v);
      return *this;
    }
    operator double() const {
      return value;
    }
};

inline ostream& operator<<(ostream &s, const TFloatModel &m) {
  return s<<m.getValue();
}

typedef GSmartPointer<TFloatModel> PFloatModel;

TTextModel * createTextModel(TFloatModel *);

} // namespace toad

bool restore(atv::TInObjectStream &in, toad::TFloatModel *value);

#endif
