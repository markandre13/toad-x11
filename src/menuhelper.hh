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

#ifndef TMenuHelper
#define TMenuHelper TMenuHelper

#include <toad/toad.hh>
#include <toad/layout.hh>
#include <list>
#include <set>

namespace toad {

class TMenuEntry;

typedef list<TMenuEntry*> TMenuEntryList;

class TMenuLayout:
  public TLayout
{
    typedef TLayout super;
  public:
    TMenuLayout();
    TMenuLayout(const TMenuLayout&);
    ~TMenuLayout();
    void arrange();
    void actionsChanged();
    
    TMenuEntryList entries;
    
    enum EScope {
      GLOBAL,
      TOPLEVEL,
      INTERACTOR
    } scope;
    TInteractor *interactor;

    SERIALIZABLE_INTERFACE(toad::, TMenuLayout)
};

class TMenuButton;
class TAction;

enum EMenuHelperState {
  MHS_WAIT = 0,
  MHS_DOWN,
  MHS_UP_N_HOLD,
  MHS_DOWN_N_HOLD,
  MHS_RESERVED,
  MHS_DOWN_N_OUTSIDE,
  MHS_DOWN_N_INSIDE_AGAIN
};

class TMenuHelper:
  public TWindow
{
    typedef TWindow super;
    friend class TMenuLayout;
  public:
    TMenuHelper(TWindow*, const string&);
    ~TMenuHelper();
    
    void setScopeInteractor(TInteractor*);

    void adjust();
    void resize();
    void closeRequest();
    
    bool vertical;
    bool close_on_close;
    
    // we have to disable menu buttons on our own
    TMenuButton *active;
    EMenuHelperState state;
    
    TMenuButton *btnmaster;
    
    // data used by menubutton children
    int menu_width_icon;
    int menu_width_text;
    int menu_width_short;
    int menu_width_sub;

    class TNode {
      friend class TMenuHelper;
      public:
        enum EType {
          NORMAL,
          SEPARATOR,
          INSERTION_POINT
        };
        TNode(const string &t,
              const char *l=NULL,
              const char *s=NULL,
              TBitmap *i=NULL,
              EType nt=NORMAL);
        TNode();
        void createWindowAt(TMenuHelper *parent);
        void deleteWindow();
        virtual ~TNode();
        bool isEnabled() const;
        bool isAvailable() const;

        TNode *next;
        TNode *down;
        TNode *parent;

        TAction * addAction(TAction*);
        void removeAction(TAction*);
        virtual void actionChanged();

        /**
         * Triggered, when one of the added actions signaled its own
         * sigChanged signal.
         */
        TSignal sigChanged;

        // interface for the menubuttons        
        void trigger(unsigned); // trigger all actions
        
        const string& getLabel(unsigned idx) const;
        const string& getShortcut() const;
        const string& getTitle() const;
        const TBitmap* getIcon() const;
        
        // added because window is now protected
        int getHeight(); // { return window->Height(); }
        int getWidth(); // { return window->Width(); }
        bool isRealized(); // { return window!=NULL; }
        void setPosition(int x, int y); // { window->SetPosition(x,y); }
        void setSize(int w, int h); // { window->SetSize(w,h); }
        void noWindow();
        
        string title;     // resource name
        EType type;

//      protected:
        typedef std::set<TAction*> TActionSet;
        TActionSet actions;
        
        bool vertical;  // from parent during createAt

        // data from the resource file
        string label;     // text label
        string shortcut;  // shortcut's
        TBitmap *icon;

        unsigned nwinarray;
        TMenuButton **winarray;
        int w, h;
    };
    
    class TRootNode:
      public TNode
    {
        TMenuHelper *owner;
      public:
        TRootNode(TMenuHelper *owner);
        ~TRootNode();
        virtual void actionChanged();
        void clear();
      protected:
        void deleteTree(TNode *p);
    };
    TRootNode root;
    
  private:
    static void sanityCheckTree(TMenuHelper::TNode *p);
};

class TMenuEntry:
  public TSerializable
{
    typedef TSerializable super;
  public:
    string title, label, shortcut, icon;
    TMenuHelper::TNode::EType type;
    TMenuEntryList entries;

    SERIALIZABLE_INTERFACE(toad::, TMenuEntry)
};

class TMenuSeparator:
  public TMenuEntry
{
    typedef TMenuEntry super;
  public:
    SERIALIZABLE_INTERFACE(toad::, TMenuSeparator)
};

} // namespace toad

#endif
