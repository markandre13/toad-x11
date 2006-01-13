/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2006 by Mark-André Hopf <mhopf@mark13.org>
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
    TModel() {_enabled=true; meta=META_CUSTOM;}
    TModel(const TModel &m) {_enabled=true; meta=META_CUSTOM;}
    ~TModel();
    TModel& operator=(const TModel&) { return *this; }
    
    void lock() { sigChanged.lock(); }
    void unlock() { sigChanged.unlock(); }

    // another dirty hack...
    bool isEnabled() const {return _enabled;}
    void setEnabled(bool b) {
      _enabled=b;
      meta=META_ENABLED;
      sigMeta();
      meta=META_CUSTOM;
    }

    TSignal sigChanged;
    TSignal sigMeta;
    
    enum EMeta {
      META_ENABLED,
      META_DESTRUCTION,
      META_CUSTOM
    };
    EMeta meta:7;
    
  private:
    bool _enabled:1;
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
        disconnect(model->sigMeta,    this, &GModelOwner<T>::modelMeta);
      }
    }
    void setModel(T *m) {
//      cout << "GModelOwner::setModel("<<m<<")"<< endl;
      if (m==model)
        return;
      if (model) {
        disconnect(model->sigChanged, this, &GModelOwner<T>::modelChanged);
        disconnect(model->sigMeta   , this, &GModelOwner<T>::modelMeta);
      }
      model = m;
      if (model) {
        connect(model->sigChanged, this, &GModelOwner<T>::modelChanged, false);
        connect(model->sigMeta   , this, &GModelOwner<T>::modelMeta);
      }
      modelChanged(true);
    }
    T* getModel() const { return model; }
    virtual void modelChanged(bool newmodel) = 0;
    void modelMeta() {
      if (model->meta!=TModel::META_DESTRUCTION)
        return;
      cout << "GModelOwner<T>: caught model destruction, set it to NULL" << endl;
      model = 0;
      cout << "GModelOwner<T>: call modelChanged" << endl;
      modelChanged(true);
      cout << "GModelOwner<T>: called modelChanged" << endl;
    }
};

} // namespace toad model.hh

#endif
