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

#ifndef _TOAD_FIGUREEDITOR
#define _TOAD_FIGUREEDITOR

#include <toad/figure.hh>
#include <toad/figuremodel.hh>
#include <toad/scrollpane.hh>
#include <toad/undo.hh>

#include <toad/boolmodel.hh>
#include <toad/textmodel.hh>
#include <toad/boundedrangemodel.hh>


namespace toad {

// class TFigure;
class TFigureEditor;
class TScrollBar;

class TFigureEditorHeaderRenderer
{
  public:
    virtual void render(TPen &pen, int pos, int size, TMatrix2D *mat) = 0;
    virtual int getSize() = 0;
    virtual void mouseEvent(TMouseEvent&);
};

class TFigurePreferences:
  public TModel
{
    TFigureEditor *current;
  public:
    TFigurePreferences();
    ~TFigurePreferences();

    void setCurrent(TFigureEditor *current) {
      if (this->current == current)
        return;
      this->current = current;
      reason = CURRENTCHANGED;
      sigChanged();
    }
    
    TFigureEditor* getCurrent() const {
      return current;
    }

    // These methods delegate to the current TFigureEditor.
    void setOperation(unsigned);
    void setCreate(TFigure*);
    void group();
    void ungroup();
    void selectionDown();
    void selection2Bottom();
    void selectionUp();
    void selection2Top();
    
    // additional information on why sigChanged was triggered
    enum EReason {
      /**
       * All parameters have changed or, apply all parameters.
       */
      ALLCHANGED,
      /**
       * Where going to edit another object.
       */
      CURRENTCHANGED,
      /**
       * Drawing properties have changed.
       */
      LINECOLOR,
      FILLCOLOR,
      UNSETFILLCOLOR,
      LINEWIDTH,
      LINESTYLE,
      ARROWMODE,
      ARROWSTYLE,
      FONTNAME,
      /**
       * This TPreferences was subclassed. Try dynamic_cast to get
       * more information.
       */
      EXTENDED
    } reason;
    
    void setLineColor(const TRGB &rgb) { 
      linecolor = rgb;
      reason = LINECOLOR;
      sigChanged();
    }
    void setFillColor(const TRGB &rgb) { 
      fillcolor = rgb; 
      filled = true; 
      reason = FILLCOLOR;
      sigChanged();
    }
    void unsetFillColor() { 
      filled = false; 
      reason = UNSETFILLCOLOR;
      sigChanged();
    }

    TBoolModel filled;
    TRGB linecolor;
    TRGB fillcolor;
    TTextModel fontname;
    TRGB background_color;
    TBoolModel drawgrid;
    // TBoolModel use_grid;
    TBoundedRangeModel gridsize;
    
    unsigned linewidth;
    TPen::ELineStyle linestyle;
    
    TFLine::EArrowMode arrowmode;
    TFLine::EArrowType arrowtype;
    unsigned arrowwidth;
    unsigned arrowheight;
};
typedef GSmartPointer<TFigurePreferences> PFigurePreferences;

/**
 * \ingroup figure
 */
class TFigureEditor:
  public TScrollPane
{
    typedef TScrollPane super;
    typedef TFigureEditor TThis;
    PFigureModel model;    
  public:
    
    TFigureEditor();
    void setWindow(TWindow*);
    TWindow* getWindow() const { return window; }

    TFigureEditor(TWindow*, const string &title);
    ~TFigureEditor();
    
    void setPreferences(TFigurePreferences *p);
    TFigurePreferences* getPreferences() const {
      return preferences;
    }
    void preferencesChanged();
    void modelChanged();

    void enableScroll(bool);
    void enableGrid(bool);
    void setGrid(int gridsize);
    void setBackground(int,int,int);

    void setRowHeaderRenderer(TFigureEditorHeaderRenderer *r) {
      row_header_renderer = r;
    }
    TFigureEditorHeaderRenderer* getRowHeaderRenderer() const {
      return row_header_renderer;
    }
    void setColHeaderRenderer(TFigureEditorHeaderRenderer *r) {
      col_header_renderer = r;
    }
    TFigureEditorHeaderRenderer* getCowHeaderRenderer() const {
      return col_header_renderer;
    }
    
    unsigned result;            // values are defined in TFigure
    
    void setModel(TFigureModel *m);
    TFigureModel * getModel() const {
      return model;
    }

  protected:
    PFigurePreferences preferences;
  
    TWindow *window;            // current window
    TMatrix2D *mat;             // transformation for the editor
    
    TFigureEditorHeaderRenderer *row_header_renderer;
    TFigureEditorHeaderRenderer *col_header_renderer;

  public:
    static const unsigned OP_SELECT = 0;
    static const unsigned OP_CREATE = 1;
    static const unsigned OP_ROTATE = 2;
    void setOperation(unsigned);
    void setCreate(TFigure*);
    
    // not all these methods work now, but the first 4 should do
    void identity();
    void rotate(double);
    void rotateAt(double x, double y, double degree);
    void translate(double, double);
    void scale(double sx, double sy);
    void shear(double, double);
    void multiply(const TMatrix2D*);

    // methods to modify selected or objects to be created
    void setLineColor(const TRGB&);
    void setFillColor(const TRGB&);
    void unsetFillColor();
    void setFont(const string &fontname);

    void invalidateWindow() { if (window) window->invalidateWindow(); }
    void invalidateFigure(TFigure*);

    void addFigure(TFigure*);   
    void deleteFigure(TFigure*);

    void selectAll();

    //! Unselect all selected objects.
    void clearSelection();
    
    //! Delete Selected Objects
    void deleteSelection();
    void deleteAll();
    
    void selection2Top();
    void selection2Bottom();
    void selectionUp();
    void selectionDown();
    
#if 0
    void figure2Top(TFigure*);
    void figure2Bottom(TFigure*);
    void figureUp(TFigure*);
    void figureDown(TFigure*);
#endif

    void group();
    void ungroup();
    
    TFigure* findFigureAt(int x, int y);

    static const unsigned STATE_NONE = 0;
    
    // states for OP_SELECT
    static const unsigned STATE_MOVE = 1;           // moving objects
    static const unsigned STATE_MOVE_HANDLE = 2;    // move handle
    static const unsigned STATE_SELECT_RECT = 3;    // select rectangular area
    static const unsigned STATE_EDIT = 4;           // edit object
    static const unsigned STATE_ROTATE = 5;         // rotate object
    static const unsigned STATE_MOVE_ROTATE = 6;
    
    // states for OP_CREATE
    static const unsigned STATE_START_CREATE = 20;   // set during `startCreate' and first `mouseLDown'
    static const unsigned STATE_CREATE = 21;
    
    int fuzziness; // fuzziness to catch handles
    unsigned state;
    
    int down_x, down_y;                             // last mouseXDown postion

    // undo stuff:
    int memo_x, memo_y;
    unsigned memo_n;
    TPoint memo_pt;

    // triggered after `selection' was modified
    TSignal sigSelectionChanged;

    TFigureSet selection;

    void paint();
    void print(TPenBase &pen, bool withSelection=false);
    void resize();
    void mouseEvent(TMouseEvent&);
    void mouseLDown(int,int,unsigned);
    void mouseMove(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void mouseRDown(int, int, unsigned);
    void keyDown(TKey, char*, unsigned);

    virtual void mouse2sheet(int mx, int my, int *sx, int *sy);
    virtual void sheet2grid(int sx, int sy, int *gx, int *gy);
    
    bool restore(TInObjectStream&);
    void store(TOutObjectStream&) const;

  protected:
    void init();
    
    TRGB background_color;
    
    unsigned operation;
    
    TFigure* gadget;        // the current gadget during create & edit
    TFigure* gtemplate;     // the gadget template during create
    
    int handle;             // the current handle or -1 during select
    
    bool use_scrollbars;
    int x1,x2, y1,y2;
    bool update_scrollbars; // checked during paint
    void updateScrollbars();
    void scrolled(int dx, int dy);
    
    void adjustPane();

    void stopOperation();   // stop the current operation
};

} // namespace toad

#endif
