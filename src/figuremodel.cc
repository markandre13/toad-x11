/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/figure.hh>
#include <toad/figuremodel.hh>
#include <toad/undo.hh>
#include <toad/undomanager.hh>
#include <toad/io/binstream.hh>

using namespace toad;

TFigureModel::TFigureModel()
{
//  cerr << "new TFigureModel " << this << endl;
}

TFigureModel::TFigureModel(const TFigureModel &m)
{
  cerr << "copy constructed TFigureModel " << this << " from " << &m << endl;
  for(TStorage::const_iterator p = m.storage.begin();
      p != m.storage.end();
      ++p)
  {
    storage.push_back( static_cast<TFigure*>( (*p)->clone() ) );
  }
}

TFigureModel::~TFigureModel()
{
  type = DELETE;
  sigChanged();
  clear();
}

class TUndoInsert:
  public TUndo
{
    TFigureModel *model;
    TFigureSet figures;
  public:
    TUndoInsert(TFigureModel *model) {
      this->model = model;
    }
    void insert(TFigure *f) {
      figures.insert(f);
    }
  protected:
    void undo() {
      model->erase(figures);
      figures.clear();
    }
    bool getUndoName(string *name) const {
      *name = "Undo: Insert";
      return true;
    }
    bool getRedoName(string *name) const {
      *name = "Redo: Remove";
      return true;
    }
};

class TUndoRemove:
  public TUndo
{
    TFigureAtDepthList figures;
    TFigureModel *model;
  public:
    TUndoRemove(TFigureModel *model) {
      this->model = model;
    }
    void insert(TFigure *f, unsigned d)
    {
//cerr << "TUndoRemove: store figure " << f << " at depth " << d << endl;
      figures.push_back(f, d);
    }
  protected:
    void undo() {
      model->insert(figures);
    }
    bool getUndoName(string *name) const {
      *name = "Undo: Remove";
      return true;
    }
    bool getRedoName(string *name) const {
      *name = "Redo: Insert";
      return true;
    }
};

class TUndoGroup:
  public TUndo
{
    TFigureAtDepthList figures;
    TFigureModel *model;
    TFGroup *group;
  public:
    TUndoGroup(TFigureModel *model) {
      this->model = model;
    }
    ~TUndoGroup() {
      figures.drop(); // clear to avoid deleting the figures
    }
    void insert(TFigure *f, unsigned d)
    {
//cerr << "TUndoRemove: store figure " << f << " at depth " << d << endl;
      figures.push_back(f, d);
    }
    void setGroup(TFGroup *group) {
      this->group = group;
    }
  protected:
    void undo() {
      group->drop();
      model->_undoGroup(group, figures);
      figures.drop();
    }
    bool getUndoName(string *name) const {
      *name = "Undo: Group";
      return true;
    }
    bool getRedoName(string *name) const {
      *name = "Redo: Ungroup";
      return true;
    }
};

class TUndoUngroup:
  public TUndo
{
    TFigureModel *model;
    TFigureSet figures;
  public:
    TUndoUngroup(TFigureModel *model) {
      this->model = model;
    }
    void insert(TFigure *f) {
      figures.insert(f);
    }
  protected:
    void undo() {
      model->group(figures);
    }
    bool getUndoName(string *name) const {
      *name = "Undo: Ungroup";
      return true;
    }
    bool getRedoName(string *name) const {
      *name = "Redo: Group";
      return true;
    }
};

TFigureAtDepthList::~TFigureAtDepthList()
{
  for(TStore::iterator p=store.begin();
      p!=store.end();
      ++p)
  {
    delete p->figure;
  }
}

void 
TFigureModel::add(TFigure *figure) {
  TUndoInsert *undo = new TUndoInsert(this);
  undo->insert(figure);
  TUndoManager::registerUndo(this, undo);
  storage.push_back(figure);
  type = ADD;
  figures.clear();
  figures.insert(figure);
  sigChanged();
}

void 
TFigureModel::add(TFigureVector &newfigures) {
  figures.clear();
  type = ADD;
  TUndoInsert *undo = new TUndoInsert(this);
  for(TFigureVector::iterator p = newfigures.begin();
      p != newfigures.end();
      ++p)
  {
    storage.push_back(*p);
    figures.insert(*p);
    undo->insert(*p);
  }
  TUndoManager::registerUndo(this, undo);
  sigChanged();
}

void
TFigureModel::insert(TFigureAtDepthList &store)
{
  figures.clear();
  type = ADD;
      
  TUndoInsert *undo = new TUndoInsert(this);
  for(TFigureAtDepthList::TStore::iterator p=store.store.begin();
      p!=store.store.end();
      ++p)
  {
//cerr << "TUndoRemove: insert figure " << p->figure << " at depth " << p->depth << endl;
    storage.insert(
      storage.begin() + p->depth,
      p->figure);
    figures.insert(p->figure);
    undo->insert(p->figure);
  }
  TUndoManager::registerUndo(this, undo);
  store.drop();
  sigChanged();
}

void
TFigureModel::erase(TFigure *figure)
{
  TFigureSet set;
  set.insert(figure);
  erase(set);
}

void
TFigureModel::erase(TFigureSet &set)
{
  if (set.empty())
    return;

  type = REMOVE;
  figures.clear();
  figures.insert(set.begin(), set.end());
  sigChanged();
  
  TUndoRemove *undo = new TUndoRemove(this);

  unsigned depth = 0;
  for(TStorage::iterator p=storage.begin();
      p!=storage.end();
      ++p, ++depth)
  {
    TFigureSet::iterator q = figures.find(*p);
    if (q!=figures.end()) {
//      cerr << "  erase found figure at depth " << depth << endl;
      undo->insert(*p, depth);
      TStorage::iterator tmp = p;
      --tmp;
      storage.erase(p);
      p=tmp;
    }
  }
  TUndoManager::registerUndo(this, undo);
}

class TUndoTranslate:
  public TUndo
{
    TFigureModel *model;
    TFigureSet figures;
    int dx, dy;
  public:
    TUndoTranslate(TFigureModel *model, const TFigureSet &set, int dx, int dy) {
      this->model = model;
      this->figures.insert(set.begin(), set.end());
      this->dx = dx;
      this->dy = dy;
    }
  protected:
    void undo() {
//cout << "undo translate " << dx << ", " << dy << endl;
      model->translate(figures, dx, dy);
    }
    bool getUndoName(string *name) const {
      *name = "Undo: Move";
      return true;
    }
    bool getRedoName(string *name) const {
      *name = "Redo: Move";
      return true;
    }
};

void
TFigureModel::translate(const TFigureSet &set, int dx, int dy)
{
  if (dx==0 && dy==0)
    return;

//cout << "translate " << dx << ", " << dy << endl;
  figures.clear();
  figures.insert(set.begin(), set.end());
  type = MODIFY;
  sigChanged();
  for(TFigureSet::iterator p = set.begin();
      p!=set.end();
      ++p)
  {
    if ( !(*p)->mat) {
      (*p)->translate(dx, dy);
    } else {
      TMatrix2D m;
      m.translate(dx, dy);
      m.multiply((*p)->mat);
      *(*p)->mat = m;
    }
  }
  type = MODIFIED;
  sigChanged();

  TUndoTranslate *undo = new TUndoTranslate(this, set, -dx, -dy);
  TUndoManager::registerUndo(this, undo);

  _modified = true;
}

/**
 * Group a given set of figures.
 *
 * \param set
 *   A list of figures to group
 * \return
 *   The new group object or NULL in case the group would have only
 *   contained one or no objects.
 */
TFigure*
TFigureModel::group(TFigureSet &set)
{
  if (set.size()<2)
    return 0;
    
  TUndoGroup *undo = new TUndoGroup(this);
  
  unsigned count = 0;
  unsigned depth = 0;
  TFGroup *group = new TFGroup();
  undo->setGroup(group);
  TStorage::iterator last;
  for(TStorage::iterator p = storage.begin();
      p!=storage.end(); ++depth)
  {
    if (set.find(*p)!=set.end()) {
      group->gadgets.add(*p);
      undo->insert(*p, depth);
      unsigned pi = p - storage.begin();
      storage.erase(p);
      last = p = storage.begin() + pi;
      ++count;
    } else {
      ++p;
    }
  }
  if (count<2) {
    cerr << "count < 2" << endl;
    // delete group;
    // return 0;
  }
  
  TUndoManager::registerUndo(this, undo);
  
  group->calcSize();
  storage.insert(last, group);
  
  type = GROUP;
  figures.clear();
  figure = group;
  sigChanged();
  
  return group;
}

/**
 * Ungroup a given set of figures.
 *
 * \param grouped
 *    A set of figures from which all groups will be ungrouped.
 * \param ungrouped
 *    A pointer to set a of figures, which will take all non-groups
 *    and ungrouped groups from 'grouped'.
 *
 * grouped and ungrouped can be the same object.
 */
void
TFigureModel::ungroup(TFigureSet &grouped, TFigureSet *ungrouped)
{
  TFigureSet memo;
  for(TStorage::iterator p = storage.begin();
      p != storage.end();
      ++p)
  {
    if (grouped.find(*p)!=grouped.end()) {
      TFGroup *group = dynamic_cast<TFGroup*>(*p);
      if (group) {
        if (group->mat) {
          for(TFigureModel::iterator vp = group->gadgets.begin();
              vp != group->gadgets.end();
              ++vp)
          {
            TMatrix2D *m = new TMatrix2D(*group->mat);
            if ((*vp)->mat) {
              m->multiply((*vp)->mat);
              delete((*vp)->mat);
            }
            (*vp)->mat = m;
          }
        }
        int pi = p - storage.begin();
        storage.erase(p);
        p = storage.begin() + pi;

        storage.insert(p, group->gadgets.storage.begin(), group->gadgets.storage.end());
        p = storage.begin() + pi + group->gadgets.storage.size()-1;
        memo.insert(group->gadgets.begin(), group->gadgets.end());
        group->gadgets.erase(group->gadgets.begin(),group->gadgets.end());
        delete group;
      } else {
        memo.insert(*p);
      }
    }
  }
  ungrouped->clear();
  ungrouped->insert(memo.begin(), memo.end());
  
  type = UNGROUP;
  figures.clear();
  figures.insert(memo.begin(), memo.end());
  sigChanged();
}

/**
 * Kludge for TUndoGroup (to be renamed into ungroup)
 */
void
TFigureModel::_undoGroup(TFGroup *group, TFigureAtDepthList &store)
{
  type = _UNDO_GROUP;
  figure = group;
  figures.clear();
  
  TUndoUngroup *undo = new TUndoUngroup(this);
  
  for(TFigureAtDepthList::TStore::iterator p = store.store.begin();
      p!=store.store.end();
      ++p)
  {
    storage.insert(
      storage.begin() + p->depth,
      p->figure);
    figures.insert(p->figure);
    undo->insert(p->figure);
  }
  sigChanged();

  for(TStorage::iterator p=storage.begin();
      p!=storage.end();
      ++p)
  {
    if (*p == group) {
      storage.erase(p);
      break;
    }
  }
  TUndoManager::registerUndo(this, undo);
}


void
TFigureModel::erase(const iterator &p)
{
  type = MODIFIED;
  sigChanged();
  storage.erase(p.p);
}

void
TFigureModel::erase(const iterator &p, const iterator &e)
{
  type = MODIFIED;
  sigChanged();
  storage.erase(p.p, e.p);
}

void
TFigureModel::insert(const iterator &p, TFigure *g)
{
  type = MODIFIED;
  sigChanged();
  storage.insert(p.p, g);
}

void
TFigureModel::insert(const iterator &at, const iterator &from, const iterator &to)
{
  type = MODIFIED;
  sigChanged();
  storage.insert(at.p, from.p, to.p);
}

/**
 * remove all gadgets
 */
void
TFigureModel::clear()
{
  type = MODIFIED;
  sigChanged();
//  cerr << "clear TFigureModel " << this << endl;
  TStorage::iterator p, e;
  p = storage.begin();
  e = storage.end();
  while(p!=e) {
    delete *p;
    ++p;
  }
  storage.erase(storage.begin(), storage.end());
}

void
TFigureModel::store(TOutObjectStream &out) const
{
  TStorage::const_iterator p, e;
  p = storage.begin();
  e = storage.end();
  while(p!=e) {
    ::store(out, *p);
    ++p;
  }
}

bool
TFigureModel::restore(TInObjectStream &in)
{
  switch(in.what) {
    case ATV_START:
      clear();
      return true;
    case ATV_GROUP: {
        TSerializable *s = in.clone(in.type);
        if (!s)
          return false;
//        cerr << "okay got " << s->name() << endl;
        TFigure *g = dynamic_cast<TFigure*>(s);
        if (!g) {
//          cerr << "  wasn't a gadget" << endl;
          delete s;
          return false;
        }
//        cerr << "adding new gadget to TFigureModel " << this << endl;
        storage.push_back(g);
//        cerr << "new storage size is " << storage.size() << endl;
        in.setInterpreter(s);
        return true;
      }
      break;
    case ATV_FINISHED:
//      cerr << "restored TFigureModel of size " << storage.size() << endl;
      type = MODIFIED;
      sigChanged();
      return true;
  }
  return false;
}
