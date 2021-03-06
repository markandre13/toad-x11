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

#include <unistd.h>

/**
 * \class toad::TWindow
 * \brief TWindow is the super class of all windows.
 *
 * A window is a rectangle area on the screen which receives input events,
 * eg. from the mouse pointer, the keyboard, displays graphics and can
 * contain other windows.
 *
 * \li
 *   parentless windows: Windows with a parent of NULL are parentless windows.
 *   An application will have at least one such windows. When all parentless
 *   windows are closed, the toad::mainLoop() function will return and the
 *   application can shutdown. (Another way is calling 'postQuitMessage').
 *
 *   See also: bParentlessAssistant.
 */
 
/*
 *
 * How paint events are handled:
 * 
 * \li
 *   TWindow::_providePaintRgn creates an empty paint event for the window
 *   and place it into the paint event queue.
 *
 * \li
 *   TWindow::_dispatchPaintEvent removes a paint event from the paint event
 *   queue and handles it.
 *
 * \todo
 *   \li
 *     Remove the code for X event selection based on overridden virtual
 *     methods. It became obsolete with event filters and mouseEvent.
 *     Instead, select all events with the exception of MotionNotify's
 *     when no mouse button is pressed. This should cover most situations,
 *     optimizations or MotionNotify's when no button is pressed must be
 *     registered by the programmer then.
 *   \li
 *     TWindow should delete all TSignal connections to a window in it's
 *     destructor (or give a warning in a debug version at last)
 *   \li
 *     the set... methods should return a pointer to the object to make
 *     code like this possible:
 *     wnd->setShape(5,5,80,16)->setTitle("window)
 */

// didn't knew a better place to describe this group:

/**
 * \defgroup control Control
 *
 * A control is a predefined window which interacts with the user like
 * a pushbutton, a textfield, a scrollbar, a slider and so on.
 */

#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/cursorfont.h>
#endif

#ifdef __WIN32__
#define STRICT
#define W32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/region.hh>
#include <toad/dragndrop.hh>
#include <toad/dialogeditor.hh>
#include <toad/layout.hh>
#include <toad/io/urlstream.hh>
#include <toad/bitmap.hh>

using namespace toad;

#include <vector>
#include <queue>
#include <map>

// obsolete:
struct TWndPtrComp
{
  bool operator()(const TWindow *a, const TWindow *b) const
  {
    return a<b;
  }
};

#ifdef __TOAD_THREADS
  #undef __TOAD_THREADS
#endif

#ifdef __TOAD_THREADS
  static TThreadMutex mutexPaintQueue;
  #define THREAD_LOCK(A) A.Lock()
  #define THREAD_UNLOCK(A) A.Unlock();
#else
  #define THREAD_LOCK(A)
  #define THREAD_UNLOCK(A)
#endif


// window icons
//---------------------------------------------------------------------------
// Since bitmaps are barely used, we don't store them in TWindow but in two
// external maps. The intention is to keep TWindow small.
typedef std::map<TWindow*,TBitmap*,TWndPtrComp> TWndBmpList;
static TWndBmpList iconlist;
static TWndBmpList backgroundlist;

typedef vector<TWindow*> TVectorParentless;
static TVectorParentless parentless;

// some extra functions
//---------------------------------------------------------------------------

#define isTopLevel(w) \
  (w->flagPopup || w->getParent()==NULL)

#ifdef __X11__

// LessTif Window Manager Hints
//---------------------------------------------------------------------------
#define PROP_MOTIF_WM_HINTS_ELEMENTS 5

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)
                    
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
                    
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
                    
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#define MWM_INPUT_APPLICATION_MODAL MWM_INPUT_PRIMARY_APPLICATION_MODAL
                    
#define MWM_TEAROFF_WINDOW      (1L<<0)

struct PropMotifWmHints {
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long inputMode;
  unsigned long status;
};
#endif

#ifdef __COCOA__

static set<TWindow*> aw;

@interface toadWindow : NSWindow
{
  @public
    TWindow *twindow;
}
@end

@implementation toadWindow : NSWindow
- (void)becomeKeyWindow {
  [super becomeKeyWindow];
  TOADBase::domainToWindow(twindow);
}
- (void)resignKeyWindow {
  [super resignKeyWindow];
  TOADBase::domainToWindow(0);
}
@end

@interface toadView : NSView
{
  @public
    TWindow *twindow;
    NSTrackingRectTag trackAll;
}
@end

@implementation toadView : NSView
- (void) initTrackAll:(NSRect)frame {
  trackAll = [self addTrackingRect: frame
              owner: self
              userData: NULL
              assumeInside: NO];
  // [self translateOriginToPoint: NSMakePoint(-0.5, -0.5)];
  // [self setBoundsOrigin: NSMakePoint(-0.5, -0.5)];
}
// TRUE: (0,0) is in the upper-left-corner
- (BOOL)isFlipped {
  return TRUE;
}
- (BOOL)isOpaque {
  return TRUE;
}
- (void) setFrameSize:(NSSize)newSize
{
//  printf("%s: (%f,%f)\n", __FUNCTION__, newSize.width, newSize.height);
  [super setFrameSize:newSize];
  [self removeTrackingRect: trackAll];
  trackAll = [self addTrackingRect: NSMakeRect(0, 0, newSize.width, newSize.height)
              owner: self
              userData: NULL
              assumeInside: NO];
  twindow->w = newSize.width;   
  twindow->h = newSize.height;  
  twindow->doResize();
}
 
- (void) drawRect:(NSRect)rect
{
  // [self inLiveResize]
//printf("print draw rect (%f,%f,%f,%f)\n",rect.origin.x,rect.origin.y,
//                                         rect.size.width,rect.size.height);
  if (aw.find(twindow)==aw.end()) {
    cerr << "window is no more" << endl;
    exit(1);
  }
   
  {
  TPen pen(twindow);
  pen.setColor(twindow->_bg.r, twindow->_bg.g, twindow->_bg.b);
  pen.fillRectangle(0,0,twindow->w,twindow->h);
  }
  twindow->paint();
}
/**
 * return YES when we can handle key events and action messages
 */
- (BOOL)acceptsFirstResponder
{
//  printf("accept focus: %s %s\n", __FUNCTION__, twindow->getTitle().c_str());
  return YES;
}
 
- (BOOL)becomeFirstResponder
{
//  printf("became focus: %s %s\n", __FUNCTION__, twindow->getTitle().c_str());
  return YES;
}
 
- (BOOL)resignFirstResponder
{
  printf("lost focus: %s %s\n", __FUNCTION__, twindow->getTitle().c_str());
  return YES;
}
 
- (void) keyDown:(NSEvent*)theEvent
{
//printf("key down\n");
  TKeyEvent ke(theEvent);
  ke.type = TKeyEvent::DOWN;
  //twindow->keyEvent(ke);  
  TOADBase::handleKeyEvent(ke);
  executeMessages();
}
- (void) keyUp:(NSEvent*)theEvent
{
//printf("key up\n");
  TKeyEvent ke(theEvent);
  ke.type = TKeyEvent::UP;
  //twindow->keyEvent(ke);
  TOADBase::handleKeyEvent(ke);
  executeMessages();
}
 
- (void) mouseEntered:(NSEvent*)theEvent
{
//printf("%s: %s\n",__FUNCTION__, twindow->getTitle().c_str());
  twindow->_inside = true;
  TMouseEvent me(theEvent, self, twindow);
  me.type = TMouseEvent::ENTER;
  me._modifier = TMouseEvent::globalModifier;
  twindow->mouseEvent(me);
  executeMessages();
}
- (void) mouseExited:(NSEvent*)theEvent
{
//printf("%s: %s\n",__FUNCTION__, twindow->getTitle().c_str());
  twindow->_inside = false;
  TMouseEvent me(theEvent, self, twindow);
  me.type = TMouseEvent::LEAVE;
  me._modifier = TMouseEvent::globalModifier;
  twindow->mouseEvent(me);
  executeMessages();
}

- (void) mouseDown:(NSEvent*)theEvent
{
  TMouseEvent::globalModifier |= MK_LBUTTON;
  twindow->_down(TMouseEvent::LDOWN, theEvent);
}
- (void) rightMouseDown:(NSEvent*)theEvent
{
  TMouseEvent::globalModifier |= MK_RBUTTON;
  twindow->_down(TMouseEvent::RDOWN, theEvent);
}
- (void) otherMouseDown:(NSEvent*)theEvent
{
  TMouseEvent::globalModifier |= MK_MBUTTON;
  twindow->_down(TMouseEvent::MDOWN, theEvent);
}
void
TWindow::_down(TMouseEvent::EType type, NSEvent *theEvent)
{
//printf("%s: %s\n",__FUNCTION__, twindow->getTitle().c_str());
  TMouseEvent me(theEvent, nsview, this);
  if (!_inside) {
//printf("  flip inside\n");
    _inside = true;
    TMouseEvent me(theEvent, nsview, this);
    me.type = TMouseEvent::ENTER;
    me._modifier = TMouseEvent::globalModifier;
    mouseEvent(me);
  }
  me.type = type;
  me._modifier = TMouseEvent::globalModifier;
  me.dblClick = [theEvent clickCount]==2;
  _inside = true;
  mouseEvent(me);
  executeMessages();
}
 
- (void) mouseUp:(NSEvent*)theEvent
{
  TMouseEvent::globalModifier &= ~MK_LBUTTON;
  twindow->_up(TMouseEvent::LUP, theEvent);
}
- (void) rightMouseUp:(NSEvent*)theEvent
{
  TMouseEvent::globalModifier &= ~MK_RBUTTON;
  twindow->_up(TMouseEvent::RUP, theEvent);
}
- (void) otherMouseUp:(NSEvent*)theEvent
{
  TMouseEvent::globalModifier &= ~MK_MBUTTON;
  twindow->_up(TMouseEvent::MUP, theEvent);
}
void
TWindow::_up(TMouseEvent::EType type, NSEvent *theEvent)
{
//printf("%s: %s\n",__FUNCTION__, twindow->getTitle().c_str());
  TMouseEvent me(theEvent, nsview, this);
  if (!_inside) {
//printf("  flip outside\n");
    _inside = false;
    TMouseEvent me(theEvent, nsview, this);
    me.type = TMouseEvent::LEAVE;
    me._modifier = TMouseEvent::globalModifier;
    mouseEvent(me);
  }
  me.type = type;
  me._modifier = TMouseEvent::globalModifier;
  mouseEvent(me);
  executeMessages();
}
- (void) mouseDragged:(NSEvent*)theEvent
{
//printf("%s: %s _inside=%i\n",__FUNCTION__, twindow->getTitle().c_str(),twindow->_inside);
  TMouseEvent me(theEvent, self, twindow);
  TRectangle r(0,0,twindow->w,twindow->h);
  if (twindow->_inside != r.isInside(me.x, me.y)) {
//printf("  flip inside/outide\n");
    twindow->_inside = !twindow->_inside;
    TMouseEvent me(theEvent, self, twindow);
    me.type = twindow->_inside ? TMouseEvent::ENTER : TMouseEvent::LEAVE;
    twindow->mouseEvent(me);
  }
  me.type = TMouseEvent::MOVE;
  twindow->mouseEvent(me);
  executeMessages();
}
 
- (void) mouseMoved:(NSEvent*)theEvent
{
//printf("%s: %s _inside=%i\n",__FUNCTION__, twindow->getTitle().c_str(),twindow->_inside);
  TMouseEvent me(theEvent, self, twindow);
  TRectangle r(0,0,twindow->w,twindow->h);
  if (twindow->_inside != r.isInside(me.x, me.y)) {
//printf("  flip inside/outide\n");
    twindow->_inside = !twindow->_inside;
    TMouseEvent me(theEvent, self, twindow);
    me.type = twindow->_inside ? TMouseEvent::ENTER : TMouseEvent::LEAVE;
    twindow->mouseEvent(me);
  }
  me.type = TMouseEvent::MOVE;
  twindow->mouseEvent(me);
  executeMessages();
}
@end

unsigned TMouseEvent::globalModifier;

TMouseEvent::TMouseEvent(NSEvent *ne, NSView *view, TWindow *w) {
  nsevent = ne;
  NSPoint pt = [view convertPoint:[ne locationInWindow] fromView:nil];
  x = pt.x;
  y = pt.y;
  x -= w->getOriginX();
  y -= w->getOriginY();
  window = w;
  dblClick = false;
}
#endif

TWindow::TWindow(TWindow *p, const string &title)
  :TInteractor(p, title)
{
  if (p==NULL) {
    parentless.push_back(this);
  }

  #ifdef __X11__
  x11window = 0;
  #endif
  
  #ifdef __WIN32__
  w32window = 0;
  paintstruct = 0;
  #endif
  
  #ifdef __COCOA__
  aw.insert(this);

  nsview = nil;
  nswindow = nil;
  _inside = false;
  _mapped = false;
  #endif
  
  // public flags
  flagShell = flagPopup = bExplicitCreate = bSaveUnder = bStaticFrame =
  bBackingStore = bNoBackground = bX11GC = bFocusManager = bNoFocus = 
  bNoMenu = bTabKey = bDialogEditRequest = bDoubleBuffer = 
  bParentlessAssistant = false;
  
  bCompressMotion = bFocusTraversal = true;

  // private flags
  bEraseBe4Paint = false;
  _bOwnsFocus = false;
  _bResizedBeforeCreate = false;
  _bToolTipAvailable = false;
  _visible = true;

  x    = 0;
  y    = 0;
  w  = 320;
  h  = 200;
  _b  = 1;
  _dx = _dy = 0;  // origin for TPen
#ifdef __X11__
  _cursor = 0;
#endif
  setCursor(TCursor::DEFAULT);
  paint_rgn = NULL;
  _bg.set(1.0,1.0,1.0);
  layout = NULL;
  
  // private flags
  flag_wm_resize    = flag_explicit_create = flag_delete_title =
  flag_mmm_modified = flag_create          = flag_suppress_msg = 
  flag_child_notify = false;
  flag_position_undefined = true;
  _allmousemove = false;
#ifdef DEBUG
  debug_flags = 0;
#endif
//  child = prev_sibling = next_sibling = NULL;

  _childNotify(TCHILD_ADD);
}

TWindow::~TWindow()
{
  if (layout) {
    layout->toFile();
    delete layout;
  }

//  printf("%s: %08x\n", __PRETTY_FUNCTION__, this);
  if (getParent()==NULL) {
//    printf("  is parentless\n");
    for(TVectorParentless::iterator p = parentless.begin();
        p != parentless.end();
        ++p)
    {
      if (*p == this) {
//        printf("  removed\n");
        parentless.erase(p);
        break;
      }
    }
  }

  // delete children before freeing resources which might be used be them
  deleteChildren();

  setToolTip("");

  // remove window from paint queue
  //--------------------------------
  THREAD_LOCK(mutexPaintQueue);
  if (paint_rgn)
    paint_rgn->wnd = NULL;
  THREAD_UNLOCK(mutexPaintQueue);

  if (isRealized())
    destroyWindow();

  #ifdef __COCOA__
  aw.erase(aw.find(this));
  #endif

  _childNotify(TCHILD_REMOVE);

  // remove messages for this window
  //--------------------------------
  removeMessage(this);

  #ifdef __X11__
  if (x11window) {
    XDeleteContext(x11display, x11window, nClassContext);
    XDestroyWindow(x11display, x11window);
    x11window = 0;
  }
  #endif
  
  #ifdef __WIN32__
  if (w32window) {
    exit(0);
  }
  #endif

  // free bitmaps (icon & background)
  //--------------------------------
  TWndBmpList::iterator p = iconlist.find(this);
  if (p!=iconlist.end()) {
    if ((*p).second)
      delete (*p).second;
    iconlist.erase(p);
  }
  
  p = backgroundlist.find(this);
  if (p!=backgroundlist.end()) {
    if ((*p).second)
      delete (*p).second;
    backgroundlist.erase(p);
  }
}

unsigned
TWindow::getParentlessCount()
{
  return parentless.size();
}

TWindow* 
TWindow::getParentless(unsigned i)
{
  if (i>parentless.size())
    return NULL;
  return parentless[i];
}

/**
 * Create all parentless windows.
 *
 * Windows with bExplicitCreate == true won't be created.
 *
 * This message is invoked by TOADBase::runApp.
 *
 * \return 'false' when there are no parentless windows.
 */
bool
TWindow::createParentless()
{
  TVectorParentless::iterator p = parentless.begin();
  TVectorParentless::iterator e = parentless.end();
  if (p==e)
    return false;
  
  while(p!=e) {
    if (!(*p)->bExplicitCreate)
      (*p)->createWindow();
    p++;
  }
  return true;
}

/**
 * Destroy all parentless windows.
 *
 * This message is invoked by TOADBase::runApp.
 */
void
TWindow::destroyParentless()
{
  TVectorParentless::iterator p = parentless.begin();
  TVectorParentless::iterator e = parentless.end();
  while(p!=e) {
//    printf("%s: %08x\n", __PRETTY_FUNCTION__, *p);
//    cout << "destroying window" << (*p)->getTitle() << endl;
    if ((*p)->isRealized()) {
      (*p)->destroyWindow();
      p = parentless.begin();
      e = parentless.end();
      continue;
    }
    p++;
  }
}


/**
 * Create the X11 window.
 */
void 
TWindow::createWindow()
{
  if (isRealized())
    return;
    
  #ifdef __X11__
  if (x11window || flag_create)
    return;

  flag_create = true; // avoid recursion (for TMDIWindow)

  if (getParent() && !getParent()->x11window ) {
#warning "disabled stupid hack for TMDIWindow without fixing TMDIWindow"
//    getParent()->createWindow(); // create parent (for TMDIWindow)
  } else if (!x11window) {
    flag_explicit_create = true; // ???
    _interactor_init();      // call create top-down
    _interactor_adjustW2C(); // call adjust bottom-up
    _interactor_create();    // now create windows top-down
    flag_explicit_create = false; // ???
  }
  flag_create = false;
  #endif

  #ifdef __COCOA__
  if (nsview || flag_create)
    return;

  flag_create = true; // avoid recursion (for TMDIWindow)

  if (getParent() && !getParent()->nsview ) {
#warning "disabled stupid hack for TMDIWindow without fixing TMDIWindow"
//    getParent()->createWindow(); // create parent (for TMDIWindow)
  } else if (!nsview) {
    flag_explicit_create = true; // ???
    _interactor_init();      // call create top-down
    _interactor_adjustW2C(); // call adjust bottom-up
    _interactor_create();    // now create windows top-down
    flag_explicit_create = false; // ???
  }
  flag_create = false;
  #endif

  #ifdef __WIN32__
  if (w32window || flag_create)
    return;

  flag_create = true; // avoid recursion (for TMDIWindow)

  if (getParent() && !getParent()->w32window ) {
#warning "disabled stupid hack for TMDIWindow without fixing TMDIWindow"
//    getParent()->createWindow(); // create parent (for TMDIWindow)
  } else if (!w32window) {
    flag_explicit_create = true; // ???
    _interactor_init();      // call create top-down
    _interactor_adjustW2C(); // call adjust bottom-up
    _interactor_create();    // now create windows top-down
    flag_explicit_create = false; // ???
  }
  flag_create = false;
  #endif
}

void 
TWindow::_interactor_init()
{
  // only init windows with bExplicitCreate style, when createWindow()
  // was called for this window directly
  //-----------------------------------------------------------------
  if ( bExplicitCreate && !flag_explicit_create )
    return;

  // mark children that have been added _before_ init(); these children
  // won't be deleted when calling 'destroyWindow()'
  //--------------------------------------------------------------------
  TInteractor *ptr = getFirstChild();
  while(ptr) {
    ptr->before_create=true;
    ptr = getNextSibling(ptr);
  }

  // call 'create()'; it's a common error to call methods now that need
  // a valid 'window', DFLAG_WMINIT will tell that this error appeared
  // during 'init()'
  //--------------------------------------------------------------------
  #ifdef DEBUG
    debug_flags|=DFLAG_WMINIT;
    create();
    debug_flags&=~DFLAG_WMINIT;
  #else
    create();
  #endif

  // init all children
  //-------------------
  ptr=getFirstChild();
  while(ptr) {
    ptr->_interactor_init();
    ptr=getNextSibling(ptr);
  }
}

void 
TWindow::_interactor_adjustW2C()
{
  if (bExplicitCreate && !flag_explicit_create)
    return;

  TInteractor *ptr = getFirstChild();
  while(ptr) {
    ptr->_interactor_adjustW2C();
    ptr=getNextSibling(ptr);
  }
  #ifdef DEBUG
    debug_flags|=DFLAG_WMADJUST;
    adjust();
    debug_flags&=~DFLAG_WMADJUST;
  #else
    adjust();
  #endif
}

void 
TWindow::_interactor_create()
{
  #ifdef SECURE

  #ifdef __X11__
  if (x11window) {
    cerr << "toad: internal error; mustn't create an existing window";
    return;
  }
  #endif
  
  #ifdef __WIN32__
  if (w32window)
    return;
  #endif
  
  #endif

  if (bExplicitCreate && !flag_explicit_create)
    return;

  _childNotify(TCHILD_BEFORE_CREATE);

  // set flagShell flag for all top level windows
  //-------------------------------------------
  if (isTopLevel(this)) // parent == NULL || flagPopup
    flagShell=true;

  // get all window attributes
  //---------------------------

#ifdef __X11__
  unsigned long mask=0;
  XSetWindowAttributes attr;

  TWndBmpList::iterator p = backgroundlist.find(this);
  if (p!=backgroundlist.end()) {
    mask|=CWBackPixmap;
    (*p).second->update();
    attr.background_pixmap = (*p).second->pixmap;
  } else if (!bNoBackground && !bDoubleBuffer) {
    mask|=CWBackPixel;
    attr.background_pixel = TColor::_getPixel(_bg);
  }

  mask|=CWSaveUnder;
  attr.save_under = bSaveUnder;
  
  mask|=CWBackingStore;
  attr.backing_store = bBackingStore ? WhenMapped : NotUseful;

  mask|=CWEventMask;
  attr.event_mask = _buildEventmask();
  
  mask|=CWColormap;
  attr.colormap = x11colormap;

  if (_cursor!=0) {
    mask|=CWCursor;
    attr.cursor = _cursor;
  }
  
  if (flagPopup) {
    mask|=CWOverrideRedirect;
    attr.override_redirect = true;
    // the override redirect option results in a 'X' cursor but we 
    // still want the windows cursor:
    mask|=CWCursor;
    // attr.cursor = TCursor::X11Cursor(TCursor::DEFAULT);
    attr.cursor = _cursor;
  }

  // 'createX11Window' messsage is needed for things like OpenGL support
  static TX11CreateWindow x11;
  x11.display   = x11display;
  x11.parent    = ( getParent() && !flagShell ) ? getParent()->x11window : DefaultRootWindow(x11display);
  x11.x         = (int)x;
  x11.y         = (int)y;
  x11.width     = (unsigned)w;
  x11.height    = (unsigned)h;
  x11.border    = _b;
  x11.depth     = x11depth;
  x11.wclass    = InputOutput;
  x11.visual    = x11visual;
  x11.valuemask = mask;
  x11.attributes= &attr;
  createX11Window(&x11);

  // create the window
  x11window = XCreateWindow(
    x11.display,
    x11.parent,
    x11.x, x11.y, x11.width, x11.height, x11.border,
    x11.depth, x11.wclass, x11.visual,
    x11.valuemask, x11.attributes
  );

  // save TWindow in Xlib window context (it's a hash table)
  if(XSaveContext(x11display, x11window, nClassContext, (XPointer)this))  {
    cerr << "toad: XSaveContext failed\n";
    exit(1);
  }
#endif

#ifdef __XCB__
  uint32_t mask = 0;
  size_t cnt=0;
  uint32_t values[16];

  // XCB_CW_BACK_PIXMAP
  // XCB_CW_BACK_PIXEL
  // XCB_CW_BORDER_PIXMAP
  // XCB_CW_BORDER_PIXEL
  // XCB_CW_BIT_GRAVITY
  // XCB_CW_WIN_GRAVITY
  // XCB_CW_BACKING_STORE
  if (bBackingStore) {
    mask |= XCB_CW_BACKING_STORE
    values[cnt++] = 
  }
  // XCB_CW_BACKING_PLANES
  // XCB_CW_BACKING_PIXEL
  // XCB_CW_OVERRIDE_REDIRECT
  // XCB_CW_SAVE_UNDER
  if (bSaveUnder) {
    mask | = XCB_CW_SAVE_UNDER;
    values[cnt++] = XCB_NONE;
  }
  // XCB_CW_EVENT_MASK
  // XCB_CW_DONT_PROPAGATE
  // XCB_CW_COLORMAP
  // XCB_CW_CURSOR

  xcbWindow = xcb_generate_id(xcbConnection);
  xcbWindow = xcb_create_window(
    xcbConnection,
    XCB_COPY_FROM_PARENT, // depth
    xcbWindow,
    (getParent()&&!flagShell) ? getParent()->xcbWindow : screen->root,
    x, y, w, h, _b,
    XCB_WINDOW_CLASS_INPUT_OUTPUT,
    mask,
    values
  );
#endif

#ifdef __COCOA__
cout << "create cocoa window '" << getTitle() << "'\n";
cout << "NSMakeRect("<<x<<", "<<y<<", "<<w<<", "<<h<<")\n";
  nsview = [[toadView alloc] initWithFrame: NSMakeRect(x,y,w,h)];
  nsview->twindow = this;
  if (getParent() && !flagShell && !flagPopup) {
    [getParent()->nsview addSubview: nsview];   
  } else {
    nswindow = [toadWindow alloc];
    nswindow->twindow = this;
    unsigned int styleMask = 0;
    if (!flagPopup) {
      styleMask |= NSTitledWindowMask
                | NSMiniaturizableWindowMask
                | NSClosableWindowMask
                | NSResizableWindowMask;
    } else {
      styleMask |= NSBorderlessWindowMask;
    }
    [nswindow initWithContentRect: NSMakeRect(x, y, w, h)
         styleMask: styleMask
         backing: NSBackingStoreBuffered
         defer: NO];
    
    [nswindow setTitle: [NSString stringWithUTF8String: getTitle().c_str()]];
    [nswindow setContentView: nsview];
    [nswindow makeKeyAndOrderFront: nil];
    [nswindow setAcceptsMouseMovedEvents: true];
  }
  // we must create the tracking window after the view was added to it's
  // parent, otherwise tracking does not work
  [nsview initTrackAll: NSMakeRect(0,0,w,h)];
#endif

#ifdef __WIN32__
  DWORD style = ( getParent() && !flagShell ) ? 
    (WS_CHILD|WS_BORDER|WS_VISIBLE) : 
    WS_OVERLAPPEDWINDOW;
    
  RECT rect;
  rect.left = x;
  rect.top  = y;
  rect.right = x+w-1 + _b*2;
  rect.bottom = y+h-1 + _b*2;
  if ( !getParent() || flagShell ) {
    ::AdjustWindowRect(&rect, style, false);
  }
#if 0
cerr << "w32createwindow: " << getTitle() << " " << x << "," << y << "," << w << "," << h << " -> "
     << (rect.right - rect.left + 1) << "," << (rect.bottom - rect.top + 1)
     << endl;
#endif
  w32window = ::CreateWindow(
    "TOAD:BASE",
    getTitle().c_str(),
    style,
    ( getParent() && !flagShell ) ? x : CW_USEDEFAULT , y,
    rect.right - rect.left + 1, rect.bottom - rect.top + 1,
    ( getParent() && !flagShell ) ? getParent()->w32window : 0,
    NULL,
    w32instance,
    NULL
  );
  ::SetWindowLong(w32window, 0, (LONG)this);
#endif
  
  focusNewWindow(this); // inform focus management about the new window

  TWindow *twMainWindow = this;
  while(true) {
    TWindow *next = twMainWindow->getParent();
    if (next)
      twMainWindow = next;
    else
      break;
  }

#ifdef __X11__
  // set additional WM parameters for top level windows
  //----------------------------------------------------
  if (flagShell) {
    XSizeHints xsizehints;
    xsizehints.flags = 0;


    // tell the WM that we want to destroy the window ourself
    XSetWMProtocols(x11display, x11window, &xaWMDeleteWindow, 1);
    // XSetWMProtocols(x11display, x11window, &xaWMSaveYourself, 1);

    // tell the WM which title to use
    XTextProperty tp;
    const char *p1 = title.c_str();
    const char **p2 = &p1;
    if (!XStringListToTextProperty((char**)p2, 1, &tp)) {
      cerr << "toad: internal error; XStringListToTextProperty failed\n";
    } else {
      XSetWMName(x11display, x11window, &tp);
      XFree(tp.value);
    }

    // set for X Session Manager (xsm)
    XSetCommand(x11display, x11window, argv, argc);
    const char *host = "localhost";
    XStringListToTextProperty(const_cast<char **>(&host), 1, &tp);
    XSetWMClientMachine(x11display, x11window, &tp);

    // all windows which are not the mainwindow are transient, e.g. dialogs
    
    if (twMainWindow && this!=twMainWindow) {
      XSetTransientForHint(x11display, x11window, twMainWindow->x11window);
    }

    if (!flag_position_undefined) {
      xsizehints.flags|=PPosition;
      xsizehints.x = (int)x;
      xsizehints.y = (int)y;
    }

    XWMHints *wh;
    if ( (wh=XAllocWMHints())==NULL) {
      cerr << "toad: internal error; XAllocWMHints failed (out of memory)\n";
    } else {
      // icon
      TWndBmpList::iterator p = iconlist.find(this);
      if (p!=iconlist.end()) {
        (*p).second->update();
        wh->flags |= IconPixmapHint;
        wh->icon_pixmap = (*p).second->pixmap;
      }

      // group hint
      if (twMainWindow) {   
        wh->flags |= WindowGroupHint;
        wh->window_group = twMainWindow->x11window;
      }
      
      XSetWMHints(x11display, x11window, wh);
      XFree(wh);
    }
    
    DnDNewShellWindow(this);
  
    if (bStaticFrame /* && !(bDialogEditRequest && TDialogEditor::running ) */) {
      // set standard X11 Window Manager Hints
      //---------------------------------------
      xsizehints.flags |= PMinSize | PMaxSize;
      xsizehints.min_width = xsizehints.max_width = (int)w;
      xsizehints.min_height= xsizehints.max_height= (int)h;
  
      PropMotifWmHints motif_hints;
      motif_hints.flags = MWM_HINTS_FUNCTIONS|MWM_HINTS_DECORATIONS|MWM_HINTS_INPUT_MODE;
      motif_hints.decorations=MWM_DECOR_BORDER|MWM_DECOR_TITLE;
      if (!bNoMenu)
        motif_hints.decorations |= MWM_DECOR_MENU;
      motif_hints.functions=MWM_FUNC_MOVE|MWM_FUNC_CLOSE ;
      motif_hints.inputMode = MWM_INPUT_MODELESS;
      motif_hints.status = 0;
      XChangeProperty (
        x11display,
        x11window,
        xaWMMotifHints,                             // Atom for "_MOTIF_WM_HINTS"
        xaWMMotifHints,                             // type
        32,                                         // format (32 bit quantities)
        PropModeReplace,                            // mode
        reinterpret_cast<unsigned char*>(&motif_hints), // data
        PROP_MOTIF_WM_HINTS_ELEMENTS                // nelements
      );
    }

    if (xsizehints.flags!=0)
      XSetWMSizeHints(x11display, x11window, &xsizehints, XA_WM_NORMAL_HINTS);
  } // end of `if (flagShell)'
#endif

  if (layout)
    layout->arrange();

  // create children
  //-----------------
  TInteractor *ptr = getFirstChild();
  while(ptr) {
    ptr->_interactor_create();
    ptr=getNextSibling(ptr);
  }

  if (_bResizedBeforeCreate) {
    doResize();
    _bResizedBeforeCreate = false;
  }
  created();
  _childNotify(TCHILD_CREATE);

  if (_visible) {
#ifdef __X11__
    XMapRaised(x11display, x11window);
    // adjust position for some window managers (ie. for fvwm2: the difference
    // between the, upper-left frame corner and the upper-left corner of our
    // window)
    if (flagShell && !flag_position_undefined)
       XMoveWindow(x11display, x11window, (int)x, (int)y);
#endif

#ifdef __WIN32__
    ::ShowWindow(w32window, w32cmdshow);
    ::UpdateWindow(w32window);
#endif
  }
}

/**
 * Destroy and remove all children that have been added since createWindow().
 */
void 
TWindow::destroyWindow()
{
  if (!isRealized())
    return;

#if 0
  // this is a situation where it would be nice to add something like
  // a window event listener as in Java AWT/Swing, so we could remove this
  // overspecialized code fragment:
  if (TDialogEditor::getEditWindow()==this) {
    TDialogEditor::setEditWindow(0);
  }
#endif
  
  _destroy();
  
  endModalLoop(this);

  if (getParent())
    return;

  // stop the message loop, when there are no more X11 windows
  for(TVectorParentless::iterator p = parentless.begin();
      p != parentless.end();
      ++p)
  {
    if (!(*p)->bParentlessAssistant && (*p)->isRealized()) {
      return;
    }
  }
  postQuitMessage(0);
}

/**
 * Destroy the X11 window associated with the TWindow object.
 */
void 
TWindow::_destroy()
{
  ENTRYEXIT("TWindow::_destroy");

#ifdef __X11__
  if(!x11window)
    return;
#endif

#ifdef __WIN32__
  if (!w32window)
    return;
#endif
    
  // take care of pointer in TOADBase
  if (this==TOADBase::wndTopPopup)
    ungrabMouse();
    
  TWindowEvent we;
  we.type = TWindowEvent::DESTROY;
  we.window = this;
  if (toad::global_evt_filter) {
    TEventFilter * p = toad::global_evt_filter;
    while(p) {
      if (p->windowEvent(we))
        break;
      p = p->next;
    }
  }
  windowEvent(we);
  _childNotify(TCHILD_DESTROY);
  if (getFirstChild()) {
    TInteractor *ptr = getFirstChild();
    while(ptr) {
      TInteractor *next=getNextSibling(ptr);
      TWindow *wnd = dynamic_cast<TWindow*>(ptr);
      if (wnd) {
        wnd->_destroy();
        if (!wnd->before_create)
          delete wnd;
      }
      ptr=next;
    }
  }
  removeMessage(this);

#ifdef __X11__
  XSaveContext(x11display, x11window, nClassContext, (XPointer)0);
  XDestroyWindow(x11display, x11window);
  x11window = 0;
#endif

#ifdef __COCOA__
  [nsview setHidden: true];
  [nsview removeFromSuperview];
  nsview = nil;
  
  if (!nswindow)
    return;
  [nswindow close];
  nswindow = nil;  
#endif

#ifdef __WIN32__
  ::DestroyWindow(w32window);
  w32window = 0;
#endif
  // take care of pointer in TFocusManager
  //---------------------------------------
  focusDelWindow(this);
}

/**
 * Set the icon to be used when the window is iconized.
 */
void 
TWindow::setIcon(TBitmap* bmp)
{
  TWndBmpList::iterator p = iconlist.find(this);
  if (p!=iconlist.end()) {
    delete (*p).second;
    (*p).second = bmp;
  } else {
    iconlist[this]=bmp;
  }
}

// Raise & Lower Window
//----------------------------------------------------------------------------
void 
TWindow::raiseWindow()
{
  CHECK_REALIZED("RaiseWindow");
#ifdef __X11__
  XRaiseWindow(x11display, x11window);
#endif

#ifdef __COCOA__
  if (!nswindow)
    return;
  [nswindow orderFront: nswindow];
#endif
  
#ifdef __WIN32__
#endif
}

void 
TWindow::lowerWindow()
{
  CHECK_REALIZED("LowerWindow");
  #ifdef __X11__
  XLowerWindow(x11display, x11window);
  #endif
  
  #ifdef __WIN32__
  #endif
}

#ifdef __X11__
// Paint Queue
//----------------------------------------------------------------------------

static queue<TWindow::TPaintRegion*> paint_region_queue;

//! Static method returning `true' when there are event in the paint queue.
//---------------------------------------------------------------------------
bool 
TWindow::_havePaintEvents()
{
  if (lock_paint_queue)
    return false;
  return !paint_region_queue.empty();
}

/**
 * Static method to fetch and handle one paint event in the paint queue
 * and to call a windows 'paint' method.
 */
void 
TWindow::_dispatchPaintEvent()
{
  THREAD_LOCK(mutexPaintQueue);
  if (paint_region_queue.empty()) {
    THREAD_UNLOCK(mutexPaintQueue);
    return;
  }
    
  TPaintRegion *rgn = paint_region_queue.front();
  paint_region_queue.pop();
//  THREAD_UNLOCK(mutexPaintQueue);
  
  if (rgn->wnd) { // when the region is still valid...
    if (rgn->wnd->x11window)  { // .. and the window is still valid:
      TRectangle wrect(0, 0, rgn->wnd->w, rgn->wnd->h);
      
      // clip update region to window (needed after scrolling)
      (*rgn) &= wrect;
      
      // clear the background
      if (rgn->wnd->bEraseBe4Paint &&   // do we have to?
          !rgn->wnd->bNoBackground &&
          !rgn->wnd->bDoubleBuffer)
      {
        long n = rgn->getNumRects();
        TRectangle r;
        for(long i=0; i<n; i++) {
          rgn->getRect(i, &r);
          XClearArea(x11display, rgn->wnd->x11window, 
                     (int)r.x, (int)r.y, (unsigned)r.w, (unsigned)r.h, false);
        }
      }
      if (rgn->wnd->layout) {
        rgn->wnd->layout->paint();
      }
      rgn->wnd->paint();
    }
    rgn->wnd->bEraseBe4Paint = false;
    rgn->wnd->paint_rgn = NULL;
  }
  delete rgn;
  THREAD_UNLOCK(mutexPaintQueue);
}


/**
 * See that `TRegion *paint_rgn' contains a region.
 */
void
TWindow::_providePaintRgn()
{
  if (paint_rgn==NULL) {      // make sure window has a paint region
//printf("TWindow: creating paint region for %lx\n",(long)this);
    paint_rgn = new TPaintRegion;
    paint_rgn->wnd = this;
    paint_region_queue.push(paint_rgn);
  }
}
#endif

/** 
 * Update the whole invalidated window region right now. 
 * The normal behaviour is to wait until no other events than paint events
 * are left in the message queue.
 */
void
TWindow::paintNow()
{
#ifdef __X11__
//  cout << "void TWindow::PaintNow()" << endl;
THREAD_LOCK(mutexPaintQueue);
  if (paint_rgn) {
    if (bEraseBe4Paint &&
        !bNoBackground &&
        !bDoubleBuffer) {
      long n = paint_rgn->getNumRects();
      TRectangle r;
      for(long i=0; i<n; i++) {
        paint_rgn->getRect(i, &r);
        XClearArea(x11display, x11window, (int)r.x,(int)r.y,(unsigned)r.w,(unsigned)r.h, false);
      }
    }
    bEraseBe4Paint = false;
    
#if 0
    if (TDialogEditor::running && 
        TDialogEditor::enabled &&
        bDialogEditRequest &&
        TDialogEditor::getEditWindow()==this ) 
    {
      TDialogEditor::getDialogEditor()->paint();
    } else {
      paint();
    }
#else
    if (layout) {
      layout->paint();
    }
    paint();
#endif
    flush();
    paint_rgn->wnd = NULL;
    paint_rgn = NULL;
  }
THREAD_UNLOCK(mutexPaintQueue);
#endif
}

/**
 * Return the invalidated region of the window.
 */
TRegion* 
TWindow::getUpdateRegion() const
{
  return paint_rgn;
}

/**
 * Invalidate an area of the window. 
 *
 * This will generate a paint event when
 * all other events in the message queue have been processed. Multiple calls
 * to 'Invalidate' sum up into a single paint event. Before 'paint' is called,
 * the background of the invalidated area is cleared with the current 
 * background color.
 */
void 
TWindow::invalidateWindow(bool clear)
{
#ifdef __X11__
  _providePaintRgn();
  paint_rgn->addRect(0,0,w,h);
  bEraseBe4Paint |= clear;
#endif

#ifdef __COCOA__
  if (nsview) {
    [nsview setNeedsDisplay: YES];
  }
#endif

#ifdef __WIN32__
  RECT rect;
  rect.left = 0;
  rect.right = w;
  rect.top = 0;
  rect.bottom = h;
  ::InvalidateRect(w32window, &rect, clear);
#endif
}

void 
TWindow::invalidateWindow(TCoord x,TCoord y,TCoord w,TCoord h, bool clear)
{
#ifdef __X11__
  _providePaintRgn();
  paint_rgn->addRect(x,y,w,h);
  bEraseBe4Paint |= clear;
#endif

#ifdef __COCOA__
  invalidateWindow(clear);
#endif

#ifdef __WIN32__
  RECT rect;
  rect.left = x;
  rect.right = x+w-1;
  rect.top = y;
  rect.bottom = y+h-1;
  ::InvalidateRect(w32window, &rect, clear);
#endif
}

void 
TWindow::invalidateWindow(const TRectangle &r, bool clear)
{
#ifdef __X11__
  _providePaintRgn();
  (*paint_rgn)|=r;
  bEraseBe4Paint |= clear;
#endif

#ifdef __COCOA__
  invalidateWindow(clear);
#endif

#ifdef __WIN32__
  RECT rect;
  rect.left = r.x;
  rect.right = r.x+r.w-1;
  rect.top = r.y;
  rect.bottom = r.y+r.h-1;
  ::InvalidateRect(w32window, &rect, clear);
#endif
}

void 
TWindow::invalidateWindow(const TRegion &r, bool clear)
{
#ifdef __X11__
  _providePaintRgn();
  (*paint_rgn)|=r;
  bEraseBe4Paint |= clear;
#endif


#ifdef __COCOA__
  invalidateWindow(clear);
#endif

#ifdef __WIN32__
  invalidateWindow(clear);
#endif
}

// ScrollWindow
//-------------------------------------------------------------------

#define SCROLL_WITH_SERVER_GRAB

#ifdef __X11__
static Bool CheckEvent(Display*, XEvent *event, char *window)
{
  if (event->xany.window==reinterpret_cast<Window>(window) && 
      ( event->type == Expose || event->type == GraphicsExpose) )
    return True;
  return False; 
}
#endif

/**
 * Scroll window contents.
 * <TABLE BORDER=1 NOSHADE>
 *   <TR><TD>dy&gt;0</TD><TD>down</TD><TR>
 *   <TR><TD>dy&lt;0</TD><TD>up</TD><TR>
 *   <TR><TD>dx&gt;0</TD><TD>right</TD><TR>
 *   <TR><TD>dx&lt;0</TD><TD>left</TD><TR>
 * </TABLE>
 * Should be done asynchronusly for fewer and faster screen updates.
 */
void
TWindow::scrollWindow(TCoord dx, TCoord dy, bool clear)
{
#ifdef __X11__
  if (!x11window || (dx==0 && dy==0)) 
    return;

  if (abs(dx)>=w || abs(dy)>=h) {
    invalidateWindow(clear);
    return;
  }

  #ifdef SCROLL_WITH_SERVER_GRAB
  XGrabServer(x11display);
  #endif

  // move paint events from the queue to the update region
  //-------------------------------------------------------
  XSync(x11display, False);
  XEvent event;
  while( XCheckIfEvent(x11display, &event, CheckEvent, (char*)x11window ) )
    invalidateWindow(event.xexpose.x, event.xexpose.y, event.xexpose.width, event.xexpose.height);

  // move update region
  //--------------------
  THREAD_LOCK(mutexPaintQueue);
  if (paint_rgn)
    paint_rgn->translate(dx,dy);
  THREAD_UNLOCK(mutexPaintQueue);

  // scroll the windows contents
  //-----------------------------
  XCopyArea(x11display, x11window, x11window, x11gc, 0,0, w, h, dx,dy);
  
  // decide which parts of the window must be redrawn
  //-------------------------------------------------------------
  if (dy>0) // scroll down, clear top
    invalidateWindow(0,0, w, dy+1, clear);
  else if (dy<0)  // scroll up, clear bottom
    invalidateWindow(0,h+dy, w, -dy, clear);

  if (dx>0) // scroll right, clear left
    invalidateWindow(0,0, dx+1, h, clear);
  else if (dx<0)  // scroll left, clear right
    invalidateWindow(w+dx, 0, -dx, h, clear);

  #ifdef SCROLL_WITH_SERVER_GRAB
  XUngrabServer(x11display);
  #endif
#endif

#ifdef __WIN32__
  ::ScrollWindow(w32window, dx, dy, NULL, NULL);
#endif
}

/**
 * Scroll area within the given rectangle.
 * No scrolling occures, when <VAR>dx</VAR> or <VAR>dy</VAR> are &gt;= the size
 * of the rectangle.
 * should be done asynchronly for fewer and faster screen updates
 */
void
TWindow::scrollRectangle(const TRectangle &r, TCoord dx, TCoord dy, bool clear)
{
#ifdef __X11__
  if (!x11window || (dx==0 && dy==0)) 
    return;

  if (abs(dx)>=r.w || abs(dy)>=r.h) {
    invalidateWindow(clear);
    return;
  }

//cerr << "scroll rectangle " << r << " by " << dx << ", " << dy << endl;

  #ifdef SCROLL_WITH_SERVER_GRAB
  XGrabServer(x11display);
  #endif

  // move paint events from the queue to the update region
  //-------------------------------------------------------
  XSync(x11display, False);
  XEvent event;
  while( XCheckIfEvent(x11display, &event, CheckEvent, (char*)x11window ) )
    invalidateWindow(event.xexpose.x,event.xexpose.y,event.xexpose.width,event.xexpose.height);

  // move rectangle within update region
  //-------------------------------------
  THREAD_LOCK(mutexPaintQueue);
  if (paint_rgn) {
    TRegion r1;
    r1|=*paint_rgn;
    r1.translate(dx,dy);
    r1&=r;
    *paint_rgn-=r;
    *paint_rgn|=r1;
  }
  THREAD_UNLOCK(mutexPaintQueue);

  // scroll the windows contents
  //-----------------------------
  TPoint s,d;
  TCoord xs,ys;

  if (dy>0) {
    s.y = r.y;
    d.y = r.y+dy;
    ys  = r.h - dy;
  } else if (dy<0)
  {
    s.y = r.y-dy;
    d.y = r.y;
    ys  = r.h + dy;
  } else {
    s.y = r.y;
    d.y = r.y;
    ys  = r.h;
  }

  if (dx>0) {
    s.x = r.x;
    d.x = r.x+dx;
    xs  = r.w - dx;
  } else if (dx<0) {
    s.x = r.x-dx;
    d.x = r.x;
    xs  = r.w + dx;
  } else {
    s.x = r.x;
    d.x = r.x;
    xs  = r.w;
  }

  XCopyArea(x11display, x11window, x11window, x11gc, s.x,s.y, xs, ys, d.x,d.y);

//cerr << "  copy area " << s.x << ", " << s.y << ", " << xs << ", " << ys << " to " << d.x << ", " << d.y << endl;
  
  // decide which parts of the window must be redrawn
  //-------------------------------------------------------------
  if (dy>0) // scroll down, clear top
    invalidateWindow(r.x, r.y, r.w, dy+1, clear);
  else if (dy<0) // scroll up, clear bottom
    invalidateWindow(r.x,r.y+r.h+dy, r.w, -dy, clear);

  if (dx>0) // scroll right, clear left
    invalidateWindow(r.x, r.y, dx+1, r.h, clear);
  else if (dx<0)  // scroll left, clear right
    invalidateWindow(r.x+r.w+dx, r.y, -dx, r.h, clear);

  #ifdef SCROLL_WITH_SERVER_GRAB
  XUngrabServer(x11display);
  #endif
#endif

#ifdef __WIN32__
  RECT wr;
  wr.left  = r.x;
  wr.right = r.x+r.w-1;
  wr.top   = r.y;
  wr.bottom= r.y+r.h-1;
  ::ScrollWindow(w32window, dx, dy, &wr, &wr);
#endif

}

/**
 * Requests the keyboard focus for the window.
 *
 * \return 'true' when the window got the focus.
 */
bool
TWindow::setFocus()
{
  setFocusWindow(this);
  return getFocusWindow()==this;
}

// call `focus' for `p' and all of its children

void
TWindow::_setFocusHelper(TInteractor *parent, bool b)
{
  parent->focus(b);
  TInteractor *p = parent->getFirstChild();
  while(p) {
    p->focus(b);
    p = p->getNextSibling();
  }
}

/**
 * Toggles the <VAR>_bOwnsFocus</VAR> flag.
 */
void
TWindow::_setFocus(bool b)
{
  if (b!=_bOwnsFocus) {
    _bOwnsFocus = b;
    _setFocusHelper(this, b);
    if (getParent())
      getParent()->childNotify(this, TCHILD_FOCUS);
  }
}

/** 
 * Delivers `true' when the window owns the keyboard focus or when the
 * window is an active focus manager.
 */
bool
TWindow::isFocus() const
{
  return (_bOwnsFocus);
}


/**
 * Called when the X11 window manager request to close the window.
 *
 * Default action is to call 'destroyWindow'.
 */
void
TWindow::closeRequest()
{
  destroyWindow();
}

/*---------------------------------------------------------------------------*
 | functions for dummy message handling that do (almost) nothing             |
 *---------------------------------------------------------------------------*/
/**
 * Called buttom-up before window creation. Default action is to call
 * resize().
 *
 * This method gives the opportunity to setup the window before it's creation.
 */
void
TWindow::adjust()
{
  resize();
}
 
void
TWindow::doResize()
{
  if (flag_wm_resize)
    return;
  flag_wm_resize = true;
  if (layout)
    layout->arrange();
  resize();
  if (getParent()) {
     _childNotify(TCHILD_RESIZE);
  }
  flag_wm_resize = false;
}

/**
 * Called when a window is resized.
 */
void TWindow::resize(){}

/**
 * Notify the parent window of a modification in the child window.
 *
 * <UL>
 *   <LI>TCHILD_TITLE
 *   <LI>TCHILD_POSITION
 *   <LI>TCHILD_RESIZE
 *   <LI>TCHILD_ADD
 *   <LI>TCHILD_REMOVE
 *   <LI>TCHILD_CREATE
 *   <LI>TCHILD_DESTROY
 *   <LI>TCHILD_ICON
 *   <LI>TCHILD_FOCUS
 *   <LI>TCHILD_DIALOG_EDIT_REQUEST
 *   <LI>TCHILD_BEFORE_ADD
 *   <LI>TCHILD_BEFORE_CREATE
 * </UL>
 */
void TWindow::childNotify(TWindow*, EChildNotify){}

void TWindow::destroy(){}

/**
 * Called when parts of the window must be redrawn.
 *
 * \sa TPen, TPencil
 */
void TWindow::paint(){}

//! Called after the window was created.
void TWindow::created(){}

/**
 * One of two ways to initialize a window.
 *  
 *  The are two ways to initialize a window:
 *  <UL>
 *    <LI> in the constructor, which is usually the best way
 *    <LI> in <I>create()</I>
 *  </UL>
 *  <I>create()</I> is invoked after a call to <I>createWindow()</I> or <I>runApp()</I>,
 *  right before the actual window is mapped on the desktop.
 *  <P>
 *  When you have a window needing some sophisticated configuration
 *  and you've decided not to do it all in the constructor but after you've 
 *  created the object, then you might need a method being called after this
 *  configuration is done and before the window will be created.<BR>
 *  And this method is <I>create()</I>.
 *  <P>
 *  When you've decided to use <I>create()</I> you will have to keep some things
 *  in mind:
 *  <UL>
 *    <LI> 
 *      Child windows created after <I>createWindow()</I> will be removed during
 *      <I>destroyWindow()</I>.
 *    <LI>
 *      Other objects than child windows you've created during <I>create()</I>
 *      should be removed during <I>destroy()</I>, otherwise multiple
 *      <I>createWindow()</I>, <I>destroyWindow()</I> would result in memory leaks.<BR>
 *      Another good thing you can do is to set pointers to child windows to
 *      <CODE>NULL</CODE> during <I>destroy</I>. This won't modify the
 *      behaviour of your program but it's easier to see during debugging
 *      that a window is actually gone.
 *  </UL>
 *  Otherwise you will get into another fine mess.
 */
void TWindow::create(){}

/**
 * \defgroup directx DirectX
 *
 * DirectX is a set of methods and structures to call X11 directly.
 * Microsoft had another idea what DirectX means but it seems they
 * don't sell X anymore. ;)
 */

#ifdef __X11__
/**
 * @ingroup directx
 *
 * This method is called after <I>create()</I> right before the X11 window
 * is created and gives you a chance to modifiy X11 specific window parameters.
 * <P>
 * It was originally introduced to support OpenGL support on SGI machines.
 */
void TWindow::createX11Window(TX11CreateWindow*){}

/**
 * @ingroup directx
 *
 * This method is called before an event is dispatched to one of the
 * windows methods. In case you have to handle X11 events on your own,
 * you can access the current XEvent in <I>TOADBase::x11event</I>.
 * <P>
 * The default implementation does nothing.
 */
void TWindow::handleX11Event(){}
#endif

/**
 * Key pressed.
 *
 *  Windows which implement this method are capable to receive the 
 *  keyboard focus. And when they own it, after calling SetFocus() or
 *  when the keyboard traversal mechanism reached the window, keyboard
 *  events arrive here.
 *  <P>
 *  Predefined values for TKey are
 *  <UL>
 *    <LI>TK_BACKSPACE
 *    <LI>TK_TAB
 *    <LI>TK_LEFT_TAB
 *    <LI>TK_LINEFEED
 *    <LI>TK_CLEAR
 *    <LI>TK_RETURN
 *    <LI>TK_PAUSE
 *    <LI>TK_SCROLL_LOCK
 *    <LI>TK_SYS_REQ
 *    <LI>TK_ESCAPE
 *    <LI>TK_DELETE
 *    <LI>TK_SPACE
 *    <LI>TK_HOME
 *    <LI>TK_LEFT
 *    <LI>TK_UP
 *    <LI>TK_RIGHT
 *    <LI>TK_DOWN
 *    <LI>TK_PAGEUP
 *    <LI>TK_PAGEDOWN
 *    <LI>TK_END
 *    <LI>TK_BEGIN
 *    <LI>TK_F1 to TK_F20
 *  </UL>
 */

void
TWindow::keyEvent(const TKeyEvent &ke)
{
  switch(ke.type) {
    case TKeyEvent::DOWN:
      keyDown(ke);
      break;
    case TKeyEvent::UP:
      keyUp(ke);
      break;
  }
}

void TWindow::keyDown(const TKeyEvent&){}
void TWindow::keyUp(const TKeyEvent&){}

void
TWindow::mouseEvent(const TMouseEvent &me)
{
  if (layout) {
    TMouseEvent me0(me);
    if (layout->mouseEvent(me0))
      return;
  }

  switch(me.type) {
    case TMouseEvent::MOVE:
      mouseMove(me);
      break;
    case TMouseEvent::ENTER:
      mouseEnter(me);
      break;
    case TMouseEvent::LEAVE:
      mouseLeave(me);
      break;
    case TMouseEvent::LDOWN:
      mouseLDown(me);
      break;
    case TMouseEvent::MDOWN:
      mouseMDown(me);
      break;
    case TMouseEvent::RDOWN:
      mouseRDown(me);
      break;
    case TMouseEvent::LUP:
      mouseLUp(me);
      break;
    case TMouseEvent::MUP:
      mouseMUp(me);
      break;
    case TMouseEvent::RUP:
      mouseRUp(me);
      break;
  }
}

//! See `SetMouseMoveMessages' when you need mouseMove.
void TWindow::mouseMove(const TMouseEvent&){}
void TWindow::mouseEnter(const TMouseEvent&){}
void TWindow::mouseLeave(const TMouseEvent&){}

//! Called when the left mouse button is pressed. Since X11 performs an
//! automatic mouse grab you will receive a mouseLUp message afterwards
//! unless you call UngrabMouse().
void TWindow::mouseLDown(const TMouseEvent&){}
//! Same as mouseLDown for the middle mouse button.
void TWindow::mouseMDown(const TMouseEvent&){}
//! Same as mouseLDown for the right mouse button.
void TWindow::mouseRDown(const TMouseEvent&){}
void TWindow::mouseLUp(const TMouseEvent&){}
void TWindow::mouseMUp(const TMouseEvent&){}
void TWindow::mouseRUp(const TMouseEvent&){}

void
TWindow::windowEvent(const TWindowEvent &we)
{
  switch(we.type) {
    case TWindowEvent::NEW:
      break;
    case TWindowEvent::DELETE:
      break;
    case TWindowEvent::CREATE:
      create();
    case TWindowEvent::CREATED:
      created();
      break;
    case TWindowEvent::DESTROY:
      destroy();
      break;
    case TWindowEvent::MAPPED:
      break;
    case TWindowEvent::UNMAPPED:
      break;
    case TWindowEvent::PAINT:
      paint();
      break;
    case TWindowEvent::ADJUST:
      adjust();
      break;
    case TWindowEvent::RESIZE:
      doResize();
      break;
    case TWindowEvent::FOCUS:
      focus(isFocus());
      break;
  }
}


//! The window manager send a `save yourself'-message to the window.
void TWindow::saveYourself(){printf("saveYourself\n");}

/**
 * Set the size of the window.
 */
void 
TWindow::setSize(TCoord w, TCoord h)
{
  assert(this!=NULL);
  if (w==TSIZE_PREVIOUS) w=this->w;
  if (h==TSIZE_PREVIOUS) h=this->h;

  if (w<=0 || h<=0) {
    // we should unmap the window instead...
    if (w<=0) w=1;
    if (h<=0) h=1;
  }

  if (this->w==w && this->h==h)
    return;

  this->w=w;
  this->h=h;

#ifdef __X11__  
  if (x11window) {
    if (bStaticFrame) {
      XSizeHints sh;
      sh.flags = PMinSize | PMaxSize;
      sh.min_width = sh.max_width = w;
      sh.min_height= sh.max_height= h;
      XSetWMSizeHints(x11display, x11window, &sh, XA_WM_NORMAL_HINTS);
    }
    XResizeWindow(x11display, x11window ,w,h);
  } else {
    _bResizedBeforeCreate = true;
    return;
  }
#endif

#ifdef __COCOA__
  if (nsview) {
    NSRect nr = NSMakeRect(x, y, w, h);
    [nsview setFrame: nr];
  }
#endif

#ifdef __WIN32__
  if (w32window) {
    ::InvalidateRect(w32window, NULL, TRUE);

  DWORD style = ( getParent() && !flagShell ) ? 
    (WS_CHILD|WS_BORDER|WS_VISIBLE) : 
    WS_OVERLAPPEDWINDOW;
    
  RECT rect;
  rect.left = y;
  rect.top  = x;
  rect.right = x+w-1+_b*2;
  rect.bottom = y+h-1+_b*2;
  ::AdjustWindowRect(&rect, style, false);

    ::MoveWindow(w32window, x, y, rect.right - rect.left+1, rect.bottom - rect.top+1, FALSE);
  } else {
    _bResizedBeforeCreate = true;
    return;
  }
#endif

  // `flag_wm_resize' is true when this method was called
  // as consquence of `_childNotify(TCHILD_RESIZE)' below so before
  // spending the rest of our life with infinite recursion we return
  // at once
  // but maybe i can remove this check because 'flag_child_notify'
  // serves a similar purpose
  //-----------------------------------------------------------------
  if ( isSuppressMessages() || flag_wm_resize ) {
#warning "try to remove not calling resize() and parents childNotify()"
//    cout << "don't calling resize() and parents childNotify()" << endl;
    return;
  }
  doResize();
}

/**
 * Notify parent of a modified child by calling it's
 * `childNotify' method.
 */
void TWindow::_childNotify(TWindow::EChildNotify type)
{
  TWindow *p = getParent();
  
  if(p && !flag_suppress_msg) {
    if (!flag_child_notify) {
      flag_child_notify = true;
      p->childNotify(this,type);
      flag_child_notify = false;
    }
  }
}

/**
 * Show or hide the window.
 */
void TWindow::setMapped(bool b)
{
  if (_visible == b)
    return;
  _visible = b;
#ifdef __X11__
  if (!x11window)
    return;
  if (b) {
    XMapWindow(x11display, x11window);
  } else {
    XUnmapWindow(x11display, x11window);
  }
#endif

#ifdef __COCOA__
  if (!nsview)
    return;   
  if (_mapped==b)
    return;
  _mapped = b;
  [nsview setHidden: !b];
  if (!nswindow)
    return;
  //[nswindow setOpaque: !b];
  if (!b) {
    [nswindow orderOut: nswindow];
  } else {
    [nswindow orderFront: nswindow];
  }
#endif

#ifdef __WIN32__
  if (!w32window)
    return;
  if (b) {
    ::ShowWindow(w32window, 1);
  } else {
    ::ShowWindow(w32window, 0);
  }
#endif
}

/**
 * Returns `true' when the window is visible.
 */
bool TWindow::isMapped() const
{
#ifdef __X11__
  if (!x11window)
    return false;
#endif

#ifdef __WIN32__
  if (!w32window)
    return false;
#endif
  return _visible;
}

/**
 * Set the origin for all drawing operations.
 */
void TWindow::setOrigin(TCoord dx, TCoord dy)
{
  _dx = dx; _dy = dy;
}

/**
 * Set the origin for all drawing operations and scroll the windows content
 * to the new position.
 *
 * \note
 *   Child windows will not be moved.
 */
void TWindow::scrollTo(TCoord nx, TCoord ny)
{
  TCoord dx = nx - _dx;
  TCoord dy = ny - _dy;
  scrollWindow(dx, dy, true);
  _dx = nx; _dy = ny;
}

/**
 * Set the title of the window.
 * \sa getTitle
 */
void TWindow::setTitle(const string &title)
{
  this->title=title;

#ifdef __X11__
  if (x11window) {
    if (flagShell) {
      XTextProperty tp;
      const char *p1 = title.c_str();
      const char **p2 = &p1;
      if (!XStringListToTextProperty((char**)p2, 1, &tp)) {
        cerr << "toad: internal error; XStringListToTextProperty failed\n";
      } else {
        XSetWMName(x11display, x11window, &tp);
        XFree(tp.value);
      } 
    }
    invalidateWindow();
  }
#endif
  
  _childNotify(TCHILD_TITLE);
}

void
TWindow::loadLayout(const string &filename)
{
  TLayout * new_layout = NULL;
  try {
    iurlstream url(filename);
    TInObjectStream in(&url);
    TSerializable *s = in.restore();
    if (!s || !in) {
      cerr << "loading layout '" << filename << "' failed " << in.getErrorText() << endl;
    } else {
      new_layout = dynamic_cast<TLayout*>(s);
      if (!new_layout) {
        cerr << "loading layout '" << filename << "' failed: doesn't provide TLayout object, "
             << "  got '" << typeid(*s).name() << "'\n";
        delete s;
      }
    }
  }
  catch(exception &e) {
    cerr << "loading layout '" << filename << "' failed:\ncaught exception: " << e.what() << endl;
  }
  if (new_layout) {
    new_layout->setFilename(filename);
    setLayout(new_layout);
  } else {
    if (layout) {
      TLayout *l = layout;
      l->setFilename(filename);
      layout = 0;
      setLayout(l);
    }
  }
}

/**
 * Set a new layout. The previous layout is deleted.
 *
 * When the new layout doesn't have a filename, the filename of the
 * old layout will be applied to the new layout and be cleared.
 *
 * This way the old layout won't be stored on destruction and the new
 * layout will be stored into the old layouts file.
 */
void
TWindow::setLayout(TLayout *l)
{
  if (layout == l)
    return;
  string oldfilename;
  if (layout) {
    oldfilename = layout->getFilename();
    layout->setFilename("");
    delete layout;
  }
  /**
   * windows without a paint method, don't get paint events, but the
   * layout might wan't 'em so set the required events again
   * todo: only subscribe for paint events, when the layout's paint
   * method is overwritten.
   */
#ifdef __X11__
  if ( x11window && ((layout && !l) || (!layout && l)) )
    XSelectInput(x11display, x11window, _buildEventmask());
#endif
  layout = l;
  if (layout) {
    if (layout->getFilename().size()==0 && oldfilename.size()!=0) {
      layout->setFilename(oldfilename);
    }
    layout->window = this;
  }
  if (isRealized()) {
    layout->arrange();
  }
}

/**
 * Set the left, upper corner of the window inside it's parent.
 *
 * @sa getXPos, getYPos, getShape, setSize, setShape
 */
void
TWindow::setPosition(TCoord x, TCoord y)
{
  assert(this!=NULL);
  flag_position_undefined = false;

  if (x==this->x && y==this->y)
    return;
  this->x = x;
  this->y = y;

#ifdef __X11__
  if (x11window)
    XMoveWindow(x11display, x11window,x,y);
#endif

#ifdef __COCOA__
  if (nsview) {
    NSRect nr = NSMakeRect(x, y, w, h);
    [nsview setFrame: nr];
  }
#endif

#ifdef __WIN32__
    ::InvalidateRect(w32window, NULL, TRUE);
    ::MoveWindow(w32window, x, y, w+(_b<<1), h+(_b<<1), FALSE);
#endif

  _childNotify(TCHILD_POSITION);
}

void
TWindow::getShape(TRectangle *r) const
{
  assert(this!=NULL);
  r->set(x, y, w + (_b<<1), h+ (_b<<1));
} 

/**
 * Changes the location and size of the the window including the windows
 * border.
 */
void 
TWindow::setShape(TCoord x, TCoord y, TCoord w, TCoord h)
{
//cout << getTitle() << ": setShape("<<x<<", "<<y<<", "<<w<<", "<<h<<")\n";
  assert(this!=NULL);

  TCoord w2=w, h2=h;
  if (w==TSIZE_PREVIOUS) w=0;
  if (h==TSIZE_PREVIOUS) h=0;
  TRectangle r(x,y,w,h);
  r.w -= (_b<<1);
  r.h -= (_b<<1);
  if (w2==TSIZE_PREVIOUS) r.w=this->w;
  if (h2==TSIZE_PREVIOUS) r.h=this->h;
#ifndef __COCOA__
  setPosition(r.x, r.y);
  setSize(r.w, r.h);
#else
  this->x = r.x;
  this->y = r.y;
  this->w = r.w;
  this->h = r.h;
  if (nsview) {
    NSRect nr = NSMakeRect(x, y, w, h);
    [nsview setFrame: nr];
  }
#endif
}

void 
TWindow::setBackground(const TRGB &nc)
{
  _bg.set(nc.r,nc.g,nc.b);
#ifdef __X11__
  if(x11window) {
    XSetWindowBackground(x11display, x11window, TColor::_getPixel(_bg));
  }
#else
  invalidateWindow();
#endif
}

/**
 * Select a background bitmap for the window. 
 *
 * @param bmp
 *   The bitmap to be used or NULL, when no bitmap shall be used.
 */
void 
TWindow::setBackground(TBitmap *bmp)
{
  TWndBmpList::iterator p = backgroundlist.find(this);
  if (p!=backgroundlist.end()) {
    delete (*p).second;
    (*p).second = bmp;
  } else {
    backgroundlist[this]=bmp;
  }

#ifdef __X11__
  if (x11window) {
    if (bmp) {
      bmp->update();
      XSetWindowBackgroundPixmap(x11display, x11window, bmp->pixmap);
    } else {
      XSetWindowBackgroundPixmap(x11display, x11window, None);
    }
  }
#endif
}

/**
 * Enables/disables drawing the windows background.
 *
 * In some situations, e.g. double buffering with TBitmap or when you
 * fill the whole window during <I>paint()</I>, this can be helpfull to
 * reduce flicker.
 */
void
TWindow::setHasBackground(bool b)
{
  bNoBackground = !b;

#ifdef __X11__
  if (!x11window)
    return;

  unsigned long mask = 0UL;
  XSetWindowAttributes attr;

  if (!b) {
    attr.background_pixmap = None;
    mask|=CWBackPixmap;
  } else {
    TWndBmpList::iterator p = backgroundlist.find(this);
    if (p!=backgroundlist.end()) {
      mask|=CWBackPixmap;
      (*p).second->update();
      attr.background_pixmap = (*p).second->pixmap;
    } else {
      mask|=CWBackPixel;
      attr.background_pixel = TColor::_getPixel(_bg);
    }
  }
  XChangeWindowAttributes(
    x11display, x11window,
    mask,
    &attr);
#endif
  invalidateWindow();
}

void 
TWindow::clearWindow()
{
#ifdef __X11__
  XClearWindow(x11display, x11window);
#endif
}

/**
 *  Get window coordiantes relative to the root window.
 *
 * (Returns <CODE>(0,0)</CODE>, when the window is not realized.)
 */
void TWindow::getRootPos(TCoord* x, TCoord* y)
{
#ifdef __X11__
  if (!x11window) {
    *x=0;
    *y=0;
  } else {
    Window wnd;
    int ix, iy;
    ix=0;
    iy=0;
    XTranslateCoordinates(x11display,
                          x11window, 
                          DefaultRootWindow(x11display),
                          0,0,&ix,&iy, &wnd);
    *x = ix - _b;
    *y = iy - _b;
  }
#endif
}

#ifdef DEBUG
void TWindow::debug_check_realized(const char *txt)
{
#ifdef __X11__
  if(!x11window)  {
    fprintf(stderr,
      "TOAD: FATAL ERROR\n"
      "      %s needs an existing window.\n",txt);
    if (debug_flags&TWindow::DFLAG_WMINIT)
    {
      fprintf(stderr, "      Occured during create() in window \"%s\".\n",
      title.c_str());
    } else if (debug_flags&TWindow::DFLAG_WMADJUST)
    {
      fprintf(stderr, "      Occured during adjust() in window \"%s\".\n",
      title.c_str());
    }
    fprintf(stderr, "      Occured in window \"%s\".\n"
                    "      But NOT during create() or adjust(). Could be the constructor...\n",
                    title.c_str());
    exit(1);
  }
#endif
}
#endif

/**
 * Grab mouse pointer so we even receive mouse events the the pointer
 * leaves the window.
 *
 * The default behaviour is to send mouse events to the window containing
 * the mouse pointer. After calling `GrabMouse' all mouse events will be
 * send to this window.<BR>
 * Note that X11 performs an automatic grab after each mouse down message.<BR>
 * If a confine_window is not NULL, the pointer is restricted to stay 
 * contained in it. The confine_window need have no relationship to 
 * the grab window.<BR>
 * Be carefull using the grab as it might lock your X11 server.
 *
 * \sa grabPopupMouse, ungrabMouse
 */
void 
TWindow::grabMouse(bool allmousemove, TWindow* confine_window, TCursor::EType cursor)
{
#ifdef __X11__
//  cerr << "grabMouse for window '" << getTitle() << "'\n";

  bSimulatedAutomaticGrab = false;

  // set flags for MouseMoveMessages
  //---------------------------------
  bool omm = _allmousemove;
  setAllMouseMoveEvents(allmousemove);

  // grab the mouse pointer
  //------------------------
  if (XGrabPointer( x11display, x11window,
    False,  // all mouse messages for this window (not it's application)
    _buildMouseEventmask(true) | ButtonReleaseMask,
    /*
        the extra ButtonReleaseMask is needed to end the simulated
        automatic grab
    */
    GrabModeAsync,
    GrabModeAsync,
    confine_window ? confine_window->x11window : None,
    TCursor::X11Cursor(cursor),
    CurrentTime ) != GrabSuccess )
  {
    cerr << "toad: warning; GrapPointer failed\n";
  }
  setAllMouseMoveEvents(omm);
#endif
}

/**
 * Almost the same as grabMouse but
 * <UL>
 *   <LI>
 *     the window will receive a `closeRequest' instead of a mouse down
 *     event when the pointer is outside the window,
 *   <LI>
 *     the other window will receive the mouse down event instead and
 *   <LI>
 *     other windows in this application will reveive mouseEnter and 
 *     mouseLeave events.
 *  </UL>
 * (grabPopupMouse is used to implement the menu bar and popup windows in
 * TOAD.)
 *
 * \sa grabMouse, ungrabMouse
 */
void
TWindow::grabPopupMouse(bool allmousemove, TCursor::EType cursor)
{
#ifdef __X11__
//printf("%s.GrabPopupMouse(%03lx) (MMM:%03lx)\n",Title(),ulMouseMessages,iflags%IFLAG_MMM_MASK);

  bSimulatedAutomaticGrab = false;

  // set flags for MouseMoveMessages
  //---------------------------------
  bool omm = _allmousemove;
  setAllMouseMoveEvents(allmousemove);

  // grab the mouse pointer
  //------------------------
  if (XGrabPointer( x11display, x11window,
    True, // grab mouse events outside the application
    _buildMouseEventmask(true) | ButtonReleaseMask,
    GrabModeAsync,
    GrabModeAsync,
    None,
    TCursor::X11Cursor(cursor),
    CurrentTime ) != GrabSuccess )
  {
    cerr << "toad: warning; GrapPointer failed\n";
  }
  
  TOADBase::wndTopPopup = this;
  setAllMouseMoveEvents(omm);
#endif
}

/**
 * To release the mouse pointer after a grab.
 *
 * \sa grabMouse, grabPopupMouse
 */
void
TWindow::ungrabMouse()
{
#ifdef __X11__
  XUngrabPointer(x11display, CurrentTime);
  TOADBase::bSimulatedAutomaticGrab = false;
  TOADBase::wndTopPopup = NULL;
#endif
}

void
TWindow::setAllMouseMoveEvents(bool all)
{
  if (_allmousemove == all)
    return;
  _allmousemove = all;
  flag_mmm_modified = true;
#ifdef __X11__
  if (x11window)
    XSelectInput(x11display, x11window, _buildEventmask());
#endif
#ifdef __COCOA__
//  if (nsview)
//    [nsview setAcceptsMouseMovedEvents: b];
#endif
}

#ifdef __X11__
/**
 * determine what events the window wants
 */
long
TWindow::_buildEventmask()
{
  long mask = _buildMouseEventmask();
  mask |= ExposureMask;

  // only shell windows will get key events
  if (flagShell) {
    mask |= KeyPressMask;
    mask |= KeyReleaseMask;
    mask |= FocusChangeMask;
    mask |= StructureNotifyMask;
  }
  
  // this is for DnD experiments, removed it by time
  mask |= PropertyChangeMask;

  return mask;              

}

//! private
long 
TWindow::_buildMouseEventmask(bool force)
{
  long mask = 0L;
/*  
  if (layout) {
    mask |= ButtonPressMask | ButtonReleaseMask;
  }
*/
  mask |= ButtonPressMask;
  mask |= ButtonReleaseMask;
  mask |= EnterWindowMask;
  mask |= LeaveWindowMask;

  if (_bToolTipAvailable) {
    mask |= EnterWindowMask | LeaveWindowMask;
  }

  if (_allmousemove) {
    mask |= PointerMotionMask;
  } else {
    mask |= ButtonMotionMask;
  }
  return mask;
}
#endif

/**
 * <B>BEWARE: OLD DESCRIPTION</B><BR>
 * Some action methods like <I>setPostion</I> or <I>setSize</I> call reaction 
 * methods, e.g. when calling <I>setSize</I> it will call <I>resize()</I> and
 * the parents <I>childResize</I> method. Calling <I>setSuppressMessages(true)
 * </I> will avoid this for some methods. Please don't use this method until 
 * you know what you're doing. See also <I>isSuppressMessages</I>.
 */
void
TWindow::setSuppressMessages(bool b)
{
  flag_suppress_msg = b;
}

bool
TWindow::isSuppressMessages() const
{
  return flag_suppress_msg;
}
