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

#ifndef _TOAD_FIGURE_HH
#define _TOAD_FIGURE_HH

#include <toad/toad.hh>
#include <toad/figurewindow.hh>
#include <toad/io/serializable.hh>

namespace toad {

class TFigureEditor;
class TMatrix2D;

/**
 * \ingroup figure
 */
class TFigure:
  public TSerializable
{
  public:
    TFigure();
    virtual ~TFigure();
    enum EPaintType {
      NORMAL,
      SELECT,
      EDIT
    };
    
    //! Called to paint the gadget.
    virtual void paint(TPenBase& pen, EPaintType) = 0;
    virtual void paintSelection(TPenBase &pen);

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
    virtual void getShape(TRectangle&) = 0;

    TRGB line_color;
    TRGB fill_color;
    bool filled:1;        // true when filled
    
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
    virtual bool startInPlace() { return false; }
    virtual unsigned stop(TFigureEditor*) { return NOTHING; };
    virtual unsigned keyDown(TFigureEditor*, TKey, char*, unsigned) { return CONTINUE; }

    // editor related stuff for manipulation & creation
    //--------------------------------------------------
    virtual void startCreate() {};
    virtual unsigned mouseLDown(TFigureEditor*, int, int, unsigned) { return STOP; }
    virtual unsigned mouseMove(TFigureEditor*, int, int, unsigned) { return CONTINUE; }
    virtual unsigned mouseLUp(TFigureEditor*, int, int, unsigned) { return CONTINUE; }

    // editor related stuff for all gadgets
    //--------------------------------------
    static double distance2Line(int x, int y, int x1, int y1, int x2, int y2);

    static const double OUT_OF_RANGE = 100.0;
    static const double RANGE = 5.0;
    static const double INSIDE = 2.5;

    // storage stuff
    //--------------------------------------
    const char * name() const { return "toad::TFigure"; }
    void store (TOutObjectStream&) const;
    bool restore(TInObjectStream&);

    // storage stuff for all gadgets
    //-------------------------------------- 
    static void initStorage();
    static void loseStorage();
    static TInObjectStream serialize;
};

/**
 * \ingroup figure
 */
class TFRectangle:
  public TFigure
{
    typedef TFigure super;
  public:
    TFRectangle() {
      filled = false;
      line_color.set(0,0,0);
      fill_color.set(0,0,0);
    }
    TFRectangle(TFigureWindow *parent, int x,int y,int w, int h) {
      parent->addGadget(this);
      setShape(x, y, w, h);
      filled = false;
    };
    void setShape(int x, int y, int w, int h) {
      p1.x = x;
      p1.y = y;
      p2.x = x+w-1;
      p2.y = y+h-1;
    }
    void paint(TPenBase &, EPaintType);
    void getShape(TRectangle&);

    double distance(int x, int y);
    void translate(int dx, int dy);
    bool getHandle(unsigned n, TPoint &p);
    void translateHandle(unsigned handle, int mx, int my);

    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    unsigned mouseMove(TFigureEditor*, int, int, unsigned);
    unsigned mouseLUp(TFigureEditor*, int, int, unsigned);
    
    TCloneable* clone() const { return new TFRectangle(*this); }

    const char * name() const { return "toad::TFRectangle"; } 
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
    
    TPoint p1, p2;
};

/**
 * \ingroup figure
 */
class TFPolygon:
  public TFigure
{
    typedef TFigure super;
  public:
    TFPolygon() {
      filled = false;
      line_color.set(0,0,0);
      fill_color.set(0,0,0);
    }
    
    // polygon creation
    unsigned mouseLDown(TFigureEditor*, int, int, unsigned);
    unsigned mouseMove(TFigureEditor*, int, int, unsigned);

    void paint(TPenBase &, EPaintType);
    double distance(int x, int y);
    void getShape(TRectangle&);
    void translate(int dx, int dy);
    bool getHandle(unsigned n, TPoint &p);
    void translateHandle(unsigned handle, int mx, int my);
    
    TCloneable* clone() const { return new TFPolygon(*this); }
    const char * name() const { return "toad::TFPolygon"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
  protected:
    typedef vector<TPoint> TPoints;
    TPolygon polygon;
};

/**
 * \ingroup figure
 */
class TFPolyline:
  public TFPolygon
{
  public:
    void paint(TPenBase &, EPaintType);
    double distance(int x, int y);
    TCloneable* clone() const { return new TFPolyline(*this); }
    const char * name() const { return "toad::TFPolyline"; }
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

    void paint(TPenBase &, EPaintType);
    double distance(int x, int y);
    
    TCloneable* clone() const { return new TFBezierline(*this); }
    const char * name() const { return "toad::TFBezierline"; }
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
    
    TCloneable* clone() const { return new TFBezier(*this); }
    const char * name() const { return "toad::TFBezier"; }
};

/**
 * \ingroup figure
 */
class TFCircle:
  public TFRectangle
{
  public:
    TFCircle(){}
    TFCircle(TFigureWindow *parent, int x, int y, int w, int h):
      TFRectangle(parent,x,y,w,h) {}
    void paint(TPenBase &, EPaintType);
    
    double distance(int x, int y);
    
    TCloneable* clone() const { return new TFCircle(*this); }
    const char * name() const { return "toad::TFCircle"; } 
};

/**
 * \ingroup figure
 */
class TFText:
  public TFRectangle
{
    typedef TFRectangle super;
  public:
    TFText() {}
    TFText(TFigureWindow *parent, int x,int y, const string &text) {
      parent->addGadget(this);
      p1.x = x;
      p1.y = y;
      this->text = text;
      calcSize();
    }
    void setText(const string &t) {
      text = t;
      calcSize();
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

    const char * name() const { return "toad::TFText"; } 
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
    
  protected:
    string text;
    virtual void calcSize();
    static int cx;  // cursor position while editing
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
    TFFrame(TFigureWindow *parent, int x,int y,int w, int h, const string &text="") {
      parent->addGadget(this);
      this->text = text;
      setShape(x,y,w,h);
    };
    void paint(TPenBase &, EPaintType);

    void getShape(TRectangle&);
    double distance(int x, int y);
    unsigned stop(TFigureEditor*);
    unsigned keyDown(TFigureEditor*, TKey, char*, unsigned);
    bool getHandle(unsigned n, TPoint &p);
    unsigned mouseLDown(TFigureEditor *e, int x, int y, unsigned m);
    unsigned mouseMove(TFigureEditor *e, int x, int y, unsigned m);
    unsigned mouseLUp(TFigureEditor *e, int x, int y, unsigned m);
    
    TCloneable* clone() const { return new TFFrame(*this); }
    const char * name() const { return "toad::TFFrame"; } 

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
    const char * name() const { return "toad::TFWindow"; }
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
    TFGroup() {
    }
    TFGroup(const TFGroup &g) {
#if 0
      gadgets.insert(gadgets.begin(), 
                     g.gadgets.begin(), 
                     g.gadgets.end());
#endif
    }
    ~TFGroup();
    void paint(TPenBase&, EPaintType);
    double distance(int x, int y);
    void translate(int dx, int dy);
    bool getHandle(unsigned n, TPoint &p);
    
    void calcSize();

    TFigureModel gadgets;

    TCloneable* clone() const { return new TFGroup(*this); }
    const char * name() const { return "toad::TFGroup"; }
    void store(TOutObjectStream&) const;
    bool restore(TInObjectStream&);
};

} // namespace toad

#endif
