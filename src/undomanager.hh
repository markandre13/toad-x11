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

#ifndef _TOAD_UNDOMANAGER_HH
#define _TOAD_UNDOMANAGER_HH

#include <toad/interactor.hh>
#include <toad/action.hh>
#include <toad/undoable.hh>
#include <toad/util/history.hh>

namespace toad {

class TUndoManager:
  public TInteractor
{
    friend class TUndoAction;

    class TUndoAction:
      public TAction
    {
        bool undo;
        TUndoManager *manager;
      public:
        TUndoAction(TInteractor *parent,
                    const string &title, 
                    TUndoManager *manager,
                    bool undo):
          TAction(parent, title)
        {
          this->manager = manager;
          this->undo = undo;
        }
        bool getState(string*, bool*) const;
    };
    TUndoAction *undo, *redo;
    GHistory<PUndoable> history;

  public:
    TUndoManager(TWindow*);
    TUndoManager(TWindow*, const string &undotext, const string &redotext);
    
    static void manage(TInteractor *parent, TUndoable *undoable);
    void add(TUndoable *undoable) {
      undo->setEnabled(true);
      history.add(undoable);
    }
  
  protected:
    void init();
    void doUndo();
    void doRedo();
};

} // namespace toad

#endif
