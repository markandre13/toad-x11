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

#ifndef TRadioButtonBase
#define TRadioButtonBase TRadioButtonBase

#include <toad/buttonbase.hh>
#include <toad/model.hh>
#include <vector>

namespace toad {

class TRadioButtonBase;

/**
 * \note
 *    Similar to the 'ButtonGroup' in Java's Swing Library.
 */
class TRadioStateModel:
  public TModel
{
    friend class TRadioButtonBase;

    typedef vector<TRadioButtonBase*> TListenerBuffer;
    TListenerBuffer listener;      
    
    TRadioButtonBase *_temporary;
    
    TRadioButtonBase *_current;
  protected:
    void add(TRadioButtonBase*);
    void remove(TRadioButtonBase*);
    
  public:
    TRadioStateModel();
    void setCurrent(TRadioButtonBase*);
    TRadioButtonBase* getCurrent() const { return _current; }

    void setTemporary(TRadioButtonBase*);
    TRadioButtonBase* getTemporary() const { return _temporary; }
};
typedef GSmartPointer<TRadioStateModel> PRadioStateModel;

template <class T> 
class GRadioStateModel:
  public TRadioStateModel
{
    struct TBtnValue {
      TBtnValue(TRadioButtonBase *b, const T& v): btn(b), value(v) {};
      TRadioButtonBase *btn;
      T value;
    };
    typedef vector<TBtnValue> TBtnValueVec;
    TBtnValueVec vec;
    T memo;
    
  public:
    TRadioButtonBase * add(TRadioButtonBase *b, const T& v) {
      b->setModel(this);
      TRadioStateModel::add(b);
      vec.push_back(TBtnValue(b, v));
      if (!getCurrent() && memo==v)
        setCurrent(b);
      return b;
    }
    
    TRadioButtonBase* getButton(const T& v) const {
      typename TBtnValueVec::const_iterator p, e;
      p = vec.begin();
      e = vec.end();
      while(p!=e && (*p).value != v )
        ++p;
      return p==e ? 0 : (*p).btn;
    }
  
    void setValue(const T& v) {
      TRadioButtonBase* btn = getButton(v);
      if (btn)
        setCurrent(btn);
      else
        memo = v;
    }
    
    const T& getValue() const {
      if (!getCurrent)
        return memo;
    
      typename TBtnValueVec::const_iterator p, e;
      p = vec.begin();
      e = vec.end();
      while(p!=e) {
        if ( (*p).btn == getCurrent() )
          break;
        ++p;
      }
      return (*p).value;
    }
    
    void setValueEnabled(const T& v, bool b) {
      TRadioButtonBase* btn = getButton(v);
      if (btn)
        btn->setEnabled(b);
    }
};

class TRadioButtonBase: 
  public TButtonBase
{
    typedef TButtonBase super;
    PRadioStateModel _state;
  public:
    TRadioButtonBase(TWindow *p, 
                     const string &t,
                     TRadioStateModel *state = NULL)
      :super(p,t)
    {
      _state = state;
      if (_state)
        _state->add(this);
    }
    ~TRadioButtonBase() {
      if (_state)
        _state->remove(this);
    }
    
    void setModel(TRadioStateModel *model) {
      _state = model;
    }
    
    bool isSelected() const {
      return _state ? (_state->getCurrent() == this) : false;
    }
    void setDown(bool down=true);
    void _setDown(bool);
    
    void mouseLDown(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void mouseEnter(int,int,unsigned);
    void mouseLeave(int,int,unsigned);
    
    bool isDown() const;
};

} // namespace toad

#endif
