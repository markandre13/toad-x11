/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef TComboBox
#define TComboBox TComboBox

#include <toad/table.hh>

namespace toad {

class TComboBox:
  public TWindow
{
    typedef TWindow super;
  public:
    TComboBox(TWindow * parent, const string &title);
    ~TComboBox() {
      if (table->getRenderer())
        disconnect(table->getRenderer()->sigChanged, this);
      if (table->getSelectionModel())
        disconnect(table->getSelectionModel()->sigChanged, this);
    }
    
    void setRenderer(TAbstractTableCellRenderer *r) {
      if (table->getRenderer())
        disconnect(table->getRenderer()->sigChanged, this);
      table->setRenderer(r);
      if (r)
        connect(r->sigChanged, this, &TComboBox::_rendererChanged);
    }
    TAbstractTableCellRenderer* getRenderer() const { 
      return table->getRenderer(); 
    }

    void setSelectionModel(TAbstractTableSelectionModel *m) {
      if (table->getSelectionModel())
        disconnect(table->getSelectionModel()->sigChanged, this);
      table->setSelectionModel(m);
      if (m)
        connect(m->sigChanged, this, &TComboBox::_selectionChanged);
    }
    TAbstractTableSelectionModel* getSelectionModel() const {
      return table->getSelectionModel();
    }
    TSignal sigSelection;
    int getLastSelectionCol() const { return table->getLastSelectionCol(); }
    int getLastSelectionRow() const { return table->getLastSelectionRow(); }

    void setCursor(int col, int row) { table->setCursor(col, row); }
    int getCursorCol() const { return table->getCursorCol(); }
    int getCursorRow() const { return table->getCursorRow(); }
    void selectAtCursor() { table->selectAtCursor(); }
    void clickAtCursor() { table->clickAtCursor(); }
    void doubleClickAtCursor() { table->doubleClickAtCursor(); }

  protected:
    void resize();
    void paint();
    void closeRequest();
    void focus(bool);
    void mouseLDown(int, int, unsigned);
    void keyDown(TKey, char*, unsigned);
    void button();
    void selected();

    void _rendererChanged();
    void _selectionChanged();

    class TComboButton;
    TComboButton *btn;
    
    TTable *table;
};

} // namespace toad

#endif
