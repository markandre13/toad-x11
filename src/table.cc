/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#include <stdio.h>
#include <unistd.h>

using namespace toad;

#define DBM(M)
#define DBM2(M)
#define DBSCROLL(M)

/**
 * @defgroup table Table
 *
 * This group contains a set of classes to display data in a table.
 *
 * @verbatim
 
The base classes are defined in table.hh:
 
TModel
+- TAbstractTableCellRenderer             
+- TAbstractTableSelectionModel           
|  +- TTableSingleSelectionModel
|  +- TTableSelectionModel
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
 *       a model to store the order has to be used by table or renderer
 *   \li edit table:
 *       edit cell, row, column; delete row, column
 */

void 
TAbstractTableCellRenderer::renderCell(TPen &pen, int col, int row, int w, int h, bool cursor, bool selected, bool focus)
{
  if (selected) {
    if (focus) {
      pen.setColor(TColor::SELECTED);
    } else {
      pen.setColor(TColor::SELECTED_GRAY);
    }
  } else {
    pen.setColor(TColor::WHITE);
  }
  pen.fillRectanglePC(0,0,w,h);
  pen.setColor(TColor::SELECTED_TEXT);
  renderItem(pen, col, row, w, h, cursor, selected, focus);
  if (selected) {
    pen.setColor(TColor::BLACK);
  }
  if (cursor) {
    pen.drawRectanglePC(0,0,w, h);
  }
}

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
TAbstractTableSelectionModel::ESelectionMode
TAbstractTableSelectionModel::getSelectionMode() const
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
TAbstractTableSelectionModel::setSelection(int col, int row, int w, int h)
{
  setSelection(col, row);
}

/**
 * XOR the selection at the given position.
 *
 * The default implementation is to do nothing.
 *
 * An new implementation should also call sigChanged().
 */
void
TAbstractTableSelectionModel::toggleSelection(int col, int row)
{
  setSelection(col, row);
}

/**
 * Clear the selection.
 *
 * The default implementation is to do nothing.
 *
 * An new implementation should also call sigChanged().
 */
void
TAbstractTableSelectionModel::clearSelection()
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
TAbstractTableSelectionModel::isEmpty() const
{
  return false;
}


#if 0
/**
 * A basic selection model for a single position.
 */
    
TAbstractTableSelectionModel::ESelectionMode
TTableSingleSelectionModel::getSelectionMode() const
{ 
  return SINGLE;
}
   
void
TTableSingleSelectionModel::clearSelection()
{ 
}

void
TTableSingleSelectionModel::setSelection(int col, int row)
{
  if (this->col == col && this->row == row)
    return;
  this->col = col;
  this->row = row;
  sigChanged();
}

void
TTableSingleSelectionModel::setSelection(int col, int row, int w, int h)
{
  setSelection(col, row);
}

void
TTableSingleSelectionModel::toggleSelection(int col, int row)
{
  setSelection(col, row);
}

bool
TTableSingleSelectionModel::isSelected(int col, int row) const
{
  return this->col == col && this->row == row;
}

bool
TTableSingleSelectionModel::isEmpty() const
{
  return false;
}
#else
/**
 * A basic selection model for a single position.
 */
    
TAbstractTableSelectionModel::ESelectionMode
TTableSingleSelectionModel::getSelectionMode() const
{ 
  return SINGLE;
}
   
void
TTableSingleSelectionModel::clearSelection()
{ 
  col = row = 0;
  if (!selected)
    return;
  selected = false;
  sigChanged();
}

void
TTableSingleSelectionModel::setSelection(int col, int row)
{
  if (selected && this->col == col && this->row == row)
    return;
  selected = true;
  this->col = col;
  this->row = row;
  sigChanged();
}

void
TTableSingleSelectionModel::setSelection(int col, int row, int w, int h)
{
  setSelection(col, row);
}

void
TTableSingleSelectionModel::toggleSelection(int col, int row)
{
  if (isSelected(col, row))
    clearSelection();
  else
    setSelection(col, row);
}

bool
TTableSingleSelectionModel::isSelected(int col, int row) const
{
  return selected && this->col == col && this->row == row;
}

bool
TTableSingleSelectionModel::isEmpty() const
{
  return !selected;
}
#endif

/**
 * @ingroup table
 * @class toad::TTableSelectionModel
 * 
 * This class represents the selected items in a list.
 *
 * @note
 *   Somehow similar to TRegion...
 */

TTableSelectionModel::iterator::iterator()
{
  rgn = NULL;
  n = EMPTY;
  x = y = 0;
}

TTableSelectionModel::iterator::iterator(const iterator &a)
{
  rgn = a.rgn;
  n   = a.n;
  x   = a.x;
  y   = a.y;
}

TTableSelectionModel::iterator::iterator(TRegion *r, bool begin)
{
  rgn = r;
  n = rgn->getNumRects();
  if (n==0) {
    n==EMPTY;
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
TTableSelectionModel::iterator::getRect()
{
  TRectangle rect;
  rgn->getRect(n, &rect);
  DBM(cout << "getRect " <<n<<": "<<rect.x<<", "<<rect.y<<", "<<rect.w<<", "<<rect.h<<endl;)
  x1 = rect.x;
  y1 = rect.y;
  x2 = x1 + rect.w - 1;
  y2 = y1 + rect.h - 1;
}
    
TTableSelectionModel::iterator
TTableSelectionModel::iterator::operator++()
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

TTableSelectionModel::iterator
TTableSelectionModel::iterator::operator--()
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
TTableSelectionModel::setSelectionMode(ESelectionMode mode)
{
  selection_mode = mode;
  sigChanged();
}

void
TTableSelectionModel::clearSelection()
{
  if (region.isEmpty())
    return;
  region.clear();
  sigChanged();
}

/**
 * Select a single entry.
 */
void
TTableSelectionModel::setSelection(int x, int y)
{
  TRectangle r(x,y,1,1);
  region.addRect(r);
  sigChanged();
}

void
TTableSelectionModel::setSelection(int x, int y, int w, int h)
{
  if (selection_mode!=MULTIPLE_INTERVAL)
    region.clear();
  region.addRect(x,y,w,h);
  sigChanged();
}

void
TTableSelectionModel::toggleSelection(int x, int y)
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
TTableSelectionModel::isSelected(int x, int y) const
{
  return region.isInside(x, y);
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
TDefaultTableHeaderRenderer::renderItem(TPen &pen, int idx, int w, int h)
{
  TRectangle r(0,0,w,h);
  char buffer[16];
  string str;
  const char *txt;
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
  }
  int x = (w - pen.getTextWidth(txt))>>1;
  int y = (h - pen.getHeight())>>1;
      
  pen.setColor(TColor::BTNFACE);
  pen.fillRectanglePC(r);
  pen.setColor(TColor::BTNTEXT);
  pen.drawString(x,y, txt);
  pen.draw3DRectanglePC(r,false);
}

/**
 * @ingroup table
 * @class toad::TTable
 *
 * This class is a widget to display tables and is also used for listboxes
 * and comboboxes.
 *
 * It uses TAbstractTableCellRenderer to render the table items and 
 * TTableSelectionModel to manage the selections.
 */

TTable::TTable(TWindow *p, const string &t): 
  super(p, t) 
{
  renderer = NULL;
  selection = 0;
  border = 0;
  cx = cy = 0;
  sx = sy = 0;
  ffx = ffy = 0;
  fpx = fpy = 0;
  row_info = col_info = NULL;
  rows = cols = 0;
  row_header_renderer = col_header_renderer = NULL;
  selecting = false;
  stretchLastColumn = true;
  noCursor = false;
  selectionFollowsMouse = false;
  bNoBackground = true;
}

void
TTable::setRenderer(TAbstractTableCellRenderer *r) 
{
  if (r==renderer)
    return;
  if (renderer)
    disconnect(renderer->sigChanged, this, &TTable::handleNewModel);
  renderer = r;
  if (renderer)
    connect(renderer->sigChanged, this, &TTable::handleNewModel);
  handleNewModel();
}

void
TTable::setSelectionModel(TAbstractTableSelectionModel *m)
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
  if (selection) {
    // stupid hack just in case someone is using sx and sy...
    // the selection model could supply these values?
    sx = sy = 0;
    for(int y=0; y<rows; ++y) {
      for(int x=0; x<cols; ++x) {
        if (selection->isSelected(x, y)) {
          sy=y;
          sx=x;
          break;
        }
      }
    }
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
  doLayout();
}

void
TTable::setColHeaderRenderer(TAbstractTableHeaderRenderer *r)
{
  if (col_header_renderer==r)
    return;
  col_header_renderer = r;
  doLayout();
}

/**
 * Called when the scrollbars are moved.
 *
 * Scrolls the table window and recalculates some internal variables.
 */
void
TTable::scrolled(int dx, int dy)
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
      ++ffy;
    }
  } else
  if (dy>0) { // up
    while( ffy > 0 && fpy > 0 ) {
      --ffy;
      fpy-=row_info[ffy].size + border;
    }
  }
}

void
TTable::invalidateCursor()
{
  if (cols==0 || rows==0)
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
  
  if (per_row) {
    invalidateWindow(visible.x, yp, visible.w, row_info[cy].size+1);
  } else if (per_col) {
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
      pen.identity();
      pen.translate(xp+1,0);
      int size = col_info[x].size;
      if (stretchLastColumn && x==cols-1 && xp+size<visible.x+visible.w)
        size = visible.x+visible.w-xp+1;
      col_header_renderer->renderItem(pen, x, size-2, h);
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
  }

  pen|=dummy;
  pen&=visible;
  pen&=*getUpdateRegion();

  // draw border between the fields
  if (border) {
    int panex, paney;
    getPanePos(&panex, &paney);
    panex&=1;
    paney&=1;
  
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

  if (!renderer)
    return;

  // draw the fields with the table renderer
  xp = fpx + visible.x;
  for(int x=ffx; x<cols && xp<visible.x+visible.w; x++) {
    yp = fpy + visible.y;
    for(int y=ffy; y<rows && yp<visible.y+visible.h; y++) {
      TRectangle check(xp,yp,col_info[x].size, row_info[y].size);
      if (!getUpdateRegion() ||
          getUpdateRegion()->isInside(check)!=TRegion::OUT)
      {
        pen.identity();
        pen.translate(xp, yp);
        bool selected;
        if (selection) {
          selected = selection->isSelected(per_row?0:x,per_col?0:y);
          DBM2(cerr << "  render " << x << ", " << y << ": has sm: selected=" << selected << endl;)
        } else {
          // no selection model, assume usage of simplified selection
          // with getLastSelectionRow() and getLastSelectionCol()
          selected = (per_row?0:x == sx) && (per_col?0:y == sy);
          DBM2(cerr << "  render " << x << ", " << y << ": no sm: selected=" << selected << endl;)
        }
        if (selecting) {
DBM2(cerr << "  selecting is enabled, fake selected" << endl;)
          if (x>=x1 && x<=x2 && y>=y1 && y<=y2)
            selected = true;
          if (per_row && y>=y1 && y<=y2)
            selected = true;
          if (per_col && x>=x1 && x<=x2)
            selected = true;
        }
        int size = col_info[x].size;
        if (stretchLastColumn && x==cols-1 && xp+size<visible.x+visible.w)
          size = visible.x+visible.w-xp;

DBSCROLL(
  pen.setColor(255,255,255);
  pen.fillRectanglePC(0,0,col_info[x].size, row_info[y].size);
  pen.setColor(0,0,0);
)
        bool cursor = false;
        if (isFocus()) {
          cursor = (cx == x && cy == y) ||
                  (per_row && cy==y) ||
                  (per_col && cx==x);
        }

        renderer->renderCell(
            pen,
            x, y,
            size, row_info[y].size,
            cursor && !noCursor,
            selected,
            isFocus()
        );
      }
      yp += row_info[y].size + border;
    }
    xp += col_info[x].size + border;
  }
  // clear unused window region (we must do it on our own because
  // background is disabled to reduce flicker)
  if (!stretchLastColumn && xp<=visible.x+visible.w) {
    pen|=dummy;
    pen.identity();
    // pen.setColor(128,64,64);
    pen.setColor(getBackground());
    xp--;
    pen.fillRectanglePC(xp,0,visible.x+visible.w-xp,getHeight());
  }
  if (yp<=visible.y+visible.h) {
    pen|=dummy;
    pen.identity();
    // pen.setColor(64,64,128);
    pen.setColor(getBackground());
    yp--;
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
  sx = per_row?0:cx;
  sy = per_col?0:cy;
  if (selection) {
    if (selectionFollowsMouse) {
      DBM2(cerr << "set selection" << endl;)
      selection->setSelection(sx, sy);
    } else {
      DBM2(cerr << "  toggle selection" << endl;)
      selection->toggleSelection(sx, sy);
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

/**
 * Convert mouse position into a field position and return 'true' 
 * or return 'false' in case it isn't possible.
 */
bool
TTable::mouse2field(int mx, int my, int *fx, int *fy)
{
  int pos1, pos2, x, y;

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

  if (per_row)
    x=0;
  if (per_col)
    y=0;

  *fx = x;
  *fy = y;
  
  return true;
}

void
TTable::mouseLDown(int mx, int my, unsigned modifier)
{
DBM2(cerr << "enter mouseLDown" << endl;)
  setFocus();

  int x, y;
  if (!mouse2field(mx, my, &x, &y)) {
    DBM2(cerr << "  mouse2field failed" << endl;
    cerr << "leave mouseLDown" << endl << endl;)
    return;
  }

  DBM(cout << "click on item " << x << ", " << y << endl;);

  sigPressed();

  if (selection && !selection->isEmpty() &&
       ( !(modifier&MK_CONTROL) || 
         selection->getSelectionMode() == TAbstractTableSelectionModel::SINGLE ))
  {
    DBM2(cerr << "  clear the whole selection" << endl;)
    selection->clearSelection();
  }

#if 0
  if ((selectionFollowsMouse || !selection) && cx==x && cy==y) {
    DBM2(cerr << "  assume that nothing has changed" << endl;
    cerr << "leave mouseLDown" << endl << endl;)
    return;
  }  
#endif

  selecting = false;
  if (selection) {
    selecting=true;
    if (modifier&MK_SHIFT && selection->getSelectionMode() != TAbstractTableSelectionModel::SINGLE) {
      // shift was hold, start selecting from the previous cursor position
      sx = cx; sy = cy;
      invalidateWindow();
    } else {
      // start selecting at the cursor position
      sx = x; sy = y;
    }
  }

  invalidateCursor();
  cx = x; cy = y;
  invalidateCursor();
  sigCursor();
    
  DBM2(cerr << "leave mouseLDown" << endl << endl;)
}

void
TTable::mouseMove(int mx, int my, unsigned)
{
DBM2(cerr << "enter mouseMove" << endl;)
  if (!selecting) {
    DBM2(cerr << "  not selecting" << endl;
    cerr << "leave mouseMove" << endl << endl;)
  }

//cerr << __PRETTY_FUNCTION__ << endl;
  int x, y;
  if (!mouse2field(mx, my, &x, &y)) {
    DBM2(cerr << "  mouse2field failed" << endl;
    cerr << "leave mouseMove" << endl << endl;)
    return;
  }
//  cout << "mouse move on field " << x << ", " << y << endl;

  if (cx==x && cy==y) {
    DBM2(cerr << "  cursor position wasn't modified" << endl;
    cerr << "leave mouseMove" << endl << endl;)
    return;
  }

  invalidateChangedArea(sx,sy,cx,cy,cx,cy);
  cx = x; cy = y;
  if (selection && selection->getSelectionMode() == TAbstractTableSelectionModel::SINGLE) {
    sx = cx; sy = cy;
    invalidateCursor();
  } else {
    invalidateChangedArea(sx,sy,cx,cy,cx,cy);
  }
//  selectAtCursor();
  sigCursor();
  DBM2(cerr << "leave mouseMove" << endl << endl;)
}

void
TTable::mouseLUp(int mx, int my, unsigned modifier)
{
DBM2(cerr << "enter mouseLUp" << endl;)
//cerr << __PRETTY_FUNCTION__ << endl;
  int x, y;
  if (!mouse2field(mx, my, &x, &y)) {
    DBM2(cerr << "  mouse2field failed" << endl;
    cerr << "leave mouseLUp" << endl << endl;)
    selecting = false;
    return;
  }

  if (selection && selecting) {
    if (cx==sx && cy==sy) {
      DBM2(cerr << "  toggle current field" << endl;)
      selection->toggleSelection(cx, cy);
    } else {
      DBM2(cerr << "  select range" << endl;)
      int x1, y1, x2, y2;    
      x1 = min(sx, cx);
      x2 = max(sx, cx);
      y1 = min(sy, cy);
      y2 = max(sy, cy);
      selection->setSelection(x1, y1, x2-x1+1, y2-y1+1);
    }
  } else {
    DBM2(
      cerr << "  not selecting or no selection" << endl;
    )
  }
  selecting = false;
  if (modifier & MK_DOUBLE)
    sigDoubleClicked();
  else  
    sigClicked();
/*
  if (cx!=x && cy!=y)
    return;
  cx = x; cy = y;
  selectAtCursor();
  sigCursor();
*/
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

  int panex, paney;
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
      
    if (yp<y1) {
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
      
    if (xp<x1) {
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
TTable::setCursor(int col, int row)
{
  if (cx == col && cy == row)
    return;
  invalidateCursor();
  cx = col; cy = row;
  if (cx>=cols)
    cx = cols-1;
  if (cy>=rows)
    cy = rows-1;
  invalidateCursor();
}

void
TTable::keyDown(TKey key, char *string, unsigned modifier)
{
  switch(key) {
    case TK_DOWN:
      if (!per_col && cy<rows-1) {
        if (!selecting) {
          invalidateCursor();
          // in case nothing indicates the cursor position, don't move
          // the cursor now:
          if (! (noCursor && selectionFollowsMouse &&
                (!getSelectionModel() || getSelectionModel()->isEmpty())))
          {
            cy++;
            invalidateCursor();
          }
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,cx,++cy);
        }
        if (selectionFollowsMouse) {
          selectAtCursor();
        }
        center(CENTER_VERT);
      }
      break;
    case TK_UP:
      if (!per_col && cy>0) {
        if (!selecting) {
          invalidateCursor();
          cy--;
          invalidateCursor();
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,cx,--cy);
        }
        if (selectionFollowsMouse) {
          selectAtCursor();
        }
        center(CENTER_VERT);
      } else {
        // why did we include 'noCursor' in the following statement?
        if (noCursor && selectionFollowsMouse) {
          selectAtCursor();
        }
      }
      break;
    case TK_PAGEUP:
      pageUp();
      break;
    case TK_PAGEDOWN:
      pageDown();
      break;
    case TK_RIGHT:
      if (!per_row && cx<cols-1) {
        if (!selecting) {
          invalidateCursor();
          // in case nothing indicates the cursor position, don't move
          // the cursor now:
          if (!(noCursor && selectionFollowsMouse &&
               (!getSelectionModel() || getSelectionModel()->isEmpty())))
          {
            cx++;
            invalidateCursor();
          }
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,++cx,cy);
        }
        if (selectionFollowsMouse) {
          selectAtCursor();
        }
        center(CENTER_HORZ);
      }
      break;
    case TK_LEFT:
      if (!per_row && cx>0) {
        if (!selecting) {
          invalidateCursor();
          cx--;
          invalidateCursor();
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,--cx,cy);
        }
        if (selectionFollowsMouse) {
          selectAtCursor();
        }
        center(CENTER_HORZ);
      }
      break;
    case TK_RETURN:
      selectAtCursor();
      sigDoubleClicked();
      break;
    case ' ': {
      if (selection) {
        if (selectionFollowsMouse)
          selection->setSelection(per_row?0:cx, per_col?0:cy);
        else
          selection->toggleSelection(per_row?0:cx, per_col?0:cy);
      }
#if 0
      else {
        // sx=cx; sy=cy;
        // selectionChanged();
        selectAtCursor();
        sigDoubleClicked();
      }
#endif
      } break;
    case TK_SHIFT_L:
    case TK_SHIFT_R:
      sx=cx; sy=cy;
      if (selection && selection->getSelectionMode() != TAbstractTableSelectionModel::MULTIPLE_INTERVAL)
        selection->clearSelection();
      else
        selectionChanged();
      selecting=true;
      invalidateCursor();
      break;
  }
}

void
TTable::keyUp(TKey key, char *string, unsigned modifier)
{
  switch(key) {
    case TK_SHIFT_L:
    case TK_SHIFT_R: 
      if (selection) {
        selecting=false;
        int x1, y1, x2, y2;    
        x1 = min(sx, cx);
        x2 = max(sx, cx);
        y1 = min(sy, cy);
        y2 = max(sy, cy);
        selection->setSelection(x1, y1, x2-x1+1, y2-y1+1);
      }
      break;
  }
}

void
TTable::handleNewModel()
{
  invalidateWindow();
  if (selection)
    selection->clearSelection();
  cx = cy = 0;
  sy = sy = 0;
  resetScrollPane();
  ffx = ffy = 0;
  fpx = fpy = 0;
  selecting = false;

  assert(renderer!=NULL);

  rows = renderer->getRows();
  cols = renderer->getCols();
  
  per_row = renderer->per_row;
  per_col = renderer->per_col;

  row_info = rows ? static_cast<TRCInfo*>(realloc(row_info, sizeof(TRCInfo)*rows)) : 0;
  col_info = cols ? static_cast<TRCInfo*>(realloc(col_info, sizeof(TRCInfo)*cols)) : 0;
  
  TRCInfo *info;

  // calculate pane.w
  pane.w=0;
  info = col_info;
  for(int i=0; i<cols; ++i) {
    DBM(cout << "pane.w: " << pane.w << endl;)
    int n = renderer->getColWidth(i);
    info->size = n;
    pane.w += n + border;
    ++info;
  }
  DBM(cout << "pane.w: " << pane.w << endl;)

  // calculate pane.h
  pane.h=0;
  info = row_info;
  for(int i=0; i<rows; ++i) {
    DBM(cout << "pane.h: " << pane.h << endl;)
    int n = renderer->getRowHeight(i);
    info->size = n;
    pane.h += n + border;
    ++info;
  }
  DBM(cout << "pane.h: " << pane.h << endl;)

  doLayout();
}

void
TTable::adjustPane()
{
  DBM(cout << "pane.h: " << pane.h << endl;)

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

//---------------------------------------------------------------------------

#ifdef TEST1
class TTableModel_CString:
  public GAbstractTableModel<const char *>
{
  private:
    const char **list;
    int size;
  public:
    TTableModel_CString(const char **l, int s): list(l), size(s) {}
    TTableModel_CString(const char **l): list(l) {
      const char **p = list;
      size = 0;
      while(*p) {
        size++;
        p++;
      }
    }
    int getRows() {
      return size;
    }
    TElement& getElementAt(int, int index) const {
      return list[index];
    }
};

typedef GSmartPointer<TTableModel_CString> PTableModel_CString;

/**
 * @ingroup table
 * @class toad::TAbstractTableCellRenderer
 *
 * This class renders a single cell in a table.
 */
class TTableCellRenderer_CString:
  public TAbstractTableCellRenderer
{
  private:
    PTableModel_CString model;
  public:
    TTableCellRenderer_CString(TTableModel_CString *m):model(m) {}
    TTableCellRenderer_CString(const char **l) {
      model = new TTableModel_CString(l);
    }
    TTableCellRenderer_CString(const char **l, int s) {
      model = new TTableModel_CString(l, s);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    TTableModel_CString * getModel() {
      return model;
    }
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return model->getCols();
    }
    int getRowHeight(int) {
      DBM(TOADBase::bell();)
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int) {
      DBM(TOADBase::bell());
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(model->getElementAt(0, i));
        if (w>max)
          max = w;
      }
      return max+2;
    }
    virtual void renderItem(TPen &pen, int, int index, int w, int h, bool cursor, bool selected, bool focus) {
      if (selected) {
        if (focus) {
          pen.setColor(TColor::SELECTED);
        } else {
          pen.setColor(TColor::SELECTED_GRAY);
        }
        pen.fillRectanglePC(0,0,w, h);
        pen.setColor(TColor::SELECTED_TEXT);
      }
      pen.drawString(
        1, 1, model->getElementAt(0, index)
      );
      if (selected) {
        pen.setColor(TColor::BLACK);
      }
      if (cursor) {
        pen.drawRectanglePC(0,0,w, h);
      }
    }
};

int
main(int argc, char **argv, char **envv)
{
  static const char* name[] = {
    " 1 blueberry",
    " 2 strawberry",
    " 3 grape",
    " 4 lime",
    " 5 tangerine",
    " 6 bondi-blue",
    " 7 ibm-blue",
    " 8 cray-red",
    " 9 sgi-brown",
    "10 black-sabbath",
    "11 deep-purple",
    NULL
  };

  toad::initialize(argc, argv, envv);
  {
    TTableModel_CString *model = new TTableModel_CString(name);
    TTableCellRenderer_CString *renderer = 
      new TTableCellRenderer_CString(model);
    TTable table(NULL, "table");
  
    table.setRenderer(renderer);
    table.setRowHeaderRenderer(new TDefaultTableHeaderRenderer());
    table.setColHeaderRenderer(new TDefaultTableHeaderRenderer());
    
    toad::mainLoop();
    
    cout << "--------------------------" << endl;
    TTableModel_CString::iterator 
      p(model, table.getSelectionModel()->begin()), 
      e(model, table.getSelectionModel()->end());
    while(p!=e) {
      cout << *p << endl;
      ++p;
    }
    cout << "--------------------------" << endl;
  }
  toad::terminate();
}
#endif

//---------------------------------------------------------------------------

#ifdef TEST2
struct TNetObject {
  unsigned id;
  const char *name;
  const char *location;
  const char *comment;
  const char *type;
};

class TTableModel_TNetObject:
  public GAbstractTableModel<const TNetObject*>
{
  private:
    const TNetObject *list;
    int size;
  public:
    TTableModel_TNetObject(const TNetObject* l, int s) {
      list = l;
      size = s;
    }
    int getRows() {
      return size;
    }
    TElement& getElementAt(int, int index) const {
      return &list[index];
    }
};

typedef GSmartPointer<TTableModel_TNetObject> PTableModel_TNetObject;

class TTableCellRenderer_TNetObject:
  public TAbstractTableCellRenderer
{
  private:
    PTableModel_TNetObject model;
  public:
    TTableCellRenderer_TNetObject(TTableModel_TNetObject *m):model(m) {}
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return 5;
    }
    
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    TTableModel_TNetObject * getModel() {
      return model;
    }
    
    int getRowHeight(int) {
      DBM(TOADBase::bell();)
      return TOADBase::getDefaultFont().getHeight()+2;
    }

    string toText(int x, const TNetObject *obj) const {
      string text;
      switch(x) {
        case 0:
          text = "000";
          break;
        case 1:
          text = obj->name;
          break;
        case 2:
          text = obj->location;
          break;
        case 3:
          text = obj->comment;
          break;
        case 4:
          text = obj->type;
          break;
      }
      return text;
    }

    int getColWidth(int x) {
      DBM(TOADBase::bell());
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(
          toText(x, model->getElementAt(0, i))
        );
        if (w>max)
          max = w;
      }
      return max+2;
    }
    
    virtual void renderItem(TPen &pen, int x, int y, int w, int h, bool selected, bool focus) {
      if (selected) {
        if (focus) {
          pen.setColor(TColor::SELECTED);
        } else {
          pen.setColor(TColor::SELECTED_GRAY);
        }
        pen.fillRectanglePC(0,0,w, h);
        pen.setColor(TColor::SELECTED_TEXT);
      }
      pen.drawString(
        1, 1, toText(x, model->getElementAt(0, y))
      );
      if (selected) {
        pen.setColor(TColor::BLACK);
      }
      if (focus) {
        pen.drawRectanglePC(0,0,w, h);
      }
    }
};

int
main(int argc, char **argv, char **envv)
{
  static const TNetObject map[] = {
    { 1, "HUB2-2L", "Hall 2 Gate 2L", "not in rack", "SNPX HUB 5000 ENET" },
    { 2, "SWI2-3R", "Hall 2 Gate 3R", "ask Frank", "28XXX Switch" },
    { 3, "SWI1-1", "Hall 1 Gate 2", "ask Frank", "28XXX Switch" },
  };
  toad::initialize(argc, argv, envv);
  {
    TTableModel_TNetObject *model = new TTableModel_TNetObject(map, 3);
    TTableCellRenderer_TNetObject *renderer = 
      new TTableCellRenderer_TNetObject(model);
    TTable table(NULL, "table");
  
    table.setRenderer(renderer);
    table.setRowHeaderRenderer(new TDefaultTableHeaderRenderer());
    table.setColHeaderRenderer(new TDefaultTableHeaderRenderer());
    
    toad::mainLoop();

    cout << "--------------------------" << endl;
    TTableModel_TNetObject::iterator 
      p(model, table.getSelectionModel()->begin()), 
      e(model, table.getSelectionModel()->end());
    while(p!=e) {
      cout << (*p)->name << endl;
      ++p;
    }
    cout << "--------------------------" << endl;
  }
  toad::terminate();
}
#endif
