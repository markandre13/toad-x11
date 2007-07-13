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

#include <toad/numbermodel.hh>
#include <toad/textmodel.hh>
#include <iostream>
#include <cfloat>

namespace toad {

using namespace std;

class TFloatModel:
  public GNumberModel<double>
{
  public:
    TFloatModel():GNumberModel<double>(-DBL_MAX, DBL_MAX, 0.0, 1.0) {}
    TFloatModel& operator=(double v) {
      setValue(v);
      return *this;
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
