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

#include <vector>
#include <toad/undomanager.hh>

#define DBM(CMD)

using namespace toad;

/**
 * \class toad::TUndoManager
 *
 * An object of this class manages an undo/redo history for its parent
 * window and also adds two TActions to the parent which can be found
 * by eg. a TMenuBar to allow the user to invoke the undo and redo commands.
 *
 * Views must register their models for undo/redo with the static function
 * TUndoManager::registerModel(TWindow*, TModel*) and models must register
 * their undo events with the static function registerUndo(TModel*, TUndo*).
 *
 * NOTE: You must add the TUndoManager before any views which want to
 * register themselfes and their models with TUndoManger::registerModel,
 * otherwise registerModel will fail and return 'false'.
 *
 * (TUndoManager is implemented as an interactor so it gets deleted along
 * with it's parent.)
 *
 * \todo
 *   \li document how to use the undomanager (reference and manual)
 *   \li nesting of groups when using spaning undo groups
 *   \li static method to disable undo/redo for a model when it's
 *       registered
 *   \li static methods which allow a window to call undo/redo
 *   \li static methods which allow a model to call undo/redo
 *   \li handle overflow of TUndo::serial counter
 *   \li limit number of stored undo event to a given maximum
 *       (to avoid huge memory consumption)
 */

// How this stuff is organized:
//
// undomanagers: contains a set of all known undomanagers
//   each undomanager contains a list of all models managed by him
//
// models: holds a map with additional information for each model
//         registered for undo/redo:
//         - undo's
//         - redo's
//         - set of undomanagers who contain views manipulating the
//           model
//
// undogroups
// redogroups
//


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

/**
 * Iterate over all 'undomanagers' and find one which is a child of the
 * specified TWindow.
 */
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

/**
 * Unregister a model from the undo/redo management.
 *
 * Usually a view registers its models for undo/redo management which
 * isn't always what you want. Eg. the row number field in a texteditor
 * doesn't need undo/redo handling as the text is the data you're
 * manipulating.
 *
 * Also note that some views create additional models so you should
 * use the views getModel() method for unregistering a model:
 *
 * \code
 * TBoundedRangeModel number;
 * TTextField *txt = new TTextField(0, "text", &number);
 * TUndoManager::unregisterModel(&number);         // failure
 * TUndoManager::unregisterModel(txt->getModel()); // okay
 * \endcode
 *
 * \param model
 *   The model to be removed from the undo/redo management.
 * \return
 *   Returns 'false' in case the model wasn't registered.
 */ 
bool
TUndoManager::unregisterModel(TModel *model)
{
#warning "TUndoManager must be informed to update it's actions"
#warning "must remove model from model-undo-store also"
  DBM(cerr << "unregister model " << model << endl;)
  TModelStore::iterator pms = models.find(model);
  if (pms==models.end()) {
    return false;
  }
  pms->second.clear(model);
  models.erase(pms);
  return true;
}

// Model added too late to undo manager
// When an already modified model (models undostack isn't empty) is added
// to an undo manager, there will be a discrepance between the models and
// the undo managers undo stack. Normally for every undo in the models undo
// stack there's a model entry in the undo managers undo stack.
// There are the following approaches:
// o ignore it, in case there are no side effects with the other algorithms
//   when a user creates a second view of, say a text editor, it wouldn't
//   be what he expects, that undo isn't available in the new window but
//   not in the old.
// o insert entries for the model into the undomanager and use all other 
//   undomanagers which are using this model as a reference.
//   but this may not work in case the undomanager other models aren't used
//   in other undomanagers together with this model, in which case we could
//   insert the model anywhere, as long as we keep the order.
//   we would need to add a global undo stack just for this case to get the
//   correct order. or just redesign the current database?
//   (a better idea came when i woke up this morning: add serial numbers to
//   the undo objects! this would allow us to use an simple insert without
//   looking into the other undomanagers. we could also drop the undomanagers
//   undo- and redostacks!)


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

bool TUndoManager::undoregistration = true;

/**
 * Enable 'registerUndo', which is the default.
 */
/*static*/ void
TUndoManager::enableUndoRegistration()
{
  undoregistration = true;
}

/**
 * Disable 'registerUndo'.
 */
/*static*/ void
TUndoManager::disableUndoRegistration()
{
  undoregistration = false;
}

/*static*/ bool
TUndoManager::isUndoRegistrationEnabled()
{
  return undoregistration;
}

namespace {

struct TUndoGroup;
typedef vector<TUndoGroup*> TUndoGroupVector;

struct TUndoGroup
{
  // serial number of first undo event
  unsigned start;
  // serial number + 1 of last undo event (start==end -> no events)
  unsigned end;
  // nested undo groups
  TUndoGroupVector nested;
  TModelSet models;
  
  /**
   * \param back
   *    true=undo, false=redo
   */
  void undo(bool back);
};

TUndoGroupVector groupstack;
TUndoGroupVector undogroups;
TUndoGroupVector redogroups;

TModel *span_undogroup = 0;

} // namespace


/**
 * Start to group upcoming undo events.
 *
 * The optional 'model' parameter can be used to span the group
 * over a longer period of user interaction, eg. TTextModel opens
 * a undo group but only closes and opens a new group when starting
 * a new line, removing after insert and vise versa, etc..
 * When another model wants to register undo events or open a new
 * group, the model parameter will be different and the previous group
 * is closed before opening the new one.
 *
 * Please see the code for TTextModel for an example.
 *
 * \sa endUndoGrouping
 */
void
TUndoManager::beginUndoGrouping(TModel *model)
{
  if (span_undogroup) {
//cout << "spaning undogroup detected" << endl;
    if (model!=span_undogroup) {
//cout << "  for different model, closing old group" << endl;
      endUndoGrouping();
    } else {
//cout << "  already open, don't open new group" << endl;
      return;
    }
  }
//if (model) cout << "spaning new undo group" << endl;
  span_undogroup = model;

  DBM(cerr << __PRETTY_FUNCTION__ << endl;)
  TUndoGroup *g = new TUndoGroup();
  if (groupstack.empty()) {
    if (!undoing) {
      DBM(cerr << "  to undo groups" << endl;)
      undogroups.push_back(g);
    } else {
      DBM(cerr << "  to redo groups" << endl;)
      redogroups.push_back(g);
    }
  } else {
    groupstack.back()->nested.push_back(g);
  }
  groupstack.push_back(g);
  g->start = TUndo::counter;
}

/**
 * Close an undo group which was opened with beginUndoGrouping.
 *
 * \sa beginUndoGrouping
 */
void 
TUndoManager::endUndoGrouping(TModel *model)
{
  // don't do anything, in case the group was already closed
  if (model && span_undogroup != model) {
//    cout << "ignoring request to close already closed undo group" << endl;
    return;
  }

  DBM(cerr << __PRETTY_FUNCTION__ << endl;)
  
  span_undogroup = 0;
  
  if (groupstack.empty()) {
    cerr << "TUndoManager::endUndoGrouping called but no group to be closed" << endl;
    // printStackTrace();
    return;
  }
  
  groupstack.back()->end = TUndo::counter;
  groupstack.pop_back();
}

// setGroupsByEvent...

/*static*/ unsigned
TUndoManager::groupingLevel()
{
  return groupstack.size();
}

//#undef DBM
//#define DBM(CMD)

/*static*/ bool
TUndoManager::registerUndo(TModel *model, TUndo *undo)
{
  if (span_undogroup && span_undogroup != model) {
//    cout << "register undo event outside the currently spaned undo group" << endl;
    endUndoGrouping();
  }

  if (!undoregistration) {
    delete undo;
    return false;
  }

  DBM(cerr << "register undo " << undo << " for model " << model << endl;)
  TModelStore::iterator p = models.find(model);
  if (p==models.end()) {
    DBM(cerr << "no model registered" << endl;)
    delete undo;
    return false;
  }
  DBM(cerr << "found undomanager(s) for undo/redo" << endl;)

  if (!groupstack.empty())
    groupstack[0]->models.insert(model);

  p->second.addUndo(model, undo);

  return true;
}

void
TUndoManager::doIt(bool back)
{
  DBM(cerr << "entering undomanager " << this << " doUndo" << endl;)

  if (!groupstack.empty()) {
    cerr << "TUndoManager::doUndo(): groupstack isn't empty, closing it" << endl;
    while(!groupstack.empty())
      endUndoGrouping();
  }

  if (back) {
    if (undoing) {
      DBM(cerr << "  undo not possible, because i'm already undoing, please try later" << endl;)
      return;
    }
    undoing = true;
  } else {
    if (redoing) {
      DBM(cerr << "  redo not possible, because i'm already redoing, please try later" << endl;)
      return;
    }
    redoing = true;
  }
   

  // find next model to be undone
  
  DBM(cerr << "checking all " << mmodels.size() << " models\n";)
  
  TModelStore::iterator pms;
  TUndo *undo = 0;
  unsigned undocount = 0;
  
  // iterate over all models managed by this undomanager and set
  // 'undo' to the latest undo object found
  for(TModelSet::iterator p=mmodels.begin();
      p!=mmodels.end();
      ++p)
  {
    // get the additional undo/redo information for the model
    TModelStore::iterator q = models.find(*p);
    assert(q!=models.end());
    
    // set 'undo' to the latest undo event
    TUndo *u;
    if (back) {
      if (q->second.undostack.empty())
        continue;
      undocount += q->second.undostack.size();
      u = q->second.undostack.back();
    } else {
      if (q->second.redostack.empty())
        continue;
      undocount += q->second.redostack.size();
      u = q->second.redostack.back();
    }

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
         
  if (undo) {
    bool foundgroup = false;
    TUndoGroupVector::iterator p, b;
    if (back) {
      b = undogroups.begin();
      p = undogroups.end();
    } else {
      b = redogroups.begin();
      p = redogroups.end();
    }
    while(p!=b) {
      --p;
      if ((*p)->start<=undo->serial && undo->serial<(*p)->end) {
        
        (*p)->undo(back);
        foundgroup = true;
        if (back)
          undogroups.erase(p);
        else
          redogroups.erase(p);
        
        break;
      } else
      if ((*p)->start<undo->serial) {
        break;
      }
    }
    if (!foundgroup) {
      // remove undo object from model
      if (back) {
        pms->second.undostack.pop_back();
      } else {
        pms->second.redostack.pop_back();
      }
      // execute undo
      undo->undo();
      delete undo;
    } else {

      // after handling an undo/redo group we need to recalculate the
      // 'undocount' variable
      undocount = 0;
      for(TModelSet::iterator p=mmodels.begin();
          p!=mmodels.end();
          ++p)
      {
        TModelStore::iterator q = models.find(*p);
        assert(q!=models.end());
        
        if (back) {
          if (!q->second.undostack.empty()) {
            undocount = 2;
            break;
          }
        } else {
          if (!q->second.redostack.empty()) {
            undocount = 2;
            break;
          }
        }
      }
    }
  } else {
    DBM(cerr << "  undo not possible, because there are no events to be undone" << endl;)
  }

  if (undocount<=1) {
    if (back)
      this->undo->setEnabled(false);
    else
      this->redo->setEnabled(false);
  }

  if (back)
    undoing = false;
  else
    redoing = false;
}

void
TUndoGroup::undo(bool back)
{
  DBM(cerr << "undo/redo TUndoGroup" << endl;)
  if (start==end) {
    cerr << "undo group is empty\n";
    return;
  }
  
  TUndoManager::beginUndoGrouping();

  while(true) {
    TUndo *undo = 0;
    TModelStore::iterator pms;
    for(TModelSet::iterator p = models.begin();
        p != models.end();
        ++p)
    {
      TModelStore::iterator q = ::models.find(*p);
      assert(q!=::models.end());
      TUndo *u;
      if (back) {
        if (q->second.undostack.empty())
          continue;
        u = q->second.undostack.back();
      } else {
        if (q->second.redostack.empty())
          continue;
        u = q->second.redostack.back();
      }
      
      if (start <= u->serial && u->serial < end) {
        if (!undo) {
          undo = u;
          pms = q;
        } else {
          if (undo->serial < u->serial) {
            undo = 0;
            pms = q;
          }
        }
      }
    }
    if (!undo) {
      DBM(cerr << "  found no more undo objects inside this group" << endl;)
      break;
    }
    DBM(cerr << "  undo object inside group" << endl;)
    if (back)
      pms->second.undostack.pop_back();
    else
      pms->second.redostack.pop_back();
    undo->undo();
    delete undo;
  }
  
  TUndoManager::endUndoGrouping();
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
    DBM(cerr << "  get state for undo" << endl;)
    for(TModelSet::iterator p=manager->mmodels.begin();
        p!=manager->mmodels.end();
        ++p)
    {
      TModelStore::iterator q = models.find(*p);
      assert(q!=models.end());
      DBM(cerr << "  check model " << *p << endl;)
      if (!q->second.undostack.empty()) {
        count += q->second.undostack.size();
        DBM(cerr << "    it's not empty" << endl;)
        TUndo *u = q->second.undostack.back();
        DBM(cerr << "      found undo " << u << " for model " << *p << endl;)
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
    DBM(cerr << "  get state for redo" << endl;)
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
        DBM(cerr << "      found redo " << u << " for model " << *p << endl;)
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
  DBM(cerr << "    it's empty" << endl;)
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

// Redo Conflict:
// When adding a new undo event, the redo stack of the model must
// be cleared. The question then is what to with the redo stacks
// in the undomanagers.
//
// a clear the models redo stack and all remove references to
//   the model in the redo stacks of all undo managers
// b clear the redo stacks of all models under control of the
//   undo manager where the undo was issued and remove all
//   references to these models in all undo managers
//   (only it can't be done, as we don't know under which undomanager
//   the undo event originated, because the model generates it)
// c same as b, but done for all undo managers known to manage
//   the model where the the undo event was issued
// d clear all redo stacks which are somehow related to each
//   other
// e clear ALL redo stacks
//
// b would have been the best approach, but since it can't be implemented
// with the current API, it's currently c.
//
// how to use b instead of c:
// o don't let the model create the undo, but the window
//   but we don't want to do that, as creating the undo in the model
//   allows us to let the model create redo's events
// o use a side channel... ie. the window which was receiving a event
//   (mouse, keyboard, ...) is the one which was modifying the model.
//   might be not a bad as it sounds, as Apple's Cocoa uses the event loop
//   to automatically group undo events (unless disabled, and grouping
//   can also be done explicitly)
//   but: how about model changes not initiated by a view at all, ie.  
//        when a ioobserver adds text to a model (as done in NetEdit 2,
//        when SNMP replies come in)
// o before a view modifies a model it can set a global variable to itself
//
// okay, solutions unsatisfactionarry, we keep it like that: variant c.

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
    
    // step 3: now we do the same with the redo groups
    for(TUndoGroupVector::iterator p=redogroups.begin();
        p!=redogroups.end();
        ++p)
    {
      for(TModelSet::iterator q=xmodels.begin();
          q!=xmodels.end();
          ++q)
      {
        (*p)->models.erase(*q);
      }
      if ((*p)->models.empty()) {
        DBM(cerr << "deleting empty redogroup" << endl;)
        int n = p - redogroups.begin();
        redogroups.erase(p);
        p = redogroups.begin() + n - 1;
      }
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

/**
 * Clears the undo/redo stacks for this model and removes this model
 * from all other TUndoManager::
 *
 * \param model
 *   The TModelUndoStore's model, which isn't part of the TModelUndoStore
 *   class and must therefore be given with this parameter.
 */
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
