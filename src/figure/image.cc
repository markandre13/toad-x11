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

#include <toad/figure.hh>
#include <toad/figureeditor.hh>
#include <toad/filedialog.hh>

using namespace toad;

void 
TFImage::paint(TPenBase &pen, EPaintType)
{
  if (bitmap) {
    pen.drawBitmap(p1.x, p1.y, bitmap);
  } else {
    pen.setLineColor(line_color);
    if (!filled) {
      pen.drawRectangle(p1,p2);
    } else {
      pen.setFillColor(fill_color);
      pen.fillRectangle(p1,p2);
    }
  }
}

bool
TFImage::startInPlace()
{
  TFileDialog dlg(NULL, "Open Image");
  dlg.doModalLoop();
  filename = dlg.getFilename();
  bitmap = new TBitmap();
  bitmap->load(filename);
  
  return false;
}

void
TFImage::store(TOutObjectStream &out) const
{
  super::store(out);
  ::store(out, "filename", filename);
}

bool
TFImage::restore(TInObjectStream &in)
{
  filled = true;
  if (in.what == ATV_FINISHED) {
    bitmap = new TBitmap();
    if (!bitmap->load(filename)) {
      cout << "failed to load bitmap" << endl;
    }
  }
  if (
    ::restore(in, "filename", &filename) ||
    super::restore(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}
