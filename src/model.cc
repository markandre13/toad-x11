/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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
 *     MacApp uses an optional Mark'n Sweep algorithm 
 *     (http://developer.apple.com/techpubs/mac/MacAppProgGuide/MacAppProgGuide-26.html)
 *     to distribute modification information.
 *   \li
 *     Cocoa: AppKit doesn't contain the word Model in our context. 
 *     NSResponder? event message, action message, responder chain?
 *   \li
 *     Swing: ?
 *   \li
 *     we want easy maintainable connections and disconnections between 
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
}
