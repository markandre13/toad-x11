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

#include "modellayout.hh"
#include "modellayouteditor.hh"

#include <toad/io/urlstream.hh>
#include <map>

#include <toad/pushbutton.hh>
#include <toad/textfield.hh>
#include <toad/checkbox.hh>
#include <toad/gauge.hh>
#include <toad/fatradiobutton.hh>
#include <toad/scrollbar.hh>

using namespace toad;

TModelLayout::TModelLayout(const string &filename)
{
  init();
  setFilename(filename);
  try {
    iurlstream url(filename);
    TInObjectStream in(&url);
    if (in) {
      TSerializable *s = in.restore();
      if (!s) {
        cerr << "failed to restore from file: " << in.getErrorText() << std::endl;
        return;
      }
      TModelLayout *l = dynamic_cast<TModelLayout*>(s);
      if (!l) {
        cerr << "failed to restore from file: expected 'toad::TModelLayout'" << std::endl;
        return;
      }
      gadgets = l->gadgets;
      height = l->height;
      width = l->width;
      drawfocus = l->drawfocus;
      
      delete l;
    }
  }
  catch(exception &e) {
    cerr << "loading layout '" << filename << "' failed:\ncaught exception: " << e.what() << endl;
  }
}

TModelLayout::TModelLayout()
{ 
  init(); 
}

void
TModelLayout::init() {
  editor = 0;
  height = 200;
  width  = 320;
  drawfocus = false;
  gadgets = new TFigureModel();
}

void
TModelLayout::paint()
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

typedef map<string, TWindow*> TTitleWindowMap;

static void
arrangeHelper(TModelLayout *layout,
              TFigureModel::iterator p,
              TFigureModel::iterator e,
              TTitleWindowMap &wmap)   
{
  while(p!=e) {
    TFWindow *gw = dynamic_cast<TFWindow*>(*p);
    if (gw) {
      TTitleWindowMap::iterator pm = wmap.find(gw->title);
      if (pm!=wmap.end()) {
        // manage an existing window
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
        gw->window->setToolTip(gw->tooltip);
        wmap.erase(pm);
      } else
      if (!gw->widget.empty()) {
        cout << "create new object " << gw->widget << " for model " << gw->model << endl;
        gw->window = layout->createWidget(gw->widget, gw->model);
        if (gw->window) {
          TRectangle r;
          gw->getShape(&r);
          gw->window->setShape(r);
        }
/*
    gw->title  = win->getTitle();
    gw->setShape(win->x, win->y, win->w, win->h);
    layout->gadgets->add(gw);
    win->createWindow();
*/
        // ...
      }
    }  
    TFGroup *gg = dynamic_cast<TFGroup*>(*p);
    if (gg) {
      arrangeHelper(layout, gg->gadgets.begin(), gg->gadgets.end(), wmap);
    }
    ++p;
  }
}  


void
TModelLayout::arrange()
{
cout << __PRETTY_FUNCTION__ << endl;
  if (!gadgets)
    return;
  window->setSize(width ? width : TSIZE_PREVIOUS,
                  height ? height : TSIZE_PREVIOUS);

  // create a map of all child windows
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
cout << "  arrangeHelper " << gadgets->size() << endl;
  arrangeHelper(this, gadgets->begin(), gadgets->end(), wmap);

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
TModelLayout::store(TOutObjectStream &out) const
{
  ::store(out, "width", width); 
  ::store(out, "height", height);
  ::store(out, "drawfocus", drawfocus);
  ::store(out, gadgets);
}

bool
TModelLayout::restore(TInObjectStream &in)
{
  TFigureModel *m;
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

void
TModelLayout::addModel(TModel *model, const string &name)
{
  modellist.push_back(TModelName(model, name));
}

TWindow*
TModelLayout::createWidget(const string &widget, const string &modelname, TModel *model)
{
  if (!model) {
    for(TModelList::const_iterator p = modellist.begin();
        p != modellist.end();
        ++p)
    {
      if (modelname == p->name) {
        model = p->model;
      }
    }
  }
  if (!model) {
    cerr << "TModelLayout::createWidget: no model '" << modelname << "' registered.\n";
    for(TModelList::const_iterator p = modellist.begin();
        p != modellist.end();
        ++p)
    {
      cerr << "  '" << p->name << "'" << endl;
    }
    return 0;
  }

  TWindow *win = 0;
  if (widget == "toad::TCheckBox") {
    TCheckBox *w = new TCheckBox(getWindow(),
                                 widget + "@" + modelname);
    win = w;
    TBoolModel *m1 = dynamic_cast<TBoolModel*>(model);
    if (m1)
      w->setModel(m1);
  } else
  if (widget == "toad::TScrollBar") {
    TScrollBar *w = new TScrollBar(getWindow(),
                                  widget + "@" + modelname);
    win = w;
    TIntegerModel *m1 = dynamic_cast<TIntegerModel*>(model);
    if (m1)
      w->setModel(m1);
  } else
  if (widget == "toad::TGauge") {
    TGauge *w = new TGauge(getWindow(),
                           widget + "@" + modelname);
    win = w;
    TIntegerModel *m1 = dynamic_cast<TIntegerModel*>(model);
    if (m1)
      w->setModel(m1);
  } else
  if (widget == "toad::TTextArea") {
    TTextArea *w = new TTextArea(getWindow(),
                                 widget + "@" + modelname);
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
    TTextField *w = new TTextField(getWindow(),
                                   widget + "@" + modelname);
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
  return win;
}
