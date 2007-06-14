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

#define DBM_FEEL(A)

#include <toad/menubar.hh>
#include <toad/toad.hh>
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
 * A horizontal menubar.
 *
 * The menubar uses a TMenuLayout for its layout definition.
 *
 * TMenuBar also installs a keyboard filter to handle keyboard shortcuts as
 * defined within the layout. Keyboard shortcuts can be defined as one
 * or more names connected with the '+' sign. For example 'Alt+F4'.
 *
 * Known names are case insensitive and can be one of the common keyboard
 * symbols or 'Alt', 'Ctrl', 'Strg', 'Del', 'PgUp', 'PgDown', 'Esc' or 'F1' to 'F20'.
 *
 * \note The name 'Shift' isn't implemented because the symbol which can
 * be typed with 'Shift' is usually already seen on the keyboard. It would
 * also have complicated the implementation of the shortcut code.
 */
static bool
iterate(TMenuBar::TNode *node, const string &str, TKey key, unsigned modifier, unsigned indent=0)
{
  TMenuBar::TNode *p = node;
  while(p) {
    string s = p->getShortcut();    
//cout << "compare shortcut '" << s << "'" << endl;
    int state = 0;
    size_t pos = 0;
    string pattern;
    bool match = true;
    unsigned m = 0;
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
//cout << "pattern '" << pattern << "'" << endl;
        if (strcasecmp(pattern.c_str(), "strg")==0 ||
            strcasecmp(pattern.c_str(), "ctrl")==0 )
        {
          m |= MK_CONTROL;
        } else
        if (strcasecmp(pattern.c_str(), "alt")==0)
        {
          m |= MK_ALT;
        } else
        if (strcasecmp(pattern.c_str(), "shift")==0)
        {
          m |= MK_SHIFT;
        } else
        if (strcasecmp(pattern.c_str(), "pgup")==0 ||
            strcasecmp(pattern.c_str(), "bildhoch")==0)
        {
//cout << "verify pgup: key="<<key<<", TK_PAGE_UP="<<TK_PAGE_UP<<endl;
          if (key!=TK_PAGE_UP)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "pgdown")==0 ||
            strcasecmp(pattern.c_str(), "bildrunter")==0)
        {
          if (key!=TK_PAGE_DOWN)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "esc")==0) {
          if (key!=TK_ESCAPE)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "del")==0 ||
            strcasecmp(pattern.c_str(), "entf")==0) 
        {
          if (key!=TK_DELETE)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f1")==0) {
          if (key!=TK_F1)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f2")==0) {
          if (key!=TK_F2)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f3")==0) {
          if (key!=TK_F3)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f4")==0) {
          if (key!=TK_F4)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f5")==0) {
          if (key!=TK_F5)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f6")==0) {
          if (key!=TK_F6)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f7")==0) {
          if (key!=TK_F7)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f8")==0) {
          if (key!=TK_F8)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f9")==0) {
          if (key!=TK_F9)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f10")==0) {
          if (key!=TK_F10)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f11")==0) {
          if (key!=TK_F11)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f12")==0) {
          if (key!=TK_F12)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f13")==0) {
          if (key!=TK_F13)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f14")==0) {
          if (key!=TK_F14)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f15")==0) {
          if (key!=TK_F15)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f16")==0) {
          if (key!=TK_F16)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f17")==0) {
          if (key!=TK_F17)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f18")==0) {
          if (key!=TK_F18)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f19")==0) {
          if (key!=TK_F19)
            match = false;
        } else
        if (strcasecmp(pattern.c_str(), "f20")==0) {
          if (key!=TK_F20)
            match = false;
        } else {
          if (strcasecmp(pattern.c_str(), str.c_str())!=0) {
            match = false;
          }
        }
        if (state==2)
          state=0;
        pattern.clear();
      }
    }
//cout << "  match="<<match<<",m="<<m<<", modifier="<<modifier<<endl;
    if (!s.empty() && match && m==modifier) {
//      cout << "found match " << p->getLabel(0) << endl;
      p->trigger(0);
      return true;
    }
    
    if (p->down) {
      if (iterate(p->down, str, key, modifier, indent+1))
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
        unsigned m = ke.modifier();
        unsigned orig = m;
        unsigned modifier = 0;
        if (m & MK_CONTROL)
          modifier|=MK_CONTROL;
        if (m & MK_SHIFT)
          modifier|=MK_SHIFT;
        if ((m & MK_ALT) || (m & MK_ALTGR))
          modifier|=MK_ALT;
        m &= ~(MK_SHIFT|MK_CONTROL|MK_ALT|MK_ALTGR);
        ke.setModifier(m);
        string str = ke.str();
        bool result = iterate(&menubar->root, str, ke.key(), modifier);
        ke.setModifier(orig);
        return result;
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
