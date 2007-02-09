/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2006 by Mark-André Hopf <mhopf@mark13.org>
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
#define _TOAD_FIGUREEDITOR 1

#include <toad/figure.hh>
#include <toad/figuremodel.hh>
#include <toad/scrollpane.hh>
#include <toad/undo.hh>

#include <toad/boolmodel.hh>
#include <toad/textmodel.hh>
#include <toad/integermodel.hh>


namespace toad {

// class TFigure;
class TFigureEditor;
class TScrollBar;

class TFigureEditorHeaderRenderer
{
  public:
    virtual void render(TPenBase &pen, int pos, int size, TMatrix2D *mat) = 0;
    virtual int getSize() = 0;
    virtual void mouseEvent(TMouseEvent&);
};

class TFigureTool
{
  public:
    virtual ~TFigureTool();
    virtual void stop(TFigureEditor*);
    virtual void mouseEvent(TFigureEditor *fe, TMouseEvent &me);
    virtual void keyEvent(TFigureEditor *fe, TKeyEvent &ke);
    virtual void setAttributes(TFigureAttributes *p);
    virtual void paintSelection(TFigureEditor *fe, TPenBase &pen);
};

/**
 * Tool to create figures.
 *
 * Mose of the code to actually create figures is located in the figures
 * themselves. This class just creates an interface between the figure
 * editor and the figure.
 */
class TFCreateTool:
  public TFigureTool
{
    TFigure *tmpl;
    TFigure *figure;
  public:
    /**
     * \param tmpl template for the figure to be created. This object will be
     *        deleted along with the TFCreateTool.
     */
    TFCreateTool(TFigure *tmpl) {
      this->tmpl = tmpl;
      figure = 0;
    }
    ~TFCreateTool() {
      delete tmpl;
    }
  protected:    
    void stop(TFigureEditor*);
    void mouseEvent(TFigureEditor *fe, TMouseEvent &me);
    void keyEvent(TFigureEditor *fe, TKeyEvent &ke);
    void setAttributes(TFigureAttributes *p);
    void paintSelection(TFigureEditor *fe, TPenBase &pen);
};

class TFigureAttributes:
  public TModel
{
    TFigureEditor *current;
    TFigureTool *tool;
    TFigureAttributes(const TFigureAttributes&) {};
  public:
    TFigureAttributes();
    virtual ~TFigureAttributes();

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
    // unsigned getOperation() const { return current->getOperation(); }
    void setCreate(TFigure *figure) {
      setTool(new TFCreateTool(figure));
    }
    void setTool(TFigureTool*);
    TFigureTool* getTool() const { return tool; }
    
    void group();
    void ungroup();
    void selectionDown();
    void selection2Bottom();
    void selectionUp();
    void selection2Top();
    void applyAll();
    
    // additional information on why sigChanged was triggered
    enum EReason {
      /**
       * All parameters have changed or, apply all parameters.
       */
      ALLCHANGED=0, ALL=0,
      /**
       * Where going to edit another object.
       */
      CURRENTCHANGED=1, CURRENT=1,
      /**
       * Drawing properties have changed.
       */
      GRID=2,
      LINECOLOR,
      FILLCOLOR,
      UNSETFILLCOLOR,
      LINEWIDTH,
      LINESTYLE,
      ARROWMODE,
      ARROWSTYLE,
      FONTNAME,
      
      TOOL,
      ALPHA,
      /**
       * This TPreferences was subclassed. Try dynamic_cast to get
       * more information.
       */
      EXTENDED = 256
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
    void setFont(const string &font) {
      this->font.setFont(font);
      fontname = font;
      reason = FONTNAME;
      sigChanged();
    }
    const string& getFont() const {
      return fontname;
    }

    TBoolModel filled;
    TRGB linecolor;
    TRGB fillcolor;
    TIntegerModel alpha;
    TTextModel fontname;
    TFont font;
    TRGB background_color;
    TBoolModel drawgrid;
    // TBoolModel use_grid;
    TIntegerModel gridsize;
    
    unsigned linewidth;
    TPen::ELineStyle linestyle;
    
    TFLine::EArrowMode arrowmode;
    TFLine::EArrowType arrowtype;
    unsigned arrowwidth;
    unsigned arrowheight;
};
typedef GSmartPointer<TFigureAttributes> PFigureAttributes;

/**
 * \ingroup figure
 */
class TFigureEditor:
  public TScrollPane
{
    typedef TScrollPane super;
    typedef TFigureEditor TThis;
    PFigureModel model;
    TFigureTool *tool;
  public:
    bool quick:1;     // active TFigureTool wants quick drawing method
    bool quickready:1;// TFigureEditor is prepared for quick drawing mode
    
    TFigureEditor();
    void setWindow(TWindow*);
    TWindow* getWindow() const { return window; }
    TFigureTool* getTool() const { return tool; }

    TFigureEditor(TWindow*, const string &title, TFigureModel *model=0);
    ~TFigureEditor();
    
    void setAttributes(TFigureAttributes *p);
    TFigureAttributes* getAttributes() const {
      return preferences;
    }
    void preferencesChanged();
    void modelChanged();

    void enableScroll(bool);
    void enableGrid(bool);
    void setGrid(int gridsize);

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
    
    void setModified(bool modified) {
      this->modified = modified;
    }
    bool isModified() const {
      return modified;
    }

  protected:
    bool modified;
    PFigureAttributes preferences;
  
    TWindow *window;            // current window
    TMatrix2D *mat;             // transformation for the editor
    
    TFigureEditorHeaderRenderer *row_header_renderer;
    TFigureEditorHeaderRenderer *col_header_renderer;

  public:
    static const unsigned OP_SELECT = 0;
//    static const unsigned OP_CREATE = 1;
    static const unsigned OP_ROTATE = 2;
    void setOperation(unsigned);
    unsigned getOperation() const { return operation; }
    void setCreate(TFigure *figure) {
      setTool(new TFCreateTool(figure));
    }
    void setTool(TFigureTool*);
    
    // not all these methods work now, but the first 4 should do
    void identity();
    void rotate(double);
    void rotateAt(double x, double y, double radiants);
    void translate(double, double);
    void scale(double sx, double sy);
    void shear(double, double);
    void multiply(const TMatrix2D*);

    const TMatrix2D* getMatrix() const { return mat; }

    // methods to modify selected or objects to be created
    void setLineColor(const TRGB&);
    void setFillColor(const TRGB&);
    void unsetFillColor();
    void setFont(const string &fontname);

    void invalidateWindow(bool b=true) { 
      if (window) 
        window->invalidateWindow(b); 
    }
    void invalidateWindow(int x, int y, int w, int h, bool b=true) {
      if (window)
        window->invalidateWindow(x, y, w, h, b);
    }
    void invalidateWindow(const TRectangle &r, bool b=true) {
      if (window)
        window->invalidateWindow(r, b);
    }
    void invalidateWindow(const TRegion &r, bool b=true) {
      if (window)
        window->invalidateWindow(r, b);
    }
    virtual void invalidateFigure(TFigure*);
    void getFigureShape(TFigure*, TRectangle*, const TMatrix2D*);

    void addFigure(TFigure*);   
    void deleteFigure(TFigure*);

    void selectAll();

    //! Unselect all selected objects.
    bool clearSelection();
    
    //! Delete Selected Objects
    void deleteSelection();
    void deleteAll();
    
    void selection2Top();
    void selection2Bottom();
    void selectionUp();
    void selectionDown();
    void applyAll();
    
    void selectionCut();
    void selectionCopy();
    void selectionPaste();

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
    
    void paintGrid(TPenBase &pen);
    void paintSelection(TPenBase &pen);
    void paintDecoration(TPenBase &pen);
    virtual void print(TPenBase &pen, TFigureModel *model, bool withSelection=false, bool justSelection=false);
    
    void resize();
    void mouseEvent(TMouseEvent&);
    void keyEvent(TKeyEvent&);
    
    void mouseLDown(int,int,unsigned);
    void mouseMove(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void mouseRDown(int, int, unsigned);
    void keyDown(TKey, char*, unsigned);

    virtual void mouse2sheet(int mx, int my, int *sx, int *sy);
    virtual void sheet2grid(int sx, int sy, int *gx, int *gy);
    
    bool restore(TInObjectStream&);
    void store(TOutObjectStream&) const;

    void setCurrent(TFigure *f) { gadget = f; }
    TFigure* getCurrent() const { return gadget; }

  protected:
    void init(TFigureModel *m);
    
    unsigned operation;
    
    TFigure* gadget;        // the current gadget during create & edit

    int handle;             // the current handle or -1 during select
    bool tht; // translate handle transform?
    
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
