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

#include <toad/pointer.hh>
#include <iostream>

using namespace toad;

/**
 * \ingroup smartpointer
 * \class toad::TSmartObject
 *
 * Classes whose objects shall be referenced by smartpointers must
 * inherit from this class.
 *
 * \todo
 *   \li
 *     Does someone know of a better way to detect whether the
 *     object was allocated on the stack? the current method
 *     looks dirty and isn't even thread safe.
 */

static char * stack_head;
enum stacktype_t {
  STACK_UNKNOWN,
  STACK_DOWN,
  STACK_UP
};
static enum stacktype_t stack_type = STACK_UNKNOWN;

/**
 * Initialize the algorithm which determined whether an object is on
 * the stack or on the heap.
 *
 * All TSmartObject's created before invocation of this method aren't
 * destroyed when they aren't referenced anymore. This is a sane
 * behaviour for static objects but might miss some objects allocated
 * by constructors.
 *
 * \param head
 *   Pointer to an object on the stack. (toad::initialize uses the address
 *   of 'argv' from the main function.)
 */
void
TSmartObject::initialize(void *head)
{
  char b;
  stack_head = (char*)head;
  if (stack_head > &b) {
    stack_type = STACK_DOWN;
  } else {
    stack_type = STACK_UP;
  }
}

TSmartObject::TSmartObject()
{
  _toad_ref_cntr = 0;

  char dummy;
  char *p = (char*)this;
  
  switch(stack_type) {
    case STACK_DOWN:
      if (stack_head >= p && p>=&dummy)
        _toad_ref_cntr = notheap;
      break;
    case STACK_UP:
      if (stack_head <= p && p<=&dummy)
        _toad_ref_cntr = notheap;
      break;
    case STACK_UNKNOWN:
      _toad_ref_cntr = notheap;
      break;
  }
}

TSmartObject::~TSmartObject() {
  if (_toad_ref_cntr!=0 && _toad_ref_cntr!=notheap) {
    cerr << "warning: object with pending references destroyed" << endl;
    // Another idea would be to find all associated smart pointers
    // and set 'em to NULL or to throw an exception
  }
}
