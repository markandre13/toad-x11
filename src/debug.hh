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

// uncomment this to compile a debug version of TOAD
#define DEBUG
#define TOAD_DEBUG

// uncomment this to compile a secure version of TOAD (extra parameter checks)
#define SECURE
//#define TOAD_SECURE

// Use event objects instead of argument lists for the mouse & keyboard
// methods (see HISTORY for more information)
//#define TOAD_EVENTCLASSES

// Enable the '--toad-debug-memory' command line parameter for applications
// and provide the location of the new statement instead of malloc for
// the GNU C libraries mtrace utility, by provinding a new implementation
// of the new and delete operator.
// #define DEBUG_MEMORY

// Don't free memory but print stacktraces when memory was tried to
// free twice. The printed stacktraces are: locations of the allocation,
// location of the first free and location of the second free. Works only
// for C++ 'new' and 'delete' operator when DEBUG_MEMORY was defined.
// #define LIKE_ELECTRIC_FENCE

// uncomment this to enable the ENTRYEXIT class, nice when also the debugger 
// fails to locate the segmentation fault
//#define TRACE

//#define TRACE_MUTEX

// trace new/delete calls
//#define TRACE_MEMORY

namespace toad {

//+-----------------+
//| ENTRYEXIT class |
//+-----------------+
#ifdef TRACE
  class _entryexit
  {
   private:
     const char *s1;
   public:
     _entryexit(const char *t1){s1=t1;fprintf(stderr,"ENTRY: %s\n",s1);}
     ~_entryexit(){fprintf(stderr,"EXIT : %s\n",s1);}
  };
  #define ENTRYEXIT(str) _entryexit _eeobj(str)
#else
  #define ENTRYEXIT(str)
#endif


#ifdef TOAD_DEBUG

// these flags toggle the output of debug messages for the various modules
// of TOAD:
  extern bool debug_menubutton;

} // namespace toad

#endif
