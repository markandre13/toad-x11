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

#ifndef GSmartPointer
#define GSmartPointer GSmartPointer

/**
 * \defgroup smartpointer Smartpointer
 *
 * Smartpointer's are pointers that delete an object when no smartpointer
 * references a previously referenced objects anymore.
 *
 * The usage style within TOAD looks like this
 *
 * @code
 * class MyClass: 
 *   public TSmartObject
 * {
 *    ...
 * };
 * typedef GSmartPointer<TMyClass> PMyClass
 * @endcode
 *
 * And common usage would look like this:
 *
 * @code
 * PMyClass ptr1, ptr2, ptr3;
 * ptr1 = new MyClass();
 * ptr2 = ptr1
 * ptr3 = ptr2;
 *
 * ptr1 = NULL;
 * ptr2 = ptr3;
 * ptr3 = new MyClass();
 * ptr2 = ptr3;
 * @endcode
 *
 * The last instruction will then delete the first referenced object.
 *
 * The approach of smart pointers is usually slower than a garbage 
 * collection in other languages but helps in situations when things 
 * get worse.
 *
 * @note
 * @li
 *   When using smartpointers together with arrays, the smartpointer
 *   must only be used with the first entry. Iterating the array with
 *   smartpointer would be destructive or slow or both.
 * @li
 *   Theses classes aren't thread safe.
 * @li
 *   These classes are able to make a difference between objects allocated
 *   on the heap, which must be destroyed when not referenced anymore and
 *   objects on the stack, which must not be destroyed.
 * @li
 *   There's a similar template in the C++ standard library \c std::auto_ptr.
 * @li
 *   This smartpointer implemententation differs from CORBA. Ie. when in
 *   someone writes
 *
 * @code
 * Object *p = new Object();
 * Object_var v1 = p;
 * Object_var v2 = p;
 * @endcode
 *
 * it will cause a segmentation fault since the reference counter isn't
 * incremented by `ObjVar<T>& ObjVar::operator=( T* ptr )'.
 * But TOAD's `GSmartPointer<T>& GSmartPointer::operator =(T *p)' will
 * increment it so code like
 *
 * @code
 * TObject *p = new TObject();
 * PObject v1 = p;
 * PObject v2 = p;
 * @endcode
 *
 * will destroy the object when BOTH PPointer objects get destroyed or
 * loose their reference on the object.
 */

#include <cstdlib>
#include <cassert>
#include <cstdio>
// #include <iostream>

namespace toad {

using namespace std;

/**
 * @ingroup smartpointer
 *
 * Classes whose objects shall be referenced by smartpointers must
 * inherit from this class.
 */
class TSmartObject
{
    static const unsigned nodelete = (unsigned)-1;
  public:
    TSmartObject();
    void * operator new(std::size_t size);
    void * operator new(std::size_t n, void* p);
    void operator delete(void *ptr);
    virtual ~TSmartObject();
//  private:
    unsigned _toad_ref_cntr;
};

/**
 * @ingroup smartpointer
 *
 * Template class for smartpointer.
 */
template <class T>
class GSmartPointer
{
  private:
    static const unsigned nodelete = (unsigned)-1;
    
  public:
    /* A reference count template that is used to encapsulate an object */
  
    GSmartPointer() {
      _ptr = 0;
    }

    GSmartPointer(T* p) {
      _ptr = 0;
      _set(p);
    }

    GSmartPointer(const GSmartPointer<T>& p) {
      _ptr = 0;
      _set(p._ptr);
    }

    template <class T2>
    GSmartPointer(const GSmartPointer<T2>& p) {
      _ptr = 0;
      _set(p._ptr);
    }

    ~GSmartPointer() {
      _set(0);
    }
    
    GSmartPointer<T>& operator =(T *p) {
      _set(p);
      return *this;
    }

    GSmartPointer<T>& operator =(const GSmartPointer<T>& p) {
      _set(p._ptr);
      return *this;
    }

    T* operator->() const {
      return _ptr;
    }
    
    operator T*() const {
      return _ptr;
    }

#if 0
    operator void*() const {
      return _ptr;
    }
    
    operator bool() const {
      return _ptr;
    }
#endif

//  private:    
    T* _ptr;
    void _set(T *p) {
      if (_ptr==p)
        return;
      if (_ptr && _ptr->_toad_ref_cntr!=nodelete) {
        _ptr->_toad_ref_cntr--;
        if (_ptr->_toad_ref_cntr==0)
          delete _ptr;
      }
      _ptr = p;
      if (_ptr && _ptr->_toad_ref_cntr!=nodelete)
        _ptr->_toad_ref_cntr++;
    }
};

} // namespace toad

#endif
