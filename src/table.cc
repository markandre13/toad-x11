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

#include <toad/toad.hh>
#include <toad/table.hh>

#include <stdio.h>
#include <unistd.h>

using namespace toad;

#define DBM(M)
#define DBSCROLL(M)

/**
 * @defgroup table Table
 *
 * This group contains a set of classes to display data in a table.
 *
 * @verbatim
 * TAbstractTableModel
 *   GAbstractTableModel<T>
 *    TTableModel_CString
 * TAbstractTableCellRenderer
 *   TTableCellRenderer_CString
 * TTableSelectionModel
 * TTable
 *
 *  mi(si)
 *       1 n             1 1
 *  model---cell_renderer---+
 *                          +--table
 *         selectionmodel---+
 *               si      1 1
 *
 * mi : model iterator
 * si : selection iterator
 * @endverbatim
 *
 * \code
 * class TMyTable:
 *   public TTable
 * {
 * }
 * \endcode
 *
 * \todo
 *   \li TTableModel_CString and TStringVector should be subclassed
 *       with an extra step, which provides an abstraction to C arrays
 *       and STL vectors
 *   \li 'setModel' seems to be the same code in most situations: find
 *       a way to put it into the template
 *   \li show focus mark
 *   \li model & render can have different size, use this to set
 *       SELECT_PER_ROW & SELECT_PER_COL automaticly
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

#if 0

TSmartObject
  TTableSelectionModel

TAbstractTableModel
  GAbstractTableModel
    TTableModel_CString

TAbstractTableCellRenderer
  TTableCellRenderer_CString

TTable
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
  if (selection_mode!=MULTIPLE_INTERVAL)
    region.clear();
  TRectangle r(x,y,1,1);
  region ^= r;
  sigChanged();
}

bool
TTableSelectionModel::isSelected(int x, int y) const
{
  return region.isInside(x, y);
}

/**
 * @ingroup table
 * @class toad::TAbstractTableModel
 *
 * This class provides the basic interface between the applications data
 * and the TOAD toolkit. It should be subclassed via the generic class
 * GAbstractTableModel.
 *
 * @sa GAbstractTableModel
 */

int
TAbstractTableModel::getCols()
{
  return 1;
}

/**
 * @ingroup table
 * @class toad::GAbstractTableModel
 *
 * This interface can be subclassed to provide an interface between
 * you applications data and the TOAD toolkit.
 *
 * This generic class uses one type only for all cells in the table. To
 * handle tables with different types per column, per row or even a different
 * type in every field, one has to write an adapter class which might
 * look like this:
 *
 * @code
 * class TMyData
 * {
 *   enum TType { 
 *     STRING, NUMBER, BITMAP, DATE, CURRENCY, EXPRESSION 
 *   } type;
 *   union {
 *     string string;
 *     double number;
 *     TBitmap bitmap;
 *     time_t date;
 *     long currency;
 *     string expression;
 *   } data;
 * }
 * @endcode
 */

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
 * @class toad:TTable
 */

TTable::TTable(TWindow *p, const string &t): 
  super(p, t) 
{
  renderer = NULL;
  selection = new TTableSelectionModel();
  border = 0;
  cx = cy = 0;
  ffx = ffy = 0;
  fpx = fpy = 0;
  row_info = col_info = NULL;
  rows = cols = 0;
  row_header_renderer = col_header_renderer = NULL;
  selecting = false;
  stretchLastColumn = true;
  connect(selection->sigChanged, this, &TTable::selectionChanged);
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
TTable::setSelectionModel(TTableSelectionModel *m)
{
  if (selection==m)
    return;
  if (selection)
    disconnect(selection->sigChanged, this);
  selection = m;
  if (selection)
    connect(selection->sigChanged, this, &TTable::selectionChanged);
}

void
TTable::selectionChanged()
{
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
  
  invalidateWindow(xp, yp, col_info[cx].size, row_info[cy].size);
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
        size = visible.x+visible.w-xp;
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
  bool perRow = renderer->getCols()!=renderer->getModel()->getCols();
  bool perCol = renderer->getRows()!=renderer->getModel()->getRows();
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
        bool selected = selection->isSelected(perRow?0:x,perCol?0:y);
        if (selecting) {
          if (x>=x1 && x<=x2 && y>=y1 && y<=y2)
            selected = true;
        }
        int size = col_info[x].size;
        if (stretchLastColumn && x==cols-1 && xp+size<visible.x+visible.w)
          size = visible.x+visible.w-xp-1;

DBSCROLL(
  pen.setColor(255,255,255);
  pen.fillRectanglePC(0,0,col_info[x].size, row_info[y].size);
  pen.setColor(0,0,0);
)
        renderer->renderItem(
            pen,
            x, y,
            size, row_info[y].size,
            selected,
            cx == x && cy == y && isFocus()
        );
      }
      yp += row_info[y].size + border;
    }
    xp += col_info[x].size + border;
  }
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
  invalidateCursor();
}

void
TTable::mouseLDown(int mx, int my, unsigned)
{
  setFocus();

  int pos1, pos2;
  int x, y;
  
  if (!visible.isInside(mx, my))
    return;
  
  mx -= visible.x + fpx;
  my -= visible.y + fpy;

  pos1 = 0;
  for(x=ffx; ; x++) {
    if (x>=cols || pos1>visible.x+visible.w)
      return;
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
  for(y=ffy; ; y++) {
    if (y>=rows || pos1>visible.y+visible.h)
      return;
    pos2 = pos1 + row_info[y].size;
    if (pos1 <= my && my < pos2) {
      break;
    }
    pos1 = pos2;
  }

  cx = x; cy = y;  
  bool perRow = renderer->getCols()!=renderer->getModel()->getCols();
  bool perCol = renderer->getRows()!=renderer->getModel()->getRows();
  selection->toggleSelection(perRow?0:x, perCol?0:y);
  DBM(cout << "click on item " << x << ", " << y << endl;)
  sigCursor();
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
TTable::keyDown(TKey key, char *string, unsigned modifier)
{
  switch(key) {
    case TK_DOWN:
      if (cy<rows-1) {
        if (!selecting) {
          invalidateCursor();
          cy++;
          invalidateCursor();
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,cx,++cy);
        }
        center(CENTER_VERT);
      }
      break;
    case TK_UP:
      if (cy>0) {
        if (!selecting) {
          invalidateCursor();
          cy--;
          invalidateCursor();
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,cx,--cy);
        }
        center(CENTER_VERT);
      }
      break;
    case TK_RIGHT:
      if (cx<cols-1) {
        if (!selecting) {
          invalidateCursor();
          cx++;
          invalidateCursor();
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,++cx,cy);
        }
        center(CENTER_HORZ);
      }
      break;
    case TK_LEFT:
      if (cx>0) {
        if (!selecting) {
          invalidateCursor();
          cx--;
          invalidateCursor();
          sigCursor();
        } else {
          invalidateChangedArea(sx,sy,cx,cy,--cx,cy);
        }
        center(CENTER_HORZ);
      }
      break;
    case ' ': {
      bool perRow = renderer->getCols()!=renderer->getModel()->getCols();
      bool perCol = renderer->getRows()!=renderer->getModel()->getRows();
      selection->toggleSelection(perRow?0:cx, perCol?0:cy);
      } break;
    case TK_SHIFT_L:
    case TK_SHIFT_R:
      if (selection->getSelectionMode() != TTableSelectionModel::MULTIPLE_INTERVAL)
        selection->clearSelection();
      sx=cx; sy=cy;
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
    case TK_SHIFT_R: {
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
  resetScrollPane();
  ffx = ffy = 0;
  fpx = fpy = 0;
  selecting = false;

  rows = renderer->getRows();
  cols = renderer->getCols();
  
  row_info = static_cast<TRCInfo*>(realloc(row_info, sizeof(TRCInfo)*rows));
  col_info = static_cast<TRCInfo*>(realloc(col_info, sizeof(TRCInfo)*cols));
  
  TRCInfo *info;

  // calculate tab_w
  tab_w=0;
  info = col_info;
  for(int i=0; i<cols; ++i) {
    DBM(cout << "tab_w: " << tab_w << endl;)
    int n = renderer->getColWidth(i);
    info->size = n;
    tab_w += n + border;
    ++info;
  }
  DBM(cout << "tab_w: " << tab_w << endl;)

  // calculate tab_h
  tab_h=0;
  info = row_info;
  for(int i=0; i<rows; ++i) {
    DBM(cout << "tab_h: " << tab_h << endl;)
    int n = renderer->getRowHeight(i);
    info->size = n;
    tab_h += n + border;
    ++info;
  }
  DBM(cout << "tab_h: " << tab_h << endl;)

  doLayout();
}

void
TTable::adjustPane()
{
  visible.set(0,0,getWidth(),getHeight());

  DBM(cout << "tab_h: " << tab_h << endl;)

  if (row_header_renderer) {
    visible.x = row_header_renderer->getWidth();
    visible.w -= visible.x;
  }
  if (col_header_renderer) {
    visible.y = col_header_renderer->getHeight();
    visible.h -= visible.y;
  }
  setUnitIncrement(cols ? tab_w/cols : 1, rows ? tab_h/rows : 1);
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
    const char* getElementAt(int, int index) {
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
    virtual void renderItem(TPen &pen, int, int index, int w, int h, bool selected, bool focus) {
      if (selected) {
        pen.setColor(TColor::SELECTED);
        pen.fillRectanglePC(0,0,w, h);
        pen.setColor(TColor::SELECTED_TEXT);
      }
      pen.drawString(
        1, 1, model->getElementAt(0, index)
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
    const TNetObject* getElementAt(int, int index) {
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
        pen.setColor(TColor::SELECTED);
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
