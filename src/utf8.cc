/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-Andre Hopf <mhopf@mark13.org>
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

#include <toad/utf8.hh>

namespace toad {

/**
 * Return the number of characters in text from start to start+bytelen.
 */
size_t
toad::utf8charcount(const string &text, size_t start, size_t bytelen)
{
  return XCountUtf8Char((const unsigned char*)text.c_str()+start, bytelen);
}

/**
 * Return the number for bytes used to store 'charlen' characters
 * beginning at 'start' in 'text'.
 */
size_t
toad::utf8bytecount(const string &text, size_t start, size_t charlen)
{
  size_t result = 0;
  unsigned char *ptr = (unsigned char*)text.c_str() + start;
  while(charlen>0) {
    ++ptr;
    --charlen;
    ++result;
    while( (*ptr & 0xC0) == 0x80 ) {
      ++result;
      ++ptr;
    }
  }
  return result;
}

string
toad::utf8fromwchar(wchar_t c)
{
  string result;
  if (c<=0x7f) {
    result.append(1, c);
  } else
  if (c<=0x7ff) {
    int c2 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c1 = c | 0xc0;
    result.append(1, (char)c1);
    result.append(1, (char)c2);
  } else
  if (c<=0xffff) {
    int c3 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c2 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c1 = c | 0xe0;
    result.append(1, (char)c1);
    result.append(1, (char)c2);
    result.append(1, (char)c3);
  } else
  if (c<=0x1fffff) {
    int c4 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c3 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c2 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c1 = c | 0xf0;
    result.append(1, (char)c1);
    result.append(1, (char)c2);
    result.append(1, (char)c3);
    result.append(1, (char)c4);
  } else
  if (c<=0x3ffffff) {
    int c5 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c4 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c3 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c2 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c1 = c | 0xf8;
    result.append(1, (char)c1);
    result.append(1, (char)c2);
    result.append(1, (char)c3);
    result.append(1, (char)c4);
    result.append(1, (char)c5);
  } else {
    int c6 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c5 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c4 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c3 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c2 = (c & 0x3f) | 0x80;
    c >>= 6;
    int c1 = c | 0xfc;
    result.append(1, (char)c1);
    result.append(1, (char)c2);
    result.append(1, (char)c3);
    result.append(1, (char)c4);
    result.append(1, (char)c5);
    result.append(1, (char)c6);
  }
  return result;
}

} // namespace toad
