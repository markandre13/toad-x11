/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef TLabelOwner
#define TLabelOwner TLabelOwner

#include <toad/control.hh>

namespace toad {

class TLabelOwner: 
  public TControl
{
  public:
    TLabelOwner(TWindow *p, const string &t): TControl(p, t) {}
    virtual ~TLabelOwner();
    string _label;
    const string& getLabel() const 
    { 
      return _label.size()==0 ? getTitle() : _label;
    }
    void setLabel(const string &label)
    { 
      _label = label;
      invalidateWindow();
    }
};

}

#endif
