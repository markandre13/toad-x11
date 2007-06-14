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

#ifndef _TOAD_FATCHECKBUTTON_HH
#define _TOAD_FATCHECKBUTTON_HH 1

#include <toad/toad.hh>
#include <toad/buttonbase.hh>
#include <toad/boolmodel.hh>

namespace toad {

class TFatCheckButton:
  public TButtonBase
{
  PBoolModel model;

  public:
    TFatCheckButton(TWindow *p, const string &t, TBoolModel * model = 0);

    void setValue(bool b) {
      getModel()->setValue(b);
    }
    bool getValue() const {
      return getModel()->getValue();
    }
    
    void setModel(TBoolModel *b);
    TBoolModel * getModel() const {
      return model;
    }
    
  protected:
    void valueChanged();
    void mouseLDown(const TMouseEvent&);
    void keyDown(const TKeyEvent&);
    void paint();
    void _init(TBoolModel*);
};

} // namespace toad

#endif
