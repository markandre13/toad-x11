/* TPaint -- a simple bitmap editor
 * Copyright (C) 1996,97 by Mark-André Hopf
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

#ifndef TZoomDlg
#define TZoomDlg TZoomDlg

class TZoomDlg: public TDialog
{
    TBitmapEditor *bme; 
  public:
    TZoomDlg(TWindow *p,const char *t,TBitmapEditor *bme);
  protected:
    TIntegerModel zoom;
    void actOk();
};

#endif
