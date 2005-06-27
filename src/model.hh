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

#ifndef TModel
#define TModel TModel

#include <toad/pointer.hh>
#include <toad/connect.hh>

#include <iostream>

namespace toad {  // model.hh

class TModel:
  public TSmartObject
{
  public:
    TModel() {}
    TModel(const TModel &m) { }
    ~TModel();
    TModel& operator=(const TModel&) { return *this; }
    
    void lock() { sigChanged.lock(); }
    void unlock() { sigChanged.unlock(); }

    TSignal sigChanged;
    TSignal sigDestruct;
};

template <class T>
class GModelOwner
{
  protected:
    GSmartPointer<T> model;
  public:
    GModelOwner() {}
    virtual ~GModelOwner() {
      if(model) {
        disconnect(model->sigChanged, this, &GModelOwner<T>::modelChanged);
        disconnect(model->sigDestruct, this, &GModelOwner<T>::modelDestruction);
      }
    }
    void setModel(T *m) {
//      cout << "GModelOwner::setModel("<<m<<")"<< endl;
      if (m==model)
        return;
      if (model) {
        disconnect(model->sigChanged, this, &GModelOwner<T>::modelChanged);
        disconnect(model->sigDestruct, this, &GModelOwner<T>::modelDestruction);
      }
      model = m;
      if (model) {
        connect(model->sigChanged, this, &GModelOwner<T>::modelChanged, false);
        connect(model->sigDestruct, this, &GModelOwner<T>::modelDestruction);
      }
      modelChanged(true);
    }
    T* getModel() const { return model; }
    virtual void modelChanged(bool newmodel) = 0;
    void modelDestruction() {
      cout << "GModelOwner<T>: caught model destruction, set it to NULL" << endl;
      model = 0;
      cout << "GModelOwner<T>: call modelChanged" << endl;
      modelChanged(true);
      cout << "GModelOwner<T>: called modelChanged" << endl;
    }
};

} // namespace toad model.hh

#endif
