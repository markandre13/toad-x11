/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#include <toad/dialogeditor.hh>
#include <toad/pushbutton.hh>
#include <toad/fatcheckbutton.hh>

/**
 * \class toad::TDialogEditor
 *
 */

using namespace toad;

class TDialogEditor::TFilter:
  public TEventFilter
{
  public:
    TFilter(TDialogEditor*);
    
    /** window controlling the dialog editor */
    TDialogEditor *parent;
    
    /** window currently edited */
    
    bool windowEvent(TWindowEvent&);
    bool mouseEvent(TMouseEvent &me);
    bool keyEvent(TKeyEvent &key);
};

TDialogEditor::TFilter::TFilter(TDialogEditor *ctrlwindow)
{
  this->parent = ctrlwindow;
}

bool
TDialogEditor::TFilter::windowEvent(TWindowEvent &we)
{
  switch(we.type) {
    case TWindowEvent::DELETE:
    case TWindowEvent::DESTROY:
      if (we.window == parent->editwindow) {
        parent->windowChanged(0);
      }
      break;
    default:
      ;
  }
  return false;
}

bool
TDialogEditor::TFilter::mouseEvent(TMouseEvent &me)
{
//cerr << __FILE__ << ':' << __LINE__ << endl;

  TWindow * w = NULL;
  
  // when the event is part of the dialog editor window, deliver it
  // here. this is to work around modal dialogs
  if (me.window == parent ||
      me.window->isChildOf(parent)) 
  {
//cerr << __FILE__ << ':' << __LINE__ << ':' << me.window->getTitle() << endl;
    me.window->mouseEvent(me);
    return true;
  }

  // when the dialog editor isn't enabled, don't filter anything
  if (!parent->enabled)
    return false;

  bool caught = false;
  
  // in case its a LDOWN event outside the window currently edited
  // try find window with a layout object and when found, use this
  // window
  if (me.type == TMouseEvent::LDOWN &&
      ! (me.window==parent->editwindow || 
         me.window->isChildOf(parent->editwindow)) )
  {
    // search upwards until we find a window to edit
    w = me.window;
    while(w && !w->getLayout()) {
      w = w->getParent();
    }
    if (w)
      parent->windowChanged(w);
    caught = true;
  }
  
  // when a layout editor is active (why don't we check this earlier?)
  if (parent->layouteditor && parent->layouteditor->isEnabled()) {
    TEventFilter *mf = parent->layouteditor->getFilter();
    if (mf) {
      while(me.window && me.window!=parent->editwindow) {
        me.x += me.window->getXPos() + me.window->getBorder();
        me.y += me.window->getYPos() + me.window->getBorder();
        me.window = me.window->getParent();
      }
      if (me.window) {
        if (me.type == TMouseEvent::LDOWN) {
          parent->editwindow->grabMouse(TWindow::TMMM_ALL);
        } else
        if (me.type == TMouseEvent::LUP) {
          parent->editwindow->ungrabMouse();
        }
        mf->mouseEvent(me);
        caught = true;
      }
    }
  }
  return caught;
}

bool
TDialogEditor::TFilter::keyEvent(TKeyEvent &ke)
{
//  cerr << "TMyMouseFilter::keyEvent" << endl;
  if (!parent->layouteditor || !parent->layouteditor->isEnabled())
    return false;

  if (ke.window==parent->editwindow || 
      ke.window->isChildOf(parent->editwindow)) 
  {
//    cerr << "  belongs to edit window" << endl;
    parent->layouteditor->getFilter()->keyEvent(ke);
    return true;
  }
  return false;
}


// TDialogEditor
//---------------------------------------------------------------------------

TDialogEditor::TDialogEditor():
  TDialog(NULL, "Dialog Editor")
{
  setLayout(0);
  TFatCheckButton *fcb;
  fcb = new TFatCheckButton(this, "Edit");
  connect(fcb->sigValueChanged, this, &TDialogEditor::editModeChanged);
  fcb->setShape(5,5,80,25);
  
  TPushButton *pb;
  pb = new TPushButton(this, "Store");
  connect(pb->sigActivate, this, &TDialogEditor::store);
  pb->setShape(5+80+5,5,80,25);
  
  layouteditor = 0;
  editwindow   = 0;
  mousefilter = new TFilter(this);
  insertEventFilter(mousefilter, NULL, KF_GLOBAL);

  enabled = false;
}

TDialogEditor::~TDialogEditor()
{
  if (layouteditor)
    delete layouteditor;
  removeEventFilter(mousefilter);
  delete mousefilter;
}

void
TDialogEditor::windowChanged(TWindow *wnew)
{
  TLayout *layout = NULL;

  if (wnew) {
    layout = wnew->getLayout();
  }

  if (layouteditor) {
    delete layouteditor;
    layouteditor = NULL;
  }

  if (layout)
    layouteditor = layout->createEditor(this, wnew);
  
  if (layouteditor) {
    layout->setModified(true);
    layouteditor->setPosition(5, 5+25+5);
    layouteditor->createWindow();
    setSize(5+layouteditor->getWidth()+5, 5+25+5+layouteditor->getHeight()+5);
  }
  
  if (editwindow)
    editwindow->invalidateWindow();
  editwindow = wnew;
  if (editwindow)
    editwindow->invalidateWindow();
}


void
TDialogEditor::editModeChanged()
{
//  cout << __PRETTY_FUNCTION__ << endl;
  enabled=!enabled;
  if (enabled) {
//    insertEventFilter(mousefilter, NULL, KF_GLOBAL);
    if (layouteditor)
      layouteditor->setEnabled(true);
  } else {
//    removeEventFilter(mousefilter);
    if (layouteditor)
      layouteditor->setEnabled(false);
//    windowChanged(0);
  }
}

void
TDialogEditor::store()
{
  if (editwindow) {
    TLayout *layout = editwindow->getLayout();
    if (layout) {
      layout->toFile();
      layout->setModified(true);
    }
  }
}
