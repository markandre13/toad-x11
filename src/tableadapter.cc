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

#include <toad/table.hh>
#include <toad/utf8.hh>

using namespace toad;

TTableModel*
TSimpleTableAdapter::getModel() const
{
  return 0;
}

TTableAdapter* TTableAdapter::edit = 0;
size_t TTableAdapter::cx;
size_t TTableAdapter::col;
size_t TTableAdapter::row;

TTableAdapter::TTableAdapter()
{
  table = 0;
  reason = TTableModel::CHANGED;
}

void
TTableAdapter::modelChanged()
{
  TTableModel *model = getModel();
  if (!model) {
    cerr << "TTableAdapter::modelChanged: received modelChanged but no model is set" << endl;
    return;
  }
  
  reason = model->reason;
  where  = model->where;
  size   = model->size;
/*
cout << "TTableAdapter::modelChanged propagates ";
switch(reason) { 
  case TTableModel::CHANGED: cout << "CHANGED" << endl; break;
  case TTableModel::INSERT_ROW: cout << "INSERT_ROW" << endl; break;
  case TTableModel::RESIZED_ROW: cout << "RESIZED_ROW" << endl; break;
  case TTableModel::REMOVED_ROW: cout << "REMOVED_ROW" << endl; break;
  default: cout << "DEFAULT..." << endl;
}
*/
  sigChanged();
};


void
TTableAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      te.w = getColWidth(te.col);
      break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = getRowHeight(te.row);
      break;
    case TTableEvent::PAINT:
      renderCell(*te.pen, te);
      break;
    case TTableEvent::MOUSE:
      mouseEvent(te.mouse, te.col, te.row, te.w, te.h);
      break;
  }
}

/**
 * Draws selection indicators, calls renderItem and draws the cursor.
 */
void 
TTableAdapter::renderCell(TPen &pen, TTableEvent &te)
{
  renderBackground(te);
  renderItem(pen, te);
  renderCursor(te);
}
  
void
TTableAdapter::renderBackground(TTableEvent &te)  
{
  if (te.type != TTableEvent::PAINT)
    return;
  te.pen->setLineStyle(TPen::SOLID);
  te.pen->setLineWidth(1);
  if (te.selected) {
    if (te.focus) {
      te.pen->setColor((te.row&1) ? TColor::SELECTED_2 : TColor::SELECTED);
    } else {
      te.pen->setColor((te.row&1) ? TColor::SELECTED_GRAY_2 : TColor::SELECTED_GRAY);
    }
  } else {
    te.pen->setColor((te.row&1) ? TColor::TABLE_CELL_2 : TColor::TABLE_CELL);
  }
  te.pen->fillRectanglePC(0,0,te.w,te.h);
  if (te.selected)
    te.pen->setColor(TColor::SELECTED_TEXT);
  else 
    te.pen->setColor(TColor::BLACK);
}

void
TTableAdapter::renderCursor(TTableEvent &te)
{
  if (te.type != TTableEvent::PAINT)
    return;
  if (te.cursor) {
    te.pen->setColor(TColor::BLACK);
    te.pen->setLineStyle(TPen::SOLID);
    te.pen->setLineWidth(1);
    if (te.per_row) {
      te.pen->drawLine(0,0,te.w-1,0);
      te.pen->drawLine(0,te.h-1,te.w,te.h-1);
      if (te.col==0) {
        te.pen->drawLine(0,0,0,te.h-1);
      }
      if (te.col==te.cols) {
        te.pen->drawLine(0,te.w-1,te.w-1,te.h-1);
      }
    } else
    if (te.per_col) {
      te.pen->drawLine(0,0,0,te.h-1);
      te.pen->drawLine(0,te.w-1,te.w-1,te.h-1);
      if (te.row==0) {
        te.pen->drawLine(0,0,te.w-1,0);
      }
      if (te.row==te.rows) {
        te.pen->drawLine(0,te.h-1,te.w-1,te.h-1);
      }
    } else {
      te.pen->drawRectanglePC(0,0,te.w, te.h);
    }
  }
}

void 
TTableAdapter::mouseEvent(TMouseEvent&, int col, int row, int w, int h)
{
}

void
TTableAdapter::handleCheckBox(TTableEvent &te, bool *value)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      te.w = 17;
      break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = 17;
      break;
    case TTableEvent::PAINT: {
      int x = (te.w-13) / 2;
      int y = (te.h-13) / 2;

      te.pen->drawRectanglePC(x,y,13,13);
      te.pen->drawRectanglePC(x+1,y+1,11,11);
      if (*value) {
        te.pen->drawLine(x+3,y+3,x+9,y+9);
        te.pen->drawLine(x+3,y+4,x+8,y+9);
        te.pen->drawLine(x+4,y+3,x+9,y+8);
        te.pen->drawLine(x+9,y+3,x+3,y+9);
        te.pen->drawLine(x+8,y+3,x+3,y+8);
        te.pen->drawLine(x+9,y+4,x+4,y+9);
      }
    } break;
    case TTableEvent::MOUSE: {
      TRectangle r((te.w-13) / 2, (te.h-13) / 2, 13, 13);
      if (!r.isInside(te.mouse.x, te.mouse.y)) {
        return;
      }
      *value = !*value;
      reason = TTableModel::CONTENT;
      sigChanged();
    } break;
  }
}

void
TTableAdapter::handleString(TTableEvent &te, string *s, int offx)
{
  int x0;
  const char *str;

  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      te.w = 150;
      break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = 18;
      break;
    case TTableEvent::PAINT:
// cout << "paint " << te.col << ", " << te.row << endl;
      if (edit==this && te.col == col && te.row == row) {
        te.pen->setColor(255,255,192);
        te.pen->fillRectanglePC(0,0,te.w,te.h);
        te.pen->setColor(0,0,0);
        te.pen->drawString(offx+2, 2, *s);
        x0 = te.pen->getTextWidth(s->substr(0,cx));
        te.pen->drawLine(2+offx+x0, 2-1,
                         2+offx+x0, 2+te.pen->getHeight()+1);
      } else {
        renderBackground(te);
        te.pen->drawString(2+offx, 2, *s);
      }
      renderCursor(te);
      break;
    case TTableEvent::KEY:
//cout << "keyEvent" << endl;
      if (te.key->type == TKeyEvent::DOWN) {
//cout << "keyDown" << endl;
        if (edit!=this) {
//cout << "not edit" << endl;
          if (te.key->getKey()==TK_RETURN ||
              te.key->getKey()==TK_KP_RETURN) 
          {
//            cout << "begin to edit string" << endl;
            if (edit!=0) {
              edit->reason = TTableModel::CONTENT;
              edit->sigChanged();
            }
            edit = this;
            cx = 0;
            row = te.row;
            col = te.col;
            te.flag = true;
          }
        } else {
          TKey key = te.key->getKey();
          switch(key) {
            case TK_KP_RETURN:
            case TK_RETURN:
//              cout << "end to edit string" << endl;
              edit = 0;
              reason = TTableModel::CONTENT;
              sigChanged();
              break;
            case TK_DOWN:
            case TK_UP:
//              cout << "end to edit string" << endl;
              edit = 0;
              reason = TTableModel::CONTENT;
              sigChanged();
              return;
            case TK_LEFT:
              if (cx>0)
                utf8dec(*s, &cx);
              break;
            case TK_RIGHT:
              if (cx<s->size())
                utf8inc(*s, &cx);
              break;
            case TK_HOME:
              cx=0;
              break;
            case TK_END:
              cx = s->size();
              break;
            case TK_DELETE:
              if (cx<s->size()) {
                s->erase(cx, utf8charsize(*s, cx));
              }
              break;
            case TK_BACKSPACE:
              if (cx>0) {
                utf8dec(*s, &cx);
                s->erase(cx, utf8charsize(*s, cx));
              }
              break;
            default:
              str = te.key->getString();
              if ( (unsigned char)str[0]>=32 ||
                   (str[0]!=0 && str[1]!=0) )
              {
                s->insert(cx, str);
                utf8inc(*s, &cx);
              }
              break;
          }
          te.flag = true;
        }
      }
      break;
  }
}
