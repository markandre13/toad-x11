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

#ifndef _TOAD_EXCEPTION_HH
#define _TOAD_EXCEPTION_HH 1

#include <exception>
#include <string>
#include <iostream>

namespace toad {

class TException:
  public exception
{
      std::string msg;
    public:
      explicit TException(const std::string &what):exception(), msg(what) {}
      virtual ~TException() throw() { }

      void printStackTrace() const { }
      void print() const {
        cerr << msg << endl;
      }

      const std::string& getMessage() const { return msg; }
      const char * what() const throw() { return msg.c_str(); }
};

} // namespace toad

#endif
