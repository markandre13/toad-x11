/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/treeadapter.hh>

#define DBM(CMD)

using namespace toad;

int
TTreeAdapter::getRowHeight(size_t row)
{
//cout << "TTreeAdapter::getRowHeight(" << row << ")"<<endl;
  TTreeModel *model = getModel();
DBM(cout << "TTreeAdapter::getRowHeight for row " << row << endl;)
  if (!model) {
    cout << "TTreeAdapter::getRowHeight: no model" << endl;
    return 0;
  }

  int y = 17;
  
  unsigned d = model->getRowDepth(row);
  if(d==0) {
DBM(
    cout << "  depth = 0, row is visible => done" << endl;
    cout << "  btw, the row is " << (table->isRowOpen(row)?"open":"closed") << endl;
)
    return y;
  }
  while(row>0) {
    row--;
    if (model->getRowDepth(row) < d) {
      if (!table->isRowOpen(row)) {
        DBM(cout << "  parent is closed, row isn't visible => done" << endl;)
        return 0;
      }
      d = model->getRowDepth(row);
      if (d==0)
        break;
    }
  }
DBM(
  if (table->isRowOpen(row)) {
    cout << "  row is open and thus visible => done" << endl;
  } else {
    cout << "  row isn't open and thus not visible => done" << endl;
  }
)
  return !table->isRowOpen(row) ? 0 : y;
}

size_t
TTreeAdapter::findOpenRowAbove(size_t row)
{
#if 0
  if (rows->empty())
    return 0;
  size_t openrow = row;
  unsigned d = (*rows)[row].depth;
  while(row>0) {
    row--;
    if ((*rows)[row].depth<d) { // this is a parent
      if ((*rows)[row].closed)
        openrow = row;
      d = (*rows)[row].depth;
      if (d==0) {
        break;
      }
    }
  }
  return openrow;
#else
  return 0;
#endif
}

int
TTreeAdapter::getColWidth(size_t)
{
  return 100;
}

      
void
TTreeAdapter::handleTree(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_ROW_SIZE:
      te.h = getRowHeight(te.row);
      break;
    case TTableEvent::PAINT:
      drawTree(*te.pen, te.row);
      break;
    case TTableEvent::MOUSE:
//      cout << "TTreeAdapter got mouse" << endl;
      TTreeAdapter::mouseEvent(te.mouse, te.col, te.row, te.w, te.h);
      break;
  }
}


void
TTreeAdapter::mouseEvent(TMouseEvent &me, int col, int row, int w, int h)
{
//cout << "  TTreeAdapter::mouseEvent" << endl;
  TTreeModel *model = getModel();
//cout << "    model="<<model<<endl;
  if (model->getDown(model->at(row))) {
    TRectangle r(model->getRowDepth(row)*12+3-1, 0, 10, h);
    if (!r.isInside(me.x, me.y))
      return;
    DBM(cout << "open/close branch" << endl;)
    table->setRowOpen(row, !table->isRowOpen(row));
    DBM(cout << table->isRowOpen(row) << endl;)

    reason = TTreeModel::RESIZED_ROW;
    where  = row+1;
    size   = 0;

    unsigned d = model->getRowDepth(row); // (*rows)[row].depth;
    for(unsigned i = row + 1;
        i < model->getRows();
        ++i)
    {
      if (model->getRowDepth(i) <= d)
        break;
      ++size;
    }
    sigChanged();
  }
}

void 
TTreeAdapter::renderItem(TPen &pen, const TTableEvent &te)
{
  TTreeModel *model = getModel();
//  cout << "render row " << te.row << endl;
  if (!model)
    return;

  if (te.row>=model->getRows()) {
    cout << "TTreeAdapter:renderItem: row number "<<te.row<<" is above row count "<<model->getRows()<< endl;
    return;
  }

  drawTree(pen, te.row);
#if 0
  pen.setColor(0,0,0);
  const char *str = name((*rows)[te.row].node);
  if (str)
    pen.drawString(getLeafPos(te.row), 2, str);
#endif
}

static const int sx = 12;  // horizontal step width
static const int dx = 3;   // additional step before drawing the rectangle
static const int dy = 4;   // step from top to rectangle
static const int rs = 8;   // rectangle width and height
static const int rx = 3;   // horizontal line from rectangle to data on the left

int
TTreeAdapter::getLeafPos(size_t row) {
  TTreeModel *model = getModel();
  if (!model)
    return 0;
  return model->getRowDepth(row)*sx+dx+rs+dx+1;
}

void
TTreeAdapter::drawTree(TPen &pen, int row)
{
  TTreeModel *model = getModel();
  if (row>=model->getRows()) {
    cout << "TTreeAdapter:renderTree: row number "<<row<<" is above row count "<<model->getRows()<< endl;
    return;
  }

  int item_h = table->getRowHeight(row);
  if (item_h==0)
    return;

  int d = model->getRowDepth(row);
  // draw node symbol
  
  pen.setColor(0,0,0);

  // draw `+' or `-' when the node has children
  if (model->getDown(model->at(row))) {
    pen.drawRectangle(d*sx+dx, dy, rs, rs);
    // minus
    pen.drawLine(d*sx+dx+(rs>>2), dy+(rs>>1), d*sx+dx+rs-(rs>>2), dy+(rs>>1)); 
    if (isClosed(row)) {
      // plus
      pen.drawLine(d*sx+dx+(rs>>1), dy+(rs>>2), d*sx+dx+(rs>>1), dy+rs-(rs>>2));
    }
    // horizontal line to data
    pen.drawLine(d*sx+dx+rs, dy+(rs>>1), d*sx+dx+rs+rx, dy+(rs>>1));
  } else {
    // upper vertical line instead of box
    pen.drawLine(d*sx+dx+(rs>>1), dy, d*sx+dx+(rs>>1), dy+(rs>>1));
    // horizonzal line to data
    pen.drawLine(d*sx+dx+(rs>>1), dy+(rs>>1), d*sx+dx+rs+rx, dy+(rs>>1));
  }

  // draw lines

  // small line above box
  pen.drawLine(d*sx+dx+(rs>>1),0, d*sx+dx+(rs>>1), dy);

  // lines connecting nodes
  for(int i=0; i<=d; i++) {
    for(int j=row+1; j<model->getRows(); j++) {

      if (model->getRowDepth(j)<i) {
        break;
      }

      if (i==model->getRowDepth(j)) {
        if (i!=d) {
          // long line without box
          pen.drawLine(i*sx+dx+(rs>>1),0,i*sx+dx+(rs>>1),item_h);
        } else {
          // small line below box
          if (model->getDown(model->at(row))) {
            // has subtree => start below box
            pen.drawLine(i*sx+dx+(rs>>1),dy+rs,i*sx+dx+(rs>>1),item_h);
          } else {
            // has no subtree => has no box => don't start below box
            pen.drawLine(i*sx+dx+(rs>>1),dy+(rs>>1),i*sx+dx+(rs>>1),item_h);
          }
        }
        break;
      }
    }
  }
}

bool
TTreeAdapter::isClosed(size_t row)
{
  return !table->isRowOpen(row);
}
