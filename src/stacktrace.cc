/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <toad/toad.hh>
#include <toad/stacktrace.hh>

/**
 * @defgroup debug Debugging
 *
 * Additonal features to ease debugging of applications written with
 * TOAD.
 */

using namespace toad;

static void* 
getReturnAddress(unsigned i)
{
  void * addr = NULL;
  switch(i+1) {
    case 1: addr = __builtin_return_address(1); break;
    case 2: addr = __builtin_return_address(2); break;
    case 3: addr = __builtin_return_address(3); break;
    case 4: addr = __builtin_return_address(4); break;
    case 5: addr = __builtin_return_address(5); break;
    case 6: addr = __builtin_return_address(6); break;
    case 7: addr = __builtin_return_address(7); break;
    case 8: addr = __builtin_return_address(8); break;
    case 9: addr = __builtin_return_address(9); break;
    case 10: addr = __builtin_return_address(10); break;
    case 11: addr = __builtin_return_address(11); break;
    case 12: addr = __builtin_return_address(12); break;
    case 13: addr = __builtin_return_address(13); break;
    case 14: addr = __builtin_return_address(14); break;
    case 15: addr = __builtin_return_address(15); break;
    case 16: addr = __builtin_return_address(16); break;
    case 17: addr = __builtin_return_address(17); break;
    case 18: addr = __builtin_return_address(18); break;
    case 19: addr = __builtin_return_address(19); break;
    case 20: addr = __builtin_return_address(20); break;
    case 21: addr = __builtin_return_address(21); break;
    case 22: addr = __builtin_return_address(22); break;
    case 23: addr = __builtin_return_address(23); break;
    case 24: addr = __builtin_return_address(24); break;
    case 25: addr = __builtin_return_address(25); break;
    case 26: addr = __builtin_return_address(26); break;
    case 27: addr = __builtin_return_address(27); break;
    case 28: addr = __builtin_return_address(28); break;
    case 29: addr = __builtin_return_address(29); break;
    case 30: addr = __builtin_return_address(30); break;
    case 31: addr = __builtin_return_address(31); break;
    case 32: addr = __builtin_return_address(32); break;
    case 33: addr = __builtin_return_address(33); break;
    case 34: addr = __builtin_return_address(34); break;
    case 35: addr = __builtin_return_address(35); break;
    case 36: addr = __builtin_return_address(36); break;
    case 37: addr = __builtin_return_address(37); break;
    case 38: addr = __builtin_return_address(38); break;
    case 39: addr = __builtin_return_address(39); break;
    case 40: addr = __builtin_return_address(40); break;
    case 41: addr = __builtin_return_address(41); break;
    case 42: addr = __builtin_return_address(42); break;
    case 43: addr = __builtin_return_address(43); break;
    case 44: addr = __builtin_return_address(44); break;
    case 45: addr = __builtin_return_address(45); break;
    case 46: addr = __builtin_return_address(46); break;
    case 47: addr = __builtin_return_address(47); break;
    case 48: addr = __builtin_return_address(48); break;
    case 49: addr = __builtin_return_address(49); break;
    case 50: addr = __builtin_return_address(50); break;
    case 51: addr = __builtin_return_address(51); break;
    case 52: addr = __builtin_return_address(52); break;
    case 53: addr = __builtin_return_address(53); break;
    case 54: addr = __builtin_return_address(54); break;
    case 55: addr = __builtin_return_address(55); break;
    case 56: addr = __builtin_return_address(56); break;
    case 57: addr = __builtin_return_address(57); break;
    case 58: addr = __builtin_return_address(58); break;
    case 59: addr = __builtin_return_address(59); break;
    case 60: addr = __builtin_return_address(60); break;
    case 61: addr = __builtin_return_address(61); break;
    case 62: addr = __builtin_return_address(62); break;
    case 63: addr = __builtin_return_address(63); break;
    case 64: addr = __builtin_return_address(64); break;
    case 65: addr = __builtin_return_address(65); break;
    case 66: addr = __builtin_return_address(66); break;
    case 67: addr = __builtin_return_address(67); break;
    case 68: addr = __builtin_return_address(68); break;
    case 69: addr = __builtin_return_address(69); break;
    case 70: addr = __builtin_return_address(70); break;
    case 71: addr = __builtin_return_address(71); break;
    case 72: addr = __builtin_return_address(72); break;
    case 73: addr = __builtin_return_address(73); break;
    case 74: addr = __builtin_return_address(74); break;
    case 75: addr = __builtin_return_address(75); break;
    case 76: addr = __builtin_return_address(76); break;
    case 77: addr = __builtin_return_address(77); break;
    case 78: addr = __builtin_return_address(78); break;
    case 79: addr = __builtin_return_address(79); break;
    case 80: addr = __builtin_return_address(80); break;
    case 81: addr = __builtin_return_address(81); break;
    case 82: addr = __builtin_return_address(82); break;
    case 83: addr = __builtin_return_address(83); break;
    case 84: addr = __builtin_return_address(84); break;
    case 85: addr = __builtin_return_address(85); break;
    case 86: addr = __builtin_return_address(86); break;
    case 87: addr = __builtin_return_address(87); break;
    case 88: addr = __builtin_return_address(88); break;
    case 89: addr = __builtin_return_address(89); break;
    case 90: addr = __builtin_return_address(90); break;
    case 91: addr = __builtin_return_address(91); break;
    case 92: addr = __builtin_return_address(92); break;
    case 93: addr = __builtin_return_address(93); break;
    case 94: addr = __builtin_return_address(94); break;
    case 95: addr = __builtin_return_address(95); break;
    case 96: addr = __builtin_return_address(96); break;
    case 97: addr = __builtin_return_address(97); break;
    case 98: addr = __builtin_return_address(98); break;
    case 99: addr = __builtin_return_address(99); break;
    case 100: addr = __builtin_return_address(100); break;
  }
  return addr;
}

/**
 * \ingroup debug
 * \class toad::TStackTrace
 *
 * Similar to printStackTrace but stores information about the stack
 * for later printing.
 *
 * \sa toad::printStackTrace
 */
TStackTrace::TStackTrace()
{
  for(size=0; size<100; ++size) {
    void * a = getReturnAddress(size);
    if (a==toad::top_address)
      break;
  }
  addr = (unsigned*) malloc(sizeof(unsigned)*size);
  for(unsigned i=0; i<size; ++i) {
    addr[i] = (unsigned)getReturnAddress(i);
  }
}

void
TStackTrace::print() const
{
  char buffer[4096];
  sprintf(buffer, "toadtrace %s ", toad::argv[0]);
  unsigned p = strlen(buffer);
  
  printf("stacktrace:\n");
  for(unsigned i=0; i<size; i++) {
    sprintf(buffer+p, "%08x ", addr[i]);
    p+=9;
    if (p>4096-20)
      break;
    if (addr==toad::top_address)
      break;
  }
  system(buffer);
}

/**
 * \ingroup debug
 *
 * Print a stacktrace.
 *
 * The output contains functions names, source file name and line 
 * numbers.
 *
 * It's intended to ease debugging.
 * 
 * \note
 * \li
 *   Unlike Java the line isn't the one containing the call but
 *   the command after the call.
 * \li 
 *   This function requires the external program 'toadtrace' being
 *   in the path which is a variant of addr2line with checks adresses
 *   in libraries also.
 */
void toad::printStackTrace()
{
  char buffer[4096];
  sprintf(buffer, "toadtrace %s ", toad::argv[0]);
  unsigned p = strlen(buffer);
  
  void *addr;
  printf("stacktrace:\n");
  for(unsigned i=0; i<100; i++) {
    addr = getReturnAddress(i);
    sprintf(buffer+p, "%08x ", (unsigned)addr);
    p+=9;
    if (p>4096-20)
      break;
    if (addr==toad::top_address)
      break;
  }
  system(buffer);
}
