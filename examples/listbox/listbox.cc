/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/table.hh>
#include <toad/checkbox.hh>
#include <toad/tablemodels.hh>

#include <stdio.h>
#include <unistd.h>

using namespace toad;

#define TEST1

#ifdef TEST1
TTable *table = 0;

void
message(const char *msg)
{
  cout << "triggered " << msg << endl;
}

void
foobar(unsigned what, TCheckBox *cb)
{
  cerr << what << " changed to " << cb->getValue() << endl;
  switch(what) {
    case 4:
      table->selectionFollowsMouse = cb->getValue();
      break;
    case 5:
      table->noCursor = cb->getValue();
      break;
    case 8:
      table->stretchLastColumn = cb->getValue();
      break;
  }
  table->invalidateWindow();
  table->setFocus();
}

int
main(int argc, char **argv, char **envv)
{
  static const char* name[] = {
    " 1 blueberry",
    " 2 strawberry",
    " 3 grape",
    " 4 lime",
    " 5 tangerine",
    " 6 bondi-blue",
    " 7 ibm-blue",
    " 8 cray-red",
    " 9 sgi-brown",
    "10 black-sabbath",
    "11 deep-purple",
    NULL
  };

  toad::initialize(argc, argv, envv);
  {
    TDialog *dlg = new TDialog(0, "Table/Listbox Example");
  
    TTableModel_CString *model = new TTableModel_CString(name);
    TCStringSelectionModel sel(model);
    TTableCellRenderer_CString *renderer = 
      new TTableCellRenderer_CString(model);
    table = new TTable(dlg, "table");
    table->setSelectionModel(&sel);
    table->setRenderer(renderer);
    table->setRowHeaderRenderer(new TDefaultTableHeaderRenderer());
    table->setColHeaderRenderer(new TDefaultTableHeaderRenderer());
    connect(table->sigSelection, &message, "sigSelection");
    connect(table->sigCursor,    &message, "sigCursor");
    connect(table->sigClicked,   &message, "sigClicked");
    connect(table->sigDoubleClicked, &message, "sigDoubleClicked");

    TCheckBox *cb;
    cb = new TCheckBox(dlg, "rowheader");
    cb = new TCheckBox(dlg, "colheader");

    cb = new TCheckBox(dlg, "selectionfollowsmouse");
    cb->setValue(table->selectionFollowsMouse);
    connect(cb->getModel()->sigChanged, foobar, 4, cb);

    cb = new TCheckBox(dlg, "nocursor");
    cb->setValue(table->noCursor);
    connect(cb->getModel()->sigChanged, foobar, 5, cb);

    cb = new TCheckBox(dlg, "selectionmodel");
    cb = new TCheckBox(dlg, "tablemodel");

    cb = new TCheckBox(dlg, "stretchlastcolumn");
    cb->setValue(table->stretchLastColumn);
    connect(cb->getModel()->sigChanged, foobar, 8, cb);
    
    dlg->loadLayout("layout.atv");
    toad::mainLoop();
    cout << "--------------------------" << endl;
    TCStringSelectionModel::iterator p, e(sel.end());
    p = sel.begin();
    while(p!=e) {
      cout << *p << endl;
      ++p;
    }
    cout << "--------------------------" << endl;
  }
  toad::terminate();
  return EXIT_SUCCESS;
}
#endif



#ifdef TEST2
struct TNetObject {
  unsigned id;
  const char *name;
  const char *location;
  const char *comment;
  const char *type;
};

class TTableModel_TNetObject:
  public GAbstractTableModel<const TNetObject*>
{
  private:
    const TNetObject *list;
    int size;
  public:
    TTableModel_TNetObject(const TNetObject* l, int s) {
      list = l;
      size = s;
    }
    int getRows() {
      return size;
    }
    const TNetObject* getElementAt(int, int index) {
      return &list[index];
    }
};

typedef GSmartPointer<TTableModel_TNetObject> PTableModel_TNetObject;

class TTableCellRenderer_TNetObject:
  public TAbstractTableCellRenderer
{
  private:
    PTableModel_TNetObject model;
  public:
    TTableCellRenderer_TNetObject(TTableModel_TNetObject *m):model(m) {}
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return 5;
    }
    
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    TTableModel_TNetObject * getModel() {
      return model;
    }
    
    int getRowHeight(int) {
      DBM(TOADBase::bell();)
      return TOADBase::getDefaultFont().getHeight()+2;
    }

    string toText(int x, const TNetObject *obj) const {
      string text;
      switch(x) {
        case 0:
          text = "000";
          break;
        case 1:
          text = obj->name;
          break;
        case 2:
          text = obj->location;
          break;
        case 3:
          text = obj->comment;
          break;
        case 4:
          text = obj->type;
          break;
      }
      return text;
    }

    int getColWidth(int x) {
      DBM(TOADBase::bell());
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(
          toText(x, model->getElementAt(0, i))
        );
        if (w>max)
          max = w;
      }
      return max+2;
    }
    
    virtual void renderItem(TPen &pen, int x, int y, int w, int h, bool selected, bool focus) {
      pen.drawString(1, 1, toText(x, model->getElementAt(0, y)));
    }
};

int
main(int argc, char **argv, char **envv)
{
  static const TNetObject map[] = {
    { 1, "HUB2-2L", "Hall 2 Gate 2L", "not in rack", "SNPX HUB 5000 ENET" },
    { 2, "SWI2-3R", "Hall 2 Gate 3R", "ask Frank", "28XXX Switch" },
    { 3, "SWI1-1", "Hall 1 Gate 2", "ask Frank", "28XXX Switch" },
  };
  toad::initialize(argc, argv, envv);
  {
    TTableModel_TNetObject *model = new TTableModel_TNetObject(map, 3);
    TTableCellRenderer_TNetObject *renderer = 
      new TTableCellRenderer_TNetObject(model);
    TTable table(NULL, "table");
  
    table.setRenderer(renderer);
    table.setRowHeaderRenderer(new TDefaultTableHeaderRenderer());
    table.setColHeaderRenderer(new TDefaultTableHeaderRenderer());
    
    toad::mainLoop();

    cout << "--------------------------" << endl;
    TTableModel_TNetObject::iterator 
      p(model, table.getSelectionModel()->begin()), 
      e(model, table.getSelectionModel()->end());
    while(p!=e) {
      cout << (*p)->name << endl;
      ++p;
    }
    cout << "--------------------------" << endl;
  }
  toad::terminate();
  return EXIT_SUCCESS;
}
#endif
