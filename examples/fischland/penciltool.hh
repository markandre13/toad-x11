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

#ifndef _FISCHLAND_PENCILTOOL_HH
#define _FISCHLAND_PENCILTOOL_HH 1

#include "fpath.hh"
#include <toad/boolmodel.hh>
#include <toad/integermodel.hh>
#include <toad/figureeditor.hh>

using namespace toad;

class TPencilTool:
  public TFigureTool
{
    TPolygon polygon;
    bool closed;
    TFPath *back, *front;
    
    TIntegerModel fidelity;
    TIntegerModel smoothness;
    TBoolModel fillNewStrokes;
    TBoolModel keepSelected; // true by default!
    TBoolModel editSelectedPaths;
    TIntegerModel withinPixels;
    
  public:
    TPencilTool();
    static TPencilTool* getTool();
    void mouseEvent(TFigureEditor *fe, const TMouseEvent &me);
    void keyEvent(TFigureEditor *fe, const TKeyEvent &ke);
    void paintSelection(TFigureEditor *fe, TPenBase &pen);
    void stop(TFigureEditor*);
    TWindow* createEditor(TWindow *inWindow);
};

#endif
