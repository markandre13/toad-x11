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

#ifndef TInteractor
#define TInteractor TInteractor

#include <toad/types.hh>

namespace toad {

class TMouseEvent {
  public:
    enum EType {
      MOVE, ENTER, LEAVE, LDOWN, MDOWN, RDOWN, LUP, MUP, RUP
    } type;
    TWindow *window;
    int x, y;
    unsigned modifier;
};

class TKeyEvent {
  public:
    enum EType {
      DOWN, UP
    } type;
    TWindow *window;
    TKey key;
    char * string;
    unsigned modifier;

    TKey getKey() const { return key; }
    unsigned getModifier() const { return modifier; }
    const char* getString() const { return string; }
};

#ifdef __WIN32__
#ifdef NEW
#undef NEW
#endif

#ifdef DELETE
#undef DELETE
#endif
#endif

class TWindowEvent {
  public:
    enum {
      NEW,
      DELETE,
      CREATE,
      CREATED,
      DESTROY,
      MAPPED,
      UNMAPPED,
      PAINT,
      ADJUST,
      RESIZE,
      FOCUS
    } type;
    TWindow *window;
};  

class TInteractor
{
  public:
    TInteractor(TInteractor *parent = NULL);
    virtual ~TInteractor();

    virtual void setTitle(const string &title) {
      this->title = title;
    }
    const string& getTitle() const { return title; }
    TInteractor* getParent() const {return parent;}
    TInteractor* getFirstChild() const;
    TInteractor* getLastChild() const;
    bool isChildOf(const TInteractor*) const;

    // Get the next/previous sibling of 'ptr' and return NULL when 'ptr'
    // points to the last/first child.
    static TInteractor* getNextSibling(const TInteractor *ptr);
    TInteractor* getNextSibling() const { return getNextSibling(this); }
    static TInteractor* getPrevSibling(const TInteractor *ptr);
    TInteractor* getPrevSibling() const { return getPrevSibling(this); }

    bool bShell:1;
    bool bFocusManager:1;
    bool bFocusTraversal:1;
    bool bNoFocus:1;
    bool before_create:1;

    virtual void windowEvent(TWindowEvent&);
    virtual void mouseEvent(TMouseEvent&);
    virtual void keyEvent(TKeyEvent&);
    
    virtual bool isRealized() const;
    virtual bool setFocus();
    virtual void setPosition(int x,int y);
    virtual void setSize(int x,int y);
    virtual void setShape(int,int,int,int);
    void setShape(const TRectangle &r){setShape(r.x,r.y,r.w,r.h);}
    void setShape(const TRectangle *r){setShape(r->x,r->y,r->w,r->h);}
    virtual void getShape(TRectangle*) const;
    virtual void setSuppressMessages(bool);
    virtual int getXPos() const;
    virtual int getYPos() const;
    virtual int getWidth() const;
    virtual int getHeight() const;

//  protected:

    // to avoid clashes with other methods, these virtual methods are
    // prefixed with 'interactor', to show that they're internal to
    // TInteractor and TWindow
    virtual void _interactor_init(){}
    virtual void _interactor_adjustW2C(){}
    virtual void _interactor_create(){}

    virtual void domainFocus(bool);
    virtual void focus(bool);
    
    unsigned taborder;

  protected:
    bool beforeAddEnabled:1;
    virtual void beforeAdd(TInteractor**);
    string title;

  private:
    TInteractor *parent;
    TInteractor *child;
    TInteractor *next;
};

} // namespace toad

#endif
