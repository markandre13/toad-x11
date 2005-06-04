/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_STL_DEQUE_HH
#define _TOAD_STL_DEQUE_HH

#include <deque>
#include <toad/table.hh>

namespace toad {

using namespace std;

template <class T>
class GDeque:
  public TTableModel
{
  private:
    deque<T> data;

  public:
    typedef typename deque<T>::const_iterator const_iterator;
    typedef typename deque<T>::iterator iterator;
    typedef typename deque<T>::size_type size_type;
    
    const T& operator[](size_type n) const { return data[n]; }
    void set(size_type n, const T &x) {
      data[n] = x;
      reason = CONTENT;
      where = n;
      TTableModel::size  = 1;
      sigChanged();
    }
    const T& at(size_type n) const { return data.at(n); }
    T& front() { return data.front(); }
    T& back() { return data.back(); }
    const T& front() const { return data.front(); }
    const T& back() const { return data.back(); }
    void push_back(const T &x) {
      data.push_back(x);
      reason = INSERT_ROW;
      where = data.size() - 1;
      TTableModel::size  = 1;
      sigChanged();
    }
    iterator insert(iterator p, const T &x) {
      reason = INSERT_ROW;
      TTableModel::size  = 1;
      where = p - begin();
      iterator i = data.insert(p, x);
//      cerr << "GVector<T>::insert: where = " << where << ", size = 1" << endl;
      sigChanged();
      return i;
    }
    void push_front(const T &x) {
      reason = INSERT_ROW;
      TTableModel::size  = 1;
      where = 0;
      data.push_front(x);
//      cerr << "GVector<T>::insert: where = " << where << ", size = 1" << endl;
      sigChanged();
    }
    iterator erase(iterator p) {
      reason = REMOVED_ROW;
      TTableModel::size  = 1;
      where = p - begin();
      iterator i = data.erase(p);
      sigChanged();
      return i;
    }
    iterator erase(iterator p, iterator e) {
      reason = REMOVED_ROW;
      TTableModel::size  = e-p+1;
      where = p - begin();
      iterator i = data.erase(p, e);
      sigChanged();
      return i;
    }
    void pop_back() {
      if (data.empty())
        return;
      reason = REMOVED_ROW;
      TTableModel::size  = 1;
      where = data.size()-1;
      data.pop_back();
      sigChanged();
    }
    // swap
    // clear
    // resize
    // operator=, copy constructor, ...
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }
    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    size_type size() const { return data.size(); }
    size_type maxsize() const { return data.max_size(); }
    size_type capacity() const { return data.capacity(); }
    bool empty() const { return data.empty(); }
    void reserve(size_type n) { data.reserve(n); }
};

} // namespace toad

#endif
