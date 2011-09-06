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

#include <toad/os.hh>

#include <errno.h>
#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
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
}

char** toad::argv;
int toad::argc;
char** toad::envv;

extern void createTOADResource();

#ifdef __COCOA__
static NSAutoreleasePool *pool;

@interface MyDelegate : NSObject
{
}
- (void) createWindow;
- (void) createMenu;  
- (void) applicationWillFinishLaunching:(NSNotification *)notification;
- (void) applicationDidFinishLaunching:(NSNotification *)notification;
@end

@implementation MyDelegate : NSObject
- (void) dealloc
{
  [super dealloc];
  // RELEASE (myWindow);
}
 
- (void) createWindow
{
  printf("create windows\n");
  TWindow::createParentless();
}
 
// this one creates a simple custom menu without NIB files
- (void) createMenu
{
  printf("create menu\n");
  NSMenu *m0, *m1;

  // menu = AUTORELEASE ([NSMenu new]);
  // m0 = [NSApp mainMenu];
  m0 = [NSMenu new];

  m1 = [NSMenu new];
  [m1 setTitle: @"myMenu"];
  [m1 setAutoenablesItems:YES];

  [m0 addItemWithTitle: @"myMenu" action: NULL keyEquivalent: @""];
  [m0 setSubmenu: m1 forItem: [m0 itemWithTitle: @"myMenu"]];
      
  [m1 addItemWithTitle: @"Quit" action: @selector (terminate:) keyEquivalent: @"q"];

  [NSApp setMainMenu: m0];
}
 
- (void)applicationWillFinishLaunching:(NSNotification *)notification;
{
  [self createWindow];
}
- (void) applicationDidFinishLaunching: (NSNotification *)notification;
{
  [self createMenu];
}
@end
#endif

namespace {

#ifdef __X11__
#ifdef HAVE_LIBXFT
string fontengine("freetype");
#else
string fontengine("x11");
#endif
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
#ifdef __X11__
          fontengine = in.value;
#endif
          break;
        }
        if (in.attribute == "font" && in.type.empty()) {
          fontname = in.value;
          break;
        }
        if (in.attribute == "debug-fontengine-x11" && in.type.empty()) {
#ifdef __X11__
          debug_fontmanager_x11 = str2bool(in.value);
#endif
          break;
        }
        if (in.attribute == "debug-fontengine-ft" && in.type.empty()) {
#ifdef __X11__
          debug_fontmanager_ft = str2bool(in.value);
#endif
          break;
        }
        if (in.attribute == "scrollwheel-slowdown" && in.type.empty()) {
          // the Wacom mouse wheel can send multiple clicks where only
          // one is expected, this is to slow it down for TTextField
          scrollwheel_slowdown = atoi(in.value.c_str());
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
#ifdef __COCOA__
  pool = [NSAutoreleasePool new];
#endif

  bool layouteditor = false;

  parseInitFile("/etc/toadrc");
  
  string ini = getenv("HOME");
  ini+="/.toadrc";
  parseInitFile(ini);

  for(int i=1; i<argc; i++) {
    if (strcmp(argv[i], "--toad-info")==0) {
      cout<< "TOAD C++ GUI Library " 
          << __TOAD_MAJOR__ << "."
          << __TOAD_MINOR__ << "."
          << __TOAD_SUBLVL__ << endl;
    } else
    if (strcmp(argv[i], "--layout-editor")==0) {
      layouteditor = true;
    } else 
#ifdef __X11__
    if (strcmp(argv[i], "--font-engine")==0) {
      if (i+1>=argc) {
        cerr << "error: missing option for argument " << argv[i] << endl;
        exit(1);
      }
      fontengine = argv[++i];
    } else
#endif
      cerr << "unknown option " << argv[i] << endl;
  }

#ifdef __X11__
  if (!TFontManager::setDefaultByName(fontengine)) {  
    cerr << "error: unknown font engine '" << fontengine << "', try x11 or freetype" << endl;
    exit(1);
  }
#endif
  
  toad::argv = argv;
  toad::argc = argc;
  toad::envv = envv;

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
#ifdef __X11__
  return TOADBase::mainLoop();
#endif

#ifdef __COCOA__
  // setup NSApp
  [NSApplication sharedApplication];
  
  // add a delegate to NSApp to customize the application
  [NSApp setDelegate: [MyDelegate new]];

  return NSApplicationMain(argc, (const char **) argv);
#endif
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
#ifdef __COCOA__
  [pool release];
#endif

  TOADBase::terminate();
  TUndoManager::terminate();
}
