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

#ifndef _TOAD_TOADBASE_HH
#define _TOAD_TOADBASE_HH 1

// TOAD version codes
//-----------------------------------------------
#define __TOAD__        1

#define __TOAD_MAJOR__  0
#define __TOAD_MINOR__  64
#define __TOAD_SUBLVL__ 0

#define TOAD_VERSION(A,B,C) __TOAD_MAJOR__==A && __TOAD_MINOR__==B && __TOAD_SUBLVL__==C

#include <toad/os.hh>

// hiding X11 types (better to be generated from the X11 headers!)
//-----------------------------------------------

#ifdef __X11__

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace toad {
  typedef GC _TOAD_GC;
  typedef Drawable _TOAD_DRAWABLE;
  typedef Window _TOAD_WINDOW;
  typedef XFontStruct* _TOAD_FONTSTRUCT;
  typedef Font _TOAD_FONT;
  typedef Cursor _TOAD_CURSOR;
  typedef Region _TOAD_REGION;
} // namespace toad
#endif

#ifdef __XCB__
  #include <xcb/xcb.h>
#endif

#ifdef __WIN32__
  #define STRICT
  #define W32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <cassert>

// ANSI headers needed everywhere in TOAD
//-----------------------------------------------
#include <typeinfo>
#include <stdexcept>
#include <iostream>
#include <string>

#include <toad/types.hh>
#include <toad/debug.hh>

#ifdef __X11__
#include <toad/X11/keyboard.hh>
#endif

#ifdef __WIN32__
#include <toad/w32/keyboard.hh>
#endif

#include <toad/connect.hh>

namespace toad {

using namespace std; // use std within namesapce toad

class TCommand;

void debug_mem_start();
void debug_mem_end();
  
extern char ** argv;
extern int argc;
extern char ** envv;

extern void* top_address;

class TWindow;
class TBitmap;
class TSignal;
class TFont;
class TDnDObject;
class TDialog;
class TDialogEditor;

#ifdef __X11__
void initialize(int argc, char **&argv, char **envv);
#endif

#ifdef __WIN32__
void initialize(HINSTANCE, HINSTANCE, LPSTR, int);
#endif

int mainLoop();
void terminate();

enum EEventFilterPos {
  KF_GLOBAL,
  KF_TOP_DOMAIN,
  KF_DOMAIN,
  KF_WINDOW
};

class TWindowEvent;
class TMouseEvent;
class TKeyEvent;

class TEventFilter
{
    friend class TOADBase;
  public:
    virtual bool windowEvent(TWindowEvent&);
    virtual bool mouseEvent(TMouseEvent&);
    virtual bool keyEvent(TKeyEvent&);
//  private:
    TEventFilter *next;
    EEventFilterPos pos;
    void *ptr;
};

#ifdef _TOAD_PRIVATE
extern TEventFilter *global_evt_filter;
#endif

enum EWindowPlacement {
  PLACE_SCREEN_CENTER,
  PLACE_SCREEN_RANDOM,
  PLACE_PARENT_CENTER,
  PLACE_PARENT_RANDOM,
  PLACE_MOUSE_POINTER,
  PLACE_CORNER_MOUSE_POINTER,
  PLACE_PULLDOWN,
  PLACE_TOOLTIP
};

#ifdef _TOAD_PRIVATE

#ifdef __X11__
extern Display *x11display;
extern Colormap x11colormap;
extern Visual* x11visual;
extern int x11depth;
extern XEvent x11event;
extern _TOAD_GC x11gc;
extern int x11screen;
extern XContext nClassContext;
extern Atom xaWMSaveYourself;
extern Atom xaWMDeleteWindow;
extern Atom xaWMMotifHints;
#endif

#ifdef __XCB__
extern xcb_connection_t *xcbConnection;
extern xcb_screen_t     *xcbScreen;
#endif

#ifdef __WIN32__
extern HINSTANCE w32instance;
extern int w32cmdshow;
#endif

#endif

class TOADBase
{
    friend class TWindow;
    static bool lock_paint_queue;

  public:
    virtual ~TOADBase();
  
    static bool bAppIsRunning;
    static TWindow* wndTopPopup;
    static bool bSimulatedAutomaticGrab;
    static TFont& getDefaultFont();
    static void setDefaultFont(TFont*);
    static void setColorLimit(unsigned);

    // `class constructor/destructor'
    //-------------------------------
    static bool initialize();
    static void terminate();
    static void initColor();
    static void initIO(int);
    static void initXInput();
    static void initDnD();
    static void select();
    
    static void closeXInput();

    // focus management
    //------------------
  private:
    static void focusNewWindow(TWindow* wnd);
    static void focusDelWindow(TWindow* wnd);
    static void domainToWindow(TWindow *wnd);
    static void handleKeyDown(TKey key, char* t, unsigned m);
    static void handleKeyUp(TKey key, char* t, unsigned m);
  public:
    static TWindow* getFocusWindow();
    static void setFocusWindow(TWindow* wnd);
    
    static void insertEventFilter(TEventFilter*, TWindow*, EEventFilterPos);
    static void removeEventFilter(TEventFilter*);

    // without category
    //------------------
    static int mainLoop();
    static void placeWindow(TWindow *window, EWindowPlacement how, TWindow *parent=NULL);

    static void postQuitMessage(int);
    static void flush();

    static const string& getExecutablePath();
    static const string& getExecutableName();

    static void bell(int volume = 0, int frequency=-1);

    // message handling
    //------------------
    static void sendMessage(TCommand*);
    static void removeMessage(void*);
    static void removeAllIntMsg();
    
    static void sendMessageDeleteWindow(TWindow*);

#ifdef __X11__
    static bool peekMessage();
    static bool handleMessage();
#endif

    static void lockPaintQueue() { lock_paint_queue = true; }
    static void unlockPaintQueue() { lock_paint_queue = false; }
    
    static void doModalLoop(TWindow*);
    static void endModalLoop(TWindow*);
    static void endAllModalLoops();
  
    static int getScreenWidth();
    static int getScreenHeight();
    static void getMousePos(int*,int*);
    static void setMousePos(int,int);

  private:
    // tooltip support (implemented in tooltip.cc)
    //--------------------------------------------
    static void toolTipOpen(TWindow*);
    static void toolTipClose();

    // drag'n drop support (implemented in dragndrop.cc)
    //--------------------------------------------
  public:
    static void startDrag(TDnDObject *obj, unsigned modifier = 0);
    static string getSelection();
    static void setSelection(const string &);

  private:
    #ifdef _TOAD_PRIVATE
    
    #ifdef __X11__
    static void DnDNewShellWindow(TWindow*);
    static bool DnDMotionNotify(XEvent &event);
    static bool DnDButtonRelease(XEvent &event);
    static bool DnDClientMessage(XEvent &event);
    static bool DnDSelectionNotify(XEvent &event);
    static bool DnDSelectionRequest(XEvent &event);
    static bool DnDSelectionClear(XEvent &event);
    #endif

    #endif
};

// wrappers for static methods inside the TOADBase class
// this is the recommended interface
inline TWindow* getFocusWindow() { return TOADBase::getFocusWindow(); }
inline void setFocusWindow(TWindow* wnd) { TOADBase::setFocusWindow(wnd); }
inline void insertEventFilter(TEventFilter *f, TWindow *w, EEventFilterPos p) { TOADBase::insertEventFilter(f, w, p); }
inline void removeEventFilter(TEventFilter *f) { TOADBase::removeEventFilter(f); }
inline void placeWindow(TWindow *w, EWindowPlacement h, TWindow *p=0) { TOADBase::placeWindow(w, h, p); }
inline void postQuitMessage(int ret) { TOADBase::postQuitMessage(ret); }
inline void flush() { TOADBase::flush(); }
inline const string& getExecutablePath() { return TOADBase::getExecutablePath(); }
inline const string& getExecutableName() { return TOADBase::getExecutableName(); }
inline void bell(int volume = 0, int frequency=-1) { TOADBase::bell(volume, frequency); }
inline void sendMessage(TCommand *cmd) { TOADBase::sendMessage(cmd); }
inline void removeMessage(void *cmd) { TOADBase::removeMessage(cmd); }
inline void removeAllIntMsg() { TOADBase::removeAllIntMsg(); }
inline void sendMessageDeleteWindow(TWindow *w) { TOADBase::sendMessageDeleteWindow(w); }
inline int getScreenWidth() { return TOADBase::getScreenWidth(); }
inline int getScreenHeight() { return TOADBase::getScreenHeight(); }
inline void getMousePos(int *x, int *y) { return TOADBase::getMousePos(x, y); }
inline void setMousePos(int x,int y) { return TOADBase::setMousePos(x, y); }
inline void startDrag(TDnDObject *obj, unsigned modifier = 0) { TOADBase::startDrag(obj, modifier); }
inline TFont& getDefaultFont() { return TOADBase::getDefaultFont(); }

} // namespace toad

#endif
