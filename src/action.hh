/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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

#ifndef TAction
#define TAction TAction

#include <toad/toad.hh>
#include <toad/model.hh>
#include <set>
#include <toad/menuhelper.hh>

namespace toad {

class TAction;

class TActionStorage:
  protected set<TAction*>
{
    typedef set<TAction*> super;
  public:
    typedef super::iterator iterator;
    typedef super::const_iterator const_iterator;
    iterator begin() { return super::begin(); }
    iterator end() { return super::end(); }
    void push_back(TAction *a) { super::insert(a); sigChanged(); }
    void erase(TAction *a) {
      iterator p = super::find(a);
      if (p!=super::end()) {
        super::erase(p);
        sigChanged();
      }
    }
    TSignal sigChanged;
};

class TAction:
  public TInteractor
{
    TBitmap *bitmap;

  private:
    bool focus;
    bool enabled;

  public:
    static TActionStorage actions;

    TAction(TInteractor *, const string&);
    virtual ~TAction();
    
    // enable/disable
    void domainFocus(bool);
    void setEnabled(bool b);
    bool isEnabled() const;
    
    //! this signal is triggered when the action has to be performed
    TSignal sigActivate;
    
    virtual void trigger(unsigned idx=0)
    {
      sigActivate.trigger();
    }

    virtual void delayedTrigger(unsigned idx=0)
    {
      sigActivate.delayedTrigger();
    }

    //! the status of the action (enabled/disabled) has changed
    TSignal sigChanged;
    
    enum EType {
      BUTTON,
      CHECKBUTTON,
      RADIOBUTTON
    } type;
    
    virtual unsigned getSize() const;
    virtual const string& getID(unsigned idx) const;
    virtual unsigned getSelection() const { return 0; }
    virtual bool getState(string *text, bool *active) const { return false; }
};

class TChoiceModel:
  public TModel
{
  public:
    TChoiceModel() {
      idx = 0;
    }
    void select(unsigned idx) {
      // cout << "selected " << data[idx]->id << endl;
      sigChanged();
      this->idx = idx;
    }
    unsigned getSelection() const {
      return idx;
    }
    TSignal sigChanged;
  protected:
    unsigned idx;
};

/**
 * Class to hold the data for a choice.
 */
template <class T>
class GChoiceModel:
  public TChoiceModel
{
  public:
    void add(const string &id, T what) {
      TData *d = new TData();
      d->id = id;
      d->what = what;
      data.push_back(d);
    }
    unsigned getSize() const { return data.size(); }
    const string& getID(unsigned idx) const { return data[idx]->id; }
    const T& getValue() const { return data[idx]->what; }
    const T& getValue(unsigned idx) const { return data[idx]->what; }
    
  protected:
    struct TData {
      string id;
      T what;
    };
    static const TData empty;
    vector<TData*> data;
};

class TAbstractChoice:
  public TAction
{
  public:
    TAbstractChoice(TWindow *p, const string &t):TAction(p, t) {}
    virtual ~TAbstractChoice();
    virtual TChoiceModel* getModel() = 0;
};

/**
 * Class to map a choice model into the display.
 */
template <class T>
class GChoice:
  public TAbstractChoice
{
  public:
    GChoice<T>(TWindow *p, 
               const string &id,
               GChoiceModel<T> *m=new GChoiceModel<T>()):
      TAbstractChoice(p,id), model(m)
    {
      type = RADIOBUTTON;
    }
    
    TChoiceModel* getModel() { return model; }
  
    void add(const string &id, T what) { model->add(id, what); }
    const T& getValue() const { return model->getValue(); }
    unsigned getSize() const { return model->getSize(); }
    virtual const string& getID(unsigned idx) const { return model->getID(idx); }
    virtual void select(unsigned idx=0) {
      model->select(idx);
      sigActivate();
    }
    virtual void trigger(unsigned idx=0)
    {
      model->select(idx);
      sigActivate.trigger();
    }
    virtual void delayedTrigger(unsigned idx=0)
    {
      model->select(idx);
      sigActivate.delayedTrigger();
    }
    virtual unsigned getSelection() const { return model->getSelection(); }
    GChoiceModel<T> *model;
};

}

#endif
