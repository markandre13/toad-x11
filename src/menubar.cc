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

#define DBM_FEEL(A)

#include <toad/menubar.hh>
#include <toad/toad.hh>
#include <toad/form.hh>
#include <toad/mdiwindow.hh>
#include <toad/pushbutton.hh>
#include <toad/textfield.hh>
#include <toad/action.hh>
#include <toad/utf8.hh>

#include <iostream>
#include <fstream>

using namespace toad;

/**
 * \ingroup control
 * \class toad::TMenuBar
 *
 */

static bool
iterate(TMenuBar::TNode *node, TKeyEvent &ke, unsigned indent=0)
{
  TMenuBar::TNode *p = node;
  while(p) {
//    for(unsigned i=0; i<indent; ++i) cout << "  ";
//    cout << "'" << p->getLabel(0) << "', '" << p->getShortcut() << "'\n";
    string s = p->getShortcut();    
    int state = 0;
    unsigned pos = 0;
    string pattern;
    bool match = true;
    while(state!=3) {
      string c;
      if (pos<s.size()) {
        c = s[pos++];
      } else {
        state = 3;
      }
      switch(state) {
        case 0:
          pattern+=c;
          state = 1;
          break;
        case 1:
          if (c=="+") {
            state = 2;
          } else {
            pattern+=c;
          }
          break;
      }
      
      if (state>=2) {
//        for(unsigned i=0; i<indent; ++i) cout << "  ";
//        cout << "  >>" << pattern << "<<\n";
        if (strcasecmp(pattern.c_str(), "strg")==0 ||
            strcasecmp(pattern.c_str(), "ctrl")==0 )
        {
//cout << "compare for control...\n";
          if (!(ke.getModifier() &  MK_CONTROL)) {
//cout << "  no match\n";
            match = false;
          }
        } else
        if (strcasecmp(pattern.c_str(), "shift")==0)
        {
          if (!(ke.getModifier() &  MK_SHIFT))
            match = false;
        } else {
#warning "crude way to convert key code into string"
          char buffer[2];
          buffer[0]=ke.getKey();
          buffer[1]=0;
//cout << "compare '" << pattern << "' with '" << buffer << "'\n";
          if (strcasecmp(pattern.c_str(), buffer)!=0) {
//cout << "  no match\n";
            match = false;
          }
        }
        if (state==2)
          state=0;
        pattern.clear();
      }
    }
    if (!s.empty() && match) {
//      cout << "found match " << p->getLabel(0) << endl;
      p->trigger(0);
      return true;
    }
    
    if (p->down) {
      if (iterate(p->down, ke, indent+1))
        return true;
    }
    p = p->next;
  }
  return false;
}

class TMenuBar::TMyKeyFilter:  
  public TEventFilter
{
    public:
      TMyKeyFilter(TMenuBar *menubar) {
        this->menubar = menubar;
      }
    protected:
      TMenuBar *menubar;
      bool keyEvent(TKeyEvent &ke) {
//        cout << "TMenuBar keyboard filter: found key event" << endl;
        return iterate(&menubar->root, ke);
      }
};


TMenuBar::TMenuBar(TWindow *p, const string& t):
  super(p, t)
{
  vertical = false; // i'm a horizontal menuhelper
  setLayout(new TMenuLayout()); // i require a layout

  insertEventFilter(new TMyKeyFilter(this), this, KF_TOP_DOMAIN);  
}

TMenuBar::~TMenuBar()
{
}
