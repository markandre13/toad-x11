/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

class TTable;
class TTableAdapter;

class TAbstractSelectionModel:
  public TModel
{
  public:
    enum ESelectionMode {
      MULTIPLE_INTERVAL,
      SINGLE_INTERVAL,
      SINGLE
    };

    enum ERowColMode {
      FIELD,
      WHOLE_ROW,
      WHOLE_COL
    } rowcolmode;

    void setRowColMode(ERowColMode rowcolmode) {
      this->rowcolmode = rowcolmode;
    }
    virtual bool perRow() const { return rowcolmode == WHOLE_ROW; }
    virtual bool perCol() const { return rowcolmode == WHOLE_COL; }
    virtual ESelectionMode getSelectionMode() const;
    
    /** 
     * Select an entry at the given column and row.
     *
     * An implementation should also call sigChanged().
     */
    virtual void setSelection(int col, int row) = 0;
    virtual bool isSelected(int col, int row) const = 0;

    virtual bool getFirst(int *x, int *y) const = 0;
    virtual bool getNext(int *x, int *y) const = 0;

    virtual void setSelection(int col, int row, int w, int h);
    virtual void toggleSelection(int col, int row);
    virtual void clearSelection();
    virtual bool isEmpty() const;
};

typedef GSmartPointer<TAbstractSelectionModel> PAbstractSelectionModel;

/**
 * A basic selection model for a single position.
 */
class TSingleSelectionModel:
  public TAbstractSelectionModel
{
    size_t col, row;
    bool selected;

  public:
    TSingleSelectionModel() {
      selected = false;
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

    bool getFirst(int *x, int *y) const;
    bool getNext(int *x, int *y) const;
};

class TRectangleSelectionModel:
  public TAbstractSelectionModel
{
    int x1, x2, y1, y2;
    bool empty;
  public:

    TRectangleSelectionModel() {
      empty = true;
      rowcolmode = FIELD;
    }
    ESelectionMode getSelectionMode() const {
      return SINGLE_INTERVAL;
    }


    void setSelection(int col, int row) {
      setSelection(col, row, 1, 1);
    }
    bool isSelected(int col, int row) const {
      if (empty)
        return false;
      return (x1<=col && col<=x2 && y1<=row && row<=y2);
    }
    void setSelection(int col, int row, int w, int h) {
      if (!empty && x1==col && x2==col+w-1 && y1==row && y2==row+h-1)
        return;
      empty = false;
      x1 = col;
      x2 = col+w-1;
      y1 = row;
      y2 = row+h-1;
      sigChanged();
    }
    void toggleSelection(int col, int row) {
      if (x1<=col && col<=x2 &&
          y1<=row && row<=y1)
      {
        empty = !empty;
      } else {
        empty = false;
      }
      x1=x2=col;
      y1=y2=row;
      sigChanged();
    }

    
    void clearSelection() {
      if (empty)
        return;
      empty = true;
      sigChanged();
    }
    bool isEmpty() const {
      return empty;
    }
    void getSelection(TRectangle *r) {
      r->set(x1, y1, x2-x1+1, y2-y1+1);
    }

    bool getFirst(int *x, int *y) const;
    bool getNext(int *x, int *y) const;
};

class TSelectionModel:
  public TAbstractSelectionModel
{
  protected:
    TRegion region;

  public:
    TSelectionModel() {
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

    bool getFirst(int *x, int *y) const;
    bool getNext(int *x, int *y) const;
    
  private:
    void prepare(int x, int y);
    ESelectionMode selection_mode;
};

typedef GSmartPointer<TSelectionModel> PSelectionModel;

inline bool operator==(const TSelectionModel::iterator &a,
                const TSelectionModel::iterator &b)
{
  return a.n == b.n && a.x == b.x && a.y == b.y;
}

inline bool operator!=(const TSelectionModel::iterator &a,
                const TSelectionModel::iterator &b)
{
  return !(a == b);
}

struct TTableEvent
{
  enum EType {
    //! return size of column 'col' in 'w'
    GET_COL_SIZE,
    //! return size of row 'row' in 'h'
    GET_ROW_SIZE,
    //! paint field (col, row) of size (w, h) and the flags
    //! cursor, selected, focus, per_row and per_col using pen
    //! 'pen'
    PAINT,
    //! handle mouse event 'mouse' for field (col, row) of size (w, h)
    MOUSE,
    //! handle keyboard event in 'key' with cursor on field (col, row)
    //! and on return set 'flag' to 'true' when TTable shall not handle
    //! the event on its own
    KEY
  } type;
  TPen *pen;
  TMouseEvent mouse;
  TKeyEvent *key;
  size_t col, row;   // current field
  int w, h;          // field size in pixels
  size_t cols, rows; // table size in fields
  bool per_row, per_col;
  bool cursor;    // show cursor
  bool selected;  // field is selected
  bool focus;     // table has keyboard focus
  bool flag;
};


class TTableModel:
  public TModel
{
  public:
    TTableModel() {
      reason = CHANGED;
    }
    // sigChanged protocol:
    enum EReason {
      // TTableModel messages
      INSERT_ROW, REMOVED_ROW,
      INSERT_COL, REMOVED_COL,
      CONTENT, // content was modified, but now row/cols added/removed
      // TTableAdapter messages (not generated by models)
      RESIZED_ROW, RESIZED_COL,
      CHANGED, // new model
    } reason;
    size_t where;
    size_t size;
    bool isEmpty() const { return getRows()==0 && getCols()==0;}
    virtual size_t getRows() const = 0;
    virtual size_t getCols() const { return 1; }
};

typedef GSmartPointer<TTableModel> PTableModel;

class TTableAdapter:
  public TModel
{
  protected:
    TTable *table;

    // support attributes for handleString
    static TTableAdapter *edit;
    static int cx;
    static size_t col, row;

  public:
    TTableAdapter();

    void setTable(TTable *t) { table = t; }
    TTable* getTable() const { return table; }
    virtual TTableModel* getModel() const = 0; // const { return 0; }

    virtual size_t getCols() { return getModel() ? getModel()->getCols() : 1; }
    virtual size_t getRows() { return getModel() ? getModel()->getRows() : 1; }

    //! Most messages from TTable to TTableAdapter go through the
    //! tableEvent method to ease delegation to other objects which
    //! may handle text, checkboxes, etc...
    virtual void tableEvent(TTableEvent &te);

    virtual int getRowHeight(size_t row) { return 0; }
    virtual int getColWidth(size_t col) { return 0; }

    void renderBackground(TTableEvent &te);
    void renderCursor(TTableEvent &te);

    // utility methods
    void handleString(TTableEvent &te, string *s);
    void handleCheckBox(TTableEvent &te, bool *b);

    /**
     * Render cell background, cursor and invoke renderItem.
     */
    virtual void renderCell(TPen&, TTableEvent&);
    virtual void renderItem(TPen&, const TTableEvent&) {}
    virtual void mouseEvent(TMouseEvent &, int xindex, int yindex, int w, int h);

    // this method is to enable signals to trigger our signal:
    void modelChanged();
    
    TTableModel::EReason reason;
    size_t where, size;
};

class TSimpleTableAdapter:
  public TTableAdapter
{
  public:
    virtual TTableModel* getModel() const;
};

template <class T>
class GModelOwner
{
  protected:
    GSmartPointer<T> model;
  public:
    GModelOwner() {}
    virtual ~GModelOwner() {
      if(model)
        disconnect(model->sigChanged, this, &GModelOwner<T>::modelChanged);
    }
    void setModel(T *m) {
//      cout << "GModelOwner::setModel("<<m<<")"<< endl;
      if (m==model)
        return;
      if (model)
        disconnect(model->sigChanged, this, &GModelOwner<T>::modelChanged);
      model = m;
      if (model)
        connect(model->sigChanged, this, &GModelOwner<T>::modelChanged, false);
      modelChanged(true);
    }
    T* getModel() const { return model; }
    virtual void modelChanged(bool newmodel) = 0;
};

template <class T>
class GTableAdapter:
  public TTableAdapter, public GModelOwner<T>
{
  public:
     GTableAdapter() { }
     GTableAdapter(T *m):GModelOwner<T>(m) { }
     void setModel(T *m) {
      cout << "GTableAdapter::setModel("<<m<<")"<< endl;
      GModelOwner<T>::setModel(m);
      reason = TTableModel::CHANGED;
      sigChanged();
    }
};

typedef GSmartPointer<TTableAdapter> PTableAdapter;

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
    vector<string> text;
  public:
    TDefaultTableHeaderRenderer(bool numeric_mode=true):
      numeric(numeric_mode) {}
    int getHeight();
    int getWidth();
    void renderItem(TPen &pen, int idx, int w, int h);
    void setText(unsigned pos, const string &txt);
};

class TTable:
  public TScrollPane
{
    typedef TScrollPane super;
//    PTableModel model;
    PTableAdapter adapter;
    PAbstractSelectionModel selection;
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
    bool start_selecting;
    
    //! first (upper, left) field
    int ffx, ffy;
    
    //! first (upper, left) pixel of first (upper, left) field
    int fpx, fpy;
    
  public:
  
    void setTableBorder(unsigned b) { border = b; }
    unsigned getTableBorder() const { return border; }
  
    TTable(TWindow *p, const string &t);

//    void setModel(TTableModel *model);
//    TTableModel *getModel() const { return model; }
    
    void setAdapter(TTableAdapter *r);
    TTableAdapter* getAdapter() const { return adapter; }
    
    void setSelectionModel(TAbstractSelectionModel *m);
    TAbstractSelectionModel* getSelectionModel() const { return selection; }
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

    void mouseEvent(TMouseEvent &me);
    void keyEvent(TKeyEvent &ke);
    
    void mouseLDown(int x,int y, unsigned modifier);
    void mouseMove(int x,int y, unsigned modifier);
    void mouseLUp(int x,int y, unsigned modifier);

    void keyDown(TKey key, char *string, unsigned modifier);
    void keyUp(TKey key, char *string, unsigned modifier);
    
    void setCursor(int col, int row);
    int getCursorCol() const { return cx; }
    int getCursorRow() const { return cy; }
    void selectAtCursor();
    void clickAtCursor();
    void doubleClickAtCursor();

    int getLastSelectionCol() const { return sx; }
    int getLastSelectionRow() const { return sy; }

    size_t getRows() const { return rows; }
    size_t getCols() const { return cols; }
    int getRowHeight(size_t row) const { return (row<0||row>=rows) ? 0 : row_info[row].size; }
    int getColWidth(size_t col) const { return (col<0||col>=cols) ? 0 : col_info[col].size; }
    void setRowHeight(size_t row, int height);
    void setColWidth(size_t row, int width);

    bool isRowOpen(size_t row) const { return (row<0||row>=rows) ? 0 : row_info[row].open; }
    bool isColOpen(size_t col) const { return (col<0||col>=cols) ? 0 : col_info[col].open; }
    void setRowOpen(size_t row, bool open) {
      if (row<0||row>=rows)
        return;
      row_info[row].open = open;
    }
    void setColOpen(size_t col, bool open) {
      if (col<0||col>=cols)
        return;
      col_info[col].open = open;
    }

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
    
    //! true: the current mouse position with button pressed will always be selected
    bool selectionFollowsMouse;
    
  protected:
    void _moveCursor(int newcx, int newcy, unsigned modifier);
    void _setSXSY(int x, int y);
    static const int CENTER_VERT=1;
    static const int CENTER_HORZ=2;
    static const int CENTER_BOTH=3;
    void center(int how);

    void adjustPane();
    void scrolled(int dx, int dy);
    bool mouse2field(int mx, int my, int *fx, int *fy, int *rfx=0, int *rfy=0);
    
    // void modelChanged();
    void _handleInsertRow();
    void _handleResizedRow();
    void _handleRemovedRow();

    void adapterChanged();

    // precalculated values for optimization
    void handleNewModel();
    size_t rows, cols;     // table size in rows & columns
//    bool per_row, per_col;
    
    // getRowHeight & getColWidth are expensive operations so call 'em
    // once and store their values in row_info and col_info
    struct TRCInfo {    // row/column info
      int size;
      bool open;
    };
    TRCInfo *row_info, *col_info;
    
    void invalidateCursor();
    void invalidateChangedArea(int sx, int sy,
                               int ax, int ay,
                               int bx, int by);
                                                                
};

} // namespace toad

#endif
