/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef TTextField
#define TTextField TTextField

#include <toad/textarea.hh>

namespace toad {

class TTextField: 
  public TTextArea
{
    typedef TTextArea super;
  public:
    TTextField(TWindow *p, const string &t):
      TTextArea(p, t)
    {
      preferences->singleline = true;
      bTabKey = false;
      TFont *font = TPen::lookupFont(preferences->getFont());
      setSize(320, font->getHeight()+4);
    }

    TTextField(TWindow *p, const string &t, TTextModel *m):
      TTextArea(p, t)
    {
      setModel(m);
      preferences->singleline = true;
      bTabKey = false;
      // PFont font = new TFont(preferences->getFont());
      TFont *font = TPen::lookupFont(preferences->getFont());
      setSize(320, font->getHeight()+4);
    }

    template <class T>
    TTextField(TWindow *p, const string &t, T *m):
      TTextArea(p, t)
    {
      setModel(m);
      preferences->singleline = true;
      bTabKey = false;
      PFont font = new TFont(preferences->getFont());
      setSize(320, font->getHeight()+2);
    }
  protected:
    void mouseEvent(TMouseEvent&);
};

} // namespace toad

#endif
