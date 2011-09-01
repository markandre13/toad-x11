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

#ifndef _FISCHLAND_DIRECTSELECTIONTOOL_HH
#define _FISCHLAND_DIRECTSELECTIONTOOL_HH 1

#include <toad/figureeditor.hh>
#include <toad/boolmodel.hh>
#include <toad/undo.hh>
#include <toad/undomanager.hh>
#include <map>

namespace fischland {

using namespace std;
using namespace toad;

class TDirectSelectionTool:
  public TFigureTool
{
    bool grab;                // grabbed selection for moving
    bool hndl;                // grabbed handle
    unsigned handle;          // the handle number
    TFigure *figure;          // selected figure
    TRectangle oldshape;
    
    bool tht;                 // do translate handle transformation

    TPoint memo_pt;

  public:
    TDirectSelectionTool() {
      grab = false;
      hndl = false;
      figure = 0;
    }
    static TDirectSelectionTool* getTool();
  
    void mouseEvent(TFigureEditor *fe, const TMouseEvent &me);
    void paintSelection(TFigureEditor*, TPenBase &pen);
    void stop(TFigureEditor *fe);
};

} // namespace toad

#endif
