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

#ifndef _TOAD_STL_VECTOR_HH
#define _TOAD_STL_VECTOR_HH

#include <vector>
#include <toad/table.hh>

namespace toad {

using namespace std;

template <class T>
class GVector:
  public TTableModel
{
  private:
    vector<T> data;

  public:
    size_t getRows() const { return data.size(); }
    size_t getCols() const { return 1; }

    typedef typename vector<T>::const_iterator const_iterator;
    typedef typename vector<T>::iterator iterator;
    typedef typename vector<T>::size_type size_type;
    
    const T& operator[](size_type n) const { return data[n]; }
    T& operator[](size_type n) { return data[n]; }
    const T& at(size_type n) const { return data.at(n); }
    T& at(size_type n) { return data.at(n); }
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
    // pop_back
    // insert
    iterator erase(iterator p) {
      reason = REMOVED_ROW;
      TTableModel::size = 1;
      where = p - data.begin();
      p = data.erase(p);
      sigChanged();
      return p;
    }
    iterator erase(iterator p, iterator e) {
      reason = REMOVED_ROW;
      TTableModel::size = e-p;
      where = p - data.begin();
      p = data.erase(p, e);
      sigChanged();
      return p;
    }
    // swap
    void clear() {
      reason = REMOVED_ROW;
      TTableModel::size = data.size();
      where = 0;
      data.clear();
      sigChanged();
    }
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
