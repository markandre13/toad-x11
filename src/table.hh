/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#ifndef TTable
#define TTable TTable

#include <toad/scrollpane.hh>
#include <toad/model.hh>

namespace toad {

class TScrollBar;

class TAbstractTableSelectionModel:
  public TModel
{
  public:
    enum ESelectionMode {
      MULTIPLE_INTERVAL,
      SINGLE_INTERVAL,
      SINGLE
    };

    // this is a hint for TTable on how to handle the user interaction:
    virtual ESelectionMode getSelectionMode() const = 0;
    
    // these are called by TTable:
    virtual void clearSelection() = 0;
    virtual void setSelection(int x, int y) = 0;
    virtual void setSelection(int x, int y, int w, int h) = 0;
    virtual void toggleSelection(int x, int y) = 0;
    virtual bool isSelected(int x, int y) const = 0;
    virtual bool isEmpty() const = 0;
};

typedef GSmartPointer<TAbstractTableSelectionModel> PAbstractTableSelectionModel;

/**
 * A basic selection model for a single position.
 */
class TTableSingleSelectionModel:
  public TAbstractTableSelectionModel
{
    int col, row;
    bool selected;

  public:
    TTableSingleSelectionModel() {
      clearSelection();
    }
    
    int getCol() const { return col; }
    int getRow() const { return row; }
    
    virtual ESelectionMode getSelectionMode() const;
    virtual void clearSelection();
    virtual void setSelection(int col, int row);
    virtual void setSelection(int col, int row, int w, int h);
    virtual void toggleSelection(int col, int row);
    virtual bool isSelected(int col, int row) const;
    virtual bool isEmpty() const;
};

class TTableSelectionModel:
  public TAbstractTableSelectionModel
{
  protected:
    TRegion region;

  public:
    TTableSelectionModel() {
      selection_mode = MULTIPLE_INTERVAL;
    }
    
    class iterator {
        friend bool operator==(const iterator&, const iterator&);
    
        static const long BEGIN = -1;
        static const long END   = -2;
        static const long EMPTY = -3;
    
        TRegion *rgn;
        long n;       // current rectangle
        int x, y;     // current position
        
        int x1, y1;   // upper left corner
        int x2, y2;   // bottom right corner
        
        void getRect();

      public:
        iterator();
        iterator(const iterator&);
        iterator(TRegion *, bool);
        iterator operator++();
        iterator operator--();
        iterator operator++(int) { iterator tmp=*this; ++(*this); return tmp; }
        iterator operator--(int) { iterator tmp=*this; --(*this); return tmp; }
        int getX() const { return x; }
        int getY() const { return y; }
    };
    
    iterator begin() { return iterator(&region, true); }
    iterator end() { return  iterator(&region, false); }

#if 0    
    void addSelectionInterval(int x, int y, int w, int h);
    void removeSelectionInterval(int x, int y, int w, int h);
    
    void clearSelection((int x, int y, int w, int h);
    bool isSelectionEmpty() const;
#endif
    
    ESelectionMode getSelectionMode() const {
      return selection_mode;
    }

    void clearSelection();
    void setSelection(int x, int y);
    void setSelection(int x, int y, int w, int h);
    void toggleSelection(int x, int y);
    bool isSelected(int x, int y) const;
    bool isEmpty() const { return region.isEmpty(); }
    
    void setSelectionMode(ESelectionMode);
    
  private:
    void prepare(int x, int y);
    ESelectionMode selection_mode;
};

typedef GSmartPointer<TTableSelectionModel> PTableSelectionModel;

inline bool operator==(const TTableSelectionModel::iterator &a,
                const TTableSelectionModel::iterator &b)
{
  return a.n == b.n && a.x == b.x && a.y == b.y;
}

inline bool operator!=(const TTableSelectionModel::iterator &a,
                const TTableSelectionModel::iterator &b)
{
  return !(a == b);
}

class TAbstractTableCellRenderer:
  public TModel
{
  public:
    TAbstractTableCellRenderer() {
      per_row = per_col = false;
    }
  
    virtual int getRows() { return 1; }
    virtual int getCols() { return 1; }
    virtual int getRowHeight(int row) = 0;
    virtual int getColWidth(int col) = 0;
    /**
     * Render cell background, cursor and invoke renderItem.
     */
    virtual void renderCell(TPen&, int xindex, int yindex, int w, int h, bool cursor, bool selected, bool focus);
    virtual void renderItem(TPen&, int xindex, int yindex, int w, int h, bool cursor, bool selected, bool focus) = 0;

    // this method is to enable signals to trigger our signal:
    void modelChanged() { sigChanged(); }

    bool per_row, per_col;
};

typedef GSmartPointer<TAbstractTableCellRenderer> PAbstractTableCellRenderer;

class TAbstractTableHeaderRenderer:
  public TModel
{
  public:
    virtual int getHeight() = 0;
    virtual int getWidth() = 0;
    virtual void renderItem(TPen &pen, int idx, int w, int h) = 0;
};

typedef GSmartPointer<TAbstractTableHeaderRenderer> PAbstractTableHeaderRenderer;

class TDefaultTableHeaderRenderer:
  public TAbstractTableHeaderRenderer
{
    bool numeric;
  public:
    TDefaultTableHeaderRenderer(bool numeric_mode=true):
      numeric(numeric_mode) {}
    int getHeight();
    int getWidth();
    void renderItem(TPen &pen, int idx, int w, int h);
};

class TTable:
  public TScrollPane
{
    typedef TScrollPane super;
    PAbstractTableCellRenderer renderer;
    PAbstractTableSelectionModel selection;
    PAbstractTableHeaderRenderer row_header_renderer;
    PAbstractTableHeaderRenderer col_header_renderer;

    unsigned border;
    
    //! Cursor position.
    int cx, cy;
    
    /**
     * Position of the last selected single field. Used for 
     * getLastSelectionRow() and -Col() and when selecting
     * areas.
     */
    int sx, sy;

    //! 'true' means, user is defining a selection over multiple fields
    bool selecting;
    
    //! first (upper, left) field
    int ffx, ffy;
    
    //! first (upper, left) pixel of first (upper, left) field
    int fpx, fpy;
    
  public:
  
    void setTableBorder(unsigned b) { border = b; }
    unsigned getTableBorder() const { return border; }
  
    TTable(TWindow *p, const string &t);
    
    void setRenderer(TAbstractTableCellRenderer *r);
    TAbstractTableCellRenderer* getRenderer() const { return renderer; }
    
    void setSelectionModel(TAbstractTableSelectionModel *m);
    TAbstractTableSelectionModel* getSelectionModel() const { return selection; }
    void selectionChanged();
    TSignal sigSelection;
    
    void setRowHeaderRenderer(TAbstractTableHeaderRenderer *r);
    TAbstractTableHeaderRenderer * getRowHeaderRenderer() const {
      return row_header_renderer;
    }
    
    void setColHeaderRenderer(TAbstractTableHeaderRenderer *r);
    TAbstractTableHeaderRenderer * getColHeaderRenderer() const {
      return col_header_renderer;
    }

    void paint();
    void resize();
    void focus(bool);

    void mouseLDown(int x,int y, unsigned modifier);
    void mouseMove(int x,int y, unsigned modifier);
    void mouseLUp(int x,int y, unsigned modifier);

    void keyDown(TKey key, char *string, unsigned modifier);
    void keyUp(TKey key, char *string, unsigned modifier);
    
    void setCursor(int col, int row);
    int getCursorCol() const { return cx; }
    int getCursorRow() const { return cy; }
    void selectAtCursor();
    
    int getLastSelectionCol() const { return sx; }
    int getLastSelectionRow() const { return sy; }
    
    //! the cursor was moved
    TSignal sigCursor;
    
    //! mouse is pressed down on an entry
    TSignal sigPressed;
    
    //! click (down-up) on an entry
    TSignal sigClicked;
    
    //! double click (down-up-down-up) on an entry
    TSignal sigDoubleClicked;
    
    //! true: expand the last column to match the table's width
    bool stretchLastColumn;
    
    //! true: don't paint a cursor
    bool noCursor;
    
    //! true: the current mouse position will always be selected
    bool selectionFollowsMouse;
    
  protected:
    static const int CENTER_VERT=1;
    static const int CENTER_HORZ=2;
    static const int CENTER_BOTH=3;
    void center(int how);

    void adjustPane();
    void scrolled(int dx, int dy);
    bool mouse2field(int mx, int my, int *fx, int *fy);
    
    // precalculated values for optimization
    void handleNewModel();
    int rows, cols;     // table size in rows & columns
    bool per_row, per_col;
    
    // getRowHeight & getColWidth are expensive operation so call 'em
    // once and store their values in row_info and col_info
    struct TRCInfo {    // row/column info
      int size;
    };
    TRCInfo *row_info, *col_info;
    
    void invalidateCursor();
    void invalidateChangedArea(int sx, int sy,
                               int ax, int ay,
                               int bx, int by);
                                                                
};

} // namespace toad

#endif
