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

#include <toad/floatmodel.hh>
#include <toad/textmodel.hh>
#include <cmath>
#include <cfloat>

using namespace toad;

#define DBM(CMD)

/**
 * \class TIntegerModel
 *
 * Defines the data model used by controls like TScrollBar.
 */

TFloatModel::TFloatModel():
  minimum(-DBL_MAX), maximum(DBL_MAX), value(0), extent(1),
  adjusting(false) 
{}

void
TFloatModel::setExtent(double extent)
{
  if (extent<1)
    extent = 1;
  if (this->extent == extent)
    return;
  this->extent = extent;
    
  if (value+extent-1>maximum)
    value = maximum-extent+1;
  if (value<minimum)
    value = minimum;
  if (!adjusting) {
    changed();
    sigChanged();
  }
}

void
TFloatModel::setMaximum(double max)
{
  if (maximum == max)
    return;
  maximum = max;
  if (value+extent-1>maximum)
    value=maximum-extent+1;
  if (value<minimum)
    value = minimum;
  if (!adjusting) {
    changed();
    sigChanged();
  }
}

void
TFloatModel::setMinimum(double min)
{
  if (minimum == min)
    return;
  minimum = min;
  if (value<minimum)
    value = minimum;
  if (!adjusting) {
    changed();
    sigChanged();
  }
}

void
TFloatModel::setRangeProperties(double value, double extent, double min, double max, bool adjusting)
{
  if (extent<1)
    extent = 1;
  if (value+extent-1>max)
    value = max - extent + 1;
  if (value<min)
    value = min;

  if (
    this->value   == value &&
    this->extent  == extent &&
    this->minimum == min &&
    this->maximum == max &&
    this->adjusting == adjusting
  )
    return;

  this->value   = value;
  this->extent  = extent;
  this->minimum = min;
  this->maximum = max;
  this->adjusting = adjusting;
  if (!adjusting) {
    changed();
    sigChanged();
  }
}

void
TFloatModel::setValue(double value) 
{
DBM(cerr << "TFloatModel::setValue(" << value << ")\n";)
  if (this->value == value) {
DBM(cerr << "-> not changed" << endl;)
    return;
  }
  out_of_range = false;
  if (value<minimum) {
    out_of_range = true;
    value = minimum;
  }
  if (value+extent-1>maximum) {
    out_of_range = true;
    value = maximum-extent+1;
  }
  if (this->value != value) {
DBM(cerr << "-> changed (previous value was " << this->value << ")\n";)
    this->value = value;
    if (!adjusting) {
      changed();
      sigChanged();
    }
  } else {
DBM(cerr << "-> not changed" << endl;)
  }
}

void
TFloatModel::changed()
{
}

void
TFloatModel::setValueIsAdjusting(bool b) 
{
  if (adjusting == b)
    return;
  adjusting = b;
  if (b == false) {
    changed();
    sigChanged();
  }
}

class TFloatTextModel:
  public TTextModel
{
    TFloatModel * model;
    bool lock;
  public:
    TFloatTextModel(TFloatModel *m) {
      model = m;
      lock = false;
      if (model) {
        connect(model->sigChanged, this, &TFloatTextModel::slaveChanged);
        slaveChanged();
      }
//      connect(this->sigChanged, this, &TIntegerTextModel::masterChanged);
    }
    ~TFloatTextModel() {
      if (model)
        disconnect(model->sigChanged, this);
    }
    int filter(int c) {
      if (c=='\n') {
DBM(cerr << "BoundedRangeTextModel filter detected '\\n', calling masterChanged\n";)
        masterChanged();
        return 0;
      }
      if (c==TTextModel::CHARACTER_CURSOR_UP) {
        model->setValue(model->getValue()+0.1);
        return 0;
      }
      if (c==TTextModel::CHARACTER_CURSOR_DOWN) {
        model->setValue(model->getValue()-0.1);
        return 0;
      }
      if ( (c<'0' || c>'9') && c!='-' && c!='.') {
        return 0;
      }
      return c;
    }
    void focus(bool b) {
DBM(cerr << "TFloatTextModel::focus(" << b << ")\n";)
      if (!b) {
DBM(cerr << "-> calling master changed\n";)
        masterChanged();
      }
    }
    void masterChanged()
    {
DBM(cerr << "TFloatTextModel::masterChanged()\n";)
      if (lock) {
DBM(cerr << "  locked => return\n";)
        return;
      }
DBM(cerr << "  not locked => setValue\n";)
      double a = atof(_data.c_str());
      lock = true;
      model->setValue(a);
      lock = false;
    }
    void slaveChanged()
    {
DBM(cerr << "TIntegerTextModel::slaveChanged()\n";)
      sigChanged();
      if (lock) {
DBM(cerr << "  locked => return\n";)
        return;
      }
DBM(cerr << "  not locked => getValue\n";)
      char buffer[16];
#ifndef __WIN32__
      snprintf(buffer, sizeof(buffer), "%f", model->getValue());
#else
      sprintf(buffer, "%i", model->getValue());
#endif
      lock = true;
      setValue(buffer);
      lock = false;
    }
};

TTextModel *
toad::createTextModel(TFloatModel * m)
{
  return new TFloatTextModel(m);
}

bool
restore(atv::TInObjectStream &in, toad::TFloatModel *value)
{
  if (in.what != ATV_VALUE)
    return false;
  char *endptr;  
  *value = strtod(in.value.c_str(), &endptr);
  if (endptr!=0 && *endptr!=0)
    return false;
  return true;   
}
