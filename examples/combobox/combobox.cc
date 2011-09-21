/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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
// #include <toad/tablemodels.hh>

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

class TCStringTableAdapter:
  public TSimpleTableAdapter
{
    const char **array;
    size_t n;
  public:
    TCStringTableAdapter(const char **);
    size_t getRows();
    size_t getCols();
    void tableEvent(TTableEvent &te);
};

TCStringTableAdapter::TCStringTableAdapter(const char **a)
{
  array = a;
  n = 0;
  for(; *a; ++a)
    ++n;
}

size_t
TCStringTableAdapter::getRows() {
  return n;
}

size_t
TCStringTableAdapter::getCols() {
  return 1;
}

void
TCStringTableAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE: {
      te.w = 0;
      for(size_t i=0; i<n; ++i) {
        TCoord w = getDefaultFont().getTextWidth(array[i]);
        if (w>te.w)
          te.w = w;
      }
    } break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = getDefaultFont().getHeight()+2;
      break;
    case TTableEvent::PAINT:
      renderBackground(te);
      te.pen->drawString(1, 1, array[te.row]);
      renderCursor(te);
      break;
  }
}

TMyWindow::TMyWindow(TWindow* p, const string& t):
  super(p, t)
{
  TComboBox *cb = new TComboBox(this, "combobox");

  cb->setShape(5,5,200,TSIZE_PREVIOUS);
  cb->setAdapter(new TCStringTableAdapter(name));
//  cb->setSelectionModel(new TSelectionModel());
  cb->getSelectionModel()->select(0,0);
  
  connect(cb->sigSelection, this, &TMyWindow::selected, cb);
}

void
TMyWindow::selected(TComboBox *cb)
{
  cout << "selected: " << cb->getSelectionModel()->getRow() << endl;
}
