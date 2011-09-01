/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _FISCHLAND_TOOLBOX_HH
#define _FISCHLAND_TOOLBOX_HH 1

#include <toad/dialog.hh>
#include <toad/figureeditor.hh>
#include <toad/table.hh>

namespace fischland {

using namespace toad;

class TToolBox:
  public toad::TDialog
{
    typedef TDialog super;
    TBitmap bmp;
  public:
    TToolBox(TWindow *p, const std::string &t);
    void paint();

    TSingleSelectionModel *lw_sm, *ls_sm, *am_sm, *at_sm;
    void preferencesChanged();

    static TToolBox *toolbox;
    static PFigureAttributes preferences;
};

} // namespace fischland

#endif
