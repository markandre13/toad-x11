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

#ifndef TWindow
#define TWindow TWindow

#include <toad/toadbase.hh>
#include <toad/interactor.hh>
#include <toad/pen.hh>
#include <toad/region.hh>
#include <toad/cursor.hh>
#include <toad/pointer.hh>
#include <toad/command.hh>

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
#endif

#include <limits.h>
#define TPOS_PREVIOUS INT_MIN
#define TSIZE_PREVIOUS INT_MIN

// TRowColumn
#define TS_HORIZONTAL 0
#define TS_VERTICAL   1

class TFocusManager;
class TPen;
class TPopup;
class TMenuBar;
class TToolTip;
class TLayout;

class TWindow: 
  public TInteractor, public TOADBase
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
    bool bPopup:1;
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

    //! Return 'true' when the window is created on the screen.
    #ifdef __X11__
    bool isRealized() const {return x11window!=0;}
    #endif
    
    #ifdef __WIN32__
    bool isRealized() const {return w32window!=0;}
    #endif

    void clearWindow();
    void grabMouse(unsigned short mouseMessages=TMMM_PREVIOUS,TWindow* confine_window=NULL, TCursor::EType type=TCursor::DEFAULT);
    void grabPopupMouse(unsigned short mouseMessages=TMMM_PREVIOUS, TCursor::EType type=TCursor::DEFAULT);
    void ungrabMouse();
    void getRootPos(int*,int*);
    
    void invalidateWindow(bool clearbg=true);
    void invalidateWindow(int,int,int,int, bool clearbg=true);
    void invalidateWindow(const TRectangle&, bool clearbg=true);
    void invalidateWindow(const TRectangle *rect, bool clearbg=true) {
      invalidateWindow(*rect, clearbg);
    }
    void invalidateWindow(const TRegion&, bool bClrBG=true);
    void invalidateWindow(const TRegion *reg, bool clearbg=true) {
      invalidateWindow(*reg, clearbg);
    }
    void paintNow();
    
    void raiseWindow();
    void lowerWindow();
    
    void scrollWindow(int x,int y, bool bClrBG=true);
    void scrollRectangle(const TRectangle &rect, int x,int y, bool bClrBG=true);
    
    void setOrigin(int x,int y);
    void scrollTo(int x, int y);
    void getOrigin(int *x, int *y) const { *x = _dx; *y = _dy; }
    int getOriginX() const { return _dx; }
    int getOriginY() const { return _dy; }

    void setPosition(int x,int y);
    void setSize(int x,int y);
    void setShape(int,int,int,int);
    void setShape(const TRectangle &r){setShape(r.x,r.y,r.w,r.h);}
    void setShape(const TRectangle *r){setShape(r->x,r->y,r->w,r->h);}
    void getShape(TRectangle*) const;
    
    void setMapped(bool);
    bool isMapped() const;

    void setIcon(TBitmap*);
    void setCursor(TCursor::EType);
    // void SetToolTip(TToolTip*);              // implemented in tooltip.cc
    void setToolTip(const string&);             // implemented in tooltip.cc
    
    void setBackground(const TColor&);
    void setBackground(byte r,byte g,byte b) {
       setBackground(TColor(r,g,b));
    }
    void setBackground(const TRGB &c) {
       setBackground(TColor(c.r,c.g,c.b));
    }
    void setBackground(TColor::ESystemColor c) {
      setBackground(TColor(c));
    }
    void setBackground(TColor::EColor16 c) {
      setBackground(TColor(c));
    }
    void setBackground(TBitmap*);
    void setHasBackground(bool);
                                          
    // MouseMoveMessages
    static const unsigned short TMMM_NONE      =0x0000;
    static const unsigned short TMMM_FIRST     =0x0010;
    static const unsigned short TMMM_ALL       =0x0020;
    static const unsigned short TMMM_LBUTTON   =0x0040;
    static const unsigned short TMMM_MBUTTON   =0x0080;
    static const unsigned short TMMM_RBUTTON   =0x0100;
    static const unsigned short TMMM_ANYBUTTON =0x0200;
    static const unsigned short TMMM_PREVIOUS  =0x03FF;
    void setMouseMoveMessages(unsigned short);
    void addMouseMoveMessages(unsigned short);
    void clrMouseMoveMessages(unsigned short);

    bool setFocus();
    bool isFocus() const;

    //! Returns the parent window of this window.
    TWindow* getParent() const {
      TInteractor *p = TInteractor::getParent();
      if (!p) return NULL;
      TWindow *w = dynamic_cast<TWindow*>(p);
      if (!w) cerr << "fatal: window with interactor parent" << endl;
      return w;
    }

    // don't you use these methods:
    void setSuppressMessages(bool);
    bool isSuppressMessages() const;

  protected:
    int _x, _y, _w, _h;       // window position and size
    int _b;                   // border width
    int _dx, _dy;             // origin for TPen
    unsigned char _cursor;    // cursor type
    
  private:
    TLayout *layout;          // layout

  public:
    void loadLayout(const string &filename);
    void setLayout(TLayout*);
    TLayout * getLayout() const { return layout; }
  
    void setBorder(unsigned b){ _b=b?1:0; }
    unsigned getBorder() const {return _b;}
    int getXPos() const { return _x; }
    int getYPos() const { return _y; }
    int getWidth() const { return _w; }
    int getHeight() const { return _h; }
    
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
    
  
    virtual void keyEvent(TKeyEvent&);
#ifdef TOAD_EVENTCLASSES
    virtual void keyDown(TKeyEvent&);
    virtual void keyUp(TKeyEvent&);
#else   
    virtual void keyDown(TKey key, char *string, unsigned modifier);
    virtual void keyUp(TKey key, char *string, unsigned modifier);
#endif    

    void mouseEvent(TMouseEvent &);
    void windowEvent(TWindowEvent &we);

#ifdef TOAD_EVENTCLASSES
    class TMouseCrossingEvent:
      public TMouseEvent
    {
      enum EMode NORMAL, GRAB, UNGRAB;
      enum EDetail ANCESTOR, INFERIOR, NONLINEAR, NONLINEARVIRTUAL, VIRTUAL;
      Emode mode;
      EDetail detail;
    };
    
    //! This method receives all mouse events and distributes them
    //! to the other mouse methods like mouseMove or mouseLDown.
    virtual void mouseMove(TMouseEvent&);
    virtual void mouseEnter(TMouseCrossingEvent&);
    virtual void mouseLeave(TMouseCrossingEvent&);
    virtual void mouseLDown(TMouseEvent&);
    virtual void mouseMDown(TMouseEvent&);
    virtual void mouseRDown(TMouseEvent&);
    virtual void mouseLUp(TMouseEvent&);
    virtual void mouseMUp(TMouseEvent&);
    virtual void mouseRUp(TMouseEvent&);
#else
    virtual void mouseMove(int x,int y, unsigned modifier);
    virtual void mouseEnter(int x,int y, unsigned modifier);
    virtual void mouseLeave(int x,int y, unsigned modifier);
    virtual void mouseLDown(int x,int y, unsigned modifier);
    virtual void mouseMDown(int x,int y, unsigned modifier);
    virtual void mouseRDown(int x,int y, unsigned modifier);
    virtual void mouseLUp(int x,int y, unsigned modifier);
    virtual void mouseMUp(int x,int y, unsigned modifier);
    virtual void mouseRUp(int x,int y, unsigned modifier);
#endif
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

  #ifdef _TOAD_PRIVATE
  public:
  #else
  private:
  #endif
    TColor background;                      // background color
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
  protected:

    #ifdef __X11__
    _TOAD_WINDOW x11window;
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
