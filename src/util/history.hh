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

#ifndef GHistory
#define GHistory GHistory

#include <toad/connect.hh>
#include <deque>
#include <stdexcept>
#include <cassert>

/**
 * \class toad::GHistory
 *
 * This template is used to provide a history for classes like
 * TUndoable or web browsers.
 *
 * \todo
 *   \li make it a model
 *   \li 
 *     provide a undo/redo tree instead of a simple timeline which
 *     opens a new branch after calls to goBack() and add() call.
 *     (there's a paper on this, find it and mention it here)
 */

namespace toad {

template<class T>
class GHistory
{
    typedef std::deque<T> TStorage;
    TStorage storage;
    typename TStorage::iterator p;
    unsigned bs, fs;
    unsigned size;
    
  public:
    GHistory() {
      p=storage.begin();
      bs = fs = 0;
      size = 20;
    }
    GHistory(const GHistory &history) {
      assert(history.storage.empty());
      p=storage.begin();
      bs = fs = 0;
      size = 20;
    }
    void setSize(unsigned s) {
      s = size;
    }
    void clear() {
      storage.clear();
      p=storage.begin();
      bs = fs = 0;
      signal();
    }
    void add(const T& d) {
      storage.erase(p, storage.end());
      storage.push_back(d);
      p = storage.end();
      fs = 0;
      if (storage.size()>size)
        storage.erase(storage.begin());
      bs=storage.size();
      signal();
    }
    const T& getCurrent() const {
      if (p==storage.begin())
        throw std::runtime_error("GHistory: no current");
      return *(p-1);
    }
    void goBack() {
      if (p==storage.begin())
        throw std::runtime_error("GHistory: can't go back");
      p--;
      bs--;
      fs++;
      signal();
    }
    void goForward() {
      if (p==storage.end())
        throw std::runtime_error("GHistory: can't go forward");
      p++;
      bs++;
      fs--;
      signal();
    }
    void signal()
    {
      sigEnableBack();    // bs!=0
      sigEnableForward(); // fs!=0
    }
    unsigned getBackSize() const { return bs; }
    unsigned getForwardSize() const { return fs; }

    TSignal sigEnableBack, sigEnableForward;
};

} // namespace toad

#endif
