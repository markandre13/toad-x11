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

#ifndef _TOAD_WINDOW_HH
#define _TOAD_WINDOW_HH 1

#include <toad/os.hh>
#include <toad/toadbase.hh>
#include <toad/interactor.hh>
#include <toad/pen.hh>
#include <toad/region.hh>
#include <toad/cursor.hh>
#include <toad/pointer.hh>
#include <toad/command.hh>

#ifdef __COCOA__
@class NSEvent, NSView, toadWindow, toadView;
#include <toad/cocoa/keyboard.hh>
#endif

#ifdef _TOAD_PRIVATE


#ifdef __X11__
#include <toad/X11/x11window.hh>
#endif

#else
namespace toad {
  struct TX11CreateWindow;
} // namespace toad
#endif

namespace toad {  // window.hh

class TWindow;
typedef GSmartPointer<TWindow> PWindow;

#ifdef DEBUG
#define CHECK_REALIZED(txt) debug_check_realized(txt)
#else
#define CHECK_REALIZED(txt)
#endif

#define TSIZE_PREVIOUS 0.0

// TRowColumn
#define TS_HORIZONTAL 0
#define TS_VERTICAL   1

class TFocusManager;
class TPen;
class TPopup;
class TMenuBar;
class TToolTip;
class TLayout;

class TMouseEvent {
  public:
    enum EType {
      MOVE, ENTER, LEAVE, 
      LDOWN, MDOWN, RDOWN, LUP, MUP, RUP,
      ROLL_UP, ROLL_UP_END,
      ROLL_DOWN, ROLL_DOWN_END
    } type;
#ifdef __COCOA__
    NSEvent *nsevent;
    TMouseEvent(NSEvent *ne, NSView *view, TWindow *window);
    static unsigned globalModifier;
#endif
    TMouseEvent(TWindow *aWindow=0, TCoord anX=0.0, TCoord anY=0.0, unsigned aModifier=0):
      window(aWindow), x(anX), y(anY), _modifier(aModifier) {dblClick=false;}
    TWindow *window;
    TCoord x, y;
    bool dblClick;
    unsigned modifier() const { return _modifier; }
    TCoord pressure() { return 0; }
    TCoord rotation() { return 0; }
    TCoord tilt() { return 0; }
    unsigned _modifier;
};

class TKeyEvent {
  public:
    enum EType {
      DOWN, UP
    } type;
    TKeyEvent(EType aType=DOWN, TWindow *aWindow=0, TKey aKey=0, unsigned aModifier=0):
      window(aWindow), type(aType), _key(aKey), _modifier(aModifier) {}
#ifdef __COCOA__
    TKeyEvent(NSEvent *ns) {
      nsevent = ns;
      _modifier = [nsevent modifierFlags];
      _key = [nsevent keyCode];
    }
    NSEvent *nsevent;
#endif
    TWindow *window;
    string str() const;
    TKey key() const { return _key; }
    void setKey(TKey key) { _key = key; }
    unsigned modifier() const { return _modifier; }
    void setModifier(unsigned modifier) { _modifier = modifier; }
    TKey _key;
    unsigned _modifier;
};

#ifdef __WIN32__
#ifdef NEW
#undef NEW
#endif

#ifdef DELETE
#undef DELETE
#endif
#endif

class TWindowEvent {
  public:
    enum {
      NEW,
      DELETE,
      CREATE,
      CREATED,
      DESTROY,
      MAPPED,
      UNMAPPED,
      PAINT,
      ADJUST,
      RESIZE,
      FOCUS
    } type;
    TWindow *window;
};  

class TWindow: 
  public TInteractor, public TRectangle, public TOADBase
{
    friend class TOADBase;
    friend class TPen;
    friend class TFocusManager;

    TWindow& operator=(const TWindow&); // not allowed
    TWindow(const TWindow&);            // not allowed

  public:
    TWindow(TWindow *parent, const string &title);
    virtual ~TWindow();

    virtual void setTitle(const string &title);
    static bool createParentless();
    static void destroyParentless();
    static unsigned getParentlessCount();
    static TWindow* getParentless(unsigned);

    void createWindow();
    void destroyWindow();

    // style flags
    //-------------
    bool flagPopup:1;
    //! don't create window when parent is created
    bool bExplicitCreate:1;
    //! suggest X11 to buffer the content of windows under this one
    bool bSaveUnder:1;
    //! don't let the window manager resize the window
    bool bStaticFrame:1;
    //! suggest X11 to buffer the window contents to avoid paint event
    bool bBackingStore:1;
    //! don't let X11 paint a background
    bool bNoBackground:1;
    //! window will cooperate with the dialog editor
    bool bDialogEditRequest:1;
    //! 'true' gets the tab key and disables traversal
    bool bTabKey:1;
    //! tell TPen to use double buffering with this window
    bool bDoubleBuffer:1;

    //! X11 internal: use original X11 GC (needed for OpenGL window)
    bool bX11GC:1;              // use the Xlib default gc

    //! don't add a menu to a shell window
    bool bNoMenu:1;
    
    //! compress mouse move events for this window (default is true)
    bool bCompressMotion:1;
    
    /**
     * toad::mainLoop will exit when all windows with a parent of NULL
     * and bParentlessAssistant equal 'false' are closed.
     * The default is 'false'.
     */
    bool bParentlessAssistant:1;

    //! Return 'true' when the window is created on the screen.
    #ifdef __X11__
    bool isRealized() const {return x11window!=0;}
    #endif
    
    #ifdef __XCB__
    bool isRealized() const {return xcbWindow!=0;}
    #endif

    #ifdef __COCOA__
    bool isRealized() const {return nsview!=0;}
    #endif
    
    #ifdef __WIN32__
    bool isRealized() const {return w32window!=0;}
    #endif

    void clearWindow();
    void setAllMouseMoveEvents(bool);
    void grabMouse(bool allmove=true, TWindow* confine_window=NULL, TCursor::EType type=TCursor::DEFAULT);
    void grabPopupMouse(bool allmove=true, TCursor::EType type=TCursor::DEFAULT);
    void ungrabMouse();
    void getRootPos(TCoord*,TCoord*);
    
    void invalidateWindow(bool clearbg=true);
    void invalidateWindow(TCoord,TCoord,TCoord,TCoord, bool clearbg=true);
    void invalidateWindow(const TRectangle&, bool clearbg=true);
    void invalidateWindow(const TRegion&, bool bClrBG=true);

    void paintNow();
    
    void raiseWindow();
    void lowerWindow();
    
    void scrollWindow(TCoord x,TCoord y, bool bClrBG=true);
    void scrollRectangle(const TRectangle &rect, TCoord x,TCoord y, bool bClrBG=true);
    
    void setOrigin(TCoord x,TCoord y);
    void scrollTo(TCoord x, TCoord y);
    void getOrigin(TCoord *x, TCoord *y) const { *x = _dx; *y = _dy; }
    TCoord getOriginX() const { return _dx; }
    TCoord getOriginY() const { return _dy; }

    void setPosition(TCoord x,TCoord y);
    void setSize(TCoord x,TCoord y);
    void setShape(TCoord,TCoord,TCoord,TCoord);
    void setShape(const TRectangle &r){setShape(r.x,r.y,r.w,r.h);}
    void setShape(const TRectangle *r){setShape(r->x,r->y,r->w,r->h);}
    void getShape(TRectangle*) const;
    
    void setMapped(bool);
    bool isMapped() const;

    void setIcon(TBitmap*);
    void setCursor(TCursor::EType);
    void setCursor(const TCursor *cursor);
    // void SetToolTip(TToolTip*);              // implemented in tooltip.cc
    void setToolTip(const string&);             // implemented in tooltip.cc
    
    const TRGB& getBackground() const { return _bg; }
    void setBackground(const TRGB &c);
    void setBackground(byte r,byte g,byte b) {
       setBackground(TRGB(r,g,b));
    }
    void setBackground(TColor::EColor c) {
      const TRGB *rgb = TColor::lookup(c);
      if (rgb)
        setBackground(*rgb);
    }
    void setBackground(TBitmap*);
    void setHasBackground(bool);
                                          
    bool setFocus();
    bool isFocus() const;

    //! Returns the parent window of this window.
    TWindow* getParent() const {
      TInteractor *p = TInteractor::getParent();
      if (!p) return NULL;
      TWindow *w = dynamic_cast<TWindow*>(p);
      if (!w) std::cerr << "fatal: window with interactor parent" << std::endl;
      return w;
    }

    // don't you use these methods:
    void setSuppressMessages(bool);
    bool isSuppressMessages() const;

  protected:
    int _b;                   // border width
    TCoord _dx, _dy;             // origin for TPen
#ifdef __X11__
    _TOAD_CURSOR _cursor;
#endif
    bool _allmousemove;
    
  private:
    TLayout *layout;          // layout

  public:
    void loadLayout(const string &filename);
    void setLayout(TLayout*);
    TLayout * getLayout() const { return layout; }
  
    void setBorder(unsigned b){ _b=b?1:0; }
    unsigned getBorder() const {return _b;}
    TCoord getXPos() const { return x; }
    TCoord getYPos() const { return y; }
    TCoord getWidth() const { return w; }
    TCoord getHeight() const { return h; }
    
    #ifdef DEBUG
    void debug_check_realized(const char *txt);
    #endif

    // TOADs message methods
    virtual void adjust();                // adjust window to children
    void doResize();
    virtual void resize();                // adjust children to window

    enum EChildNotify {
      TCHILD_TITLE, TCHILD_POSITION, TCHILD_RESIZE, TCHILD_ADD,
      TCHILD_REMOVE, TCHILD_CREATE, TCHILD_DESTROY, TCHILD_ICON,
      TCHILD_FOCUS, TCHILD_DIALOG_EDIT_REQUEST, TCHILD_BEFORE_ADD,
      TCHILD_BEFORE_CREATE
    };
    //! this method should be removed in favour for event filters
    virtual void childNotify(TWindow*, EChildNotify);
    
  private:
    void _childNotify(EChildNotify type);
    
  public:
    virtual void closeRequest();          // the window should destroy itself
    virtual void create();                // called before the window is created
    virtual void created();               // called after the window is created; add childs here etc.
    virtual void destroy();

    #ifdef __X11__
    virtual void createX11Window(TX11CreateWindow*);
    virtual void handleX11Event();
    #endif
    
  
    virtual void keyEvent(const TKeyEvent&);
    virtual void keyDown(const TKeyEvent&);
    virtual void keyUp(const TKeyEvent&);

    void mouseEvent(const TMouseEvent &);
    void windowEvent(const TWindowEvent &we);
#if 0
    class TMouseCrossingEvent:
      public TMouseEvent
    {
      enum EMode NORMAL, GRAB, UNGRAB;
      enum EDetail ANCESTOR, INFERIOR, NONLINEAR, NONLINEARVIRTUAL, VIRTUAL;
      Emode mode;
      EDetail detail;
    };
#endif    
    //! This method receives all mouse events and distributes them
    //! to the other mouse methods like mouseMove or mouseLDown.
    virtual void mouseMove(const TMouseEvent&);
    virtual void mouseEnter(const TMouseEvent&);
    virtual void mouseLeave(const TMouseEvent&);
    virtual void mouseLDown(const TMouseEvent&);
    virtual void mouseMDown(const TMouseEvent&);
    virtual void mouseRDown(const TMouseEvent&);
    virtual void mouseLUp(const TMouseEvent&);
    virtual void mouseMUp(const TMouseEvent&);
    virtual void mouseRUp(const TMouseEvent&);

    virtual void paint();
    virtual void saveYourself();            // the window should save itself

//    void StartDrag(TDragContext*);
//    void RegisterDrop(TDropContext*);
//    TDropContext* GetDropArea(int,int);

  private:

    void _interactor_init();
    void _interactor_adjustW2C();
    void _interactor_create();
    void _destroy();
    void _cons(TWindow*, const char *);
    static void _setFocusHelper(TInteractor *parent, bool b);
  public:
    void _setFocus(bool b);
  private:

#ifdef __X11__
    void _providePaintRgn();
#endif
    unsigned short _mmm_mask;

    // internal flags
    bool bEraseBe4Paint:1;
    bool _bOwnsFocus:1;
    bool _visible:1;        // true when window should be visible
    bool _bResizedBeforeCreate:1;
    bool _bToolTipAvailable:1;
    bool flag_wm_resize:1;
    bool flag_explicit_create:1;
    bool flag_delete_title:1;
    bool flag_mmm_modified:1;
    bool flag_create:1;
    bool flag_suppress_msg:1;
    bool flag_child_notify:1;
    /**
     * When 'true' and this window is owned by the window manager, let the
     * window manager decide where to place the window.
     * This values is set to 'false' by setPosition and setShape.
     */
    bool flag_position_undefined:1;
#ifdef __X11__    
    long _buildEventmask();
    long _buildMouseEventmask(bool force = true);
#endif

    #ifdef DEBUG
    static const unsigned DFLAG_WMINIT    = 1;
    static const unsigned DFLAG_WMADJUST  = 2;
    unsigned debug_flags;
    #endif

  public:
    TRGB _bg; // background color
    class TPaintRegion;
  private:
    static bool _havePaintEvents();
    static void _dispatchPaintEvent();
    TPaintRegion *paint_rgn;
    
  public:
    TRegion* getUpdateRegion() const;
    #ifdef __WIN32__
    static void w32registerclass();
    #endif
  #ifndef _TOAD_PRIVATE
  protected:
  #endif

    #ifdef __X11__
    _TOAD_WINDOW x11window;
    #endif
    
    #ifdef __XCB__
    xcb_window_t xcbWindow;
    #endif
    
    #ifdef __COCOA__
    bool _inside:1; // helper to emulate mouseEnter, mouseLeave on Cocoa
    bool _mapped:1;
    void _down(TMouseEvent::EType type, NSEvent *theEvent);
    void _up(TMouseEvent::EType type, NSEvent *theEvent);
    toadView *nsview;
    toadWindow *nswindow;
    #endif
    
    #ifdef __WIN32__
    HWND w32window;
    static LRESULT CALLBACK w32proc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
    
    struct TPaintStruct {
      TPaintStruct() {
        hdc = 0;
        refcount = 0;
        origpen = 0;
        origbrush = 0;
        currentpen = 0;
      }
      HDC hdc;
      unsigned refcount;
      HGDIOBJ origpen;
      HGDIOBJ origbrush;
      const TPen *currentpen;
    };
    TPaintStruct *paintstruct;
    #endif
};

class TWindow::TPaintRegion:
  public TRegion
{
  public:
    TWindow *wnd;
};

} // namespace toad   window.hh

// these should've been in `toadbase.hh' but this confuses the include
// hierachy
#include <toad/connect.hh>
#include <toad/messagebox.hh>

#endif
