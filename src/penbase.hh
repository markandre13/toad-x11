/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef __TOAD_PENBASE_HH
#define __TOAD_PENBASE_HH 1

//#import <Cocoa/Cocoa.h>

#include <toad/types.hh>
#include <toad/color.hh>
#include <toad/font.hh>
#include <toad/matrix2d.hh>

namespace toad {

class TBitmap;
class TWindow;

class TPenBase
{
  protected:
    TRGB stroke, fill;
  public:
    PFont font;
    bool keepcolor:1;
    bool outline:1;
    
    TPenBase();
    virtual ~TPenBase();

    enum EMode {
      NORMAL=3,
      XOR=6,
      INVERT=10
    };

    enum ELineStyle {
      SOLID=1,
      DASH,
      DOT,
      DASHDOT,
      DASHDOTDOT
    };

    virtual void setFont(const string&) = 0;

    virtual void identity() = 0;
    virtual void translate(TCoord dx, TCoord dy) = 0;
    virtual void scale(TCoord dx, TCoord dy) = 0;
    virtual void rotate(TCoord radiants) = 0;
    virtual void push() = 0;
    virtual void pop() = 0;
    virtual void multiply(const TMatrix2D*) = 0;
    virtual void setMatrix(TCoord a11, TCoord a21, TCoord a12, TCoord a22, TCoord tx, TCoord ty) = 0;
    virtual const TMatrix2D* getMatrix() const = 0;

    virtual void getClipBox(TRectangle*) const = 0;
    virtual void operator&=(const TRectangle&) = 0;
    virtual void operator|=(const TRectangle&) = 0;
    
    virtual void setMode(EMode) = 0;
    virtual void setLineWidth(TCoord) = 0;
    virtual void setLineStyle(ELineStyle) = 0;
    //virtual void setColorMode(TColor::EDitherMode) = 0;
    //virtual void setClipChildren(bool) = 0;

    virtual void vsetColor(TCoord r, TCoord g, TCoord b) = 0;
    virtual void vsetLineColor(TCoord r, TCoord g, TCoord b) = 0;
    virtual void vsetFillColor(TCoord r, TCoord g, TCoord b) = 0;
    
    virtual void vdrawBitmap(TCoord,TCoord,const TBitmap&) = 0;

    virtual void drawPoint(TCoord x, TCoord y) = 0;
    virtual void vdrawRectangle(TCoord x,TCoord y,TCoord w,TCoord h) = 0;
    virtual void vfillRectangle(TCoord x,TCoord y,TCoord w,TCoord h) = 0;
    virtual void vdrawCircle(TCoord x,TCoord y,TCoord w,TCoord h) = 0;
    virtual void vfillCircle(TCoord x,TCoord y,TCoord w,TCoord h) = 0;
    virtual void vdrawArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2) = 0;
    virtual void vfillArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2) = 0;
    virtual void vdrawString(TCoord x, TCoord y, const char *str, size_t len, bool transparent) = 0;

    virtual void drawLine(TCoord x0, TCoord y0, TCoord x1, TCoord y1) {
      TPoint p[2];
      p[0].set(x0, y0);
      p[1].set(x1, y1);
      drawLines(p, 2);
    }
    void drawLine(const TPoint &a, const TPoint &b) {
      drawLine(a.x, a.y, b.x, b.y);
    }
    virtual void drawLines(const TPoint *points, size_t n) = 0; 
    virtual void drawLines(const TPolygon&) = 0;

    TCoord getTextWidth(const char* text, size_t len) const { return font->getTextWidth(text, len); }
    TCoord getTextWidth(const char* text) { return font->getTextWidth(text, strlen(text)); }
    TCoord getTextWidth(const string &text) const { return font->getTextWidth(text.c_str(), text.size()); }
    TCoord getAscent() const { return font->getAscent(); }
    TCoord getDescent() const { return font->getDescent(); }
    TCoord getHeight() const { return font->getHeight(); }

    void setColor(TCoord r, TCoord g, TCoord b) { vsetColor(r, g, b); }
    void setLineColor(TCoord r, TCoord g, TCoord b) { vsetLineColor(r, g, b); }
    void setFillColor(TCoord r, TCoord g, TCoord b) { vsetFillColor(r, g, b); }
    void setColor(const TRGB &rgb) { vsetColor(rgb.r, rgb.g, rgb.b); }
    void setLineColor(const TRGB &rgb) { vsetLineColor(rgb.r, rgb.g, rgb.b); }
    void setFillColor(const TRGB &rgb) { vsetFillColor(rgb.r, rgb.g, rgb.b); }
    void setColor(TColor::EColor e);
    void setLineColor(TColor::EColor e);
    void setFillColor(TColor::EColor e);
    virtual void setAlpha(TCoord a) = 0;
    virtual TCoord getAlpha() const = 0;

    void drawRectanglePC(TCoord x,TCoord y,TCoord w,TCoord h);
    void drawRectanglePC(const TRectangle &r) { drawRectanglePC(r.x,r.y,r.w,r.h); }
    void drawRectanglePC(const TRectangle *r) { drawRectanglePC(r->x,r->y,r->w,r->h); }
    void drawRectanglePC(const TPoint &a, const TPoint &b) { vdrawRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawRectangle(TCoord x,TCoord y,TCoord w,TCoord h) { vdrawRectangle(x, y, w, h); } 
    void drawRectangle(const TRectangle &r) { vdrawRectangle(r.x,r.y,r.w,r.h); }
    void drawRectangle(const TRectangle *r) { vdrawRectangle(r->x,r->y,r->w,r->h); }
    void drawRectangle(const TPoint &a, const TPoint &b) { vdrawRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillRectangle(TCoord x,TCoord y,TCoord w,TCoord h) { vfillRectangle(x, y, w, h); }
    void fillRectangle(const TRectangle &r) { vfillRectangle(r.x,r.y,r.w,r.h); }
    void fillRectangle(const TRectangle *r) { vfillRectangle(r->x,r->y,r->w,r->h); }
    void fillRectangle(const TPoint &a, const TPoint &b) { vfillRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillRectanglePC(TCoord x,TCoord y,TCoord w,TCoord h);
    void fillRectanglePC(const TRectangle &r) { fillRectanglePC(r.x,r.y,r.w,r.h); }
    void fillRectanglePC(const TRectangle *r) { fillRectanglePC(r->x,r->y,r->w,r->h); }
    void fillRectanglePC(const TPoint &a, const TPoint &b) { vfillRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawCirclePC(TCoord x,TCoord y,TCoord w,TCoord h);
    void drawCirclePC(const TRectangle &r) { drawCirclePC(r.x,r.y,r.w,r.h); }
    void drawCirclePC(const TRectangle *r) { drawCirclePC(r->x,r->y,r->w,r->h); }
    void drawCirclePC(const TPoint &a, const TPoint &b) { vdrawCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawCircle(TCoord x,TCoord y,TCoord w,TCoord h) { vdrawCircle(x, y, w, h); } 
    void drawCircle(const TRectangle &r) { vdrawCircle(r.x,r.y,r.w,r.h); }
    void drawCircle(const TRectangle *r) { vdrawCircle(r->x,r->y,r->w,r->h); }
    void drawCircle(const TPoint &a, const TPoint &b) { vdrawCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillCircle(TCoord x,TCoord y,TCoord w,TCoord h) { vfillCircle(x, y, w, h); }
    void fillCircle(const TRectangle &r) { vfillCircle(r.x,r.y,r.w,r.h); }
    void fillCircle(const TRectangle *r) { vfillCircle(r->x,r->y,r->w,r->h); }
    void fillCircle(const TPoint &a, const TPoint &b) { vfillCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillCirclePC(TCoord x,TCoord y,TCoord w,TCoord h);
    void fillCirclePC(const TRectangle &r) { fillCirclePC(r.x,r.y,r.w,r.h); }
    void fillCirclePC(const TRectangle *r) { fillCirclePC(r->x,r->y,r->w,r->h); }
    void fillCirclePC(const TPoint &a, const TPoint &b) { vfillCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawArc(TCoord x,TCoord y,TCoord w,TCoord h, TCoord r1, TCoord r2) { vdrawArc(x, y, w, h, r1, r2); }
    void drawArc(const TRectangle &r, TCoord r1, TCoord r2) { vdrawArc(r.x,r.y,r.w,r.h, r1, r2); }
    void drawArc(const TRectangle *r, TCoord r1, TCoord r2) { vdrawArc(r->x,r->y,r->w,r->h, r1, r2); }
    void drawArc(const TPoint &a, const TPoint &b, TCoord r1, TCoord r2) { vdrawArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void drawArcPC(TCoord x,TCoord y,TCoord w,TCoord h, TCoord r1, TCoord r2);
    void drawArcPC(const TRectangle &r, TCoord r1, TCoord r2) { drawArcPC(r.x,r.y,r.w,r.h, r1, r2); }
    void drawArcPC(const TRectangle *r, TCoord r1, TCoord r2) { drawArcPC(r->x,r->y,r->w,r->h, r1, r2); }
    void drawArcPC(const TPoint &a, const TPoint &b, TCoord r1, TCoord r2) { vdrawArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void fillArc(TCoord x,TCoord y,TCoord w,TCoord h, TCoord r1, TCoord r2) { vfillArc(x, y, w, h, r1, r2); }
    void fillArc(const TRectangle &r, TCoord r1, TCoord r2) { vfillArc(r.x,r.y,r.w,r.h, r1, r2); }
    void fillArc(const TRectangle *r, TCoord r1, TCoord r2) { vfillArc(r->x,r->y,r->w,r->h, r1, r2); }
    void fillArc(const TPoint &a, const TPoint &b, TCoord r1, TCoord r2) { vfillArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void fillArcPC(TCoord x,TCoord y,TCoord w,TCoord h, TCoord r1, TCoord r2);
    void fillArcPC(const TRectangle &r, TCoord r1, TCoord r2) { fillArcPC(r.x,r.y,r.w,r.h, r1, r2); }
    void fillArcPC(const TRectangle *r, TCoord r1, TCoord r2) { fillArcPC(r->x,r->y,r->w,r->h, r1, r2); }
    void fillArcPC(const TPoint &a, const TPoint &b, TCoord r1, TCoord r2) { vfillArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    virtual void drawPolygon(const TPoint *points, size_t n) = 0;
    virtual void drawPolygon(const TPolygon &p) = 0;
    virtual void fillPolygon(const TPoint *points, size_t n) = 0;
    virtual void fillPolygon(const TPolygon &p) = 0;
    
    virtual void drawBezier(TCoord,TCoord,TCoord,TCoord,TCoord,TCoord,TCoord,TCoord) = 0;
    virtual void drawBezier(const TPoint *points, size_t n) = 0;
    virtual void drawBezier(const TPolygon &p) = 0;
    virtual void fillBezier(const TPoint *points, size_t n) = 0;
    virtual void fillBezier(const TPolygon &p) = 0;

    // 3D rectangle
    //-----------------------
    void vdraw3DRectangle(TCoord x, TCoord y, TCoord w, TCoord h, bool inset=true);

    void draw3DRectangle(TCoord x, TCoord y, TCoord w, TCoord h, bool inset=true) { vdraw3DRectangle(x,y,w,h,inset); }
    void draw3DRectangle(const TRectangle &r, bool inset=true) { vdraw3DRectangle(r.x,r.y,r.w,r.h,inset); }
    void draw3DRectangle(const TRectangle *r, bool inset=true) { vdraw3DRectangle(r->x,r->y,r->w,r->h,inset); }
    void draw3DRectangle(const TPoint &a, const TPoint &b, double r1, double r2, bool inset=true) { vdraw3DRectangle(a.x, a.y, b.x-a.x, b.y-a.y, inset); }

    void draw3DRectanglePC(TCoord x, TCoord y, TCoord w, TCoord h, bool inset=true);
    void draw3DRectanglePC(const TRectangle &r, bool inset=true) { draw3DRectanglePC(r.x,r.y,r.w,r.h,inset); }
    void draw3DRectanglePC(const TRectangle *r, bool inset=true) { draw3DRectanglePC(r->x,r->y,r->w,r->h,inset); }
    void draw3DRectanglePC(const TPoint &a, const TPoint &b, double r1, double r2, bool inset=true) { vdraw3DRectangle(a.x, a.y, b.x-a.x, b.y-a.y, inset); }

    void drawString(TCoord x, TCoord y, const char *str, size_t len) { vdrawString(x, y, str, len, true); }
    void drawString(TCoord x, TCoord y, const char *str) { vdrawString(x, y, str, strlen(str), true); }
    void drawString(TCoord x, TCoord y, const string &s) { vdrawString(x, y, s.c_str(), s.size(), true); }
    void fillString(TCoord x, TCoord y, const char *str, size_t len) { vdrawString(x, y, str, len, false); }
    void fillString(TCoord x, TCoord y, const char *str) { vdrawString(x, y, str, strlen(str), false); }
    void fillString(TCoord x, TCoord y, const string &s) { vdrawString(x, y, s.c_str(), s.size(), false); }

    TCoord drawTextWidth(TCoord x, TCoord y, const string &text, unsigned width);
    static TCoord getHeightOfTextFromWidth(TFont *font, const string &text, int width);
    static TCoord textRatio(TCoord x,TCoord y,const string &str, unsigned width, TFont *font, TPenBase *pen);
    void drawBitmap(TCoord x, TCoord y, const TBitmap &bmp) { vdrawBitmap(x,y,bmp); }
    void drawBitmap(TCoord x, TCoord y, const TBitmap *bmp) { vdrawBitmap(x,y,*bmp); }

    static void poly2Bezier(const TPoint *src, size_t n, TPolygon &dst);
    static void poly2Bezier(const TPolygon &p, TPolygon &d);

    virtual void showPage();
};

}

#endif
