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

#include "penciltool.hh"
#include "fischland.hh"

#include <toad/undomanager.hh>

#include <toad/textfield.hh>
#include <toad/checkbox.hh>

TPencilTool::TPencilTool()
{
  back = front = 0; 
  keepSelected=true;
  withinPixels = 10;  // snap to path
  smoothness   = 50*960;
}

TPencilTool*
TPencilTool::getTool()
{
  static TPencilTool* tool = 0;
  if (!tool)
    tool = new TPencilTool();
  return tool;
}

TWindow*
TPencilTool::createEditor(TWindow *inWindow)
{
  TWindow *w = new TWindow(inWindow, "Pencil Tool");
  w->setBorder(0);
  w->setBackground(TColor::DIALOG);
  w->setSize(320,350);
  
  new TTextField(w, "fidelity", &fidelity);
  new TTextField(w, "smoothness", &smoothness);
  new TCheckBox(w, "fill-new-strokes", &fillNewStrokes);
  new TCheckBox(w, "keep-selected", &keepSelected);
  new TCheckBox(w, "edit-selected-paths", &editSelectedPaths);
  new TTextField(w, "within-pixels", &withinPixels);
  w->loadLayout(RESOURCE("TPencilToolDialog.atv"));
  return w;
}

void
TPencilTool::paintSelection(TFigureEditor *fe, TPenBase &pen)
{
//cout << __PRETTY_FUNCTION__ << endl;
  // copied from TSelectionTool:
  pen.keepcolor = false;
  pen.setColor(TColor::FIGURE_SELECTION);

  pen.outline = true;
  pen.keepcolor = true;

  TFigureSet::const_iterator sp, se;
  sp = fe->selection.begin();
  se = fe->selection.end();  

  for(; sp != se; ++sp) {
    pen.push();
    if ((*sp)->mat)
      pen.multiply( (*sp)->mat );
    (*sp)->paint(pen);
    pen.pop();
  }  
  pen.outline = false;
  pen.keepcolor = false;
}

void
TPencilTool::stop(TFigureEditor *fe)
{
  fe->getWindow()->setAllMouseMoveEvents(false);
  fe->getWindow()->bCompressMotion = true;
  fe->getWindow()->setCursor(0);
  fe->state = TFigureEditor::STATE_NONE;
  fe->invalidateWindow();
}

void fitCurve(const TPolygon &in, TPolygon *out, int smoothness);

void
TPencilTool::keyEvent(TFigureEditor *fe, const TKeyEvent &ke)
{
  if (ke.key() != TK_CONTROL_L &&
      ke.key() != TK_CONTROL_R)
    return;
  if (ke.type != TKeyEvent::UP)
    fe->getWindow()->setCursor(fischland::cursor[4]);
  else
    fe->getWindow()->setCursor(fischland::cursor[5]);
}
 

void 
TPencilTool::mouseEvent(TFigureEditor *fe, const TMouseEvent &me)
{
  TCoord x, y;
  fe->mouse2sheet(me.x, me.y, &x, &y);

  switch(me.type) {
    case TMouseEvent::ENTER:
      if (me.modifier() & MK_CONTROL)
        fe->getWindow()->setCursor(fischland::cursor[4]);
      else
        fe->getWindow()->setCursor(fischland::cursor[5]);
      break;
    case TMouseEvent::LDOWN:
      // prepare to sample the freehand curve
      fe->getWindow()->setAllMouseMoveEvents(true);
      fe->getWindow()->bCompressMotion = false;
      polygon.clear();
      polygon.addPoint(x, y);
      closed = false;
      
      // check if we start at the head or tail of another line
      for(TFigureSet::const_iterator p = fe->selection.begin();
          p != fe->selection.end();
          ++p)
      {
        TFPath *f = dynamic_cast<TFPath*>(*p);
        if (f) {
/*
          cout << "found path" << endl;
          cout << "  front: " << f->polygon.front().x << ", " << f->polygon.front().y << endl;
          cout << "  back: " << f->polygon.back().x << ", " << f->polygon.back().y << endl;
          cout << "  mouse: " << x << ", " << y << endl;
          cout << "  fuzziness: " << fe->fuzziness << endl;
*/
          if (!f->closed &&
              f->polygon.back().x - fe->fuzziness <= x &&
              f->polygon.back().x + fe->fuzziness >= x &&
              f->polygon.back().y - fe->fuzziness <= y &&
              f->polygon.back().y + fe->fuzziness >= y )
          {
            back = f;
            break;
          } else
          if (!f->closed &&
              f->polygon.front().x - fe->fuzziness <= x &&
              f->polygon.front().x + fe->fuzziness >= x &&
              f->polygon.front().y - fe->fuzziness <= y &&
              f->polygon.front().y + fe->fuzziness >= y )
          {
            front = f;
            break;
          }
        }
      }
      break;

    case TMouseEvent::MOVE: {
      // when not in drawing mode, adjust the cursor when it is
      // near another selected path
      if (! (me.modifier() & MK_LBUTTON)) {
        for(TFigureSet::const_iterator p = fe->selection.begin();
            p != fe->selection.end();
            ++p)
        {
          TFPath *f = dynamic_cast<TFPath*>(*p);
          if (f) {
            if (!f->closed &&
                f->polygon.back().x - fe->fuzziness <= x &&
                f->polygon.back().x + fe->fuzziness >= x &&
                f->polygon.back().y - fe->fuzziness <= y &&
                f->polygon.back().y + fe->fuzziness >= y )
            {
              fe->getWindow()->setCursor(fischland::cursor[4]);
              return;
            } else
            if (!f->closed &&
                f->polygon.front().x - fe->fuzziness <= x &&
                f->polygon.front().x + fe->fuzziness >= x &&
                f->polygon.front().y - fe->fuzziness <= y &&
                f->polygon.front().y + fe->fuzziness >= y )
            {
              fe->getWindow()->setCursor(fischland::cursor[4]);
              return;
            }
          }
        }
        fe->getWindow()->setCursor(fischland::cursor[5]);
        return;
      }

      // closed := near end of other selected path
      if (
        ( front &&
          front->polygon.back().x-fe->fuzziness <= x && 
          front->polygon.back().x+fe->fuzziness >= x &&
          front->polygon.back().y-fe->fuzziness <= y &&
          front->polygon.back().y+fe->fuzziness >= y) ||
        ( back &&
          back->polygon.front().x-fe->fuzziness <= x && 
          back->polygon.front().x+fe->fuzziness >= x &&
          back->polygon.front().y-fe->fuzziness <= y &&
          back->polygon.front().y+fe->fuzziness >= y) ||
        (!polygon.empty() &&
          polygon.front().x-fe->fuzziness<=x && x<=polygon.front().x+fe->fuzziness &&
          polygon.front().y-fe->fuzziness<=y && y<=polygon.front().y+fe->fuzziness)
        )
      {
        fe->getWindow()->setCursor(fischland::cursor[4]);
        closed = true;
      } else {
        fe->getWindow()->setCursor(fischland::cursor[5]);
        closed = false;
      }
      
      // sample the new point
      polygon.addPoint(x,y);

      //TCairo pen(fe->getWindow()); // too slow

      // indicate the new point to the user
      TPen pen(fe->getWindow());
      TFigureAttributes *a = fe->getAttributes();
      pen.setAlpha(a->alpha);
      pen.setColor(a->linecolor);
      pen.setLineStyle(a->linestyle);
      pen.setLineWidth(1 /*a->linewidth*/);

      pen.setClipRect(fe->getVisible());
      TWindow *window = fe->getWindow();
      pen.translate(fe->getVisible().x,
                    fe->getVisible().y);
      pen.multiply(fe->getMatrix());
      TPolygon::size_type i = polygon.size();
      pen.drawLine(polygon[i-2].x, polygon[i-2].y,
                   polygon[i-1].x, polygon[i-1].y);
    } break;

    case TMouseEvent::LUP:
      if (polygon.size()>1) {
        TFPath *f = new TFPath();
        if (me.modifier() & MK_CONTROL || (closed&&!front&&!back)) {
          polygon.addPoint(polygon[0].x, polygon[0].y);
          f->closed = true;
        }
        fe->getAttributes()->reason = TFigureAttributes::ALLCHANGED;
        f->setAttributes(fe->getAttributes());

        fitCurve(polygon, &f->polygon, smoothness);

        // snap to another path
        // this could be improved: if we cross the other line, we cut
        // the new path, otherwise we extend it
        if (!f->closed) {
          TCoord x0, y0, d0, x1, y1, d1;
          d0 = d1 = fe->fuzziness*TFigure::RANGE*withinPixels.getValue();
        
          for(TFigureModel::const_iterator p = fe->getModel()->begin();
              p != fe->getModel()->end();
              ++p)
          {
            TFPath *path = dynamic_cast<TFPath*>(*p);
            if (!path)
              continue;
            TCoord x, y, d;
            d = path->findPointNear(f->polygon[0].x, f->polygon[0].y, &x, &y);
            if (d < d0) {
              d0 = d;
              x0 = x;
              y0 = y;
            }
            d = path->findPointNear(f->polygon.back().x, f->polygon.back().y, &x, &y);
            if (d < d1) {
              d1 = d;
              x1 = x;
              y1 = y;
            }
          }
          if ( d0 <= fe->fuzziness*2.0 ) {
            f->polygon[0].x = x0;
            f->polygon[0].y = y0;
          }
          if ( d1 <= fe->fuzziness*2.0 ) {
            f->polygon.back().x = x1;
            f->polygon.back().y = y1;
          }
        }
        
        // [] O O [] + [] O O [] 
        
        if (back) {
          // insert at the back of another path
          TUndoManager::beginUndoGrouping(fe->getModel());
          f->polygon.insert(f->polygon.begin(),
                            back->polygon.begin(),
                            back->polygon.end()-1);
          f->closed = closed;
          fe->deleteFigure(back);
          fe->addFigure(f);
          TUndoManager::endUndoGrouping();
          back = 0;
        } else
        if (front) {
          // insert at the front of another path
          TUndoManager::beginUndoGrouping(fe->getModel());
          TPoint pt;
          f->polygon.insert(f->polygon.begin(),
                            front->polygon.size()-1,
                            pt);
          for(size_t i=0, j=front->polygon.size()-1;
              i<front->polygon.size()-1;
              ++i, --j)
          {
            assert(i<front->polygon.size());
            assert(j<f->polygon.size());
            f->polygon[i] = front->polygon[j];
          }
          f->closed = closed;
          fe->deleteFigure(front);
          fe->addFigure(f);
          TUndoManager::endUndoGrouping();
          front = 0;
        } else {
          // just insert a new figure
          fe->addFigure(f);
          fe->invalidateFigure(f);
        }
        if (keepSelected) {
          fe->clearSelection();
          fe->selection.insert(f);
        }
        // set the corner flags
        f->corner.clear();
        if (f->polygon.size()>=3) {
          f->corner.push_back(2);
          for(size_t i=0; i<(f->polygon.size()+1)/3; ++i) {
            f->corner.push_back(4);
          }
          f->corner.push_back(1);
        }
      }
      polygon.clear();
      fe->getWindow()->bCompressMotion = true;
      break;
  }
}
