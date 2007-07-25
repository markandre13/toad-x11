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

#include "modellayout.hh"
#include "modellayouteditor.hh"

using namespace toad;

void
TModelLayout::paint()
{
  if (!editor || !editor->isEnabled()) {
    TPen pen(window);
    TFigureModel::iterator p = gadgets->begin();
    while(p!=gadgets->end()) {
      (*p)->paint(pen, TFigure::NORMAL);
      ++p;
    }
  } else {
    editor->gedit.paint();
  }  
}  

void
TModelLayout::arrange()
{
}

void
TModelLayout::store(TOutObjectStream&) const
{
}

bool
TModelLayout::restore(TInObjectStream&)
{
}

void
TModelLayout::addModel(TModel *model, const string &name)
{
  modellist.push_back(TModelName(model, name));
}

