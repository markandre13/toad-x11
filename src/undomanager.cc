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

#include <vector>
#include <toad/undomanager.hh>

#define DBM(CMD)

using namespace toad;

/**
 * \class toad::TUndoManager
 *
 * An object of this class manages an undo/redo history and adds two 
 * TActions to it's parent to control it.
 *
 * This class is an interactor so it gets deleted together with it's
 * parent.
 *
 * You must add the TUndoManager before any views which want to register
 * themselfes and their models with TUndoManger::registerModel, otherwise
 * registerModel will fail and return 'false'.
 *
 * \todo
 *   \li document how to use the undomanager (reference and manual)
 *   \li static method to disable undo/redo for a model when it's
 *       registered
 *   \li static methods which allow a window to call undo/redo
 *   \li static methods which allow a model to call undo/redo
 *   \li handle overflow of TUndo::serial counter
 *   \li limit number of stored undo event to a given maximum
 *   \li grouping of undo events
 */

namespace {

typedef set<TUndoManager*> TUndoManagerStore;
TUndoManagerStore undomanagers;

} // namespace

class toad::TModelUndoStore
{
  public:
    // undomanager for the model
    TUndoManagerStore undomanagers;

    // undo/redo objects for the model
    typedef vector<TUndo*> TUndoStack;
    TUndoStack undostack, redostack;  

    void addUndo(TModel *model, TUndo *undo);
    void clearRedo();
    void clear(TModel *model);
};

namespace {
  
typedef map<TModel*, TModelUndoStore> TModelStore;
TModelStore models;

} // namespace

/**
 * The titles of the actions are "edit|undo" and "edit|redo".
 */
TUndoManager::TUndoManager(TWindow *parent, const string &title):
  TInteractor(parent, title)
{
  undo = new TUndoAction(getParent(), "edit|undo", this, true);
  redo = new TUndoAction(getParent(), "edit|redo", this, false);
  init();
}

/**
 * TUndoManager with custom actions titles, ie. "go|back" and "go|forward".
 */
TUndoManager::TUndoManager(TWindow *parent,
                           const string &title,
                           const string &undotext,
                           const string &redotext):
  TInteractor(parent, title)
{
  undo = new TUndoAction(getParent(), undotext, this, true);
  redo = new TUndoAction(getParent(), redotext, this, false);
  init();
}

void
TUndoManager::init()
{
  undoing = redoing = false;
  undomanagers.insert(this);
  
  connect(undo->sigActivate, this, &TUndoManager::doUndo);
  undo->setEnabled(false);
  
  connect(redo->sigActivate, this, &TUndoManager::doRedo);
  redo->setEnabled(false);
}

TUndoManager::~TUndoManager()
{
  DBM(cerr << "destroy undomanager " << this << endl;)
  for(TModelSet::iterator p=mmodels.begin();
      p!=mmodels.end();
      ++p)
  {
    TModelStore::iterator q = models.find(*p);
    assert(q!=models.end());
    TUndoManagerStore::iterator r = q->second.undomanagers.find(this);
    assert(r!=q->second.undomanagers.end());
    q->second.undomanagers.erase(r);
  }
  undomanagers.erase(undomanagers.find(this));
}

bool TUndoManager::undoing = false;
bool TUndoManager::redoing = false;

static TUndoManagerStore::iterator
findUndoManager(TWindow *window)
{
  TUndoManagerStore::iterator p;
  for(p=undomanagers.begin();   
      p!=undomanagers.end();    
      ++p)
  {
    DBM(cerr << "  check window '" << (*p)->getParent()->getTitle() << "'" << endl;)
    if (window == (*p)->getParent() ||
        window->isChildOf((*p)->getParent()))
    {
      return p;
    }
  }
  return p;
}

/**
 * Register model for an undomanager.
 */
/*static*/ bool
TUndoManager::registerModel(TWindow *window, TModel *model)
{
  DBM(cerr << "register model " << model << " for window " << window << endl;)
  TUndoManagerStore::iterator p = findUndoManager(window);
  if (p==undomanagers.end())
  {
    DBM(cerr << "no undomanager found for model " << model << endl;)
    return false;
  }
  
  // add reference from TUndoManager to TModel
  DBM(cerr << "  add model " << model << " to undomanager " << *p << endl;)
  (*p)->mmodels.insert(model);
  
  // add reference from TModelUndoStore to TUndoManager
  TModelStore::iterator q = models.find(model);
  if (q==models.end()) {
    DBM(cerr << "  insert to new TModelUndoStore" << endl;)
    models[model].undomanagers.insert(*p);
  } else {
    DBM(cerr << "  insert to existing TModelUndoStore" << endl;)
    q->second.undomanagers.insert(*p);
  }
  return true;
}
 
/*static*/ void
TUndoManager::unregisterModel(TModel *model)
{
#warning "TUndoManager must be informed to update it's actions"
#warning "must remove model from model-undo-store also"
  DBM(cerr << "unregister model " << model << endl;)
  TModelStore::iterator p = models.find(model);
  if (p==models.end()) {
    return;
  }
  p->second.clear(model);
  models.erase(p);
}

//#undef DBM
//#define DBM(CMD) CMD

/*static*/ void 
TUndoManager::unregisterModel(TWindow *window, TModel *model)
{
#warning "TUndoManager must be informed to update it's actions"
  DBM(cerr << "unregister model " << model << " for window " << window << endl;)
  TUndoManagerStore::iterator p = findUndoManager(window);
  if (p==undomanagers.end())
  {
    DBM(cerr << "  no undomanager found for model " << model << endl;)
    return;
  }
  
  // remove reference from TUndoManager to TModel
  DBM(cerr << "  remove model " << model << " from undomanager " << *p << endl;)
  TModelSet::iterator r = (*p)->mmodels.find(model);
  if (r==(*p)->mmodels.end()) {
    DBM(cerr << "model not found in undomanager" << endl;)
    return;
  }
  (*p)->mmodels.erase(r);
  DBM(cerr << "    ok" << endl;)
  
  // add reference from TModelUndoStore to TUndoManager
  TModelStore::iterator q = models.find(model);
  if (q==models.end()) {
    DBM(cerr << "  model not found in model-undo-store" << endl;)
    return;
  }
  
  DBM(cerr << "  remove from existing TModelUndoStore" << endl;)
  TUndoManagerStore::iterator s = q->second.undomanagers.find(*p);
  if (s==q->second.undomanagers.end()) {
    DBM(cerr << "  undomanager not model-undo-store" << endl;)
    return;
  }
  
  q->second.undomanagers.erase(s);
  DBM(
    cerr << "    ok" << endl;
    cerr << "  done" << endl;)
}

//#undef DBM
//#define DBM(CMD)

/*static*/ bool
TUndoManager::registerUndo(TModel *model, TUndo *undo)
{
  DBM(cerr << "register undo " << undo << " for model " << model << endl;)
  TModelStore::iterator p = models.find(model);
  if (p==models.end()) {
    DBM(cerr << "no model registered" << endl;)
    delete undo;
    return false;
  }
  DBM(cerr << "found undomanager(s) for undo/redo" << endl;)
  p->second.addUndo(model, undo);
  return true;
}

void
TUndoManager::doUndo()
{
  DBM(cerr << "entering undomanager " << this << " undo" << endl;)
  if (undoing) {
    DBM(cerr << "  undo not possible, because i'm already undoing, please try later" << endl;)
    return;
  }
   
  undoing = true;

  // find next model to be undone
  
  DBM(cerr << "checking all " << mmodels.size() << " models\n";)
  
  TModelStore::iterator pms;
  TUndo *undo = 0;
  unsigned undocount = 0;
  for(TModelSet::iterator p=mmodels.begin();
      p!=mmodels.end();
      ++p)
  {
    TModelStore::iterator q = models.find(*p);
    assert(q!=models.end());
    if (!q->second.undostack.empty()) {
      undocount += q->second.undostack.size();
      TUndo *u = q->second.undostack.back();
      if (!undo) {
        undo = u; 
        pms = q;  
      } else {    
        if (undo->serial < u->serial) {
          undo = u;
          pms = q; 
        }
      }  
    }    
  }      
         
  if (!undo) {
    DBM(cerr << "  undo not possible, because there are no events to be undone" << endl;)
  } else {
    pms->second.undostack.pop_back();
    undo->undo();
    delete undo;
  }
  if (undocount<=1) {
    this->undo->setEnabled(false);
  }
   
  undoing = false;
}

void
TUndoManager::doRedo()
{
  DBM(cerr << "\nredo" << endl;)
  if (redoing) {
    DBM(cerr << "  redo not possible" << endl;)
    return;
  }
   
  redoing = true;

  // find next model to be redone
  
  DBM(cerr << "checking all " << mmodels.size() << " models\n";)
  
  TModelStore::iterator pms;
  TUndo *undo = 0;
  unsigned redocount = 0;
  for(TModelSet::iterator p=mmodels.begin();
      p!=mmodels.end();
      ++p)
  {
    TModelStore::iterator q = models.find(*p);
    assert(q!=models.end());
    DBM(cerr << "  check model " << *p << endl;)
    if (!q->second.redostack.empty()) {
      redocount += q->second.redostack.size();
      DBM(cerr << "    it's not empty" << endl;)
      TUndo *u = q->second.redostack.back();
      if (!undo) {
        undo = u; 
        pms = q;  
      } else {    
        if (undo->serial < u->serial) {
          undo = u;
          pms = q; 
        }
      }  
    } else {
      DBM(cerr << "    it's empty" << endl;)
    }
  }  
     
  if (!undo) {
    DBM(cerr << "  redo not possible, because there are no events to be redone" << endl;)
  } else {
    pms->second.redostack.pop_back();
    undo->undo();
    delete undo;
  }
  if (redocount<=1) {
    this->redo->setEnabled(false);
  }
   
  redoing = false;
}

bool
TUndoManager::isUndoing()
{
  return undoing;
}
 
bool
TUndoManager::isRedoing()
{
  return redoing;
}

bool
TUndoManager::TUndoAction::getState(string *text, bool *active) const
{
  DBM(
    cerr << __PRETTY_FUNCTION__ << endl;
    cerr << "  action is:" << getTitle() << endl;
  )
  TModelStore::iterator pms;
  TUndo *undo = 0;
  unsigned count = 0;

  if (this->undo) {

    for(TModelSet::iterator p=manager->mmodels.begin();
        p!=manager->mmodels.end();
        ++p)
    {
      TModelStore::iterator q = models.find(*p);
      assert(q!=models.end());
      if (!q->second.undostack.empty()) {
        count += q->second.undostack.size();
        TUndo *u = q->second.undostack.back();
        if (!undo) {
          undo = u; 
          pms = q;  
        } else {    
          if (undo->serial < u->serial) {
            undo = u;
            pms = q; 
          }
        }  
      }    
    }
    if (undo) {
      return undo->getUndoName(text);
    }

  } else {

    for(TModelSet::iterator p=manager->mmodels.begin();
        p!=manager->mmodels.end();
        ++p)
    {
      TModelStore::iterator q = models.find(*p);
      assert(q!=models.end());
      DBM(cerr << "  check model " << *p << endl;)
      if (!q->second.redostack.empty()) {
        count += q->second.redostack.size();
        DBM(cerr << "    it's not empty" << endl;)
        TUndo *u = q->second.redostack.back();
        if (!undo) {
          undo = u; 
          pms = q;  
        } else {    
          if (undo->serial < u->serial) {
            undo = u;
            pms = q; 
          }
        }  
      } else {
        DBM(cerr << "    it's empty" << endl;)
      }
    }
    if (undo) {
      return undo->getRedoName(text);
    }

  }
  return false;
}

bool
TUndoManager::canUndo() const
{
  if (undoing)
    return false;
    
  // can be optimized with a counter in TUndoManager which tracks the
  // number of possible undo's in case this method is called too much
  for(TModelSet::const_iterator p=mmodels.begin();
      p!=mmodels.end();
      ++p)
  {
    TModelStore::const_iterator q = models.find(*p);
    assert(q!=models.end());
    if (!q->second.undostack.empty())
      return true;
  }
  return false;
}
 
bool
TUndoManager::canRedo() const
{
  if (redoing)
    return false;
    
  // can be optimized with a counter in TUndoManager which tracks the
  // number of possible undo's in case this method is called too much
  for(TModelSet::const_iterator p=mmodels.begin();
      p!=mmodels.end();
      ++p)
  {
    TModelStore::const_iterator q = models.find(*p);
    assert(q!=models.end());
    if (!q->second.redostack.empty())
      return true;
  }
  return false;
}

void
TModelUndoStore::addUndo(TModel *model, TUndo *undo)
{
  DBM(cerr << "add undo " << undo << " to model " << model << endl;)
  if (!TUndoManager::isUndoing() && !TUndoManager::isRedoing()) {   
    DBM(cerr << "  not undoing, clear redostacks" << endl;)
    // step 1: 'undomanagers' contains a list of all undo managers which
    //         manage our current model, since we don't know for which  
    //         one the new undo event was generated, we must traverse all
    //         of them.
    //         we now create a set of all models they manage
    DBM(cerr << "    create list of all models managed by undo managers which also manage model " << model << endl;)
    TModelSet xmodels;
    for(TUndoManagerStore::iterator p=undomanagers.begin();
        p!=undomanagers.end();
        ++p)
    {
      DBM(cerr << "      add all models " << (*p)->mmodels.size() << " owned by undo manager " << *p << endl;)
      xmodels.insert((*p)->mmodels.begin(), (*p)->mmodels.end());
    }
    DBM(
      cerr << "    models to be cleared:";
      for(TModelSet::iterator p=xmodels.begin();
          p!=xmodels.end();
          ++p)
      {
        cerr << " " << *p;
      }
      cerr << endl;
    )
     
    // step 2: all the models found in the previous step be cleared
    //         of their redo list
    for(TModelSet::iterator p=xmodels.begin();
        p!=xmodels.end();
        ++p)
    {
      TModelStore::iterator q = models.find(*p);
      assert(q!=models.end());
      DBM(cerr << "  clear redo list of model " << *p << endl;)
      q->second.clearRedo();
    }
  }  
  assert(undomanagers.begin()!=undomanagers.end());
  if (!TUndoManager::isUndoing()) {
    DBM(cerr << "add undo to models undostack" << endl;)
    undostack.push_back(undo);
    for(TUndoManagerStore::iterator p=undomanagers.begin();
        p!=undomanagers.end();
        ++p)
    {
      DBM(cerr << "enabled undo for undomanager " << *p << endl;)
      (*p)->undo->setEnabled(true);
      bool memo = TUndoManager::redoing; // cheat canRedo method...
      TUndoManager::redoing = false;
      if (!(*p)->canRedo()) {
        (*p)->redo->setEnabled(false);
      }
      TUndoManager::redoing = memo;
    }
  } else {
    DBM(cerr << "add undo to models redostack" << endl;)
    redostack.push_back(undo);
    for(TUndoManagerStore::iterator p=undomanagers.begin();
        p!=undomanagers.end();
        ++p)
    {
      DBM(cerr << "enabled redo for undomanager " << *p << endl;)
      (*p)->redo->setEnabled(true);
    }
  }
}  

void
TModelUndoStore::clearRedo() {
  while(!redostack.empty()) { 
    delete redostack.back();
    redostack.pop_back();     
  }
}  

void
TModelUndoStore::clear(TModel *model)
{
  for(TUndoStack::iterator p=undostack.begin();
      p!=undostack.end();
      ++p)
  {
    delete *p;
  }
  for(TUndoStack::iterator p=redostack.begin();
      p!=redostack.end();
      ++p)
  {
    delete *p;
  }
  for(TUndoManagerStore::iterator p=undomanagers.begin();
      p!=undomanagers.end();
      ++p)
  {
    DBM(
      cerr << "  remove model " << model << " from undomanager " << *p << endl;
      cerr << "    undomanager manages:";
      for(TModelSet::iterator i=(*p)->mmodels.begin();
          i!=(*p)->mmodels.end();
          ++i) cerr << " " << *i << endl;
      cerr << endl;
    )
    TModelSet::iterator q = (*p)->mmodels.find(model);
    assert(q!=(*p)->mmodels.end());
    (*p)->mmodels.erase(q);
  }
}
