/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <toad/tabbedlayout.hh>

using namespace toad;

void
TTabbedLayout::arrange()
{
  TWindow *w;
  TInteractor *p;
  unsigned n;

  window->setBackground(TColor::DIALOG);

  n = 0;
  p = window->getFirstChild();
  while(p) {
    w = dynamic_cast<TWindow*>(p);
    if (w) {
      ++n;
    }
    p = p->getNextSibling();
  }
  
  if (!tab || ntabs!=n) {
    delete[] tab;
    tab = 0;
  }
  if (n==0)
    return;
  if (!tab) {
    tab = new TTab[n];
    ntabs = n;
  }

  TFont &fnt(TOADBase::getDefaultFont());
  
  int h = fnt.getHeight();
  static const int left = 5;

  int x, y;
  x = left;
  y = 5;
  n = 0;
  p = window->getFirstChild();
  while(p) {
    w = dynamic_cast<TWindow*>(p);
    if (w) {
      string title = p->getTitle();
      int width = fnt.getTextWidth(title);
      if (x!=left && x+width+8>=window->getWidth()-5) {
        x = left;
        y += h + 5;
      }
      tab[n].window = w;
      tab[n].set(x,y,width+8,h+2);
      if (!current && n!=0)
        w->setMapped(false);
      ++n;

      x+=width + 8;
      if (x>=window->getWidth()) {
        x = left;
        y += h + 5;
      }
    }
    p = p->getNextSibling();
  }
  if (!current) {
    current = tab[0].window;
    current->setMapped(true);
  }
  current->setShape(
    left, tab[ntabs-1].y + tab[ntabs-1].h + 3,
    window->getWidth()-2*left, window->getHeight() - tab[ntabs-1].y - tab[ntabs-1].h - 5 - 3);
  window->invalidateWindow(true);
}

static TRGB pastel[18] = {
  TRGB(226, 145, 145), 
  TRGB(153, 221, 146),
  TRGB(147, 216, 185),
  TRGB(148, 196, 211),
  TRGB(148, 154, 206),
  TRGB(179,148,204),
  TRGB(204,150,177),
  TRGB(204,164,153),
  TRGB(223,229,146),
  TRGB(255,165,96),
  TRGB(107,255,99),
  TRGB(101,255,204),
  TRGB(101,196,255),
  TRGB(101,107,255),
  TRGB(173,101,255),
  TRGB(255,101,244),
  TRGB(255,101,132),
  TRGB(255,101,101),
};

void
TTabbedLayout::paintTab(TPen &pen, TTab &tab, bool filled)
{
  TPoint pt[10];
  pt[0].set(tab.x          , tab.y+tab.h);
  pt[1].set(tab.x+3        , tab.y+1);
  pt[2].set(tab.x+4        , tab.y);
  pt[3].set(tab.x+4+tab.w-4, tab.y);
  pt[4].set(tab.x+4+tab.w-3, tab.y+1);
  pt[5].set(tab.x+4+tab.w  , tab.y+tab.h);
  
  pt[6].set(window->getWidth()-5   , tab.y+tab.h);
  pt[7].set(window->getWidth()-5   , window->getHeight()-5);
  pt[8].set(5                      , window->getHeight()-5);
  pt[9].set(5                      , tab.y+tab.h);
  
  if (filled)
    pen.fillPolygon(pt, 10);
  else
    pen.drawPolygon(pt, 10);
}

void
TTabbedLayout::paint()
{
  if (!tab)
    return;
    
  TPen pen(window);

  int n = 0;
  int idxcurrent = -1;
  int colcurrent;
  bool row_below_current = false;
  for(int i=0; i<ntabs; ++i) {
    if (tab[i].window != current) {
      if (row_below_current) {
        pen.setLineColor(
          (pastel[colcurrent].r*4+pastel[n].r)/6,
          (pastel[colcurrent].g*4+pastel[n].g)/6,
          (pastel[colcurrent].b*4+pastel[n].b)/6
        );
        pen.setFillColor(
          (pastel[colcurrent].r*4+pastel[n].r)/5,
          (pastel[colcurrent].g*4+pastel[n].g)/5,
          (pastel[colcurrent].b*4+pastel[n].b)/5
        );
      } else {
        pen.setFillColor(pastel[n]);
        paintTab(pen, tab[i], true);
      }
      paintTab(pen, tab[i], true);
      pen.drawString(tab[i].x+6, tab[i].y+1, tab[i].window->getTitle());
    } else {
      idxcurrent = i;
      colcurrent = n;
    }
    if (i+1>=ntabs || tab[i+1].x <= tab[i].x) {
      if (idxcurrent>=0 && !row_below_current) {
        pen.setFillColor(pastel[colcurrent]);
        paintTab(pen, tab[idxcurrent], true);
        pen.drawString(tab[idxcurrent].x+6, tab[idxcurrent].y+1, tab[idxcurrent].window->getTitle());
        row_below_current = true;        
      }
    }
    ++n;
    if (n>=18) n=0;
  }
  if (idxcurrent>=0) {
    pen.setLineColor(0,0,0);
     paintTab(pen, tab[idxcurrent], false);
  }
}

bool
TTabbedLayout::mouseEvent(TMouseEvent &me)
{
  if (tab && me.type == TMouseEvent::LDOWN) {
    for(int i=0; i<ntabs; ++i) {
      if (tab[i].isInside(me.x, me.y)) {
        if (tab[i].window!=current) {
          current->setMapped(false);
          current=tab[i].window;
          current->setShape(
            5, tab[ntabs-1].y + tab[ntabs-1].h + 3,
            window->getWidth()-10, window->getHeight() - tab[ntabs-1].y - tab[ntabs-1].h - 5 - 3);
          current->setMapped(true);
          window->invalidateWindow(false);
        }
        break;
      }
    }
  }
  return false;
}

void TTabbedLayout::store(TOutObjectStream&) const {}
bool TTabbedLayout::restore(TInObjectStream&) {}
