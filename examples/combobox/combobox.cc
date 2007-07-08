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

#include <toad/toad.hh>
#include <toad/combobox.hh>
#include <toad/tablemodels.hh>

using namespace toad;

class TMyWindow:
  public TWindow
{
    typedef TWindow super;
  public:
    TMyWindow(TWindow* p, const string& t);
    void selected(TComboBox*);
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  {
    TMyWindow wnd(NULL,"ComboBox eXperiment");
    toad::mainLoop();
  }   
  toad::terminate();
  return EXIT_SUCCESS;
}

static const char* name[] = {
  "blueberry",
  "strawberry",
  "grape",
  "lime",
  "tangerine",
  "bondi-blue",
  "ibm-blue",
  "cray-red",
  "sgi-brown",
  "black-sabbath",
  "deep-purple",
  NULL
};

TMyWindow::TMyWindow(TWindow* p, const string& t):
  super(p, t)
{
  TTableModel_CString *model = new TTableModel_CString(name);
  TTableCellRenderer_CString *renderer = 
    new TTableCellRenderer_CString(model);
  TComboBox *cb = new TComboBox(this, "combobox");

  cb->setShape(5,5,200,TSIZE_PREVIOUS);
  cb->setRenderer(renderer);
  
  connect(cb->sigSelection, this, &TMyWindow::selected, cb);
}

void
TMyWindow::selected(TComboBox *cb)
{
  TTableSelectionModel::iterator p, e;
  p = cb->getSelectionModel()->begin();
  e = cb->getSelectionModel()->end();
  while (p!=e) {
    cout << "selected: " << name[p.getY()] << endl;
    ++p;
  }
}
