/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#include <toad/dnd/image.hh>
#include <sstream>

using namespace toad;

// image/*
//---------------------------------------------------------------------------
#if 0
TDnDImage::TDnDImage(TBitmap *bmp)
{
  data = bmp;
#ifdef HAVE_PNG
  setType("image/png", ACTION_COPY);
#endif
  setType("image/bmp", ACTION_COPY);
#ifdef HAVE_JPEGLIB
  setType("image/jpeg", ACTION_COPY);
#endif
}

void TDnDImage::flatten()
{
}

#endif

bool TDnDImage::select(TDnDObject &drop)
{
  if (dynamic_cast<TDnDImage*>(&drop))
    return true;

#ifdef HAVE_PNG
  if (TDnDObject::select(drop, "image", "png"))
    return true;
#endif
  if (TDnDObject::select(drop, "image", "gif"))
    return true;
  if (TDnDObject::select(drop, "image", "bmp"))
    return true;
#ifdef HAVE_JPEGLIB
  if (TDnDObject::select(drop, "image", "jpeg"))
    return true;
#endif
  return false;
}


PDnDImage TDnDImage::convertData(TDnDObject &drop)
{
  TDnDImage *result;

  result = dynamic_cast<TDnDImage*>(&drop);
  if (result ||
      !drop.type ||
      drop.type->major!="image")
    return result;

  TBitmap *bmp = new TBitmap();
#if 1
  string s(drop.flatdata.c_str(), drop.flatdata.size());
  istringstream in(s);
#else  
  istringstream in(drop.flatdata.c_str(), drop.flatdata.size());
#endif
  try {
    bmp->load(in);
  } catch(...) {
    delete bmp;
    return result;
  }
  result = new TDnDImage();
  result->bmp = bmp;
  result->x = drop.x;
  result->y = drop.y;
  return result;
}
