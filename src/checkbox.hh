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

#ifndef TCheckBox
#define TCheckBox TCheckBox

#include <toad/labelowner.hh>
#include <toad/model.hh>

namespace toad {

class TBoolModel:
  public TModel
{
    bool value;
  public:
    TBoolModel() {
      value = false;
    }
    void setValue(bool b) {
      if (b==value)
        return;
      value = b;
      sigChanged();
    }
    void toggleValue() {
      value = !value;
      sigChanged();
    }
    bool getValue() const {
      return value;
    }
    const bool operator=(bool b) { setValue(b); return b; }
    operator bool() const { return value; }
};

typedef GSmartPointer<TBoolModel> PBoolModel;

class TCheckBox: 
  public TLabelOwner
{
  typedef TLabelOwner super;
  PBoolModel model;

  public:
    TCheckBox(TWindow *parent, const string &title, TBoolModel * model = 0);
    ~TCheckBox(); // force vtable creation in checkbox.cc (EGCS 1.0.3a)

#if 0
    template <class T>
    TCheckBox(TWindow *p, const std::string &t, T *d)
      :super(p,t)
    {
      _init();
    }
#endif

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
    
  private:
    void valueChanged();
    void keyDown(TKey,char*,unsigned);
    void mouseLDown(int,int,unsigned);
    void paint();

    void _init(TBoolModel *);
};

} // namespace toad

#endif
