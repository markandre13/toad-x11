/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2006 by Mark-André Hopf <mhopf@mark13.org>
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

class TFigureEditor;
class TFigure;
class TFGroup;

/**
 * \ingroup figure
 */
typedef set<TFigure*> TFigureSet;
typedef vector<TFigure*> TFigureVector;

class TFigureAtDepthList;
class TFigureAttributes;

/**
 * \ingroup figure
 */
class TFigureModel:
  public TModel, public TSerializable
{
//    friend class TFigureWindow; // debugging
  protected:
    typedef std::vector<TFigure*> TStorage;
  public:
    typedef TStorage::iterator iterator;
    typedef TStorage::const_iterator const_iterator;
    iterator begin() { return storage.begin(); }
    iterator end() { return storage.end(); }
    const_iterator begin() const { return storage.begin(); }
    const_iterator end() const { return storage.end(); }
  
    /**
     * Kind of modification that took place.
     */
    enum { MODIFY,    // before modification (figures)
           MODIFIED,  // after modification (figures)
           DELETE, 
           ADD,       // after figures are added
           REMOVE,    // before figures are removed
           GROUP, UNGROUP,
           TRANSLATE,
           ROTATE,
           _UNDO_GROUP
    } type;
    //! additional information for type attribute
    TFigureSet figures;
    //! additional information for type attribute
    TFigure *figure;
#if 0    
    bool _modified;
    bool isModified() const { return _modified; }
    void setModified(bool m) {
      _modified = m;
    }
#endif
    TFigureModel();
    TFigureModel(const TFigureModel&);
    ~TFigureModel();
    
    void push_back(TFigure *f) { add(f); }
    void add(TFigure*);
    void erase(TFigure*);
    void add(TFigureVector&);
    virtual void erase(TFigureSet&);
    size_t size() const { return storage.size(); }
    bool empty() const { return storage.empty(); }
    
    
    void insert(TFigureAtDepthList &store);

    void translate(const TFigureSet&, int dx, int dy);
    void translateHandle(TFigure *figure, unsigned handle, int dx, int dy, unsigned);
    bool startInPlace(TFigure *figure, TFigureEditor *fe = 0);
    TFigure* group(TFigureSet &);
    void _undoGroup(TFGroup*, TFigureAtDepthList &figures);

    void ungroup(TFigureSet &grouped, TFigureSet *ungrouped);
    
    void setAttributes(TFigureSet &set, const TFigureAttributes *attributes);
    
    void erase(const iterator&);
    void erase(const iterator&, const iterator&);

    void insert(const iterator &, TFigure*);
    void insert(const iterator &at, const iterator &from, const iterator &to);

    //! remove and delete all figures
    void clear();
    
    //! remove all figures but don't delete them
    void drop() {
      storage.clear();
    }

    SERIALIZABLE_INTERFACE_PUBLIC(toad::, TFigureModel)
  protected:
    TStorage storage;
};

/**
 * \ingroup figure
 *
 * This class and the corresponding insert method in TFigureModel
 * are required to implement the undo/redo functionality.
 *
 * The destructor of this class deletes all figures owned by this
 * class.
 */
class TFigureAtDepthList
{
    friend class TFigureModel;
    struct figdep_t {
      figdep_t(TFigure *f, unsigned d) {
        figure = f;
        depth  = d;
      }
      TFigure *figure;
      unsigned depth;
    };
    typedef vector<figdep_t> TStore;
    TStore store;
  public:
    ~TFigureAtDepthList();
  
    void push_back(TFigure *figure, unsigned depth) {
      store.push_back(figdep_t(figure, depth));
    }
    /**
     * Drop references to all figures (Clear list and don't delete figures.)
     */
    void drop() {
      store.clear();
    }
};

typedef GSmartPointer<TFigureModel> PFigureModel;

} // namespace toad

#endif
