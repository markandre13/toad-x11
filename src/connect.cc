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

/**
 * @defgroup callback Signal & Slot
 *
 * TOAD's implementation of callbacks to member functions.
 */

using namespace std;

#ifndef TEST_CONNECT

#include <cstddef>
#include <toad/toad.hh>
#include <toad/connect.hh>

// debug only:
#include <iostream>
#include <assert.h>

#else

#include <cstddef>
#include "connect.hh"
#include <iostream>
#include <cassert>

using namespace toad;

struct TMySource 
{
  TSignal sigAction;
  int value;
  int getValue() { return value; }
};

struct TMyDestination
{
  void DoIt() { cout << "DoIt" << endl; exit(0); }
  void DoIt(int n) { cout << "DoIt: " << n << endl; exit(0); }
  void DoIt2(int n=7) { cout << "DoIt2: " << n << endl; exit(0); }
};

static
void DoItFunc()
{
  exit(0);
}

static
void DoIt2Func(int n)
{
  cout << "DoIt2Func " << n << endl;
  exit(0);
}

int
main()
{
  TMySource *source = new TMySource();
  source->value = 42;
  TMyDestination *destination = new TMyDestination();

  // plain connect (inline & macro variant)
  //----------------------------------------
#ifdef TEST1
  connect(
    source->sigAction,
    destination, &TMyDestination::DoIt);
#endif

#ifdef TEST2
  CONNECT(
    source->sigAction,
    destination, DoIt);
#endif

  // connect with one parameter from method (inline & macro variant)
  //-----------------------------------------------------------------
#ifdef TEST3
  connect_value_of(
    source->sigAction, 
    destination, &TMyDestination::DoIt,
    source, &TMySource::getValue);
#endif

#ifdef TEST4
  CONNECT_VALUE_OF(
    source->sigAction, 
    destination, DoIt,
    source, getValue());
#endif

  // connect with one parameter from variable (inline only)
  //--------------------------------------------------------
#ifdef TEST5
  connect_value_of(
    source->sigAction, 
    destination, &TMyDestination::DoIt,
    &source->value);
#endif

  // connect with one parameter from getValue method (inline & macro variant)
  //---------------------------------------------------------------
#ifdef TEST6
  connect_value(
    source->sigAction, 
    destination, &TMyDestination::DoIt,
    source);
#endif

#ifdef TEST7
  CONNECT_VALUE(
    source->sigAction,
    destination, DoIt,
    source);
#endif

  // connection with code (macro only)
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#ifdef TEST8
  TSignalLink *r;
  BGN_CONNECT_CODE(source->sigAction, destination, source, &r);
    dst->DoIt(src->getValue()*10);
  END_CONNECT_CODE();
#endif

// the usual connect doesn't work with predefined arguments, but
// '*_CONNECT_CODE'
#if 0
  connect(
    source->sigAction,
    destination, &TMyDestination::DoIt2);
#endif

#ifdef TEST9
  BGN_CONNECT_CODE(source->sigAction, destination, source, NULL);
    dst->DoIt2();
  END_CONNECT_CODE();
#endif

  // connection with one predefined value
#ifdef TEST10
  connect(
    source->sigAction,
    destination, &TMyDestination::DoIt2, 43);
#endif

  // plain connect with function
#ifdef TEST11
  connect(source->sigAction, DoItFunc);
#endif

#ifdef TEST12
  connect(source->sigAction, DoIt2Func, 43);
#endif


  source->sigAction();

  exit(1);
}

#endif

//---------------------------------------------------------------------------

namespace toad {

TSignalLink::TSignalLink()
{
  next = 0;
  lock = false;
  dirty = false;
}

TSignalLink::~TSignalLink() {}
void* TSignalLink::objref() {return NULL; }
TSignalLink::TMethod TSignalLink::metref() {return NULL; }

TSignal::TSignal()
{
  _list = NULL;
#ifdef TOAD_SECURE
  delayedtrigger=0;
#endif
}

TSignal::~TSignal()
{
#ifdef TOAD_SECURE
  assert(delayedtrigger==0);
#endif
  remove();
}

/**
 * Lock the signal.
 *
 * The callbacks connected with this signal aren't called. Instead a dirty
 * flag will be set.
 *
 * The behaviour when callbacks are added or removed while a lock is active
 * is undefined.
 *
 * \sa unlock
 */
void
TSignal::lock()
{
  if (_list)
    _list->lock = true;
}

/**
 * Unlock the signal and trigger it in case it was triggered while the
 * lock was active.
 *
 * The behaviour when callbacks are added or removed while a lock is active
 * is undefined.
 *
 * \sa lock
 */
void
TSignal::unlock()
{
  if (_list) {
    bool flag = _list->lock && _list->dirty;
    _list->lock = false;
    _list->dirty = false;
    if (flag)
      trigger();
  }
}

/**
 * This method prints information for debugging purposes.
 */
void
TSignal::print()
{
  unsigned count = 0;
  TSignalLink *p = _list;
  while(p) {
    ++count;
    p = p->next;
  }
  cerr << "signal owns " << count << " links" << endl;
}

TSignalLink*
TSignal::add(TSignalLink *node)
{
  if (!node)
    return NULL;

  node->next = NULL;
  if (_list==NULL) {
    _list = node;
  } else {
    TSignalLink *p = _list;
    while(p->next)
      p=p->next;
    p->next = node;
  }
  return node;
}

void TSignal::remove()
{
//  cout << "remove all" << endl;
  TSignalLink *p;
  while(_list) {
    p = _list;
    _list = _list->next;
    delete p;
  }
}

void TSignal::remove(void(*f)(void))
{
//  cout << "remove function" << endl;
  #warning "no code to remove function!"
}

void TSignal::remove(void *object, TSignalLink::TMethod method)
{
//  cout << "remove object/method" << endl;
  if (!object)
    return;
  TSignalLink *p = _list, *last = NULL;
  while(p) {
    if (p->objref()==object &&
        p->metref()==method) {
//      cout << "found method" << endl;
      if (p==_list) {
        _list = p->next;
        delete p;
        p = _list;
        continue;
      } else {
        last->next = p->next;
        delete p;
        p = last->next;
        continue;
      }
    }
    last = p;
    p = p->next;
  }
}

// Remove all connections for an object
//-------------------------------------
void TSignal::remove(void *object)
{
//  cout << "remove all for one object" << endl;
  if (!object)
    return;
  TSignalLink *p = _list, *last = NULL;
  while(p) {
    if (p->objref()==object) {
//      cout << "found" << endl;
      if (p==_list) {
        _list = p->next;
        delete p;
        p = _list;
        continue;
      } else {
        last->next = p->next;
        delete p;
        p = last->next;
        continue;
      }
    }
    last = p;
    p = p->next;
  }
}

void TSignal::remove(TSignalLink *node)
{
  if (!node)
    return;
  TSignalLink *p = _list, *last = NULL;
  while(p) {
    if (p==node) {
//      cout << "found node" << endl;
      if (p==_list) {
        _list = p->next;
        delete p;
        p = _list;
        continue;
      } else {
        last->next = p->next;
        delete p;
        p = last->next;
        continue;
      }
    }
    last = p;
    p = p->next;
  }
}

/**
 * Invoke all actions connected to the signal.
 *
 * Actions are executed before returning when the signal isn't locked.
 *
 * When the signal is locked, it is triggered during 'unlock'.
 * 
 * \return 'false' when the signal is connected to anything
 * \sa delayedTrigger, unlock
 */
bool 
TSignal::trigger()
{
  if (!_list) return false;
  
  if (_list->lock) {
    _list->dirty = true;
    return true;
  }
  
  TSignalLink *p = _list;
  while(p) {
    p->execute();
    p = p->next;
  }
  return true;
}

#ifndef TEST_CONNECT

class TCommandDelayedTrigger:
  public TCommand
{     
    TSignal *signal;
  public:
    TCommandDelayedTrigger(TSignal *s):signal(s){}
    void execute() {
#ifdef TOAD_SECURE
      signal->delayedtrigger--;
#endif
      signal->trigger();
    }
};

/**   
 * Invoke all actions connected to the signal from the message queue.
 *
 * Unlike <code>trigger()<code/> this method returns <i>before</i>
 * the signal is triggered. Execution will start after all events  
 * in the message queue (at the time this method was invoked) are 
 * processed.
 *
 * A risk of this function is that the signal get's destroyed before
 * execution. TOAD catches this error when compiled with
 * TOAD_SECURE is defined during compilation of the TOAD library.
 *
 * \return false' when the signal is connected to anything
 * \sa trigger
 */
bool TSignal::delayedTrigger()
{
  if (!_list) return false;
#ifdef TOAD_SECURE
  delayedtrigger++;
#endif
  TOADBase::sendMessage(new TCommandDelayedTrigger(this));
}

#endif

} // namespace toad
