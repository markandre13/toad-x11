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

#include <toad/toad.hh>

using namespace toad;

/**
 * \class toad::TInteractor
 *  TInteractor is the base class for all objects in the window
 *  hierachy. So the window hierachy isn't limited to windows.<br>
 *  An interactor can't be the parent of a window but window and
 *  interactor can be the parent of an interactor.
 *   <ul>
 *     <li>By <code>TWindow(TWindow*, const string&amp;)</code>
 *         interactor can't be parents of windows.</li>
 *    <li>By <code>TInteractor(TInteractor*)</code> windows can
 *         be parents of interactors.</li>
 *   </ul>
 *   Interactors are (will be) used as
 *  <ul>
 *     <li>lightweight widgets</li>
 *     <li>drop areas for drag'n drop operations</li>
 *     <li>actions that register and unregister themselfes in menubars 
 *         and toolbars</li>
 *     <li>hot spots (invisible area which trigger events on mouse clicks)</li>
 *     <li>figures that use their parent windows for drawing</li>
 *     <li>gesture recognition<br>
 *         (There is a PhD thesis from 1990 by Dean Rubine on gesture 
 *          recognition, which was used for Amulet. His code is under GNU GPL.)
 *   </ul>
 */


TInteractor::TInteractor(TInteractor *parent, const string &title)
{
  taborder=0;
  bShell = bFocusManager = bFocusTraversal = false;
  bNoFocus = true;
  before_create = true;
  beforeAddEnabled = false;
  
  if (parent && parent->beforeAddEnabled) {
    parent->beforeAdd(&parent);
  }

  this->parent = parent;
  this->title = title;
  child = next = NULL;
  
  if (parent) {
    TInteractor *ptr = parent->child;
    if (!ptr) {
      parent->child = this;
    } else {
      // new taborder is calculated as `maximum+1' just in case we
      // encounter gaps
      //----------------------------------------------------------
      taborder = max(taborder, ptr->taborder);
      while(ptr->next) {
        ptr = ptr->next;
        taborder = max(taborder, ptr->taborder);
      }
      ptr->next = this;
      taborder++;
    }
  }
}

TInteractor::~TInteractor()
{
  TInteractor *ptr;
  
  // delete all children
  ptr = getFirstChild();
  while(ptr) {
    TInteractor *w = ptr;
    ptr = ptr->next;
    delete w;
  }

  // remove myself from the parent
  if (parent) {

if (!parent->child) {
  cerr << "child '" << getTitle() << "' was already removed from its parent '"
       << parent->getTitle() << "'\n";
  exit(0);
}
  
    if (parent->child == this) {
      parent->child = next;
      next = NULL;
    } else {
      TInteractor *prev = getPrevSibling(this);
      prev->next = next;
      next = NULL;
    }
  }
}

void TInteractor::windowEvent(TWindowEvent&) {}
void TInteractor::mouseEvent(TMouseEvent&) {}
void TInteractor::keyEvent(TKeyEvent&) {}

/**
 * This method was added for TMDIWindow and allows the window to define a
 * new parent before a child window is being added to it. The 
 * `enableBeforeAdd' attribute toggles whether this method is called.
 */
void TInteractor::beforeAdd(TInteractor**)
{
}

/**
 * Called when the window (interactor) or one of its parents received the 
 * keyboard focus.<br>
 * <i>TWindow::isFocus()</i> will return `true' when the window really has 
 * the focus.
 */
void TInteractor::focus(bool){}

/**
 * Called when the keyboard focus has changed.
 *
 * `true' when the windows (interactors) focus domain received the focus and
 * `false' when the focus domain lost the focus.
 */
void TInteractor::domainFocus(bool){}


/**
 * Returns the windows (interactors) first child. This method is faster
 * than <code>LastChild()</code>.
 */
TInteractor* 
TInteractor::getFirstChild() const
{
  return child;
}

TInteractor* 
TInteractor::getLastChild() const
{
  if (!child)
    return NULL;
    
  TInteractor *ptr = child;
  while(ptr->next) {
    ptr = ptr->next;
  }
  return ptr;
}

/**
 * Returns `true' when argument is a direct or indirect child of this window.
 */
bool 
TInteractor::isChildOf(const TInteractor *p) const
{
  const TInteractor *ptr = this->parent;
  while(ptr) {
    if (ptr==p)
      return true;
    ptr = ptr->parent;
  }
  return false;
}

TInteractor* 
TInteractor::getNextSibling(const TInteractor *ptr)
{
  return ptr->next;
}

/**
 * This method is slower than <code>NextSibling</code>.
 */
TInteractor* 
TInteractor::getPrevSibling(const TInteractor *ptr)
{
  TInteractor *prev = ptr->parent->child;
  if (prev==ptr)
    return 0;
  while(prev->next!=ptr)
    prev = prev->next;
  return prev;
}

bool TInteractor::isRealized() const {return true;}
bool TInteractor::setFocus() {return false;}
void TInteractor::setPosition(int x,int y) {}
void TInteractor::setSize(int x,int y) {}
void TInteractor::setShape(int,int,int,int) {}
//void TInteractor::SetShape(const TRectangle &r){SetShape(r.x,r.y,r.w,r.h);}
//void TInteractor::SetShape(const TRectangle *r){SetShape(r->x,r->y,r->w,r->h);}
void TInteractor::getShape(TRectangle*) const {}
void TInteractor::setSuppressMessages(bool) {}
int TInteractor::getXPos() const {return 0;}
int TInteractor::getYPos() const {return 0;}
int TInteractor::getWidth() const {return 0;}
int TInteractor::getHeight() const {return 0;}

bool TEventFilter::windowEvent(TWindowEvent&) { return false; }
bool TEventFilter::mouseEvent(TMouseEvent&) { return false; }
bool TEventFilter::keyEvent(TKeyEvent&) { return false; }
