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

// Copy TPoint array to XPoint array and add translation in (_dx, _dy).
#define TPOINT_2_XPOINT(SRC, DST, SIZE) \
  XPoint DST[n]; \
  const TPoint *sp = SRC; \
  const TPoint *se = SRC+SIZE; \
  XPoint *dp = DST; \
  while(sp!=se) { \
    dp->x = sp->x + _dx; \
    dp->y = sp->y + _dy; \
    dp++; \
    sp++; \
  }

// Adjust (width, height) to raster coordinates for XDraw... operations.
#define XDRAW_RASTER_COORD(w,h) \
  if (!w || !h) \
    return; \
  if (w<0) { \
    w=-w; \
    x-=w-1; \
  } \
  if (h<0) { \
    h=-h; \
    y-=h-1; \
  } \
  w--; \
  h--;

// Adjust (width, height) to pixel coordinates for XDraw... operations.
#define XDRAW_PIXEL_COORD(w,h) \
  if (w<0) { \
    w=-w; \
    x-=w; \
  } \
  if (h<0) { \
    h=-h; \
    y-=h; \
  }

// point
//----------------------------------------------------------------------------
void TPen::drawPoint(int x, int y) const
{
  XDrawPoint(x11display, x11drawable, o_gc, x+_dx, y+_dy);
}

// line
//----------------------------------------------------------------------------
void TPen::drawLine(int x1, int y1, int x2, int y2) const
{
  XDrawLine(x11display, x11drawable, o_gc, x1+_dx,y1+_dy, x2+_dx,y2+_dy);
}

void TPen::drawLines(const TPoint *s, int n) const
{
  TPOINT_2_XPOINT(s,d,n)
  XDrawLines(x11display, x11drawable, o_gc, d, n, CoordModeOrigin);
}

void TPen::drawLines(const TPolygon &polygon) const
{
  unsigned n = polygon.size();
  XPoint DST[n];
  TPolygon::const_iterator sp, se;
  sp = polygon.begin();
  se = polygon.end();
  XPoint *dp = DST;
  while(sp!=se) {
   dp->x = sp->x + _dx;
   dp->y = sp->y + _dy;
   dp++;
   ++sp;
  }

  XDrawLines(x11display, x11drawable, o_gc, 
    DST, n, CoordModeOrigin);
}

// rectangle
//----------------------------------------------------------------------------
void TPen::drawRectangle(int x, int y, int w, int h) const
{
  XDRAW_RASTER_COORD(w,h)
  if (w==0 || h==0) {
    XDrawLine(x11display, x11drawable, o_gc, x+_dx, y+_dy, x+_dx+w,y+_dy+h);
    return;
  }
  XDrawRectangle(x11display, x11drawable, o_gc, x+_dx,y+_dy,w,h);
}

void TPen::drawRectanglePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  XDrawRectangle(x11display, x11drawable, o_gc, x+_dx,y+_dy,w,h);
}

void TPen::fillRectangle(int x, int y, int w, int h) const
{
  XDRAW_RASTER_COORD(w,h)
  if (two_colors) {
    XFillRectangle(x11display, x11drawable, f_gc, x+_dx, y+_dy,w,h);
    XDrawRectangle(x11display, x11drawable, o_gc, x+_dx, y+_dy,w,h);
  } else {
    XFillRectangle(x11display, x11drawable, o_gc, x+_dx,y+_dy,w+1,h+1);
  }
}

void TPen::fillRectanglePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  if (two_colors) {
    XFillRectangle(x11display, x11drawable, f_gc, x+_dx, y+_dy,w,h);
    XDrawRectangle(x11display, x11drawable, o_gc, x+_dx, y+_dy,w,h);
  } else {
    XFillRectangle(x11display, x11drawable, o_gc, x+_dx,y+_dy,w+1,h+1);
  }
}

// circle
//----------------------------------------------------------------------------
void TPen::drawCircle(int x, int y, int w, int h) const
{
  XDRAW_RASTER_COORD(w,h)
  if (w==0 || h==0) {
    XDrawLine(x11display, x11drawable, o_gc, x+_dx, y+_dy, x+_dx+w,y+_dy+h);
    return;
  }
  XDrawArc(x11display, x11drawable, o_gc, x+_dx,y+_dy,w,h, 0,360*64);
}

void TPen::drawCirclePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  XDrawArc(x11display, x11drawable, o_gc, x+_dx,y+_dy,w,h, 0,360*64);
}

void TPen::fillCircle(int x, int y, int w, int h) const
{
  XDRAW_RASTER_COORD(w,h)
  XDRAW_PIXEL_COORD(w,h)
  XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x+_dx, y+_dy,w,h, 0,360*64);
  XDrawArc(x11display, x11drawable, o_gc, x+_dx, y+_dy,w,h, 0,360*64);
}

void TPen::fillCirclePC(int x, int y, int w, int h) const
{
  XDRAW_PIXEL_COORD(w,h)
  XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x+_dx, y+_dy,w,h, 0,360*64);
  XDrawArc(x11display, x11drawable, o_gc, x+_dx, y+_dy,w,h, 0,360*64);
}

// arc
//----------------------------------------------------------------------------
void TPen::drawArc(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_RASTER_COORD(w,h)
  if (w==0 || h==0) {
    XDrawLine(x11display, x11drawable, o_gc, x+_dx, y+_dy, x+_dx+w,y+_dy+h);
    return;
  }
  XDrawArc(x11display, x11drawable, o_gc, x+_dx,y+_dy,w,h, (int)(r1*64.0),(int)(r2*64.0));
}

void TPen::drawArcPC(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_PIXEL_COORD(w,h)
  XDrawArc(x11display, x11drawable, o_gc, x+_dx,y+_dy,w,h, (int)(r1*64.0),(int)(r2*64.0));
}

void TPen::fillArc(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_RASTER_COORD(w,h)
  XDRAW_PIXEL_COORD(w,h)
  int i1=(int)(r1*64.0);
  int i2=(int)(r2*64.0);
  XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x+_dx, y+_dy,w,h, i1,i2);
  XDrawArc(x11display, x11drawable, o_gc, x+_dx, y+_dy,w,h, i1,i2);
}

void TPen::fillArcPC(int x, int y, int w, int h, double r1, double r2) const
{
  XDRAW_PIXEL_COORD(w,h)
  int i1=(int)(r1*64.0);
  int i2=(int)(r2*64.0);
  XFillArc(x11display, x11drawable, two_colors ? f_gc : o_gc, x+_dx, y+_dy,w,h, i1,i2);
  XDrawArc(x11display, x11drawable, o_gc, x+_dx, y+_dy,w,h, i1,i2);
}

// polygon
//----------------------------------------------------------------------------
void
TPen::drawPolygon(const TPoint points[], int n) const
{
  drawLines(points, n);
  drawLine(points[0].x,points[0].y,points[n-1].x,points[n-1].y);
}

void
TPen::fillPolygon(const TPoint s[], int n) const
{
   TPOINT_2_XPOINT(s,d,n);
   XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
   d, n, Nonconvex, CoordModeOrigin);
   XDrawLines(x11display, x11drawable, o_gc, 
       d, n, CoordModeOrigin);
   XDrawLine(x11display, x11drawable, o_gc,
      s[0].x,s[0].y,
      s[n-1].x,s[n-1].y);
}

void
TPen::drawPolygon(const TPolygon &polygon) const
{
  unsigned n = polygon.size();
  XPoint DST[n];
  TPolygon::const_iterator sp, se;
  sp = polygon.begin();
  se = polygon.end();
  XPoint *dp = DST;
  while(sp!=se) {
   dp->x = sp->x + _dx;
   dp->y = sp->y + _dy;
   dp++;
   ++sp;
  }

  XDrawLines(x11display, x11drawable, o_gc, 
    DST, n, CoordModeOrigin);
  XDrawLine(x11display, x11drawable, o_gc,
    DST[0].x,DST[0].y,
    DST[n-1].x,DST[n-1].y);
}

void
TPen::fillPolygon(const TPolygon &polygon) const
{
  unsigned n = polygon.size();
  XPoint DST[n];
  TPolygon::const_iterator sp, se;
  sp = polygon.begin();
  se = polygon.end();
  XPoint *dp = DST;
  while(sp!=se) {
   dp->x = sp->x + _dx;
   dp->y = sp->y + _dy;
   dp++;
   ++sp;
  }

  XFillPolygon(x11display, x11drawable, two_colors? f_gc : o_gc, 
    DST, n, Nonconvex, CoordModeOrigin);
  XDrawLines(x11display, x11drawable, o_gc, 
    DST, n, CoordModeOrigin);
  XDrawLine(x11display, x11drawable, o_gc,
    DST[0].x,DST[0].y,
    DST[n-1].x,DST[n-1].y);
}

// bitmap
//----------------------------------------------------------------------------
void TPen::drawBitmap(int x, int y, const TBitmap* bmp) const
{
  bmp->drawBitmap(this, x+_dx, y+_dy);
}

void TPen::drawBitmap(int x, int y, const TBitmap& bmp) const
{
  bmp.drawBitmap(this, x+_dx, y+_dy);
}

void TPen::drawBitmap(int x, int y, const TBitmap* bmp, int ax, int ay, int aw, int ah) const
{
  bmp->drawBitmap(this, x+_dx, y+_dy, ax,ay,aw,ah);
}

void TPen::drawBitmap(int x, int y, const TBitmap& bmp, int ax, int ay, int aw, int ah) const
{
  bmp.drawBitmap(this, x+_dx, y+_dy, ax,ay,aw,ah);
}

// 3D rectangle
//----------------------------------------------------------------------------
/**
 * This is a special function for widgets.
 */
void TPen::draw3DRectangle(int x, int y, int w, int h, bool inset) const
{
  TColor saved_color = o_color;
  
  TPen *t = const_cast<TPen*>(this);
  
  TPoint p[3];
  if (inset)
    t->setColor(255,255,255);
  else
    t->setColor(0,0,0);
  p[0].set(x+1  ,y+h-1);
  p[1].set(x+w-1,y+h-1);
  p[2].set(x+w-1,y);
  drawLines(p,3);

  if (inset)  
    t->setColor(192,192,192);
  else
    t->setColor(128,128,128);
  p[0].set(x+2  ,y+h-2);
  p[1].set(x+w-2,y+h-2);
  p[2].set(x+w-2,y+1);
  drawLines(p,3);

  if (inset)
    t->setColor(128,128,128);
  else
    t->setColor(192,192,192);
  p[0].set(x    ,y+h-1);
  p[1].set(x    ,y);
  p[2].set(x+w-1,y);
  drawLines(p,3);
  
  if (inset)
    t->setColor(0,0,0);
  else
    t->setColor(255,255,255);
  p[0].set(x+1  ,y+h-2);
  p[1].set(x+1  ,y+1);
  p[2].set(x+w-2,y+1);
  drawLines(p,3);
  
  t->setColor(saved_color);
}

// text string
//----------------------------------------------------------------------------
int TPen::getTextWidth(const string &str) const
{
  return font->getTextWidth(str.c_str());
}

int TPen::getTextWidth(const char *str) const
{
  return font->getTextWidth(str);
}

/**
 * Width of 'str' when printed with the current font.
 */
int TPen::getTextWidth(const char *str, int len) const
{
  return font->getTextWidth(str,len);
}

/**
 * Ascent of the current font.
 */
int TPen::getAscent() const
{
  return font->getAscent();
}

/**
 * Descent of the current font.
 */
int TPen::getDescent() const
{
  return font->getDescent();
}

/**
 * Height of the current font.
 */
int TPen::getHeight() const
{
  return font->getHeight();
}

/**
 * Draw string `str'. <VAR>x</VAR>, <VAR>y</VAR> is the upper left
 * coordinate of the string.<BR>
 * DrawString is a little bit slower than FillString.
 */
void TPen::drawString(int x,int y, const string &str) const
{
  TPen::drawString(x,y,str.c_str(),(int)str.size());
}

void TPen::drawString(int x,int y, const char *str, int strlen) const
{
  if (!str)
    return;
  XDrawString(x11display, x11drawable, o_gc, x+_dx,y+_dy+getAscent(), str, strlen);
}

/**
 * FillString will fill the background with the current back color when
 * drawing the string.<BR>
 * The back color can be set with SetBackColor.<BR>
 * Please note that FillString doesn't support color dithering and will
 * use the nearest color TOAD was able to allocate.<BR>
 * Maybe i'm going to rename this method into `PrintString' since
 * `FillString' is really a very idiotic name.
 */
void TPen::fillString(int x,int y, const string &str) const
{
  TPen::fillString(x,y,str.c_str(),(int)str.size());
}

void TPen::fillString(int x,int y, const char *str, int strlen) const
{
  if (!str)
    return;
  XDrawImageString(x11display, x11drawable, o_gc, x+_dx,y+_dy+getAscent(), str, strlen);
}

// DrawTextWidth
//-------------------------------------------------------------------------
//. Draw string 'str' in multiple lines, reduce spaces between words to one 
//. an break lines to fit width. 'str' can contain '\n'.
int TPen::drawTextWidth(int x,int y,const string &str, unsigned width) const
{
  x+=_dx;
  y+=_dy;
  const char* text=str.c_str();
  
  unsigned i;
  
  // 1st step: count words and lines
  unsigned word_count, min_lines;
  font->count_words_and_lines(text, &word_count, &min_lines);
  if (!word_count) return 0;
  
  // 2nd step: create a word list
  TFont::TWord* word = font->make_wordlist(text, word_count);
  
  // 3rd step: output
  unsigned blank_width = getTextWidth(" ",1);
  unsigned line_len = 0;
  unsigned word_of_line = 1;
  
  for(i=0; i<word_count; i++)
    {
      if ((line_len+word[i].len>width && i!=0) || word[i].linefeeds)
  {
    if (word[i].linefeeds)
      y+=getHeight()*word[i].linefeeds;
    else
      y+=getHeight();
    line_len = 0;
    word_of_line = 0;
  }
      drawString(x+line_len,y, word[i].pos, word[i].bytes);
      line_len+=word[i].len+blank_width;
      word_of_line++;
    }
  
  delete[] word;
  return y+getHeight();
}
