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
#include <toad/dialog.hh>
#include <toad/textfield.hh>
#include <toad/pushbutton.hh>

#include "BitmapEditor.hh"
#include "ZoomDlg.hh"

TZoomDlg::TZoomDlg(TWindow *p,const char *t,TBitmapEditor *bme)
  :TDialog(p,t)
{
  this->bme = bme;

  zoom.setMinimum(1);
  zoom = bme->getEditZoom();
  
  TPushButton *pb;
  pb = new TPushButton(this, "ok");
    CONNECT(pb->sigClicked, this, actOk);
    
  TTextField *tf;
  tf = new TTextField(this, "zoom", &zoom);
  
  loadLayout("dialog#TZoomDlg");
}

void TZoomDlg::actOk()
{
#if 0
  apply();
  if (zoom<=0) {
    char buffer[80];
    snprintf(buffer, 80, "wrong zoom %i",zoom);
    messageBox(this, getTitle(), buffer, 
      TMessageBox::OK | TMessageBox::ICON_EXCLAMATION);
  } else {
#endif
    bme->setEditZoom(zoom);
    destroyWindow();
#if 0
  }
#endif
}
