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

#ifndef _TOAD_FIGUREEDITOR
#define _TOAD_FIGUREEDITOR

#include <toad/scrollpane.hh>
#include <toad/figureeditor/undoable.hh>
#include <toad/util/history.hh>

#include <set>

namespace toad {

class TFigure;
class TScrollBar;

/**
 * \ingroup figure
 */
class TFigureEditor:
  public TScrollPane
{
    typedef TScrollPane super;
    PFigureModel model;    
  public:
    
    TFigureEditor();
    void setWindow(TWindow*);
    TWindow* getWindow() const { return window; }

    TFigureEditor(TWindow*, const string &title);
    ~TFigureEditor();

    void enableScroll(bool);
    void enableGrid(bool);
    void setGrid(int x, int y);
    void setBackground(int,int,int);

    unsigned result;            // values are defined in TFigure
    
    void setModel(TFigureModel *m) {
      model = m;
      invalidateWindow();
    }
    TFigureModel * getModel() const {
      return model;
    }

  protected:
    TWindow *window;            // current window
    TMatrix2D *mat;             // transformation for the editor
    int fuzziness;              // fuzziness to catch handles

  public:
    static const unsigned OP_SELECT = 0;
    static const unsigned OP_CREATE = 1;
    static const unsigned OP_ROTATE = 2;
    void setOperation(unsigned);
    void setCreate(TFigure*);
    
    void identity();
    void rotate(double);
    void rotateAt(double x, double y, double degree);
    void translate(double, double);
    void scale(double sx, double sy);
    void shear(double, double);
    void multiply(const TMatrix2D*);

    void setLineColor(const TRGB&);
    void setFillColor(const TRGB&);
    void setFilled(bool);

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
    void gadget2Top(TFigure*);
    void gadget2Bottom(TFigure*);
    void gadgetUp(TFigure*);
    void gadgetDown(TFigure*);
#endif

    void group();
    void ungroup();
    
    typedef GHistory<PUndoable> THistory;
    THistory history;
    void undo();
    void redo();
    void clearHistory() {history.clear();}
    
    TFigure* findGadgetAt(int x, int y);

    static const unsigned STATE_NONE = 0;
    
    // states for OP_SELECT
    static const unsigned STATE_MOVE = 1;           // moving objects
    static const unsigned STATE_MOVE_HANDLE = 2;    // move handle
    static const unsigned STATE_SELECT_RECT = 3;    // select rectangular area
    static const unsigned STATE_EDIT = 4;           // edit object
    static const unsigned STATE_ROTATE = 5;         // rotate object
    
    // states for OP_CREATE
    static const unsigned STATE_START_CREATE = 5;   // set during `startCreate' and first `mouseLDown'
    static const unsigned STATE_CREATE = 6;
    
    unsigned state;
    
    int down_x, down_y;                             // last mouseXDown postion

    // undo stuff:
    int memo_x, memo_y;
    unsigned memo_n;
    TPoint memo_pt;

    // triggered after `selection' was modified
    TSignal sigSelectionChanged;

    typedef set<TFigure*> TFigureSet;
    TFigureSet selection;

    void paint();
    void print(TPenBase&);
    void resize();
    void mouseLDown(int,int,unsigned);
    void mouseMove(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void keyDown(TKey, char*, unsigned);

    bool restore(TInObjectStream&);
    void store(TOutObjectStream&) const;

    class TColorSelector:
      public TWindow
    {
        TFigureEditor *gedit;
        TRGB linecolor;
        TRGB fillcolor;
        bool filled;
        int border;
      public:
        typedef TWindow super;
        TColorSelector(TWindow *parent, const string &title, TFigureEditor *gedit);
        void paint();
        void mouseLDown(int, int, unsigned);
    };

  protected:
    void init();
  
    bool filled;
    TRGB line_color;
    TRGB fill_color;
    
    TRGB background_color;
    bool draw_grid;
    
    unsigned operation;
    
    TFigure* gadget;        // the current gadget during create & edit
    TFigure* gtemplate;     // the gadget template during create
    unsigned gridx, gridy;
    
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
