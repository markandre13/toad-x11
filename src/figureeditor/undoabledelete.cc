/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#include <toad/figureeditor/undoabledelete.hh>

#define VERBOSE 0

using namespace toad;

TUndoableDelete::TUndoableDelete(TFigureModel& g,
                                 const TFigureEditor::TFigureSet& selection):
  gadgets(g)
{
  done = true;
  // store gadgets & their depth in a way which makes it possible to go
  // two directions
  
  unsigned pos=0;
  TFigureModel::iterator p,e;
  p = gadgets.begin();
  e = gadgets.end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end() &&
        (*p)->removeable )
    {
#if VERBOSE
      cout << "memorizing gagdet " << *p << " at pos " << pos << endl;
#endif
      memo.push_back(TMemo(*p,pos));
    }
    ++p;
    ++pos;
  }
}

TUndoableDelete::~TUndoableDelete()
{
  if (done) {
    vector<TMemo>::iterator p,e;
    p = memo.begin();
    e = memo.end();
    while(p!=e) {
      delete (*p).gadget;
#if VERBOSE
      cout << "finaly removing gadget " << (*p).gadget << endl;
#endif
      p++;
    }
  }
}

void TUndoableDelete::undo()
{
  done = false;
  unsigned pos=0;
  TFigureModel::iterator gp;
  gp = gadgets.begin();
  vector<TMemo>::iterator mp,me;
  mp = memo.begin();
  me = memo.end();
  while(mp!=me) {
    if (pos==(*mp).pos) {
      gadgets.insert(gp, (*mp).gadget);
#if VERBOSE
      cout << "inserting gadget " << (*mp).gadget << " at " << pos << endl;
#endif
      mp++;
    } else {
      gp++;
    }
    pos++;
  }
}
