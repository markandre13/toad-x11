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

#ifndef _TOADX11_KEYBOARD_HH
#define _TOADX11_KEYBOARD_HH

// max number of chars that can be read from a SINGLE key
#define KB_BUFFER_SIZE 20

// modifier keys that came from "X11/X.h"

// Linux: Shift 1
//        Caps Lock 2
//        Ctrl  4
//        Alt   8
//        AltGr 8192
// MacOS: 
//        Ctrl  4
//        Apple 16
//        Alt   8192

#define MK_SHIFT   1
#define MK_LOCK    2
#define MK_CONTROL 4
#define MK_MOD1    8
#define MK_ALT     8
#define MK_MOD2    16
#define MK_APPLE   16
#define MK_MOD3    32
#define MK_MOD4    64
#define MK_MOD5    128
#define MK_LBUTTON 256
#define MK_MBUTTON 512
#define MK_RBUTTON 1024
#define MK_ROLLUP  2048
#define MK_ROLLDN  4096
#define MK_ALTGR   8192
//#define MK_ANY     (1<<15)

// a special TOAD modifier for double click events
#define MK_DOUBLE  (1<<14)

// key definitions that came from "X11/keysymdef.h"

typedef unsigned long TKey;

#define TK_BACKSPACE  0xFF08
#define TK_TAB        0xFF09
#define TK_LEFT_TAB   0xFE20
#define TK_LINEFEED   0xFF0A
#define TK_CLEAR      0xFF0B
#define TK_RETURN     0xFF0D
#define TK_KP_RETURN  0xFF8D
#define TK_PAUSE      0xFF13
#define TK_SCROLL_LOCK  0xFF14
#define TK_SYS_REQ    0xFF15
#define TK_ESCAPE     0xFF1B
#define TK_SHIFT_L    0xFFE1
#define TK_SHIFT_R    0xFFE2
#define TK_CONTROL_L  0xFFE3
#define TK_CONTROL_R  0xFFE4
#define TK_CAPS_LOCK  0xFFE5
#define TK_SHIFT_LOCK 0xFFE6
#define TK_DELETE     0xFFFF
#define TK_INSERT     0xFF63
#define TK_SPACE      0x0020

#define TK_HOME     0xFF50
#define TK_LEFT     0xFF51
#define TK_UP       0xFF52
#define TK_RIGHT    0xFF53
#define TK_DOWN     0xFF54
#define TK_PAGEUP   0xFF55
#define TK_PAGE_UP  0xFF55
#define TK_PAGEDOWN 0xFF56
#define TK_PAGE_DOWN 0xFF56
#define TK_END      0xFF57
#define TK_BEGIN    0xFF58

#define TK_F1       0xFFBE
#define TK_F2       0xFFBF
#define TK_F3       0xFFC0
#define TK_F4       0xFFC1
#define TK_F5       0xFFC2
#define TK_F6       0xFFC3
#define TK_F7       0xFFC4
#define TK_F8       0xFFC5
#define TK_F9       0xFFC6
#define TK_F10      0xFFC7
#define TK_F11      0xFFC8
#define TK_F12      0xFFC9
#define TK_F13      0xFFCA
#define TK_F14      0xFFCB
#define TK_F15      0xFFCC
#define TK_F16      0xFFCD
#define TK_F17      0xFFCE
#define TK_F18      0xFFCF
#define TK_F19      0xFFD0
#define TK_F20      0xFFD1

#endif
