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

#include <set>

#include <toad/interactor.hh>
#include <toad/action.hh>
#include <toad/undo.hh>

namespace toad {

typedef set<TModel*> TModelSet;

class TModelUndoStore;

class TUndoManager:
  public TInteractor
{
    friend class TModelUndoStore;
  public:
    TUndoManager(TWindow *parent, const string &title);
    TUndoManager(TWindow *parent, const string &title,
                                  const string &undotext,
                                  const string &redotext);
    ~TUndoManager();
    
  protected:
    void init();

    static bool undoing;
    static bool redoing;
  
  public:
    static bool registerModel(TWindow*, TModel*);
    static void unregisterModel(TModel*);
    static void unregisterModel(TWindow *, TModel*);
    static bool registerUndo(TModel*, TUndo*);
    
    bool canUndo() const;
    bool canRedo() const;
    void doUndo();
    void doRedo();

    static bool isUndoing();
    static bool isRedoing();

  protected:    
    //! models managed by the undomanager
    TModelSet mmodels;


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

};

} // namespace toad

#endif
