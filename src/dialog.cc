/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/dialog.hh>
#include <toad/dialogeditor.hh>
#include <toad/control.hh>
#include <toad/figure.hh>
#include <toad/labelowner.hh>
#include <toad/io/urlstream.hh>

#include <map>

#include <toad/scrollbar.hh>
#include <toad/pushbutton.hh>
#include <toad/fatradiobutton.hh>
#include <toad/textfield.hh>
#include <toad/figureeditor.hh>
#include <toad/colorselector.hh>

namespace toad {

class TLayoutEditDialog:
  public TLayoutEditor
{
  public:
    TLayoutEditDialog(TWindow*, const string&, TDialogLayout*, TWindow *forWindow);
    ~TLayoutEditDialog();
    
    class TMyMouseFilter:
      public TEventFilter
    {
      public:
        TMyMouseFilter(TLayoutEditDialog *parent) {
          this->parent = parent;
        }
        bool mouseEvent(TMouseEvent &me) {
          parent->gedit.mouseEvent(me);
          return true;
        }
        bool keyEvent(TKeyEvent &ke) {
          parent->gedit.keyEvent(ke);
          return true;
        }
        
        TLayoutEditDialog *parent;
    };
    TMyMouseFilter mymousefilter;
    
    TEventFilter * getFilter();
    
    TDialogLayout * layout;
    TWindow * forWindow;
    TFigureEditor gedit;
    
    void paint();

    string selectionname;
    TTextModel label;
    TBoundedRangeModel width, height;
    TLabelOwner * labelowner;
    
    void enabled();
    void selectionChanged();
    void labelChanged();
    void sizeChanged();
};

} // namespace

using namespace toad;

/**
 * \class toad::TDialog
 * TDialog can be used as a parent for dialog windows. It has some special
 * methods to control the children.
 */

TDialog::TDialog(TWindow* parent, const string &title):
  super(parent, title)
{
  setBackground(TColor::DIALOG);
  bShell = bStaticFrame = true;
  bFocusManager = true;
  bDrawFocus = false;
  if (getParent())
    TOADBase::placeWindow(this, PLACE_PARENT_CENTER, getParent());
  setLayout(new TDialogLayout());
}

TDialog::~TDialog()
{
}

void
TDialog::destroy()
{
}

/**
 * When bDrawFocus is true, this method paints a rounded frame around
 * the active control.
 */
void
TDialog::paint()
{
  // move this into TDialogLayout:
  if (bDrawFocus) {
    TWindow *wnd = getFocusWindow();
    if (wnd && wnd->getParent()==this) {
      TPen pen(this);
      TRectangle r;
      wnd->getShape(&r);
      pen.drawRectanglePC(r.x-3,r.y-2,r.w+6,r.h+4);
      pen.drawRectanglePC(r.x-2,r.y-3,r.w+4,r.h+6);
    }
  }
}

void 
TDialog::childNotify(TWindow *who, EChildNotify type)
{
  if (type==TCHILD_FOCUS && bDrawFocus) {
    TRectangle r;
    who->getShape(&r);
    r.x-=3; r.w+=6;
    r.y-=3; r.h+=6;
    invalidateWindow(r);
  }
}

/**
 * Create window as a modal dialog and wait until the dialog is destroyed.
 */
void
TDialog::doModalLoop()
{
  TOADBase::doModalLoop(this);
  flush();
}

void
TDialog::adjust()
{
}

//---------------------------------------------------------------------

namespace toad {

bool
toad::restore(TInObjectStream &p, const char *name, PFigureModel &value)
{
  if (p.what != ATV_GROUP)
    return false;
  if (name) {
    if (p.attribute != name)
      return false;
  } else {
    if (!p.attribute.empty())
      return false;
  }
//  if (p.type.empty() || ... )
//    return false;

  TSerializable *s = p.clone(p.type);
  
  if (!s) {
    return false;
  }

  value = dynamic_cast<TFigureModel*>(s);
  if (!value) {
    p.err << "type mismatch is object store";
    delete s;
    return false;
  }
  p.setInterpreter(value);
  return true;
}

} // namespace toad

typedef map<string, TWindow*> TTitleWindowMap;

/**
 * @sa arrange
 */
static void 
arrangeHelper(TFigureModel::iterator p, 
              TFigureModel::iterator e,
              TTitleWindowMap &wmap)
{
  while(p!=e) {
    TFWindow *gw = dynamic_cast<TFWindow*>(*p);
    if (gw) {
      TTitleWindowMap::iterator pm = wmap.find(gw->title);
      if (pm!=wmap.end()) {
        gw->window = (*pm).second;
        TRectangle r;
        gw->getShape(&r);
        gw->window->setShape(r.x,r.y,r.w,r.h);
        TLabelOwner *lo = dynamic_cast<TLabelOwner*>(gw->window);
        if (lo) {
          if (gw->label.size()!=0)
            lo->setLabel(gw->label);
          gw->label.erase(); // ???
        }
        gw->window->taborder = gw->taborder;
        wmap.erase(pm);
      }
    }
    TFGroup *gg = dynamic_cast<TFGroup*>(*p);
    if (gg) {
      arrangeHelper(gg->gadgets.begin(), gg->gadgets.end(), wmap);
    }
    ++p;
  }
}

TDialogLayout::TDialogLayout()
{
  editor = NULL;
  width = 0;
  height = 0;
  drawfocus = false;
  gadgets = new TFigureModel();
}

TDialogLayout::~TDialogLayout()
{
  if (editor) {
    delete editor;
  }
}

void
TDialogLayout::arrange()
{
  window->setSize(width ? width : TSIZE_PREVIOUS,
                  height ? height : TSIZE_PREVIOUS);

  // create a map of all child windows
  //-----------------------------------
  TTitleWindowMap wmap;
  TInteractor *ip = window->getFirstChild();
  while(ip) {
    TWindow *wnd = dynamic_cast<TWindow*>(ip);
    if (wnd) {
      if (wmap.find(wnd->getTitle())!=wmap.end()) {
        cerr << "  child title \"" << wnd->getTitle() << "\" isn't unique" << endl;
      } else {
        wmap[wnd->getTitle()] = wnd;
      }
    }
    ip = TInteractor::getNextSibling(ip);
  }

  // arrange all child windows with the information store 
  // in their related TFWindow's
  //-----------------------------------------------------
  arrangeHelper(gadgets->begin(), gadgets->end(), wmap);

  // add TFWindow's for the remaining windows
  //-----------------------------------------------------
  TRectangle noshape(4,4,33,33);
  TTitleWindowMap::iterator p,e;
  p = wmap.begin();
  e = wmap.end();
  while(p!=e) {
    TFWindow *gw = new TFWindow();
    gw->title  = (*p).second->getTitle();
    gw->window = (*p).second;
    gw->setShape(noshape.x, noshape.y, noshape.w, noshape.h);
    gw->window->setShape(noshape.x, noshape.y, noshape.w, noshape.h);
    gadgets->add(gw);
    noshape.x+=36;
    if (noshape.x>window->getWidth()) {
      noshape.x=4;
      noshape.y+=36;
    }
    ++p;
  }
}

void
TDialogLayout::paint()
{
  if (!editor || !editor->isEnabled()) {
    TPen pen(window);
    TFigureModel::iterator p = gadgets->begin();
    while(p!=gadgets->end()) {
      (*p)->paint(pen, TFigure::NORMAL);
      ++p;
    }
  } else {
    editor->gedit.paint();
  }
}

void
TDialogLayout::store(TOutObjectStream &out) const
{
  ::store(out, "width", width);
  ::store(out, "height", height);
  ::store(out, "drawfocus", drawfocus);
  ::store(out, gadgets);
}

bool
TDialogLayout::restore(TInObjectStream &in)
{
  TFigureModel *m;
#warning "kludge"
  if (::restorePtr(in, &m)) {
    gadgets = m;
    return true;
  }
  if (
    ::restore(in, "width", &width) ||
    ::restore(in, "height", &height) ||
    ::restore(in, "drawfocus", &drawfocus) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in);
  return false;
}

TLayoutEditor *
TDialogLayout::createEditor(TWindow *inWindow, TWindow *forWindow)
{
  return new TLayoutEditDialog(inWindow, "TDialogLayout.editor", this, forWindow);
}

TLayoutEditDialog::TLayoutEditDialog(TWindow *parent,
                                     const string &title,
                                     TDialogLayout *layout,
                                     TWindow *forWindow)
  :TLayoutEditor(parent, title), mymousefilter(this)
{
  setBackground(TColor::DIALOG);
  gedit.setWindow(forWindow);
  gedit.setModel(layout->gadgets);
  connect(gedit.sigSelectionChanged, this, &TLayoutEditDialog::selectionChanged);
  this->forWindow = forWindow;

  // let the layout redirect it's paint event to our gadget editor...
  layout->editor = this;
  // ... and store a reference to the layout so that we can remove ourself
  // from it later
  this->layout = layout;

  labelowner = 0;
  
  int x, y, w, h, hmax;
  TPushButton *pb;
  TRadioStateModel *state = new TRadioStateModel();
  TFatRadioButton *rb = NULL;
  
  x=5; y=5; w=64; h=25; hmax=0;

  static TFFrame gframe;
  static TFRectangle grect;
  static TFCircle gcirc;
  static TFText gtext;
  static TFLine gline; 
    
  for(unsigned i=0; i<=5; ++i) {
    rb = NULL;
    switch(i) {
      case 0:
        rb = new TFatRadioButton(this, "Select", state);
        CONNECT(rb->sigActivate, &gedit, setOperation, TFigureEditor::OP_SELECT);
        rb->setDown(true);
        break;
      case 1:
        rb = new TFatRadioButton(this, "Frame", state);
        CONNECT(rb->sigActivate, &gedit, setCreate, &gframe);
        break;
      case 2:
        rb = new TFatRadioButton(this, "Line", state);
        CONNECT(rb->sigActivate, &gedit, setCreate, &gline);
        break;
      case 3:
        rb = new TFatRadioButton(this, "Rect", state);
        CONNECT(rb->sigActivate, &gedit, setCreate, &grect);
        break;
      case 4:
        rb = new TFatRadioButton(this, "Circle", state);
        CONNECT(rb->sigActivate, &gedit, setCreate, &gcirc);
        break;
      case 5:
        rb = new TFatRadioButton(this, "Text", state);
        CONNECT(rb->sigActivate, &gedit, setCreate, &gtext);
        break;
    }
    if (rb) {
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
        CONNECT(pb->sigActivate, &gedit, selection2Top);
        break;
      case 1:
        pb = new TPushButton(this, "Up");
        CONNECT(pb->sigActivate, &gedit, selectionUp);
        break;
      case 2:
        pb = new TPushButton(this, "Down");
        CONNECT(pb->sigActivate, &gedit, selectionDown);
        break;
      case 3:
        pb = new TPushButton(this, "Bottom");
        CONNECT(pb->sigActivate, &gedit, selection2Bottom);
        break;
      case 4:
        pb = new TPushButton(this, "Group");   
        CONNECT(pb->sigActivate, &gedit, group);
        break;
      case 5:
        pb = new TPushButton(this, "Ungroup");   
        CONNECT(pb->sigActivate, &gedit, ungroup);
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
                                          gedit.getPreferences());
  cs->dialogeditorhack = true;
  cs->setShape(x,y,64,32);
  
  y=80;
  
  TTextField *tf;
  
  tf = new TTextField(this, "label", &label);
  tf->setShape(x+40, y, 145,18);
  connect(label.sigChanged, this, &TLayoutEditDialog::labelChanged);
  
  y+=30;
  tf = new TTextField(this, "width", &width);
  tf->setShape(x+40, y, 40,18);
  tf = new TTextField(this, "height", &height);
  tf->setShape(x+40+40+10, y, 40,18);
  
  width = layout->width;
  height = layout->height;
  
  connect(width.sigChanged, this, &TLayoutEditDialog::sizeChanged);
  connect(height.sigChanged, this, &TLayoutEditDialog::sizeChanged);

  setSize(5+320+5, hmax+5);
}

void
TLayoutEditDialog::paint()
{
  TPen pen(this);
  
  pen.drawString(128+15+40, 63, selectionname);
  pen.drawString(128+15, 63, "Title:");
  pen.drawString(128+15, 63+20, "Label:");
  pen.drawString(128+15, 63+50, "Size:");
  pen.drawString(128+15+82, 63+50, "x");
}

TLayoutEditDialog::~TLayoutEditDialog()
{
  layout->editor = 0;
}


TEventFilter *
TLayoutEditDialog::getFilter()
{
  return &mymousefilter;
}

void
TLayoutEditDialog::enabled()
{
  gedit.getWindow()->invalidateWindow();
}

void
TLayoutEditDialog::selectionChanged()
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
TLayoutEditDialog::labelChanged()
{
//  cerr << "label changed to '" << label << "'\n";
  if (labelowner)
    labelowner->setLabel(label);
}

void
TLayoutEditDialog::sizeChanged()
{
  layout->width = width;
  layout->height = height;
  forWindow->setSize(width, height);
}
