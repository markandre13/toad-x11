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

/*
 * This file looks really nasty because it's the oldest and has seen a
 * lot of modifications too. I will change this in the future...
 */

#include <errno.h>
#include <toad/os.hh>
#include <toad/config.h>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#endif

#include <cstdarg>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __X11__
#ifdef HAVE_LIBXUTF8
#include "xutf8/Xutf8.h"
#endif

#include <sys/wait.h>
#endif

#include <deque>
#include <set>
#include <vector>

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/font.hh>
#include <toad/region.hh>
#include <toad/dragndrop.hh>
#include <toad/figure.hh>
#include <toad/dialogeditor.hh>

// TMessageBox
#include <toad/dialog.hh>
#include <toad/messagebox.hh>

#define SECURE

using namespace toad;

#ifdef __X11__
static string GetWindowProperty(Window source, Atom property, Atom type);
static string AtomName(Atom atom) {
  string result = "(None)";
  if (atom) {
    char *name = XGetAtomName(toad::x11display, atom);
    result = name;
    XFree(name);
  }
  return result;
}
#endif

static string selection_kludge_data;
static bool selection_kludge_flag;
static bool selection_kludge_owner = false;

// define this for a periodic screen update every 1/12 seconds
#define PERIODIC_PAINT

namespace toad {
#ifdef __X11__
extern XIC xic_current;
#endif
} // namespace toad


// X11 data
//---------------------------------------------------------------------------
#ifdef __X11__
Display*  toad::x11display = NULL;
Visual*   toad::x11visual = NULL;
int       toad::x11depth = 0;
int       toad::x11screen;


//! \ingroup directx
XEvent    toad::x11event;
GC        toad::x11gc = 0;
XContext  toad::nClassContext;
Atom      toad::xaWMSaveYourself;
Atom      toad::xaWMDeleteWindow;
Atom      toad::xaWMMotifHints;

static Atom xaWMProtocols;
static Atom xaUTF8_STRING;
static Atom xaTEXT;
#endif

TEventFilter * toad::global_evt_filter = 0;

// TOAD data
//---------------------------------------------------------------------------
PFont    toad::default_font;
PFont    toad::bold_font;;
unsigned toad::scrollwheel_slowdown=1;

TFont& 
TOADBase::getDefaultFont() {
  return *default_font;
}

int       nStatus;
bool      TOADBase::bAppIsRunning=true;
TWindow*  TOADBase::wndTopPopup;
bool      TOADBase::bSimulatedAutomaticGrab;
bool      TOADBase::lock_paint_queue = false;

namespace {
struct TModalNode {
  TWindow *wnd;
  bool running;
};
} // namespace

typedef vector<TModalNode*> TModalStack;
static TModalStack modal_stack;

fd_set _io_set_rd, _io_set_wr, _io_set_ex;

static string executable_path;
static string executable_name;

const string& TOADBase::getExecutablePath()
{
  return executable_path;
}

const string& TOADBase::getExecutableName()
{
  return executable_name;
}

/**
 * called by 'TOADBase::Open' to set the 'executable_path' string
 */
static void get_executable_path(char *prgname)
{
  char buffer[PATH_MAX+1];
  string cwd;
  executable_path = prgname;
  int p = executable_path.rfind('/');
  if (p>0) {
    executable_name = executable_path.substr(p+1);
    executable_path = executable_path.substr(0,p)+"/"; 
  } else {
    executable_name = executable_path;
    executable_path = "";
  }    
  getcwd(buffer,PATH_MAX); cwd = buffer;
  chdir(executable_path.c_str());
  getcwd(buffer,PATH_MAX); executable_path = buffer;  
  executable_path+='/';
  chdir(cwd.c_str());
}

/**
 * Rings the bell on the keyboard if possible. The specified volume is
 * relative to the base volume for the keyboard.
 */
void TOADBase::bell(int volume, int freq)
{
#ifdef __X11__
  assert(x11display);

  if (volume<-100) volume=-100; else if (volume>100) volume=100;
  if (freq>=0) {
    XKeyboardControl c;
    c.bell_pitch = freq;
    XChangeKeyboardControl(x11display, KBBellPitch, &c);
  }
  XBell(x11display, volume);
#endif
}

// open/close toad
//---------------------------------------------------------------------------
TOADBase::~TOADBase()
{
}

bool
TOADBase::initialize()
{
#ifdef __X11__
  bool x11sync = false;
  string x11displayname;

  for(int i=1; i<argc; i++) {
    if (strcmp(argv[i],"--sync")==0) {
      cout << "XSynchronize on\n";
      x11sync = true;
    } else
    if (strcmp(argv[i],"--display")==0) {
      i++;
      if (i>=argc) {
        cerr << "--display: missing display name" << endl;
      } else {
        x11displayname = argv[i];
      }
    }
  }

  bool i18n = true;

  if (setlocale(LC_ALL, "")==NULL) {
    if (setlocale(LC_ALL, "POSIX")==NULL) {
      if (setlocale(LC_ALL, "C")==NULL) {
        i18n = false;
      }
    }
  }

  if ((x11display = XOpenDisplay(x11displayname.c_str()))==NULL) {
    cerr << "Couldn't open display \"" << x11displayname << "\"\n";
    exit(1);
  }

  if (x11sync)
    XSynchronize(x11display, True);

  x11screen         = DefaultScreen(x11display);
  nClassContext     = XUniqueContext();
  xaWMSaveYourself  = XInternAtom(x11display, "WM_SAVE_YOURSELF", False);
  xaWMDeleteWindow  = XInternAtom(x11display, "WM_DELETE_WINDOW", False);
  xaWMProtocols     = XInternAtom(x11display, "WM_PROTOCOLS", False);
  xaWMMotifHints    = XInternAtom(x11display, "_MOTIF_WM_HINTS", False);
  
  xaUTF8_STRING     = XInternAtom(x11display, "UTF8_STRING", False);
  xaTEXT            = XInternAtom(x11display, "TEXT", False);

  if (i18n)
    initXInput();
#endif

  bSimulatedAutomaticGrab = false;
  wndTopPopup = NULL;

#ifdef __X11__
  initColor();

  initIO(ConnectionNumber(x11display));
#endif
  TFigure::initialize();
#ifdef __X11__
  initDnD();
#endif

  // parse arguments
  get_executable_path(*argv);

  // set up default font
  //---------------------
#ifdef __X11__
  x11gc = DefaultGC(x11display, DefaultScreen(x11display));
#endif
  default_font = new TFont("arial,helvetica,sans-serif:size=12");
  bold_font    = new TFont("arial,helvetica,sans-serif:size=12:bold");

  TBitmap::initialize();
  TPen::initialize();

  return true;
}

void
TOADBase::terminate()
{
#if 0
  if (TDialogEditor::getCtrlWindow()) {
    delete TDialogEditor::getCtrlWindow();
    TDialogEditor::setCtrlWindow(NULL);
  }
#endif

  default_font = 0;
  bold_font = 0;

  TFigure::terminate();
  TBitmap::terminate();
  TPen::terminate();


#ifdef __X11__
  closeXInput();

  //  close connection to the X11 server
  XCloseDisplay(x11display);
  x11display = 0;
#endif
  removeAllIntMsg();
}

/*---------------------------------------------------------------------------*
 | without category                                                          |
 *---------------------------------------------------------------------------*/

// there seems to be a problem with nested exceptions
static bool show_exception_message = false;

#ifdef __X11__
/**
 * Start TOAD message loop.
 *
 * All windows with parent NULL will be created also.
 */
int
TOADBase::mainLoop()
{
  ENTRYEXIT(__PRETTY_FUNCTION__);

  if (!TWindow::createParentless())
    return 0;

  bAppIsRunning = true;
  string msg;
  while(bAppIsRunning) {
    try {
      handleMessage();
    } catch(exception &e) {
      cerr << "caught exception in toad::TOADBase::runApp:" << endl
           << e.what() << endl;
#if 0
      if (show_exception_message) {
        cerr << "caught another exception: " << e.what() << endl;
      } else {
        show_exception_message = true;
        messageBox(NULL, 
                   "Encountered Exception", 
                   e.what(), 
                   TMessageBox::ICON_EXCLAMATION | 
                   TMessageBox::OK);
        show_exception_message = false;
      }
#endif
    }
  }


  // flush paint event buffer
  //---------------------------------
  while(TWindow::_havePaintEvents())
    TWindow::_dispatchPaintEvent();

  TWindow::destroyParentless();

  return nStatus;
}
#endif

/**
 * Stop mainLoop().
 *
 * \sa mainLoop
 */
void
TOADBase::postQuitMessage(int nExitCode)
{
  nStatus = nExitCode;
  bAppIsRunning = false;
}

/**
 * Send all stored events to the X Server.
 */
void
TOADBase::flush()
{
#ifdef __X11__
  XFlush(x11display);
#endif
}

/**
 * Return `true' when the message queue is not empty.
 */
#ifdef __X11__
bool
TOADBase::peekMessage()
{
  bool result;
  result = (XPending(x11display)!=0 || 
            TWindow::_havePaintEvents() || 
            countAllIntMsg()!=0 );
  return result;
}
#endif

/**
 * Wait when the message queue is empty until a new message arrives
 * and handle one message.
 * Returns `false' when `PostQuitMessage(..)' was been called.
 */
//---------------------------------------------------------------------------

#ifdef __X11__
static const char *x11eventname[34] = {
  "(reserved)",
  "(reserved)",
  "KeyPress",
  "KeyRelease",
  "ButtonPress",
  "ButtonRelease",
  "MotionNotify"
  "EnterNotify",
  "LeaveNotify",
  "FocusIn",
  "FocusOut",
  "KeymapNotify",
  "Expose",
  "GraphicsExpose",
  "NoExpose",
  "VisibilityNotify",
  "CreateNotify",
  "DestroyNotify",
  "UnmapNotify",
  "MapNotify",
  "MapRequest",
  "ReparentNotify",
  "ConfigureNotify",
  "ConfigureRequest",
  "GravityNotify",
  "ResizeRequest",
  "CirculateNotify",
  "CirculateRequest",
  "PropertyNotify",
  "SelectionClear",
  "SelectionRequest",
  "SelectionNotify",
  "ColormapNotify",
  "ClientMessage",
  "MappingNotify"
};

#define CONSIDER_GRAB(etype) \
  x=x11event.xbutton.x; \
  y=x11event.xbutton.y; \
  if (wndTopPopup && wndTopPopup != window) { \
    if (!window->isChildOf(wndTopPopup)) { \
      XTranslateCoordinates(x11display,window->x11window,wndTopPopup->x11window,x,y,&x,&y,&dummy_window); \
      window=wndTopPopup; \
    } \
  }

#define MODAL_BREAK \
  if( !modal_stack.empty() && \
      window != modal_stack.back()->wnd && \
      !window->isChildOf(modal_stack.back()->wnd) ) { \
      break; }

static Window checkMotionWindow;

static Bool
checkMotion(Display*, XEvent *event, XPointer) {
  return event->type == MotionNotify &&
         event->xany.window == checkMotionWindow;
}

bool
TOADBase::handleMessage()
{
  ENTRYEXIT("TOADBase::handleMessage()");

  bool dispatch_paint_event = false;
  static bool last_button_down_was_double = false;
  static TMouseEvent me;
  
#ifdef PERIODIC_PAINT
  static struct timeval first_paint_event_age;
#endif

  // wait for an event
  //-------------------
  while(true) {
    XFlush(x11display);

    // test if any TOAD internal message exist
    //-----------------------------------------
    if (countAllIntMsg()!=0 || !bAppIsRunning) {
      goto handle_event;                // => yes, handle event
    }

    // loop while event queue is empty
    //---------------------------------
    while (XPending(x11display)==0) {

      // don't wait when we have paint events
      //--------------------------------------
      if(TWindow::_havePaintEvents()) {
        XSync(x11display, False);       // wait for other events
        if (XPending(x11display)!=0)
          break;
        dispatch_paint_event = true;
        goto handle_event;
      }
      
      // wait for the events to come
      //--------------------------------------
      if (!bAppIsRunning) {
        cerr << "unexpected end of message loop [1]" << endl;
        return false;
      }
      select(); // won't return for `TIOObserver' and `TSimpleTimer' events

      if (countAllIntMsg()!=0 || !bAppIsRunning) {
        goto handle_event;
      }
    }

    // pop event from the event queue
    //--------------------------------
    XNextEvent(x11display, &x11event);
    
    if (XFilterEvent(&x11event, None))
      continue;

    // don't handle paint events, just collect them
    //-----------------------------------------------------------
    if(x11event.type == Expose || x11event.type == GraphicsExpose ) {
      TWindow *window;
      if(XFindContext(x11display, x11event.xany.window, nClassContext, (XPointer*)&window))
      {
        cerr << "toad: XFindContext failed for Expose event" << endl;
      } else {
        if (window && window->x11window) {
          window->handleX11Event();
#ifdef PERIODIC_PAINT         
          bool dispatch_all_paint_events = false;
          if (!TWindow::_havePaintEvents()) {
            gettimeofday(&first_paint_event_age, NULL);
          } else {
            struct timeval crnt_time;
            gettimeofday(&crnt_time, NULL);
            if ( (ulong)(crnt_time.tv_sec -first_paint_event_age.tv_sec)*1000000UL
                +(ulong)(crnt_time.tv_usec-first_paint_event_age.tv_usec) 
                >= 1000000UL / 12UL ) 
            {
              dispatch_all_paint_events = true;
            }
          }
#endif
          window->invalidateWindow(
            x11event.xexpose.x, x11event.xexpose.y,
            x11event.xexpose.width, x11event.xexpose.height,
            false
          );
#ifdef PERIODIC_PAINT         
          if (dispatch_all_paint_events) {
            while(TWindow::_havePaintEvents())
              TWindow::_dispatchPaintEvent();
          }
#endif
        }
      }
    } else {
      goto handle_event;
    }
  }

handle_event:

#if 0
  if (!bAppIsRunning) {
    return false;
  }
#endif

  // dispatch local messages
  //--------------------------
  if (countAllIntMsg()!=0)  {
    executeMessage();
    return bAppIsRunning;
  }

#ifdef PERIODIC_PAINT
  // flush all paint events 12 times per second
  //--------------------------------------------
  if (TWindow::_havePaintEvents()) {
    struct timeval crnt_time;
    gettimeofday(&crnt_time, NULL);
    if ( (ulong)(crnt_time.tv_sec -first_paint_event_age.tv_sec)*1000000UL
        +(ulong)(crnt_time.tv_usec-first_paint_event_age.tv_usec) 
        >= 1000000UL / 12UL ) {
      while(TWindow::_havePaintEvents())
        TWindow::_dispatchPaintEvent();
    }
  }
#endif

  // no other event, dispatch a single paint event
  //-----------------------------------------------
  if (dispatch_paint_event) {
    TWindow::_dispatchPaintEvent();
    return bAppIsRunning;
  }

  // first event dispatching switch for events not 
  // related to actual `TWindow' objects
  //-----------------------------------------------
  switch(x11event.type) {
    case ButtonRelease:
      if (DnDButtonRelease(x11event))
        return bAppIsRunning;
      break;
      
    case MotionNotify:
      if (DnDMotionNotify(x11event))
        return bAppIsRunning;
      break;
      
    case ClientMessage:
      if (DnDClientMessage(x11event))
        return bAppIsRunning;
      break;
      
    case SelectionClear:
//      cout << "SelectionClear" << endl;
      selection_kludge_owner = false;
      if (DnDSelectionClear(x11event));
        return bAppIsRunning;
      if (x11event.xselectionclear.selection==XA_PRIMARY) {
//cout << "XA_PRIMARY clear" << endl;
        selection_kludge_owner = false;
      }
      break;

    case SelectionNotify: {
//      cout << "SelectionNotify" << endl;
      if (DnDSelectionNotify(x11event))
        return bAppIsRunning;
// start of hack
#if 0
      cout << "got SelectionNotify" << endl;
      cout << "  requestor: " << x11event.xselection.requestor << endl;
      cout << "  selection: " << AtomName(x11event.xselection.selection) << endl;
      cout << "  target   : " << AtomName(x11event.xselection.target) << endl;
      cout << "  property : " << AtomName(x11event.xselection.property) << endl;
      cout << "  time     : " << x11event.xselection.time << endl;
#endif
      if (x11event.xselection.selection==XA_PRIMARY) {
        cout << "XA_PRIMARY notify" << endl;
        if ( x11event.xselection.target==XA_STRING)
        {
          selection_kludge_data = GetWindowProperty(
            x11event.xselection.requestor,
            XA_PRIMARY,
            XA_STRING
          );
        }
        selection_kludge_flag = false;
      }
// end of hack
    } break;

    case SelectionRequest:
//      cout << "SelectionRequest" << endl;
      if (DnDSelectionRequest(x11event))
        return bAppIsRunning;
#if 0
      cout << "got SelectionRequest" << endl;
      cout << "  requestor: " << x11event.xselection.requestor << endl;
      cout << "  selection: " << AtomName(x11event.xselectionrequest.selection) << endl;
      cout << "  target   : " << AtomName(x11event.xselectionrequest.target) << endl;
      cout << "  property : " << AtomName(x11event.xselectionrequest.property) << endl;
      cout << "  time     : " << x11event.xselectionrequest.time << endl;
#endif
      if (x11event.xselectionrequest.selection==XA_PRIMARY) {
//        cout << "XA_PRIMARY request" << endl;
        if (x11event.xselectionrequest.target==XA_STRING ||
            x11event.xselectionrequest.target==xaUTF8_STRING ||
            x11event.xselectionrequest.target==xaTEXT /* ||
            x11event.xselectionrequest.target==xa_COMPOUND_TEXT */
           ) 
        {
//cout << "change property..." << endl;
          XChangeProperty(x11display,
            x11event.xselectionrequest.requestor,
            x11event.xselectionrequest.property,
            x11event.xselectionrequest.target, 8,
            PropModeReplace,
            (ubyte*)selection_kludge_data.c_str(),
            selection_kludge_data.size());
//cout << "send selection notify..." << endl;        
          XEvent sevent;
          sevent.xselection.type      = SelectionNotify;
          sevent.xselection.serial    = 0;
          sevent.xselection.send_event= True;
          sevent.xselection.display   = x11display;
          sevent.xselection.requestor = x11event.xselectionrequest.requestor;
          sevent.xselection.selection = x11event.xselectionrequest.selection;
          sevent.xselection.target    = x11event.xselectionrequest.target;
          sevent.xselection.property  = x11event.xselectionrequest.property;
          sevent.xselection.time      = x11event.xselectionrequest.time;
          if (XSendEvent(x11display, 
                         x11event.xselectionrequest.requestor,
                         False,
                         NoEventMask,
                         &sevent)==0)
          {
             cerr << __FILE__ << ":" << __LINE__ << ": XSendEvent failed\n";
          }
        } else {
          cout << "  unsupported target " << XGetAtomName(x11display, x11event.xselectionrequest.target) << endl;
        }
      }
      break;
  }

  // handle messages for window 0
  //------------------------------
  if (x11event.xany.window==0) {
    if (x11event.type == ClientMessage) {
      cerr<< "toad: unknown client message: " 
          << AtomName(x11event.xclient.message_type) << endl;
    } else {
      cerr << "toad: event " << x11event.type << " for window 0" << endl;
    }   
    return bAppIsRunning;
  }

  // get TWindow object
  //---------------------
  TWindow *window;
  if(XFindContext(x11display, 
  x11event.xany.window, nClassContext, (XPointer*)&window)) {
    if (x11event.type==GraphicsExpose || x11event.type==NoExpose) {
      // `x11event.xany.window' could be a bitmap => ignore the failure
    } else {
      cerr<< "toad: fatal; XFindContext failed for window "
          << x11event.xany.window << ". Eventtype was "
          << x11eventname[x11event.type] << "." << endl;
    }
    return bAppIsRunning; // ignore event
  }

#if 0
  if (window) {
    cerr << "got " << x11eventname[x11event.type] << " for " << window->getTitle() << endl;
  }
#endif
  
  // window doesn't exist anymore
  if (!window || !window->x11window)
    return bAppIsRunning;

  // dispatch event
  //----------------

  window->handleX11Event();

  int x,y;
  Window dummy_window;
  switch(x11event.type) {
    case Expose:
    case GraphicsExpose:
      cerr << "toad: internal error; unhandled expose event\n";
      break;

    case NoExpose:
      //  printf("NoExpose for '%s'\n", window->Title());
      break;
  
    // ButtonPress
    //-------------
    case ButtonPress: {
      toolTipClose();
      x=x11event.xbutton.x;
      y=x11event.xbutton.y;

      // special for popup windows
      //---------------------------
      if (wndTopPopup && (wndTopPopup!=window || 
                          (x<0 || y<0 || 
                           x>=window->w || y>=window->h)) )
      {
        // printf("special for popup windows\n");
        if (!window->isChildOf(wndTopPopup) 
          || wndTopPopup==window) 
        {
          // printf("stop popup grab\n");
          // stop popup grab
          //-----------------

          // 1st: Destroy popup window
          wndTopPopup->closeRequest();
          wndTopPopup=NULL;

          // 2nd: Stop Grab
          window->ungrabMouse();

          // 3rd: Get window beneath the mouse pointer
          Window root,child,dest=None;
          int rx,ry,wx,wy;
          unsigned m;
          child=DefaultRootWindow(x11display);
          while(child) {
            dest = child;
            XQueryPointer(x11display, child, &root, &child, &rx,&ry, &wx,&wy, &m);
          }

          // 4th: Get position of mouse pointer within this window
          XTranslateCoordinates(x11display, x11event.xbutton.root, dest,
            x11event.xbutton.x_root, x11event.xbutton.y_root,
            &x,&y,
            &dummy_window);

          // 5th: Redo event
          TWindow* wnd;
          if (XFindContext(x11display, dest, nClassContext, (XPointer*)&wnd) ) {
            // window is not part of the application
            XEvent se;
            se.xbutton.type       = ButtonPress;
            se.xbutton.window     = dest;
            se.xbutton.root       = x11event.xbutton.root;
            se.xbutton.subwindow  = dest;
            se.xbutton.time       = CurrentTime;
            se.xbutton.x          = x;
            se.xbutton.y          = y;
            se.xbutton.x_root     = x11event.xbutton.x_root;
            se.xbutton.y_root     = x11event.xbutton.y_root;
            se.xbutton.state      = x11event.xbutton.state;
            se.xbutton.button     = x11event.xbutton.button;
            se.xbutton.same_screen= x11event.xbutton.same_screen;
            if (!XSendEvent(x11display, dest, False, ButtonPressMask, &se))
              printf(__FILE__": The conversion to the wire protocol format failed.\n");
            break;
          } else {
            // window is part of the application
            me.window = wnd;
            me.x = x;
            me.y = y;
            me._modifier = x11event.xbutton.state;
            wnd->mouseEnter(me);
            window = wnd;
            x11event.xany.send_event = true;
          }
        }
      } // end of special for popup windows

      // do automatic grab when event came from XSendEvent
      if (x11event.xany.send_event) {
        window->grabMouse();
        bSimulatedAutomaticGrab = true;
      }

      me.window = window;
      me.x = x-window->getOriginX();
      me.y = y-window->getOriginY();
      me._modifier = x11event.xbutton.state;
      switch(x11event.xbutton.button) {
        case Button1:
          me.type = TMouseEvent::LDOWN;
          break;
        case Button2:
          me.type = TMouseEvent::MDOWN;
          break;
        case Button3:
          me.type = TMouseEvent::RDOWN;
          break;
        case Button4:
          me.type = TMouseEvent::ROLL_UP;
          break;
        case Button5:
          me.type = TMouseEvent::ROLL_DOWN;
          break;
      }

      me.dblClick = false;
      if (me.type!=TMouseEvent::ROLL_UP &&
          me.type!=TMouseEvent::ROLL_DOWN)
      {
        static Time last_click_time = 0;
        static Window last_click_window = 0;
        if (x11event.xbutton.time-last_click_time<250 &&
            last_click_window == x11event.xbutton.window )
        {
          me.dblClick = true;
          last_button_down_was_double = true;
        } else {
          last_button_down_was_double = false;
        }
        last_click_time = x11event.xbutton.time;
        last_click_window = x11event.xbutton.window;
      } else {
        static unsigned counter=0;
        if (++counter<scrollwheel_slowdown)
          break;
        counter = 0;
      }

      TEventFilter *flt = toad::global_evt_filter;
      if (flt) {
        while(flt) {
          if (flt->mouseEvent(me))
            break;
          flt = flt->next;
        }
      }

      MODAL_BREAK;
      
      if (!flt)
        window->mouseEvent(me);
      } break;

    // ButtonRelease
    //---------------
    case ButtonRelease: {
      toolTipClose();
      if (bSimulatedAutomaticGrab)
        window->ungrabMouse();
  
      me.window = window;
      me.x = x11event.xbutton.x-window->getOriginX();
      me.y = x11event.xbutton.y-window->getOriginY();
      me._modifier = x11event.xbutton.state;
      switch(x11event.xbutton.button) {
        case Button1:
          me.type = TMouseEvent::LUP;
          break;
        case Button2:
          me.type = TMouseEvent::MUP;
          break;
        case Button3:
          me.type = TMouseEvent::RUP;
          break;
        case Button4:
          me.type = TMouseEvent::ROLL_UP_END;
          break;
        case Button5:
          me.type = TMouseEvent::ROLL_DOWN_END;
          break;
      }
      if (last_button_down_was_double) {
        me.dblClick = true;
        last_button_down_was_double = false;
      }
      TEventFilter *flt = toad::global_evt_filter;
      while(flt) {
        if (flt->mouseEvent(me))
          break;
        flt = flt->next;
      }
      CONSIDER_GRAB(xbutton);
      MODAL_BREAK
      if (!flt)
        window->mouseEvent(me);
      } break;

    // EnterNotify
    //-------------
    case EnterNotify:
#if 0
cout << "EnterNotify: " << window->getTitle() << endl;
cout << "  mode:";
switch(x11event.xcrossing.mode) {
  case NotifyNormal: cout << "NotifyNormal"; break;
  case NotifyGrab: cout << "NotifyGrab"; break;
  case NotifyUngrab: cout << "NotifyUngrab"; break;
}
cout << endl;
cout << "  detail:";
switch(x11event.xcrossing.detail) {
  case NotifyAncestor: cout << "NotifyAncestor"; break;
  case NotifyInferior: cout << "NotifyInferior"; break;
  case NotifyNonlinear: cout << "NotifyNonlinear"; break;
  case NotifyNonlinearVirtual: cout << "NotifyNonlinearVirtual"; break;
  case NotifyVirtual: cout << "NotifyVirtual"; break;
}
cout << endl;
#endif
      if (window->_bToolTipAvailable)
        toolTipOpen(window);
        // skip NotifyGrab and NotifyUngrab modes, otherwise one may receive
        // to many mouseEnter events
        if (x11event.xcrossing.mode==NotifyNormal) {
          me.window = window;
          me.x = x11event.xcrossing.x-window->getOriginX();
          me.y = x11event.xcrossing.y-window->getOriginY();
          me._modifier = x11event.xcrossing.state;
          me.type = TMouseEvent::ENTER;
          TEventFilter *flt = toad::global_evt_filter;
          while(flt) {
            if (flt->mouseEvent(me))
              break;
            flt = flt->next;
          }
          if (!flt)
            window->mouseEvent(me);
        }
      break;

    // LeaveNotify
    //-------------
    case LeaveNotify:
#if 0
cout << "LeaveNotify: " << window->Title() << endl;
cout << "  mode:";
switch(x11event.xcrossing.mode) {
  case NotifyNormal: cout << "NotifyNormal"; break;
  case NotifyGrab: cout << "NotifyGrab"; break;
  case NotifyUngrab: cout << "NotifyUngrab"; break;
}
cout << endl;
cout << "  detail:";
switch(x11event.xcrossing.detail) {
  case NotifyAncestor: cout << "NotifyAncestor"; break;
  case NotifyInferior: cout << "NotifyInferior"; break;
  case NotifyNonlinear: cout << "NotifyNonlinear"; break;
  case NotifyNonlinearVirtual: cout << "NotifyNonlinearVirtual"; break;
  case NotifyVirtual: cout << "NotifyVirtual"; break;
}
cout << endl;
#endif
      toolTipClose();
      if (x11event.xcrossing.mode==NotifyNormal) {
        me.window = window;
        me.x = x11event.xcrossing.x-window->getOriginX();
        me.y = x11event.xcrossing.y-window->getOriginY();
        me._modifier = x11event.xcrossing.state;
        me.type = TMouseEvent::LEAVE;
        TEventFilter *flt = toad::global_evt_filter;
        while(flt) {
          if (flt->mouseEvent(me))
            break;
          flt = flt->next;
        }
        if (!flt)
          window->mouseEvent(me);
      }
      break;

    // KeyPress
    //----------
    case KeyPress: { 
      toolTipClose();
      TKeyEvent ke(TKeyEvent::DOWN,
                   window,
                   XLookupKeysym(&x11event.xkey, 0), 
                   x11event.xkey.state);
      handleKeyEvent(ke);
    } break;

    // KeyRelease
    //------------
    case KeyRelease: {
      toolTipClose();
      TKeyEvent ke(TKeyEvent::UP,
                   window,
                   XLookupKeysym(&x11event.xkey, 0), 
                   x11event.xkey.state);
      handleKeyEvent(ke);
    } break;

    // MotionNotify
    //--------------
    case MotionNotify: {

      if (window->bCompressMotion) {
        checkMotionWindow = window->x11window;
        while(XCheckIfEvent(x11display, &x11event, checkMotion, 0))
          ;
      }

      CONSIDER_GRAB(xmotion)
#if 1
//      if (!window->isChildOf(TDialogEditor::getCtrlWindow()))
        MODAL_BREAK;
#endif
//cerr << "motion notify for window '" << window->getTitle() << "'\n";
      TMouseEvent me;
      me.type = TMouseEvent::MOVE;
      me.window = window;
#if 0
      me.x = x-window->getOriginX();
      me.y = y-window->getOriginY();
      me._modifier = x11event.xbutton.state;
#else
      me.x = x11event.xmotion.x-window->getOriginX();
      me.y = x11event.xmotion.y-window->getOriginY();
      me._modifier = x11event.xmotion.state;
#endif
      
      TEventFilter *flt = toad::global_evt_filter;
      while(flt) {
        if (flt->mouseEvent(me))
          break;
        flt = flt->next;
      }
      if (!flt)
        window->mouseEvent(me);
      } break;
      
    // ReparentNotify
    //----------------
    case ReparentNotify:
#if 0
        printf("ReparentNotify for '%s' to x=%4i, y=%4i, parent=%lx\n",
               window->getTitle().c_str(),
               x11event.xreparent.x, 
               x11event.xreparent.y, 
               (long) x11event.xreparent.parent);
#endif
      break;

    // ConfigureNotify
    //-----------------
    case ConfigureNotify:
      {
        /*
        printf("ConfigureNotify for '%s'\n"
             "     to x=%4i, y=%4i, w=%4i, h=%4i\n"
             "   from x=%4i, y=%4i, w=%4i, h=%4i\n"
        ,window->Title(),
        x11event.xconfigure.x, x11event.xconfigure.y,
        x11event.xconfigure.width, x11event.xconfigure.height,
        window->_x,window->_y,
        (int)window->width, (int)window->height);
        */
        // resize
        //--------
        if (  x11event.xconfigure.width  != window->w
           || x11event.xconfigure.height  != window->h )
        {
          // FVWM does not deliver the right position relative to the root
          // window after resizing the toplevel window. Instead it's (0,0).
          // The following code will solve the problem:
          // !!! ATTENTION: THE FOLLOWING CODE BADLY NEEDS TO BE OPTIMIZED !!!
          if (!window->getParent() && 
              !x11event.xconfigure.x && 
              !x11event.xconfigure.y )
          {
            Window root, wnd, *children;
            x=y=0;
            int xp,yp; unsigned n;
            wnd = x11event.xconfigure.window;
            do {
              XQueryTree(x11display, wnd, &root, &wnd, &children, &n);
              XFree(children);
              XGetGeometry(x11display, wnd, &root, &xp,&yp, &n,&n,&n,&n);
              x+=xp;
              y+=yp;
            }while(root!=wnd);
            x11event.xconfigure.x = x;
            x11event.xconfigure.y = y;
          }
          window->w = x11event.xconfigure.width;
          window->h = x11event.xconfigure.height;

//printf("resize: '%s' is set to (%i,%i)\n", window->Title(),(int)window->_w, (int)window->_h);
          window->doResize(); // event structure maybe changed afterwards!
        }

        // REPOSITION EVENT
        if (  x11event.xconfigure.x != window->x
           || x11event.xconfigure.y != window->y )
        {
/*          printf("Position of %s set to (%i,%i)\n",
            window->Title(),
            event.xconfigure.x,
            event.xconfigure.y); */
          window->x = x11event.xconfigure.x;
          window->y = x11event.xconfigure.y;
        }
      }
      break;

    //+---------------+
    //| ClientMessage |
    //+---------------+

    // here are parts of the drag'n drop stuff too 
    
    case ClientMessage:
      {
        if (x11event.xclient.message_type == xaWMProtocols) {
          Atom at=x11event.xclient.data.l[0];
          if (at==xaWMDeleteWindow)
            window->closeRequest();
          else if (at==xaWMSaveYourself)
            window->saveYourself();
        }
      }
      break;        

    case PropertyNotify:
      {
        #if 0
        // printf("property notify:\n");
        if (x11event.xproperty.atom) {
          const char *name;
          name = XGetAtomName(x11display, event.xproperty.atom);
          printf("property '%s' of window '%s' is changed\n",name,window->Title().c_str());
          XFree((void*)name);
        }
        #endif
      }
      break;

    case SelectionClear:
      break;

    case SelectionNotify:
      break;

    case SelectionRequest:
      break;

    case MappingNotify:
      XRefreshKeyboardMapping(&x11event.xmapping);
      break;

    case FocusIn:
      domainToWindow(window);
      break;
      
    case FocusOut:
      domainToWindow(NULL);
      break;

    case DestroyNotify:
      //fprintf(stderr, "toad: DestroyNotify\n");
      break;

    case UnmapNotify:
      //fprintf(stderr, "toad: UnmapNotify\n");
      break;

    case MapNotify:
      //fprintf(stderr, "toad: MapNotify\n");
      break;

    // default:
      // fprintf(stderr, "toad: (TOADBase.ToadDispatchEvent) unhandled event occured\n");
      // fprintf(stderr, "      type was %i.\n",x11event.type);
  }
  return bAppIsRunning;
}
#endif

/**
 * Create window 'wnd' as a modal window and return when 'wnd' is being
 * destroyed.
 */
void
TOADBase::doModalLoop(TWindow *wnd)
{
  TModalNode node;
  node.wnd = wnd;
  node.running = true;
  modal_stack.push_back(&node);

  wnd->createWindow();
  while(node.running && bAppIsRunning) {
#if 0
    try {
      handleMessage();
    } catch(exception &e) {
      if (show_exception_message) {
        cerr << "caught another exception: " << e.what() << endl;
      } else {
        show_exception_message = true;
        messageBox(NULL, 
                   "Encountered Exception", 
                   e.what(), 
                   TMessageBox::ICON_EXCLAMATION | 
                   TMessageBox::OK);
        show_exception_message = false;
      }
    }
#else
    try {
#ifdef __X11__
      handleMessage();
#endif
    } catch(exception &e) {
      wnd->destroyWindow();
      modal_stack.pop_back();
      throw e;
    }
#endif
  }
  wnd->destroyWindow();
  modal_stack.pop_back();
}

/** 
 * End modal message loop for the given window and all modal
 * message loops started from there.
 */
void
TOADBase::endModalLoop(TWindow *wnd)
{
  if (modal_stack.empty()) {
    return;
  }

  TModalStack::iterator p,e;
  p = modal_stack.begin();
  e = modal_stack.end();
  bool flag=true;
  while(p!=e) {
    if ( (*p)->wnd == wnd )
      flag = false;      
    (*p)->running = flag;
    p++;
  }  
}

void
TOADBase::endAllModalLoops()
{
  TModalStack::iterator p,e;
  p = modal_stack.begin();
  e = modal_stack.end();
  while(p!=e) {
    (*p)->running = false;
    p++;
  }
}

/**
 * Returns the width of the screen in pixels.
 */
int
TOADBase::getScreenWidth()
{
#ifdef __X11__
  return WidthOfScreen(DefaultScreenOfDisplay(x11display));
#endif

#ifdef __WIN32__
  return 800;
#endif
}

/**
 * Returns the height of the screen in pixels.
 */
int
TOADBase::getScreenHeight()
{
#ifdef __X11__
  return HeightOfScreen(DefaultScreenOfDisplay(x11display));
#endif

#ifdef __WIN32__
  return 600;
#endif
}

/**
 * Returns the position of the mouse pointer relative to the
 * root window. (Root window is the whole screen background.)
 */
void
TOADBase::getMousePos(int *x,int *y)
{
#ifdef __X11__
  Window w1,w2;
  int c1,c2;
  unsigned m;
  XQueryPointer(
    x11display,
    RootWindow(x11display, x11screen),
    &w1,&w2,
    x,y,
    &c1,&c2,
    &m);
#endif
}

/**
 * Sets the position of the mouse pointer relative to the root
 * window. (Root window is the whole screen background.)
 */
void
TOADBase::setMousePos(int x,int y)
{
#ifdef __X11__
  XWarpPointer(
    x11display,
    None,
    RootWindow(x11display, x11screen),
    0,0,0,0,
    x,y
  );
#endif
}

/**
 * PlaceWindow is intended to place top level windows, e.g. dialog windows.
 * 
 * There are several modes of operation:
 * 
 * \li PLACE_PARENT_CENTER
 *   Place window centered above the parent.
 * \li PLACE_PARENT_RANDOM
 *   Random position above the parent.
 * \li PLACE_SCREEN_CENTER
 *   Place window in the center of the screen.
 * \li PLACE_SCREEN_RANDOM
 *   Place window somewhere on the screen.
 * \li PLACE_MOUSE_POINTER
 *   Place window centered under the mouse pointer.
 * \li PLACE_CORNER_MOUSE_POINTER
 *   Place the windows upper left corner at the mouse pointer.
 * \li PLACE_PULLDOWN
 *   Place window beyond it's parent.
 * \li PLACE_TOOLTIP
 *   Place window near it's parent.
 *
 *  When you use the 'PARENT_' types and 'parent' is NULL, the function will
 *  try the applications first window and when there is none, the whole screen.
 */
//---------------------------------------------------------------------------
void
TOADBase::placeWindow(TWindow *window, EWindowPlacement how, TWindow *parent)
{
#ifdef __X11__
  TRectangle who;
  window->getShape(&who);

  int sw = TOADBase::getScreenWidth();
  int sh = TOADBase::getScreenHeight();

  if (parent && how!=PLACE_PULLDOWN && how!=PLACE_TOOLTIP ) {
    while(!parent->flagShell && parent->getParent())
      parent = parent->getParent();
  }

  TRectangle where;
  switch(how) {
    case PLACE_PARENT_RANDOM:
    case PLACE_PARENT_CENTER:
      if (parent) {
        parent->getShape(&where);
        break;
      }
    case PLACE_SCREEN_RANDOM:
    case PLACE_SCREEN_CENTER:
    case PLACE_MOUSE_POINTER:
    case PLACE_CORNER_MOUSE_POINTER:
      where.set(0, 0, sw, sh);
      break;
    case PLACE_PULLDOWN:
    case PLACE_TOOLTIP:
      if (parent==NULL) {
        cerr << __FUNCTION__ << ": parent is NULL, can't place window" << endl;
        return;
      }
      parent->getShape(&where);
  }

  int x,y;

  switch(how) {
    case PLACE_SCREEN_RANDOM:
    case PLACE_PARENT_RANDOM: {
      x = where.x + (where.w/2) - (who.w/2);
      y = where.y + (where.h/2) - (who.h/2);
      int xr = (int) (0.5*where.w*rand()/(RAND_MAX+1.0));
      xr-=(where.w/4);
      x+=xr;
      int yr = (int) (0.5*where.h*rand()/(RAND_MAX+1.0));
      yr-=(where.h/4);
      y+=yr;
      } break;
    case PLACE_SCREEN_CENTER:
    case PLACE_PARENT_CENTER:
      x = where.x + (where.w/2) - (who.w/2);
      y = where.y + (where.h/2) - (who.h/2);
      break;
    case PLACE_MOUSE_POINTER:
      getMousePos(&x,&y);
      x-=who.w/2;
      y-=who.h/2;
      break;
    case PLACE_CORNER_MOUSE_POINTER:
      getMousePos(&x,&y);
      break;
    case PLACE_PULLDOWN:
      parent->getRootPos(&where.x, &where.y);
      x = where.x;
      y = where.y + where.h - 1;
      if (y+who.h>sh)
        y = where.y - who.h + 1;
      break;
    case PLACE_TOOLTIP:
      parent->getRootPos(&where.x, &where.y);
      x = where.x + (where.w/4);
      y = where.y + where.h + 5;
      if (y+who.h>sh)
        y = where.y - who.h - 5;
      break;
  }
  
  // keep window inside the screen boundarys
  // `dist' is an additional distance to hide the additional size most
  // window managers add with their frames
  static const int dist = 32;
  if (x+who.w>sw-dist)
    x = sw-who.w-dist;
  if (y+who.h>sh-dist)
    y = sh-who.h-dist;
  if (x<dist)
    x=dist;
  if (y<dist)
    y=dist;
  window->setPosition(x,y);
#endif
}

/**
 * Returns the currently selected string.
 */
string TOADBase::getSelection()
{
#ifdef __X11__
  if (selection_kludge_owner)
    return selection_kludge_data;

  selection_kludge_data.erase();
  selection_kludge_flag = true;

  // XA_PRIMARY
  XConvertSelection(
    x11display,
    XA_PRIMARY,               // the primary selection
    XA_STRING,                // type
    XA_PRIMARY,               // destination property
    TWindow::getParentless(0)->x11window,// destination window
    CurrentTime);

  // timeout needed!
  time_t t = time(0);
  do {
    handleMessage();
  } while(selection_kludge_flag && time(0)-t<5);
  selection_kludge_flag = false;
  return selection_kludge_data;
#endif
}

// SetSelection
void
TOADBase::setSelection(const string &data)
{
  selection_kludge_owner = true;
  selection_kludge_data = data;
#ifdef __X11__
  cout << "XSetSelectionOwner " <<
  XSetSelectionOwner(
    x11display,
    XA_PRIMARY,
    TWindow::getParentless(0)->x11window,
    CurrentTime) << endl;
#endif
}

//---------------------------------------------------------------------------

#ifdef __X11__
// duplicated from dragndrop.cc for experiments:
//-----------------------------------------------
string 
GetWindowProperty(Window source, Atom property, Atom type)
{
  assert(source!=None);
  assert(property!=None);

  Atom out_type;
  int format;
  unsigned long position, received, remaining;
  position = 0L;
  unsigned char *buffer;
  string data;
  do {
    if (XGetWindowProperty(toad::x11display,
                           source,
                           property,
                           position, 1024L,
                           False,
                           type,
                           &out_type,
                           &format,
                           &received,
                           &remaining,
                           &buffer) == Success )
    {
      unsigned long n = received * (format/8);
      data.append((char*)buffer, n);
      position+=n/4L;
      XFree(buffer);
    } else {
      break;
    }
  }while(remaining!=0);
  return data;
}
#endif

// The following section is some rather ugly hack to test some new
// design decisions for TKeyEvent (namely TKeyEvent::setModifier
// which will ease the implementation of TMenuBar keyboard shortcuts):


string
TKeyEvent::str() const
{
#ifdef __X11__
  int count;
  static char buffer[KB_BUFFER_SIZE+1];

  if (xic_current) {
    Status status;
    KeySym key=NoSymbol;
#ifndef HAVE_LIBXUTF8
#ifdef X_HAVE_UTF8_STRING
    count = Xutf8LookupString(
#else
    count = XmbLookupString(
#endif
#else
    count = XUtf8LookupString(
#endif
      xic_current, 
      &x11event.xkey, 
      buffer, KB_BUFFER_SIZE,
      &key,
      &status );

    // the above functions do fail for KeyReleased event so we look
    // up the symbol again in case they failed:
    if (key==NoSymbol)
      key = XLookupKeysym(&x11event.xkey, 0);
    if (key!=this->_key) {
//      cerr << "XUtf8LookupString key differs from XLookupKeysym" << endl;
    }
    if (status==XLookupNone)
      return "";
    if (status==XBufferOverflow) {
      cerr << "TOAD keyboard buffer overflow" << endl;
      XmbResetIC(xic_current);
      return "";
    }
  } else {
    static XComposeStatus compose_status = {NULL, 0};
    KeySym key;
    count = XLookupString(&x11event.xkey,
                          buffer, KB_BUFFER_SIZE, 
                          &key, 
                          &compose_status);
    if (key!=this->_key) {
//      cerr << "XUtf8LookupString key differs from XLookupKeysym" << endl;
    }
  }
  buffer[count]=0;        // add zero terminator to string
  return buffer;
#endif

#ifdef __COCOA__
  NSString *ns = [nsevent characters];
  return [ns UTF8String];
#endif
}
