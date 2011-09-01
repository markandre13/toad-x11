/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2007 by Mark-André Hopf <mhopf@mark13.org>
 * Visit http://www.mark13.org/fischland/.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "selectiontool.hh"

using namespace fischland;

TSelectionTool*
TSelectionTool::getTool()
{
  static TSelectionTool* tool = 0;
  if (!tool)
    tool = new TSelectionTool();
  return tool;
}

void
TSelectionTool::stop(TFigureEditor *fe)
{
  fe->getWindow()->setAllMouseMoveEvents(false);
  fe->getWindow()->setCursor(0);
  fe->state = TFigureEditor::STATE_NONE;
  fe->quick = false;
}

void
TSelectionTool::keyEvent(TFigureEditor *fe, const TKeyEvent &ke)
{
#if 0
  switch(ke.getKey()) {
    case TK_ESCAPE:
      fe->clearSelection();
      break;
    case TK_DELETE:
    case TK_BACKSPACE:
      fe->deleteSelection();
      break;
    case TK_PAGE_DOWN:
      fe->selectionDown();
      break;
    case TK_PAGE_UP:
      fe->selectionUp();
      break;
  }
#endif
}

void
TSelectionTool::mouseEvent(TFigureEditor *fe, const TMouseEvent &me)
{
  TFigure *figure;
  TCoord x, y;
  TRectangle r;
  
  switch(me.type) {
    case TMouseEvent::RDOWN: {
      TCoord x, y;
      fe->mouse2sheet(me.x, me.y, &x, &y);
      TFigure *f =  fe->findFigureAt(x, y);
      if (f && fe->selection.find(f)!=fe->selection.end())
        f->mouseRDown(fe, TMouseEvent(fe, x, y, me.modifier()));
    } break;
      if (figure)
        figure->mouseRDown(fe, TMouseEvent(fe, x, y, me.modifier()));
      break;
    case TMouseEvent::LDOWN:
      down = true;
//cout << "mouse down" << endl;
      // get all mouse move events
      if (fe->state == TFigureEditor::STATE_NONE) {
        fe->state = TFigureEditor::STATE_CREATE;
        fe->getWindow()->setAllMouseMoveEvents(true);
        action = SELECT;
      }
      
      // check for mouse down on handle
      if (!fe->selection.empty()) {
        // origin is already applied by scroll pane?
        TCoord x = me.x /*+ fe->getWindow()->getOriginX()*/ - fe->getVisible().x;
        TCoord y = me.y /*+ fe->getWindow()->getOriginY()*/ - fe->getVisible().y;
//        cout << "down at " << x << ", " << y << endl;
        for(unsigned i=0; i<8; ++i) {
          getBoundingHandle(i, &r);
//          cout << "  check " << r.x << ", " << r.y << endl;
          if (r.isInside(x, y)) {
            handle = i;
            ox0 = x0;
            oy0 = y0;
            ox1 = x1;
            oy1 = y1;
            skew_x = x-r.x;
            skew_y = y-r.y;
            action = SCALE;
//cout << "  -> scale" << endl;
            fe->quick = true;
            return;
          }
        }
      }
      
      // find figure
      fe->mouse2sheet(me.x, me.y, &x, &y);
      figure = fe->findFigureAt(x, y);
      
      // check for grab on figure for translation
      if (fe->selection.find(figure) != fe->selection.end()) {
        fe->sheet2grid(x, y, &last_x, &last_y);
        last_sx = me.x;
        last_sy = me.y;
        action = TRANSLATE;
        TUndoManager::beginUndoGrouping(fe->getModel());
        break;
      }
      
      if (!me.modifier() & MK_SHIFT) {
        fe->clearSelection();
      }
      if (!selection.empty()) {
        fe->invalidateWindow();
      }
      selection.clear();
      if (figure) {
        selection.insert(figure);
        fe->invalidateWindow();
      }
      down = true;
      rx0 = rx1 = me.x;
      ry0 = ry1 = me.y;
      fe->quick = true;
      break;
      
    case TMouseEvent::MOVE:
//cout << "mouse move " << action << endl;
      switch(action) {
        case SELECT:
          // modify mouse cursor above handles
          if (!down) {
            if (!fe->selection.empty()) {
              // origin is already applied by scroll pane?
              TCoord x = me.x /*+ fe->getWindow()->getOriginX()*/ - fe->getVisible().x;
              TCoord y = me.y /*+ fe->getWindow()->getOriginY()*/ - fe->getVisible().y;
              TCursor::EType cursor = TCursor::DEFAULT;
              for(unsigned i=0; i<8; ++i) {
                getBoundingHandle(i, &r);
                if (r.isInside(x, y)) {
                  static const TCursor::EType wind[8] = {
                   TCursor::NW_RESIZE,
                   TCursor::N_RESIZE,
                   TCursor::NE_RESIZE,
                   TCursor::E_RESIZE,
                   TCursor::SE_RESIZE,
                   TCursor::S_RESIZE,
                   TCursor::SW_RESIZE,
                   TCursor::W_RESIZE
                  };
                  cursor = wind[i];
                  break;
                }
              }
              fe->getWindow()->setCursor(cursor);
            }
          } else {
//cout << "mouse moved and down" << endl;
        if (me.x < rx0-2 || me.x > rx0+2 || me.y < ry0-2 || me.y > ry0+2) {
//cout << "start select rect" << endl;
          action = SELECT_RECT;
//          selection.clear();
          fe->getWindow()->invalidateWindow();
          rx1 = me.x;
          ry1 = me.y;
          }
          }
          break;

        case SELECT_RECT: {
//cout << "select rect" << endl;
          rx1 = me.x;
          ry1 = me.y;
          
          // update selection
          selection.clear();
          TPoint p0, p1;
          fe->mouse2sheet(rx0, ry0, &p0.x, &p0.y);
          fe->mouse2sheet(rx1, ry1, &p1.x, &p1.y);
          TRectangle r0(p1, p0), r1;
          for(TFigureModel::const_iterator p = fe->getModel()->begin();
              p != fe->getModel()->end();
              ++p)
          {
            fe->getFigureShape(*p, &r1, NULL);
            if (r0.isInside( r1.x, r1.y ) &&
                r0.isInside( r1.x+r1.w, r1.y+r1.h ) )
            {
              selection.insert(*p);
            }
          }
          
          fe->getWindow()->invalidateWindow();
        } break;
          
        case SCALE: {
          // move handle, scale the selection
//cout << "  scale" << endl;
          TCoord x, y;
          x = me.x - fe->getVisible().x - skew_x;
          y = me.y - fe->getVisible().y - skew_y;
          switch(handle) {
            case 0:
              x0 = x;
              y0 = y;
              break;
            case 1:
              y0 = y;
              break;
            case 2:
              x1 = x;
              y0 = y;
              break;
            case 3:
              x1 = x;
              break;
            case 4:
              x1 = x;
              y1 = y;
              break;
            case 5:
              y1 = y;
              break;
            case 6:
              x0 = x;
              y1 = y;
              break;
            case 7:
              x0 = x;
              break;
          }
          // can't use x0, y1 as we already removed getVisible().x|.y
          // fe->mouse2sheet(x0, y0, &bx, &by);
          if (fe->getMatrix()) {
            TMatrix2D m(*fe->getMatrix());
            m.invert();
            m.map(x0, y0, &bx, &by);
          }
          
          fe->getWindow()->invalidateWindow();
        } break;
          
        case TRANSLATE: {
          // move the selection
          TCoord x, y;
          fe->mouse2sheet(me.x, me.y, &x, &y);
          fe->sheet2grid(x, y, &x, &y);
          TCoord dx = x-last_x;
          TCoord dy = y-last_y;
//          invalidateBounding(fe);
          fe->getModel()->translate(fe->selection, dx, dy);
//          calcSelectionsBoundingRectangle(fe);
//          invalidateBounding(fe);
          last_x=x;
          last_y=y;
        } break;
      }
      break;
      
    case TMouseEvent::LUP:
//cout << "mouse up" << endl;
      down = false;
      switch(action) {
        case SCALE: {
          TCoord sx = (double)(x1-x0)/(ox1 - ox0);
          TCoord sy = (double)(y1-y0)/(oy1 - oy0);
          TMatrix2D m;
          m.translate(bx,by);
          m.scale(sx, sy);
          m.translate(-obx, -oby);
          fe->getModel()->transform(fe->selection, m);
//          calcSelectionsBoundingRectangle(fe);
          action = SELECT;
//          fe->quickready = false;
//          fe->getWindow()->invalidateWindow();
        } break;
        case TRANSLATE:
          action = SELECT;
          TUndoManager::endUndoGrouping(fe->getModel());
          fe->quickready = false;
          fe->getWindow()->invalidateWindow();
          break;
        case SELECT_RECT:
          action = SELECT;
        case SELECT:
          // add new selection to figure editors selection
          fe->selection.insert(selection.begin(), selection.end());
          selection.clear();
          // calculate the selections bounding rectangle
          calcSelectionsBoundingRectangle(fe);
          fe->invalidateWindow();
          break;
      }
      break;
    default:;
  }
}

void
TSelectionTool::invalidateBounding(TFigureEditor *fe)
{
  fe->getWindow()->invalidateWindow(
    x0-4 + fe->getWindow()->getOriginX() + fe->getVisible().x,
    y0-4 + fe->getWindow()->getOriginY() + fe->getVisible().y,
    x1-x0+10,y1-y0+10);
}

void
TSelectionTool::invalidateOldBounding(TFigureEditor *fe)
{
  fe->getWindow()->invalidateWindow(
    ox0-4 + fe->getWindow()->getOriginX() + fe->getVisible().x,
    oy0-4 + fe->getWindow()->getOriginY() + fe->getVisible().y,
    ox1-ox0+10,oy1-oy0+10);
}

void
TSelectionTool::paintSelection(TFigureEditor *fe, TPenBase &pen)
{
  // draw selection rectangle
  if (action==SELECT_RECT) {
//cout << "draw selection rectangle" << endl;
    double tx = 0.0, ty = 0.0;
    if (pen.getMatrix()) {
      tx = pen.getMatrix()->tx - fe->getVisible().x;
      ty = pen.getMatrix()->ty - fe->getVisible().y;
    }
#if 1
    pen.push();
    pen.identity();
    pen.translate(tx, ty);
    pen.setColor(TColor::FIGURE_SELECTION);
    pen.setLineWidth(1);
    pen.drawRectanglePC(rx0, ry0, rx1-rx0, ry1-ry0);
    pen.pop();
#else
    TCairo pen2(fe->getWindow());
    pen2.identity();
    pen2.translate(tx, ty);
    pen2.setColor(TColor::FIGURE_SELECTION);
    pen2.setLineWidth(1);
    pen2.setAlpha(32);
    pen2.fillRectangle(rx0, ry0, rx1-rx0+1, ry1-ry0);
    pen2.setAlpha(255);
    pen2.drawRectangle(rx0, ry0, rx1-rx0+1, ry1-ry0);
#endif
  }

  // draw selection outline
//cout << "draw outline" << endl;
  pen.outline = true;
  pen.setColor(TColor::FIGURE_SELECTION);
  pen.keepcolor = true;
    
  TFigureSet::const_iterator sp, se;
  for(unsigned i=0; i<2; ++i) {
    if (i==0) {
      sp = fe->selection.begin();
      se = fe->selection.end();
    } else {
      sp = selection.begin();
      se = selection.end();
    }
    
    for(; sp != se; ++sp) {
      pen.push();
      if ((*sp)->mat)
        pen.multiply( (*sp)->mat );
      if (action==SCALE) {
        double sx = (double)(x1-x0)/(ox1 - ox0);
        double sy = (double)(y1-y0)/(oy1 - oy0);
        pen.translate(bx,by);
        pen.scale(sx, sy);
        pen.translate(-obx, -oby);
      }
      (*sp)->paint(pen);
      pen.pop();
    }
  }
  pen.outline = false;
  pen.keepcolor = false;

  // draw bounding rectangle
  if (action!=SELECT_RECT && !fe->selection.empty()) {
//    cout << "draw bounding rectangle " << x0 << ", " << y0 << " to " << x1 << ", " << y1 << endl;
    const TMatrix2D *m = 0;
    if (pen.getMatrix()) {
      m = pen.getMatrix();
      double tx, ty;
      m->map(0.0, 0.0, &tx, &ty);
      pen.push();
      pen.identity();
      pen.translate(tx, ty);
    }
    pen.setColor(TColor::FIGURE_SELECTION);
    pen.setLineWidth(1);
    pen.setAlpha(255);
    pen.drawRectangle(x0, y0, x1-x0, y1-y0);

    pen.setFillColor(255,255,255);
    TRectangle r;
    for(unsigned i=0; i<8; ++i) {
      getBoundingHandle(i, &r);
      pen.fillRectangle(r);
    }
    
    if (pen.getMatrix()) {
      pen.pop();
    }
  }
}

void
TSelectionTool::getBoundingHandle(unsigned i, TRectangle *r)
{
  TCoord w = x1 - x0;
  TCoord h = y1 - y0;
  switch(i) {
    case 0: r->set(x0-3  , y0-3  , 5, 5); break;
    case 1: r->set(x0+w/2, y0-3  , 5, 5); break;
    case 2: r->set(x0+w-1, y0-3  , 5, 5); break;
    case 3: r->set(x0+w-1, y0+h/2, 5, 5); break;
    case 4: r->set(x0+w-1, y0+h-1, 5, 5); break;
    case 5: r->set(x0+w/2, y0+h-1, 5, 5); break;
    case 6: r->set(x0-3,   y0+h-1, 5, 5); break;
    case 7: r->set(x0-3,   y0+h/2, 5, 5); break;
  }
}
      
void
TSelectionTool::calcSelectionsBoundingRectangle(TFigureEditor *fe)
{
  for(TFigureSet::const_iterator p = fe->selection.begin();
      p != fe->selection.end();
      ++p)
  {
    TRectangle r;
#if 1
    fe->getFigureShape(*p, &r, NULL);
#else
    (*p)->getShape(&r);
    if ( (*p)->mat ) {
      TPoint p0, p1;
      (*p)->mat->map(r.x, r.y,             &p0.x, &p0.y);
      (*p)->mat->map(r.x+r.w-1, r.y+r.h-1, &p1.x, &p1.y);
      r.set(p0, p1);
    }
#endif
    if (p==fe->selection.begin()) {
      x0 = r.x; 
      y0 = r.y;
      x1 = r.x + r.w;
      y1 = r.y + r.h;
    } else {
      if (x0 > r.x) x0 = r.x;
      if (y0 > r.y) y0 = r.y;
      if (x1 < r.x + r.w) x1 = r.x + r.w;
      if (y1 < r.y + r.h) y1 = r.y + r.h;
    }
  }
  
  bx = obx = x0;
  by = oby = y0;

  // map figure coordinates to screen coordinates
  if (fe->getMatrix()) {
    fe->getMatrix()->map(x0, y0, &x0, &y0);
    fe->getMatrix()->map(x1, y1, &x1, &y1);
  }
  --x1;
  --y1;
}

void
TSelectionTool::modelChanged(TFigureEditor *fe)
{
  invalidateBounding(fe);
  calcSelectionsBoundingRectangle(fe);
  invalidateBounding(fe);
}
