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
#define __TOAD_PEN_HH 1

#include <toad/penbase.hh>
#include <toad/config.h>

#ifdef __COCOA__
#import <Cocoa/Cocoa.h>
#endif

#ifdef HAVE_LIBXFT
typedef struct _XftDraw XftDraw;
#endif

namespace toad {

class TRegion;

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

#ifdef __COCOA__
    TWindow *window;
    NSBezierPath *clipPath;
    typedef vector<NSAffineTransform*> mstack_t;
    mstack_t mstack;
#endif

  public:
    TMatrix2D *mat;

    TPen(TBitmap*);
    TPen(TWindow*);
    virtual ~TPen();

    void identity();
    void translate(TCoord dx, TCoord dy);
    void scale(TCoord dx, TCoord dy);
    void rotate(TCoord radiants);
    void multiply(const TMatrix2D*);
    void setMatrix(TCoord a11, TCoord a21, TCoord a12, TCoord a22, TCoord tx, TCoord ty);
    void push();
    void pop();
    void popAll();
    TMatrix2D *getMatrix() const { return mat; }

    // color & pattern
    //-----------------------
    void setBitmap(TBitmap*);

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
    void setLineWidth(TCoord);
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

    void vsetColor(TCoord r, TCoord g, TCoord b);
    void vsetLineColor(TCoord r, TCoord g, TCoord b);
    void vsetFillColor(TCoord r, TCoord g, TCoord b);
    void setAlpha(TCoord a) {}
    TCoord getAlpha() const { return 0; }
    void vdrawRectangle(TCoord x, TCoord y, TCoord w, TCoord h);
    void vfillRectangle(TCoord x, TCoord y, TCoord w, TCoord h);
    void vdrawCircle(TCoord x,TCoord y,TCoord w,TCoord h);
    void vfillCircle(TCoord x,TCoord y,TCoord w,TCoord h);
    void vdrawArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2);
    void vfillArc(TCoord x, TCoord y, TCoord w, TCoord h, TCoord r1, TCoord r2);
    void vdrawBitmap(TCoord x, TCoord y, const TBitmap&);
    void vdrawString(TCoord x, TCoord y, const char *str, size_t len, bool transparent);

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
