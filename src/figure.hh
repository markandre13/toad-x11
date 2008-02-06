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

#ifndef _TOAD_FIGURE_HH
#define _TOAD_FIGURE_HH 1

#include <math.h>
#include <toad/toad.hh>
#include <toad/bitmap.hh>
#include <toad/figuremodel.hh>
#include <toad/io/serializable.hh>

namespace toad {

class TFigureEditor;
class TFigureAttributes;
class TMatrix2D;

class TFigureEditEvent
{
  public:
    enum EType {
      PAINT_NORMAL,
      PAINT_SELECT,
      PAINT_EDIT,
      PAINT_SELECTION,
    
      //! figure was added to model
      ADDED,
      //! figure was removed from model
      REMOVED,
    
      GET_DISTANCE,
      GET_HANDLE,
      GET_SHAPE,
      BEGIN_TRANSLATE,
      TRANSLATE,
      END_TRANSLATE,
      TRANSLATE_HANDLE,
      START_IN_PLACE,
      STOP_EDIT,
      KEY_EVENT,
      MOUSE_EVENT,
      
    } type;

    TFigureEditor *editor; // in:START_IN_PLACE
    TFigureModel *model; // in
    
    TPenBase *pen;    // in
    
    TRectangle shape; // out:GET_SHAPE

    TCoord x, y;         // in:GET_DISTANCE, in:GET_HANDLE, in:TRANSLATE, 
                      // in:TRANSLATE_HANDLE
    TCoord distance;  // out:GET_DISTANCE

    unsigned handle;  // out:GET_HANDLE, in:TRANSLATE_HANDLE, in:PAINT_SELECTION
    
    TKeyEvent key;
    TMouseEvent mouse;
};

/**
 * \ingroup figure
 *
 */
class TFigure:
  public TSerializable
{
  public:
    TFigure();
    TFigure(const TFigure &);
    virtual ~TFigure();

    virtual bool editEvent(TFigureEditEvent &ee);
    
    enum EPaintType {
      NORMAL,
      SELECT,
      EDIT,
      OUTLINE
    };
    virtual void setAttributes(const TFigureAttributes*);
    virtual void getAttributes(TFigureAttributes*) const;
    
    //! Called to paint the gadget.
    virtual void paint(TPenBase& pen, EPaintType type = NORMAL) = 0;
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
    
    /**
     * Coordinate transformation matrix
     */
    TMatrix2D *cmat;
  
    // editor related stuff per gadget for manipulation
    //-------------------------------------------------
    static const unsigned NOTHING  = 0;
    static const unsigned CONTINUE = 1; // continue editing
    static const unsigned STOP     = 2; // stop editing
    static const unsigned REPEAT   = 4; // repeat the last event
    static const unsigned DELETE   = 8; // delete this object
    static const unsigned NOGRAB   = 16; // don't grab

    // stage 1: select:
    virtual TCoord _distance(TFigureEditor *fe, TCoord x, TCoord y);
    virtual TCoord distance(TCoord x, TCoord y);
    
    // stage 2: move
    virtual void translate(TCoord dx, TCoord dy);
    
    // stage 3: manipulate
    static const int NO_HANDLE = -1;
    virtual bool getHandle(unsigned n, TPoint *p);
    virtual bool startTranslateHandle();
    virtual void translateHandle(unsigned handle, TCoord x, TCoord y, unsigned modifier);
    virtual void endTranslateHandle();

    // stage 4: in place editing
    //! Return `true' when in-place editing is desired.
    virtual bool startInPlace();
    virtual unsigned stop(TFigureEditor*);
    virtual unsigned keyDown(TFigureEditor*, const TKeyEvent&);

    // editor related stuff for manipulation & creation
    //--------------------------------------------------
    virtual void startCreate();
    virtual unsigned mouseLDown(TFigureEditor*, const TMouseEvent&);
    virtual unsigned mouseMove(TFigureEditor*, const TMouseEvent&);
    virtual unsigned mouseLUp(TFigureEditor*, const TMouseEvent&);
    virtual unsigned mouseRDown(TFigureEditor*, const TMouseEvent&);

    // editor related stuff for all gadgets
    //--------------------------------------
    static TCoord distance2Line(TCoord x, TCoord y, TCoord x1, TCoord y1, TCoord x2, TCoord y2);

#if 0
    static const TCoord OUT_OF_RANGE = HUGE_VAL;
    static const TCoord RANGE = 5.0;
    static const TCoord INSIDE = -1.0;
#else
    static TCoord OUT_OF_RANGE;
    static TCoord RANGE;
    static TCoord INSIDE;
#endif
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
    byte alpha;
  public:
    bool filled:1;        // true when filled
    bool closed:1;        // true when closed
  protected:
    TPen::ELineStyle line_style;
    unsigned line_width;

    void store(TOutObjectStream &out) const;
    bool restore(TInObjectStream &in);

  public:
    virtual void setAttributes(const TFigureAttributes*);
    virtual void getAttributes(TFigureAttributes*) const;

    void setColor(const TRGB &color) {
      line_color = fill_color = color;
    }
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
    bool isFilled() const { return filled && closed; }
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
    TFRectangle(TCoord x,TCoord y,TCoord w, TCoord h) {
      setShape(x, y, w, h);
    };
    void setShape(TCoord x, TCoord y, TCoord w, TCoord h) {
      p1.x = x;
      p1.y = y;
      p2.x = x+w-1;
      p2.y = y+h-1;
    }
    void paint(TPenBase &, EPaintType);
    void getShape(TRectangle*);

    double distance(TCoord x, TCoord y);
    void translate(TCoord dx, TCoord dy);
    bool getHandle(unsigned n, TPoint *p);
    void translateHandle(unsigned handle, TCoord mx, TCoord my, unsigned);

    SERIALIZABLE_INTERFACE(toad::, TFRectangle);    
    
  protected:
    TPoint p1, p2;

    unsigned mouseLDown(TFigureEditor*, const TMouseEvent&);
    unsigned mouseMove(TFigureEditor*, const TMouseEvent&);
    unsigned mouseLUp(TFigureEditor*, const TMouseEvent&);
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
    double distance(TCoord x, TCoord y);
    void getShape(TRectangle*);
    void translate(TCoord dx, TCoord dy);
    bool getHandle(unsigned n, TPoint *p);
    void translateHandle(unsigned handle, TCoord mx, TCoord my, unsigned);
    TPolygon polygon;
    
    SERIALIZABLE_INTERFACE(toad::, TFPolygon);
  protected:
    // polygon creation
    unsigned mouseLDown(TFigureEditor*, const TMouseEvent&);
    unsigned mouseMove(TFigureEditor*, const TMouseEvent&);
    unsigned keyDown(TFigureEditor *editor, const TKeyEvent&);
    unsigned mouseRDown(TFigureEditor *editor, const TMouseEvent&);
    virtual void _insertPointNear(TCoord, TCoord, bool filled);
  public:
    virtual void insertPointNear(TCoord, TCoord);
    virtual void deletePoint(unsigned);
    void addPoint(const TPoint &p) { polygon.addPoint(p); }
    void addPoint(TCoord x, TCoord y) { polygon.addPoint(x,y); }
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
    void setAttributes(const TFigureAttributes*);
    void getAttributes(TFigureAttributes*) const;
    void paint(TPenBase &, EPaintType);
    double distance(TCoord x, TCoord y);

    static void drawArrow(TPenBase &pen,
                          const TPoint &p1, const TPoint &p1, 
                          const TRGB &line, const TRGB &fill,
                          TCoord w, TCoord h,
                          EArrowType type);
    
    virtual void insertPointNear(TCoord, TCoord);
  protected:
    unsigned mouseLDown(TFigureEditor*, const TMouseEvent&);
    SERIALIZABLE_INTERFACE(toad::, TFLine);
};

/**
 * \ingroup figure
 */
class TFBezierline:
  public TFLine
{
  protected:
    void paintSelectionLines(TPenBase &pen);
  public:
    unsigned mouseLDown(TFigureEditor*, const TMouseEvent&);
    unsigned mouseLUp(TFigureEditor*, const TMouseEvent&);
    unsigned mouseMove(TFigureEditor*, const TMouseEvent&);

    void insertPointNear(TCoord x, TCoord y);
    void deletePoint(unsigned i);

    void paint(TPenBase &, EPaintType);
    void paintSelection(TPenBase &pen, int handle);
    void _paintSelection(TPenBase &pen, int handle, bool filled);
    double _distance(TFigureEditor *fe, TCoord x, TCoord y);
    void translateHandle(unsigned handle, TCoord mx, TCoord my, unsigned);
    void _translateHandle(unsigned handle, TCoord mx, TCoord my, unsigned, bool filled);
    unsigned mouseRDown(TFigureEditor*, const TMouseEvent&);
    
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
    unsigned mouseLDown(TFigureEditor*, const TMouseEvent&);

    void paint(TPenBase &, EPaintType);
    void paintSelection(TPenBase &pen, int handle);
    double _distance(TFigureEditor *fe, TCoord x, TCoord y);
    void translateHandle(unsigned handle, TCoord x, TCoord y, unsigned);
    void setAttributes(const TFigureAttributes*);
    
    TCloneable* clone() const { return new TFBezier(*this); }
    const char * getClassName() const { return "toad::TFBezier"; }
    void store(TOutObjectStream &out) const;
};

/**
 * \ingroup figure
 */
class TFCircle:
  public TFRectangle
{
  public:
    TFCircle(){}
    TFCircle(TCoord x, TCoord y, TCoord w, TCoord h):
      TFRectangle(x,y,w,h) {}
    void paint(TPenBase &, EPaintType);
    
    double distance(TCoord x, TCoord y);
    
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
    TFText(TCoord x, TCoord y, const string &text) {
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

    void setAttributes(const TFigureAttributes*);
    void getAttributes(TFigureAttributes*) const;

    void paint(TPenBase &, EPaintType);

    double distance(TCoord x, TCoord y);
    bool getHandle(unsigned n, TPoint *p);

    bool startInPlace();
    void startCreate();
    unsigned stop(TFigureEditor*);

    unsigned keyDown(TFigureEditor*, const TKeyEvent&);
    unsigned mouseLDown(TFigureEditor*, const TMouseEvent&);
    unsigned mouseMove(TFigureEditor*, const TMouseEvent&);
    unsigned mouseLUp(TFigureEditor*, const TMouseEvent&);

    TCloneable* clone() const { return new TFText(*this); }

    const char * getClassName() const { return "toad::TFText"; } 
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
    
  protected:
    string text;
    string fontname;
    virtual void calcSize();
    static size_t cx;  // cursor position while editing
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
    TFFrame(TCoord x,TCoord y,TCoord w, TCoord h, const string &text="") {
      this->text = text;
      setShape(x,y,w,h);
    };
    void paint(TPenBase &, EPaintType);

    void getShape(TRectangle*);
    double distance(TCoord x, TCoord y);
    unsigned stop(TFigureEditor*);
    unsigned keyDown(TFigureEditor*, const TKeyEvent&);
    bool getHandle(unsigned n, TPoint *p);
    unsigned mouseLDown(TFigureEditor *e, const TMouseEvent&);
    unsigned mouseMove(TFigureEditor *e, const TMouseEvent&);
    unsigned mouseLUp(TFigureEditor *e, const TMouseEvent&);
    
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
    ~TFWindow();

    void paint(TPenBase&, EPaintType);
    double distance(TCoord x, TCoord y);
    void translate(TCoord dx, TCoord dy);
    void translateHandle(unsigned handle, TCoord x, TCoord y, unsigned);
    
    TCloneable* clone() const { return new TFWindow(*this); }
    const char * getClassName() const { return "toad::TFWindow"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
    
    string title;
    string label;
    string tooltip;
    string widget;
    string model;
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
    double _distance(TFigureEditor *fe, TCoord x, TCoord y);
    bool getHandle(unsigned n, TPoint *p);
    bool startTranslateHandle();
    void translateHandle(unsigned handle, TCoord dx, TCoord dy, unsigned);
    void endTranslateHandle();
    
    void drop() {
      gadgets.drop();
    }
    
    void calcSize();

    TFigureModel gadgets;

    void translate(TCoord dx, TCoord dy);
    bool editEvent(TFigureEditEvent &ee);

    TCloneable* clone() const { return new TFGroup(*this); }
    const char * getClassName() const { return "toad::TFGroup"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);

  protected:
    void modelChanged();
};

class TFImage:
  public TFigure
{
    typedef TFigure super;
  protected:
    string filename;
    PBitmap bitmap;
    int x, y;
  public:
    TFImage();
    TFImage(const string &filename);

    void paint(TPenBase &, EPaintType);
    void getShape(TRectangle*);
    
    bool editEvent(TFigureEditEvent &ee);

    TCloneable* clone() const { return new TFImage(*this); }
    const char * getClassName() const { return "toad::TFImage"; } 
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
};

} // namespace toad

#endif
