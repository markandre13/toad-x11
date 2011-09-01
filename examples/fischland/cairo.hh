/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2007 by Mark-André Hopf <mhopf@mark13.org>
 * Visit http://www.mark13.org/fischland/.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef HAVE_LIBCAIRO

#ifndef _TOAD_CAIRO_HH
#define _TOAD_CAIRO_HH 1

extern "C" {
#include <cairo/cairo-xlib-xrender.h>
#include <cairo/cairo-pdf.h>
}

#include <stack>
#include <toad/pen.hh>

namespace toad {

class TCairo:
  public TPenBase
{
public:
    typedef TPenBase super;
    TRectangle clipbox;
    
    stack<cairo_matrix_t> mstack;

    cairo_surface_t *cs;
    cairo_t *cr;

    TRGB foreground, background;
    TCoord alpha;
    TCoord width;

  public:
    TCairo(TWindow*);
    TCairo(TBitmap*);
    TCairo(cairo_surface_t *);
    TCairo(const string &pdffilename);
    ~TCairo();
    
    void draw(double x, double y, cairo_surface_t *s);
    
    void showPage();
    void identity();
    void translate(TCoord dx, TCoord dy);
    void scale(TCoord dx, TCoord dy);
    void rotate(TCoord radiants);
    void multiply(const TMatrix2D*);
    void setMatrix(TCoord a11, TCoord a21, TCoord a12, TCoord a22, TCoord tx, TCoord ty);
    void push();
    void pop(); 
    void popAll();
    const TMatrix2D *getMatrix() const;
                                            
    void _updateCairoMatrix();

    void vsetColor(TCoord r, TCoord g, TCoord b);
    void vsetLineColor(TCoord r, TCoord g, TCoord b);
    void vsetFillColor(TCoord r, TCoord g, TCoord b);
    void setAlpha(TCoord a);
    TCoord getAlpha() const;
    void _fill();
    
    void setLineWidth(TCoord w);
    void setLineStyle(ELineStyle n);
    
    void setBitmap(TBitmap *b);
    void setFont(const string &fn);
    void setMode(EMode m);
    void setColorMode(TColor::EDitherMode m);
    void setClipChildren(bool b);
    void setClipRegion(TRegion *r);
    void setClipRect(const TRectangle &r);
    void clrClipBox();
    void getClipBox(TRectangle *r) const;
    void operator&=(const TRectangle&);
    void operator|=(const TRectangle&);
    void operator&=(const TRegion&);
    void operator|=(const TRegion&);

    void vdrawRectangle(TCoord x, TCoord y, TCoord w, TCoord h);
    void vfillRectangle(TCoord x, TCoord y, TCoord w, TCoord h);
    void vdrawCircle(TCoord x,TCoord y,TCoord w,TCoord h);
    void vfillCircle(TCoord x,TCoord y,TCoord w,TCoord h);
    void vdrawArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2);
    void vfillArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2);
    void vdrawBitmap(TCoord x, TCoord y, const TBitmap&);
    void vdrawString(TCoord x, TCoord y, const char *str, size_t len, bool transparent);

    TCoord vgetTextWidth(const char *txt, size_t len) const;
    TCoord getAscent() const;
    TCoord getDescent() const;
    TCoord getHeight() const;

    void drawPoint(TCoord x, TCoord y);
    void drawLines(const TPoint *points, size_t n);
    void drawLines(const TPolygon&);

    void drawPolygon(const TPoint *points, size_t n);
    void drawPolygon(const TPolygon &p);
    void fillPolygon(const TPoint *points, size_t n);
    void fillPolygon(const TPolygon &p);

    void drawBezier(TCoord,TCoord,TCoord,TCoord,TCoord,TCoord,TCoord,TCoord);
    void drawBezier(const TPoint *points, size_t n);
    void drawBezier(const TPolygon &p);
    void fillBezier(const TPoint *points, size_t n);
    void fillBezier(const TPolygon &p);

    // bitmap
    //-----------------------
    void drawBitmap(int x,int y, const TBitmap*);
    void drawBitmap(int x,int y, const TBitmap&);
    void drawBitmap(int,int,const TBitmap*, int,int,int,int);
    void drawBitmap(int,int,const TBitmap&, int,int,int,int);

    void moveTo(int, int);
    void moveTo(const TPoint&);
    void lineTo(int, int);
    void lineTo(const TPoint&);
    void curveTo(int, int, int, int, int, int);
    void curveTo(const TPoint&, const TPoint&, const TPoint&);
    void curveTo(double, double, double, double, double, double);
};

} // namespace toad

#endif

#endif
