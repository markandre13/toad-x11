/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-Andre Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_UTF8_HH
#define _TOAD_UTF8_HH

#include <string>
#include <libXutf8/Xutf8.h>

namespace toad {

using namespace std;

/**
 * Set *cx to the next UTF-8 character in text.
 */
inline void 
utf8inc(const string &text, unsigned int *cx)
{
  ++*cx;
  while( ((unsigned char)text[*cx] & 0xC0) == 0x80)
    ++*cx;
}

/**
 * Set *cx to the previous UTF-8 character in text.
 */
inline void 
utf8dec(const string &text, unsigned int *cx)
{
  --*cx;
  while( ((unsigned char)text[*cx] & 0xC0) == 0x80)
    --*cx;
}

/**
 * Return the number of characters in text from start to start+bytelen.
 */
int utf8charcount(const string &text, int start, int bytelen);

/**
 * Return the number for bytes used to store 'charlen' characters
 * beginning at 'start' in 'text'.
 */
int utf8bytecount(const string &text, int start, int charlen);

/**
 * Return the number of bytes required to store the character at position
 * 'pos' in 'text'.
 */
inline int
utf8charsize(const string &text, int pos)
{
  return utf8bytecount(text, pos, 1);
}

string utf8fromwchar(wchar_t c);

} // namespace toad

#endif
