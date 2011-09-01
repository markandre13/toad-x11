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

#ifndef _FISCHLAND_SELECTIONTOOL_HH
#define _FISCHLAND_SELECTIONTOOL_HH 1

#include <toad/figureeditor.hh>
#include <toad/boolmodel.hh>
#include <toad/undo.hh>
#include <toad/undomanager.hh>
#include <map>

namespace fischland {

using namespace std;
using namespace toad;

class TSelectionTool:
  public TFigureTool
{
    // by default strokes and effects aren't scaled
    TBoolModel scale_strokes_and_fx; // Preferences > General

    enum {
      SELECT,
      SELECT_RECT,
      TRANSLATE,
      SCALE
    } action;
    bool down;

    unsigned handle;
    TCoord rx0, ry0, rx1, ry1;   // rectangle for rectangle selection
    TCoord bx, by, obx, oby;     // upper left corner of bounding in sheet space
    TCoord x0, y0, x1, y1;       // bounding rectangle
    TCoord ox0, oy0, ox1, oy1;   // bounding rectangle before resizing it
    TCoord last_x, last_y;       // last mouse position in figure coordinates when moving selection
    TCoord last_sx, last_sy;     // last mouse position in screen coordinates when moving selection
    TCoord skew_x, skew_y;       // skew between mouse
    TFigureSet selection;     // selected figures

  public:
    TSelectionTool() {
      scale_strokes_and_fx = false;
      down = false;
      action = SELECT;
    }
    static TSelectionTool* getTool();
  
    void mouseEvent(TFigureEditor *fe, const TMouseEvent &me);
    void keyEvent(TFigureEditor *fe, const TKeyEvent &ke);
    void paintSelection(TFigureEditor*, TPenBase &pen);
    void stop(TFigureEditor *fe);
    
    void invalidateBounding(TFigureEditor *fe);
    void invalidateOldBounding(TFigureEditor *fe);
    void getBoundingHandle(unsigned i, TRectangle *r);
    void calcSelectionsBoundingRectangle(TFigureEditor *fe);
    void modelChanged(TFigureEditor *fe);
};

} // namespace toad

#endif
