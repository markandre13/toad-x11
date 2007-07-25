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

#include "modellayouteditor.hh"

#include <toad/radiobuttonbase.hh>
#include <toad/action.hh>

#include <toad/layout.hh>
#include <toad/dialog.hh>
#include <toad/table.hh>
#include <toad/pushbutton.hh>
#include <toad/textfield.hh>
#include <toad/checkbox.hh>
#include <toad/gauge.hh>
#include <toad/fatradiobutton.hh>
#include <toad/figureeditor.hh>
#include <toad/colorselector.hh>

#include <toad/stl/vector.hh>

#include <typeinfo>

using namespace toad;

TLayoutEditor*
TModelLayout::createEditor(TWindow *inWindow, TWindow *forWindow)
{
  return new TModelLayoutEditor(inWindow, "TModelLayoutEditor", this, forWindow);
}

void
TModelLayoutEditor::TModelListAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      te.w = 40;
      break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = 18;
      break;
    case TTableEvent::PAINT:
      renderBackground(te);
      switch(te.col) {
        case 0:
          te.pen->drawString(1, 1, (*model)[te.row].name);
          break;
      }
      renderCursor(te);
      break;
  }
}

struct TWidget
{
  TWidget(const string &aWidgetClass, const type_info &aModel):
    widget(aWidgetClass), model(&aModel) {}
  string widget;
  const type_info *model;
};
typedef vector<TWidget> TWidgetList;
TWidgetList widgetlist;

// gcc 4.0.1 does not like this one within a function
  // ARGH! This sucks... SIMPLIFY!!!
  struct A: public GTableAdapter< GVector<string> > {
    A(GVector<string> *m):GTableAdapter< GVector<string> >(m) {}
    GVector<string>* getModel() const { return model; }
    void modelChanged(bool newmodel) {
      TTableAdapter::modelChanged(newmodel);
    };
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE: te.w = 40; break;
        case TTableEvent::GET_ROW_SIZE: te.h = 18; break;
        case TTableEvent::PAINT:
          renderBackground(te);
          switch(te.col) {
            case 0:
              te.pen->drawString(1, 1, (*model)[te.row]);
              break;
          }
          renderCursor(te);
          break;
      }
    }
  };

TModelLayoutEditor::TModelLayoutEditor(TWindow *p, const string &t, TModelLayout *l, TWindow *fw):
  TLayoutEditor(p, t), mymousefilter(this), layout(l), forWindow(fw)
{
  setSize(506, 480);
  setBorder(0);
  setBackground(TColor::DIALOG);

  gedit.setWindow(forWindow);
  gedit.setModel(layout->gadgets);
  connect(gedit.sigSelectionChanged, this, &TModelLayoutEditor::selectionChanged);

  tmodel = new TTable(this, "models");
  tmodel->selectionFollowsMouse = true;
  tmodel->setAdapter(new TModelListAdapter(&layout->modellist));
  tmodel->setShape(0,240,240,200);
  CONNECT(tmodel->sigSelection, this, modelSelected);
  
  twidget = new TTable(this, "widgets");
  twidget->selectionFollowsMouse = true;
  twidget->setShape(245,240,240,200);
  
  twidget->setAdapter(new A(&widgets));
  
  TPushButton *pb;
  pb = new TPushButton(this, "+");
  pb->setShape(490,0,16,16);
  CONNECT(pb->sigClicked, this, addWidget);

  labelowner = 0;
  int x, y, w, h, hmax;
  TRadioStateModel *state = new TRadioStateModel();
  TFatRadioButton *rb = NULL;

  x=5; y=5; w=64; h=25; hmax=0;

  static TFCreateTool gframe(new TFFrame);
  static TFCreateTool grect(new TFRectangle);
  static TFCreateTool gcirc(new TFCircle);   
  static TFCreateTool gtext(new TFText);     
  static TFCreateTool gline(new TFLine);     
    
  for(unsigned i=0; i<=5; ++i) {
    rb = NULL;
    switch(i) {
      case 0:  
        rb = new TFatRadioButton(this, "Select", state);
        CONNECT(rb->sigClicked, &gedit, setOperation, TFigureEditor::OP_SELECT);
        rb->setDown(true);
        break;
      case 1: 
        rb = new TFatRadioButton(this, "Frame", state);
        CONNECT(rb->sigClicked, &gedit, setTool, &gframe);
        break;
      case 2: 
        rb = new TFatRadioButton(this, "Line", state);
        CONNECT(rb->sigClicked, &gedit, setTool, &gline);
        break;
      case 3: 
        rb = new TFatRadioButton(this, "Rect", state);
        CONNECT(rb->sigClicked, &gedit, setTool, &grect);
        break;
      case 4: 
        rb = new TFatRadioButton(this, "Circle", state);
        CONNECT(rb->sigClicked, &gedit, setTool, &gcirc);
        break;
      case 5: 
        rb = new TFatRadioButton(this, "Text", state);
        CONNECT(rb->sigClicked, &gedit, setTool, &gtext);
        break;
    }
    if (rb) {
      rb->setShape(x,y,w,h);
      rb->setShape(x,y,w,h);
      y+=h-1;
    }
  }  
  hmax=max(hmax, y);

  x += w+5;
  y = 5;   
  for(unsigned i=0; i<=5; i++) {
    switch(i) {
      case 0:  
        pb = new TPushButton(this, "Top");
        CONNECT(pb->sigClicked, &gedit, selection2Top);
        break;
      case 1: 
        pb = new TPushButton(this, "Up");
        CONNECT(pb->sigClicked, &gedit, selectionUp);
        break;
      case 2: 
        pb = new TPushButton(this, "Down");
        CONNECT(pb->sigClicked, &gedit, selectionDown);
        break;
      case 3: 
        pb = new TPushButton(this, "Bottom");
        CONNECT(pb->sigClicked, &gedit, selection2Bottom);
        break;
      case 4: 
        pb = new TPushButton(this, "Group");
        CONNECT(pb->sigClicked, &gedit, group);
        break;
      case 5: 
        pb = new TPushButton(this, "Ungroup");
        CONNECT(pb->sigClicked, &gedit, ungroup);
        break;
    }
    if (pb) {
      pb->setShape(x,y,w,h);
      y+=h-1;
    }
  }  
     
  hmax=max(hmax, y);
  x += w+5;
  y = 5;   

  TColorSelector *cs = new TColorSelector(this,
                                          "colorselector",
                                          gedit.getAttributes());
  cs->dialogeditorhack = true;
  cs->setShape(x,y,64,32);

  y=80;

  TTextField *tf;

  tf = new TTextField(this, "label", &label);
  tf->setShape(x+40, y, 145,18);
  connect(label.sigChanged, this, &TModelLayoutEditor::labelChanged);

  y+=30;
  tf = new TTextField(this, "width", &width);
  tf->setShape(x+40, y, 40,18);
  tf = new TTextField(this, "height", &height);
  tf->setShape(x+40+40+10, y, 40,18);

  width = layout->width;
  height = layout->height;

  connect(width.sigChanged, this, &TModelLayoutEditor::sizeChanged);
  connect(height.sigChanged, this, &TModelLayoutEditor::sizeChanged);
  
  if (widgetlist.empty()) {
    widgetlist.push_back(TWidget("toad::TCheckBox",       typeid(TBoolModel)));
    widgetlist.push_back(TWidget("toad::TTextArea",       typeid(TTextModel)));
    widgetlist.push_back(TWidget("toad::TTextField",      typeid(TTextModel)));
    widgetlist.push_back(TWidget("toad::TTextField",      typeid(TIntegerModel)));
    widgetlist.push_back(TWidget("toad::TTextField",      typeid(TFloatModel)));
    widgetlist.push_back(TWidget("toad::TScrollBar",      typeid(TIntegerModel)));
    widgetlist.push_back(TWidget("toad::TScrollBar",      typeid(TFloatModel)));
    widgetlist.push_back(TWidget("toad::TGauge",          typeid(TIntegerModel)));
    widgetlist.push_back(TWidget("toad::TGauge",          typeid(TFloatModel)));
    widgetlist.push_back(TWidget("toad::TRadioButton",    typeid(TRadioStateModel)));
    widgetlist.push_back(TWidget("toad::TFatRadioButton", typeid(TRadioStateModel)));
    widgetlist.push_back(TWidget("toad::TPushButton",     typeid(TAction)));
  }
}

TModelLayoutEditor::~TModelLayoutEditor()
{
  layout->editor = 0;
}

void
TModelLayoutEditor::paint()
{
  TPen pen(this);
  
  pen.drawString(128+15+40, 63, selectionname);
  pen.drawString(128+15, 63, "Title:");
  pen.drawString(128+15, 63+20, "Label:");
  pen.drawString(128+15, 63+50, "Size:"); 
  pen.drawString(128+15+82, 63+50, "x");  
}

TEventFilter *
TModelLayoutEditor::getFilter()
{
  return &mymousefilter;
}
 
void
TModelLayoutEditor::enabled()
{
  gedit.getWindow()->invalidateWindow();
}

void
TModelLayoutEditor::selectionChanged()
{
  selectionname = "(none)";
  labelowner = 0;
  if (gedit.selection.size()==1) {
    TFWindow *g = dynamic_cast<TFWindow*>(*gedit.selection.begin());
    if (g) {
      selectionname = g->title;
      if (g->window) {
        TLabelOwner *lo = dynamic_cast<TLabelOwner*>(g->window);
        if (lo)
          label = lo->getLabel();
        labelowner = lo;
      }
    }  
  }    
//  cerr << "new selectionname = '" << selectionname << "'\n";
//  cerr << "label             = '" << label.getValue() << "'\n";
  invalidateWindow(true);
}

void
TModelLayoutEditor::labelChanged()
{
//  cerr << "label changed to '" << label << "'\n";
  if (labelowner)
    labelowner->setLabel(label);
}
 
void
TModelLayoutEditor::sizeChanged()
{
  layout->width = width;
  layout->height = height;
  forWindow->setSize(width, height);
}

void
TModelLayoutEditor::modelSelected()
{
  size_t row = tmodel->getSelectionModel()->getRow();
  cout << layout->modellist[row].name << endl;
  
  TModel *model = layout->modellist[row].model;
  cout << "  model '" << typeid(*model).name() << "'" << endl;
//  cout << "  " << typeid(TTextModel).name() << endl;
  widgets.clear();
  for(TWidgetList::const_iterator p = widgetlist.begin();
      p != widgetlist.end();
      ++p)
  {
    if (typeid(*model)==*(p->model)) {
      cout << p->widget << endl;
      widgets.push_back(p->widget);
    }
  }
}

void
TModelLayoutEditor::addWidget()
{
  if (twidget->getSelectionModel()->empty() ||
      tmodel->getSelectionModel()->empty())
  {
    cout << "nothing selected. should disable push button" << endl;
    return;
  }
  string widget = widgets[twidget->getSelectionModel()->getRow()];
  TModel *model = layout->modellist[tmodel->getSelectionModel()->getRow()].model;
  cout << "add widget '"
       << widget
       << "' for model "
       << model
       << endl;
  TWindow *win = 0;
  if (widget == "toad::TCheckBox") {
    TCheckBox *w = new TCheckBox(layout->getWindow(),
                                  widget + "@" + layout->modellist[tmodel->getSelectionModel()->getRow()].name);
    win = w;
    TBoolModel *m1 = dynamic_cast<TBoolModel*>(model);
    if (m1)
      w->setModel(m1);
  } else
  if (widget == "toad::TScrollBar") {
    TScrollBar *w = new TScrollBar(layout->getWindow(),
                                  widget + "@" + layout->modellist[tmodel->getSelectionModel()->getRow()].name);
    win = w;
    TIntegerModel *m1 = dynamic_cast<TIntegerModel*>(model);
    if (m1)
      w->setModel(m1);
  } else
  if (widget == "toad::TGauge") {
    TGauge *w = new TGauge(layout->getWindow(),
                           widget + "@" + layout->modellist[tmodel->getSelectionModel()->getRow()].name);
    win = w;
    TIntegerModel *m1 = dynamic_cast<TIntegerModel*>(model);
    if (m1)
      w->setModel(m1);
  } else
  if (widget == "toad::TTextArea") {
    TTextArea *w = new TTextArea(layout->getWindow(),
                                 widget + "@" + layout->modellist[tmodel->getSelectionModel()->getRow()].name);
    win = w;
    TTextModel *m0 = dynamic_cast<TTextModel*>(model);
    if (m0) {
      w->setModel(m0);
    } else {
      TIntegerModel *m1 = dynamic_cast<TIntegerModel*>(model);
      if (m1)
        w->setModel(m1);
    }
  } else
  if (widget == "toad::TTextField") {
    TTextField *w = new TTextField(layout->getWindow(),
                                   widget + "@" + layout->modellist[tmodel->getSelectionModel()->getRow()].name);
    win = w;
    TTextModel *m0 = dynamic_cast<TTextModel*>(model);
    if (m0) {
      w->setModel(m0);
    } else {
      TIntegerModel *m1 = dynamic_cast<TIntegerModel*>(model);
      if (m1)
        w->setModel(m1);
    }
  }
  if (win) {
    TFWindow *gw = new TFWindow();
    gw->title  = win->getTitle();
    gw->window = win;
    gw->setShape(win->x, win->y, win->w, win->h);
    layout->gadgets->add(gw);
    win->createWindow();
  }
  cout << "nothin created" << endl;
}

