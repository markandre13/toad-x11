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

#include <toad/textmodel.hh>
#include <toad/undomanager.hh>

using namespace toad;

TTextModel::TTextModel()
{
  nlines = 0;
  _modified = false;
}

void
TTextModel::setValue(const string &d)
{
//cerr << "TTextModel[" << this << "]::setValue(string)\n";
  if (data==d) {
//    cerr << "-> not changed\n";
    return;
  }
//  DBM(cout << __PRETTY_FUNCTION__ << endl;)

  offset = 0;
  data = d;
  length = data.size();
  lines = (size_t)-1;   // all lines have changed

  nlines = 0;
  size_t pos = 0;
  while(true) {
    pos = d.find('\n', pos);
    if (pos==string::npos)
      break;
    pos++;
    nlines++;
  }
  
  _modified = false;
  type = CHANGE;
  sigTextArea();
  sigChanged();
}

void
TTextModel::setValue(const char *d, size_t len)
{
//cerr << "TTextModel[" << this << "]::setValue(char*)\n";
#ifndef OLDLIBSTD
  if (data.compare(0, string::npos, d, len)==0) {
//    cerr << "-> not changed\n";
    return;
  }
#else
  if (data.compare(d, len)==0) {
//    cerr << "-> not changed\n";
    return;
  }
#endif

  offset = 0;
  length = len;
  data.assign(d, len);
  lines = (size_t)-1;   // all lines have changed

  nlines = 0;
  size_t pos = 0;
  while(true) {
    pos = data.find('\n', pos);
    if (pos==string::npos)
      break;
    pos++;
    nlines++;
  }
  
  _modified = false;
  type = CHANGE;
  sigTextArea();
  sigChanged();
}

/**
 * insert a single char
 */
void
TTextModel::insert(size_t p, int c)
{
  c = filter(c);
  if (!c)
    return;

  // group undo events until...
  if (type==CHANGE ||       // ...the whole model was changed,
      type==REMOVE ||       // we were removing before,
      p!=offset+length ||   // insert at a new cursor position or
      c=='\n')              // we start a new line
  {
    //cout << "* new undo group for textarea" << endl;
    TUndoManager::endUndoGrouping(this);
  }
  TUndoManager::beginUndoGrouping(this);
  TUndoManager::registerUndo(this, new TUndoInsert(this, p, 1));
  data.insert(p, 1, c);
  
  type = INSERT;
  offset = p;
  length = 1;
  lines  = c=='\n' ? 1 : 0;
  _modified = true;
  
  nlines += lines;
  
  sigTextArea();
  sigChanged();
}

/**
 * insert multiple chars
 */
void
TTextModel::insert(size_t p, const string &aString)
{
  if (aString.size()==0)
    return;
  string s(aString);

  string::iterator sp;
  sp = s.begin();
  while(sp!=s.end()) {
    if (filter(*sp)==0)
      sp = s.erase(sp);
    else  
      ++sp;
  }

  if (s.empty())
    return;

  if (type==CHANGE || type==REMOVE || p!=offset+length || s=="\n" || s.size()>1) {
    //cout << "* new undo group for textarea" << endl;
    TUndoManager::endUndoGrouping();
  }
  TUndoManager::beginUndoGrouping(this);
  TUndoManager::registerUndo(this, new TUndoInsert(this, p, s.size()));

  data.insert(p, s);
  
  type = INSERT;
  offset = p;
  length = s.size();
  lines = 0;
  for(size_t i=0; i<length; i++) {
    if (s[i]=='\n')
      lines++;
  }

  nlines += lines;
  _modified = true;
  
  sigTextArea();
  sigChanged();
}

/**
 * remove multiple chars
 *
 * \param p    offset
 * \param l    length
 * \param undo store in undo history if true
 */
void
TTextModel::remove(size_t p, size_t l)
{
  if (l==0)
    return;
    
  // cout << "remove at " << p << endl;
  if (type==CHANGE || type==INSERT || p!=offset-length) {
    //cout << "* new undo group for textarea" << endl;
    TUndoManager::endUndoGrouping();
  }
  TUndoManager::beginUndoGrouping(this);
  TUndoManager::registerUndo(this, new TUndoRemove(this, p, data.substr(p,l)));

  lines = 0;
  for(size_t i=p; i<p+l; ++i) {
    if (data[i]=='\n')
      ++lines;
  }
  nlines -= lines;
  type   = REMOVE;
  offset = p;
  length = l;
  _modified = true;
  sigTextArea();
  data.erase(p, l);
  sigChanged();
}

int
TTextModel::filter(int c)
{
  return c;
}

void
TTextModel::focus(bool)
{
}

bool
TTextModel::TUndoInsert::getUndoName(string *name) const
{
  *name = "Undo: Insert";
  return true;
}

bool
TTextModel::TUndoInsert::getRedoName(string *name) const
{
  *name = "Redo: Remove";
  return true;
}

bool
TTextModel::TUndoRemove::getUndoName(string *name) const
{
  *name = "Undo: Remove";
}

bool
TTextModel::TUndoRemove::getRedoName(string *name) const
{
  *name = "Redo: Insert";
}

void
store(atv::TOutObjectStream &out, const TTextModel &value)
{
  store(out, value.getValue());
}

bool 
restore(atv::TInObjectStream &in, toad::TTextModel *value)
{
  if (in.what != ATV_VALUE)
    return false;
  value->setValue(in.value);
  return true;
}
