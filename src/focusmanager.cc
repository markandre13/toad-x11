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

/**
 * \defgroup keyboardfocus Keyboard Focus Manager
 *
 * 
 *  Keyboard Focus Manager version 3.1
 *
 *  For every top level window (aka shell window, aka child of the root
 *  window) TOAD creates a focus domain (TDomain) object with informations
 *  on how to distribute keyboard events to the top level window and most of
 *  its' children.
 *
 *  - every window with Parent()==NULL has bShell=true    
 *  - every window with bShell==true is a top level window
 *  - every top level domain is stored in `top_domain_map'
 *  - every window with bFocusManager==true adds a sub domain to the top
 *    level domains' tree
 *  - current_domain points to the active top level domain or NULL
 *  - current_domain->focus_window points to the window owning the
 *      keyboard focus
 *    
 *  The focus traversal is limited to the domains and sub domains,
 *  which can be part of the top level windows' domain.
 *    
 *  All sub domains are linked in a tree structure whose root node is
 *  the top domain. This `focus tree' exists in parallel to the `window
 *  tree'.
 *    
 *  The main reason for this complication is to simplify the usage of
 *  MDI windows.
 *    
 *  Keys for the focus traversal:
 *  - F7 (CTRL+TAB) : left
 *  - F8 (TAB     ) : right
 *  - F5            : down   (not implemented yet)
 *  - F6            : up     (not implemented yet)
 *  A better solution would be some kind of mapping to the cursor keys.
 *  (I've read IBM has a patent on the focus traversal with the TAB key.)
 *
 *  Other special keys:
 *  - F12           : print debug information
 *    
 *  ATTENTION:
 *  Errors are bound to happen when the TWindow attributes `bFocusManager'
 *  and `bShell' are modified between calls to `createWindow()' and 
 *  `destroyWindow()'.
 *    
 *  STUFF TO ADD:
 *  A sub domain should store it's last focus and reactivate it when it
 *  gets the focus again.
 *    
 *  Since the X11 support for internationalized text input is bound to the
 *  focus management, the i18n related code is placed here also.
 */

/*
  A VERY IMPORTANT THING TO DO IS:
  - to modify `XNFocusWindow' when the focus changes
*/

#define DBM(CMD)

#include <toad/os.hh>

#ifdef __X11__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlocale.h>
#endif

#define _TOAD_PRIVATE

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/dialogeditor.hh>

#include <map>
#include <assert.h>

// debugging:
#include <toad/action.hh>

namespace toad {

#ifdef __X11__
static XIM xim = NULL;
static XIMStyle xim_style;

XIC xic_current = NULL;
#endif

struct TDomain
{
  TDomain() {
    owner=focus_window=NULL; 
    focus=next=first_child=parent=NULL;
    filter = NULL;
#ifdef __X11__
    xic = NULL;
#endif
  }
  ~TDomain() {
#ifdef __X11__
    if (xic)
      XDestroyIC(xic);
#endif
  }
  TWindow *owner, *focus_window;
  TDomain *focus, *next, *first_child, *parent;
  TEventFilter *filter;
#ifdef __X11__
  XIC xic;
#endif
};

typedef map<TWindow*,TDomain*> TDomainMap;
static TDomainMap top_domain_map;

static TDomain *current_domain = NULL;

static void AddSubDomain(TWindow*);
static void DelSubDomain(TWindow*);

// call `domainFocus' for all windows in a domain
static void ToggleDomain(TDomain *d, bool on);

static TDomain* GetDomain(TWindow*);
static TDomain* GetTopDomain(TWindow *wnd);
static TWindow* Walk(TWindow *top, TWindow *focus, bool bNext);
static TDomain* SetPathTo(TWindow *wnd, TWindow *who);

// called everytime a new window is created on the screen and it creates
// a new domain if necessary
//---------------------------------------------------------------------------
void
TOADBase::focusNewWindow(TWindow* wnd)
{
  assert(wnd!=NULL);
  
  if (wnd->bShell) {
//if(wnd->bPopup) cout << "new popup: " << wnd->getTitle() << endl;
    // create a new domain for window `wnd'
    //----------------------------------------------------------------
    assert(top_domain_map.find(wnd)==top_domain_map.end());

    TDomain *domain = new TDomain();
    domain->owner = wnd;
#ifdef __X11__
    if (xim) {
      domain->xic = XCreateIC(xim,
                              XNInputStyle, xim_style,
                              XNClientWindow, wnd->x11window,
                              XNFocusWindow, wnd->x11window,
                              NULL);
      if (domain->xic==NULL)
        cerr << "toad: Couldn't create X Input Context for window \"" 
             << wnd->getTitle() << "\"" << endl;
    }
#endif
    top_domain_map[wnd]=domain;
  } else if (wnd->bFocusManager) {
    // add a new sub domain for window `wnd' in the windows' top level 
    // window parents' domain
    //----------------------------------------------------------------
    AddSubDomain(wnd);
  }
}

void
TOADBase::focusDelWindow(TWindow* wnd)
{
  assert(wnd!=NULL);

  TDomain *domain = GetTopDomain(wnd);

  // check top level domain
  //-----------------------
  if (domain->focus_window==wnd) {
    domain->focus_window=NULL;
    wnd->_setFocus(false);
  }

  // check sub domain
  //-----------------------
  TDomain *subdomain = GetDomain(wnd);
  if (subdomain->focus_window==wnd) {
    subdomain->focus_window=NULL;
  }

#if 0
cout << "FocusDelWindow" << endl
     << "  current_domain: " << current_domain << endl
     << "  domain        : " << domain << endl
     << "  subdomain     : " << subdomain << endl;
#endif

  // remove domain
  //------------------------------------------------------------------
  if (wnd->bShell) {
    // when we remove a domain it's subdomains should be removed
    // already
    assert(domain->first_child==NULL);
    
    TDomainMap::iterator dp = top_domain_map.find(wnd);
    top_domain_map.erase(dp);
    if (current_domain == domain) {
//      toggleDomain(domain, false);
      current_domain = NULL;
    }
    delete domain;
    return;
  }
  
  // remove sub domain
  //------------------------------------------------------------------
  if (wnd->bFocusManager) {
    // if (subdomain active)
//    toggleDomain(subdomain, false);
    DelSubDomain(wnd);
  }
}

void
TOADBase::domainToWindow(TWindow *wnd)
{
DBM(cout << "DomainToWindow " << (wnd ? wnd->getTitle() : "(NULL)") << endl;)
  TDomain *new_domain = wnd ? GetTopDomain(wnd) : NULL;

  if (new_domain!=current_domain) {
  
    // deactivate old domain
    //-----------------------
    if (current_domain) {
#ifdef __X11__
      if (current_domain->xic) {
        xic_current = NULL;
        XUnsetICFocus(current_domain->xic);
      }
#endif
      // deactivate subtree
      TDomain *p = current_domain;
      while(p) {
        p->owner->_setFocus(false);
        p=p->focus;
      }
      
      if (current_domain->focus_window)
        current_domain->focus_window->_setFocus(false);
    }

    current_domain = new_domain;

    // activate new domain
    //---------------------
    if (current_domain) {
#ifdef __X11__
      if (current_domain->xic) {
        xic_current = current_domain->xic;
        XSetICFocus(current_domain->xic);
      }
#endif
      // activate subtree
      TDomain *p = current_domain;
      while(p) {
        p->owner->_setFocus(true);
        p=p->focus;
      }

      // just in case there is no focus window: find one
      //-------------------------------------------------
      // OUCH!!! The same code is duplicated below in `KeyDown'
      if (!current_domain->focus_window) {
        wnd = Walk(current_domain->owner, NULL, true);
        if (!wnd)
          wnd=current_domain->owner;
        setFocusWindow(wnd);
      }
    
      if (current_domain->focus_window)
        current_domain->focus_window->_setFocus(true);
    }
  }
}


/** 
 * Called by <CODE>bool TWindow::SetFocus</CODE> to set the keyboard
 * focus. The real job is done in `SetPathTo'.
 */
void
TOADBase::setFocusWindow(TWindow* wnd)
{
  assert(wnd!=NULL);

DBM(cout << "setFocusWindow " << wnd->getTitle() << endl;)
  if (wnd->bNoFocus && !wnd->bFocusManager && !wnd->bShell) {
DBM(
    cout << "window \"" << wnd->getTitle() << "\" doesn't need focus" << endl;
    cout << "  bNoFocus     : " << (wnd->bNoFocus?"true":"false") << endl;
    cout << "  bFocusManager: " << (wnd->bFocusManager?"true":"false") << endl;
    cout << "  bShell       : " << (wnd->bShell?"true":"false") << endl;
)
    return;
  }
  
  if (!wnd->isRealized()) {
#if 1
    cerr << "window \"" << wnd->getTitle() << "\": can't set focus because parent isn't realized yet\n"
            "  (don't worry, it's TOADs' fault)\n";
#endif
    return;
  }
  
  TDomain *top_domain = GetTopDomain(wnd);
  assert(top_domain!=NULL);
  if (top_domain->focus_window==wnd)
    return;

  // when `wnd' is a window with a focus domain and it doesn't want to
  // receive keyboard events, try to take one of its children
  //------------------------------------------------------------------
  if ( wnd->bNoFocus && (wnd->bFocusManager || wnd->bShell) )
  {
    TWindow *memo = wnd;
DBM(cout << "trying one of the cildren" << endl;)
    // set focus to an inferior window that wants to receive keyboard
    // events
    TWindow *inferior = Walk(wnd, NULL, true);
    if (inferior) {
      wnd = inferior;
    } else {
      // there is no inferior window, instead walk down the list of
      // domains
      TDomain *bottom = GetDomain(wnd);
      while(bottom->first_child) {
        if (!bottom->focus)
          bottom->focus = bottom->first_child;
        bottom = bottom->focus;
      }
      wnd = Walk(bottom->owner, NULL, true);
    }

    // when no one wants keyboard events fall back to the focusmanager
    // or shell window
    if (!wnd)
      wnd = memo;
  }
DBM(
if (!wnd)
  cout << "no destination for focus" << endl;
else
  cout << "destination for focus is " << wnd->getTitle() << endl;
)

  // set a new path to `wnd' in the domain tree
  if (wnd) {
    TDomain *d = SetPathTo(wnd, wnd);
    ToggleDomain(d, true);
  }
}

static void
ToggleDomain2(TInteractor *wnd, bool on)
{
  wnd->domainFocus(on);
  TInteractor *p = wnd->getFirstChild();
  while(p) {
    if (!p->bFocusManager && !p->bShell)
      ToggleDomain2(p, on);
    p = TInteractor::getNextSibling(p);
  }
}

void
ToggleDomain(TDomain *d, bool on)
{
  DBM(cout << "toggle domain \"" << d->owner->getTitle() << "\" " << (on ? "on" : "off") << endl;)
  ToggleDomain2(d->owner, on);
}

// Called as `SetPathTo(wnd, wnd)' to set the focus to window `wnd'.
// This recursive function works on two separate trees: the window
// tree and the focus domain tree.
TDomain*
SetPathTo(TWindow *wnd, TWindow *new_focus)
{
  // top of recursion: return the top level domain 
  // and set new focus window
  //-----------------------------------------------
  if (wnd->bShell) {
DBM(cout << "top of recursion" << endl;)
    TDomainMap::iterator dp = top_domain_map.find(wnd);
    assert(dp!=top_domain_map.end());
    TWindow *focus = (*dp).second->focus_window;
    if (focus && focus!=new_focus)
      focus->_setFocus(false);
    (*dp).second->focus_window = new_focus;

    if (wnd==new_focus) {
      wnd->_setFocus(true);
    }

    return (*dp).second;
  }
  
  // recurse upwards
  //-----------------------------------------------
  TDomain *superior_domain = SetPathTo(wnd->getParent(), new_focus);
  
  if (wnd->bFocusManager) {
    // locate the focus managers domain in it's superior domain
    // domain := domain of wnd
    TDomain *domain = superior_domain->first_child;
    while(domain) {
      if (domain->owner == wnd)
        break;
      domain = domain->next;
    }
    assert(domain!=NULL);
DBM(    
    cout << "superior domain points to " << (superior_domain->focus ? superior_domain->focus->owner->getTitle() : "(NULL)") << endl;
    cout << "  new focus will be " << wnd->getTitle() << endl;
)

    if (superior_domain->focus && superior_domain->focus->owner != wnd) {
      // deactivate old focus subtree
DBM(      cout << "deactivating old subtree" << endl;)
      TDomain *p = superior_domain->focus;
      while(true) {
        p->owner->_setFocus(false);
        if (!p->focus) {
          ToggleDomain(p, false);
          break;
        }
        p = p->focus;
      }
      
      superior_domain->focus = domain;
    }

DBM(    cout << "calling _setFocus(true) in " << domain->owner->getTitle() << endl;)
    domain->owner->_setFocus(true);
  
    return domain;
  } 
  
  if (wnd==new_focus) {
    wnd->_setFocus(true);
  }
  
  return superior_domain;
}

TWindow*
TOADBase::getFocusWindow()
{
  return current_domain ? current_domain->focus_window : NULL;
}

static bool flag[1024];
static TDomain *print_top_domain;

/**
 * Helper for debugging output.
 */
static void
print(TInteractor *p, unsigned d=0)
{
  for(int i=0; i<d; i++) {
    if (i==d-1) {
      if(flag[d-1])
         cout << "\033(0tqqq\033(B";
      else
         cout << "\033(0mqqq\033(B";
    } else {
      if (flag[i])
        cout << "\033(0x\033(B   ";
      else
        cout << "    ";
    }
  }
  cout << p;
  TWindow *w = dynamic_cast<TWindow*>(p);
  if (w) {
    cout << " (" << w->getTitle();
    if (w->isRealized()) {
      cout<< ":" 
          << w->getXPos() << ","
          << w->getYPos() << ","
          << w->getWidth() << ","
          << w->getHeight();
    }
    cout << ")";
    TDomainMap::iterator dp = top_domain_map.find(w);
    if (dp!=top_domain_map.end()) {
      cout << " [top]";
      print_top_domain = (*dp).second;
      if (print_top_domain == current_domain) {
        cout << " [current top]";
      }
    }
    
    TDomain *domain = GetDomain(w);
    if (domain->owner==w) {
      cout << " [domain owner]";
    }
    if (domain->focus_window==w) {
      cout << " [current sub]";
    }
    
    if (w == print_top_domain->focus_window) {
      cout << " (focus window)";
    }
  } else {
    cout << " (" << p->getTitle() << ")";
  }
  
  cout << endl;
  
  TInteractor *c = p->getFirstChild();
  if (c==NULL)
    return;
  do {
    TInteractor *n = c->getNextSibling();
    flag[d] = (n!=NULL);
    print(c, d+1);
    c = n;
  } while(c!=NULL);
}

// called from the message loop to distribute the `keyDown' event
void
TOADBase::handleKeyDown(TKey key, char* t, unsigned m)
{
  if (!current_domain)    // paranoia check
    return;
#if 0
cout << "keyDown" << endl
     << "  current_domain: " << current_domain << endl;
printf("keyDown for %08x\n", current_domain->focus_window);
#endif

  if (key==TK_F12) {
    cout << "DEBUG: Window & Keyboard Focus Tree" << endl;
    flag[0]=true;
    int n = TWindow::getParentlessCount();
    for(int i=0; i<n; i++) {
      print(TWindow::getParentless(i));
    }
  }

  // focus traversal
  //-------------------------------------------------
  TWindow *wnd = NULL;
  
  if ((key==TK_TAB && !(m & MK_SHIFT)) || 
      key==TK_F8) {
    wnd = Walk( current_domain->focus_window
                  ? GetDomain(current_domain->focus_window)->owner
                  : current_domain->owner,
                current_domain->focus_window,
                true );
  } else if (key==TK_LEFT_TAB || 
            (key==TK_TAB && (m & MK_SHIFT)) ||
            key==TK_F7 ) 
  {
    wnd = Walk( current_domain->focus_window
                  ? GetDomain(current_domain->focus_window)->owner
                  : current_domain->owner,
                current_domain->focus_window,
                false );
  }

  if ( (key==TK_TAB || key==TK_LEFT_TAB) && 
        current_domain->focus_window &&
        current_domain->focus_window->bTabKey )
    wnd = NULL;

  if (wnd && wnd!=current_domain->focus_window) {
    setFocusWindow(wnd);
    return;
  }

  // just in case there is no focus window: find one
  //-------------------------------------------------
  if (!current_domain->focus_window) {
    wnd = Walk(current_domain->owner, NULL, true);
    // BAD STYLE!!!
    if (!wnd) { // // make exception for single toplevel window apps :(
      if (!current_domain->owner->bNoFocus)
        wnd=current_domain->owner;
    }
    if (wnd) {
      current_domain->focus_window = wnd;
      current_domain->focus_window->_setFocus(true);
    }
  }
    

  // iterate through the keyboard filters
  //-------------------------------------------------
  if (current_domain->focus_window) {
    TEventFilter *filter;
  
    static TKeyEvent keyevent;
    keyevent.type = TKeyEvent::DOWN;
    keyevent.window = current_domain->focus_window;
    keyevent.key = key;
    keyevent.string = t;
    keyevent.modifier = m;
  
    filter = toad::global_evt_filter;
    while(filter) {
      if (filter->keyEvent(keyevent))
        return;
      filter = filter->next;
    }
  
    filter = current_domain->filter;
    while(filter) {
      if (filter->keyEvent(keyevent))
        return;
      filter = filter->next;
    }

    // dispatch the key event
    //------------------------
    current_domain->focus_window->keyEvent(keyevent);
  }
}

void
TOADBase::handleKeyUp(TKey key, char* t, unsigned m)
{
  if (current_domain && current_domain->focus_window)
    current_domain->focus_window->keyUp(key,t,m);
}


//---------------------------------------------------------------------------
void
AddSubDomain(TWindow *wnd)
{
  assert(wnd!=NULL);
  
  // get the windows domain
  //------------------------
  TDomain *domain = GetDomain(wnd);
  assert(domain!=NULL);
  
  // create a new domain and add it to the windows' domain
  //------------------------------------------------------
  TDomain *new_domain = new TDomain();
  new_domain->owner = wnd;
  new_domain->parent = domain;
  new_domain->next = domain->first_child;
  domain->first_child = new_domain;
  
  // the first domain added to another domain gets the focus
  //--------------------------------------------------------
  if (!domain->focus)
    domain->focus = new_domain;
}

void
DelSubDomain(TWindow *wnd)
{
  assert(wnd!=NULL);
  
  TDomain *domain = GetDomain(wnd);
  assert(domain!=NULL);
#if 0
cout << "domain->owner:" << domain->owner->getTitle() << endl;
cout << "popup?       :" << domain->owner->bPopup << endl;
cout << "domain for   :" << wnd->getTitle() << endl;
#endif
  assert(domain->owner==wnd);   // window isn't a focus manager
#if 0
if (domain->first_child)
  cout << "uh, remaining domain for window " << domain->first_child->owner->getTitle() << endl;
#endif
  assert(domain->first_child==NULL);  // should be removed already
  
  // remove `domain' from the parent domain
  //----------------------------------------
  TDomain *p1 = domain->parent->first_child;
  TDomain *p2;
  
  if (p1==domain) {
    domain->parent->first_child = p1->next;
  } else {
    while(true) {
      p2 = p1->next;
      assert(p2);               // couldn't find domain in parent
      if (p2==domain) {
        p1->next = p2->next;
        break;
      }
      p1 = p2;
    }
  }
  
  // when necessary modify the parent domains focus
  //-----------------------------------------------
  if (domain->parent->focus == domain)
    domain->parent->focus = domain->parent->first_child;

  // okay, remove the domains data
  //------------------------------
  delete domain;
}

TDomain*
GetDomain(TWindow *wnd)
{
  assert(wnd!=NULL);

  // this method is called recursive and the recursion stops when we've
  // reached the top level window 
  //-------------------------------------------------------------------
  if (wnd->bShell) {
    TDomainMap::iterator p = top_domain_map.find(wnd);
    assert(p!=top_domain_map.end());  // shell without top domain
    return (*p).second;
  }

  TDomain *domain = GetDomain(wnd->getParent());

  // in most situations the domain we wanted to find is now available in
  // `domain'; but when we encounter a chance for a sub domain on our way 
  // down from the top level window, we do a loop to find the sub domain 
  // and return it instead of the domain found earlier
  //--------------------------------------------------------------------
  if (wnd->bFocusManager) {
    TDomain *p = domain->first_child;
    while(p) {
      if (p->owner == wnd)
        return p;
      p = p->next;
    }
  }
  return domain;
}

// returns the root domain of wnd's focus tree
TDomain*
GetTopDomain(TWindow *wnd)
{
  assert(wnd!=NULL);
  
  while(wnd->getParent() && !wnd->bShell)
    wnd = wnd->getParent();
    
  TDomainMap::iterator dp = top_domain_map.find(wnd);
  assert(dp!=top_domain_map.end());
  return (*dp).second;
}

//! Walk the tree for focus traversal.
TWindow*
Walk(TWindow *top, TWindow *focus, bool bNext)
{
  assert(top!=NULL);
  
  TWindow *old_focus = focus;
  
  if (!focus)
    focus=top;

  TInteractor *ptr = focus;
  
  do {
    if ( ptr->getFirstChild() &&      // go down when `ptr' has a child
         !(ptr->bShell&&ptr!=top) &&  // but skip alien focus domains
         ptr->bFocusTraversal &&      // and windows that don't want to be part of the focus traversal
         ptr->isRealized() &&         // and when the window is mapped
         ( ptr->bNoFocus              // and only go down when window rejects focus
           || ptr->bFocusManager      //   or when it's a focusmanager
           || ptr->bShell             //   or a shell window
        ))
    {
      // go one step down
      bNext ? ptr = ptr->getFirstChild() : ptr = ptr->getLastChild();
    } else {
      // couldn't go downwards and can't go sideways or up either because
      // `ptr' is the `top' window and we have to stay in the tops' subtree
      // so leave =>
      if (ptr==top)
        return NULL;
        
      // try to get the next/previous sibling of the window; when there is
      // no such sibling walk upwards until there's one
      TInteractor *next;
      while( bNext ? (next=TWindow::getNextSibling(ptr))==NULL
                   : (next=TWindow::getPrevSibling(ptr))==NULL )
      {
        // no next/prev sibling so go one step upwards
        ptr = ptr->getParent();
        
        // when we've reached the top of the tree, leave the loop to
        // walk down again
        if (ptr==top) {
          next = top;
          break;
        }
      }
      ptr = next;
      // home run, we haven't found another focus window => leave
      if (ptr==focus)
        return old_focus;
    }
  } while(
      ptr->bNoFocus ||          // continue when window doesn't want the focus
      !ptr->isRealized() ||     // or isn't mapped
      (ptr!=top && ptr->bShell) // continue when window belongs to another domain
  );
    
  return dynamic_cast<TWindow*>(ptr);
}

#ifdef __X11__

/*
 *
 * The X Input Context is bound to the focus management so it's located here:
 *
 */

// Find and create an X11 Input Context
//---------------------------------------------------------------------------
void
TOADBase::initXInput()
{
  int idx, quality;

  if (!XSupportsLocale()) {
    cerr << "X doesn't support locale " << setlocale(LC_ALL,NULL) << endl;
    return;
  }

  if (XSetLocaleModifiers("")==NULL) {
    cerr << "Can't set locale modifier" << endl;
    return;
  }
  
  if ((xim = XOpenIM(toad::x11display, NULL, NULL, NULL))==NULL) {
    cerr << "Failed to open X Input Method for locale " << 
            setlocale(LC_ALL,NULL) << endl;
    return;
  }

  XIMStyles *xim_styles=NULL;

  if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL)) {
    cerr << "XGetIMValues failed" << endl;
    goto error1;
  }
  if (!xim_styles) {
    cerr << "Input method doesn't support any style" << endl;
    goto error1;
  }

  idx = -1;
  quality = 0;
  for (unsigned i=0; i<xim_styles->count_styles; i++) {
    if (quality<5 && xim_styles->supported_styles[i] & 
          (XIMPreeditNone|XIMStatusNone|XIMPreeditNothing|XIMStatusNothing) )
    {
      idx=i; quality = 5;
    }
    if (quality<10 && xim_styles->supported_styles[i] &
          (XIMPreeditNone|XIMStatusNone) )
    {
      idx=i; quality = 10;
    }
    if (quality<15 && xim_styles->supported_styles[i] &
          (XIMPreeditNothing|XIMStatusNothing) )
    {
      idx=i; quality = 15;
    }
  }

  if (idx==-1) {
    cerr << "Didn't found a X Input Method TOAD can handle" << endl;
    goto error1;
  }

  xim_style = xim_styles->supported_styles[idx];

  if (xim_styles)
    XFree(xim_styles);

  return;

error1:
  if (xim_styles)
    XFree(xim_styles);
  if (xim)
    XCloseIM(xim);  
  xim = NULL;
}

void
TOADBase::closeXInput()
{
  if (xim)
    XCloseIM(xim);
  xim = NULL;
}

#endif

// key filter stuff
//---------------------------------------------------------------------------

/**
 * Insert a <code>TEventFilter</code> into the keyboard handling.
 *
 * A keyboard filter allows you to handle keyDown and keyUp event
 * before they are deliverd to a window. TOAD for example uses them
 * to provide modal dialogs where only one window of the application
 * will handle keyboard events.
 *
 * It's guaranted that the handler added last will be tried first.
 *
 * @param flt
 *    The keyfilter to be added.
 * @param window
 *    A window or NULL.
 * @param pos
 *    @li KF_GLOBAL: Keyfilter to be called first: Before all other
 *       keyfilters and windows, no matter which window owns the
 *       keyboard focus.
 *    @li KF_TOP_DOMAIN: Keyfilter is called when the top level window
 *       <code>wnd</code> belongs to owns the focus.
 *
 * @sa removeKeyFilter
 *
 * @todo 
 * @li Add check that filter isn't already in the list.
 * @li Modal dialog currently don't use this. And I guess that I have
 *     to add a 'destination window' parameter to <code>TEventFilter</code>
 */
void 
TOADBase::insertEventFilter(TEventFilter *flt, TWindow *wnd, EEventFilterPos pos)
{
  flt->pos = pos;
  switch(pos) {
    case KF_GLOBAL:
      flt->next = toad::global_evt_filter;
      toad::global_evt_filter = flt;
      break;
    case KF_TOP_DOMAIN: {
        TDomain *domain = GetTopDomain(wnd);
        if (domain) {
          flt->next = domain->filter;
          flt->ptr  = domain;
          domain->filter = flt;
        }
      } break;
    case KF_DOMAIN:
      cerr << __FILE__ << ":" 
           << __LINE__ << " in " 
           << __PRETTY_FUNCTION__ << " isn't implemented yet" << endl;
      break;
    case KF_WINDOW:
      cerr << __FILE__ << ":" 
           << __LINE__ << " in " 
           << __PRETTY_FUNCTION__ << " isn't implemented yet" << endl;
      break;
  }
}

static void
remove_from_list(TEventFilter **p1, TEventFilter *f)
{
  if (*p1==f) {
    *p1 = f->next;
    return;
  }
  
  TEventFilter **p2;
  do {
    p2=&((*p1)->next);
    if (*p2==f) {
      (*p1)->next=(*p2)->next;
      return;
    }
    p1=p2;
  } while((*p2)->next);
}

/**
 * @sa insertKeyFilter
 */
void
TOADBase::removeEventFilter(TEventFilter *flt)
{
  switch(flt->pos) {
    case KF_GLOBAL:
      remove_from_list(&toad::global_evt_filter, flt);
      break;
    case KF_TOP_DOMAIN:
      remove_from_list(&((TDomain*)flt->ptr)->filter, flt);
      break;
    case KF_DOMAIN:
      cerr << __FILE__ << ":" 
           << __LINE__ << " in " 
           << __PRETTY_FUNCTION__ << " isn't implemented yet" << endl;
      break;
    case KF_WINDOW:
      cerr << __FILE__ << ":" 
           << __LINE__ << " in " 
           << __PRETTY_FUNCTION__ << " isn't implemented yet" << endl;
      break;
  }
}

} // namespace toad
