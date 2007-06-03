/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/treemodel.hh>
#include <algorithm>

using namespace toad;

#define AUTOMATIC_UPDATE
#define DBM(CMD)

/**
 * Update the models internal data structure in case the tree was
 * modified. The method will detect simple add and remove modifications
 * and inform TTable. In case the algorithm implemented here fails your
 * requirements, pass 'false' to the method to disable this mechanism.
 *
 * The code which perform the signaling will always be executed as its
 * main task is to copy the information whether a node is opened or closed
 * from the old structure to the new one. (But this behaviour MAY change in
 * the future, which means disabling the automatic signaling and doing it
 * on your own MAY increase your applications performance.)
 *
 * \param signal
 *   When 'true', which is the default, the TTable views will be notified
 *   about the change. When 'false', you have to notify TTable on your own.
 * \return
 *   When signal was set to 'true', the return value suggests a new row for
 *   the cursor within the table.
 */
size_t
TTreeModel::update(bool signal)
{
  // we're going to rebuild the whole rows vector, which is the KISS approach
  // by now.

  vector<TRow> *orows = rows;
  rows = new vector<TRow>();
  // rows->clear();
  _update(0, 0);

DBM(  
{
  cout << "--------------------- begin update ------------------" << endl;
  unsigned i;
  cout << "orows:" << endl;
  i=0;
  for(vector<TRow>::iterator p = orows->begin();
      p != orows->end();
      ++p)
  {
    cout << i++ << ":" << p->node << endl;
  }
  cout << "rows:" << endl;
  i=0;
  for(vector<TRow>::iterator p = rows->begin();
      p != rows->end();
      ++p)
  {
    cout << i++ << ":" << p->node << endl;
  }
  cout << "----------------------- do update --------------------" << endl;
}
)
  
  // afterwards we need to copy the 'closed' flags from the old vector to the
  // new vector. and while we're spending time doing it, we can also try to
  // find out what we're going to tell TTable what has happend.
  bool depthchangeflag = false;
  size_t depthchangestart = 0;
  
  bool removedflag = false;
  size_t removedstart = 0;

  vector<TRow>::iterator p, q, p0, q0;
  p = orows->begin();
  q = rows->begin();
  while(true) {
    // reached tail of new or old vector
    if (p==orows->end() || q==rows->end())
      break;
    
    if (p->node == q->node) {
//      q->closed = p->closed;
    } else {
      // failed to match entry: walk through the new list and in case we
      // find the current entry in the old list, assume that new entries have
      // been added here.
      q0 = q; // q0 = new rows
      ++q0;
      while(q0!=rows->end()) {
        if (p->node == q0->node) {
          where = q - rows->begin();
          DBM(cout << "found new: where=" << where << ", size=" << (q0-q) << endl;)
          if (signal) {
            reason = INSERT_ROW;
            size  = q0 - q;
            sigChanged();
          }
          orows->insert(p, q, q0);
          p = orows->begin()+where+size;
          q = q0;
          break;
        }
        ++q0;
      }
      // no new entry found, lets check for removed entries: walk through
      // the old list and in case we find the current entry in the new list,
      // assume that entries have been removed here.
      if (q0==rows->end()) {
        p0 = p; // p0 = oldrows
        ++p0;   // we've already compare q->node & p->node so skip it
        while(p0!=orows->end()) {
          if (p0->node == q->node) {
            DBM(cout << "found equal old and new nodes " << p0->node << endl;)
            where = p - orows->begin();
            DBM(cout << "found removed: where=" << where << ", size=" << (p0-p) << endl;)
            if (signal) {
              reason  = REMOVED_ROW;
              removedstart = where;
              size  = p0 - p;
              removedflag = true;
              sigChanged();
            }
            orows->erase(p, p0); // erase from orows so algorithm matches table
            p0 = p = orows->begin()+where;
            break;
          }
          ++p0;
        }
        if (p0==orows->end()) {
          // cout << "TTreeAdapter::update: i have no idea what's going on" << endl;
          DBM(cout << "found removed and added, assume whole change" << endl;)
          if (signal) {
            reason = REMOVED_ROW;
            where = p - orows->begin();
            size  = orows->end() - p;
            DBM(cout << "assume removed: where=" << (p - orows->begin()) << ", size=" << (p0-p) << endl;)
            sigChanged();
            
            reason = INSERT_ROW;
            where = 0;
            size = rows->size();
            DBM(cout << "assume insert: where=" << (p - orows->begin()) << ", size=" << (p0-p) << endl;)
            sigChanged();
          }            
          delete orows;
          return 0;
        }
      }
    }
    
    if (q->depth != p->depth) {
      // cout << "different depth for " << (q - rows->begin()) << endl;
      if (!depthchangeflag) {
        depthchangeflag = true;
        depthchangestart = p - orows->begin();
      }
    } else {
      if (depthchangeflag) {
        depthchangeflag = false;
        if (signal /*&& type==REMOVED_ROW*/) {
          // THIS IS A ADAPTER MESSAGE, NOT A MODEL MESSAGE!!!
          reason  = RESIZED_ROW;
          where = depthchangestart;
          size  = (p - orows->begin()) - depthchangestart;
//          cout << "signal size change from where=" << where << ", size=" << size << endl;
          sigChanged();
        }
      }
    }
    ++p;
    ++q;
  } // end of loop
  
  // handle tails
  if (p!=orows->end()) {
//    cout << "found removed: where=" << (p - orows->begin()) << ", size=" << (orows->end() - p) << endl;
    if (signal) {
      reason = REMOVED_ROW;
      removedstart = where = p - orows->begin();
      size  = orows->end() - p;
      sigChanged();
      removedflag = true;
    }
  }
  if (q!=rows->end()) {
//    cout << "found new: where=" << (q - rows->begin()) << ", size=" << (rows->end() - q) << endl;
    if (signal) {
      reason = INSERT_ROW;
      where = q - rows->begin();
      size  = rows->end() - q;
      sigChanged();
    }
  }
  if (depthchangeflag) {
    depthchangeflag = false;
    if (signal /*&& type==REMOVED_ROW*/) {
      // THIS IS A ADAPTER MESSAGE, NOT A MODEL MESSAGE!!!
      reason  = RESIZED_ROW;
      where = depthchangestart;
      size  = (p - orows->begin()) - depthchangestart;
//      cout << "signal size change from where=" << where << ", size=" << size << endl;
      sigChanged();
    }
  }

  int cy = where;
  if (signal) {
    if (removedflag) {
      cy = where = removedstart;
//      cout << "using removedstart=" << removedstart << endl;
      if (where>0) {
        if ( where>=rows->size()) {
          cy = rows->size() - 1;
        } else
        if (where+1<orows->size() &&
            (*orows)[where].depth > (*orows)[where+1].depth )
        {
//          cout << "*** one up ***" << endl;
          --cy;
        }
      }
    }
    // cy = findOpenRowAbove(cy);
    // go up to find first visible item...
    
    // place cursor to a valid position
    // insert: at the first new entry (err, what about being visible?)
    // delete: the first visble entry above the deleted item
  }
  
  delete orows;
DBM(
  cout << "----------------------- did update --------------------" << endl;
switch(reason) {
  case CHANGED: cout << "CHANGED" << endl; break;
  case INSERT_ROW: cout << "INSERT_ROW" << endl; break;
  case RESIZED_ROW: cout << "RESIZED_ROW" << endl; break;
  case REMOVED_ROW: cout << "REMOVED_ROW" << endl; break;
  default: cout << "DEFAULT..." << endl;
}
)
  return cy;
}

void
TTreeModel::_update(void *ptr, unsigned depth)
{
//cerr << "update: tree = " << root() << endl;
  if (!ptr) {
    ptr = _getRoot();
  }
  
  while(ptr) {
    rows->push_back(TRow(ptr, depth));
    void *d = _getDown(ptr);
    if (d) {
      _update(d, depth+1);
    }
    ptr = _getNext(ptr);
  }
}

size_t
TTreeModel::whereIs(void *ptr) const
{
  size_t i = 0;
  for(vector<TRow>::const_iterator p = rows->begin();
      p != rows->end();
      ++p)
  {
    if (p->node == ptr)
      return i;
    ++i;
  }
  return (size_t)-1;
}

size_t
TTreeModel::addBefore(size_t row)
{
// if (!getRoot()) return 0;

  void *nn = _createNode();
  // nn->name = number();

  if (!rows->empty()) {
    // find parent (either down or next)
    void *dn = (*rows)[row].node;
    void *pn;
    vector<TRow>::iterator p;
    for(p = rows->begin();
        p != rows->end();
        ++p)
    {
      pn = p->node;
      if (_getNext(pn) == dn) {
        DBM(cout << "before next" << endl;)
        _setNext(pn, nn);
        _setNext(nn, dn);
        break;
      }
      if (_getDown(pn) == dn) {
        DBM(cout << "before down" << endl;)
        _setDown(pn, nn);
        _setNext(nn, dn);
        break;
      }
    }

    if (p==rows->end()) {
      _setNext(nn, _getRoot());
      _setRoot(nn);
      row = 0;
    }
  } else {
    _setRoot(nn);
    row = 0;
  }

DBM(cout << "insert 4: where=" << row << ", size=1" << endl;)
#ifdef AUTOMATIC_UPDATE
  return update();
#else
  update(false);
  reason = INSERT_ROW;
  where = row;
  size = 1;
  sigChanged();
  return row;
#endif
}

size_t 
TTreeModel::addBelow(size_t row)
{
//  if (!_root) return 0;
//  cout << p->name << endl;
  void *np = _createNode();

  if (!rows->empty()) {
    if (row>rows->size()) {
      cout << "TTreeModel::addBelow(" << row << ") is out of range" << endl;
      return rows->size();
    }
    void *p = (*rows)[row].node;
    if (!p) {
      cout << "TTreeModel::addBelow: row contains no node" << endl;
      return row;
    }
    _setNext(np, _getNext(p));
    _setNext(p, np);
  } else {
    _setRoot(np);
  }

DBM(cout << "model: insert 3: where=" << whereIs(np) << ", size=1" << endl;)
#ifdef AUTOMATIC_UPDATE
  return update();
#else
  update(false);
  reason = INSERT_ROW;
  where = whereIs(np);
  size = 1;
DBM(cout << "model " << this << ": trigger signal" << endl;)
  sigChanged();
  return where;
#endif
}

size_t
TTreeModel::addTreeBelow(size_t row)
{
//  if (!_root) return 0;

//  cout << p->name << endl;
  void *np = _createNode();

  if (!rows->empty()) {
    if (row>rows->size()) {
      cout << "warning: TTreeModel::addTreeBelow("<<row<<") is out of range, using end" << endl;
      row = rows->size()-1;
    }
    void *p = (*rows)[row].node;
    _setDown(np, _getDown(p));
    _setDown(p, np);
  } else {
    _setRoot(np);
  }
  
DBM(cout << "insert 1: where=" << (row+1) << ", size=1" << endl;)
#ifdef AUTOMATIC_UPDATE
  return update();
#else
  update(false);
  type = INSERT_ROW;
  where = row+1;
  size = 1;
  sigChanged();
  return row+1;
#endif
}

size_t
TTreeModel::addTreeBefore(size_t row)
{
//  if (!_root) return 0;

cout << __PRETTY_FUNCTION__ << endl;
  void *nn = _createNode();
//  nn->name = number();

  if (!rows->empty()) {
#if 0
    // approach: find first and last of selection and use 'em for
    // insert, in case there's no selection model, first & last are
    // the same
    void *first, *last;
    if (sm) {
      size_t idx;
      idx = row;
      while(idx>0 && sm->isSelected(0,idx-1))
         --idx;
      first = (*rows)[idx].node;
      idx = row;
      while(idx+1<rows->size() && sm->isSelected(0,idx+1))
         ++idx;
      last = (*rows)[idx].node;
    } else {
      first = last = (*rows)[row].node;
    }
#else
    void *first, *last;
    first = last = (*rows)[row].node;
#endif

    // find parent (either down or next)
    // TPlainSlide *dn = static_cast<TPlainSlide*>((*rows)[row].node);
    void *pn;
    vector<TRow>::iterator p;
    for(p = rows->begin();
        p != rows->end();
        ++p)
    {
      pn = p->node;
      if (_getNext(pn) == first) {
        DBM(cout << "before next" << endl;)
        _setNext(pn, nn);
        _setNext(nn, _getNext(last));
        _setDown(nn, first);
        _setNext(last, 0);
        break;
      } else
      if (_getDown(pn) == first) {
        DBM(cout << "before down" << endl;)
        _setDown(pn, nn);
        _setDown(nn, first);
        break;
      }
    }
    if (p==rows->end()) {
      _setDown(nn, _getRoot());
      _setRoot(nn);
      row = 0;
    }
  } else {
    _setRoot(nn);
    row = 0;
  }

DBM(cout << "insert 2: where=" << row << ", size=1" << endl;)
#ifdef AUTOMATIC_UPDATE
  return update();
#else
  update(false);
  type = INSERT_ROW;
  where = row;
  size = 1;
  sigChanged();
  return row;
#endif
}

size_t
TTreeModel::deleteRow(size_t row)
{
  // if (!_root) return 0;

  if (row>=rows->size()) {
    DBM(cout << "nothin' to delete" << endl;)
    return row;
  }

  void *dn = (*rows)[row].node;

  // find parent (either down or next)
  void *pn;
  vector<TRow>::iterator p;
  for(p = rows->begin();
      p != rows->end();
      ++p)
  {
    pn = p->node;
    // parent has us in 'next'
    if (_getNext(pn) == dn) {
      DBM(cout << "next" << endl;)
      if (!_getDown(dn)) {
        _setNext(pn, _getNext(dn));
      } else {
        // attention: when dn is closed, we need to resize the children!!
        _setNext(pn, _getDown(dn));
        pn = _getDown(dn);
        while(_getNext(pn)) {
          pn = _getNext(pn);
        }
        _setNext(pn, _getNext(dn));
      }
      break;
    }
    // parent has us in 'down'
    if (_getDown(pn) == dn) {
      DBM(cout << "down" << endl;)
      if (!_getDown(dn)) {
        _setDown(pn, _getNext(dn));
      } else {
        // attention: when dn is closed, we need to resize the children!!
        _setDown(pn, _getDown(dn));
        pn = _getDown(dn);
        while(_getNext(pn)) {
          pn = _getNext(pn);
        }
        _setNext(pn, _getNext(dn));
      }
      break;
    }
  }
  if (p==rows->end()) {
    DBM(cout << "delete without a parent" << endl;)
    if (_getRoot()) {
      dn = _getRoot();
      if (_getDown(dn)) {
        DBM(cout << "  found a down" << endl;)
        pn = _getDown(dn);
        while(_getNext(pn))
          pn = _getNext(pn);
        _setNext(pn, _getNext(dn));
        _setRoot(_getDown(dn));
      } else {
        _setRoot(_getNext(dn));
      }
    } else {
      DBM(cout << "found no parent" << endl;)
      return row;
    }
//    exit(0);
  }

  // inform listeners, that we're going to delete the entry, then delete it
  DBM(cout << "remove: where=" << row << ", size=1" << endl;)
#ifdef AUTOMATIC_UPDATE
  row = update();
#else
  update(false);
  type = REMOVED_ROW;
  where = row;
  size = 1;
  sigChanged();
#endif
  _deleteNode(dn);
  return row;
}

/**
 * Reorder tree.
 *
 * \param mark
 *   A list of selected rows in the tree. The first column (x=0) is
 *   used.
 * \param row
 *   The destination row
 * \param asParent
 *   false: move selected rows behind 'row'
 *   true: 'row' becomes a parent of the selected rows
 *
 * reorder({row 4 selected}, 0, true) will make row 4 the root of the tree
 */
void
TTreeModel::reorder(TAbstractSelectionModel *selection, size_t row, bool asParent, TTable *table)
{
//  cout << "TTreeModel::reorder(..., "<<row<<", "<<asParent<<")"<< endl;

  // we must use the _* function to work the tree
  // 'rows' provides the relation to the selection model
  
  // 1st: reorder the 'rows' table
/*
cout << "before:" << endl;
  for(size_t i=0; i<rows->size(); ++i) {
    cout << i << ": " << (*rows)[i].depth << " " << (*rows)[i].node << endl;
  }
*/
  vector<bool> mark;
  mark.resize(rows->size(), false);
  size_t marked=0;
  for(size_t i=0; i<rows->size(); ++i) {
    if ( selection->isSelected(0, i) ) {
      mark[i] = true;
      ++marked;
    }
  }

//  vector<bool> rowopen;
  if (table) {
//    rowopen.resize(rows->size(), true);
    // fill in missing mark definition and set the mark of hidden
    // children to the parents mark value
    int last_depth = -1;
    for(size_t i=0; i<rows->size(); ++i) {
//      if (!table->isRowOpen(i))
//        rowopen[i] = false;
      if (last_depth!=-1) {
        if (last_depth < (*rows)[i].depth) {
          if ( !mark[i] ) {
            mark[i] = true;
            ++marked;
          }
        } else {
          last_depth = -1;
        }
      }
      if (last_depth == -1) {
        if (!table->isRowOpen(i) && mark[i]) {
          last_depth = (*rows)[i].depth;
        }
      }
    }
  }

  // new depth of moved rows will be relative to destination
  int last_depth = -1;
  int dest_depth;
  if (row==0)
    dest_depth = 0;
  else
    dest_depth = (*rows)[row-1].depth;
  if (asParent)
    ++dest_depth;
    
  // skip subtree
  while(row<rows->size() && (*rows)[row].depth>dest_depth)
    ++row;
  
  // create alternative order in 'order' and adjust depth for each entry
  vector<unsigned> order;
  order.resize(rows->size(), 0);
  size_t hd_cntr = 0;
  size_t md_cntr = row;
  for(size_t i=0; i<row; ++i) {
    if ( mark[i] )
      --md_cntr;
  }
  size_t tl_cntr = md_cntr+marked;
  for(size_t i=0; i<rows->size(); ++i) {
    if ( mark[i] ) {
      order[i] = md_cntr++;
      // adjust depth of moved rows
      if (last_depth == -1) {
        last_depth = (*rows)[i].depth;
      }
      (*rows)[i].depth += dest_depth - last_depth;
    } else {
      if (i < row)
        order[i] = hd_cntr++;
      else
        order[i] = tl_cntr++;
      if (last_depth!=-1 && last_depth<(*rows)[i].depth) {
        (*rows)[i].depth--;
      } else {
        last_depth = -1;
      }
    }
  }
/*
cout << "order:" << endl;  
for(size_t i=0; i<order.size(); ++i) {
  cout << i << ": " << order[i] << " " << (*rows)[i].depth << " " << (*rows)[i].node << endl;
}
*/
  // sort 'rows' using 'order'
  for(size_t i=0; i<rows->size(); ++i) {
    for(size_t j=0; j<rows->size(); ++j) {
      if (order[i]<order[j]) {
//        int o = order[i]; order[i]=order[j]; order[j]=o;
        swap(order[i], order[j]);
//        void *n = (*rows)[i].node; (*rows)[i].node = (*rows)[j].node; (*rows)[j].node = n;
        swap((*rows)[i], (*rows)[j]);
#if 0
        if (table) {
          // swap(rowopen[i], rowopen[j]);
          bool b = rowopen[i];
          rowopen[i] = rowopen[j];
          rowopen[j] = b;
        }
#endif
      }
    }
  }
/*
cout << "order:" << endl;  
for(size_t i=0; i<order.size(); ++i) {
  cout << i << ": " << order[i] << endl;
}

cout << "after: rows->size()=" << rows->size() << endl;
  for(size_t i=0; i<rows->size(); ++i) {
    cout << i << ": " << (*rows)[i].depth << " " << (*rows)[i].node << endl;
  }
*/
  // 2nd: construct a new tree based on the 'rows' table
  for(size_t i=0; i<rows->size(); ++i) {
    _setNext((*rows)[i].node, 0);
    _setDown((*rows)[i].node, 0);
  }

  _setRoot((*rows)[0].node);
//cout << "first is " << (*rows)[0].node << endl;
  vector<void*> stack;
  stack.push_back((*rows)[0].node);
  for(size_t i=1; i<rows->size(); ++i) {
    if ((*rows)[i-1].depth==(*rows)[i].depth) {
//cout << "next of " << stack.back() << " is " << (*rows)[i].node << endl;
      _setNext(stack.back(), (*rows)[i].node);
      stack.back() = (*rows)[i].node;
    } else
    if ((*rows)[i-1].depth<(*rows)[i].depth) {
//cout << "down of " << stack.back() << " is " << (*rows)[i].node << endl;
      _setDown(stack.back(), (*rows)[i].node);
      stack.push_back((*rows)[i].node);
    } else {
//cout << "stack size " << stack.size() << endl;
      stack.resize((*rows)[i].depth+1);
//cout << "stack size " << stack.size() << endl;
//stack.pop_back();
//cout << "Next of " << stack.back() << " is " << (*rows)[i].node << endl;
      _setNext(stack.back(), (*rows)[i].node);
      stack.back() = (*rows)[i].node;
    }
  }
/* 
cout<<"tree now:"<<endl;
void *n = _getRoot();
while(n) {
  cout << n << endl;
  void *n1 = _getDown(n);
  if (n1)
    cout << "  " << n1 << endl;
  n = _getNext(n);
};
*/
  update(false);
  reason = CHANGED; // well, not exactly a new model...
  sigChanged();
#if 0
  if (table) {
    for(size_t i=0; i<rows->size(); ++i) {
      if (!rowopen[i]) {
cout << "close row " << i << endl;
        table->setRowOpen(i, false);
      }
    }
  }
#endif
}
