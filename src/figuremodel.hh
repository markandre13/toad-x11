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

#ifndef _TOAD_FIGUREMODEL_HH
#define _TOAD_FIGUREMODEL_HH

#include <vector>
#include <set>
#include <toad/toad.hh>
#include <toad/model.hh>
#include <toad/io/serializable.hh>

namespace toad {

class TFigure;

typedef set<TFigure*> TFigureSet;

/**
 * \ingroup figure
 */
class TFigureModel:
  public TModel, public TSerializable
{
    class TUndoRemove;
    friend class TUndoRemove;
    class TUndoInsert;
    friend class TUndoInsert;

    friend class TFigureWindow; // debugging
    typedef std::vector<TFigure*> TStorage;
  public:
    /**
     * Kind of modification that took place.
     */
    enum { MODIFIED, DELETE, 
           ADD, REMOVE,
           GROUP, UNGROUP,
           TRANSLATE,
           ROTATE,
    } type;
    TFigureSet figures;
    
    class iterator
    {
      friend class TFigureModel;
        TFigureModel *owner;
        TStorage::iterator p;
      public:
        iterator(): owner(0) {}
        iterator(const iterator &a): owner(a.owner), p(a.p) {}
        iterator(TFigureModel *aOwner, TStorage::iterator aPointer):
          owner(aOwner), p(aPointer) {}
        friend bool operator==(const iterator&, const iterator&);
        friend bool operator!=(const iterator&, const iterator&);
        iterator& operator++() { ++p; return *this; }
        iterator& operator--() { --p; return *this; }
        iterator operator++(int) { iterator tmp=*this; ++(*this); return tmp; }
        iterator operator--(int) { iterator tmp=*this; --(*this); return tmp; }
        TFigure*& operator*() const { return *p; }
    };

#if 0
    class const_iterator
    {
      friend class TFigureModel;
        const TFigureModel *owner;
        TStorage::const_iterator p;
      public:
        const_iterator(): owner(0) {}
        const_iterator(const const_iterator &a): owner(a.owner), p(a.p) {}
        const_iterator(const TFigureModel *aOwner, const TStorage::iterator aPointer):
          owner(aOwner), p(aPointer) {}
        friend bool operator==(const const_iterator&, const const_iterator&);
        friend bool operator!=(const const_iterator&, const const_iterator&);
        const_iterator& operator++() { ++p; return *this; }
        const_iterator& operator--() { --p; return *this; }
        const_iterator operator++(int) { const_iterator tmp=*this; ++(*this); return tmp; }
        const_iterator operator--(int) { const_iterator tmp=*this; --(*this); return tmp; }
        const TFigure* operator*() const { return *p; }
    };
#endif
    
    TFigureModel();
    TFigureModel(const TFigureModel&);
    ~TFigureModel();
    
    void add(TFigure*);
    void erase(TFigure*);
    void add(TFigureSet&);
    void erase(TFigureSet&);
    
    TFigure* group(TFigureSet &);
    void ungroup(TFigureSet &grouped, TFigureSet *ungrouped);
    
    void erase(const iterator&);
    void erase(const iterator&, const iterator&);

    void insert(const iterator &, TFigure*);
    void insert(const iterator &at, const iterator &from, const iterator &to);

    //! remove all figures
    void clear();
    
    iterator begin() {
      return iterator(this, storage.begin());
    }
    iterator end() {
      return iterator(this, storage.end());
    }
/*
    const_iterator begin() const {
      return iterator(this, storage.begin());
    }
    const_iterator end() const {
      return const_iterator(this, storage.end());
    }
*/
    SERIALIZABLE_INTERFACE_PUBLIC(toad::, TFigureModel)
  protected:
    TStorage storage;
};

inline bool operator==(const TFigureModel::iterator &a,
                const TFigureModel::iterator &b)
{
  return a.p == b.p;
}

inline bool operator!=(const TFigureModel::iterator &a,
                const TFigureModel::iterator &b)
{
  return a.p != b.p;
}

typedef GSmartPointer<TFigureModel> PFigureModel;

} // namespace toad

#endif
