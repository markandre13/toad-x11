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

#ifndef TPen
#define TPen TPen

#include <toad/font.hh>
#include <toad/color.hh>
#include <toad/bitmap.hh>
#include <toad/matrix2d.hh>

namespace toad {

class TRegion;

class TPenBase:
  public TOADBase
{
  public:
    TMatrix2D *mat;

    virtual ~TPenBase();
  
    // rename this into EMode
    enum EPenMode {
      NORMAL=3,
      XOR=6,
      INVERT=10
    };

    // rename this into ELineStyle
    enum EPenLineStyle {
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

    // color & pattern
    //-----------------------
    virtual void setBitmap(TBitmap*) = 0;

    virtual void setColor(const TColor&) = 0;
    virtual void setColor(byte r,byte g,byte b) { setColor(TColor(r,g,b)); }
    virtual void setColor(const TRGB &c) { setColor(TColor(c.r,c.g,c.b)); }
    virtual void setColor(TColor::ESystemColor c) { setColor(TColor(c)); }
    virtual void setColor(TColor::EColor16 c) { setColor(TColor(c)); }

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

    virtual void setBackColor(const TColor&) = 0;
    virtual void setBackColor(byte r, byte g, byte b) { setBackColor(TColor(r,g,b)); }
    virtual void setBackColor(const TRGB &c) { setBackColor(TColor(c.r,c.g,c.b)); }
    virtual void setBackColor(TColor::ESystemColor c) { setBackColor(TColor(c)); }
    virtual void setBackColor(TColor::EColor16 c) { setBackColor(TColor(c)); }

    // more parameters
    //-----------------------
    virtual TFont* setFont(TFont*) = 0;
    virtual void setMode(EPenMode) = 0;
    virtual void setLineWidth(int) = 0;
    virtual void setLineStyle(EPenLineStyle) = 0;
    virtual void setColorMode(TColor::EDitherMode) = 0;
    virtual void setClipChildren(bool) = 0;

    // clipping
    //-----------------------
    virtual void setClipRegion(TRegion*) = 0;
    virtual void setClipRect(const TRectangle&) = 0;
    
    virtual void clrClipBox() = 0;
    virtual void getClipBox(TRectangle*) const = 0;

    virtual void operator&=(TRectangle&) = 0;
    virtual void operator|=(TRectangle&) = 0;

    // point
    //-----------------------
    virtual void drawPoint(int x,int y) const = 0;
    virtual void drawPoint(const TPoint &p) const { drawPoint(p.x, p.y); }
    
    // line
    //-----------------------
    virtual void vdrawLine(int x1,int y1,int x2,int y2) const = 0;

    void drawLine(int x1,int y1,int x2,int y2) const { vdrawLine(x1, y1, x2, y2); }
    void drawLine(const TPoint &a, const TPoint &b) const { vdrawLine(a.x, a.y, b.x, b.y); }

    virtual void drawLines(const TPoint *points, int n) const = 0;
    virtual void drawLines(const TPolygon&) const = 0;

    // rectangle
    //-----------------------
    virtual void vdrawRectangle(int x,int y,int w,int h) const = 0;
    virtual void vfillRectangle(int x,int y,int w,int h) const = 0;

    void drawRectangle(int x,int y,int w,int h) const { vdrawRectangle(x, y, w, h); }
    void drawRectangle(const TRectangle &r) const { vdrawRectangle(r.x,r.y,r.w,r.h); }
    void drawRectangle(const TRectangle *r) const { vdrawRectangle(r->x,r->y,r->w,r->h); }
    void drawRectangle(const TPoint &a, const TPoint &b) const { vdrawRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawRectanglePC(int x,int y,int w,int h) const;
    void drawRectanglePC(const TRectangle &r) const { drawRectanglePC(r.x,r.y,r.w,r.h); }
    void drawRectanglePC(const TRectangle *r) const { drawRectanglePC(r->x,r->y,r->w,r->h); }
    void drawRectanglePC(const TPoint &a, const TPoint &b) const { vdrawRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillRectangle(int x,int y,int w,int h) const { vfillRectangle(x, y, w, h); }
    void fillRectangle(const TRectangle &r) const { vfillRectangle(r.x,r.y,r.w,r.h); }
    void fillRectangle(const TRectangle *r) const { vfillRectangle(r->x,r->y,r->w,r->h); }
    void fillRectangle(const TPoint &a, const TPoint &b) const { vfillRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillRectanglePC(int x,int y,int w,int h) const;
    void fillRectanglePC(const TRectangle &r) const { fillRectanglePC(r.x,r.y,r.w,r.h); }
    void fillRectanglePC(const TRectangle *r) const { fillRectanglePC(r->x,r->y,r->w,r->h); }
    void fillRectanglePC(const TPoint &a, const TPoint &b) const { vfillRectangle(a.x, a.y, b.x-a.x, b.y-a.y); }

    // circle
    //-----------------------
    virtual void vdrawCircle(int x,int y,int w,int h) const = 0;
    virtual void vfillCircle(int x,int y,int w,int h) const = 0;
    
    void drawCircle(int x,int y,int w,int h) const { vdrawCircle(x, y, w, h); }
    void drawCircle(const TRectangle &r) const { vdrawCircle(r.x,r.y,r.w,r.h); }
    void drawCircle(const TRectangle *r) const { vdrawCircle(r->x,r->y,r->w,r->h); }
    void drawCircle(const TPoint &a, const TPoint &b) const { vdrawCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void drawCirclePC(int x,int y,int w,int h) const;
    void drawCirclePC(const TRectangle &r) const { drawCirclePC(r.x,r.y,r.w,r.h); }
    void drawCirclePC(const TRectangle *r) const { drawCirclePC(r->x,r->y,r->w,r->h); }
    void drawCirclePC(const TPoint &a, const TPoint &b) const { vdrawCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillCircle(int x,int y,int w,int h) const { vfillCircle(x, y, w, h); }
    void fillCircle(const TRectangle &r) const { vfillCircle(r.x,r.y,r.w,r.h); }
    void fillCircle(const TRectangle *r) const { vfillCircle(r->x,r->y,r->w,r->h); }
    void fillCircle(const TPoint &a, const TPoint &b) const { vfillCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    void fillCirclePC(int x,int y,int w,int h) const;
    void fillCirclePC(const TRectangle &r) const { fillCirclePC(r.x,r.y,r.w,r.h); }
    void fillCirclePC(const TRectangle *r) const { fillCirclePC(r->x,r->y,r->w,r->h); }
    void fillCirclePC(const TPoint &a, const TPoint &b) const { vfillCircle(a.x, a.y, b.x-a.x, b.y-a.y); }

    // arc
    //-----------------------
    virtual void vdrawArc(int x,int y,int w,int h, double r1, double r2) const = 0;
    virtual void vfillArc(int x,int y,int w,int h, double r1, double r2) const = 0;
    
    void drawArc(int x,int y,int w,int h, double r1, double r2) const { vdrawArc(x, y, w, h, r1, r2); }
    void drawArc(const TRectangle &r, double r1, double r2) const { vdrawArc(r.x,r.y,r.w,r.h, r1, r2); }
    void drawArc(const TRectangle *r, double r1, double r2) const { vdrawArc(r->x,r->y,r->w,r->h, r1, r2); }
    void drawArc(const TPoint &a, const TPoint &b, double r1, double r2) const { vdrawArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void drawArcPC(int x,int y,int w,int h, double r1, double r2) const;
    void drawArcPC(const TRectangle &r, double r1, double r2) const { drawArcPC(r.x,r.y,r.w,r.h, r1, r2); }
    void drawArcPC(const TRectangle *r, double r1, double r2) const { drawArcPC(r->x,r->y,r->w,r->h, r1, r2); }
    void drawArcPC(const TPoint &a, const TPoint &b, double r1, double r2) const { vdrawArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void fillArc(int x,int y,int w,int h, double r1, double r2) const { vfillArc(x, y, w, h, r1, r2); }
    void fillArc(const TRectangle &r, double r1, double r2) const { vfillArc(r.x,r.y,r.w,r.h, r1, r2); }
    void fillArc(const TRectangle *r, double r1, double r2) const { vfillArc(r->x,r->y,r->w,r->h, r1, r2); }
    void fillArc(const TPoint &a, const TPoint &b, double r1, double r2) const { vfillArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    void fillArcPC(int x,int y,int w,int h, double r1, double r2) const;
    void fillArcPC(const TRectangle &r, double r1, double r2) const { fillArcPC(r.x,r.y,r.w,r.h, r1, r2); }
    void fillArcPC(const TRectangle *r, double r1, double r2) const { fillArcPC(r->x,r->y,r->w,r->h, r1, r2); }
    void fillArcPC(const TPoint &a, const TPoint &b, double r1, double r2) const { vfillArc(a.x, a.y, b.x-a.x, b.y-a.y, r1, r2); }

    // polygon
    //-----------------------
    virtual void fillPolygon(const TPoint *points, int n) const = 0;
    virtual void drawPolygon(const TPoint *points, int n) const = 0;
    virtual void fillPolygon(const TPolygon &polygon) const = 0;
    virtual void drawPolygon(const TPolygon &polygon) const = 0;

    // bezier curves
    //-----------------------
    virtual void drawBezier(int x1,int y1, int x2,int y2, int x3,int y3, int x4,int y4) const = 0;
    virtual void drawBezier(double x1,double y1, double x2,double y2, double x3,double y3, double x4,double y4) const = 0;
    virtual void drawBezier(const TPoint *points) const { drawPolyBezier(points,4); }
    virtual void drawBezier(const TDPoint *points) const { drawPolyBezier(points,4); }

    virtual void drawPolyBezier(const TPoint *points, int n) const = 0;
    virtual void drawPolyBezier(const TPolygon &p) const = 0;
    virtual void drawPolyBezier(const TDPoint *points, int n) const = 0;
    virtual void drawPolyBezier(const TDPolygon &p) const = 0;

    virtual void fillPolyBezier(const TPoint *points, int n) const = 0;
    virtual void fillPolyBezier(const TPolygon &p) const = 0;
    virtual void fillPolyBezier(const TDPoint *points, int n) const = 0;
    virtual void fillPolyBezier(const TDPolygon &p) const = 0;

    static void poly2Bezier(const TPoint *src, int n, TPolygon &dst);
    static void poly2Bezier(const TPolygon &p, TPolygon &d);
    static void poly2Bezier(const TDPoint *src, int n, TDPolygon &dst);
    static void poly2Bezier(const TDPolygon &p, TDPolygon &d);

    // 3D rectangle
    //-----------------------
    virtual void vdraw3DRectangle(int x, int y, int w, int h, bool inset=true) const = 0;

    void draw3DRectangle(int x, int y, int w, int h, bool inset=true) const { vdraw3DRectangle(x,y,w,h,inset); }
    void draw3DRectangle(const TRectangle &r, bool inset=true) const { vdraw3DRectangle(r.x,r.y,r.w,r.h,inset); }
    void draw3DRectangle(const TRectangle *r, bool inset=true) const { vdraw3DRectangle(r->x,r->y,r->w,r->h,inset); }
    void draw3DRectangle(const TPoint &a, const TPoint &b, double r1, double r2, bool inset=true) const { vdraw3DRectangle(a.x, a.y, b.x-a.x, b.y-a.y, inset); }

    void draw3DRectanglePC(int x, int y, int w, int h, bool inset=true) const;
    void draw3DRectanglePC(const TRectangle &r, bool inset=true) const { draw3DRectanglePC(r.x,r.y,r.w,r.h,inset); }
    void draw3DRectanglePC(const TRectangle *r, bool inset=true) const { draw3DRectanglePC(r->x,r->y,r->w,r->h,inset); }
    void draw3DRectanglePC(const TPoint &a, const TPoint &b, double r1, double r2, bool inset=true) const { vdraw3DRectangle(a.x, a.y, b.x-a.x, b.y-a.y, inset); }

    // text string
    //-----------------------
    virtual int getTextWidth(const char* string) const = 0;
    virtual int getTextWidth(const char* string, int len) const = 0;
    virtual int getTextWidth(const string&) const = 0;
    virtual int getAscent() const = 0;
    virtual int getDescent() const = 0;
    virtual int getHeight() const = 0;
    virtual void drawString(int x, int y, const char*, int len) const = 0;
    virtual void drawString(int x, int y, const string&) const = 0;
    virtual void fillString(int x, int y, const char*, int len) const = 0;
    virtual void fillString(int x, int y, const string&) const = 0;
    virtual int drawTextWidth(int x, int y, const string &text, unsigned width) const = 0;
    // void drawTextAspect(int x,int y,const char* text,double xa,double ya);

    // bitmap
    //-----------------------
    virtual void drawBitmap(int x,int y, const TBitmap*) const = 0;
    virtual void drawBitmap(int x,int y, const TBitmap&) const = 0;
    virtual void drawBitmap(int,int,const TBitmap*, int,int,int,int) const = 0;
    virtual void drawBitmap(int,int,const TBitmap&, int,int,int,int) const = 0;

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

class TPen:
  public TPenBase
{
    friend class TWindow;
    friend class TBitmap;
    friend class TColor;

  public:
    TPen(TBitmap*);
    TPen(TWindow*);
    virtual ~TPen();
/*
    void translate(int dx,int dy);
*/
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

    void setBackColor(const TColor&);
    void setBackColor(byte r, byte g, byte b) { setBackColor(TColor(r,g,b)); }
    void setBackColor(const TRGB &c) { setBackColor(TColor(c.r,c.g,c.b)); }
    void setBackColor(TColor::ESystemColor c) { setBackColor(TColor(c)); }
    void setBackColor(TColor::EColor16 c) { setBackColor(TColor(c)); }

    // more parameters
    //-----------------------
    TFont* setFont(TFont*);
    void setMode(EPenMode);
    void setLineWidth(int);
    void setLineStyle(EPenLineStyle);
    void setColorMode(TColor::EDitherMode);
    void setClipChildren(bool);

    // clipping
    //-----------------------
    void setClipRegion(TRegion*);
    void setClipRect(const TRectangle&);
    
    void clrClipBox();
    void getClipBox(TRectangle*) const;

    void operator&=(TRectangle&);
    void operator|=(TRectangle&);

    // point
    //-----------------------
    void drawPoint(int x,int y) const;
    void drawPoint(const TPoint &p) const { drawPoint(p.x, p.y); }
    
    // line
    //-----------------------
    void vdrawLine(int x1,int y1,int x2,int y2) const;
    void drawLines(const TPoint *points, int n) const;
    void drawLines(const TPolygon&) const;

    // rectangle
    //-----------------------
    void vdrawRectangle(int x,int y,int w,int h) const;
    void vfillRectangle(int x,int y,int w,int h) const;

    // circle
    //-----------------------
    void vdrawCircle(int x,int y,int w,int h) const;
    void vfillCircle(int x,int y,int w,int h) const;

    // arc
    //-----------------------   
    void vdrawArc(int x,int y,int w,int h, double r1, double r2) const;
    void vfillArc(int x,int y,int w,int h, double r1, double r2) const;

    // polygon
    //-----------------------
    void fillPolygon(const TPoint *points, int n) const;
    void drawPolygon(const TPoint *points, int n) const;
    void fillPolygon(const TPolygon &polygon) const;
    void drawPolygon(const TPolygon &polygon) const;

    // bezier curves
    //-----------------------
    void drawBezier(int x1,int y1, int x2,int y2, int x3,int y3, int x4,int y4) const;
    void drawBezier(double x1,double y1, double x2,double y2, double x3,double y3, double x4,double y4) const;
    void drawBezier(const TPoint *points) const { drawPolyBezier(points,4); }
    void drawBezier(const TDPoint *points) const { drawPolyBezier(points,4); }

    void drawPolyBezier(const TPoint *points, int n) const ;
    void drawPolyBezier(const TPolygon &p) const; // { drawPolyBezier(p.begin(), p.size()); }
    void drawPolyBezier(const TDPoint *points, int n) const;
    void drawPolyBezier(const TDPolygon &p) const ; // { drawPolyBezier(p.begin(), p.size()); }

    void fillPolyBezier(const TPoint *points, int n) const;
    void fillPolyBezier(const TPolygon &p) const; // { fillPolyBezier(p.begin(), p.size()); }
    void fillPolyBezier(const TDPoint *points, int n) const;
    void fillPolyBezier(const TDPolygon &p) const; // { fillPolyBezier(p.begin(), p.size()); }

    // 3D rectangle
    //-----------------------
    void vdraw3DRectangle(int x, int y, int w, int h, bool inset=true) const;

    // text string
    //-----------------------
    int getTextWidth(const char* string) const;
    int getTextWidth(const char* string, int len) const;
    int getTextWidth(const string&) const;
    int getAscent() const;
    int getDescent() const;
    int getHeight() const;
    void drawString(int x, int y, const char*, int len) const;
    void drawString(int x, int y, const string&) const;
    void fillString(int x, int y, const char*, int len) const;
    void fillString(int x, int y, const string&) const;
    int drawTextWidth(int x, int y, const string &text, unsigned width) const;
    // void drawTextAspect(int x,int y,const char* text,double xa,double ya);

    // bitmap
    //-----------------------
    void drawBitmap(int x,int y, const TBitmap*) const;
    void drawBitmap(int x,int y, const TBitmap&) const;
    void drawBitmap(int,int,const TBitmap*, int,int,int,int) const;
    void drawBitmap(int,int,const TBitmap&, int,int,int,int) const;

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
      _pos.set(x4,y4);
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
    EPenLineStyle style;
    TWindow *wnd;
    TBitmap *bmp;
    TFont *font;
  public:
//#warning "'TRegion *region' is public"
    TRegion *region;
  private:
    void _setLineAttributes();
    void _init();
    
    _TOAD_GC o_gc, f_gc;
    _TOAD_DRAWABLE x11drawable; // either window or pixmap
};

} // namespace toad

#endif
