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

#ifndef _TOAD_DIALOG_HH
#define _TOAD_DIALOG_HH

#include <toad/layout.hh>
#include <toad/figuremodel.hh>

namespace toad {

class TLayoutEditDialog;

class TDialogLayout:
  public TLayout
{
    typedef TLayout super;
  public:
    TDialogLayout();
    ~TDialogLayout();
    void arrange();
    void paint();
  
    unsigned height;
    unsigned width;
    bool drawfocus;
    PFigureModel gadgets;

    TLayoutEditor * createEditor(TWindow *inWindow, TWindow *forWindow);
    TLayoutEditDialog * editor;

    SERIALIZABLE_INTERFACE(toad::, TDialogLayout)
};

class TDialog: 
  public TWindow
{
    typedef TWindow super;
  public:
    TDialog(TWindow *parent, const string &title);
    ~TDialog();
    
    void doModalLoop();

    bool bDrawFocus;
    
    void adjust();
    void destroy();
    void paint();
    void childNotify(TWindow*, EChildNotify);

  protected:

    string _resource_name;
};

} // namespace toad

#endif
