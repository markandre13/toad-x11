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

/** \mainpage TOAD Reference Manual
 *
 * \section intro Introduction
 *
 * The introduction is yet to be written.
 */

#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstring>

#define _TOAD_PRIVATE

#include <toad/toad.hh>
#include <toad/debug.hh>
#include <toad/dialogeditor.hh>

using namespace toad;

namespace toad {
  extern char** argv;
  extern void* top_address;
}

char** toad::argv;
int toad::argc;
char** toad::envv;

void* toad::top_address;

/**
 * Initialize the TOAD library.
 *
 * You have to call this method from within main to let stack tracing
 * produce reasonable output.
 *
 * \param argc Number of argument from the main function.
 * \param argv Argument list from the main function.
 * \param envv Environment variables fro the main function.
 *
 * @sa terminate
 */
void
toad::initialize(int argc, char **&argv, char **envv)
{
  bool layouteditor = false;

  for(int i=1; i<argc; i++) {
    if (strcmp(argv[i], "--toad-debug-memory")==0) {
#ifdef DEBUG_MEMORY
      toad::debug_mem_start();
#else
      cerr << argv[i] << "isn't enabled in library" << endl;
#endif
    } else 
    if (strcmp(argv[i], "--toad-info")==0) {
      cout<< "TOAD C++ GUI Library " 
          << __TOAD_MAJOR__ << "."
          << __TOAD_MINOR__ << "."
          << __TOAD_SUBLVL__ << endl;
      cerr << "Enabled features:" << endl;
#ifdef DEBUG_MEMORY
      cerr << "  DEBUG_MEMORY" << endl;
#endif
#ifdef LIKE_ELECTRIC_FENCE
      cerr << "  LIKE_ELECTRIC_FENCE" << endl;
#endif
    } else
    if (strcmp(argv[i], "--layout-editor")==0 ||
        strcmp(argv[i], "--dialog-editor")==0) {
      layouteditor = true;
    } else {
      cerr << "unknown option " << argv[i] << endl;
    }
  }
  toad::argv = argv;
  toad::argc = argc;
  toad::envv = envv;

  // initialize stacktrace.cc
  toad::top_address = __builtin_return_address(1);

  // this is something other OO languages call class initialisation
  TOADBase::initTOAD();

  if (layouteditor)
    new TDialogEditor();
}

/**
 * \sa TOADBase::mainLoop
 */
int
toad::mainLoop()
{
  return TOADBase::mainLoop();
}

/**
 *
 * @note
 *   The names initialize and terminate were inspired by the IBM's XML
 *   library Xerces used in Apache.
 * @sa initialize
 */
void
toad::terminate()
{
  TOADBase::closeTOAD();

#ifdef DEBUG_MEMORY
  toad::debug_mem_end();
#endif
}
