/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-Andr� Hopf <mhopf@mark13.de>
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
    
    void setRenderer(TAbstractTableCellRenderer *r) {
      table->setRenderer(r);
    }
    TAbstractTableCellRenderer* getRenderer() const { 
      return table->getRenderer(); 
    }
    void setSelectionModel(TTableSelectionModel *m) {
      table->setSelectionModel(m);
    }
    TTableSelectionModel* getSelectionModel() const {
      return table->getSelectionModel();
    }
    TSignal sigSelection;

  protected:
    void resize();
    void paint();
    void button();
    void selected();

    class TComboButton;
    TComboButton *btn;
    
    TTable *table;
};

} // namespace toad

#endif
