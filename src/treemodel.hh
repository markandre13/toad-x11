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

#ifndef _TOAD_TREEMODEL_HH
#define _TOAD_TREEMODEL_HH

#include <toad/table.hh>

namespace toad {

class TTreeModel:
  public TTableModel
{
  protected:
    struct TRow {
      TRow(void *node, unsigned depth) {
        this->node = node;
        this->depth = depth;
      }
      void *node;
      unsigned depth;
    };
    vector<TRow> *rows;

  public:
    TTreeModel() { rows = new vector<TRow>; }
    ~TTreeModel() { if (rows) delete rows; }
    
    size_t addBefore(size_t row);
    size_t addBelow(size_t row);
    size_t addTreeBefore(size_t row);
    size_t addTreeBelow(size_t row);
    size_t deleteRow(size_t row);
    
    virtual void* _createNode() = 0;
    virtual void _deleteNode(void*) = 0;
    virtual void* _getRoot() const = 0;
    virtual void _setRoot(void*) = 0;
    virtual void* _getDown(void*) const = 0;
    virtual void _setDown(void*, void*) = 0;
    virtual void* _getNext(void*) const = 0;
    virtual void _setNext(void*, void*) = 0;

    void* at(size_t row) const { return (*rows)[row].node; }
    size_t whereIs(void*) const;
    size_t getRows() const { return rows->size(); }
    unsigned getRowDepth(size_t row) const { 
      return (row>=rows->size()) ? 0 : (*rows)[row].depth;
    }
    size_t update(bool signal=true);
    void _update(void *ptr, unsigned depth);

    bool empty() const { return rows->empty(); }
    // size_t size() const { return rows->size(); }
};

/**
 * An template tree model, which assumes that each node contains the
 * pointer attributes 'next' and 'down'.
 */
template <class T>
class GTreeModel:
  public TTreeModel
{
    T *root;
  public:
    GTreeModel() {
      root = 0;
    }
    T& operator[](size_t i) {
      if (i>=rows->size())
        return *static_cast<T*>(0);
      return *static_cast<T*>( (*rows)[i].node );
    }
    T& at(size_t i) {
      if (i>=rows->size())
        return *static_cast<T*>(0);
      return *static_cast<T*>( rows->at(i).node );
    }
    void* _createNode() { return new T(); }
    void _deleteNode(void *n) { delete static_cast<T*>(n); }
    void* _getRoot() const { return root; }
    void _setRoot(void *n) { root = static_cast<T*>(n); }
    void* _getDown(void *n) const { return static_cast<T*>(n)->down; }
    void _setDown(void *n0, void *n1) { static_cast<T*>(n0)->down = static_cast<T*>(n1); }
    void* _getNext(void *n) const { return static_cast<T*>(n)->next; }
    void _setNext(void *n0, void *n1) { static_cast<T*>(n0)->next = static_cast<T*>(n1); }

    T* getRoot() const { return root; }
    void setRoot(void *n) { root = static_cast<T*>(n); }
};

} // namespace toad

#endif
