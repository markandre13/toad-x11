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

#include <toad/arrowbutton.hh>

using namespace toad;

void
TArrowButton::paint()
{
  TPen pen(this);

  drawShadow(pen, bDown && bInside);

  int n=bDown && bInside ? 1:0;

  const int d=4;
  TPoint p[3];
  
  switch(direction)
  {
    case ARROW_DOWN:
      p[0].set(n+(_w>>1), n+_h-d-2);
      p[1].set(n+_w-d   , n+d-1+2);
      p[2].set(n+d-1        , n+d-1+2);
      break;
    case ARROW_UP:
      p[2].set(n+_w-d   , n+_h-d-2);
      p[1].set(n+d-1        , n+_h-d-2);
      p[0].set(n+(_w>>1), n+d-1+2);
      break;
    case ARROW_LEFT:
      p[0].set(n+d-1+2      , n+(_h)>>1);
      p[1].set(n+_w-d-2 , n+d-1);
      p[2].set(n+_w-d-2 , n+_h-d);
      break;
    case ARROW_RIGHT:
      p[0].set(n+d-1+2      , n+d-1);
      p[1].set(n+_w-d-2 , n+(_h)>>1);
      p[2].set(n+d-1+2      , n+_h-d);
      break;
  }
  if (isEnabled()) {
    pen.setColor(TColor::BTNTEXT);
  } else {
    pen.setColor(TColor::BTNSHADOW);
  }
  pen.fillPolygon(p,3);
}

void
TArrowButton::mouseLDown(int,int,unsigned)
{
  if (!isEnabled())
    return;
  bDown=true;
  invalidateWindow();
  sigArm();
  sigClicked();
  delay = 0;
  startTimer(0, 1000000/48);
}

void
TArrowButton::mouseLUp(int,int,unsigned)
{
  if (!bDown)
    return;
  bDown=false;
  invalidateWindow();
  sigDisarm();
  stopTimer();
}

void
TArrowButton::tick()
{
  delay++;
  if(delay<12)
    return;
  if (bInside)
    sigClicked();
}
