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

#include <toad/types.hh>

using namespace toad;

void TRectangle::set(int x, int y, int w, int h)
{
  if (w<0) {
    w=-w;
    x-=w-1;
  }
  if (h<0) {
    h=-h;
    y-=h-1;
  }
  this->x = x;
  this->y = y;
  this->w = w;
  this->h = h;
}

void TRectangle::set(const TPoint &p1, const TPoint &p2)
{
  x = p1.x;
  y = p1.y;
  w = p2.x-p1.x;
  h = p2.y-p1.y;
  if (w<0) {
    w=-w;
    x-=w-1;
  }
  if (h<0) {
    h=-h;
    y-=h-1;
  }
  w++;
  h++;    
}
