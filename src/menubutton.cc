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

#define DBM_FEEL(A)

#include <toad/menubutton.hh>
#include <toad/popup.hh>
#include <toad/action.hh>

using namespace toad;

/**
 *
 * \param p  The menubutton's parent
 */
TMenuButton::TMenuButton(TMenuHelper *p, TMenuHelper::TNode *n):
  TWindow(p, n->getTitle()), node(n), master(p)
{
  assert(master!=NULL);
//cout << "MENUBUTTON: CREATE  " << this << endl;
  setBorder(0);
  bNoBackground = true;
  _down = false;
  popup = NULL;
  idx = 0;
#warning "TMenuButton doesn't see when node was changed"
  CONNECT(node->sigChanged, this, adjust);
}

TMenuButton::~TMenuButton()
{
//cout << "MENUBUTTON: DESTROY " << this << endl;
//if (Title()=="open") {
//  cout << "UPSI!" << endl;
//}

//  printf("%s\n", __PRETTY_FUNCTION__);
  DISCONNECT(node->sigChanged, this, adjust);
}

void TMenuButton::adjust()
{
  if (node->type==TMenuHelper::TNode::SEPARATOR) {
    setSize(1,4);
    return;
  }

  int w,h, n;
  
  w=0;
  h = TOADBase::getDefaultFont().getHeight();
  
  n = drawIcon();
  master->menu_width_icon = max(master->menu_width_icon, n);
  w+=n;
  
  n = TOADBase::getDefaultFont().getTextWidth(node->getLabel(idx));
  master->menu_width_text = max(master->menu_width_text, n);
  w+=n;
 
  if (node->getShortcut().size()!=0) {
    n = TOADBase::getDefaultFont().getTextWidth(node->getShortcut());
    master->menu_width_short = max(master->menu_width_short, n);
    w+=n;
  }
#if 1
  if (master->vertical) {
    w += 12+2;
    h += 2;
  } else {
    w += 12+8;
    h += 2+8;
  }

  setSize(w,h);
#endif
}

/**
 * Draws an icon when <code>pen<code> isn't NULL and return the size of
 * the icon.
 *
 * \param pen NULL or a pen.
 * \param x   The x-coordinate.
 * \param y   The y-coordinate.
 *
 * \todo
 *   The interface isn't perfect yet:
 *   \li How about one parameter being a rectangle within the function
 *       has to place the icon?
 */
unsigned
TMenuButton::drawIcon(TPen *pen, int x, int y)
{
  if (pen && node->getIcon()) {
    pen->drawBitmap(x, y, node->getIcon());
  }
  
  if (node->getIcon())
    return node->getIcon()->getWidth();
  return 0;
}

void 
TMenuButton::paint()
{
//cout << "painting " << Title() << endl;
  TPen pen(this);

  bool mark = false;
  if (master->active==this) {
    switch(master->state) {
      case MHS_DOWN:
      case MHS_UP_N_HOLD:
      case MHS_DOWN_N_HOLD:
      case 4:
      case MHS_DOWN_N_INSIDE_AGAIN:
        mark=true;
        break;
      case MHS_DOWN_N_OUTSIDE:
        mark=node->down && node->isEnabled(); // same cond as in openPopup
    }
  }

  int x, y;
  if (master->vertical) {
    x = 3;
    y = 1;
  } else {
    x = 6+4-8;
    y = 1+4;
  }

#if 1
  // debug code: check `isAvailable()'
  if (!node->isAvailable()) {
    pen.setColor(0,128,0);
    pen.fillRectangle(0,0, getWidth(), getHeight());
    return;
  }
#endif

  const string& label = node->getLabel(idx);
  const string& shortcut = node->getShortcut();

  if (node->type==TMenuHelper::TNode::SEPARATOR) {
    pen.setColor(TColor::MENU);
    pen.fillRectangle(0,0,getWidth(), getHeight());
    int y = getHeight()/2;
    pen.setColor(TColor::BTNSHADOW);
    pen.drawLine(0, y, getWidth(), y);
    pen.setColor(TColor::BTNLIGHT);
    y++;
    pen.drawLine(0, y, getWidth(), y);
    return;
  }

  int x_icon = x;
  int x_text = x_icon + master->menu_width_icon+8;
  int x_short= x_text + master->menu_width_text+8;
  
  if (node->isEnabled()) {
    pen.setColor(mark ? TColor::MENUTEXT : TColor::MENU);
    pen.fillRectangle(0,0, getWidth(),getHeight());
    pen.setColor(mark ? TColor::MENU : TColor::MENUTEXT);
    drawIcon(&pen, x_icon, y);
    pen.drawString(x_text, y, label);
    if (!shortcut.empty())
      pen.drawString(x_short, y, shortcut);
  } else {
    pen.setColor(TColor::MENU);
    pen.fillRectangle(0,0,getWidth(), getHeight());

    pen.setColor(TColor::BTNLIGHT);
    pen.drawString(x_text+1, y+1, label);
    if (!shortcut.empty())
      pen.drawString(x_short+1, 1+y, shortcut);

    pen.setColor(TColor::BTNSHADOW);
    drawIcon(&pen, x_icon, y);
    pen.drawString(x_text, y, label);
    if (shortcut.size()!=0)
      pen.drawString(x_short, y, shortcut);
  }
  
  if(node->down && master->vertical) {
    int y=_h>>1;
    TPoint tri[3];
    tri[0].x=_w-7; tri[0].y=y-4;
    tri[1].x=_w-3; tri[1].y=y;
    tri[2].x=_w-7; tri[2].y=y+4;
    pen.fillPolygon(tri, 3);
  }
}

void 
TMenuButton::closeRequest()
{
  deactivate();
  collapse();
  dropKeyboard();
  master->state=MHS_WAIT;
}

namespace toad {
  class TMenuKeyFilter:
    public TEventFilter
  {
    public:
      TMenuButton *active;
      bool keyEvent(TKeyEvent&);
  };
}

bool 
TMenuKeyFilter::keyEvent(TKeyEvent &ke)
{
  TKey key = ke.key;
  
  if (!active) {
    if (toad::debug_menubutton) {
      cout << "KEYDOWN (active=NULL)" << endl;
    }
    return true;
  }
  
  if (toad::debug_menubutton) {
    cout << "KEYDOWN (active=" << active->getTitle() << ")" << endl;
  }

  #warning "better to make sure that mouse button isn't pressed?"

  if(active->master->vertical) {
    switch(key) {
      case TK_RIGHT:
        key = TK_DOWN;
        break;
      case TK_LEFT:
        key = TK_UP;
        break;
      case TK_UP:
        key = TK_LEFT;
        break;
      case TK_DOWN:
        key = TK_RIGHT;
        break;
    }
  }

  TMenuButton *i=NULL;
  TMenuButton *m;
  TMenuHelper::TNode *ptr;
  
  switch(key) {
    case TK_SPACE:
    case TK_RETURN:
      active->trigger();
#if 0
      // causes segfault
      if (active->master)
        active->master->state=MHS_WAIT;
#endif
      active->dropKeyboard();
      break;
    case TK_RIGHT:
      // use the order of nodes to find the next window
      // (make a subroutine out of it and add a helper function to
      // node as a node can contain multiple windows in the future!)
      ptr = active->node->next;
      while(ptr) {
#if 1
        if (ptr->winarray && ptr->nwinarray>=1) {
          i = ptr->winarray[0];
          break;
        }
#else
        if (ptr->window && ptr->isEnabled()) {
          i = ptr->window;
          break;
        }
#endif
        ptr=ptr->next;
      }
      break;
    case TK_LEFT:
#if 0 
      ptr = active->node->parent->down;
      while(ptr) {
#if 1
        TWindow * window = 0;
        if (ptr->winarray && ptr->nwinarray>0)
          window = ptr->winarray[0];
        if (window == active)
          break;
        if (window && ptr->isEnabled()) {
          i = window;
        }
#else
        if (ptr->window == active)
          break;
        if (ptr->window && ptr->isEnabled()) {
          i = ptr->window;
        }
#endif
        ptr=ptr->next;
      }
      // no left window, one level up
      if (i==NULL || ptr->window == i) {
        cout << "UP" << endl;
        if (active->master->master) {
          TMenuButton *old = active;
          active=active->master->master;
          old->master->active=NULL;
          old->invalidate();
          old->closePopup();
          active->activate();
        }
        return true;
      }
#endif
      break;
  }
  
  if (i) {
    if (active->master->active)
      active->master->active->deactivate();
    i->activate();
    active->master->state=MHS_UP_N_HOLD;
    return true;
  }
  
  switch(key) {
    case TK_DOWN:
#if 0
cout << "TK_DOWN" << endl;
      ptr = active->node->down;
      while(ptr) {
#if 1
        Window * window = 0;
        if (ptr->winarray && ptr->nwinarray>0)
          window = ptr->winarray[0];
        if (window && ptr->isEnabled()) {
          i = window;
          break;
        }
#else
        if (ptr->window && ptr->isEnabled()) {
          i = ptr->window;
          break;
        }
#endif 
        ptr = ptr->next;
      }
#endif
      break;
  }
  
  if (i) {
    i->grabKeyboard();
    i->activate();
    i->master->state=MHS_UP_N_HOLD;
  }
  return true;
}


static TMenuKeyFilter *keyfilter = NULL;

void 
TMenuButton::grabKeyboard()
{
  if (keyfilter==NULL) {
//    if (toad::debug_menubutton) {
//      cout << "ADDING KEYFILTER" << endl;
//    }
    keyfilter = new TMenuKeyFilter();
    insertEventFilter(keyfilter, NULL, KF_GLOBAL);
  }
}

void 
TMenuButton::dropKeyboard()
{
  if (keyfilter!=NULL) {
//    if (toad::debug_menubutton) {
//      cout << "REMOVING KEYFILTER" << endl;
//    }
    removeEventFilter(keyfilter);
    delete keyfilter;
    keyfilter=NULL;
  }
}

/**
 * When receiving a closeRequest message we do call Collapse() to deactivate
 * the whole menubar. Collapse() sets stop_collapse to `false' and appends a
 * message to TOADs message queue, which will perform the real job of
 * deactivation.
 * Now in case we receive a mouseLDown message, stop_collapse will be set to
 * `true', which will disable the deactivation.
 */
static bool stop_collapse;

void 
TMenuButton::mouseLDown(int,int,unsigned)
{
  stop_collapse=true;
  if (toad::debug_menubutton) {
    cout << "+ mouseLDown " << this << ": state " << master->state << endl;
  }
  switch(master->state) {
    case MHS_WAIT:
      activate();
      master->state=MHS_DOWN;
      break;
    case MHS_UP_N_HOLD:
      if (master->active!=this) {
        activate();
      } else {
        master->state=MHS_DOWN_N_HOLD;
      }
      break;
    default:
      cout << __PRETTY_FUNCTION__ << ": unexpected state " << master->state << endl;
  }
  if (toad::debug_menubutton) {
    cout << "- mouseLDown " << this << ": state " << master->state << endl;
  }
}



void 
TMenuButton::mouseLUp(int,int,unsigned)
{
  if (toad::debug_menubutton) {
    cout << "+ mouseLUp " << this << ": state " << master->state << endl;
  }
  switch(master->state) {
    case MHS_DOWN:
      if (
        master->btnmaster // this should be always true: master->master->state=2
        && !(node->down && node->isEnabled()) // must be same cond as in OpenPopup
         ) 
      {
        dropKeyboard();
        trigger();
        master->state=MHS_WAIT;
      } else {
        master->state=MHS_UP_N_HOLD;
      }
      break;
    case MHS_DOWN_N_HOLD:
    case MHS_DOWN_N_OUTSIDE:
      deactivate();
      collapse();
      dropKeyboard();
      master->state=MHS_WAIT;
      if (master->close_on_close)
        master->destroyWindow(); 
      break;
    case MHS_DOWN_N_INSIDE_AGAIN:
      dropKeyboard();
      trigger();
      break;
    default:
      cout << __PRETTY_FUNCTION__ << ": unexpected state " << master->state << endl;
  }
  if (toad::debug_menubutton) {
    cout << "- mouseLUp " << this << ": state " << master->state << endl;
  }
}

void 
TMenuButton::mouseRDown(int x, int y, unsigned m)
{
  mouseLDown(x, y, m);
}

void 
TMenuButton::mouseRUp(int x, int y, unsigned m)
{
  mouseLUp(x, y, m);
}

void 
TMenuButton::mouseLeave(int,int,unsigned m)
{
  if (toad::debug_menubutton) {
    cout << "+ mouseLeave " << this << ": state " << master->state << endl;
  }
  switch(master->state) {
    case MHS_WAIT:
    case MHS_DOWN_N_OUTSIDE:
    case MHS_UP_N_HOLD:
      break;
    case MHS_DOWN:
    case MHS_DOWN_N_INSIDE_AGAIN:
      master->state=MHS_DOWN_N_OUTSIDE;
      invalidateWindow();
      break;
    default:
      cout << __PRETTY_FUNCTION__ << ": unexpected state " << master->state << endl;
  }
  if (toad::debug_menubutton) {
    cout << "- mouseLeave " << this << ": state " << master->state << endl;
  }
}

void TMenuButton::mouseEnter(int,int,unsigned m)
{
  if (toad::debug_menubutton) {
    cout << "+ mouseEnter " << this << ": state " << master->state << endl;
  }
#warning "menu starts just by moving into with pressed button, check upper button!"
  switch(master->state) {
    case MHS_WAIT:
    case MHS_UP_N_HOLD:
    case MHS_DOWN_N_OUTSIDE:
      if (m&(MK_LBUTTON|MK_RBUTTON) && node->isEnabled()) {
        if (master->active)
          master->active->deactivate();
        activate();
        master->state = MHS_DOWN_N_INSIDE_AGAIN;
      }
      break;
    default:
      cout << __PRETTY_FUNCTION__ << ": unexpected state " << master->state << endl;
  }
  if (toad::debug_menubutton) {
    cout << "- mouseEnter " << this << ": state " << master->state << endl;
  }
}

void 
TMenuButton::closePopup()
{
  if (popup) {
    delete popup;
    popup = NULL;
  }
}

void 
TMenuButton::openPopup()
{
  if (node->down && node->isEnabled() && popup==NULL) {
    popup = new TPopup(this, "popup");
    popup->btnmaster = this;
    popup->root.down = node->down;
    int x,y;
    getRootPos(&x, &y);
    if (master->vertical)
      popup->setPosition(x+getWidth(), y);
    else
      popup->setPosition(x, y+getHeight());
    popup->setSize(80,200);
    popup->createWindow();
  }
}

void 
TMenuButton::activate()
{
//  GrabPopupMouse(TMMM_PREVIOUS, TCursor::MOVE);
  grabPopupMouse(TMMM_PREVIOUS);
  grabKeyboard();
  if (toad::debug_menubutton) {
    cout << "grabbed " << this << endl;
    DBM_FEEL(cout << "grab " << getTitle() << endl;)
  }

  if (master->active && master->active!=this) {
    master->active->closePopup();
    master->active->invalidateWindow();
  }
  master->active = this;
  if (keyfilter)
    keyfilter->active = this;
  invalidateWindow();

  openPopup();
}

void
TMenuButton::deactivate()
{
  if (master->active) {
    master->active->closePopup();
    master->active->invalidateWindow();
  }
  master->active = NULL;
  if (keyfilter)
    keyfilter->active = NULL;
  
  ungrabMouse();
  if (toad::debug_menubutton) {
    cout << "ungrabbed " << this << endl;
    DBM_FEEL(cout << "ungrab " << getTitle() << endl;)
  }
  closePopup();
  master->state = MHS_WAIT;
}

namespace toad {
  class TCommandCollapseMenu:
    public TCommand
  {
      TMenuButton *mb;
    public:
      TCommandCollapseMenu(TMenuButton *m):mb(m) {}
      void execute() {
        if (!stop_collapse) {
          mb->deactivate();
          mb->master->state = MHS_WAIT;  // right position?
          mb->collapse();
          
          if (mb->master->close_on_close)
            mb->master->destroyWindow(); 
        }
      }
  };
}

void
TMenuButton::trigger()
{
  node->trigger(idx);
  deactivate();
  collapse();
}

void TMenuButton::collapse()
{
  stop_collapse = false;
  // is there an upper menubuton (types are: TMenuHelper->TMenuButton)
  if (master->btnmaster)
    sendMessage(new TCommandCollapseMenu(master->btnmaster));
}

unsigned
TMenuRadioButton::drawIcon(TPen *pen, int x, int y)
{
  if (pen && choice->getSelection()==idx) {
    pen->fillCircle(x+1,getHeight()/2-3,6,6);
  }
  return 6;
}
