/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_FIGURE_HH
#define _TOAD_FIGURE_HH

#include <math.h>
#include <toad/toad.hh>
#include <toad/figuremodel.hh>
#include <toad/io/serializable.hh>

namespace toad {

class TFigureEditor;
class TFigurePreferences;
class TMatrix2D;

/**
 * \ingroup figure
 *
 * To be renamed into 'TPlainFigure' or 'TVerbatimFigure' or ...
 */
class TFigure:
  public TSerializable
{
  public:
    TFigure();
    TFigure(const TFigure &);
    virtual ~TFigure();
    enum EPaintType {
      NORMAL,
      SELECT,
      EDIT
    };
    virtual void setFromPreferences(TFigurePreferences*);
    
    //! Called to paint the gadget.
    virtual void paint(TPenBase& pen, EPaintType) = 0;
    virtual void paintSelection(TPenBase &pen, int handle);

    /**
     * Called to get the gadgets bounding rectangle.
     *
     * Used to
     * \li
     *    invalidate the area the figure occupies to redraw the figure with
     *    TFigureEditor::invalidateFigure
     * \li
     *    calculate the size of the area all gadgets occupy and to set up the
     *    scrolling area accordingly and
     * \li
     *    paint a marker with the default implementation of 'paintSelection'
     */
    virtual void getShape(TRectangle*) = 0;

  public:    
    /**
     * 'true' when TFigureEditor is allowed to delete this object.
     */
    bool removeable:1;
    
    /**
     * Transformation matrix to be applied before this object is painted
     * or 0.
     */
    TMatrix2D *mat;
  
    // editor related stuff per gadget for manipulation
    //-------------------------------------------------
    static const unsigned NOTHING  = 0;
    static const unsigned CONTINUE = 1; // continue editing
    static const unsigned STOP     = 2; // stop editing
    static const unsigned REPEAT   = 4; // repeat the last event
    static const unsigned DELETE   = 8; // delete this object

    // stage 1: select:
    virtual double distance(int x, int y) = 0;
    
    // stage 2: move
    virtual void translate(int dx, int dy) = 0 ;
    
    // stage 3: manipulate
    static const int NO_HANDLE = -1;
    virtual bool getHandle(unsigned n, TPoint &p);
    virtual void translateHandle(unsigned handle, int x, int y);

    // stage 4: in place editing
    //! Return `true' when in-place editing is desired.
    virtual bool startInPlace();
    virtual unsigned stop(TFigureEditor*);
    virtual unsigned keyDown(TFigureEditor*, TKey, char*, unsigned);

    // editor related stuff for manipulation & creation
    //--------------------------------------------------
    virtual void startCreate();
    virtual unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    virtual unsigned mouseMove(TFigureEditor*, int, int, unsigned);
    virtual unsigned mouseLUp(TFigureEditor*, int, int, unsigned);
    virtual unsigned mouseRDown(TFigureEditor*, int, int, unsigned);

    // editor related stuff for all gadgets
    //--------------------------------------
    static double distance2Line(int x, int y, int x1, int y1, int x2, int y2);

    static const double OUT_OF_RANGE = HUGE_VAL;
    static const double RANGE = 5.0;
    static const double INSIDE = -1.0;

    // storage stuff for all gadgets
    //-------------------------------------- 
    static void initialize();
    static void terminate();
    static TInObjectStream serialize;

//    SERIALIZABLE_INTERFACE(toad::, TFigure);
    void store(TOutObjectStream &out) const;
    bool restore(TInObjectStream &in);
};

//! To be renamed into 'TFigure'
//! what about *mat ?
class TColoredFigure:
  public TFigure
{
    typedef TFigure super;
  protected:
    TColoredFigure();
    TSerializableRGB line_color;
    TSerializableRGB fill_color;
    bool filled:1;        // true when filled
    TPen::ELineStyle line_style;
    unsigned line_width;

    void store(TOutObjectStream &out) const;
    bool restore(TInObjectStream &in);

  public:
    virtual void setFromPreferences(TFigurePreferences*);

    void setLineColor(const TRGB &color) {
      line_color = color;
    }
    void setFillColor(const TRGB &color) {
      fill_color = color;
      filled = true;
    }
    void unsetFillColor() {
      filled = false;
    }
//    virtual void setFont(const string&);
};


/**
 * \ingroup figure
 */
class TFRectangle:
  public TColoredFigure
{
    typedef TColoredFigure super;
  public:
    TFRectangle() {}
    TFRectangle(int x,int y,int w, int h) {
      setShape(x, y, w, h);
    };
    void setShape(int x, int y, int w, int h) {
      p1.x = x;
      p1.y = y;
      p2.x = x+w-1;
      p2.y = y+h-1;
    }
    void paint(TPenBase &, EPaintType);
    void getShape(TRectangle*);

    double distance(int x, int y);
    void translate(int dx, int dy);
    bool getHandle(unsigned n, TPoint &p);
    void translateHandle(unsigned handle, int mx, int my);

    SERIALIZABLE_INTERFACE(toad::, TFRectangle);    
    
  protected:
    TPoint p1, p2;

    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    unsigned mouseMove(TFigureEditor*, int, int, unsigned);
    unsigned mouseLUp(TFigureEditor*, int, int, unsigned);
};

/**
 * \ingroup figure
 */
class TFPolygon:
  public TColoredFigure
{
    typedef TColoredFigure super;
  public:
    void paint(TPenBase &, EPaintType);
    double distance(int x, int y);
    void getShape(TRectangle*);
    void translate(int dx, int dy);
    bool getHandle(unsigned n, TPoint &p);
    void translateHandle(unsigned handle, int mx, int my);
    
    SERIALIZABLE_INTERFACE(toad::, TFPolygon);
  protected:
    TPolygon polygon;

    // polygon creation
    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    unsigned mouseMove(TFigureEditor*, int, int, unsigned);
};

/**
 * \ingroup figure
 */
class TFLine:
  public TFPolygon
{
    typedef TFPolygon super;
  public:
    enum EArrowMode {
      NONE, HEAD, TAIL, BOTH
    } arrowmode;
    
    enum EArrowType {
      SIMPLE,
      EMPTY,
      FILLED,
      EMPTY_CONCAVE,
      FILLED_CONCAVE,
      EMPTY_CONVEX,
      FILLED_CONVEX
    } arrowtype;
    
    unsigned arrowheight;
    unsigned arrowwidth;

    TFLine();
    void setFromPreferences(TFigurePreferences*);
    void paint(TPenBase &, EPaintType);
    double distance(int x, int y);

    static void drawArrow(TPenBase &pen,
                          const TPoint &p1, const TPoint &p1, 
                          const TRGB &line, const TRGB &fill,
                          int w, int h,
                          EArrowType type);
    
  protected:
    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    SERIALIZABLE_INTERFACE(toad::, TFLine);
};

class TFPolyline:
  public TFLine
{
  public:
    const char * getClassName() const { return "toad::TFPolyline"; }
};

/**
 * \ingroup figure
 */
class TFBezierline:
  public TFPolygon
{
  public:
    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    unsigned mouseMove(TFigureEditor*, int, int, unsigned);

    void insertPointNear(int x, int y);

    void paint(TPenBase &, EPaintType);
    void paintSelection(TPenBase &pen, int handle);
    double distance(int x, int y);
    void translateHandle(unsigned handle, int mx, int my);
    unsigned mouseRDown(TFigureEditor*, int, int, unsigned);
    
    TCloneable* clone() const { return new TFBezierline(*this); }
    const char * getClassName() const { return "toad::TFBezierline"; }
};

/**
 * \ingroup figure
 */
class TFBezier:
  public TFBezierline
{
    typedef TFBezier super;
  public:
    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);

    void paint(TPenBase &, EPaintType);
    double distance(int x, int y);
    void translateHandle(unsigned handle, int x, int y);
    
    TCloneable* clone() const { return new TFBezier(*this); }
    const char * getClassName() const { return "toad::TFBezier"; }
};

/**
 * \ingroup figure
 */
class TFCircle:
  public TFRectangle
{
  public:
    TFCircle(){}
    TFCircle(int x, int y, int w, int h):
      TFRectangle(x,y,w,h) {}
    void paint(TPenBase &, EPaintType);
    
    double distance(int x, int y);
    
    TCloneable* clone() const { return new TFCircle(*this); }
    const char * getClassName() const { return "toad::TFCircle"; } 
};

/**
 * \ingroup figure
 */
class TFText:
  public TFRectangle
{
    typedef TFRectangle super;
  public:
    TFText() {
      p1.x = p1.y = 0;
      fontname = "arial,helvetica,sans-serif:size=12";
    }
    TFText(int x,int y, const string &text) {
      p1.x = x;
      p1.y = y;
      fontname = "arial,helvetica,sans-serif:size=12";
      this->text = text;
      calcSize();
    }
    void setText(const string &t) {
      text = t;
      calcSize();
    }
    void setFont(const string &fontname) {
      this->fontname = fontname;
    }
    void paint(TPenBase &, EPaintType);

    double distance(int x, int y);
    bool getHandle(unsigned n, TPoint &p);

    bool startInPlace();
    void startCreate();
    unsigned stop(TFigureEditor*);

    unsigned keyDown(TFigureEditor*, TKey, char*, unsigned);
    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    unsigned mouseMove(TFigureEditor*, int, int, unsigned);
    unsigned mouseLUp(TFigureEditor*, int, int, unsigned);

    TCloneable* clone() const { return new TFText(*this); }

    const char * getClassName() const { return "toad::TFText"; } 
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
    
  protected:
    string text;
    string fontname;
    virtual void calcSize();
    static int cx;  // cursor position while editing
    void setFromPreferences(TFigurePreferences*);
};

/**
 * \ingroup figure
 */
class TFFrame:
  public TFText
{
    typedef TFText super;
  public:
    TFFrame() {}
    TFFrame(int x,int y,int w, int h, const string &text="") {
      this->text = text;
      setShape(x,y,w,h);
    };
    void paint(TPenBase &, EPaintType);

    void getShape(TRectangle*);
    double distance(int x, int y);
    unsigned stop(TFigureEditor*);
    unsigned keyDown(TFigureEditor*, TKey, char*, unsigned);
    bool getHandle(unsigned n, TPoint &p);
    unsigned mouseLDown(TFigureEditor *e, int x, int y, unsigned m);
    unsigned mouseMove(TFigureEditor *e, int x, int y, unsigned m);
    unsigned mouseLUp(TFigureEditor *e, int x, int y, unsigned m);
    
    TCloneable* clone() const { return new TFFrame(*this); }
    const char * getClassName() const { return "toad::TFFrame"; } 

    void calcSize();
};

/**
 * \ingroup figure
 */
class TFWindow:
  public TFRectangle
{
    typedef TFRectangle super;
  public:
    TFWindow();

    void paint(TPenBase&, EPaintType);
    double distance(int x, int y);
    void translate(int dx, int dy);
    void translateHandle(unsigned handle, int x, int y);
    
    TCloneable* clone() const { return new TFWindow(*this); }
    const char * getClassName() const { return "toad::TFWindow"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
    
    string title;
    string label;
    unsigned taborder;
    TWindow *window;
};

/**
 * \ingroup figure
 */
class TFGroup:
  public TFRectangle
{
    typedef TFRectangle super;
  public:
    TFGroup();
    TFGroup(const TFGroup &g);
    ~TFGroup();
    void paint(TPenBase&, EPaintType);
    double distance(int x, int y);
    void translate(int dx, int dy);
    bool getHandle(unsigned n, TPoint &p);
    void translateHandle(unsigned handle, int dx, int dy);
    
    void drop() {
      gadgets.drop();
    }
    
    void calcSize();

    TFigureModel gadgets;

    TCloneable* clone() const { return new TFGroup(*this); }
    const char * getClassName() const { return "toad::TFGroup"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);

  protected:
    void modelChanged();
};

class TFImage:
  public TFRectangle
{
    string filename;
    PBitmap bitmap;
    typedef TFRectangle super;
  public:
    TFImage() {}
    bool startInPlace();
    void paint(TPenBase &, EPaintType);
    TCloneable* clone() const { return new TFImage(*this); }
    const char * getClassName() const { return "toad::TFImage"; } 
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
};

} // namespace toad

#endif
