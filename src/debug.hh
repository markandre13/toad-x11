/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

// uncomment this to compile a debug version of TOAD (obsolete)
//#define DEBUG
#define TOAD_DEBUG

// uncomment this to compile a secure version of TOAD (extra parameter checks)
#define SECURE
//#define TOAD_SECURE

// Use event objects instead of argument lists for the mouse & keyboard
// methods (see HISTORY for more information)
//#define TOAD_EVENTCLASSES

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
  extern bool debug_fontmanager_x11;
  extern bool debug_fontmanager_ft;
  extern unsigned debug_table;

} // namespace toad

#endif
