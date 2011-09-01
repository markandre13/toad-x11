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

#include "pentool.hh"
#include "fischland.hh"

#include <toad/boolmodel.hh>
#include <toad/integermodel.hh>

TPenTool*
TPenTool::getTool()
{
  static TPenTool* tool = 0;
  if (!tool)
    tool = new TPenTool();
  return tool;
}

void
TPenTool::cursor(TFigureEditor *fe, TCoord x, TCoord y)
{
  if (!path) {
    fe->getWindow()->setCursor(fischland::cursor[0]);
    return;
  }
  if (down) {
    fe->getWindow()->setCursor(fischland::cursor[3]);
    return;
  }
  if (!path->polygon.empty() &&
       path->polygon.front().x-fe->fuzziness<=x && x<=path->polygon.front().x+fe->fuzziness &&
       path->polygon.front().y-fe->fuzziness<=y && y<=path->polygon.front().y+fe->fuzziness)
  {
    fe->getWindow()->setCursor(fischland::cursor[1]);
    return;
  }
  if (!path->polygon.empty() &&
       path->polygon.back().x-fe->fuzziness<=x && x<=path->polygon.back().x+fe->fuzziness &&
       path->polygon.back().y-fe->fuzziness<=y && y<=path->polygon.back().y+fe->fuzziness)
  {
    fe->getWindow()->setCursor(fischland::cursor[2]);
    return;
  }
  fe->getWindow()->setCursor(fischland::cursor[0]);
}

void
TPenTool::stop(TFigureEditor *fe)
{
//cout << "stop pen" << endl;
//  fe->getWindow()->ungrabMouse();
  fe->getWindow()->setAllMouseMoveEvents(false);
  fe->getWindow()->setCursor(0);
  fe->state = TFigureEditor::STATE_NONE;
  if (path) {
/*
cout << "---------------" << endl;
for(unsigned i=0; i<path->corner.size(); ++i)
  cout << i << ": " << (unsigned)path->corner[i] << endl;
cout << "---------------" << endl;
*/
    if (path->polygon.size()>=4)
      fe->addFigure(path);
    else
      delete path;
    path = 0;
  }
  fe->invalidateWindow();
}

void
TPenTool::mouseEvent(TFigureEditor *fe, const TMouseEvent &me)
{
  TCoord x, y;
  fe->mouse2sheet(me.x, me.y, &x, &y);

  switch(me.type) {
    case TMouseEvent::ENTER:
      cursor(fe, x, y);
      break;
    case TMouseEvent::LDOWN:
      if (fe->state == TFigureEditor::STATE_NONE) {
        // start creation
        fe->state = TFigureEditor::STATE_CREATE;
        fe->getWindow()->setAllMouseMoveEvents(true);
        path = new TFPath();
        path->removeable = true;
        fe->getAttributes()->reason = TFigureAttributes::ALLCHANGED;
        path->setAttributes(fe->getAttributes());
      } else
      if (me.modifier() & MK_CONTROL || me.dblClick) {
        // end with open path
        stop(fe);
        fe->getWindow()->setCursor(fischland::cursor[0]);
        return;
      } else
      if (!path->polygon.empty() &&
          path->polygon.front().x-fe->fuzziness<=x && x<=path->polygon.front().x+fe->fuzziness &&
          path->polygon.front().y-fe->fuzziness<=y && y<=path->polygon.front().y+fe->fuzziness)
      {
        // end with closed path
        TPolygon::iterator p0, p1;
        if (path->polygon.size()%3 == 1) {
          p0 = path->polygon.end();
          --p0;
          p1 = p0;
          --p0;
          if (p0->x == p1->x && p0->y == p1->y) {
            path->corner.push_back(0);
          } else {
//            cout << "last corner is " << (unsigned)path->corner.back() << endl;
            if (path->corner.back()!=2) // ==1
              path->corner.back() = 4;
            path->corner.push_back(1);
          }
          path->polygon.addPoint(p1->x - ( p0->x - p1->x ),
                                 p1->y - ( p0->y - p1->y ));
        } else {
          path->corner.push_back(1);
        }
        p0 = p1 = path->polygon.begin();
        ++p0;
        TCoord x1 = p1->x, y1 = p1->y;
        path->polygon.addPoint(p1->x - ( p0->x - p1->x ),
                               p1->y - ( p0->y - p1->y ));
        path->polygon.addPoint(x1, y1);
        path->closed = true;
        stop(fe);
        fe->getWindow()->setCursor(fischland::cursor[0]);
        return;
      }
      //    )(      )(      )
      // 1 2  3 4  5  6 7  8
      // 1 2  0 1  2  0 1  2
      // 0 1  2 3  4  5 6  7
      // ^      ^       ^
//      cout << "going to add points: " << path->polygon.size() << endl;
      if (path->polygon.size()%3 == 1) {
        // if (x == polygon.back().x && y==polygon.back().y) {
        if (path->polygon.back().x-fe->fuzziness<=x && x<=path->polygon.back().x+fe->fuzziness &&
            path->polygon.back().y-fe->fuzziness<=y && y<=path->polygon.back().y+fe->fuzziness)
        {
          // corner after smooth curve
          path->polygon.addPoint(x, y);
//cout << "corner after smooth curve" << endl;
        } else {
          // smooth curve after smooth curve
//cout << "smooth curve after smooth curve" << endl;
          TPolygon::iterator p0, p1;
          p0 = path->polygon.end();
          --p0;
          p1 = p0;
          --p0;
          if (p0->x != p1->x || p0->y != p1->y)
            path->corner.back() = 4;
          path->polygon.addPoint(p1->x - ( p0->x - p1->x ),
                                 p1->y - ( p0->y - p1->y ));
          path->polygon.addPoint(x, y);
          path->polygon.addPoint(x, y);
          path->corner.push_back(0);
        }
      } else {
//cout << "hmm 1: add two start points ?o|" << endl;
        // this one add's point 0,1 and 2,3
        path->polygon.addPoint(x, y);
        path->polygon.addPoint(x, y);
        path->corner.push_back(0);
      }
//      cout << "points now: " << path->polygon.size() << endl;
      down = true;
      break;
    case TMouseEvent::MOVE:
      if (down) {
//        cout << "move with " << path->polygon.size() << ", " << path->polygon.size()%3 << endl;
        if (path->polygon.size()%3 == 2) {
//cout << "hmm 2" << endl;
          // make points 0,1 a smooth point
          path->corner.back() |= 2; // 2nd point has curve
          path->polygon.back().x = x;
          path->polygon.back().y = y;
        } else {
//cout << "hmm 3" << endl;
          path->corner.back() |= 1; // 1st point has curve
          TPolygon::iterator p0, p1;
          p0 = path->polygon.end();
          --p0;
          p1 = p0;
          --p0;
          p0->x = p1->x - ( x - p1->x );
          p0->y = p1->y - ( y - p1->y );
        }
      }
      cursor(fe, x, y);
      break;
    case TMouseEvent::LUP:

      down = false;
      cursor(fe, x, y);
      break;
  }
  fe->invalidateWindow();
}

void
TPenTool::keyEvent(TFigureEditor *fe, const TKeyEvent &ke)
{
  if (ke.type != TKeyEvent::DOWN)
    return;
  if (!path)
    return;
  TPolygon &polygon = path->polygon;
  vector<byte> &corner = path->corner;
  switch(ke.key()) {
    case TK_ESCAPE:
      delete path;
      path = 0;
      stop(fe);
      break;
    case TK_DELETE:
    case TK_BACKSPACE:
#if 0
      cout << "delete with " << polygon.size() 
           << " (" << polygon.size()%3 << "), "
           << corner.size() << endl;
#endif
      // fe->invalidateFigure(path);
      fe->invalidateWindow();
      if (polygon.size()==2) {
        if (corner[0] & 2) {
          polygon[1].x = polygon[0].x;
          polygon[1].y = polygon[0].y;
          corner[0] = 0;
        } else {
          polygon.erase(polygon.end()-2, polygon.end());
          corner.erase(corner.end()-1);
        }
      } else
      if (polygon.size()%3 == 1) { // 4 7 10 ...
#if 0
        if (corner[polygon.size()/3] & 1) {
          polygon[polygon.size()-2].x = polygon[polygon.size()-1].x;
          polygon[polygon.size()-2].y = polygon[polygon.size()-1].y;
          corner[polygon.size()/3] = 0;
        } else {
#endif
          polygon.erase(polygon.end()-2, polygon.end());
          corner.erase(corner.end()-1);
//        }
      } else
      if (polygon.size()%3 == 2) { // 5 8 11 ...
        polygon.erase(polygon.end()-1);
        corner.back() &= 1;
      }
#if 0
      cout << "delete with " << polygon.size() 
           << " (" << polygon.size()%3 << "), "
           << corner.size() << endl;
#endif
      break;
  }
}

void
TPenTool::paintSelection(TFigureEditor *fe, TPenBase &pen)
{
  if (!path)
    return;
  path->paint(pen, TFigure::EDIT);

  const TMatrix2D *m0 = pen.getMatrix();
  if (m0) {
    pen.push();
    pen.identity();
  }
  TPolygon &polygon = path->polygon;
  int i = polygon.size();

  if (i<4 || (i%3)!=1)  
    return;
  --i;
  TCoord x0 = polygon[i].x;
  TCoord y0 = polygon[i].y;
  TCoord x1 = polygon[i].x - (polygon[i-1].x - polygon[i].x);
  TCoord y1 = polygon[i].y - (polygon[i-1].y - polygon[i].y);
  if (m0) {
    m0->map(x0, y0, &x0, &y0);
    m0->map(x1, y1, &x1, &y1);
  }
  pen.drawLine(x0, y0, x1, y1);
  pen.fillCirclePC(x1-2,y1-2,6,6);

  if (m0) {
    pen.pop();
  }
}
