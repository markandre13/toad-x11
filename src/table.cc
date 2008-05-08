/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/toad.hh>
#include <toad/table.hh>
#include <toad/figure.hh>
#include <toad/dragndrop.hh>

#include <stdio.h>
#include <unistd.h>

#ifdef check
#undef check
#endif

using namespace toad;

#define DBM(M)
#define DBM2(M)
#define DBSCROLL(M)

TTableModel::~TTableModel()
{
}

/**
 * @defgroup table Table
 *
 * This group contains a set of classes to display data in a table.
 *
 * TTable: This widget controls the following objects:
 *   TTableModel (optional)
 *     The table model keeps track of changes made to the table. This
 *     model is optional. Please refer to TAbstractSelectionModel for
 *     a full explanation.
 *   TSelectionModel (optional)
 *     The selection model tracks the selected elements in the table.
 *     There a three pre-defined selection models
 *       TSingleSelectionModel
 *       TRectangleSelectionModel
 *       TSelectionModel
 *     Sometimes the selection model itself is the data to be controlled
 *     by the table. Eg. the selected enumeration within a combobox.
 *   THeaderRenderer (optional for columns and rows)
 *   TTableRenderer (required)
 *     
 *
 * @verbatim
 
The base classes are defined in table.hh:
 
TModel
+- TTableAdapter             
+- TAbstractSelectionModel           
|  +- TSingleSelectionModel
|  +- TRectangleSelectionModel
|  +- TSelectionModel
+- TAbstractTableHeaderRenderer             
   +- TDefaultTableHeaderRenderer         
   @endverbatim
 *
 * \code
 * class TMyTable:
 *   public TTable
 * {
 * }
 * \endcode
 *
 * \note
 *   TTable comes with a vast collection of classes and is difficult
 *   to use and currently not well document, the first being the reason
 *   for the second. My idea is a add more and more utility classes for
 *   some time and boil it down to a minimum later when all requirements
 *   have become visible.
 *
 * \todo
 *   \li TTableModel_CString and TStringVector should be subclassed
 *       with an extra step, which provides an abstraction to C arrays
 *       and STL vectors
 *   \li 'setModel' seems to be the same code in most situations: find
 *       a way to put it into the template
 *   \li show focus mark
 *   \li model & render can have different size, use this to set
 *       SELECT_PER_ROW & SELECT_PER_COL automatically
 *   \li insert/remove col/row
 *   \li implement SELECT_PER_ROW & SELECT_PER_COL
 *   \li invalidateChangedArea
 *   \li only scrolling with the scrollbars is optimized, using the keyboard
 *       to move the cursor or to mark an area is still missing
 *   \li cellspacing & cellpadding, border, induvidual border for each side
 *       of each cell
 *   \li select multiple entries with mouse
 *   \li hooks for insert, delete, copy, cut, ...
 *   \li resize column and/or row
 *   \li sorting: 
 *       the model must provide a comparasion method for this
 *   \li reorder columns and rows:
 *       a model to store the order has to be used by table or adapter
 *   \li edit table:
 *       edit cell, row, column; delete row, column
 */

/**
 * Returns a hint for TTable about the selection models
 * capabilities.
 *
 * \li
 *   SINGLE: Only one value at a time can be selected.
 * \li
 *   SINGLE_INTERVAL: Only a single continues interval can
 *   be selected.
 * \li
 *   MULTIPLE_INTERVAL: There's no limitation on how entries
 *   can be selected.
 *
 * The default implementation returns 'SINGLE'.
 */
TAbstractSelectionModel::ESelectionMode
TAbstractSelectionModel::getMode() const
{
  return SINGLE;
}

/**
 * Select the specified rectangle, which will be used for the 'interval'
 * selection modes.
 *
 * The default implementation is to call setSelection(col, row).
 *
 * An new implementation should also call sigChanged().
 */
void
TAbstractSelectionModel::select(size_t col, size_t row, size_t w, size_t h)
{
  select(col, row);
}

/**
 * XOR the selection at the given position.
 *
 * The default implementation is to do nothing.
 *
 * An new implementation should also call sigChanged().
 */
void
TAbstractSelectionModel::toggle(size_t col, size_t row)
{
  select(col, row);
}

/**
 * Clear the selection.
 *
 * The default implementation is to do nothing.
 *
 * An new implementation should also call sigChanged().
 */
void
TAbstractSelectionModel::clear()
{
}

/**
 * Indicates whether any entries are selected or not.
 *
 * The default implementation is to return 'false'.
 *
 * \return 'true' in case there's no entry selected.
 */
bool
TAbstractSelectionModel::empty() const
{
  return false;
}


/**
 * A basic selection model for a single position.
 */

bool
TSingleSelectionModel::getFirst(size_t *x, size_t *y) const
{
  if (!emptyflag)
    return false;
  *x = col;
  *y = row;
  return true;
}

bool
TSingleSelectionModel::getNext(size_t *x, size_t *y) const
{
  return false;
}
    
TAbstractSelectionModel::ESelectionMode
TSingleSelectionModel::getMode() const
{ 
  return SINGLE;
}
   
void
TSingleSelectionModel::clear()
{ 
  col = row = 0;
  if (!emptyflag)
    return;
  emptyflag = false;
  sigChanged();
}

void
TSingleSelectionModel::select(size_t col, size_t row)
{
  if (emptyflag && this->col == col && this->row == row)
    return;
  emptyflag = true;
  this->col = col;
  this->row = row;
  sigChanged();
}

void
TSingleSelectionModel::select(size_t col, size_t row, size_t w, size_t h)
{
  select(col, row);
}

void
TSingleSelectionModel::toggle(size_t col, size_t row)
{
  if (isSelected(col, row))
    clear();
  else
    select(col, row);
}

bool
TSingleSelectionModel::isSelected(size_t col, size_t row) const
{
  return emptyflag && this->col == col && this->row == row;
}

bool
TSingleSelectionModel::empty() const
{
  return !emptyflag;
}


bool 
TRectangleSelectionModel::getFirst(size_t *x, size_t *y) const
{
  if (emptyflag)
    return false;
  *x = x1;
  *y = y1;
  return true;
}

bool
TRectangleSelectionModel::getNext(size_t *x, size_t *y) const
{
  ++(*x);
  if (*x<=x2)
    return true;
  *x = x1;
  ++(*y);
  if (*y<=y2)
    return true;
  return false;
}



/**
 * @ingroup table
 * @class toad::TSelectionModel
 * 
 * This class represents the selected items in a list.
 *
 * @note
 *   Somehow similar to TRegion...
 */
bool
TSelectionModel::getFirst(size_t *x, size_t *y) const
{
  return false;
}

bool
TSelectionModel::getNext(size_t *x, size_t *y) const
{
  return false;
}


TSelectionModel::iterator::iterator()
{
  rgn = NULL;
  n = EMPTY;
  x = y = 0;
}

TSelectionModel::iterator::iterator(const iterator &a)
{
  rgn = a.rgn;
  n   = a.n;
  x   = a.x;
  y   = a.y;
}

TSelectionModel::iterator::iterator(TRegion *r, bool begin)
{
  rgn = r;
  n = rgn->getNumRects();
  if (n==0) {
    n=EMPTY;
    x = y = 0;
    return;
  }
  if (begin) {
    n=0;
    getRect();
    x=x1;
    y=y1;
  } else {
    n = END;
    x = y = 0;
  }
}

void
TSelectionModel::iterator::getRect()
{
  TRectangle rect;
  rgn->getRect(n, &rect);
  DBM(cout << "getRect " <<n<<": "<<rect.x<<", "<<rect.y<<", "<<rect.w<<", "<<rect.h<<endl;)
  x1 = rect.x;
  y1 = rect.y;
  x2 = x1 + rect.w - 1;
  y2 = y1 + rect.h - 1;
}
    
TSelectionModel::iterator
TSelectionModel::iterator::operator++()
{
  if (n==EMPTY || n==END)
    return *this;
    
  DBM(cout << __PRETTY_FUNCTION__ << ": in "
    <<x1<<"<="<<x<<"<="<<x2<<", "
    <<y1<<"<="<<y<<"<="<<y2<<endl;)
  
  x++;
  if (x>x2) {
    x=x1;
    y++;
    if (y>y2) {
      n++;
      if (n<rgn->getNumRects()) {
        getRect();
        x=x1;
        y=y1;
      } else {
        n=END;
        x = y = 0;
      }
    }
  }
  
  DBM(cout << __PRETTY_FUNCTION__ << ": out "
    <<x1<<"<="<<x<<"<="<<x2<<", "
    <<y1<<"<="<<y<<"<="<<y2<<endl;)
  
  return *this;
}

TSelectionModel::iterator
TSelectionModel::iterator::operator--()
{
  if (n==EMPTY)
    return *this;
  if (n==END) {
    n = rgn->getNumRects()-1;
  }

  x--;
  if (x<x1) {
    x=x2;
    y--;
    if (y<y1) {
      n--;
      if (n<0) {
        getRect();
        x=x2;
        y=y2;
      } else {
        n=0;
        x=x1;
        y=y1;
      }
    }
  }
  return *this;
}

void
TSelectionModel::setMode(ESelectionMode mode)
{
  selection_mode = mode;
  sigChanged();
}

void
TSelectionModel::clear()
{
  if (region.isEmpty())
    return;
  region.clear();
  if (!region.isEmpty())
    cerr << "error: region isn't empty!!!" << endl;
  sigChanged();
}

/**
 * Select a single entry.
 */
void
TSelectionModel::select(size_t x, size_t y)
{
  TRectangle r(x,y,1,1);
  region.addRect(r);
  sigChanged();
}

void
TSelectionModel::select(size_t x, size_t y, size_t w, size_t h)
{
  if (selection_mode!=MULTIPLE_INTERVAL)
    region.clear();
  region.addRect(x,y,w,h);
  sigChanged();
}

void
TSelectionModel::toggle(size_t x, size_t y)
{
DBM2(cerr << "toggle " << x << ", " << y << endl;)
  if (selection_mode!=MULTIPLE_INTERVAL) {
DBM2(cerr << "  not multiple interval, clear selection" << endl;)
    region.clear();
  }
DBM2(cerr << "  position before change " << isSelected(x,y) << endl;)
  TRectangle r(x,y,1,1);
  region ^= r;
DBM2(cerr << "  position after change " << isSelected(x,y) << endl;)
  sigChanged();
}

bool
TSelectionModel::isSelected(size_t x, size_t y) const
{
  return region.isInside(x, y);
}

/**
 * @ingroup table
 * \class TDefaultTableHeaderRenderer
 *
 * Objects of this class can be used to draw the table row and column
 * header. Numeric headers are used per default, Alphabetic headers can
 * be selected by passing 'false' to the constructor.
 *
 * Custom text can be selected using the 'setText' method which will be
 * used instead of the numeric or alphabetic header in case the text for
 * the position isn't empty. (Thus to set an empty header, pass a string
 * containing one or more blanks.)
 */

TDefaultTableHeaderRenderer::~TDefaultTableHeaderRenderer()
{
  for(vector<TFigure*>::iterator p = figures.begin();
      p != figures.end();
      ++p)
    if (*p)
      delete *p;
}

void
TDefaultTableHeaderRenderer::setText(unsigned pos, const string &txt) {
  setFigure(pos, new TFText(0,0,txt));
}

void
TDefaultTableHeaderRenderer::setImage(unsigned pos, const string &filename) {
  setFigure(pos, new TFImage(filename));
}

void
TDefaultTableHeaderRenderer::setFigure(unsigned pos, TFigure *figure) {
  if (figures.size()<=pos) {
    vector<TFigure*>::size_type i = figures.size();
    figures.resize(pos+1);
    for(;i<=pos;++i)
      figures[i]=0;
  }
  figures[pos] = figure;
}
 
int
TDefaultTableHeaderRenderer::getHeight()
{
  return TOADBase::getDefaultFont().getHeight()+4;
}

int
TDefaultTableHeaderRenderer::getWidth()
{
  return 42;
}

void
TDefaultTableHeaderRenderer::renderItem(TPen &pen, size_t idx, int w, int h)
{
  TRectangle r(0,0,w,h);
  char buffer[16];
  string str;
  const char *txt = 0;
  TFigure *fig = 0;
  int x, y;
  if (idx<figures.size() && figures[idx]) {
    fig = figures[idx];
    TRectangle r;
    fig->getShape(&r);
    x = (w - r.w)/2;
    y = (h - r.h)/2;
  } else
  if (numeric) {
    snprintf(buffer, 15, "%i", idx+1);
    txt = buffer;
  } else {
    do {
      char c = (idx%26)+'A';
      str.insert(str.begin(), 1, c);
      idx/=26;
    } while(idx>0);
    txt = str.c_str();
    x = (w - pen.getTextWidth(txt))/2;
    y = (h - pen.getHeight())/2;
  }
  
  if (x<0)
    x=0;
    
  pen.setColor(TColor::BTNFACE);
  pen.fillRectanglePC(r);
  pen.setColor(TColor::BTNTEXT);
  if (txt)
    pen.drawString(x,y, txt);
  if (fig) {
    pen.translate(x,y);
    fig->paint(pen);
    pen.translate(-x, -y);
  }
  pen.draw3DRectanglePC(r,false);
}

/**
 * @ingroup table
 * @class toad::TTable
 *
 * This class is a widget to display tables and is also used for listboxes
 * and comboboxes.
 *
 * It uses TTableAdapter to render the table items and 
 * TSelectionModel to manage the selections.
 */

TTable::TTable(TWindow *p, const string &t): 
  super(p, t) 
{
  adapter = NULL;
  selection = 0;
  border = 0;
  cx = cy = 0;
  sx = sy = 0;
  ffx = ffy = 0;
  fpx = fpy = 0;
  feven = true;
  row_info = col_info = NULL;
  rows = cols = 0;
  row_header_renderer = col_header_renderer = NULL;
  selecting = false;
  start_selecting = false;
  stretchLastColumn = true;
  noCursor = false;
  selectionFollowsMouse = false;
  bNoBackground = true;
  setSelectionModel(new TSingleSelectionModel);
}

#if 0
void
TTable::setModel(TTableModel *m) 
{
  if (m==model)
    return;
  model = m;
  if (adapter)
    adapter->setModel(m);
  handleNewModel();
}
#endif

namespace toad {
class TTableDropSite:
  public TDropSite
{
    typedef TDropSite super;
    TTable *table;
  public:
    TTableDropSite(TTable *p):super(p), table(p) {};
    TTableDropSite(TTable *p, const TRectangle &r):super(p,r), table(p) {};
  protected:
    void dropRequest(TDnDObject&);
    void drop(TDnDObject&);
    void paint();
};
} // namespace toad

void
TTableDropSite::dropRequest(TDnDObject &obj)
{
  obj.action = ACTION_NONE;
  table->dropRequest(obj);
}

void
TTableDropSite::drop(TDnDObject &obj)
{
  table->drop(obj);
}

void
TTableDropSite::paint()
{
  // no operation, TTableAdapter shall do this during dropRequest
}

void
TTable::dropRequest(TDnDObject &obj)
{
  if (adapter)
    adapter->dropRequest(obj);
}

void
TTable::drop(TDnDObject &obj)
{
  if (adapter)
    adapter->drop(obj);
}

void
TTable::setAdapter(TTableAdapter *r) 
{
  if (r==adapter)
    return;
  if (adapter) {
    disconnect(adapter->sigChanged, this, &TTable::adapterChanged);
    adapter->setTable(0);
    // adapter->setModel(0);
  }
  adapter = r;
  if (adapter) {
    connect(adapter->sigChanged, this, &TTable::adapterChanged);
    adapter->setTable(this);
//    if (model)
//      adapter->setModel(model);
    if (adapter->canDrag()) {
      new TTableDropSite(this); // we need a way to delete dropsites!
    }
  }
  handleNewModel();
}

void
TTable::setSelectionModel(TAbstractSelectionModel *m)
{
  if (selection==m)
    return;
  if (selection)
    disconnect(selection->sigChanged, this);
  selection = m;
  if (selection) {
    connect(selection->sigChanged, this, &TTable::selectionChanged);
  }
  selectionChanged();
}

void
TTable::selectionChanged()
{
#if 0
  if (selection) {
    // stupid hack just in case someone is using sx and sy...
    // the selection model could supply these values?
    sx = sy = 0;
    for(int y=0; y<rows; ++y) {
      for(int x=0; x<cols; ++x) {
        if (selection->isSelected(x, y)) {
          _setSXSY(sy,sx);
          sy=y;
          sx=x;
          break;
        }
      }
    }
  }
#endif
  if (!adapter)
    return;
  if (selection && !selection->empty() && selectionFollowsMouse) {
    size_t x, y;
    selection->getFirst(&x, &y);
//    cout << "adjust cursor to selection " << x << ", " << y << endl;

#if 1    
    // selection changed may be delivered before adapter changed...
    // which is a bad thing but can we prevent it? we must!
    if (cols != adapter->getCols() ||
        rows != adapter->getRows() )
    {
      cout << "warning: selection change signaled before adapter change" << endl;
    }
    size_t c = cols, r = rows;
    cols = adapter->getCols(); rows = adapter->getRows();

    // 'setCursor(x, y)' without invalidate
    if (x>=cols)
      x = x ? x-1 : 0;
    if (y>=rows)
      y = y ? y-1 : 0;
    cx = x; cy = y;
    if (selectionFollowsMouse) {
      _setSXSY(cx, cy);
    }

    cols = c; rows = r;
#else    
    setCursor(x, y);
#endif
//    cout << "  new cursor pos is " << cx << ", " << cy << endl;
  }
  invalidateWindow();
  sigSelection();
}

void
TTable::setRowHeaderRenderer(TAbstractTableHeaderRenderer *r)
{
  if (row_header_renderer==r)
    return;
  row_header_renderer = r;
  setAllMouseMoveEvents(row_header_renderer || col_header_renderer);
  doLayout();
  invalidateWindow();
}

void
TTable::setColHeaderRenderer(TAbstractTableHeaderRenderer *r)
{
  if (col_header_renderer==r)
    return;
  col_header_renderer = r;
  setAllMouseMoveEvents(row_header_renderer || col_header_renderer);
  doLayout();
  invalidateWindow();
}

/**
 * Called when the scrollbars are moved.
 *
 * Scrolls the table window and recalculates some internal variables.
 */
void
TTable::scrolled(TCoord dx, TCoord dy)
{
  // adjust (ffx, ffy) and (fpx, fpy)

  fpx+=dx;
  fpy+=dy;

  if (dx<0) { // right
    while( ffx < cols && fpx+col_info[ffx].size < 0 ) {
      fpx+=col_info[ffx].size + border;
      ++ffx;
    }
  } else
  if (dx>0) { // left
    while( ffx > 0 && fpx > 0 ) {
      --ffx;
      fpx-=col_info[ffx].size + border;
    }
  }

  if (dy<0) { // down
    while( ffy < rows && fpy+row_info[ffy].size < 0 ) {
      fpy+=row_info[ffy].size + border;
      if (row_info[ffy].size)
        feven = !feven;
      ++ffy;
    }
  } else
  if (dy>0) { // up
    while( ffy > 0 && fpy > 0 ) {
      --ffy;
      if (row_info[ffy].size)
        feven = !feven;
      fpy-=row_info[ffy].size + border;
    }
  }
}

void
TTable::invalidateCursor()
{
  if (cx >= cols || cy >= rows)
    return;

  int xp, yp;
  xp = fpx + visible.x;
  yp = fpy + visible.y;

  for(int x=ffx; x<cx; x++) {
    xp += col_info[x].size;
  }
  for(int y=ffy; y<cy; y++) {
    yp += row_info[y].size;
  }
  
  int size = col_info[cx].size;
  
  if (stretchLastColumn && cx==cols-1 && xp+size<visible.x+visible.w) {
    size = visible.x+visible.w-xp;
  }
  
  if (selection && selection->perRow()) {
    invalidateWindow(visible.x, yp, visible.w, row_info[cy].size+1);
  } else 
  if (selection && selection->perCol()) {
    invalidateWindow(xp, visible.y, size, visible.h);
  } else {
    invalidateWindow(xp, yp, size, row_info[cy].size+1);
  }
}

/**
 * Invalidate (sx,sy)-(bx,by) - (sx,sy)-(ax,ay).
 *
 * \todo
 *   \li implement this method and improve the comment
 */
void
TTable::invalidateChangedArea(int sx, int sy,
                              int ax, int ay,
                              int bx, int by)
{
#if 1
  invalidateWindow();
#else
  TRegion region;
  
  /* wrong: the coordinates aren't pixels but fields! */
  
  TPoint s(sx, sy);
  TPoint a(ax, ay);
  TPoint b(bx, by);
  
  TRectangle r;
  r.set(s, b);
  region|=r;
  r.set(s, a);
  region-=r;
  invalidateWindow(region);
#endif
}

// clipping: we don't need clipping at all, when we ensure that field content
// is never drawn with the negative area. this way we can fields on the right
// and below overdraw the content from the previous field and thus simulate
// the clipping, which may be expensive
void
TTable::paint()
{
DBM2(cerr << "enter paint" << endl;)
  // 'dummy' is only used until the clipping methods in
  // TPen are improved.
  TRectangle dummy(0,0,getWidth(), getHeight());

  TPen pen(this);

DBSCROLL({
  pen.setColor(255,0,0);
  TRegion *rgn = getUpdateRegion();
  TRectangle r;
  long n = rgn->getNumRects();
  for(long i=0; i<n; i++) {
    rgn->getRect(i, &r);
    pen.fillRectanglePC(r);
  }
  flush();
  sleep(1);
  pen.setColor(0,0,0);
})

  int xp, yp;

  if (col_header_renderer) {
    TRectangle clip(visible.x, 0, visible.w, visible.y);
    pen|=dummy;
    pen&=clip;
    pen&=*getUpdateRegion();
    xp = fpx + visible.x;
    int h = col_header_renderer->getHeight();
    for(int x=ffx; x<cols && xp<visible.x+visible.w; x++) {
      if (col_info[x].size==0)
        continue;
      pen.identity();
      pen.translate(xp,0);
      int size = col_info[x].size;
      if (stretchLastColumn && x==cols-1 && xp+size<visible.x+visible.w)
        size = visible.x+visible.w-xp+1;
      col_header_renderer->renderItem(pen, x, size, h);
      xp+=col_info[x].size;
      if (border) {
        pen.setColor(0,0,0);
        pen.fillRectanglePC(size, 0, border, h);
        xp+=border;
      }
    }
  }

  if (row_header_renderer) {
    TRectangle clip(0, visible.y, visible.x, visible.h);
    pen|=dummy;
    pen&=clip;
    pen&=*getUpdateRegion();
    yp = fpy + visible.y;
    int w = row_header_renderer->getWidth();
    for(int y=ffy; y<rows && yp<visible.y+visible.h; y++) {
      if (row_info[y].size==0)
        continue;
      pen.identity();
      pen.translate(0,yp);
      row_header_renderer->renderItem(pen, y, w, row_info[y].size);
      yp+=row_info[y].size;
      if (border) {
        pen.setColor(0,0,0);
        pen.fillRectanglePC(0, row_info[y].size, w, border);
        yp+=border;
      }
    }
  }

  int x1, y1, x2, y2;    
  if (selecting) {
    x1 = min(sx, cx);
    x2 = max(sx, cx);
    y1 = min(sy, cy);
    y2 = max(sy, cy);
//cout << "sx="<<sx<<", sy="<<sy<<", cx="<<cx<<", cy="<<cy<<endl;
//cout << "fake selection from "<<x1<<"-"<<x2<<", "<<y1<<"-"<<y2<<endl;
  }

  pen|=dummy;
  pen.setColor(TColor::DIALOG);
  pen.identity();

  if (visible.x>0) {
    if (visible.y>0) {
      pen.fillRectanglePC(0, 0, visible.x, visible.y);
    }
    if (visible.y+visible.h<getHeight()) {
      pen.fillRectanglePC(0, visible.y+visible.h,
                          visible.x, getHeight()-visible.y-visible.h);
    }
  }

  if (visible.x+visible.w<getWidth()) {
    if (visible.y>0) {
      pen.fillRectanglePC(visible.x+visible.w, 0,
                          getWidth()-visible.x-visible.w, visible.y);
    }
    if (visible.y+visible.h<getHeight()) {
      pen.fillRectanglePC(visible.x+visible.w, visible.y+visible.h,
                          getWidth()-visible.x-visible.w, getHeight()-visible.y-visible.h);
    }
  }

  pen&=visible;
  pen&=*getUpdateRegion();

  // draw border between the fields
  if (border) {
    TCoord panex, paney;
#if 1
  panex = paney = 0.0;
#else
    getPanePos(&panex, &paney);
    panex&=1; // ???
    paney&=1;
#endif
  
    pen.identity();
    pen.setColor(0,0,0);
    pen.setLineStyle(TPen::DOT);
    
    xp = fpx + visible.x + border/2;
    for(int x=ffx; x<cols && xp<visible.x+visible.w; x++) {
      xp += col_info[x].size;
      pen.drawLine(xp, visible.y-paney, xp, visible.y+visible.h);
      xp += border;
    }
    
    yp = fpy + visible.y + border/2;
    for(int y=ffy; y<rows && yp<visible.y+visible.h; y++) {
      yp += row_info[y].size;
      pen.drawLine(visible.x-panex, yp, visible.x+visible.w, yp);
      yp += border;
    }
    
    pen.setLineStyle(TPen::SOLID);
  }

  if (!adapter) {
    cout << getTitle() << ": no adapter" << endl;
    return;
  }

  TTableEvent te;
  te.cols = cols;
  te.rows = rows;
  te.focus = isFocus();
  te.even  = feven;
  if (selection) {
    te.per_row = selection->perRow();
    te.per_col = selection->perCol();
  } else {
    te.per_row = false;
    te.per_col = false;
  }

  // draw the fields with the table adapter
  yp = fpy + visible.y;
  for(int y=ffy; y<rows && yp<visible.y+visible.h; y++) {
    if (row_info[y].size==0) {
      continue;
    }
    te.even = !te.even;
    xp = fpx + visible.x;
    te.row = y;
    for(int x=ffx; x<cols && xp<visible.x+visible.w; x++) {

      TRectangle check(xp,yp,col_info[x].size, row_info[y].size);
      if (stretchLastColumn && 
          x==cols-1 && 
          xp+col_info[x].size<visible.x+visible.w) 
      {
        check.w = visible.x+visible.w-xp;
      }

      if (!getUpdateRegion() ||
          getUpdateRegion()->isInside(check)!=TRegion::OUT)
      {
        pen.identity();
        pen.translate(xp, yp);
        bool selected=false;
        if (selection) {
          if (selection->perRow()) {
            selected = selection->isSelected(0, y);
          } else
          if (selection->perCol()) {
            selected = selection->isSelected(x, 0);
          } else
            selected = selection->isSelected(x, y);
          DBM2(cerr << "  render " << x << ", " << y << ": has sm: selected=" << selected << endl;)
        } else {
          // no selection model, assume usage of simplified selection
          // with getLastSelectionRow() and getLastSelectionCol()
          selected = (x==sx && y==sy);
          DBM2(cerr << "  render " << x << ", " << y << ": no sm: selected=" << selected << endl;)
        }
        if (selecting && !selected) {
          if (x1<=x && x<=x2 && y1<=y && y<=y2) {
//cout << "  selecting is enabled, fake selected for "<<x<<", "<<y<<endl;
            selected = true;
          }
          if (selection && selection->perRow() && y>=y1 && y<=y2)
            selected = true;
          if (selection && selection->perCol() && x>=x1 && x<=x2)
            selected = true;
        }

DBSCROLL(
  pen.setColor(255,255,255);
  pen.fillRectanglePC(0,0,col_info[x].size, row_info[y].size);
  pen.setColor(0,0,0);
)
        bool cursor = false;
        if (isFocus()) {
          cursor = (cx == x && cy == y) ||
                  (selection && selection->perRow() && cy==y) ||
                  (selection && selection->perCol() && cx==x);
        }
        te.col = x;
        te.row = y;
        te.w = check.w;
        te.h = check.h;
        te.cursor = cursor && !noCursor;
        te.selected = selected;
        te.pen = &pen;
        te.type = TTableEvent::PAINT;
        adapter->tableEvent(te);
      }
      xp += col_info[x].size + border;
    }
    yp += row_info[y].size + border;
  }
  
  // clear unused window region (we must do it on our own because
  // background is disabled to reduce flicker)
  if (!stretchLastColumn && xp<=visible.x+visible.w) {
    pen|=dummy;
    pen.identity();
    pen.setColor(128,64,64);
    pen.setColor(getBackground());
    xp--;
    pen.fillRectanglePC(xp,0,visible.x+visible.w-xp,getHeight());
  }
  if (yp<=visible.y+visible.h) {
    pen|=dummy;
    pen.identity();
    pen.setColor(64,64,128);
    pen.setColor(getBackground());
    // yp--;
    pen.fillRectanglePC(0,yp,getWidth(),visible.y+visible.h-yp);
  }
  DBM2(cerr << "leave paint" << endl << endl;)
}

void
TTable::resize()
{
  doLayout();
  center(CENTER_BOTH);
}

void
TTable::focus(bool)
{
  invalidateWindow();
}

void
TTable::selectAtCursor()
{
DBM2(cerr << "selectAtCursor" << endl;)
  if (selecting)
    return;
  _setSXSY(cx, cy);
  if (selection) {
    if (selectionFollowsMouse) {
      DBM2(cerr << "set selection" << endl;)
      selection->select(sx, sy);
    } else {
      DBM2(cerr << "  toggle selection" << endl;)
      selection->toggle(sx, sy);
    }
  } else {
    selectionChanged();
  }
}

void
TTable::clickAtCursor()
{
  selectAtCursor();
  sigClicked();
}

void
TTable::doubleClickAtCursor()
{
  selectAtCursor();
  sigClicked();
}

void
TTable::setRowHeight(size_t row, int height)
{
  cerr << __PRETTY_FUNCTION__ << " isn't implemented yet" << endl;
}

void
TTable::setColWidth(size_t col, int width)
{
  cerr << __PRETTY_FUNCTION__ << " isn't implemented yet" << endl;
}


/**
 * Convert mouse position into a field position and return 'true' 
 * or return 'false' in case it isn't possible.
 *
 * (mx, my) the position of the mouse
 * (fx, fy) returns the field
 * (rfx, rfy) when not NULL, the mouse position inside the field
 */
bool
TTable::mouse2field(TCoord mx, TCoord my, size_t *fx, size_t *fy, TCoord *rfx, TCoord *rfy)
{
  TCoord pos1, pos2;
  size_t x, y;

  if (!visible.isInside(mx, my)) {
    // code to handle this event outside the visible area is missing
    return false;
  }

  // transform (mx, my) from screen pixel to table pixel coordinates
  mx -= visible.x + fpx;
  my -= visible.y + fpy;

  pos1 = 0;
  for(x=ffx; ; x++) {
    if (x>=cols /* || pos1>visible.x+visible.w */) {
//cerr << __FILE__ << ':' << __LINE__ << endl;  
      return false;
    }
    int size = col_info[x].size;
    pos2 = pos1 + col_info[x].size;
    if (stretchLastColumn && x==cols-1)
      pos2 = visible.x+visible.w;
    if (pos1 <= mx && mx < pos2) {
      break;
    }
    pos1 = pos2;
  }
  if (rfx)
    *rfx = mx - pos1;
  
  pos1 = 0;
  y = ffy;  // first upper left field
  while(true) {
    if (y>=rows /* || pos1>visible.y+visible.h */) {
/*
cerr << __FILE__ << ':' << __LINE__ << endl;  
cerr << " y=" << y
     << " rows=" << rows
     << " pos1=" << pos1
     << " visible.y+visible.h=" << (visible.y+visible.h)
     << endl;
*/
      return false;
    }
    pos2 = pos1 + row_info[y].size;
    if (pos1 <= my && my < pos2) {
      break;
    }
    pos1 = pos2;
    ++y;
  }
  if (rfy)
    *rfy = my - pos1;

/*
  if (selection&&selection->perRow())
    x=0;
  if (selection&&selection->perCol())
    y=0;
*/
  *fx = x;
  *fy = y;
  
  return true;
}

void
TTable::mouseEvent(const TMouseEvent &me)
{
  if (me.type == TMouseEvent::ROLL_UP ||
      me.type == TMouseEvent::ROLL_UP_END ||
      me.type == TMouseEvent::ROLL_DOWN ||
      me.type == TMouseEvent::ROLL_DOWN_END)
  {
    TScrollPane::mouseEvent(me);
    return;
  }

  // handle resizing of columns
  //----------------------------
  bool between_h = false;
  static unsigned state = 0;
  static int col;
  static int osize;
  static int mdown;
  static int opane;

  switch(state) {
    case 0:
    case 1:
      int colx;
      if (col_header_renderer) {
        TRectangle clip(visible.x, 0, visible.w, visible.y);
        if (clip.isInside(me.x, me.y)) {
          int xp = fpx + visible.x;
          int h = col_header_renderer->getHeight();
          for(int x=ffx; x<cols && xp<visible.x+visible.w; x++) {
            int size = col_info[x].size;
            if (stretchLastColumn && x==cols-1 && xp+size<visible.x+visible.w)
              size = visible.x+visible.w-xp+1;
//            cout << "xp="<<xp<<", size="<<size<<", mx="<<me.x<<endl;
            if (xp+1 <= me.x && me.x <= xp+size-2) {
//              cout << "  inside " << x << endl;
            } else
            if (xp+size-1<=me.x && me.x<=xp+size+1+border) {
//              cout << "between" << endl;
              colx = x;
              between_h = true;
            }
            xp+=col_info[x].size;
            if (border) {
              xp+=border;
            }
          }
        }
      }
      if (state == 0 && between_h) {
        TWindow::setCursor(TCursor::HORIZONTAL);
        state = 1;
//        cout << "into between" << endl;
      } else
      if (state == 1 && !between_h) {
        TWindow::setCursor(TCursor::DEFAULT);
        state = 0;
//        cout << "out of between" << endl;
      }
      if (state==1 && me.type == TMouseEvent::LDOWN) {
        state = 2;
        col = colx;
        osize = col_info[col].size;
        opane = pane.w;
        mdown = me.x;
//        cout << "grep between " << col << endl;
      }
      break;
    case 2:
      if (me.type==TMouseEvent::LUP) {
        state = 1;
//        cout << "ungrep between" << endl;
      } else
      if (me.type==TMouseEvent::MOVE) {
//        cout << "move col "<<col<<" between" << endl;
//        cout << "  dx=" << (osize+me.x-mdown) << endl;
        col_info[col].size = osize+me.x-mdown;
        if (col_info[col].size<3)
          col_info[col].size=3;
        pane.w = opane - osize + col_info[col].size;
        invalidateWindow();
        doLayout();
      }
      break;
  }
  if (state!=0)
    return;

  // TWindow::setCursor(TCursor::DEFAULT);
  if (me.type == TMouseEvent::LUP) {
    mouseLUp(me);
  } else {
    TWindow::mouseEvent(me);
  }
}

//#warning "mouseLDown must not change the selection, only mouseLUp should"
//#warning "do this!"
void
TTable::mouseLDown(const TMouseEvent &me)
{
  setFocus();

  size_t x, y;   // field
  TCoord fx, fy; // mouse within field
  if (!mouse2field(me.x, me.y, &x, &y, &fx, &fy)) {
    return;
  }
  // invoke adapter mouseEvent (ie. for tree widgets, check boxes, etc.)
  if (adapter) {
    TTableEvent te;
    
    te.mouse.window = this;
    te.mouse.x = fx;
    te.mouse.y = fy;
    te.mouse._modifier = me.modifier();
    te.mouse.type = me.type;
    te.mouse.dblClick = me.dblClick;
    // this should also contain a pointer to this adapter, in case
    // mouseEvent makes modifications?
    int size = col_info[x].size;
    if (stretchLastColumn && x==cols-1) {
      int xp;
      xp = fpx + visible.x;
      for(int _x=ffx; _x<x; _x++) {
        xp += col_info[_x].size;
      }
      if (xp+size<visible.x+visible.w)
        size = visible.x+visible.w-xp;
    }
    te.col = x;
    te.row = y;
    te.w   = size;
    te.h   = row_info[y].size;
    te.type= TTableEvent::MOUSE;
    adapter->tableEvent(te);
  }

  sigPressed();

  if (selection)
    selection->sigChanged.lock();

  TAbstractSelectionModel::ESelectionMode mode;
  mode = selection ? selection->getMode() : TAbstractSelectionModel::SINGLE;

  selecting = start_selecting = false;
  bool justcursor = true;
  if (me.modifier()&MK_SHIFT) {
    if (mode != TAbstractSelectionModel::SINGLE) {
      // start selecting an area from the previous cursor position
//      cout << "  start selecting area" << endl;
      _setSXSY(cx, cy);
      start_selecting = true;
    } else {
      // single selection
      _setSXSY(x, y);
    }
    justcursor = false;
    invalidateWindow();
  } else 
  if (selectionFollowsMouse) {
    _setSXSY(x, y);
    if (mode != TAbstractSelectionModel::SINGLE)
      start_selecting = true;
    justcursor = false;
  }

  if (!justcursor && selection) {
    if (mode == TAbstractSelectionModel::SINGLE) {
      if (!(me.modifier()&MK_CONTROL)) {
        if (!selection->isSelected(x, y)) {
          selection->clear();
          _setSXSY(x, y);
          selection->select(sx, sy);
        }
      } else {
//        cout << "  toggle selection" << endl;
        _setSXSY(x, y);
        selection->toggle(sx, sy);
      }
    } else {
      if (!(me.modifier()&MK_CONTROL)) {
        selection->clear();
        if (mode != TAbstractSelectionModel::SINGLE) {
          selecting = true;
          start_selecting = false;
        }
      } else {
        _setSXSY(x, y);
        selection->toggle(sx, sy);
      }
    }
  }
  invalidateWindow();

  invalidateCursor();
  cx = x; cy = y;
  invalidateCursor();
  sigCursor();
    
  DBM2(cerr << "leave mouseLDown" << endl << endl;)
}

void
TTable::mouseMove(const TMouseEvent &me)
{
  if (!(me.modifier()&MK_LBUTTON))
    return;

  size_t x, y;
  if (!mouse2field(me.x, me.y, &x, &y)) {
    DBM2(cerr << "  mouse2field failed" << endl;
    cout << "leave mouseMove" << endl << endl;)
    return;
  }

  // experimental drag'n drop
  if (adapter && adapter->canDrag() &&
      selection && selection->isSelected(x, y)) 
  {
    startDrag(new TDnDTable(this), MK_SHIFT /* move */);
    return;
  }

//  cout << "mouse move on field " << x << ", " << y << endl;

  if (cx==x && cy==y) {
    DBM2(cerr << "  cursor position wasn't modified" << endl;
    cout << "leave mouseMove" << endl << endl;)
    return;
  }

  if (start_selecting) {
    start_selecting = false;
    selecting = true;
  } else
  if (!selecting && (me.modifier()&MK_SHIFT)) {
    TAbstractSelectionModel::ESelectionMode mode;
    mode = selection ? selection->getMode() : TAbstractSelectionModel::SINGLE;
    if (mode != TAbstractSelectionModel::SINGLE) {
      // start selecting an area from the previous cursor position
//      cout << "  start selecting area" << endl;
      _setSXSY(cx, cy);
      selecting = true;

      if (!(me.modifier()&MK_CONTROL)) {
        selection->clear();
      }

    } else {
      // single selection
      _setSXSY(x, y);
    }
    invalidateWindow();
  }

//cout << "MOUSE move" << endl;

  TAbstractSelectionModel::ESelectionMode mode;
  mode = selection ? selection->getMode() : TAbstractSelectionModel::SINGLE;

  if (selectionFollowsMouse || me.modifier() & MK_SHIFT) {
    invalidateChangedArea(sx,sy,cx,cy,cx,cy);
    cx = x; cy = y;
    if (!selecting) {
      _setSXSY(x, y);
    }
    if (selection && mode==TAbstractSelectionModel::SINGLE) {
      selection->select(sx, sy);
    }
    invalidateChangedArea(sx,sy,cx,cy,cx,cy);
  } else {
    invalidateCursor();
    cx = x; cy = y;
    invalidateCursor();
  }
  
  sigCursor();
  DBM2(cerr << "leave mouseMove" << endl << endl;)
}

void
TTable::mouseLUp(const TMouseEvent &me)
{
DBM2(cerr << "enter mouseLUp" << endl;)
  size_t x, y;
  if (!mouse2field(me.x, me.y, &x, &y)) {
    x = cx;
    y = cy;
  }

//cout << "MOUSE up" << endl;

  if (selection && selecting) {
    size_t x1, y1, x2, y2;    
    x1 = min(sx, cx);
    x2 = max(sx, cx);
    y1 = min(sy, cy);
    y2 = max(sy, cy);
//    cout << "  set selection" << endl;
    selection->select(x1, y1, x2-x1+1, y2-y1+1);
    selecting = false;
    invalidateWindow();
  }
  
  if (me.dblClick)
    sigDoubleClicked();
  else  
    sigClicked();

  if (selection)
    selection->sigChanged.unlock();

  DBM2(cerr << "leave mouseLUp" << endl << endl;)
}

/**
 * Adjust scrollbars so that the cursor is visible.
 */
void
TTable::center(int how)
{
  if (cols==0 || rows==0)
    return;

  TCoord panex, paney;
  panex = paney = -1;
  getPanePos(&panex, &paney, false);

  if (paney!=-1 && how&CENTER_VERT) {
    int yp = 0;
    int y;
    for(y=0; y<cy; y++) {
      yp += row_info[y].size + border;
    }
    
    int y1 = paney;
    int y2 = y1 + visible.h;
    
    if (yp<=y1) {
      paney = yp;
    } else {
      yp += row_info[y].size + border;
      if (yp>y2) {
        paney = yp-visible.h;
      }
    }
  }

  if (panex!=-1 && how&CENTER_HORZ) {
    int xp = 0;
    int x;
    for(x=0; x<cx; x++) {
      xp += col_info[x].size + border;
    }
    
    int x1 = panex;
    int x2 = x1 + visible.w;
      
    if (xp<=x1) {
      panex = xp;
    } else {
      xp += col_info[x].size + border;
      if (xp>x2) {
        panex = xp-visible.w;
      }
    }
  }
  
  setPanePos(panex, paney);

  invalidateWindow();
}

void
TTable::setCursor(size_t col, size_t row)
{
  if (col>=cols)
    col = cols ? cols-1 : 0;
  if (row>=rows)
    row = rows ? rows-1 : 0;
  if (cx == col && cy == row)
    return;
  invalidateCursor();
  cx = col; cy = row;
  if (selectionFollowsMouse) {
    _setSXSY(cx, cy);
  }
  invalidateCursor();
}

void
TTable::_moveCursor(size_t newcx, size_t newcy, unsigned modifier)
{
  int how = 0;
  if (cols==0 || rows==0)
    return;
  
  if (newcx>=cols)
    newcx=cols-1;
  if (newcy>=rows)
    newcy=rows-1;
  
  if (newcx!=cx)
    how |= CENTER_HORZ;
  if (newcy!=cy)
    how |= CENTER_VERT;
  if (how==0)
    return;

  TAbstractSelectionModel::ESelectionMode mode;
  mode = selection ? selection->getMode() : TAbstractSelectionModel::SINGLE;

  if (selecting && !(modifier&MK_SHIFT)) {
//    cout << "stop select area" << endl;
    selecting = false;
  }
  
  if (!selecting) {
    if (mode!=TAbstractSelectionModel::SINGLE && (modifier & MK_SHIFT)) {
//      cout << "start select area" << endl;
      if (selection && !(modifier & MK_CONTROL) ) {
//        cout << "  clear selection" << endl;
//        cout << "  cx="<<cx<<", cy="<<cy<<", sx="<<sx<<", sy="<<sy<<endl;
        selection->clear();
      }
      _setSXSY(cx, cy);
//      cout << "  create selection starting at" << endl;
//      cout << "  cx="<<cx<<", cy="<<cy<<", sx="<<sx<<", sy="<<sy<<endl;
      selecting = true;
    }
    cy = newcy;
    cx = newcx;
    if (selectionFollowsMouse && !(modifier&MK_CONTROL)) {
      selectAtCursor();
    }
//    invalidateCursor();
invalidateWindow();
    sigCursor();
  } else {
    invalidateChangedArea(sx,sy,cx,cy,cx,newcy);
    cy = newcy;
    cx = newcx;
  }
  center(how);
}

void
TTable::keyEvent(const TKeyEvent &ke)
{
  if (adapter) {
    TTableEvent te;
    te.type = TTableEvent::KEY;
    te.key = &ke;
    te.col = cx;
    te.row = cy;
    te.flag= false;
    adapter->tableEvent(te);
    if (te.flag) {
      invalidateCursor();
      return;
    }
  }
  TWindow::keyEvent(ke);
}

void
TTable::keyDown(const TKeyEvent &ke)
{
//cout << "keyDown: enter: sx="<<sx<<", sy="<<sy<<endl;
  switch(ke.key()) {
    case TK_DOWN: {
      int newcy = cy+1;
      while(newcy<rows && row_info[newcy].size==0)
        ++newcy;
      _moveCursor(cx, newcy, ke.modifier());
    } break;
    case TK_UP: {
      int newcy = cy;
      while(newcy>0 && row_info[--newcy].size==0)
        ;
      _moveCursor(cx, newcy, ke.modifier());
    } break;
    case TK_PAGEUP:
      pageUp();
      break;
    case TK_PAGEDOWN:
      pageDown();
      break;
    case TK_RIGHT: {
      int newcx = cx+1;
      while(newcx<cols && col_info[newcx].size==0)
        ++newcx;
      _moveCursor(newcx, cy, ke.modifier());
    } break;
    case TK_LEFT: {
      int newcx = cx;
      while(newcx>0 && col_info[--newcx].size==0)
        ;
      _moveCursor(newcx, cy, ke.modifier());
    } break;
    case TK_RETURN:
      selectAtCursor();
      sigDoubleClicked();
      break;
    case ' ': {
      size_t osx=sx, osy=sy;
      _setSXSY(cx, cy);
      if (selection) {
//        cout << "SPACE: toggle selection" << endl;
        if (!(ke.modifier()&MK_CONTROL)) {
          bool flag = selection->isSelected(sx, sy);
          selection->clear();
          if (!(ke.modifier()&MK_SHIFT) ) {
            if (!flag)
              selection->select(sx, sy);
          } else {
            size_t x1, y1, x2, y2;    
            x1 = min(osx, sx);
            x2 = max(osx, sx);
            y1 = min(osy, sy);
            y2 = max(osy, sy);
//            cout << "  set selection" << endl;
            selection->select(x1, y1, x2-x1+1, y2-y1+1);
          }
        } else {
//          cout << "  toggle selection" << endl;
          selection->toggle(sx, sy);
        }
      }
//      invalidateCursor();
      invalidateWindow();
      } break;
    case TK_SHIFT_L:
#ifndef __COCOA__
    case TK_SHIFT_R:
#endif
      if (selection)
        selection->sigChanged.lock();
      break;
  }
//cout << "keyDown: leave: sx="<<sx<<", sy="<<sy<<endl;
}

void
TTable::_setSXSY(size_t x, size_t y)
{
  sx = x;
  sy = y;
  if (selection) {
    if (selection->perRow())
      sx=0;
    if (selection->perCol())
      sy=0;
  }
//cout << "_setSXSY("<<x<<", "<<y<<") -> "<<sx<<", "<<sy<<endl;
}

void
TTable::keyUp(const TKeyEvent &ke)
{
  switch(ke.key()) {
    case TK_SHIFT_L:
#ifndef __COCOA__
    case TK_SHIFT_R:
#endif
//      cout << "SHIFT up" << endl;
      if (selection && selecting) {
//        cout << "  stop selecting and set selection" << endl;
        size_t osx=sx, osy=sy;
        _setSXSY(cx, cy);
        selecting=false;
        size_t x1, y1, x2, y2;    
        x1 = min(osx, sx);
        x2 = max(osx, sx);
        y1 = min(osy, sy);
        y2 = max(osy, sy);
        selection->select(x1, y1, x2-x1+1, y2-y1+1);
        sx=osx; sy=osy;
      }
      if (selection)
        selection->sigChanged.unlock();
      break;
  }
}

void
TTable::adapterChanged()
{
  if (!adapter) {
    cerr << "TTable::adapterChanged: adapterChanged for table '" 
         << getTitle()
         << "' but no adapter\n";
    return;
  }
  switch(adapter->reason) {
    case TTableModel::INSERT_ROW:
      if (debug_table>0)
        cout << "table: insert row " << adapter->where << ", " << adapter->size << endl;
      _handleInsertRow();
      break;
    case TTableModel::RESIZED_ROW:
      if (debug_table>0)
        cout << "table: resized row " << adapter->where << ", " << adapter->size << endl;
      _handleResizedRow();
      break;
    case TTableModel::REMOVED_ROW:
      if (debug_table>0)
        cout << "table: removed row " << adapter->where << ", " << adapter->size << endl;
      _handleRemovedRow();
      break;
    case TTableModel::CONTENT:
      if (debug_table>0)
        cout << "table: content" << endl;
      invalidateWindow();
      break;
    default:
      if (debug_table>0)
        cout << "table: new model" << endl;
      handleNewModel();
      break;
  }
}

void
TTable::_handleInsertRow()
{
  // allocate space for the new rows in 'row_info'
  int new_rows = rows + adapter->size;
  row_info = static_cast<TRCInfo*>(realloc(row_info, sizeof(TRCInfo)*new_rows));
  memmove(
    row_info + adapter->where + adapter->size,
    row_info + adapter->where,
    (rows - adapter->where) * sizeof(TRCInfo)
  );

  // initialize the new entries
  TRCInfo *info = row_info + adapter->where;
  TTableEvent te;
  te.type = TTableEvent::GET_ROW_SIZE;
  te.col  = 0;
  for(te.row=adapter->where; te.row<adapter->where+adapter->size; ++te.row) {
    DBM(cout << "pane.h: " << pane.h << endl;)
    info->open = true; // getRowHeight may query this values
    info->size = 0;    // just in case...
//cout << "going to get height of row " << te.row << endl;
//cout << "  open == " << (isRowOpen(i)?"open":"closed") << endl;
    adapter->tableEvent(te);
    info->size = te.h;
    pane.h += te.h + border;
    ++info;
  }
  
  // adjust cy, sy, feven and ffy
  if (adapter->where<cy)
    cy+=adapter->size;
  if (adapter->where<sy)
    sy+=adapter->size;
  if (adapter->where<ffy) {
    for(size_t y=ffy; y<ffy+adapter->size; ++y) {
      if (row_info[y].size)
        feven = !feven;
    }
    ffy+=adapter->size;
  }

  rows = new_rows;
  doLayout();

  // don't invalidate window in case rows where added below current
  // visible area
  if (ffy <= adapter->where) {
    int py = fpy;
    for(size_t y = ffy; y<new_rows; ++y) {
      if (py>visible.h) {
        cout << "return" << endl;
        return;
      }
      if (y==adapter->where)
        break;
      py += row_info[y].size + border;
    }
  }
  
  // invalidate the window (can't scroll because of the different
  // color scheme for odd and even rows)
  invalidateWindow();
}

void
TTable::_handleRemovedRow()
{
//cout << "_handleRemovedRow: where="<<adapter->where<<", size="<<adapter->size<<endl;
  if (adapter->where + adapter->size - 1> rows) {
    cout << "_handleRemovedRow: where=" << adapter->where
         << " and size="<<adapter->size
         << " but only " << rows << " rows." << endl;
    return;
  }
  int new_rows = rows - adapter->size;
  // row_info = static_cast<TRCInfo*>(realloc(row_info, sizeof(TRCInfo)*new_rows));

//cout << "move from " << (adapter->where+adapter->size) << " to " << (adapter->where) << " an amount of " << (rows - adapter->where - adapter->size) << " entries" << endl;
  TRCInfo *info = row_info + adapter->where;
  for(int i=adapter->where; i<adapter->where+adapter->size; ++i) {
    DBM(cout << "pane.h: " << pane.h << endl;)
    pane.h -= info->size + border;
    ++info;
  }

  memmove(
    row_info + adapter->where,
    row_info + adapter->where + adapter->size,
    (rows - adapter->where - adapter->size) * sizeof(TRCInfo)
  );

  if (adapter->where<cy)
    cy+=adapter->size;
  if (adapter->where<sy)
    sy+=adapter->size;
  if (adapter->where<ffy) {
    for(unsigned y=ffy; y<ffy+adapter->size; ++y) {
      if (row_info[y].size)
        feven = !feven;
    }
    ffy+=adapter->size;
  }

  // fpy ...
    
  // scrolling, screen update
      
  // selection model ??? ouch ....

  rows = new_rows;
//cout << "number of rows is now " << rows << endl;
//cout << __FILE__ << ":" << __LINE__ << " rows = "<<rows<<endl;
  setCursor(cx, cy);
  invalidateWindow();

  doLayout();

  if (selection) {  
    if (rows==0) {
      // the obvious check -> no rows, no selection
      selection->clear();
    }
    else if (adapter->where+adapter->size > rows) {
      // the selection model doesn't provide enough information yet
      // to deselect all field being out of range so the following
      // assumption must be enough for now:
      // usually the area removed was also the selected area thus...
      // hey! we should also be able to MOVE the selection as we do
      // with the row and col information... work is lurking out of
      // every corner it seems ;)
//      cout << "design problem: emergency clear selection" << endl;
//      selection->clearSelection();
cout << "no emergency clear selection..." << endl;
    } else {
      size_t x, y;
      if (selection->getFirst(&x, &y)) {
        do {
          if (y>=adapter->where) {
            selection->sigChanged();
            break;
          }
        } while(selection->getNext(&x, &y));
      }
    }
  }
}

void
TTable::_handleResizedRow()
{
  TRCInfo *info = row_info + adapter->where;
  TTableEvent te;
  te.type = TTableEvent::GET_ROW_SIZE;
  te.col = 0;
  for(te.row=adapter->where; te.row<adapter->where+adapter->size; ++te.row) {
    DBM(cout << "pane.h: " << pane.h << endl;)
    pane.h -= info->size;
    adapter->tableEvent(te);
    info->size = te.h;
    pane.h += te.h;
    ++info;
  }
#if 0    
  if (adapter->where<cy)
    cy+=adapter->size;
  if (adapter->where<sy)
    sy+=adapter->size;
  if (adapter->where<ffy)
    ffy+=adapter->size;
#endif
  // fpy ...
    
  // scrolling, screen update
      
  // selection model ??? ouch ....

  invalidateWindow();

  doLayout();
}


/**
 * reset the TTable widget to adjust to a new adapter
 */
void
TTable::handleNewModel()
{
// cout << "TTable::handleNewModel()" << endl;
  invalidateWindow();
//  if (selection)
//    selection->clearSelection();
  cx = cy = 0;
  sy = sy = 0;
  resetScrollPane();
  ffx = ffy = 0;
  fpx = fpy = 0;
  feven = true;
  selecting = false;

//  if (!adapter && model)
//    adapter = model->getDefaultAdapter();
  if (!adapter)
    return;

  cols = adapter->getCols();
  rows = adapter->getRows();

  row_info = rows ? static_cast<TRCInfo*>(realloc(row_info, sizeof(TRCInfo)*rows)) : 0;
  col_info = cols ? static_cast<TRCInfo*>(realloc(col_info, sizeof(TRCInfo)*cols)) : 0;
  
  TRCInfo *info;

  // calculate pane.w
  pane.w=0;
  info = col_info;
  TTableEvent te;
  te.type = TTableEvent::GET_COL_SIZE;
  for(te.col=0; te.col<cols; ++te.col) {
    DBM(cout << "pane.w: " << pane.w << endl;)
    adapter->tableEvent(te);
    info->size = te.w;
    pane.w += te.w + border;
    ++info;
  }
  DBM(cout << "pane.w: " << pane.w << endl;)

  // calculate pane.h
  pane.h=0;
  info = row_info;
  te.type = TTableEvent::GET_ROW_SIZE;
  te.col = 0;
  for(te.row=0; te.row<rows; ++te.row) {
    DBM(cout << "pane.h: " << pane.h << endl;)
    info->open = true;
    info->size = 0;
    adapter->tableEvent(te);
    info->size = te.h;
    pane.h += te.h + border;
    ++info;
  }
  DBM(cout << "pane.h: " << pane.h << endl;)
//cout << __FILE__ << ":" << __LINE__ << " rows = "<<rows<<endl;

  doLayout();
}

void
TTable::adjustPane()
{
  if (row_header_renderer) {
    visible.x = row_header_renderer->getWidth();
    visible.w -= visible.x;
  }
  if (col_header_renderer) {
    visible.y = col_header_renderer->getHeight();
    visible.h -= visible.y;
  }
  setUnitIncrement(cols ? pane.w/cols : 1, rows ? pane.h/rows : 1);
}
