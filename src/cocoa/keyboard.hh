/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

/*
 * This files defines X11 constants to avoid the inclusion of X11
 * header files in programs written with TOAD.
 */

#ifndef _TOADCOCOA_KEYBOARD_HH
#define _TOADCOCOA_KEYBOARD_HH 1

// see /Developer/SDKs/MacOSX10.3.9.sdk/Developer/Headers/CFMCarbon/Events.h

#define MK_SHIFT     (1 << 9)
#define MK_LOCK      (1 << 10)
#define MK_CONTROL   (1 << 12)
#define MK_ALT       (1 << 11)
#define MK_APPLE     (1 << 8)

#define MK_ALTGR   0
#define MK_LBUTTON 1
#define MK_MBUTTON 2
#define MK_RBUTTON 4
#define MK_DOUBLE  8

#define TK_SHIFT_L 56
#define TK_SHIFT_R 56

typedef unsigned short TKey;

#define TK_RETURN    36
#define TK_KP_RETURN 0xffff
#define TK_TAB       48
#define TK_LEFT_TAB  0xffff
#define TK_INSERT    0xffff
#define TK_SPACE     49
#define TK_BACKSPACE 51
#define TK_ESCAPE    53
#define TK_SHIFT     56
#define TK_CONTROL_L   0xffff
#define TK_CONTROL_R   0xffff
#define TK_HOME      115
#define TK_PAGEUP    116
#define TK_DELETE    117
#define TK_END       119
#define TK_PAGEDOWN  121
#define TK_LEFT      123
#define TK_RIGHT     124   
#define TK_DOWN      125   
#define TK_UP        126   
#define TK_F1        122   
#define TK_F2        120   
#define TK_F3        99     
#define TK_F4        118   
#define TK_F5        96 
#define TK_F6        97 
#define TK_F7        98 
#define TK_F8        100
#define TK_F9        101
#define TK_F10       109
#define TK_F11       103
#define TK_F12       111
#define TK_F13       0xffff
#define TK_F14       0xffff
#define TK_F15       0xffff
#define TK_F16       0xffff
#define TK_F17       0xffff
#define TK_F18       0xffff
#define TK_F19       0xffff
#define TK_F20       0xffff

#endif
