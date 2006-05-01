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

#ifndef __TOAD_PEN_HH
#define __TOAD_PEN_HH

#include <toad/os.hh>
#include <toad/config.h>
#include <toad/font.hh>
#include <toad/color.hh>
#include <toad/bitmap.hh>
#include <toad/matrix2d.hh>

#ifdef HAVE_LIBXFT
typedef struct _XftDraw XftDraw;
#endif

namespace toad {

class TRegion;

class TPenBase:
  public TOADBase
{
  public:
    TMatrix2D *mat;

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

/*
    // origin
    //-----------------------
    //! Set the origin to (x, y).
    virtual void setOrigin(int x,int y) = 0;
    //! Move the origin by (dx, dy).
    virtual void translate(int dx,int dy) = 0;
    virtual int originX() const = 0;
    virtual int originY() const = 0;
*/

    virtual void identity() = 0;
    virtual void translate(double dx, double dy) = 0;
    virtual void rotate(double) = 0;
    virtual void scale(double, double) = 0;
    virtual void shear(double, double) = 0;
    virtual void multiply(const TMatrix2D*) = 0;
    virtual void setMatrix(double a11, double a12, double a21, double a22, double tx, double ty) = 0;
    virtual void push() = 0;
    virtual void pop() = 0;
    virtual void popAll() = 0;
    TMatrix2D *getMatrix() const { return mat; }

    // color & pattern
    //-----------------------
    virtual void setBitmap(TBitmap*) = 0;

    virtual void setColor(const TColor&) = 0;
    virtual void setColor(byte r,byte g,byte b) { setColor(TColor(r,g,b)); }
    virtual void setColor(const TRGB &c) { setColor(TColor(c.r,c.g,c.b)); }
    virtual void setColor(TColor::ESystemColor c) { setColor(TColor(c)); }
    virtual void setColor(TColor::EColor16 c) { setColor(TColor(c)); }

    virtual void setAlpha(byte) {}
    virtual byte getAlpha() const { return 255; }

    virtual void setFillColor(const TColor&) = 0;
    virtual void setFillColor(byte r, byte g, byte b) { setFillColor(TColor(r,g,b)); }
    virtual void setFillColor(const TRGB &c) { setFillColor(TColor(c.r,c.g,c.b)); }
    virtual void setFillColor(TColor::ESystemColor c) { setFillColor(TColor(c)); }
    virtual void setFillColor(TColor::EColor16 c) { setFillColor(TColor(c)); }

    virtual void setLineColor(const TColor&) = 0;
    virtual void setLineColor(byte r, byte g, byte b) { setLineColor(TColor(r,g,b)); }
    virtual void setLineColor(const TRGB &c) { setLineColor(TColor(c.r,c.g,c.b)); }
    virtual void setLineColor(TColor::ESystemColor c) { setLineColor(TColor(c)); }
    virtual void setLineColor(TColor::EColor16 c) { setLineColor(TColor(c)); }

    // more parameters
    //-----------------------
    virtual void setFont(const string&) = 0;
    virtual void setMode(EMode) = 0;
    virtual void setLineWidth(int) = 0;
    virtual void setLineStyle(ELineStyle) = 0;
    virtual void setColorMode(TColor::EDitherMode) = 0;
    virtual void setClipChildren(bool) = 0;

    // clipping
    //-----------------------
    virtual void setClipRegion(TRegion*) = 0;
    virtual void setClipRect(const TRectangle&) = 0;
    
    virtual void clrClipBox() = 0;
    virtual void getClipBox(TRectangle*) const = 0;

    virtual void operator&=(const TRectangle&) = 0;
    virtual void operator|=(const TRectangle&) = 0;
    virtual void operator&=(const TRegion&) = 0;
    virtual void operator|=(const TRegion&) = 0;

    // point
    //-----------------------
    virtual void drawPoint(int x,int y) = 0;
    virtual void drawPoint(const TPoint &p) { drawPoint(p.x, p.y); }
    
    // line
    //-----------------------
    virtual void vdrawLine(int x1,int y1,int x2,int y2) = 0;

    void drawLine(int x1,int y1,int x2,int y2) { vdrawLine(x1, y1, x2, y2); }
    void drawLine(const TPoint &a, const TPoint &b) { vdrawLine(a.x, a.y, b.x, b.y); }

    virtual void drawLines(const TPoint *points, int n) = 0;
    virtual void drawLines(const TPolygon&) = 0;

    // rectangle
    //-----------------------
    virtual void vdrawRectangle(int x,int y,int w,int h) = 0;
    virtual void vfillRectangle(int x,int y,int w,int h) = 0;

    void drawRectangle(int x,int y,int w,int h) { vdrawRectangle(x, y, w, h); }
    void drawRectangle(const TRectangle &r) { vdrawRectangle(r.x,r.y,r.w,r.h); }
    void drawRectangle(const TRectangle *r) { vdrawRectangle(r->x,r->y,r->w,r->h); }
    void drawRectangle(const TPoint &a, const TPoint &b) { vdrawRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawRectanglePC(int x,int y,int w,int h);
    void drawRectanglePC(const TRectangle &r) { drawRectanglePC(r.x,r.y,r.w,r.h); }
    void drawRectanglePC(const TRectangle *r) { drawRectanglePC(r->x,r->y,r->w,r->h); }
    void drawRectanglePC(const TPoint &a, const TPoint &b) { vdrawRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillRectangle(int x,int y,int w,int h) { vfillRectangle(x, y, w, h); }
    void fillRectangle(const TRectangle &r) { vfillRectangle(r.x,r.y,r.w,r.h); }
    void fillRectangle(const TRectangle *r) { vfillRectangle(r->x,r->y,r->w,r->h); }
    void fillRectangle(const TPoint &a, const TPoint &b) { vfillRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillRectanglePC(int x,int y,int w,int h);
    void fillRectanglePC(const TRectangle &r) { fillRectanglePC(r.x,r.y,r.w,r.h); }
    void fillRectanglePC(const TRectangle *r) { fillRectanglePC(r->x,r->y,r->w,r->h); }
    void fillRectanglePC(const TPoint &a, const TPoint &b) { vfillRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    // circle
    //-----------------------
    virtual void vdrawCircle(int x,int y,int w,int h) = 0;
    virtual void vfillCircle(int x,int y,int w,int h) = 0;
    
    void drawCircle(int x,int y,int w,int h) { vdrawCircle(x, y, w, h); }
    void drawCircle(const TRectangle &r) { vdrawCircle(r.x,r.y,r.w,r.h); }
    void drawCircle(const TRectangle *r) { vdrawCircle(r->x,r->y,r->w,r->h); }
    void drawCircle(const TPoint &a, const TPoint &b) { vdrawCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawCirclePC(int x,int y,int w,int h);
    void drawCirclePC(const TRectangle &r) { drawCirclePC(r.x,r.y,r.w,r.h); }
    void drawCirclePC(const TRectangle *r) { drawCirclePC(r->x,r->y,r->w,r->h); }
    void drawCirclePC(const TPoint &a, const TPoint &b) { vdrawCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillCircle(int x,int y,int w,int h) { vfillCircle(x, y, w, h); }
    void fillCircle(const TRectangle &r) { vfillCircle(r.x,r.y,r.w,r.h); }
    void fillCircle(const TRectangle *r) { vfillCircle(r->x,r->y,r->w,r->h); }
    void fillCircle(const TPoint &a, const TPoint &b) { vfillCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillCirclePC(int x,int y,int w,int h);
    void fillCirclePC(const TRectangle &r) { fillCirclePC(r.x,r.y,r.w,r.h); }
    void fillCirclePC(const TRectangle *r) { fillCirclePC(r->x,r->y,r->w,r->h); }
    void fillCirclePC(const TPoint &a, const TPoint &b) { vfillCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    // arc
    //-----------------------
    virtual void vdrawArc(int x,int y,int w,int h, double r1, double r2) = 0;
    virtual void vfillArc(int x,int y,int w,int h, double r1, double r2) = 0;
    
    void drawArc(int x,int y,int w,int h, double r1, double r2) { vdrawArc(x, y, w, h, r1, r2); }
    void drawArc(const TRectangle &r, double r1, double r2) { vdrawArc(r.x,r.y,r.w,r.h, r1, r2); }
    void drawArc(const TRectangle *r, double r1, double r2) { vdrawArc(r->x,r->y,r->w,r->h, r1, r2); }
    void drawArc(const TPoint &a, const TPoint &b, double r1, double r2) { vdrawArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void drawArcPC(int x,int y,int w,int h, double r1, double r2);
    void drawArcPC(const TRectangle &r, double r1, double r2) { drawArcPC(r.x,r.y,r.w,r.h, r1, r2); }
    void drawArcPC(const TRectangle *r, double r1, double r2) { drawArcPC(r->x,r->y,r->w,r->h, r1, r2); }
    void drawArcPC(const TPoint &a, const TPoint &b, double r1, double r2) { vdrawArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void fillArc(int x,int y,int w,int h, double r1, double r2) { vfillArc(x, y, w, h, r1, r2); }
    void fillArc(const TRectangle &r, double r1, double r2) { vfillArc(r.x,r.y,r.w,r.h, r1, r2); }
    void fillArc(const TRectangle *r, double r1, double r2) { vfillArc(r->x,r->y,r->w,r->h, r1, r2); }
    void fillArc(const TPoint &a, const TPoint &b, double r1, double r2) { vfillArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void fillArcPC(int x,int y,int w,int h, double r1, double r2);
    void fillArcPC(const TRectangle &r, double r1, double r2) { fillArcPC(r.x,r.y,r.w,r.h, r1, r2); }
    void fillArcPC(const TRectangle *r, double r1, double r2) { fillArcPC(r->x,r->y,r->w,r->h, r1, r2); }
    void fillArcPC(const TPoint &a, const TPoint &b, double r1, double r2) { vfillArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    // polygon
    //-----------------------
    virtual void fillPolygon(const TPoint *points, int n) = 0;
    virtual void drawPolygon(const TPoint *points, int n) = 0;
    virtual void fillPolygon(const TPolygon &polygon) = 0;
    virtual void drawPolygon(const TPolygon &polygon) = 0;

    // bezier curves
    //-----------------------
    virtual void drawBezier(int x1,int y1, int x2,int y2, int x3,int y3, int x4,int y4) = 0;
    virtual void drawBezier(double x1,double y1, double x2,double y2, double x3,double y3, double x4,double y4) = 0;
    virtual void drawBezier(const TPoint *points) { drawPolyBezier(points,4); }
    virtual void drawBezier(const TDPoint *points) { drawPolyBezier(points,4); }

    virtual void drawPolyBezier(const TPoint *points, int n) = 0;
    virtual void drawPolyBezier(const TPolygon &p) = 0;
    virtual void drawPolyBezier(const TDPoint *points, int n) = 0;
    virtual void drawPolyBezier(const TDPolygon &p) = 0;

    virtual void fillPolyBezier(const TPoint *points, int n) = 0;
    virtual void fillPolyBezier(const TPolygon &p) = 0;
    virtual void fillPolyBezier(const TDPoint *points, int n) = 0;
    virtual void fillPolyBezier(const TDPolygon &p) = 0;

    static void poly2Bezier(const TPoint *src, int n, TPolygon &dst);
    static void poly2Bezier(const TPolygon &p, TPolygon &d);
    static void poly2Bezier(const TDPoint *src, int n, TDPolygon &dst);
    static void poly2Bezier(const TDPolygon &p, TDPolygon &d);

    // 3D rectangle
    //-----------------------
    virtual void vdraw3DRectangle(int x, int y, int w, int h, bool inset=true) = 0;

    void draw3DRectangle(int x, int y, int w, int h, bool inset=true) { vdraw3DRectangle(x,y,w,h,inset); }
    void draw3DRectangle(const TRectangle &r, bool inset=true) { vdraw3DRectangle(r.x,r.y,r.w,r.h,inset); }
    void draw3DRectangle(const TRectangle *r, bool inset=true) { vdraw3DRectangle(r->x,r->y,r->w,r->h,inset); }
    void draw3DRectangle(const TPoint &a, const TPoint &b, double r1, double r2, bool inset=true) { vdraw3DRectangle(a.x, a.y, b.x-a.x, b.y-a.y, inset); }

    void draw3DRectanglePC(int x, int y, int w, int h, bool inset=true);
    void draw3DRectanglePC(const TRectangle &r, bool inset=true) { draw3DRectanglePC(r.x,r.y,r.w,r.h,inset); }
    void draw3DRectanglePC(const TRectangle *r, bool inset=true) { draw3DRectanglePC(r->x,r->y,r->w,r->h,inset); }
    void draw3DRectanglePC(const TPoint &a, const TPoint &b, double r1, double r2, bool inset=true) { vdraw3DRectangle(a.x, a.y, b.x-a.x, b.y-a.y, inset); }

    // text string
    //-----------------------
    int getTextWidth(const char* text, size_t len) const {
      return vgetTextWidth(text, len);
    }
    int getTextWidth(const char* text) {
      return vgetTextWidth(text, strlen(text));
    }
    int getTextWidth(const string &text) const {
      return vgetTextWidth(text.c_str(), text.size());
    }
    virtual int vgetTextWidth(const char* text, size_t len) const = 0;

    virtual int getAscent() const = 0;
    virtual int getDescent() const = 0;
    virtual int getHeight() const = 0;

    void drawString(int x, int y, const char *str, size_t len) {
      vdrawString(x, y, str, len, true);
    }
    void drawString(int x, int y, const char *str) {
      vdrawString(x, y, str, strlen(str), true);
    }
    void drawString(int x, int y, const string &s) {
      vdrawString(x, y, s.c_str(), s.size(), true);
    }
    void fillString(int x, int y, const char *str, size_t len) {
      vdrawString(x, y, str, len, false);
    }
    void fillString(int x, int y, const char *str) {
      vdrawString(x, y, str, strlen(str), false);
    }
    void fillString(int x, int y, const string &s) {
      vdrawString(x, y, s.c_str(), s.size(), false);
    }
    virtual void vdrawString(int x, int y, const char *str, int len, bool transparent) = 0;

    virtual int drawTextWidth(int x, int y, const string &text, unsigned width) = 0;
    // void drawTextAspect(int x,int y,const char* text,double xa,double ya);

    // bitmap
    //-----------------------
    virtual void drawBitmap(int x,int y, const TBitmap*) = 0;
    virtual void drawBitmap(int x,int y, const TBitmap&) = 0;
    virtual void drawBitmap(int,int,const TBitmap*, int,int,int,int) = 0;
    virtual void drawBitmap(int,int,const TBitmap&, int,int,int,int) = 0;

    // ops with cursor
    //-----------------------
    virtual void moveTo(int x, int y) = 0;
    virtual void moveTo(const TPoint &p) = 0;
    virtual void lineTo(int x, int y) = 0;
    virtual void lineTo(const TPoint &p) = 0;
    virtual void curveTo(int x2, int y2, int x3, int y3, int x4, int y4) = 0;
    virtual void curveTo(const TPoint &p2, const TPoint &p3, const TPoint &p4) = 0;
    virtual void curveTo(double x2, double y2, double x3, double y3, double x4, double y4) = 0;
};

class TFontManagerX11;
class TFontMangerFT;

class TPen:
  public TPenBase
{
    friend class TWindow;
    friend class TBitmap;
    friend class TColor;
    friend class TFontManagerX11;
    friend class TFontManagerFT;

  public:
    TPen(TBitmap*);
    TPen(TWindow*);
    virtual ~TPen();

    void identity();
    void translate(double dx, double dy);
    void rotate(double);
    void scale(double, double);
    void shear(double, double);
    void multiply(const TMatrix2D*);
    void setMatrix(double a11, double a12, double a21, double a22, double tx, double ty);
    void push();
    void pop();
    void popAll();

    // color & pattern
    //-----------------------
    void setBitmap(TBitmap*);

    void setColor(const TColor&);
    void setColor(byte r,byte g,byte b) { setColor(TColor(r,g,b)); }
    void setColor(const TRGB &c) { setColor(TColor(c.r,c.g,c.b)); }
    void setColor(TColor::ESystemColor c) { setColor(TColor(c)); }
    void setColor(TColor::EColor16 c) { setColor(TColor(c)); }

    void setFillColor(const TColor&);
    void setFillColor(byte r, byte g, byte b) { setFillColor(TColor(r,g,b)); }
    void setFillColor(const TRGB &c) { setFillColor(TColor(c.r,c.g,c.b)); }
    void setFillColor(TColor::ESystemColor c) { setFillColor(TColor(c)); }
    void setFillColor(TColor::EColor16 c) { setFillColor(TColor(c)); }

    void setLineColor(const TColor&);
    void setLineColor(byte r, byte g, byte b) { setLineColor(TColor(r,g,b)); }
    void setLineColor(const TRGB &c) { setLineColor(TColor(c.r,c.g,c.b)); }
    void setLineColor(TColor::ESystemColor c) { setLineColor(TColor(c)); }
    void setLineColor(TColor::EColor16 c) { setLineColor(TColor(c)); }

    // more parameters
    //-----------------------
private:
    void setFont(TFont*);
public:
    static void initialize();
    static void terminate();

    void setFont(const string &fontname);
    static TFont* lookupFont(const string &fontname);
    void setMode(EMode);
    void setLineWidth(int);
    void setLineStyle(ELineStyle);
    void setColorMode(TColor::EDitherMode);
    void setClipChildren(bool);

    // clipping
    //-----------------------
    void setClipRegion(TRegion*);
    void setClipRect(const TRectangle&);
    
    void clrClipBox();
    void getClipBox(TRectangle*) const;

    void operator&=(const TRectangle&);
    void operator|=(const TRectangle&);
    void operator&=(const TRegion&);
    void operator|=(const TRegion&);

    // point
    //-----------------------
    void drawPoint(int x,int y);
    void drawPoint(const TPoint &p) { drawPoint(p.x, p.y); }
    
    // line
    //-----------------------
    void vdrawLine(int x1,int y1,int x2,int y2);
    void drawLines(const TPoint *points, int n);
    void drawLines(const TPolygon&);

    // rectangle
    //-----------------------
    void vdrawRectangle(int x,int y,int w,int h);
    void vfillRectangle(int x,int y,int w,int h);

    // circle
    //-----------------------
    void vdrawCircle(int x,int y,int w,int h);
    void vfillCircle(int x,int y,int w,int h);

    // arc
    //-----------------------   
    void vdrawArc(int x,int y,int w,int h, double r1, double r2);
    void vfillArc(int x,int y,int w,int h, double r1, double r2);

    // polygon
    //-----------------------
    void fillPolygon(const TPoint *points, int n);
    void drawPolygon(const TPoint *points, int n);
    void fillPolygon(const TPolygon &polygon);
    void drawPolygon(const TPolygon &polygon);

    // bezier curves
    //-----------------------
    void drawBezier(int x1,int y1, int x2,int y2, int x3,int y3, int x4,int y4);
    void drawBezier(double x1,double y1, double x2,double y2, double x3,double y3, double x4,double y4);
    void drawBezier(const TPoint *points) { drawPolyBezier(points,4); }
    void drawBezier(const TDPoint *points) { drawPolyBezier(points,4); }

    void drawPolyBezier(const TPoint *points, int n);
    void drawPolyBezier(const TPolygon &p); // { drawPolyBezier(p.begin(), p.size()); }
    void drawPolyBezier(const TDPoint *points, int n);
    void drawPolyBezier(const TDPolygon &p); // { drawPolyBezier(p.begin(), p.size()); }

    void fillPolyBezier(const TPoint *points, int n);
    void fillPolyBezier(const TPolygon &p); // { fillPolyBezier(p.begin(), p.size()); }
    void fillPolyBezier(const TDPoint *points, int n);
    void fillPolyBezier(const TDPolygon &p); // { fillPolyBezier(p.begin(), p.size()); }

    // 3D rectangle
    //-----------------------
    void vdraw3DRectangle(int x, int y, int w, int h, bool inset=true);

    // text string
    //-----------------------
    int vgetTextWidth(const char* text, size_t len) const;
    int getAscent() const;
    int getDescent() const;
    int getHeight() const;
    void vdrawString(int x, int y, const char*, int len, bool transparent);
    int drawTextWidth(int x, int y, const string &text, unsigned width);
    static int getHeightOfTextFromWidth(TFont *font, const string &text, int width);
    // void drawTextAspect(int x,int y,const char* text,double xa,double ya);

    // bitmap
    //-----------------------
    void drawBitmap(int x,int y, const TBitmap*);
    void drawBitmap(int x,int y, const TBitmap&);
    void drawBitmap(int,int,const TBitmap*, int,int,int,int);
    void drawBitmap(int,int,const TBitmap&, int,int,int,int);

    // ops with cursor
    //-----------------------
    void moveTo(int x, int y) { _pos.set(x,y); }
    void moveTo(const TPoint &p) { _pos = p; }
    void lineTo(int x, int y) { drawLine(_pos.x, _pos.y, x,y); _pos.set(x,y); }
    void lineTo(const TPoint &p) { drawLine(_pos.x, _pos.y, p.x,p.y); _pos = p; }
    void curveTo(int x2, int y2, int x3, int y3, int x4, int y4) {
      drawBezier(_pos.x, _pos.y, x2,y2, x3,y3, x4,y4);
      _pos.set(x4,y4);
    }
    void curveTo(const TPoint &p2, const TPoint &p3, const TPoint &p4) {
      drawBezier(_pos.x, _pos.y, p2.x,p2.y, p3.x,p3.y, p4.x,p4.y);
      _pos=p4;
    }
    void curveTo(double x2, double y2, double x3, double y3, double x4, double y4) {
      drawBezier((double)_pos.x, (double)_pos.y, x2,y2, x3,y3, x4,y4);
      _pos.set((int)x4,(int)y4);
    }
    
  private:
    //! position for operations with cursor
    TPoint _pos;
  
    TColor o_color,       // outline/only color
           f_color;       // fill color
    bool two_colors:1;    // `true' when fill- and outline color are different
    bool using_bitmap:1;  // `true' after `SetBitmap'
    bool bDeleteRegion:1; // delete `region' in destructor
    TColor::EDitherMode cmode;
    int width;
    ELineStyle style;
    TWindow *wnd;
    TBitmap *bmp;
    PFont font;
  public:
//#warning "'TRegion *region' is public"
    TRegion *region;
//  private:
    void _init();
    
    #ifdef __X11__    
    _TOAD_GC o_gc, f_gc;
    _TOAD_DRAWABLE x11drawable; // either window or pixmap
    
    #ifdef HAVE_LIBXFT
    XftDraw *xftdraw;
    #endif
    
    void _setLineAttributes();
    #endif
    
    #ifdef __WIN32__
    HDC w32hdc;
    HWND w32window;

    HGDIOBJ w32pen;
    LOGPEN w32logpen;
    
    HGDIOBJ w32brush;
    // LOGBRUSH w32logbrush;

    void activateW32() const;
    void updateW32Pen() const;
    void updateW32Brush() const;

    #endif
};

} // namespace toad

#endif
