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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <assert.h>
#include <cstring>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/region.hh>

#include <iostream>

using namespace toad;

TPenBase::~TPenBase()
{
}

/** 
 * \class toad::TPen
 * TPen provides fast integer and raster oriented drawing operations.
 *
 * <IMG SRC="img/paint-raster-pixel.gif">
 * <P>
 * Methods with the suffix `PC' use pixel oriented coordiantes.
 *
 * \sa TScreenPencil
 *
 * \todo
 *   \li
 *     clean up the clipping methods
 *   \li
 *     have look at libpango for text rendering
 *   \li
 *     provide an interface to the Xft and Xr extensions
 */

// constructor & destructor
//---------------------------------------------------------------------------

/**
 * Create a pen that draws into a bitmap.
 *
 * \sa toad::TBitmap
 */
TPen::TPen(TBitmap *bmp)
{
  assert(bmp!=0 && bmp->pixmap!=0);
  this->bmp = bmp;
  wnd = 0;
  x11drawable = bmp->pixmap;
  _init();
//  XCopyGC(x11display, TOADBase::x11gc, GCFont, o_gc);
  setFont(&getDefaultFont());
} 

/**
 * Creates a pen that draws into a window.
 *
 * This contains some code for the double buffering also.
 * In case the window requests DB, the pen creates a bitmap which is
 * copied to the window from TPen's destructor.
 */
TPen::TPen(TWindow *wnd)
{
  assert(wnd!=0 && wnd->x11window!=0);
  this->wnd = wnd;
  
  if (wnd->bDoubleBuffer) {
    bmp = new TBitmap(wnd->getWidth(), wnd->getHeight(), TBITMAP_SERVER);
    x11drawable = bmp->pixmap;
  } else {
    bmp = 0;
    x11drawable = wnd->x11window;
  }
  _init();
  translate(wnd->_dx, wnd->_dy);

  if (wnd->bDoubleBuffer) {
    setColor(wnd->background);
    fillRectangle(0,0,wnd->getWidth(), wnd->getHeight());
    setColor(0,0,0);
  }

  if (!wnd->bX11GC)
    setFont(&getDefaultFont());

  // use the clip region when window has one
  if (wnd->paint_rgn) {
    if (!wnd->bDoubleBuffer) {
#if 0
      setClipRegion(static_cast<TRegion*>(wnd->paint_rgn));
#else
      // someone might call TWindow::getUpdateRegion to reuse the
      // original update region multiple times, so we always have to
      // use a copy, add copy-on-write to TRegion instead
      setClipRegion(new TRegion(*static_cast<TRegion*>(wnd->paint_rgn)));
      bDeleteRegion = true;
#endif      
    } else {
      setClipRegion(new TRegion(*static_cast<TRegion*>(wnd->paint_rgn)));
      bDeleteRegion = true;
    }
  }
}

void TPen::_init()
{
  // create X graphic context
  //--------------------------
  o_gc = XCreateGC(x11display, x11drawable,0,0);
  f_gc = 0;
  two_colors = false;
 
  // set attributes for the default font
  font = &getDefaultFont();
  
  cmode = TColor::NEAREST;
  // cmode = TColor::DITHER28;
  region = 0;
  mat = 0;
  
  // make black the default color (needed by X11R5 on Sun)
  o_color.set(0,0,1);   // cheat SetColor to do it!
  setColor(0,0,0);
  setBackColor(255,255,255);
  
  width = 1;
  style = TPen::SOLID;
  bDeleteRegion = false;
  using_bitmap = false;
}

TPen::~TPen()
{
  popAll();
  if (wnd && bmp) { // double buffer mode...
    x11drawable = wnd->x11window;
    if (wnd->paint_rgn)
      setClipRegion(static_cast<TRegion*>(wnd->paint_rgn));
    XFreeGC(x11display, o_gc);
    o_gc = XCreateGC(x11display, x11drawable,0,0);
    if (wnd->paint_rgn)
      setClipRegion(static_cast<TRegion*>(wnd->paint_rgn));
    bmp->drawBitmap(this, 0, 0);
    delete bmp;
  }
  XFreeGC(x11display, o_gc);
  if (f_gc)
    XFreeGC(x11display, f_gc);
  if (region && bDeleteRegion )
    delete region;
}

/**
 * Sets a new font for drawString.
 */
void
TPen::setFont(TFont *newfont)
{
  assert(newfont!=0);
  TFont *oldfont = font;
  font = newfont;
  XSetFont(x11display, o_gc, font->fs->fid);
}

/**
 * When <I>true</I>, TPen will not paint inside the child windows of
 * the current window (which is the standard setting) and when <I>false</I>,
 * TPen will use the entire area of the current window for painting
 * including its children.
 */
void TPen::setClipChildren(bool flag) {
  XSetSubwindowMode(x11display, o_gc,flag ? ClipByChildren : IncludeInferiors);
}

/**
 * You shouldn't call this yourself currently, it's used by
 * TWindow::_DispatchPaintEvent().
 */
void TPen::setClipRegion(TRegion *rgn)
{
  if (region!=rgn) {
    if (region) {
      if (bDeleteRegion)
        delete region;
      else
        printf("toad: internal warning; TPen::setClipRegion is a prototype\n");
    }
    bDeleteRegion = false;
    region = rgn;
  }
  if (region) {
    XSetRegion(x11display,o_gc,region->x11region);
    if (f_gc)
      XSetRegion(x11display,f_gc,region->x11region);
  }
}

/**
 * Sets a clipping rectangle.
 *
 * This method should be obsolete as it's incompatible with
 * clipping regions!
 */
void TPen::setClipRect(const TRectangle &r)
{
  XRectangle xr;
  xr.x = r.x;
  xr.y = r.y;
  xr.width = r.w;
  xr.height = r.h;
  ::XSetClipRectangles(x11display, o_gc, 0,0, &xr, 1, Unsorted);
}

/**
 * Modify the pen's clipping region.
 *
 * The new clipping region will be the <B>union</B> of the pens' current
 * region and the given rectangle.
 * <P>
 * <I>Note: when the pen has no clipping region yet, this method doesn't
 * do anything. I guess we will change the behaviour by the time.</I>
 */
void
TPen::operator&=(const TRectangle &rect)
{
  if (!region) return;
  *region&=rect;
  XSetRegion(x11display, o_gc, region->x11region);
}

void
TPen::operator&=(const TRegion &rect)
{
  if (!region) return;
  *region&=rect;
  XSetRegion(x11display, o_gc, region->x11region);
}

/**
 * Modify the pen's clipping region.
 *
 * The new clipping region will be the <B>intersection</B> of the pens'
 * current region and the given rectangle.
 * <P>
 * <I>Note: when the pen has no clipping region yet, this method doesn't
 * do anything. I guess we will change the behaviour by the time.</I>
 */
void
TPen::operator|=(const TRectangle &rect)
{
  if (!region) return;
  *region|=rect;
  XSetRegion(x11display, o_gc, region->x11region);
}

void
TPen::operator|=(const TRegion &rect)
{
  if (!region) return;
  *region|=rect;
  XSetRegion(x11display, o_gc, region->x11region);
}

/**
 * Remove the pen's clipping region.
 *
 * \todo
 *   Does this really work?
 */
void TPen::clrClipBox()
{
  if (!region)
    return;
  region->clear();
  XSetClipMask(x11display,o_gc,None);
}

/**
 * Returns the bounding rectangle of the pens current clipping region
 * or the size of the window the pen is related to, when the pen has no
 * clipping region.
 */
void TPen::getClipBox(TRectangle* r) const
{
  if (!region) {
      r->x = 0;
      r->y = 0;
      if (wnd) {
        r->w = wnd->getWidth();
        r->h = wnd->getHeight();
      } else if (bmp) {
        r->w = bmp->width;
        r->h = bmp->height;
      }
  } else {
    region->getBoundary(r);
  }
}

/**
 * Sets the mode for the drawing operations. Possible values are:
 * 
 * \li TPen::NORMAL
 * \li TPen::XOR
 * \li TPen::INVERT
 */
void TPen::setMode(EPenMode mode)
{
  XSetFunction(x11display,o_gc,mode);
}

/**
 * Sets the width of the line for all line drawing operations, eg.
 * drawLine, drawRectangle, etc.
 */
void TPen::setLineWidth(int n)
{
  if (n<0)
    width=0;
  else
    width=n;
  _setLineAttributes();
}

/**
 * Set a line style.
 *
 * \sa EPenLineStyle
 */
void TPen::setLineStyle(EPenLineStyle n)
{
  style=n;
  _setLineAttributes();
}

void TPen::_setLineAttributes()
{
  char dash[6];
  int w = width ? width : 1;
  for(int i=1;i<6;i++)
    dash[i]=w;
  switch(style) {
  case SOLID:
    break;
  case DASH:
    dash[0]=w<<1;
    XSetDashes(x11display,o_gc,0, dash,2);
    break;
  case DOT:
    dash[0]=w;
    XSetDashes(x11display,o_gc,0, dash,2);
    break;
  case DASHDOT:
    dash[0]=(w<<1)+w;
    XSetDashes(x11display,o_gc,0, dash,4);
    break;
  case DASHDOTDOT:
    dash[0]=(w<<1)+w;
    XSetDashes(x11display,o_gc,0, dash,6);
    break;
  }
  XSetLineAttributes(x11display,o_gc,width,
         style==SOLID ? LineSolid : LineOnOffDash,
         CapButt,JoinMiter);
}

#ifdef COLORREF
#undef COLORREF
#endif

/**
 * Select the color dithering mode.
 */
void TPen::setColorMode(TColor::EDitherMode cm)
{
  if (cmode==cm) return;
  cmode = cm;
  
  // update colors
  if (o_gc)
    o_color._setPen(this, o_gc);
  if (f_gc)
    f_color._setPen(this, f_gc);
}

// SetColor
//----------------------------------------------------------------------------

/**
 * Use the bitmap during drawing operations. NULL removes the bitmap.
 */
void 
TPen::setBitmap(TBitmap *bmp)
{
  if (bmp) {
    two_colors = false;
    bmp->update();
    if (bmp->pixmap) {
      XSetTile(x11display, o_gc, bmp->pixmap);
      XSetFillStyle(x11display, o_gc, FillTiled);
      using_bitmap = true;
    }
  } else {
    if (using_bitmap) {
      XSetFillStyle(x11display, o_gc, FillSolid);
      using_bitmap = false;
    }
  }
}

// here is plenty room for optimations:

/**
 * Set the line and fill color to <VAR>color</VAR>.
 */
void 
TPen::setColor(const TColor &color)
{
  if (using_bitmap) {
    XSetFillStyle(x11display, o_gc, FillSolid);
    using_bitmap = false;
  }

  two_colors = false;
  if (color==o_color) {
    return;
  }
  o_color=color;
  o_color._setPen(this, o_gc);
}

/**
 * Set the line color to <VAR>color</VAR>.
 */
void 
TPen::setLineColor(const TColor &color)
{
  if (using_bitmap) {
    XSetFillStyle(x11display, o_gc, FillSolid);
    using_bitmap = false;
  }

  if (two_colors) {
    if (color==o_color)
      return;
  } else {
    if (color==o_color)
      return;
    f_color = o_color;
    f_color._setPen(this, f_gc);
    two_colors=true;
  }
  o_color = color;
  o_color._setPen(this, o_gc);
}

/**
 * Set the fill color to <VAR>color</VAR>.
 */
void 
TPen::setFillColor(const TColor &color)
{
  if (using_bitmap) {
    XSetFillStyle(x11display, o_gc, FillSolid);
    using_bitmap = false;
  }

  if (two_colors) {
    if (color==f_color)
      return;
    if (color==o_color) {
      two_colors = false;
      return;
    }
  } else {
    if (color==o_color)
      return;
    two_colors = true;
    if (color==f_color)
      return;
  }
  f_color = color;
  f_color._setPen(this, f_gc);
}

/**
 * Sets the background color for `fillString(..)'.
 *
 * \sa fillString
 */
void
TPen::setBackColor(const TColor& color)
{
  if (using_bitmap) {
    XSetFillStyle(x11display, o_gc, FillSolid);
    using_bitmap = false;
  }

  XSetBackground(x11display, o_gc, TColor::_getPixel(color));
}

