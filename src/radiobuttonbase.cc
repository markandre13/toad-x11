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

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/radiobuttonbase.hh>

using namespace toad;

/**
 * \class toad::TRadioStateModel
 * Coordinates several radio buttons.
 *
 * A fine variant to use this class is the following one:
 * 
 * \code
 * TMyWindow::TMyWindow() {
 *   GRadioStateModel<int> *state = new GRadioStateModel<int>();
 *   state->add(new TRadioButton(this, "One"),   1);
 *   state->add(new TRadioButton(this, "Two"),   2);
 *   state->add(new TRadioButton(this, "Three"), 3);
 *   CONNECT_VALUE(state->sigChanged, this, action, state);
 * }
 *
 * void TMyWindow::action(int id) {
 *   ...
 * }
 * \endcode
 */

TRadioStateModel::TRadioStateModel()
{
  _current = NULL;
  _temporary = NULL;
}

void 
TRadioStateModel::add(TRadioButtonBase *rb)
{
  // to call button
  listener.push_back(rb);
  
  // to be called from button
  connect(rb->sigActivate, this, &TRadioStateModel::setCurrent, rb);
}

void 
TRadioStateModel::remove(TRadioButtonBase *rb)
{
  TListenerBuffer::iterator p,e;
  p = listener.begin();
  e = listener.end();
  while(p!=e) {
    if (*p==rb) {
      listener.erase(p);
      break;
    }
    ++p;
  }  

  disconnect(rb->sigActivate, this);
}

void 
TRadioStateModel::setCurrent(TRadioButtonBase *btn)
{
  if (btn==_current)
    return;
  TRadioButtonBase *old = _current;
  _current = btn;
  if (old)
    old->_setDown(false);
  if (_current)
    _current->_setDown(true);
  sigChanged();
}

void 
TRadioStateModel::setTemporary(TRadioButtonBase *btn)
{
  if (btn==_temporary)
    return;
  
  _temporary = btn;
  
  if (_current)
    _current->invalidateWindow();  
}

#if 0
void 
TRadioStateModel::setValue(int value)
{
  TListenerBuffer::iterator p,e;
  p = listener.begin();
  e = listener.end();
  while(p!=e) {
    if ((*p)->getValue()==value) {
      setCurrent(*p);
      return;
    }
    p++;
  }
  cerr << "TRadioState: `" << value << "' no such value" << endl;
  p = listener.begin();
  e = listener.end();
  
  while(p!=e) {
    cerr << (*p)->getValue() << " ";
    p++;
  }
  cerr << endl;
}

int 
TRadioState::getValue() const
{
  return _current ? _current->getValue() : 0;
}
#endif

/**
 * \class toad::TRadioButtonBase
 * The base class for all radio buttons.
 *
 * \sa TRadioState
 */

//! <I>Note: Only 'true' is implemented yet, which is the default value</I>
//---------------------------------------------------------------------------
void 
TRadioButtonBase::setDown(bool down)
{
  setFocus();
  if (_state) {
    _state->setCurrent(this);
  } else {
    if (!isDown()) {
      super::setDown(true);
    } else {
      super::setDown(down);
    }
  }
}

void 
TRadioButtonBase::_setDown(bool down)
{
  if (bDown==down)
    return;
  super::setDown(down);
}

void
TRadioButtonBase::mouseLDown(int x, int y, unsigned m)
{
  super::mouseLDown(x, y, m);
  if (_state)
    _state->setTemporary(this);
}

void
TRadioButtonBase::mouseLeave(int x, int y, unsigned m)
{
  if (_state && bDown)
    _state->setTemporary(NULL);
  super::mouseLeave(x, y, m);
}

void
TRadioButtonBase::mouseEnter(int x, int y, unsigned m)
{
  if (_state && bDown)
    _state->setTemporary(this);
  super::mouseEnter(x, y, m);
}

void
TRadioButtonBase::mouseLUp(int x, int y, unsigned m)
{
  if (_state)
    _state->setTemporary(NULL);
  super::mouseLUp(x, y, m);
}

bool
TRadioButtonBase::isDown() const
{
  bool down = (bDown && bInside) || isSelected();
  if (_state && _state->getTemporary()!=NULL && _state->getTemporary()!=this)
    down = false;
  return down;
}
