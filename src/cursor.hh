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

#ifndef TCursor
#define TCursor TCursor

namespace toad {

class TWindow;

class TCursor
{
  public:
    enum EType {
      // Java compatible cursor types
      DEFAULT,
      CROSSHAIR,
      HAND,
      TEXT,
      WAIT,
      MOVE,
      N_RESIZE,
      NE_RESIZE,
      NW_RESIZE,
      S_RESIZE,
      SE_RESIZE,
      SW_RESIZE,
      W_RESIZE,
      E_RESIZE,
      
      // other stuff
      QUESTION,
      EXCHANGE,
      RESIZE,
      PIRATE,
      MOUSE,
      PENCIL,
      SPRAY,
      HORIZONTAL,
      VERTICAL,
      TARGET,
      DOT,
      CIRCLE,
      _MAX
    };

  #ifdef _TOAD_PRIVATE
    #ifdef __X11__
    static Cursor X11Cursor(EType);
    #endif
  #endif
};

} // namespace toad

#endif
