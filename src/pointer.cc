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
#include <toad/stacktrace.hh>

#define DBM(X)

using namespace toad;

namespace {

static const unsigned heaptrack_size = 256;

struct THeapTrack
{
  void *start;
  void *end;
};

THeapTrack heap[heaptrack_size];
unsigned heapidx = 0;

} // namespace

TSmartObject::TSmartObject() {
  // We do a range check instead of a simple pointer validation because 
  // this object might to have been this first one in the list of super 
  // classes and the TSmartObject is somewhere inside of it.

  DBM(
  cerr << "check for " << this << endl;
  for(unsigned i=0; i<heapidx; ++i) {
    cerr << "  heap[" << i << "] = " << heap[i].start << " - " << heap[i].end << endl;
  }
  )

  if (heapidx && 
      heap[heapidx-1].start<=this && 
      this<=heap[heapidx-1].end) 
  {
    --heapidx;
    _toad_ref_cntr = 0;
  } else {
    _toad_ref_cntr = nodelete;
  }
}
    
/**
 * This method is called, when a smart object is created
 * with new, which means that a smart pointer is allowed to
 * delete the object.
 */
void * 
TSmartObject::operator new(std::size_t size) {
  assert(heapidx<heaptrack_size);

  char * ptr = new char[size];
  heap[heapidx].start = ptr;
  heap[heapidx].end   = ptr+size;
  
  DBM(
  cerr << "heap[" << heapidx << "] = " << (void*)ptr << " - " << (void*)(ptr+size) << endl;
  )
  
  ++heapidx;
  return ptr;
}

void * 
TSmartObject::operator new(std::size_t n, void* p) {
  cout << "new2 TSmartObject" << endl;
  return p;
}
    
TSmartObject::~TSmartObject() {
  if (_toad_ref_cntr!=0 && _toad_ref_cntr!=nodelete) {
    cerr << "warning: object with pending references destroyed" << endl;
    printStackTrace();
    // Another idea would be to find all associated smart pointers
    // and set 'em to NULL or to throw an exception
  }
}
