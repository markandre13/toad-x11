/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#ifndef TTableModel_CString
#define TTableModel_CString TTableModel_CString

#include <toad/table.hh>
#include <string>
#include <vector>

namespace toad {

class TTableModel_CString:
  public GAbstractTableModel<const char *>
{
  private:
    const char **list;
    int size;
  public:
    TTableModel_CString(const char **l, int s): list(l), size(s) {}
    TTableModel_CString(const char **l): list(l) {
      const char **p = list;
      size = 0;
      while(*p) {
        size++;
        p++;
      }
    }
    int getRows() {
      return size;
    }
    const char* getElementAt(int, int index) {
      assert(index<size);
      return list[index];
    }
};

typedef GSmartPointer<TTableModel_CString> PTableModel_CString;
typedef GTableSelectionModel<TTableModel_CString> TCStringSelectionModel;

class TTableCellRenderer_CString:
  public TAbstractTableCellRenderer
{
  private:
    PTableModel_CString model;
  public:
    TTableCellRenderer_CString(TTableModel_CString *m) {
      setModel(m);
    }
    TTableCellRenderer_CString(const char **l) {
      setModel(new TTableModel_CString(l));
    }
    TTableCellRenderer_CString(const char **l, int s) {
      setModel(new TTableModel_CString(l, s));
    }
    void setModel(TTableModel_CString *m) {
      if (model)
        disconnect(model->sigChanged, this);
      model = m;
      if (model)
        connect(model->sigChanged, 
                this, &TTableCellRenderer_CString::modelChanged);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    TTableModel_CString * getModel() {
      return model;
    }
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return model->getCols();
    }
    int getRowHeight(int) {
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int) {
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(model->getElementAt(0, i));
        if (w>max)
          max = w;
      }
      return max+2;
    }
    void renderItem(TPen &pen, int, int index, int w, int h, bool selected, bool focus) {
      if (selected) {
        pen.setColor(TColor::SELECTED);
        pen.fillRectangle(0,0,w, h);
        pen.setColor(TColor::SELECTED_TEXT);
      }
      pen.drawString(
        1, 1, model->getElementAt(0, index)
      );
      if (selected) {
        pen.setColor(TColor::BLACK);
      }
      if (focus) {
        pen.drawRectangle(0,0,w, h);
      }
    }
};

class TStringVector:
  public vector<string>,
  public GAbstractTableModel<string>
{
    typedef vector<string> vec;
  public:
    int getRows() {
      return size();
    }
    string getElementAt(int, int index) {
      assert(index<size());
      return (*this)[index];
    }
    void push_back(const string &s) {
      vec::push_back(s);
      sigChanged();
    }
};

typedef GSmartPointer<TStringVector> PStringVector;
typedef GTableSelectionModel<TStringVector> TStringVectorSelectionModel;

class TTableCellRenderer_String:
  public TAbstractTableCellRenderer
{
  private:
    PStringVector model;
  public:
    TTableCellRenderer_String(TStringVector *m)
    {
      setModel(m);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    TStringVector * getModel() {
      return model;
    }
    void setModel(TStringVector *m) {
      if (model)
        disconnect(model->sigChanged, this);
      model = m;
      if (model)
        connect(model->sigChanged,
                this, &TTableCellRenderer_String::modelChanged);
    }
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return model->getCols();
    }
    int getRowHeight(int) {
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int) {
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(model->getElementAt(0, i));
        if (w>max)
          max = w;
      }
      return max+2;
    }
    void renderItem(TPen &pen, int, int index, int w, int h, bool selected, bool focus) {
      if (selected) {
        pen.setColor(TColor::SELECTED);
        pen.fillRectangle(0,0,w, h);
        pen.setColor(TColor::SELECTED_TEXT);
      }
      pen.drawString(
        1, 1, model->getElementAt(0, index)
      );
      if (selected) {
        pen.setColor(TColor::BLACK);
      }
      if (focus) {
        pen.drawRectangle(0,0,w, h);
      }
    }
};

} // namespace toad

#endif
