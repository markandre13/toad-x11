/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#ifndef _TOAD_FORMLAYOUT_HH
#define _TOAD_FORMLAYOUT_HH

#include <toad/layout.hh>

namespace toad {

class TFormLayout:
  public TLayout
{
  public:
    // attachment side
    static const unsigned TOP=1, BOTTOM=2, LEFT=4, RIGHT=8, ALL=15;
    
    // attachment method
    enum EMethod { NONE=0, FORM, WINDOW, OPPOSITE_WINDOW };
  
    TFormLayout();
    ~TFormLayout();

    void attach(const string &window, unsigned where, EMethod how, const string &which);
    void attach(const string &window, unsigned where, const string &which="") {
      attach(wnd, where, which.empty() ? WINDOW : FORM, which);
    }
    void distance(const string &window, int distance, unsigned where=ALL);

  private:
    void arrange();
    int nBorderOverlap;
    bool bKeepOwnBorder;
    bool running;
    
    class TFormNode;
    TFormNode* _find(const string &window);
    TFormNode *flist, *lastadd;
  
    TCloneable* clone() const { return new TFormLayout(*this); }
    const char * name() const { return "toad::TFormLayout"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
};

} // namespace toad

#endif
