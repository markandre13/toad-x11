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

#include <toad/boundedrangemodel.hh>

using namespace toad;

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
  if (this->extent == extent)
    return;
  this->extent = extent;
  if (value+extent>maximum)
    value = maximum-extent;
  if (!adjusting)
    sigChanged();
}

void
TBoundedRangeModel::setMaximum(int max)
{
  if (maximum == max)
    return;
  maximum = max;
  if (value+extent>maximum)
    value=maximum-extent;
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
  if (value<min)
    value = min;
  if (value+extent>max)
    value = max - extent;

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
  if (this->value == value)
    return;
  out_of_range = false;
  if (value<minimum) {
    out_of_range = true;
    value = minimum;
  }
  if (value+extent>maximum) {
    out_of_range = true;
    value = maximum-extent;
  }
  if (this->value != value) {
    this->value = value;
    if (!adjusting)
      sigChanged();
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
