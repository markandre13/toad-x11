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

#include <toad/undomanager.hh>

using namespace toad;

/**
 * \class toad::TUndoManager
 *
 * An object of this class manages an undo/redo history and adds two 
 * TActions to it's parent to control it.
 *
 * This class is an interactor so it gets deleted together with it's
 * parent.
 */

namespace {

typedef vector<TUndoManager*> TUndoManagerStorage;
TUndoManagerStorage umstorage;

} // namespace

/**
 * The titles of the actions are "edit|undo" and "edit|redo".
 */
TUndoManager::TUndoManager(TWindow *parent):
  TInteractor(parent)
{
  undo = new TUndoAction(getParent(), "edit|undo", this, true);
  redo = new TUndoAction(getParent(), "edit|redo", this, false);
  init();
}

/**
 * TUndoManager with custom actions titles, ie. "go|back" and "go|forward".
 */
TUndoManager::TUndoManager(TWindow *parent, 
                           const string &undotext,
                           const string &redotext):
  TInteractor(parent)
{
  undo = new TUndoAction(getParent(), undotext, this, true);
  redo = new TUndoAction(getParent(), redotext, this, false);
  init();
}

void
TUndoManager::init()
{
  connect(undo->sigActivate, this, &TUndoManager::doUndo);
  undo->setEnabled(false);

  connect(redo->sigActivate, this, &TUndoManager::doRedo);
  redo->setEnabled(false);
  
  umstorage.push_back(this);
}

/**
 * Locates an TUndoManger for 'parent' and adds 'undoable' to it or
 * deletes the object.
 */
void
TUndoManager::manage(TInteractor *parent, TUndoable *undoable)
{
//cerr << "TUndoManager::manage undoable for '" << parent->getTitle() << "'" << endl;
  TUndoManagerStorage::iterator p, e;
  p = umstorage.begin();
  e = umstorage.end();
  while(p!=e) {
//cerr << "  is it a child of '" << (*p)->getParent()->getTitle() << "'\n";
    if (parent == (*p)->getParent() ||
        parent->isChildOf((*p)->getParent()) )
    {
//      cerr << "  undoable is a child of '" << (*p)->getParent()->getTitle() << "'\n";
      (*p)->add(undoable);
      return;
    }
    ++p;
  }
//cerr << "  found no one to manage the event, deleting it\n";
  delete undoable;
}

void
TUndoManager::doUndo()
{
//  cerr << "doUndo" << endl;
  if (history.getBackSize()>0) {
    history.getCurrent()->undo();
    history.goBack();
  }
  if (history.getBackSize()==0)
    undo->setEnabled(false);
  redo->setEnabled(true);
}

void
TUndoManager::doRedo()
{
//  cerr << "doRedo" << endl;
  if (history.getForwardSize()>0) {
    history.goForward();
    history.getCurrent()->redo();
  }
  if (history.getForwardSize()==0)
    redo->setEnabled(false);
  undo->setEnabled(true);
}


bool
TUndoManager::TUndoAction::getState(string *text, bool *active) const
{
//  cerr << __PRETTY_FUNCTION__ << endl;
  if (undo) {
    if (manager->history.getBackSize()>0)
      return manager->history.getCurrent()->getUndoName(text);
  } else {
    if (manager->history.getForwardSize()>0) {
      manager->history.goForward();
      bool b = manager->history.getCurrent()->getRedoName(text);
      manager->history.goBack();
      return b;
    }
  }
  return false;
}
