/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#include <toad/toad.hh>
#include <toad/control.hh>

using namespace toad;

/**
 * \class toad::TControl
 * <B>Attention: A current idea is to rename this class in `TView'
 * during the introduction of the MVC paradigm.</B>
 * <P>
 * The base class for controllers like radiobuttons, checkboxes, textfields
 * and the like.
 */

/**
 * Enable/Disable the control and call enabled() afterwards.<BR>
 * It's up to the implementation of the control to decide what
 * to do with it.
 */
void TControl::setEnabled(bool b)
{
  if (b==_enabled)
    return;
  _enabled = b;
  enabled();
}

/**
 * Called after the control was enabled/disabled by a call to SetEnabled().
 * The default behaviour is to call Invalidate() and to accept/reject the
 * keyboard focus.
 */
void TControl::enabled()
{
  bNoFocus = !isEnabled();
  invalidateWindow();
}

/**
 * The default action of <I>focus(bool)</I> in TControl is to call
 * <I>TWindow::Invalidate()</I>.
 */
void TControl::focus(bool)
{
  invalidateWindow();
}
