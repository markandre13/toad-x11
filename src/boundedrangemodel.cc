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

#include <toad/boundedrangemodel.hh>
#include <toad/textmodel.hh>
#include <limits.h>

using namespace toad;

#define DBM(CMD)

/**
 * \class TBoundedRangeModel
 *
 * Defines the data model used by controls like TScrollBar.
 */

TBoundedRangeModel::TBoundedRangeModel():
  minimum(INT_MIN), maximum(INT_MAX), value(0), extent(1), 
  adjusting(false) 
{}

void
TBoundedRangeModel::setExtent(int extent)
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
  if (!adjusting)
    sigChanged();
}

void
TBoundedRangeModel::setMaximum(int max)
{
  if (maximum == max)
    return;
  maximum = max;
  if (value+extent-1>maximum)
    value=maximum-extent+1;
  if (value<minimum)
    value = minimum;
  if (!adjusting)
    sigChanged();
}

void
TBoundedRangeModel::setMinimum(int min)
{
  if (minimum == min)
    return;
  minimum = min;
  if (value<minimum)
    value = minimum;
  if (!adjusting)
    sigChanged();
}

void
TBoundedRangeModel::setRangeProperties(int value, int extent, int min, int max, bool adjusting)
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
  if (!adjusting)
    sigChanged();
}

void
TBoundedRangeModel::setValue(int value) 
{
DBM(cerr << "TBoundedRangeModel::setValue(" << value << ")\n";)
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
    if (!adjusting)
      sigChanged();
  } else {
DBM(cerr << "-> not changed" << endl;)
  }
}

void
TBoundedRangeModel::setValueIsAdjusting(bool b) 
{
  if (adjusting == b)
    return;
  adjusting = b;
  if (b == false)
    sigChanged();
}

class TBoundedRangeTextModel:
  public TTextModel
{
    TBoundedRangeModel * model;
    bool lock;
  public:
    TBoundedRangeTextModel(TBoundedRangeModel *m) {
      model = m;
      lock = false;
      if (model) {
        connect(model->sigChanged, this, &TBoundedRangeTextModel::slaveChanged);
        slaveChanged();
      }
//      connect(this->sigChanged, this, &TBoundedRangeTextModel::masterChanged);
    }
    ~TBoundedRangeTextModel() {
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
        model->setValue(model->getValue()+1);
        return 0;
      }
      if (c==TTextModel::CHARACTER_CURSOR_DOWN) {
        model->setValue(model->getValue()-1);
        return 0;
      }
      if ( (c<'0' || c>'9') && c!='-') {
        return 0;
      }
      return c;
    }
    void focus(bool b) {
DBM(cerr << "TBoundedRangeTextModel::focus(" << b << ")\n";)
      if (!b) {
DBM(cerr << "-> calling master changed\n";)
        masterChanged();
      }
    }
    void masterChanged()
    {
DBM(cerr << "TBoundedRangeTextModel::masterChanged()\n";)
      if (lock) {
DBM(cerr << "  locked => return\n";)
        return;
      }
DBM(cerr << "  not locked => setValue\n";)
      int a = atoi(data.c_str());
      lock = true;
      model->setValue(a);
      lock = false;
    }
    void slaveChanged()
    {
DBM(cerr << "TBoundedRangeTextModel::slaveChanged()\n";)
      sigChanged();
      if (lock) {
DBM(cerr << "  locked => return\n";)
        return;
      }
DBM(cerr << "  not locked => getValue\n";)
      char buffer[16];
#ifndef __WIN32__
      snprintf(buffer, sizeof(buffer), "%i", model->getValue());
#else
      sprintf(buffer, "%i", model->getValue());
#endif
      lock = true;
      setValue(buffer);
      lock = false;
    }
};

TTextModel *
toad::createTextModel(TBoundedRangeModel * m)
{
  return new TBoundedRangeTextModel(m);
}

bool
restore(atv::TInObjectStream &in, toad::TBoundedRangeModel *value)
{
  if (in.what != ATV_VALUE)
    return false;
  char *endptr;  
  *value = strtol(in.value.c_str(), &endptr, 10);
  if (endptr!=0 && *endptr!=0)
    return false;
  return true;   
}
