/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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
#include <fstream>

#define _TOAD_PRIVATE

#include <toad/toad.hh>
#include <toad/debug.hh>
#include <toad/dialogeditor.hh>
#include <toad/undomanager.hh>
#include <toad/font.hh>

using namespace toad;

namespace toad {
  extern char** argv;
  extern void* top_address;
}

char** toad::argv;
int toad::argc;
char** toad::envv;

void* toad::top_address;

extern void createTOADResource();

namespace {

#ifdef HAVE_LIBXFT
string fontengine("freetype");
#else
string fontengine("x11");
#endif
string fontname("arial,helvetica,sans-serif");

static bool
str2bool(const string &str) {
  if (str=="true")
    return true;
  if (str=="false")
    return false;
  cout << "unexpected bool value " << str << endl;
  return false;
}

void
parseInitFile(const string &filename)
{  
  ifstream is(filename.c_str());
  if (!is) 
    return;
  TATVParser in(&is);
  while(in.parse()) {
    switch(in.what) {
      case ATV_VALUE:
        if (in.attribute == "fontengine" && in.type.empty()) {
          fontengine = in.value;
          break;
        }
        if (in.attribute == "font" && in.type.empty()) {
          fontname = in.value;
          break;
        }
        if (in.attribute == "debug-fontengine-x11" && in.type.empty()) {
          debug_fontmanager_x11 = str2bool(in.value);
          break;
        }
        if (in.attribute == "debug-fontengine-ft" && in.type.empty()) {
          debug_fontmanager_ft = str2bool(in.value);
          break;
        }
         cerr << "unexpected entry '" << in.attribute << "' in file " << filename << endl;
        break;
      default:
        cerr << "unexpected entry in file " << filename << endl;
    }
  }
}

} // namespace

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

  parseInitFile("/etc/toadrc");
  
  string ini = getenv("HOME");
  ini+="/.toadrc";
  parseInitFile(ini);

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
    if (strcmp(argv[i], "--layout-editor")==0) {
      layouteditor = true;
    } else 
    if (strcmp(argv[i], "--font-engine")==0) {
      if (i+1>=argc) {
        cerr << "error: missing option for argument " << argv[i] << endl;
        exit(1);
      }
      fontengine = argv[++i];
    } else {
      cerr << "unknown option " << argv[i] << endl;
    }
  }

  if (!TFontManager::setDefaultByName(fontengine)) {  
    cerr << "error: unknown font engine '" << fontengine << "', try x11 or freetype" << endl;
    exit(1);
  }
  
  TFont::default_font.setFont(fontname);
  
  toad::argv = argv;
  toad::argc = argc;
  toad::envv = envv;

  // initialize stacktrace.cc
  toad::top_address = __builtin_return_address(1);

  // register memory files from the resource directory
  createTOADResource();

  // this is something other OO languages call class initialisation
  TOADBase::initialize();

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
  TOADBase::terminate();
  TUndoManager::terminate();

#ifdef DEBUG_MEMORY
  toad::debug_mem_end();
#endif
}
