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

#ifndef _TOAD_TREERENDERER_HH
#define _TOAD_TREERENDERER_HH

#include <toad/treemodel.hh>

namespace toad {

class TTreeAdapter:
  public TTableAdapter
{
  protected:
//    TTreeModel *model;
    
  public:
    virtual TTreeModel* getModel() const = 0;
//    void setModel(TTreeModel*) = 0;

    void handleTree(TTableEvent &te);

    void drawTree(TPen&,int);
    int getLeafPos(size_t);
  
    int getRowHeight(size_t row);
    int getColWidth(size_t col);
    virtual bool isClosed(size_t row);
    void renderItem(TPen &pen, const TTableEvent&);
    void mouseEvent(TMouseEvent &me, int col, int row, int w, int h);
    size_t findOpenRowAbove(size_t row);
    
    size_t getRows() { return getModel() ? getModel()->getRows() : 0; }
};

template <class T>
class GTreeAdapter:
  public TTreeAdapter, public GModelOwner<T>
{
  public:
    GTreeAdapter() { }
    GTreeAdapter(T *m) { GModelOwner<T>::setModel(m); }
    void setModel(T *m) {
//      cout << "GTreeAdapter::setModel("<<m<<")"<< endl;
      GModelOwner<T>::setModel(m);
      reason = TTableModel::CHANGED;
      sigChanged();
    }
    TTreeModel* getModel() const { return GModelOwner<T>::getModel(); }
    void modelChanged(bool newmodel) {
      if (!newmodel) {
        TTreeAdapter::modelChanged();
      } else {
        reason = TTableModel::CHANGED;
        sigChanged();
      }
    }
};

} // namespace toad

#endif
