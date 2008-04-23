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

#include <toad/os.hh>
#include <toad/config.h>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include <assert.h>
#include <cmath>
#include <cstring>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/bitmap.hh>
#include <toad/region.hh>

#include <iostream>
#include <map>

#ifdef __X11__
#ifdef HAVE_LIBXFT

#ifdef _XFT_NO_COMPAT_
#undef _XFT_NO_COMPAT_
#endif

#include <X11/Xft/Xft.h>

#ifdef FC_VERSION
#define HAVE_FONTCONFIG
#endif

#endif
#endif

using namespace toad;

TPenBase::TPenBase()
{
  keepcolor = false;
  outline   = false;
  font = default_font;
}

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
#ifdef __X11__
  assert(bmp!=0 && bmp->pixmap!=0);
  this->bmp = bmp;
  wnd = 0;
  x11drawable = bmp->pixmap;
  _init();
//  XCopyGC(x11display, TOADBase::x11gc, GCFont, o_gc);
  setFont(&getDefaultFont());
#endif

#ifdef __WIN32__
  
#endif
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
#ifdef __X11__
  assert(wnd!=0);
  assert(wnd->x11window!=0);
  
  if (wnd->bDoubleBuffer) {
    bmp = new TBitmap(wnd->getWidth(), wnd->getHeight(), TBITMAP_SERVER);
    x11drawable = bmp->pixmap;
  } else {
    bmp = 0;
    x11drawable = wnd->x11window;
  }
#endif

#ifdef __COCOA__
  [[NSGraphicsContext currentContext] saveGraphicsState];
  [[NSGraphicsContext currentContext] setShouldAntialias: NO];
  clipPath = [NSBezierPath bezierPath];
  [clipPath appendBezierPathWithRect: NSMakeRect(0, 0, wnd->w, wnd->h)];
  [clipPath setClip];
  mstack.push_back([NSAffineTransform transform]);
#endif

  this->wnd = wnd;
  _init();
  translate(wnd->_dx, wnd->_dy);

#ifdef __X11__
  if (wnd->bDoubleBuffer) {
    setColor(wnd->_bg);
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
#endif
}

void
TPen::_init()
{
  mat = 0;
  width = 1;
  style = TPen::SOLID;

#ifdef __X11__
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
  using_bitmap = false;
  
  // make black the default color (needed by X11R5 on Sun)
  o_color.set(0,0,1);   // cheat SetColor to do it!
  setColor(0,0,0);
  
  bDeleteRegion = false;
#ifdef HAVE_LIBXFT
  xftdraw = 0;
#endif

#endif

#ifdef __WIN32__
  w32logpen.lopnStyle = PS_SOLID;
  w32logpen.lopnWidth.x = 1;
  w32logpen.lopnColor = RGB(0,0,0);
  w32pen = NULL;
  w32brush = NULL;

  w32window = wnd->w32window;
  if (wnd->paintstruct) {
    wnd->paintstruct->refcount++;
    w32hdc = wnd->paintstruct->hdc; 
  } else {
    wnd->paintstruct = new TWindow::TPaintStruct();
    wnd->paintstruct->refcount = 1;
    wnd->paintstruct->hdc = w32hdc = ::GetDC(w32window);
  }
#endif
}

TPen::~TPen()
{
  popAll();
#ifdef __X11__
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
#ifdef HAVE_LIBXFT
  if (xftdraw) {
    XftDrawDestroy(xftdraw);
    xftdraw = 0;
  }
#endif
#endif

#ifdef __COCOA__
  [NSBezierPath setDefaultLineWidth: 1];
  [[NSGraphicsContext currentContext] restoreGraphicsState];
  // [clipPath release];
#endif

#ifdef __WIN32__
  assert(wnd->paintstruct);
  if (wnd->paintstruct->currentpen == this) {
    if (wnd->paintstruct->origpen) {
      HGDIOBJ prevpen = ::SelectObject(w32hdc, wnd->paintstruct->origpen);
      wnd->paintstruct->origpen = 0;
      ::DeleteObject(prevpen);
    }
    if (wnd->paintstruct->origbrush) {
      HGDIOBJ prevbrush = ::SelectObject(w32hdc, wnd->paintstruct->origbrush);
      wnd->paintstruct->origbrush = 0;
      ::DeleteObject(prevbrush);
    }
    wnd->paintstruct->currentpen = 0;
  }
  wnd->paintstruct->refcount--;
  if (wnd->paintstruct->refcount==0) {
    ::ReleaseDC(w32window, w32hdc);
    delete(wnd->paintstruct);
    wnd->paintstruct=0;
  }
#endif
}

/**
 * Sets a new font for drawString.
 */
void
TPen::setFont(TFont *newfont)
{
#ifdef __X11__
  assert(newfont!=0);
  font = newfont;
//  XSetFont(x11display, o_gc, font->fs->fid);
#endif
}

namespace {

typedef map<string,PFont> TFontMap;
TFontMap fontmap;

} // end namespace

void
TPen::initialize()
{
}

void
TPen::terminate()
{
  fontmap.clear();
}

TFont *
TPen::lookupFont(const string &fontname)
{
  TFontMap::iterator p = fontmap.find(fontname);
  if (p!=fontmap.end()) {
    return p->second;
  }
  TFont *newfont = new TFont(fontname);
  fontmap[fontname] = newfont;
#warning "fontmap isn't limited in its size"
  return newfont;
}

void
TPen::setFont(const string &fontname)
{
  setFont(lookupFont(fontname));
}


/**
 * When <I>true</I>, TPen will not paint inside the child windows of
 * the current window (which is the standard setting) and when <I>false</I>,
 * TPen will use the entire area of the current window for painting
 * including its children.
 */
void
TPen::setClipChildren(bool flag) {
#ifdef __X11__
  XSetSubwindowMode(x11display, o_gc,flag ? ClipByChildren : IncludeInferiors);
#endif
}

/**
 * You shouldn't call this yourself currently, it's used by
 * TWindow::_DispatchPaintEvent().
 */
void
TPen::setClipRegion(TRegion *rgn)
{
#ifdef __X11__
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
#endif
}

/**
 * Sets a clipping rectangle.
 *
 * This method should be obsolete as it's incompatible with
 * clipping regions!
 */
void
TPen::setClipRect(const TRectangle &r)
{
#ifdef __X11__
  XRectangle xr;
  xr.x = r.x;
  xr.y = r.y;
  xr.width = r.w;
  xr.height = r.h;
  ::XSetClipRectangles(x11display, o_gc, 0,0, &xr, 1, Unsorted);
#endif
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
#ifdef __X11__
  if (!region) return;
  *region&=rect;
  XSetRegion(x11display, o_gc, region->x11region);
  if (f_gc)
    XSetRegion(x11display, f_gc, region->x11region);
#endif
}

void
TPen::operator&=(const TRegion &rect)
{
#ifdef __X11__
  if (!region) return;
  *region&=rect;
  XSetRegion(x11display, o_gc, region->x11region);
  if (f_gc)
    XSetRegion(x11display, f_gc, region->x11region);
#endif
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
#ifdef __X11__
  if (!region) return;
  *region|=rect;
  XSetRegion(x11display, o_gc, region->x11region);
  if (f_gc)
    XSetRegion(x11display, f_gc, region->x11region);
#endif
}

void
TPen::operator|=(const TRegion &rect)
{
#ifdef __X11__
  if (!region) return;
  *region|=rect;
  XSetRegion(x11display, o_gc, region->x11region);
  if (f_gc)
    XSetRegion(x11display, f_gc, region->x11region);
#endif
}

/**
 * Remove the pen's clipping region.
 *
 * \todo
 *   Does this really work?
 */
void TPen::clrClipBox()
{
#ifdef __X11__
  if (!region)
    return;
  region->clear();
  XSetClipMask(x11display,o_gc,None);
  if (f_gc)
    XSetClipMask(x11display,f_gc,None);
#endif
}

/**
 * Returns the bounding rectangle of the pens current clipping region
 * or the size of the window the pen is related to, when the pen has no
 * clipping region.
 */
void TPen::getClipBox(TRectangle* r) const
{
#ifdef __X11__
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
#endif
}

/**
 * Sets the mode for the drawing operations. Possible values are:
 * 
 * \li TPen::NORMAL
 * \li TPen::XOR
 * \li TPen::INVERT
 */
void TPen::setMode(EMode mode)
{
#ifdef __X11__
  XSetFunction(x11display,o_gc,mode);
#endif
}

/**
 * Sets the width of the line for all line drawing operations, eg.
 * drawLine, drawRectangle, etc.
 */
void TPen::setLineWidth(TCoord n)
{
  if (n<0)
    width=0;
  else
    width=n;

#ifdef __X11__
  _setLineAttributes();
#endif

#ifdef __COCOA__
  [NSBezierPath setDefaultLineWidth: width];
#endif

#ifdef __WIN32__
  w32logpen.lopnWidth.x = width;
  updateW32Pen();
#endif
}

/**
 * Set a line style.
 *
 * \sa ELineStyle
 */
void TPen::setLineStyle(ELineStyle n)
{
  style=n;
#ifdef __X11__
  _setLineAttributes();
#endif

#ifdef __WIN32__
  switch(style) {
    case SOLID:
      w32logpen.lopnStyle = PS_SOLID;
      break;
    case DASH:
      w32logpen.lopnStyle = PS_DASH;
      break;
    case DOT:
      w32logpen.lopnStyle = PS_DOT;
      break;
    case DASHDOT:
      w32logpen.lopnStyle = PS_DASHDOT;
      break;
    case DASHDOTDOT:
      w32logpen.lopnStyle = PS_DASHDOTDOT;
      break;
  }
  updateW32Pen();
#endif
}

#ifdef __X11__
void TPen::_setLineAttributes()
{
  char dash[6];
  int w = width;
  
  if (mat) {
    double x0, y0, x1, y1;
    mat->map(0.0, 0.0, &x0, &y0);
    mat->map(1.0, 1.0, &x1, &y1);
    double d = (double)w * (x1-x0);
    if (d<0.0)
      d = -d;
    w = round(d);
    // cout << "  set line width " << w << " for " << width << endl;
  }
  int pw = w;
  if (w==0)
    w=1;
  if (pw==1)
    pw=0;
  
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
  XSetLineAttributes(x11display,o_gc,pw,
         style==SOLID ? LineSolid : LineOnOffDash,
         CapButt,JoinMiter);
}
#endif

#ifdef COLORREF
#undef COLORREF
#endif

/**
 * Select the color dithering mode.
 */
void TPen::setColorMode(TColor::EDitherMode cm)
{
#ifdef __X11__
  if (cmode==cm) return;
  cmode = cm;
  
  // update colors
  if (o_gc)
    o_color._setPen(this, o_gc);
  if (f_gc)
    f_color._setPen(this, f_gc);
#endif
}

// SetColor
//----------------------------------------------------------------------------

/**
 * Use the bitmap during drawing operations. NULL removes the bitmap.
 */
void 
TPen::setBitmap(TBitmap *bmp)
{
#ifdef __X11__
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
#endif
}

#ifdef __WIN32__
void
TPen::activateW32() const
{
  wnd->paintstruct->currentpen = this;
  updateW32Pen();
  updateW32Brush();
}

void
TPen::updateW32Pen() const
{
  if (wnd->paintstruct->currentpen!=this)
    return;
  HPEN newpen  = ::CreatePenIndirect(&w32logpen);
  HGDIOBJ prevpen = ::SelectObject(w32hdc, newpen);
  if (wnd->paintstruct->origpen) {
    ::DeleteObject(prevpen);
  } else {
    wnd->paintstruct->origpen = prevpen;
  }
}

void
TPen::updateW32Brush() const
{
  if (wnd->paintstruct->currentpen!=this)
    return;
  HBRUSH newbrush;
  if (!two_colors)
    newbrush = ::CreateSolidBrush(o_color.colorref);
  else
    newbrush = ::CreateSolidBrush(f_color.colorref);
  HGDIOBJ prevbrush = ::SelectObject(w32hdc, newbrush);
  if (wnd->paintstruct->origbrush) {
    ::DeleteObject(prevbrush);
  } else {
    wnd->paintstruct->origbrush = prevbrush;
  }
}
#endif

// here is plenty room for optimations:

/**
 * Set the line and fill color to <VAR>color</VAR>.
 */
void 
TPen::vsetColor(TCoord r, TCoord g, TCoord b)
{
  if (keepcolor)
    return;
#ifdef __X11__
  if (using_bitmap) {
    XSetFillStyle(x11display, o_gc, FillSolid);
    using_bitmap = false;
  }
#endif

  two_colors = false;
  TColor color(r, g, b);
  if (color==o_color) {
    return;
  }
  o_color=color;

#ifdef __X11__
  o_color._setPen(this, o_gc);
  XSetBackground(x11display, o_gc, TColor::_getPixel(color));
#endif

#ifdef __WIN32__
  w32logpen.lopnColor = o_color.colorref;
  updateW32Pen();
  updateW32Brush();
#endif

#ifdef __COCOA__
  stroke.r = fill.r = r;
  stroke.g = fill.g = g;
  stroke.b = fill.b = b;
  [[NSColor colorWithDeviceRed: r green: g blue: b alpha: 1.0 - stroke.a] set];
#endif
}

/**
 * Set the line color to <VAR>color</VAR>.
 */
void 
TPen::vsetLineColor(TCoord r, TCoord g, TCoord b)
{
  if (keepcolor)
    return;
#ifdef __X11__
  if (using_bitmap) {
    XSetFillStyle(x11display, o_gc, FillSolid);
    using_bitmap = false;
  }
#endif
  TColor color(r, g, b);
  if (two_colors) {
    if (color==o_color)
      return;
  } else {
    if (color==o_color)
      return;
    f_color = o_color;
#ifdef __X11__
    f_color._setPen(this, f_gc);
#endif
    two_colors=true;
  }
  o_color = color;
#ifdef __X11__
  o_color._setPen(this, o_gc);
#endif
#ifdef __WIN32__
  updateW32Pen();
#endif
#ifdef __COCOA__
  stroke.r = r;
  stroke.g = g;
  stroke.b = b;
  [[NSColor colorWithDeviceRed: r green: g blue: b alpha: stroke.a] setStroke];
#endif
}

/**
 * Set the fill color to <VAR>color</VAR>.
 */
void 
TPen::vsetFillColor(TCoord r, TCoord g, TCoord b)
{
  if (keepcolor)
    return;
#ifdef __X11__
  if (using_bitmap) {
    XSetFillStyle(x11display, o_gc, FillSolid);
    using_bitmap = false;
  }
#endif
  TColor color(r, g, b);
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
#ifdef __X11__
  f_color._setPen(this, f_gc);
  XSetBackground(x11display, o_gc, TColor::_getPixel(color));
#endif
#ifdef __WIN32__
  updateW32Brush();
#endif
#ifdef __COCOA__
  fill.r = r;
  fill.g = g;
  fill.b = b;
  [[NSColor colorWithDeviceRed: r green: g blue: b alpha: fill.a] setFill];
#endif
}
