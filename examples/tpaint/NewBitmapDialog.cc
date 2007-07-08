/*
 * TPaint -- a simple bitmap editor
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <toad/toad.hh>
#include <toad/mdiwindow.hh>
#include <toad/dialog.hh>
#include <toad/textfield.hh>
#include <toad/pushbutton.hh>

#include "BitmapEditor.hh"
#include "NewBitmapDialog.hh"

TNewBitmapDialog::TNewBitmapDialog(TWindow *p,const string &t,TMDIWindow *m)
:TDialog(p,t)
{
  mdi=m;

  w = h = 32;
  zoom = 1;
  
  w.setMinimum(0);
  h.setMinimum(0);
  zoom.setMinimum(1);

  TPushButton *pb;
  pb = new TPushButton(this, "ok");
    CONNECT(pb->sigClicked, this, actOk);
  pb = new TPushButton(this, "close");
    CONNECT(pb->sigClicked, this, closeRequest);

  TTextField *tf;   
  tf = new TTextField(this, "width", &w);
  tf = new TTextField(this, "height", &h);
  tf = new TTextField(this, "zoom", &zoom);
  
  loadLayout("dialog#TNewBitmapDialog");
}

void TNewBitmapDialog::actOk()
{
#if 0
  if (h<=0 || w<=0) {
    char buffer[80];
    sprintf(buffer, "%i,%i is a wrong size.",w,h);
    messageBox(this, getTitle(), buffer, 
      TMessageBox::OK | TMessageBox::ICON_EXCLAMATION);
    return;
  }
  
  if (zoom<=0) {
    messageBox(this, getTitle(), "'zoom' must be a value >0", 
      TMessageBox::OK | TMessageBox::ICON_EXCLAMATION);
    return;
  }
#endif
  
  TBitmapEditor *be;
  be = new TBitmapEditor(mdi, "(unnamed2)");
  be->setEditSize(w,h);
  be->setEditZoom(zoom);
  be->createWindow();
  destroyWindow();
}
