/*
 * TOAD Spreadsheet Example 
 * Copyright (C) 2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <toad/toad.hh>
#include <toad/form.hh>
#include <toad/menubar.hh>
#include <toad/action.hh>
#include <toad/textfield.hh>
#include <toad/pushbutton.hh>
#include <toad/table.hh>
#include <toad/tablemodels.hh>

#include <map>
#include <stdio.h>
#include <unistd.h>

#define DBM(M)

using namespace toad;

struct TField {
  string data;
};

class TSpreadsheetModel:
  public GAbstractTableModel<const TField*>
{
  private:
    typedef map<unsigned, TField*> TRow;
    typedef map<unsigned, TRow> TStorage;
    TStorage storage;
    unsigned rows, cols;
    TField dummy;

  public:
    TSpreadsheetModel() {
      rows = 255;
      cols = 255;
    }
    int getRows() {
      return rows;
    }
    int getCols() {
      return cols;
    }
    const TField* getElementAt(int x, int y) {
      return get(x, y);
    }
    TField* get(int x, int y) {
      TStorage::iterator py = storage.find(y);
      if (py!=storage.end()) {
        TRow::iterator px = (*py).second.find(x);
        if (px!=(*py).second.end()) {
          return (*px).second;
        }
      }
      return NULL;
    }
    void set(int x, int y, TField *field) {
      storage[y][x]=field;
    }
};

typedef GSmartPointer<TSpreadsheetModel> PSpreadsheetModel;

class TSpreadsheetRenderer:
  public TAbstractTableCellRenderer
{
  private:
    PSpreadsheetModel model;
  public:
    TSpreadsheetRenderer(TSpreadsheetModel *m):model(m) {}
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return model->getCols();
    }
    
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    TSpreadsheetModel * getModel() {
      return model;
    }
    
    int getRowHeight(int) {
      DBM(TOADBase::bell();)
      return TOADBase::getDefaultFont().getHeight()+2;
    }

    int getColWidth(int x) {
      DBM(TOADBase::bell());
      int n = model->getRows();
      int max = 80;
      for(int i=0; i<n; i++) {
        const TField *field =  model->getElementAt(0, i);
        int w = 0;
        if (field)
          w = TOADBase::getDefaultFont().getTextWidth(field->data);
        if (w>max)
          max = w;
      }
      return max+2;
    }
    
    virtual void renderItem(TPen &pen, int x, int y, int w, int h, bool selected, bool focus) {
      pen.setLineStyle(TPen::SOLID);
      if (selected && !focus) {
        pen.setColor(TColor::SELECTED);
        pen.fillRectangle(0,0,w, h);
        pen.setColor(TColor::SELECTED_TEXT);
      }
      const TField *field = model->getElementAt(x, y);
      if (field)
        pen.drawString(1, 1, field->data);
      if (selected) {
        pen.setColor(TColor::BLACK);
      }
#warning "Fix me: TTable should draw the border"
      if (focus) {
        pen.drawRectangle(0,0,w, h);
        pen.drawRectangle(-2,-2,w+4, h+4);
      }
    }

};

class TSpreadsheetTable:
  public TTable
{
    typedef TTable super;
    TTextArea *fieldeditor;
  public:
    bool editmode;
  
    TSpreadsheetTable(TWindow *parent, const string &title, TTextArea *fieldeditor):
      super(parent, title) 
    { 
      this->fieldeditor = fieldeditor; 
      editmode = false;
      setTableBorder(1);
    }

#warning "Fix me: Should use keyEvent here!"      
    void keyDown(TKey key, char *string, unsigned modifier);
    void keyUp(TKey key, char *string, unsigned modifier);
};
 
void
TSpreadsheetTable::keyDown(TKey key, char *str, unsigned modifier)
{
  if (editmode) {
    cerr << "field editor" << endl;
    fieldeditor->keyDown(key, str, modifier);
  } else {
    switch(key) {
      case TK_DOWN:
      case TK_UP:
      case TK_LEFT:
      case TK_RIGHT:
      case TK_SHIFT_L:
      case TK_SHIFT_R:
        super::keyDown(key, str, modifier);
        break;
      default:
        cerr << "field editor" << endl;
        editmode = true;
        fieldeditor->keyDown(key, str, modifier);
    }
  }
} 

void
TSpreadsheetTable::keyUp(TKey key, char *str, unsigned modifier)
{
  if (editmode) {
    fieldeditor->keyUp(key, str, modifier);
    cerr << "field editor" << endl;
  } else {
    super::keyUp(key, str, modifier);
  }
}

class TMainWindow:
  public TForm
{
    typedef TForm super;
    TTextModel line;
  public:
    TMainWindow(TWindow *parent, const string &title);
    void create();
    
  protected:
    TSpreadsheetModel *model;
    TTableSelectionModel *selection;
    TSpreadsheetTable *table;
  
    void apply();
    void cancel();
    void cursor();
};

TMainWindow::TMainWindow(TWindow *parent, const string &title):
  super(parent, title)
{
  setBackground(TColor::DIALOG);
}

#warning "Fix me: TForm requires a 'create()' method for its configuration"
void
TMainWindow::create()
{
  TMenuBar *mb = new TMenuBar(this, "menubar");

  TAction *action;
  action = new TAction(this, "file|new");
  // CONNECT(action->sigClicked, this, menuNew);
  action = new TAction(this, "file|open");
  // CONNECT(action->sigClicked, this, menuOpen);
  action = new TAction(this, "file|save");  
  // CONNECT(action->sigClicked, this, menuSave);
  action = new TAction(this, "file|save_as");  
  // CONNECT(action->sigClicked, this, menuSaveAs);
  action = new TAction(this, "file|quit");
  // CONNECT(action->sigClicked, this, menuQuit);

  TTextField *tf = new TTextField(this, "textfield", &line);
#warning "Fix me: TTextField height is 2 pixels too small"
  tf->setSize(320, 17);
  int x = 17-2;
  // connect(line.sigChanged, this, &TMainWindow::apply);

  model = new TSpreadsheetModel();
  TSpreadsheetRenderer *renderer = new TSpreadsheetRenderer(model);
  table = new TSpreadsheetTable(this, "table", tf);
  table->setRenderer(renderer);
  table->setRowHeaderRenderer(new TDefaultTableHeaderRenderer());
  table->setColHeaderRenderer(new TDefaultTableHeaderRenderer(false));
  selection = table->getSelectionModel();
  connect(table->sigCursor, this, &TMainWindow::cursor);

  TPushButton *apply = new TPushButton(this, "!");
  connect(apply->sigClicked, this, &TMainWindow::apply);
  apply->setSize(x,x);
  
  TPushButton *cancel = new TPushButton(this, "X");
  connect(cancel->sigClicked, this, &TMainWindow::cancel);
  cancel->setSize(x,x);

  attach(mb, TOP|LEFT|RIGHT);

  attach(apply, TOP, mb);
  attach(apply, RIGHT, cancel);
//  attach(apply, BOTTOM, table);
  distance(apply, 3);
  
  attach(cancel, TOP, mb);
  attach(cancel, RIGHT);
//  attach(cancel, BOTTOM, table);
  distance(cancel, 3);

  attach(tf, TOP, mb);
  attach(tf, LEFT);
  attach(tf, RIGHT, apply);
  distance(tf, 3);

  attach(table, TOP, tf);
  attach(table, LEFT|RIGHT|BOTTOM);
}

#warning "Fix me: 'line' should be updated when TTables cursor is moved"

void
TMainWindow::cursor()
{
  TField *field = model->get(table->getCursorX(), table->getCursorY());
  if (field) {
    line = field->data;
  } else {
    line="";
  }
  table->editmode = false;
}

void
TMainWindow::apply()
{
  TTableSelectionModel::iterator p, e;
  p = selection->begin();
  e = selection->end();
  while(p!=e) {
    cerr << "apply to " << p.getX() << ", " << p.getY() << endl;
    int x = p.getX();
    int y = p.getY();
    TField *field = model->get(x, y);
    if (!field) {
      field = new TField();
      model->set(x, y, field);
    }
    field->data = line;
    ++p;
  }
#warning "need TTable to invalidate window after changes"
  table->invalidateWindow();
}

void
TMainWindow::cancel()
{
}

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  {
    TMainWindow wnd(0, "TOAD Spreadsheet Example");    
    toad::mainLoop();
/*
    cout << "--------------------------" << endl;
    TTableModel_TNetObject::iterator 
      p(model, table.getSelectionModel()->begin()), 
      e(model, table.getSelectionModel()->end());
    while(p!=e) {
      cout << (*p)->name << endl;
      ++p;
    }
    cout << "--------------------------" << endl;
*/
  }
  toad::terminate();
}
