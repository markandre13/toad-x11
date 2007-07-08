/* TPaint -- a simple bitmap editor
 * Copyright (C) 1996-2002 by Mark-André Hopf <mhopf@mark13.de>
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

using namespace toad;

extern TColor color[1];

#include "BitmapEditor.hh"
#include "Tool.hh"

/***************************************************************************
                                  TPlotTool
 ***************************************************************************/
void TPlotTool::mouseLDown(TBitmapEditor *be,int x,int y,unsigned)
{
  TPen pen(be);
  be->setPixel(x,y,color[0],pen);
}

void TPlotTool::mouseMove(TBitmapEditor *be,int x,int y,unsigned)
{
  TPen pen(be);
  be->setPixel(x,y,color[0],pen);
}

/***************************************************************************
                                  TDrawTool
 ***************************************************************************/
void TDrawTool::mouseLDown(TBitmapEditor*,int x,int y,unsigned)
{
  dx=x; dy=y; state=0;
}

void TDrawTool::mouseMove(TBitmapEditor *be,int x,int y,unsigned)
{
  if (state==1) {
    invertLine(be, dx,dy, ox,oy);
    state = 0;
  }
  ox=x; oy=y;
  if (be->isInside(dx,dy) && be->isInside(ox,oy)) {
    invertLine(be, dx,dy, ox,oy);
    state = 1;
  }
}

void TDrawTool::mouseLUp(TBitmapEditor* be,int,int,unsigned)
{
  if (state==1)
    drawLine(be, dx,dy, ox,oy);
}

void TDrawTool::invertLine(TBitmapEditor *be,int x1,int y1,int x2,int y2)
{
  TPen pen(be);

  int dx,dy, xstep,ystep, e;

  dx=x2-x1; dy=y2-y1;
  if(dx>0)
    xstep=1;
  else {
    xstep=-1;
    dx=-dx;
  }
  if(dy>0)
    ystep=1;
  else {
    ystep=-1;
    dy=-dy;
  }

  if (dy>dx) {
    e=-dy;
    dx<<=1;
    dy<<=1;
    while(y1!=y2) {
      be->invertPixel(x1,y1,pen);
      e+=dx;
      y1+=ystep;
      if (e>0) {
        x1+=xstep;
        e-=dy;
      }
    }
  } else {
    e=-dx;
    dy<<=1;
    dx<<=1;
    while(x1!=x2) {
      be->invertPixel(x1,y1,pen);
      x1+=xstep;
      e+=dy;
      if (e>0) {
        y1+=ystep;
        e-=dx;
      }
    }
  }
  be->invertPixel(x1,y1,pen);
}

void TDrawTool::drawLine(TBitmapEditor *be,int x1,int y1,int x2,int y2)
{
  TPen pen(be);
  int dx,dy, xstep,ystep, e;

  dx=x2-x1; dy=y2-y1;
  if(dx>0)
    xstep=1;
  else {
    xstep=-1;
    dx=-dx;
  }
  if(dy>0)
    ystep=1;
  else {
    ystep=-1;
    dy=-dy;
  }

  if (dy>dx) {
    e=-dy;
    dx<<=1;
    dy<<=1;
    while(y1!=y2) {
      be->setPixel(x1,y1,color[0],pen);
      e+=dx;
      y1+=ystep;
      if (e>0) {
        x1+=xstep;
        e-=dy;
      }
    }
  } else {
    e=-dx;
    dy<<=1;
    dx<<=1;
    while(x1!=x2) {
      be->setPixel(x1,y1,color[0],pen);
      x1+=xstep;
      e+=dy;
      if (e>0) {
        y1+=ystep;
        e-=dx;
      }
    }
  }
  be->setPixel(x1,y1,color[0],pen);
}

/***************************************************************************
                                  TFillTool
 ***************************************************************************/
void TFillTool::Push(int x,int y)
{
  coord *n = new coord;
  n->x=x;
  n->y=y;
  n->next=stack;
  stack=n;  
}

bool TFillTool::Pop(int *x,int *y)
{
  if (!stack)
    return false;
  coord *o = stack;
  stack = stack->next;
  *x = o->x;
  *y = o->y;
  delete o;
  return true;
}

void TFillTool::mouseLDown(TBitmapEditor* be,int x,int y,unsigned)
{
//printf("Fill...\n");
  TPen pen(be);

  if (!be->isInside(x,y))
    return;
    
  int oben,unten,x1,y1,ystep,xstep;

  TColor h,g;
  be->getPixel(x,y,&h);
  if (h==color[0]) return;  // gibt nichts zu füllen
  stack = NULL;
  Push(x,y);                // Koordinate aus Stack
  while(Pop(&x,&y)) {       // Solange Stack nicht leer
    x1=x; y1=y;
    ystep=-1;
    do {
      while(true) {
        if (!be->getPixel(x1,y1,&g) || g!=h) break;
        oben=0; unten=0;
        xstep=-1;
        
        do { // links & rechts
          while(true) { // oben & unten prüfen
            if (!be->getPixel(x1,y1,&g) || g!=h) break;
            if (oben) {
              if (be->getPixel(x1,y1-1,&g) && g==h)
              { Push(x1,y1-1); oben=0; }
            }
            else
              if (be->getPixel(x1,y1-1,&g) && g!=h) oben=-1;
            if (unten) {
              be->getPixel(x1,y1+1,&g);
              if (g==h) { 
                Push(x1,y1+1); unten=0; 
              }
            }
            else
              if (be->getPixel(x1,y1+1,&g) && g!=h) unten=-1;
            be->setPixel(x1,y1,::color[0],pen);
            x1+=xstep;
          }
          x1=x;
          oben=0; unten=0;
          if (be->getPixel(x1,y1-1,&g) && g!=h) oben=-1;
          if (be->getPixel(x1,y1+1,&g) && g!=h) unten=-1;
          x1++;
          xstep=-xstep;
        }while(xstep==1);
        x1=x; y1+=ystep;
      }
      x1=x; y1=y+1;
      ystep=-ystep;
    }while(ystep==1);
  }
}
