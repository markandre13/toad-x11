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

#ifndef _TOAD_NUMBERMODEL_HH
#define _TOAD_NUMBERMODEL_HH 1

#include <toad/model.hh>

namespace toad {

using namespace std;

template <class T>
class GNumberModel:
  public TModel
{
  public:
    typedef T value_type;
    
  protected:
    T minimum;
    T maximum;
    T value;
    T extent;
    
    bool adjusting;

    /** true, when the last setValue call had an out of range value */    
    bool out_of_range;

    virtual void changed() {}

  public:
    GNumberModel<T>(T min, T max, T val, T ext):
      minimum(min), maximum(max), value(val), extent(ext),
      adjusting(false) {}
    T getExtent() const { return extent; }
    T getMaximum() const { return maximum; }
    T getMinimum() const { return minimum; }
    T getValue() const { return value; }
    // name inconsistence: isAdjusting() const would be right
    bool getValueIsAdjusting() const { return adjusting; }
    
    void setExtent(T extent);
    void setMaximum(T max);
    void setMinimum(T min);
    void setRangeProperties(T value, T extent, T min, T max, bool adjusting=false);
    void setValue(T value);
    void setValueIsAdjusting(bool b);
    
    bool wasOutOfRange() const { return out_of_range; }
    GNumberModel<T>& operator=(const GNumberModel &m) {
      setValue(m.value);
      return *this;
    }
    GNumberModel<T>& operator=(T v) {
      setValue(v);
      return *this;
    }
    operator T() const {
      return value;
    }
};

template <class T>
void GNumberModel<T>::setExtent(T extent)
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

template <class T>
void GNumberModel<T>::setMaximum(T max)
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

template <class T>
void GNumberModel<T>::setMinimum(T min)
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

template <class T>
void GNumberModel<T>::setRangeProperties(T value, T extent, T min, T max, bool adjusting)
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

template <class T>
void GNumberModel<T>::setValue(T value) 
{
  if (this->value == value) {
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
    this->value = value;
    if (!adjusting) {
      changed();
      sigChanged();
    }
  } else {
  }
}

template <class T>
void GNumberModel<T>::setValueIsAdjusting(bool b) 
{
  if (adjusting == b)
    return;
  adjusting = b;
  if (b == false) {
    changed();
    sigChanged();
  }
}

} // namespace toad

#endif
