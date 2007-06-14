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

#ifndef TMenuButton
#define TMenuButton TMenuButton

#include <toad/menuhelper.hh>
#include <toad/toad.hh>

namespace toad {

class TPopup;

class TMenuButton:
  public TWindow
{
    friend class TMenuHelper;
    friend class TCommandCollapseMenu;
    friend class TMenuKeyFilter;
    TMenuHelper::TNode *node;
    TMenuHelper *master;

  public:
    TMenuButton(TMenuHelper *menubar, TMenuHelper::TNode *node);
    ~TMenuButton();
    void adjustButton();
    void paint();
    void mouseLDown(const TMouseEvent&);
    void mouseLUp(const TMouseEvent&);
    void mouseRDown(const TMouseEvent&);
    void mouseRUp(const TMouseEvent&);
    void mouseEnter(const TMouseEvent&);
    void mouseLeave(const TMouseEvent&);
    void closeRequest();

  protected:
    bool _down;         // anchor
    void activate();    // activate button
    void deactivate();  // deactivate button & drop grab
    void trigger();     // trigger button & close the whole menu
    void collapse();
    void grabKeyboard();
    void dropKeyboard();
    
    void openPopup();
    void closePopup();

    virtual unsigned drawIcon(TPen *pen=NULL, int x=0, int y=0);
    
    TPopup *popup;    // popup child
    
    unsigned idx;
};

class TChoiceModel;

class TMenuRadioButton:
  public TMenuButton
{
  public:
    TMenuRadioButton(TMenuHelper *menubar, 
                     TMenuHelper::TNode *node, 
                     TChoiceModel *c, unsigned i):
      TMenuButton(menubar, node) 
    {
      choice = c;
      idx = i;
    }
  protected:
    TChoiceModel *choice;
    
    unsigned drawIcon(TPen *pen=NULL, int x=0, int y=0);
};

} // namespace toad

#endif
