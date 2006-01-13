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

/**
 * \class toad::TModel
 *
 * \note
 *   \li
 *     The event on destruction is an ugly hack.
 *   \li
 *     The meta signal was added because after adding the destruction signal
 *     I discovered that I also need to enabled/disabled model and communicate
 *     this change also...
 *   \li
 *     communication the reason for sigChanged by attributes within the
 *     model as currently done in TOAD and application has two drawbacks:
 *     (1) it fails in case the sigChanged is locked and is triggered more
 *         than once
 *     (2) these attributes do add additional bloat to the model
 *   \li
 *     MacApp uses an optional Mark'n Sweep algorithm 
 *     (http://developer.apple.com/techpubs/mac/MacAppProgGuide/MacAppProgGuide-26.html)
 *     to distribute modification information.
 *   \li
 *     Cocoa: AppKit doesn't contain the word Model in our context. 
 *     NSResponder? event message, action message, responder chain?
 *   \li
 *     Swing: ?
 *   \li
 *     we want easily maintainable connections and disconnections between 
 *     'model and view' and 'model and model'.
 *   \li
 *     we need a way to connect entities of different type (ie. numbers
 *     to be edited via TTextArea via TTextModel)
 *   \li
 *     models have to keep track of the undo and redo list
 *   \li
 *     ...
 */

#include <toad/model.hh>
#include <toad/undomanager.hh>

using namespace toad;

TModel::~TModel()
{
  TUndoManager::unregisterModel(this);
  meta = META_DESTRUCTION;
  sigMeta();
  meta = META_CUSTOM;
}
