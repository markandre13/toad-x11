/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

class TTableSelectionModel:
  public TModel
{
  protected:
    TRegion region;

  public:
    TTableSelectionModel() {
      selection_mode = SINGLE;
    }
  
    enum ESelectionMode{
      MULTIPLE_INTERVAL,
      SINGLE_INTERVAL,
      SINGLE
    };
    
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

class TAbstractTableModel:
  public TModel
{
  public:
    virtual int getRows() = 0;
    virtual int getCols();
};

typedef GSmartPointer<TAbstractTableModel> PAbstractTableModel;

template <class T>
class GAbstractTableModel:
  public TAbstractTableModel
{
  public:
    typedef T TElement;
    virtual const TElement& getElementAt(int xindex, int yindex) = 0;
};

template <class T>
class GTableSelectionModel:
  public TTableSelectionModel
{
  public:
    typedef T TModel;
    typedef typename T::TElement TElement;
   
    TModel *model;
 
    GTableSelectionModel(TModel *m):model(m) { }
  
    class iterator:
      public TTableSelectionModel::iterator
    {
        TModel *model;
        typedef TTableSelectionModel::iterator super;
      public:
        iterator() {
          model = 0;
        }
        iterator(TRegion *r, bool b, TModel *m):
           TTableSelectionModel::iterator(r, b), model(m) {}
        const TElement& operator*() { return model->getElementAt(getX(), getY()); }
    };
    iterator begin() {
      return iterator(&region, true, model);
    }
    iterator end() {
      return iterator(&region, false, model);
    }
};

class TAbstractTableCellRenderer:
  public TModel
{
  public:
    TAbstractTableCellRenderer() {
      per_row = per_col = false;
    }
  
    virtual int getRows() = 0;
    virtual int getCols() = 0;
    virtual int getRowHeight(int row) = 0;
    virtual int getColWidth(int col) = 0;

    /**
     * Return the model associated with this renderer.
     *
     * This method isn't required for rendering and may return NULL.
     *
     * The purpose of this method is to use it combination with
     * GTableSelectionModel?
     */
    virtual TAbstractTableModel * getModel() { return 0; }
    virtual void renderItem(TPen&, int xindex, int yindex, int w, int h, bool cursor, bool selected, bool focus) = 0;

    void modelChanged() {
      sigChanged();
    }
    
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
    PTableSelectionModel selection;
    PAbstractTableHeaderRenderer row_header_renderer;
    PAbstractTableHeaderRenderer col_header_renderer;

    unsigned border;
    
    //! cursor position
    int cx, cy;
    
    //! position where selection over multiple fields started
    int sx, sy;

    //! true means, user is defining a selection over multiple fields
    bool selecting;
    
    //! first (upper, left) field;
    int ffx, ffy;
    
    //! first (upper, left) pixel of first (upper, left) field
    int fpx, fpy;
    
  public:
  
    void setTableBorder(unsigned b) { border = b; }
    unsigned getTableBorder() const { return border; }
  
    TTable(TWindow *p, const string &t);
    
    void setRenderer(TAbstractTableCellRenderer *r);
    TAbstractTableCellRenderer* getRenderer() const { return renderer; }
    
    void setSelectionModel(TTableSelectionModel *m);
    TTableSelectionModel* getSelectionModel() const { return selection; }
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
    
    int getCursorX() const { return cx; }
    int getCursorY() const { return cy; }
    void selectAtCursor();
    
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
