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

/* 
The contents of the file 'xc/lib/Xmu/ClientWin.c' are within this source
file to avoid inclusion of libXmu, libSM, libICE and libXt.

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/


/**
 * @defgroup dnd Drag'n Drop
 *
 * Functions to implement the Xdnd3 Drag'n Drop Protocol.
 */

/*
  serious:
    - "XFindContext failed for window42" message
    - losing the cursor shape (netedit, during first dnd operation when
      moving over the drop target)
      
    (its the same problem, maybe I should take a look at the time stamps
    to fix it!)
*/

#include <toad/os.hh>

// #define WHERE { cout << __FILE__ << ":" << __LINE__ << endl; }
#define WHERE

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif

#define _TOAD_PRIVATE

#include <toad/toad.hh>
#include <toad/dragndrop.hh>
#include <toad/bitmap.hh>

#include <map>
#include <vector>

// test:
#include <toad/dnd/dropobject.hh>

namespace toad {

#define XDND_VERSION 3;
#define VERBOSE 0

#define KEEP_ACTION 0

// #define throw(txt) { fprintf(stderr, "%s\n", txt); return; }

#define DBM(CMD)

#ifdef __X11__
#define x11display toad::x11display

static void PrepareXdndClientMessage(XEvent&, Window, Atom);
static string GetWindowProperty(Window source, Atom property, Atom type);
static Window FindWindow(Window xw, int x, int y, int *tx, int *ty);
static void HandlePosition(Window,int,int,TDnDObject*,unsigned);
static void HandleDrop(TDnDObject *drop);
static unsigned Atom2Action(Atom atom);
static Atom Action2Atom(unsigned action);
string AtomName(Atom atom);

// X11 Atoms
//---------------------------------------------------------------------------
static Atom xaXdndAware;
static Atom xaXdndEnter;
static Atom xaXdndTypeList;
static Atom xaXdndPosition;
static Atom xaXdndStatus;
static Atom xaXdndLeave;
static Atom xaXdndDrop;
static Atom xaXdndFinished;
static Atom xaXdndActionCopy;
static Atom xaXdndActionMove;
static Atom xaXdndActionLink;
static Atom xaXdndActionAsk;
static Atom xaXdndActionPrivate;
static Atom xaXdndSelection;

//---------------------------------------------------------------------------
// Initiating Drop
//---------------------------------------------------------------------------

/** 
 * This event structure is used to send `XdndEnter' messages and is
 * initialized in `DnDInit' and `StartDrag' and is used in DnDMotionNotify.
 */
static XEvent x11_message_enter;

/**
 * This window ID is used as the drag source window during the drag.
 * This could be any window so it's currently the X11 drag source window.
 */
static Window x11_drag_source_window;

/**
 * The root window we've grab the mouse events from when doing the drag.
 */
static Window x11_root_window;

/**
 * The window the mouse pointer is in during dragging.
 */
static Window x11_current_window;
#endif

/**
 * `true' when `x11_current_window' is willing to accept Xdnd messages and
 * we've send an XdndEnter message. Oh, and `inside_local_window' is NULL.
 */
static bool inside_extern_window = false;

/**
 * Same as `x11_current_window' but the corresponding TWindow inside this
 * application when available.
 */
static bool inside_local_window;

/**
 * The drag object given to the `StartDrag' call. It's NULL when we're not
 * dragging.
 */
static PDnDObject drag_object;

#ifdef __X11__
/**
 * The Xdnd protocol version of the last window we've send a XdndEnter
 * message;
 */
static int drag_xdnd_version;

/**
 * This atom describes the action for the current drag.
 */
static Atom drag_action;
static unsigned drag_eaction;

/**
 * A XdndPosition message is only to be send after we've received an
 * XdndStatus or send an XdndEnter message.
 */
static bool ready_to_send_position = false;

/**
 * In case we've received some motion events before a XdndStatus reply,
 * `have_motion_event' will be `true' and some information about the
 * last motion event is available in `last_motion_*'.
 */
static bool have_last_motion_event = false;
static int last_motion_x;
static int last_motion_y;
static Time last_motion_time;

/**
 * The type of action the target is willing to perform.
 * This value is set when receiving XdndStatus messages;
 */
static Atom target_action;

static void SendXdndPosition(Window, int, int, Atom, Time);
static void ReceivedXdndStatus(XEvent &event);
static void SendXdndLeave();
static void SendXdndDrop(Time time);

static Cursor dnd_cursor[6] = { None, };

static void UpdateCursor();
static Cursor current_cursor;

static void BuildCursor();
#endif

// defined for drop
static void SetDropSite(TDropSite*);
static TDropSite *dropsite = NULL;

/** 
 * The Motif style guide defines that:
 * <UL>
 *   <LI>CTRL selects a copy operation
 *   <LI>SHIFT selects a move operation
 *   <LI>CTRL+SHIFT select a link operation
 * </UL>
 * TOADs behaviour is a little bit different:
 * <UL>
 *   <LI>The middle mouse button is used to initiate a DND operation
 *   <LI>SHIFT selects a move operation
 *   <LI>CTRL selects a link operation
 * </UL>
 */
void
TOADBase::startDrag(TDnDObject *source, unsigned modifier)
{
#ifdef __X11__
  PDnDObject dummy = source;

  BuildCursor();

  assert(source!=NULL);
  assert(source->typelist.size()>0);

  if (drag_xdnd_version<3 && drag_object) {
WHERE
    drag_object = NULL;
  }
  
  if (drag_object) {
    cerr << "toad: error, StartDrag during active drag, canceling old drag" << endl;
WHERE
    drag_object = NULL;
  }

  x11_root_window = DefaultRootWindow(x11display);
dropsite=NULL;

  // set action from `modifier'
  //---------------------------------------------------
  drag_eaction = ACTION_COPY;
  if (modifier & MK_SHIFT)
    drag_eaction = ACTION_MOVE;
  if (modifier & MK_CONTROL)
    drag_eaction = ACTION_LINK;

  // check how many types are available for this action
  //---------------------------------------------------
  unsigned ntypes=0;  
  TDnDTypeList::iterator p, e;
  p = source->typelist.begin();
  e = source->typelist.end();
  while(p!=e) {
    if ( (*p)->actions & drag_eaction )
      ntypes++;
    p++;
  }
  
  if (ntypes==0) {
    return;
  }
  
  drag_action = Action2Atom(drag_eaction);
WHERE
  drag_object = source;
  current_cursor = None;
  inside_extern_window = false;
  inside_local_window = false;
DBM(cout << __FILE__ << ":" << __LINE__ << endl;)
  UpdateCursor();
  
  if (XGrabPointer(x11display,
                   x11_root_window,
                   False,
                   ButtonReleaseMask | PointerMotionMask,
                   GrabModeAsync,
                   GrabModeAsync,
                   None,
                   current_cursor,
                   CurrentTime ) != GrabSuccess )
  {
    throw runtime_error("XGrabPointer failed\n");
  }

  x11_drag_source_window = 0;
  for(unsigned i=0; i<TWindow::getParentlessCount(); ++i) {
    x11_drag_source_window = TWindow::getParentless(i)->x11window;
    if (x11_drag_source_window!=0)
      break;
  }
  if (x11_drag_source_window == 0) {
    cerr << "toad: couldn't find an X11 window to use as my drag source => stopped" << endl;
    return;
  }

  x11_message_enter.xclient.data.l[1] = 0;
  x11_message_enter.xclient.data.l[2] = 0;
  x11_message_enter.xclient.data.l[3] = 0;
  x11_message_enter.xclient.data.l[4] = 0;
  unsigned n;

  x11_message_enter.xclient.data.l[0] = x11_drag_source_window;
  x11_message_enter.xclient.data.l[1] |= 0x03000000;

  n = source->typelist.size();
  if (ntypes>3) {
    Atom atoms[ntypes];
    p = drag_object->typelist.begin();
    unsigned i = 0;
    while(p!=e) {
      if ( (*p)->actions & drag_eaction ) {
        atoms[i] = XInternAtom(x11display, (*p)->mime.c_str(), False);
        i++;
      }
      p++;
    }
    XChangeProperty(x11display, 
                    x11_drag_source_window,
                    xaXdndTypeList, XA_ATOM, 32,
                    PropModeReplace,
                    (ubyte*)&atoms, ntypes);
    x11_message_enter.xclient.data.l[1] |= 1; // more than 3 types
    n = 3;
  }
  p = drag_object->typelist.begin();
  for(unsigned i=0; i<ntypes; i++) {
    while( !((*p)->actions & drag_eaction) )
      p++;
    x11_message_enter.xclient.data.l[i+2] 
      = XInternAtom(x11display, (*p)->mime.c_str(), False);
    p++;
  }

#if VERBOSE
  cout << "makeing window " << x11_drag_source_window << " owner of selection XdndSelection" << endl;
#endif
  XSetSelectionOwner(x11display, xaXdndSelection, x11_drag_source_window, CurrentTime);

  x11_current_window = None;
  have_last_motion_event = false;
  target_action = None;
  
  // try a handle position right here!
#endif
}

/* 
 
Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

#ifdef __X11__
/*
 * Prototypes
 */
static Window TryChildren(Display*, Window, Atom);

/* Find a window with WM_STATE, else return win itself, as per ICCCM */

static Window
XmuClientWindow(Display *dpy, Window win)
{
  Atom WM_STATE;
  Atom type = None;
  int format;
  unsigned long nitems, after;
  unsigned char *data = NULL;
  Window inf;

  WM_STATE = XInternAtom(dpy, "WM_STATE", True);
  if (!WM_STATE)
    return win;
  XGetWindowProperty(dpy, win, WM_STATE, 0, 0, False, AnyPropertyType,
                     &type, &format, &nitems, &after, &data);
  if (data)
    XFree(data);
  if (type)
    return win;
  inf = TryChildren(dpy, win, WM_STATE);
  if (!inf)
    inf = win;
  return inf;
}

static Window
TryChildren(Display *dpy, Window win, Atom WM_STATE)
{
  Window root, parent;
  Window *children;
  unsigned int nchildren;
  unsigned int i;
  Atom type = None;
  int format;
  unsigned long nitems, after;
  unsigned char *data;
  Window inf = 0;

  if (!XQueryTree(dpy, win, &root, &parent, &children, &nchildren))
    return 0;
  for (i = 0; !inf && (i < nchildren); i++) {
  data = NULL;
  XGetWindowProperty(dpy, children[i], WM_STATE, 0, 0, False,
         AnyPropertyType, &type, &format, &nitems,
         &after, &data);
  if (data)
    XFree(data);
  if (type)
    inf = children[i];
  }
  for (i = 0; !inf && (i < nchildren); i++)
  inf = TryChildren(dpy, children[i], WM_STATE);
  if (children)
    XFree(children);
  return inf;
}

/**
 * @ingroup dnd
 */
bool 
TOADBase::DnDMotionNotify(XEvent &event)
{
  if (event.xany.window != x11_root_window ||
      drag_object==NULL) {
#if VERBOSE
    if (event.xany.window != x11_root_window)
      cout << "  MotionNotify isn't for root window" << endl;
    else if (drag_object==NULL) {
      cout << "  MotionNotify but drag object is NULL" << endl;
    }
#endif
    return false;
  }

  if (event.xmotion.subwindow==None) {
    if (x11_current_window!=None) {
      if (inside_extern_window) {
        SendXdndLeave();
      }
      x11_current_window = None;
#if VERBOSE
      cout << "current window = None" << endl;
#endif
      inside_extern_window = false;
      inside_local_window = false;
      target_action = None;
DBM(cout << __FILE__ << ":" << __LINE__ << endl;)
      UpdateCursor();
    }
  } else {
    Window w = toad::XmuClientWindow(x11display, event.xmotion.subwindow);
    if (x11_current_window != w) {
      if (inside_extern_window) {
        SendXdndLeave();
      }
      inside_extern_window = false;
      target_action = None;
      inside_local_window = false;
      x11_current_window = w;
#if VERBOSE
      cout << "current window = " << w << endl;
#endif
      TWindow *tw;
      if(!XFindContext(x11display, x11_current_window, 
                       nClassContext, 
                       (XPointer*)&tw))
      {
        inside_local_window = true;
      } else {
        string data = GetWindowProperty(x11_current_window, 
                                        xaXdndAware, XA_ATOM);
        if (!data.empty()) {
#if VERBOSE
          cout << "Xdnd version on target: " << (int)data[0] << endl;
#endif
          drag_xdnd_version = (int)data[0];
          x11_message_enter.xclient.window    = x11_current_window;
          x11_message_enter.xclient.display   = x11display;
          x11_message_enter.xclient.data.l[0] = x11_drag_source_window;
          if (XSendEvent(x11display, x11_current_window,
                         False, 
                         NoEventMask, 
                         &x11_message_enter)!=0) 
          {
            inside_extern_window = true;
            ready_to_send_position = true;
          }
        }
      }
DBM(cout << __FILE__ << ":" << __LINE__ << endl;)
      UpdateCursor();
    }
  }

  if (inside_extern_window) {
    if(ready_to_send_position) {
      SendXdndPosition(x11_current_window,
                       event.xmotion.x_root, event.xmotion.y_root,
                       drag_action,
                       event.xmotion.time);
      ready_to_send_position = false;
      have_last_motion_event = false;
    } else {
      have_last_motion_event = true;
      last_motion_x = event.xmotion.x_root;
      last_motion_y = event.xmotion.y_root;
      last_motion_time = event.xmotion.time;
    }
    SetDropSite(NULL);
  } else if (inside_local_window) {
    // now do a direct call into the drop code and it's
    // XTranslateCoordinates stuff
    HandlePosition(x11_current_window,
                   event.xmotion.x_root,
                   event.xmotion.y_root,
                   drag_object,
                   drag_eaction);
    target_action = Action2Atom(drag_object->action);
#if !(KEEP_ACTION)
    target_action = drag_eaction==drag_object->action ? drag_action : None;
#endif
DBM(cout << __FILE__ << ":" << __LINE__ << endl;)
    UpdateCursor();
  } else {
    SetDropSite(NULL);
  }

  return true;
}

void ReceivedXdndStatus(XEvent &event)
{
  if (!drag_object) {
    // cerr << "received unexpected XdndStatus message, ignoring" << endl;
    return;
  }

#if VERBOSE
  cout << "  from window      : " << event.xclient.data.l[0] << endl;
  cout << "  accept           : " << (event.xclient.data.l[1]&1 ? "yes" : "no") << endl;
  cout << "  events in rect   : " << (event.xclient.data.l[1]&2 ? "yes" : "no") << endl;
  cout << "  rect             : " << (event.xclient.data.l[2]>>16) << ", "
                                  << (event.xclient.data.l[2] & 0xFFFF) << ", "
                                  << (event.xclient.data.l[3]>>16) << ", "
                                  << (event.xclient.data.l[3] & 0xFFFF) << endl;
  cout << "  action           : " << AtomName(event.xclient.data.l[4]) << endl;
#endif

#if KEEP_ACTION
  if (event.xclient.data.l[1]&1)
    target_action = event.xclient.data.l[4];
  else
    target_action = None;
#else
  if (event.xclient.data.l[1]&1 &&
      static_cast<Atom>(event.xclient.data.l[4]) == drag_action)
    target_action = drag_action;
  else
    target_action = None;
#endif
    
  // need to handle rectangle here...
    
DBM(cout << __FILE__ << ":" << __LINE__ << endl;)
  UpdateCursor();
  if(have_last_motion_event) {
    SendXdndPosition(x11_current_window,
                     last_motion_x, last_motion_y,
                     drag_action,
                     last_motion_time);
    have_last_motion_event = false;
    ready_to_send_position = false;
  } else {
    ready_to_send_position = true;
  }
}

static void
UpdateCursor()
{
  // target_action: the action the target wants
  // drag_action  : the action we want
  Atom action = drag_action;
  int cs=0;
  if ((inside_extern_window||inside_local_window) && target_action!=None) {
    action = target_action;
  } else {
    cs=1;
  }

  if (action==xaXdndActionMove) {
    cs += 2;
  } else if (action==xaXdndActionLink) {
    cs += 4;
  }
DBM(cout << "selecting cursor " << cs << endl;)
  Cursor nc = dnd_cursor[cs];
  if (nc != current_cursor) {
    current_cursor = nc;
    XChangeActivePointerGrab(x11display,
                             ButtonReleaseMask | PointerMotionMask,
                             current_cursor,
                             CurrentTime);
  } 
}

bool
TOADBase::DnDButtonRelease(XEvent &event)
{
  if (event.xany.window != x11_root_window ||
      drag_object==NULL)
    return false;

//cout << "ButtonRelease during drop" << endl;

  XUngrabPointer(x11display, CurrentTime);

  if (inside_extern_window) {
    if (target_action==None) {
      SendXdndLeave();
WHERE
      drag_object = NULL;
    } else {
      SendXdndDrop(event.xbutton.time);
    }
    inside_extern_window = false;
  } else {
    if (inside_local_window &&
        target_action == drag_action)
    {
      HandleDrop(drag_object);
    } else {
WHERE
      drag_object = NULL;
    }
  }

  return true;
}

bool
TOADBase::DnDSelectionClear(XEvent &event)
{
  if (event.xselectionclear.selection!=xaXdndSelection)
    return false;
#if VERBOSE
  cout << "got SelectionClear for XdndSelection" << endl;
#endif
  if (drag_object) {
    if (drag_xdnd_version>=3)
      cerr << "toad: error; lost XdndSelection while dragging, aborting" << endl;
    SendXdndLeave();
WHERE
    drag_object = NULL;
    XUngrabPointer(x11display, CurrentTime);
  }
  return true;
}

static void
SendXdndPosition(Window wdest, int x, int y, Atom action, Time time)
{
  XEvent event;
  PrepareXdndClientMessage(event, x11_current_window, xaXdndPosition);
  event.xclient.data.l[1]     = 0; /* flags */
  event.xclient.data.l[2]     = (x << 16) | y;
  event.xclient.data.l[3]     = time;
  event.xclient.data.l[4]     = action;
#if VERBOSE
  cout << "sending XdndPosition" << endl;
  cout << "  from window      : " << event.xclient.data.l[0] << endl;
  cout << "  flags            : " << event.xclient.data.l[1] << endl;
  cout << "  position         : " << (event.xclient.data.l[2]>>16) << ", "
                                  << (event.xclient.data.l[2] & 0xFFFF) << endl;
  cout << "  time             : " << event.xclient.data.l[3] << endl;
  cout << "  action           : " << AtomName(event.xclient.data.l[4]) << endl;
#endif
  if (XSendEvent(x11display, wdest, False, NoEventMask, &event)==0) {
    cerr << __FILE__ << ":" << __LINE__ << ": XSendEvent failed\n";
  }
}

static void
SendXdndLeave()
{
#if VERBOSE
  cout << "sending XdndLeave" << endl;
#endif
  XEvent event;
  PrepareXdndClientMessage(event, x11_current_window, xaXdndLeave);
  if (XSendEvent(x11display, 
                 x11_current_window, 
                 False, 
                 NoEventMask, &event)==0)
  {
    cerr << __FILE__ << ":" << __LINE__ << ": XSendEvent failed\n";
  }
}

/**
 * Inform target that it can retreive the data.
 */
static void
SendXdndDrop(Time time)
{
#if VERBOSE
  cout << "sending XdndDrop..." << endl;
  cout << "  to window  : " << x11_current_window << endl;
  cout << "  from window: " << x11_drag_source_window << endl;
  cout << "  time       : " << time << endl;
#endif
  XEvent event;
  PrepareXdndClientMessage(event, x11_current_window, xaXdndDrop);
  event.xclient.data.l[2] = time;
  if (XSendEvent(x11display, x11_current_window, False, NoEventMask, &event)==0) {
    cerr << __FILE__ << ":" << __LINE__ << ": XSendEvent failed\n";
  }
}

/**
 * The target wants the data.
 */
bool
TOADBase::DnDSelectionRequest(XEvent &event)
{
  if (event.xselectionrequest.selection!=xaXdndSelection)
    return false;
  if (!drag_object) {
    cerr << "toad: warning, received unexpected SelectionRequest for XdndSelection, ignoring" << endl;
    return true;
  }
#if VERBOSE
  cout << "selection request" << endl;
  cout << "  owner window    : " << event.xselectionrequest.owner << endl;
  cout << "  requestor window: " << event.xselectionrequest.requestor << endl;
  cout << "  selection       : " << AtomName(event.xselectionrequest.selection) << endl;
  cout << "  target          : " << AtomName(event.xselectionrequest.target) << endl;
  cout << "  property        : " << AtomName(event.xselectionrequest.property) << endl;
  cout << "  time            : " << event.xselectionrequest.time << endl;
#endif

  string mime = AtomName(event.xselectionrequest.target);
  TDnDTypeList::iterator p, e;
  p = drag_object->typelist.begin();
  e = drag_object->typelist.end();
  drag_object->type = NULL;
  while(p!=e) {
    if (mime==(*p)->mime) {
      drag_object->type = *p;
      break;
    }
    p++;
  }

  if (drag_object->type) {
#if VERBOSE
    cout << "setting property on requestor window" << endl;
#endif
    drag_object->flatdata.erase();
    drag_object->flatten();
#warning "must also retrieve data format from drag object, currently fixed to 16"    
    XChangeProperty(x11display,
                    event.xselectionrequest.requestor,
                    event.xselectionrequest.property,
                    event.xselectionrequest.target, 16,
                    PropModeReplace,
                    (ubyte*)drag_object->flatdata.c_str(),
                    drag_object->flatdata.size()/2);
  } else {
    cerr << "toad: warning, failed to convert property" << endl;
  }
  XEvent sevent;
  sevent.xselection.type      = SelectionNotify;
  sevent.xselection.serial    = 0;
  sevent.xselection.send_event= True;
  sevent.xselection.display   = x11display;
  sevent.xselection.requestor = event.xselectionrequest.requestor;
  sevent.xselection.selection = event.xselectionrequest.selection;
  sevent.xselection.target    = 
    drag_object->type ? event.xselectionrequest.target : None;
  sevent.xselection.property  = event.xselectionrequest.property;
  sevent.xselection.time      = event.xselectionrequest.time;
#if VERBOSE
  cout << "sending SelectionNotify" << endl;
  cout << "  to window: " << event.xselectionrequest.requestor/*x11_current_window*/ << endl;
#endif
  if (XSendEvent(x11display, event.xselectionrequest.requestor/*x11_current_window*/,
      False, NoEventMask, &sevent)==0)
  {
    cerr << __FILE__ << ":" << __LINE__ << ": XSendEvent failed\n";
  }

  return true;
}

static void
ReceivedXdndFinished(XEvent &event)
{
  if (!drag_object) {
    cerr << "toad: received unexpected XdndFinished message, ignoring" << endl;
    return;
  }
WHERE
  drag_object = NULL;

#if VERBOSE
  cout << "  from window      : " << event.xclient.data.l[0] << endl;
  cout << "  accepted         : " << (event.xclient.data.l[1]&1 ? "yes" : "no") << endl;
  cout << "  action           : " <<  AtomName(event.xclient.data.l[2]) << endl;
#endif
  
}

#endif

//---------------------------------------------------------------------------
// Receiving Drop
//---------------------------------------------------------------------------

// TDnDType
//---------------------------------------------------------------------------
TDnDType::TDnDType(const string &m, unsigned actions_in)
{
  wanted = false;
  actions = actions_in;
  mime=m;
  size_t n1, n2;
  n1 = mime.find("/");
  if (n1==string::npos) {
    major="x-not-mime";
    n1 = 0;
  } else {
    major = mime.substr(0,n1);
    n1++;
  }
  n2 = mime.find_first_of(" ;", n1);
  minor = mime.substr(n1,n2);
#if VERBOSE
  cout << "  new dnd type: `" << major << "/" << minor << "'" << endl;
#endif
}

// TWindowDropSite
//---------------------------------------------------------------------------
struct TWindowDropSite
{
  void Add(TDropSite *ds) {
    storage.push_back(ds);
  }
  TWindow *parent;
  TDropSite* find(int x, int y);
  typedef std::vector<TDropSite*> TStorage;
  TStorage storage;
};

TDropSite* TWindowDropSite::find(int x, int y) {
  if (x<0 || y<0 || x>parent->getWidth() || y>parent->getHeight())
    return NULL;
#if VERBOSE
  cout << "searching drop site in `" << parent->getTitle() << "' at " << x << ", " << y << endl;
#endif
  TStorage::iterator p, e;
  p = storage.begin();
  e = storage.end();
  while(p!=e) {
    if ( (*p)->getShape().isInside(x,y) ) {
#if VERBOSE
      cout << "  found `" << (*p)->getParent()->getTitle() << "'" << endl;
#endif
      return *p;
    }
    p++;
  }
#if VERBOSE
  cout << "  no window dropsite there" << endl;
#endif
  return NULL;
}

typedef map<TWindow*, TWindowDropSite*> TWindowDropSiteMap;
static TWindowDropSiteMap dropsitemap;

TDropSite::TDropSite(TWindow *p, TRectangle const &r)
{
  parent = p;
  rect = r;
  use_parent = false;
  init();
}

TDropSite::TDropSite(TWindow *p)
{
  parent = p;
  use_parent = true;
  init();
}

void TDropSite::init()
{
  TWindowDropSiteMap::iterator p = dropsitemap.find(parent);
  if (p==dropsitemap.end()) {
    TWindowDropSite *wds = new TWindowDropSite;
    dropsitemap[parent] = wds;
    wds->parent = parent;
    wds->Add(this);
  } else {
    (*p).second->Add(this);
  }
}

TDropSite::~TDropSite()
{
}

const TRectangle&
TDropSite::getShape()
{
  if (use_parent)
    rect.set(0,0,parent->getWidth(), parent->getHeight());
  return rect;
}

void
TDropSite::setShape(int x, int y, int w, int h)
{
  use_parent = false;
  rect.set(x, y, w, h);
}

void
TDropSite::setShape(const TRectangle &r)
{
  use_parent = false;
  rect = r;
}

void TDropSite::leave()
{
  cout << __PRETTY_FUNCTION__ << endl;
}

/**
 * This virtual method is called to draw into the drop sites parent
 * window to indicate that the drop site is willing to accept a drop.
 *
 * The default implementation is to draw a black and white dotted
 * two pixel wide frame.
 */
void
TDropSite::paint()
{
  TPen pen(getParent());
  pen.identity();
  TBitmap bitmap(2,2, TBITMAP_SERVER);
  TPen bpen(&bitmap);
  bpen.setColor(255,255,255);   
  bpen.fillRectanglePC(0,0,2,2);
  bpen.setColor(0,0,0);  
  bpen.drawLine(0,0,1,1);
  pen.setBitmap(&bitmap);
  TRectangle r(getShape());
  pen.drawRectanglePC(r);
  r.x++;
  r.y++; 
  r.w-=2;
  r.h-=2;
  pen.drawRectanglePC(r);
}

//---------------------------------------------------------------------------
// drop handling (extern)
//---------------------------------------------------------------------------

#ifdef __X11__

/**
 * Window that wants to drop us the data. It is not `None' between an
 * XdndEnter and XdndLeave message. When set, all other Xdnd messages
 * are ignored.
 */
static Window x11_drop_source_window = None;

/**
 * Same as `x11_drop_source_window' but this time the target.
 */
static Window x11_drop_target_window = None;

#endif

/**
 * The drop request object we give to the `TDropSite::dropRequest' call.
 */
static TDnDObject drop_request;

// static TDropSite *dropsite;

void
SetDropSite(TDropSite *ds)
{
  if (ds==dropsite)
    return;
  if (dropsite) {
    dropsite->getParent()->invalidateWindow(dropsite->getShape());
    dropsite->getParent()->paintNow();
  }
  dropsite = ds;
  if (dropsite) {
    dropsite->paint();
  }
}

static 
void ClearDropSite()
{
  if (dropsite) {
    dropsite->getParent()->invalidateWindow();
    dropsite->getParent()->paintNow();
  }
}

#ifdef __X11__

/**
 * Get the next requested type from `drop_request' and call XConvertSelection
 * to get it or send an XdndFinished.
 */
static void ConvertNextTypeOrFinish(Time);

/**
 * The type requested by `ConvertNextType'.
 */
static TDnDType *requested_type;
static Time request_time;

static bool dummy;

static int
dummyhandler(Display *display, XErrorEvent *ev)
{
  dummy = true;
  char buffer[2048];
  
  XGetErrorText(display, ev->error_code, buffer, sizeof(buffer));
  cerr << buffer << endl;
}

void ReceivedXdndEnter(XEvent &event)
{
#if VERBOSE
  cout << "  from window      : " << event.xclient.data.l[0] << endl;
  cout << "  more than 3 types: " << (event.xclient.data.l[1]&1 ? "yes" : "no") << endl;
  cout << "  type 1           : " << AtomName(event.xclient.data.l[2]) << endl;
  cout << "  type 2           : " << AtomName(event.xclient.data.l[3]) << endl;
  cout << "  type 3           : " << AtomName(event.xclient.data.l[4]) << endl;
#endif

  if (x11_drop_source_window) {
    cerr << "toad: received unexpected XdndEnter, ignoring" << endl;
    return;
  }

#if 0
  // get DestroyNotifyMessages from the source and check if the source
  // window id is valid
  XSelectInput(x11display,
               event.xclient.data.l[0],
               SubstructureNotifyMask);
#endif

  x11_drop_target_window = event.xclient.window;
  x11_drop_source_window = event.xclient.data.l[0];

  requested_type = NULL;
  dropsite = NULL;

  string data;
  long *typeptr;
  unsigned n;
  if (event.xclient.data.l[1]&1) {
    data = GetWindowProperty(x11_drop_source_window, xaXdndTypeList, XA_ATOM);
    typeptr = (long*)data.c_str();
    n = data.size()/sizeof(long);
  } else {
    typeptr = &event.xclient.data.l[2];
    n = 3;
  }

  drop_request.typelist.erase(drop_request.typelist.begin(), 
                              drop_request.typelist.end());
  for(unsigned i=0; i<n; i++) {
    if (*typeptr!=None) {
      drop_request.typelist.push_back(new TDnDType(AtomName(*typeptr)));
    }
    typeptr++;
  }

  if (drop_request.typelist.empty()) {
    cerr << "toad: received XdndEnter without types or illegal window id, ignoring" << endl;

    XErrorHandler oldhandler;
    oldhandler = XSetErrorHandler(dummyhandler);
    dummy = false;

    XSelectInput(x11display, x11_drop_source_window, 0);
    XSync(x11display, False);

    XSetErrorHandler(oldhandler);
    if (dummy) {
      cerr << "toad: received XdndEnter with illegal window id, ignoring" << endl;
    } else {
      cerr << "toad: received XdndEnter without types, ignoring" << endl;
    }

    x11_drop_target_window = None;
    x11_drop_source_window = None;
    return;
  }

  // start timeout timer
  // [code to add]    
}

void ReceivedXdndPosition(XEvent &event)
{
  if (x11_drop_source_window==None) {
    cerr << "toad: received unexpected XdndPosition event, ignoring" << endl;
    return;
  }
  if (static_cast<Window>(event.xclient.data.l[0])!=x11_drop_source_window) {
    cerr << "toad: received XdndPosition from the wrong source window, ignoring" << endl;
    cerr << "  source  : " << event.xclient.data.l[0] << endl;
    cerr << "  expected: " << x11_drop_source_window << endl;
    return;
  }
  if (event.xclient.window!=x11_drop_target_window) {
    cerr << "toad: received XdndPosition for the wrong target window, ignoring" << endl;
    return;
  }

#if VERBOSE
  cout << "  from window      : " << event.xclient.data.l[0] << endl;
  cout << "  flags            : " << event.xclient.data.l[1] << endl;
  cout << "  position         : " << (event.xclient.data.l[2]>>16) << ", "
                                  << (event.xclient.data.l[2] & 0xFFFF) << endl;
  cout << "  time             : " << event.xclient.data.l[3] << endl;
  cout << "  action           : " << AtomName(event.xclient.data.l[4]) << endl;
#endif
    
  int x = event.xclient.data.l[2] >> 16;
  int y = event.xclient.data.l[2] & 0xFFFF;

  // find the window
  //-----------------------------------
  HandlePosition(x11_drop_target_window,
                 x, y,
                 &drop_request,
                 Atom2Action(event.xclient.data.l[4]));
  
  XEvent se;
  se.xclient.type         = ClientMessage;
  se.xclient.serial       = 0;
  se.xclient.message_type = xaXdndStatus;
  se.xclient.format       = 32;
  se.xclient.window       = x11_drop_source_window;
  se.xclient.display      = x11display;
  se.xclient.send_event   = True;
  se.xclient.data.l[0]    = x11_drop_target_window;
  se.xclient.data.l[1]    = dropsite ? 1 : 0;   // accept drop? yes/no
  se.xclient.data.l[1]    |= 2;                 // position events in rectangle
  se.xclient.data.l[2]    = 0;                  // x,y
  se.xclient.data.l[3]    = 0;                  // w,h
  se.xclient.data.l[4]    = Action2Atom(drop_request.action);

#if VERBOSE
  cout << "sending XdndStatus" << endl;
  cout << "  to window        : " << x11_drop_source_window << endl;
  cout << "  from window      : " << se.xclient.data.l[0] << endl;
  cout << "  accept           : " << (se.xclient.data.l[1]&1 ? "yes" : "no") << endl;
  cout << "  events in rect   : " << (se.xclient.data.l[1]&2 ? "yes" : "no") << endl;
  cout << "  rect             : " << (se.xclient.data.l[2]>>16) << ", "
                                  << (se.xclient.data.l[2] & 0xFFFF) << ", "
                                  << (se.xclient.data.l[3]>>16) << ", "
                                  << (se.xclient.data.l[3] & 0xFFFF) << endl;
  cout << "  action           : " << AtomName(se.xclient.data.l[4]) << endl;
#endif

  if (XSendEvent(x11display, x11_drop_source_window, False, NoEventMask, &se)==0) {
    cerr << __FILE__ << ":" << __LINE__ << ": XSendEvent failed\n";
  }
}

void ReceivedXdndDrop(XEvent &event)
{
#if VERBOSE
  cout << "  to window        : " << event.xclient.window << endl;
  cout << "  from window      : " << event.xclient.data.l[0] << endl;
  cout << "  time             : " << event.xclient.data.l[2] << endl;
#endif
  if (x11_drop_source_window==None || dropsite==NULL) {
    cerr << "toad: received unexpected XdndDrop, ignoring" << endl;
    return;
  }
  if (static_cast<Window>(event.xclient.data.l[0])!=x11_drop_source_window) {
    cerr << "toad: received XdndDrop from the wrong source window, ignoring" << endl;
    return;
  }
  if (event.xclient.window!=x11_drop_target_window) {
    cerr << "toad: received XdndDrop for the wrong target window, ignoring" << endl;
    return;
  }
  ClearDropSite();
  ConvertNextTypeOrFinish(event.xclient.data.l[2]);
}

void ConvertNextTypeOrFinish(Time time)
{
  TDnDTypeList::iterator p, e;
  p = drop_request.typelist.begin();
  e = drop_request.typelist.end();
  while(p!=e) {
    if ((*p)->wanted) {
#if VERBOSE
      cout << "requesting type `" << (*p)->mime << "' at " << time << " for window " << x11_drop_target_window << endl;
#endif
      requested_type = *p;
      request_time = time;
      XConvertSelection(
        x11display,
        xaXdndSelection,                                    // selection name
        XInternAtom(x11display, (*p)->mime.c_str(), False), // target
        xaXdndSelection,                                    // property
        x11_drop_target_window,
        time);                                // time stamp
      (*p)->wanted = false;
      return;
    }
    p++;
  }

  XEvent event;
  PrepareXdndClientMessage(event, x11_drop_source_window, xaXdndFinished);
  event.xclient.data.l[0] = x11_drop_target_window;
  event.xclient.data.l[1] = 0;
#if VERBOSE
  cout << "sending XdndFinished" << endl;
  cout << "  from window      : " << event.xclient.data.l[0] << endl;
  cout << "  flags            : " << event.xclient.data.l[1] << endl;
#endif
  if (XSendEvent(x11display, x11_drop_source_window, False, NoEventMask, &event)==0) {
    cerr << __FILE__ << ":" << __LINE__ << ": XSendEvent failed\n";
  }
  
  x11_drop_source_window = None;
  x11_drop_target_window = None;
}

bool TOADBase::DnDSelectionNotify(XEvent &event)
{
#if VERBOSE
  cout << "got SelectionNotify" << endl;
  cout << "  requestor: " << event.xselection.requestor << endl;
  cout << "  selection: " << AtomName(event.xselection.selection) << endl;
  cout << "  target   : " << AtomName(event.xselection.target) << endl;
  cout << "  property : " << AtomName(event.xselection.property) << endl;
  cout << "  time     : " << event.xselection.time << endl;
#endif
  if (event.xselection.selection != xaXdndSelection)
    return false;

  if (dropsite==NULL || requested_type==NULL) {
    cerr << "toad: received unexpected SelectionNotify on XdndSelection, ignoring" << endl;
    return true;
  }
  if (event.xselection.requestor != x11_drop_target_window) {
    cerr << "toad: received SelectionNotify on XdndSelection for the wrong target window, ignoring" << endl;
    return true;
  }

  if (event.xselection.property==None) {
    event.xselection.property = xaXdndSelection;
  }

  if (event.xselection.target==None) {
    cerr << "toad: warning; received SelectionNotify but source failed to convert the data" << endl;
  } else {
    string data = GetWindowProperty(
      event.xselection.requestor,
      event.xselection.property,
      event.xselection.target);
#if VERBOSE
    cout << "received " << data.size() << " bytes" << endl;
#endif
    XDeleteProperty(x11display, 
                    event.xselection.requestor, 
                    event.xselection.property);
  
    TDnDObject drop;
    drop.flatdata = data;
    drop.typelist.push_back(new TDnDType(AtomName(event.xselection.target)));
    drop.type=drop.typelist[0];
    drop.x = drop_request.x;
    drop.y = drop_request.y;
    drop.local = false;
    dropsite->drop(drop);
  }
  requested_type = NULL;
  
  ConvertNextTypeOrFinish(event.xselection.time);
  
  return true;
}

/**
 * As of Xdnd 3.0: `Forget the whole incident'.
 */
void ReceivedXdndLeave(XEvent &event)
{
  if (x11_drop_source_window==None) {
    cerr << "toad: received unexpected XdndLeave, ignoring" << endl;
    return;
  }
  if (static_cast<Window>(event.xclient.data.l[0])!=x11_drop_source_window) {
    cerr << "toad: received XdndLeave from the wrong source window, ignoring" << endl;
    return;
  }
  if (event.xclient.window!=x11_drop_target_window) {
    cerr << "toad: received XdndLeave for the wrong target window, ignoring" << endl;
    return;
  }
  x11_drop_source_window = None;
  x11_drop_target_window = None;
  SetDropSite(NULL);
}

//---------------------------------------------------------------------------
// drop handling (extern & local)
//---------------------------------------------------------------------------

/**
 * Handle a local and external mouse position information.<BR>
 * The functions locates a drop site at the given position, sets the
 * attributes x,y and action in `drop', calls TDropSite::dropRequest
 * and sets the global `dropsite' variable.
 */
void HandlePosition(Window wnd,         // top-level window
                    int x, int y,       // mouse position relative to root
                    TDnDObject *drop,   // the object for `dropRequest'
                    unsigned action)  // prefered action
{
  Window xw = FindWindow(wnd,
                         x, y, 
                         &drop->x, &drop->y);
  drop->action=ACTION_NONE;
  TWindow *tw;
  if(!XFindContext(x11display, xw, toad::nClassContext, (XPointer*)&tw)) {
    // find all drop site for this window
    TWindowDropSiteMap::iterator wds = dropsitemap.find(tw);
    if (wds!=dropsitemap.end()) {
      TDropSite *ds = (*wds).second->find(drop->x, drop->y);
      if (ds) {
        drop->x -= tw->getOriginX();
        drop->y -= tw->getOriginY();
      
        // Xdnd defines that the dropsite can query the data right now if
        // it needs more information wether it accepts the drop or not.
        // The time stamp in XdndPosition should be used for XConvertSelection.
        // This feature isn't supported by TOAD.
        drop->action = action;
        drop->local = inside_local_window;
        ds->dropRequest(*drop);
        if (drop->action==ACTION_NONE)
          ds = NULL;
      }
      SetDropSite(ds);
    } else {
      SetDropSite(NULL);
    }
  } else {
    cout << "toad: error, XFindContext failed for drop position" << endl;
  }
}

#endif

// local
void
HandleDrop(TDnDObject *drop)
{
  if (dropsite) {
    ClearDropSite();
    drop->type = NULL;
    drop->local = true;
    dropsite->drop(*drop);
  }
}

//---------------------------------------------------------------------------
// common utility functions
//---------------------------------------------------------------------------

void TOADBase::initDnD()
{
#if VERBOSE
  cout << "Initializing Drag'n Drop" << endl;
#endif
#ifdef __X11__
  // Xdnd v3.0 Drag And Drop
  //-------------------------------
  xaXdndAware      = XInternAtom(x11display, "XdndAware", False);
  xaXdndEnter      = XInternAtom(x11display, "XdndEnter", False);
  xaXdndTypeList   = XInternAtom(x11display, "XdndTypeList", False);
  xaXdndPosition   = XInternAtom(x11display, "XdndPosition", False);
  xaXdndStatus     = XInternAtom(x11display, "XdndStatus", False);
  xaXdndLeave      = XInternAtom(x11display, "XdndLeave", False);
  xaXdndDrop       = XInternAtom(x11display, "XdndDrop", False);
  xaXdndFinished   = XInternAtom(x11display, "XdndFinished", False);
  xaXdndActionAsk  = XInternAtom(x11display, "XdndActionAsk", False);
  xaXdndActionCopy = XInternAtom(x11display, "XdndActionCopy", False);
  xaXdndActionMove = XInternAtom(x11display, "XdndActionMove", False);
  xaXdndActionLink = XInternAtom(x11display, "XdndActionLink", False);
  xaXdndActionPrivate= XInternAtom(x11display, "XdndActionPrivate", False);
  xaXdndSelection  = XInternAtom(x11display, "XdndSelection", False);

  x11_message_enter.xclient.type        = ClientMessage;
  x11_message_enter.xclient.serial      = 0;
  x11_message_enter.xclient.message_type= xaXdndEnter;
  x11_message_enter.xclient.format      = 32;
  x11_message_enter.xclient.send_event  = True;
#endif
}

#ifdef __X11__
void TOADBase::DnDNewShellWindow(TWindow *window)
{
  static long xdnd_version = XDND_VERSION;
  XChangeProperty(x11display, window->x11window,
                  xaXdndAware, XA_ATOM, 32,
                  PropModeReplace,
                  (unsigned char*)&xdnd_version,1);
}
#endif

#ifdef __X11__
bool TOADBase::DnDClientMessage(XEvent &event)
{
#if VERBOSE
  cout << "got " << AtomName(event.xclient.message_type) << endl;
#endif
  if (event.xclient.message_type == xaXdndEnter) {
    ReceivedXdndEnter(event);
  } else if (event.xclient.message_type == xaXdndPosition) {
    ReceivedXdndPosition(event);
  } else if (event.xclient.message_type == xaXdndStatus) {
    ReceivedXdndStatus(event);
  } else if (event.xclient.message_type == xaXdndDrop) {
    ReceivedXdndDrop(event);
  } else if (event.xclient.message_type == xaXdndFinished) {
    ReceivedXdndFinished(event);
  } else if (event.xclient.message_type == xaXdndLeave) {
    ReceivedXdndLeave(event);
  }
  return false;
}

/**
 * Prepares an XEvent structure for an XSendEvent function.
 */
void PrepareXdndClientMessage(XEvent &event, Window dest, Atom type)
{
  event.xclient.type          = ClientMessage;
  event.xclient.serial        = 0;
  event.xclient.message_type  = type;
  event.xclient.format        = 32;
  event.xclient.window        = dest;
  event.xclient.display       = x11display;
  event.xclient.send_event    = True;
  event.xclient.data.l[0]     = x11_drag_source_window;
  event.xclient.data.l[1]     = 0;
  event.xclient.data.l[2]     = 0;
  event.xclient.data.l[3]     = 0;
  event.xclient.data.l[4]     = 0;
}

#if 0
void HexDump(byte* buffer, int received)
{
    int data = 0;
    while(data<received) {
      for(int x=0; x<16; x++) {
        if (data<received)
          printf("%02x ", (int)buffer[data]);
        else
          printf("   ");
        data++;
      }
      data-=16;
      for(int x=0; x<16; x++) {
        if (data<received)
          printf("%c", buffer[data]>=32 && buffer[data]<=127 ? buffer[data] : '.');
        else
          printf(" ");
        data++;
      }
      printf("\n");
    }
}
#endif

string AtomName(Atom atom)
{
  string result = "(None)";
  if (atom) {
    char *name = XGetAtomName(x11display, atom);
    result = name;
    XFree(name);
  }
  return result;
}

string GetWindowProperty(Window source, Atom property, Atom type)
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
    if (XGetWindowProperty(x11display,
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

Window FindWindow(Window xw, int x, int y, int *tx, int *ty)
{
  while(true) {
    Window nw;
    XTranslateCoordinates(
      x11display,
      DefaultRootWindow(x11display),
      xw,
      x,y,
      tx, ty,
      &nw);
    if (nw==None)
      break;
    xw = nw;
  }
  return xw;
}

unsigned Atom2Action(Atom atom)
{
  if (atom==xaXdndActionCopy) {
    return ACTION_COPY;
  } else if (atom==xaXdndActionMove) {
    return ACTION_MOVE;
  } else if (atom==xaXdndActionLink) {
    return ACTION_LINK;
  } else if (atom==xaXdndActionAsk) {
    return ACTION_ASK;
  }
  return ACTION_PRIVATE;
}

Atom Action2Atom(unsigned action) {
  switch(action) {
    case ACTION_NONE:
      return None;
    case ACTION_COPY:
      return xaXdndActionCopy;
    case ACTION_MOVE:
      return xaXdndActionMove;
    case ACTION_LINK:
      return xaXdndActionLink;
    case ACTION_ASK:
      return xaXdndActionAsk;
    case ACTION_PRIVATE:
      break;
  }
  return ACTION_PRIVATE;
}

/**
 * No folks, no! I asure ya, this won't be the final way we do create our
 * cursors! The right thing to is to rewrite TBitmap and load the stuff
 * via a "memory://" location.
 * <p>
 * (please take a look at the code to understand this comment)
 */
static void BuildCursor()
{
  static bool once = false;
  if (once) return;
  once = true;

  static const char bm[6][32][32+1] = {
  // copy
  // 0        1         2         3
  // 12345678901234567890123456789012
  { "..................              ", // 1
    ".################..             ", // 2
    ".################.#.            ", // 3
    ".################.##.           ", // 4
    ".################.###.          ", // 5
    ".################......         ", // 6
    ".#####################.         ", // 7
    ".#####################.         ", // 8
    ".#####################.         ", // 9
    ".#######..############.         ", // 10
    ".#######....##########.         ", // 11
    ".########.....########.         ", // 12
    ".########.......######.         ", // 13
    ".#########........####.         ", // 14
    ".#########........####.         ", // 15
    ".##########...###########       ", // 16
    ".##########...#.........#       ", // 17
    ".###########..#.#######.#       ", // 18
    ".###########..#.#######.######  ", // 19
    ".##############.#######......#  ", // 20
    ".##############.#######.####.#  ", // 21
    ".##############.#######.####.#  ", // 22
    ".##############.#######.####.#  ", // 23
    ".##############.#######.####.#  ", // 24
    ".##############.#######.####.#  ", // 25
    "..............#.#######.####.#  ", // 26
    "              #.........####.#  ", // 27
    "              #####.########.#  ", // 28
    "                  #.########.#  ", // 29
    "                  #.########.#  ", // 30
    "                  #..........#  ", // 31
    "                  ############  "  // 32
  },
  // no copy
  // 0        1         2         3
  // 12345678901234567890123456789012
  { "..................              ", // 1
    ".################..             ", // 2
    ".################.#.            ", // 3
    ".################.##.           ", // 4
    ".################.###.          ", // 5
    ".################......         ", // 6
    ".#####################.         ", // 7
    ".#####################.         ", // 8
    ".#####################.         ", // 9
    ".###########.....#####.         ", // 10
    ".#########.........###.         ", // 11
    ".########...######..##.         ", // 12
    ".########..#####....##.         ", // 13
    ".#######..#####...#..#.         ", // 14
    ".#######..####...##..#.         ", // 15
    ".#######..###.###########       ", // 16
    ".#######..##..#.........#       ", // 17
    ".#######..#...#.#######.#       ", // 18
    ".########....##.#######.######  ", // 19
    ".########...###.#######......#  ", // 20
    ".#########....#.#######.####.#  ", // 21
    ".###########..#.#######.####.#  ", // 22
    ".##############.#######.####.#  ", // 23
    ".##############.#######.####.#  ", // 24
    ".##############.#######.####.#  ", // 25
    "..............#.#######.####.#  ", // 26
    "              #.........####.#  ", // 27
    "              #####.########.#  ", // 28
    "                  #.########.#  ", // 29
    "                  #.########.#  ", // 30
    "                  #..........#  ", // 31
    "                  ############  "  // 32
  },
  // move
  // 0        1         2         3
  // 12345678901234567890123456789012
  { "..................              ", // 1
    ".################..             ", // 2
    ".################.#.            ", // 3
    ".################.##.           ", // 4
    ".################.###.          ", // 5
    ".################......         ", // 6
    ".#####################.         ", // 7
    ".#####################.         ", // 8
    ".#####################.         ", // 9
    ".#######..############.         ", // 10
    ".#######....##########.         ", // 11
    ".########.....########.         ", // 12
    ".########.......######.         ", // 13
    ".#########........####.         ", // 14
    ".#########........####.         ", // 15
    ".##########.....######.         ", // 16
    ".##########......#####.         ", // 17
    ".###########..#...####.         ", // 18
    ".###########..##...###.         ", // 19
    ".################...##.         ", // 20
    ".#################...##         ", // 21
    ".##################...#         ", // 22
    ".###################..#         ", // 23
    ".######################         ", // 24
    ".#####################.         ", // 25
    ".......................         ", // 26
    "                                ", // 27
    "                                ", // 28
    "                                ", // 29
    "                                ", // 30
    "                                ", // 31
    "                                "  // 32
  },
  // no move
  // 0        1         2         3
  // 12345678901234567890123456789012
  { "..................              ", // 1
    ".################..             ", // 2
    ".################.#.            ", // 3
    ".################.##.           ", // 4
    ".################.###.          ", // 5
    ".################......         ", // 6
    ".#####################.         ", // 7
    ".#####################.         ", // 8
    ".#####################.         ", // 9
    ".#####################.         ", // 10
    ".############.....####.         ", // 11
    ".##########.........##.         ", // 12
    ".#########...#####...#.         ", // 13
    ".#########..#####....#.         ", // 14
    ".########..#####...#..#         ", // 15
    ".########..####...##..#         ", // 16
    ".########..###...###..#         ", // 17
    ".########..##...####..#         ", // 18
    ".########..#...#####..#         ", // 19
    ".#########....#####..#.         ", // 20
    ".#########...#####...#.         ", // 21
    ".##########.........##.         ", // 22
    ".############.....####.         ", // 23
    ".#####################.         ", // 24
    ".#####################.         ", // 25
    ".......................         ", // 26
    "                                ", // 27
    "                                ", // 28
    "                                ", // 29
    "                                ", // 30
    "                                ", // 31
    "                                "  // 32
  },
  // link
  // 0        1         2         3
  // 12345678901234567890123456789012
  { "..................              ", // 1
    ".################..             ", // 2
    ".################.#.            ", // 3
    ".################.##.           ", // 4
    ".################.###.          ", // 5
    ".################......         ", // 6
    ".#####################.         ", // 7
    ".#####################.         ", // 8
    ".#######..############.         ", // 9
    ".#######....##########.         ", // 10
    ".########.....########.         ", // 11
    ".########.......######.         ", // 12
    ".#########........####.         ", // 13
    ".#########........####.         ", // 14
    ".##########...############      ", // 15
    ".##########...#..........#      ", // 16
    ".###########..#.########.#      ", // 17
    ".###########..#.########.#      ", // 18
    ".##############.##..####.#      ", // 19
    ".##############.##..####.#######", // 20
    ".##############.####.###.......#", // 21
    ".##############.#####.########.#", // 22
    ".##############.######.#######.#", // 23
    ".##############.#######.######.#", // 24
    ".##############.....####.#####.#", // 25
    "..............#####.#####.####.#", // 26
    "                  #.######..##.#", // 27
    "                  #.######..##.#", // 28
    "                  #.##########.#", // 29
    "                  #.##########.#", // 30
    "                  #............#", // 31
    "                  ##############"  // 32
  },
  // no link
  // 0        1         2         3
  // 12345678901234567890123456789012
  { "..................              ", // 1
    ".################..             ", // 2
    ".################.#.            ", // 3
    ".################.##.           ", // 4
    ".################.###.          ", // 5
    ".################......         ", // 6
    ".#####################.         ", // 7
    ".#####################.         ", // 8
    ".###########.....#####.         ", // 9
    ".#########.........###.         ", // 10
    ".########...#####...##.         ", // 11
    ".########..#####....##.         ", // 12
    ".#######..#####...#..#.         ", // 13
    ".#######..####...##..#.         ", // 14
    ".#######..###.############      ", // 15
    ".#######..##..#..........#      ", // 16
    ".#######..#...#.########.#      ", // 17
    ".#######.....##.########.#      ", // 18
    ".########...###.##..####.#      ", // 19
    ".#########....#.##..####.#######", // 20
    ".###########..#.####.###.......#", // 21
    ".##############.#####.########.#", // 22
    ".##############.######.#######.#", // 23
    ".##############.#######.######.#", // 24
    ".##############.....####.#####.#", // 25
    "..............#####.#####.####.#", // 26
    "                  #.######..##.#", // 27
    "                  #.######..##.#", // 28
    "                  #.##########.#", // 29
    "                  #.##########.#", // 30
    "                  #............#", // 31
    "                  ##############"  // 32
  }
  };

  XColor fc, bc;
  fc.red = fc.green = fc.blue = 0xFFFF;
  fc.flags = DoRed|DoGreen|DoBlue;
  bc.red = bc.green = bc.blue = 0x0000;
  fc.flags = DoRed|DoGreen|DoBlue;

  GC gc0, gc1;
  gc0 = None;

  for(int i=0; i<6; i++) {
    Pixmap pm_icon;
    Pixmap pm_mask;
    pm_icon = XCreatePixmap(x11display, DefaultRootWindow(x11display),
              32, 32, 1);
    pm_mask = XCreatePixmap(x11display, DefaultRootWindow(x11display),
              32, 32, 1);

    if (gc0==None) {
      XGCValues gv;
      gv.foreground=0;
      gc0 = XCreateGC(x11display, pm_icon, GCForeground, &gv);
      gv.foreground=1;
      gc1 = XCreateGC(x11display, pm_icon, GCForeground, &gv);
    }

    for(int y=0; y<32; y++) {
      for(int x=0; x<32; x++) {
        if (bm[i][y][x]==' ') {
          XDrawPoint(x11display, pm_mask, gc0, x,y);
        } else {
          XDrawPoint(x11display, pm_mask, gc1, x,y);
        }
        if (bm[i][y][x]=='.') {
          XDrawPoint(x11display, pm_icon, gc0, x,y);
        } else {
          XDrawPoint(x11display, pm_icon, gc1, x,y);
        }
      }
    }
    dnd_cursor[i] = XCreatePixmapCursor(x11display, 
                                     pm_icon, pm_mask,
                                     &fc, &bc,
                                     0,0);
    XFreePixmap(x11display, pm_icon);
    XFreePixmap(x11display, pm_mask);
  }
  
  XFreeGC(x11display, gc0);
  XFreeGC(x11display, gc1);
}

#endif

} // namespace toad
