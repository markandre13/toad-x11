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

#include <toad/toad.hh>
#include <toad/figure.hh>
#include <toad/figureeditor.hh>
#include <toad/colordialog.hh>
#include <toad/scrollbar.hh>
#include <toad/checkbox.hh>
#include <toad/action.hh>
#include <toad/figureeditor/undoablemove.hh>
#include <toad/figureeditor/undoablehandlemove.hh>
#include <toad/figureeditor/undoabledelete.hh>
#include <toad/figureeditor/undoablecreate.hh>

#include <algorithm>

using namespace toad;

#define DBM(CMD)
#define VERBOSE 0

/**
 * \class toad::TFigureEditor
 * TFigureEditor is a common graphical editor for TFigure objects.
 * 
 * It's still experimental so expect major changes in the future before
 * using it. One major goal is to make it possible to edit scaleable and
 * rotateable 2D gadgets and to create 3D objects.
 * <P>
 * Originally written for the dialog editor, it can be used for lots of
 * other jobs, e.g. a MacOS(tm) alike file folder.
 * <P>
 * TFigureEditor can be used as a window of its own or just as an object 
 * to delegate events from other windows to. The later was needed for the
 * dialog editor.
 *
 * Selection Mode
 * \li click           : select and move object
 * \li CTRL+click      : only select object
 * \li SHIFT+click     : add single object to selection
 * \li SHIFT+CTRL+click: remove single object from selection
 *
 * \todo
 *   \li
 *      Make sure that we can have multiple views of one model
 *   \li
 *      Add a selection model
 *   \li
 *      Consider making two classes of this one.
 *   \li
 *      actVScroll & actHScroll called updateScrollbars which lead to
 *      an endless recursion; these calls are currently removed
 *   \li
 *      allow the usage of multiple models to provide layers
 *   \li
 *      TFigureEditor::TColorSelector: the dialogs set the colors even
 *      when [Abort] was pressed.
 *   \li
 *      TFigureEditor::TColorSelector: should add a third mode:
 *      outline, filled, filled & outline == fillcolor
 *   \li
 *      segfault after certain number of undos
 *   \li
 *      color changes aren't part of undo/redo
 */

/**
 * This constructer is to be used when TFigureEditor isn't used as a
 * window itself but handles events delegated to it from another window.
 *
 * This feature 
 *
 * \sa setWindow
 */
TFigureEditor::TFigureEditor():
  super(NULL, "(TFigureEditor: no window)")
{
  init();
  bExplicitCreate = true; // don't create, see TWindow::createParentless()
  window = NULL;
}

/**
 * Handle events for another window.
 *
 * This feature was added for the internal dialogeditor.
 *
 * \param w Then window to handle.
 */
void
TFigureEditor::setWindow(TWindow *w)
{
  // [store gadgets to `window']
  if (window)
    window->invalidateWindow();
  window = w;
  // [get gadgets from `window']
  if (window)
    window->invalidateWindow();
}

TFigureEditor::TFigureEditor(TWindow *p, const string &t):
  super(p, t)
{
  init();
  setMouseMoveMessages(TMMM_LBUTTON);
  bNoBackground = true;
  window = this;
}

void
TFigureEditor::init()
{
  line_color.set(0,0,0);
  fill_color.set(255,255,255);
  filled = false;
  background_color.set(192,192,192);

  gridx = gridy = 4;
  draw_grid = true;
  handle = -1;
  gadget = gtemplate = NULL;
  operation = OP_SELECT;
  state = STATE_NONE;
  use_scrollbars = true;
  vscroll = NULL;
  hscroll = NULL;
  x1=y1=x2=y2=0;
  update_scrollbars = false;

  TAction *action;
/*
  action = new TAction(this, "edit|cut");
  CONNECT(action->sigActivate, this, _selection_cut);
  action = new TAction(this, "edit|copy");
  CONNECT(action->sigActivate, this, _selection_copy);
  action = new TAction(this, "edit|paste");
  CONNECT(action->sigActivate, this, _selection_paste);
*/
  action = new TAction(this, "edit|delete");
  CONNECT(action->sigActivate, this, deleteSelection);

  action = new TAction(this, "edit|undo");
  CONNECT(action->sigActivate, this, undo);
  action = new TAction(this, "edit|redo");
  CONNECT(action->sigActivate, this, redo);
}

TFigureEditor::~TFigureEditor()
{
//  SetMode(MODE_SELECT);
//  cout << "gadgets total: " << gadgets.size() << endl;
}

bool
TFigureEditor::restore(TInObjectStream &in)
{
  clearSelection();
  super::restore(in);
}

/**
 * \param b 'true' if scrollbars shall be used.
 */
void
TFigureEditor::enableScroll(bool b)
{
  use_scrollbars = b;
}

/**
 * \param b 'true' if a grid shall be drawn into the window.
 */
void
TFigureEditor::enableGrid(bool b)
{
  draw_grid = b;
}

/**
 * Set the background color.   
 */
void
TFigureEditor::setBackground(int r,int g,int b)
{
  background_color.set(r,g,b);
}

void
TFigureEditor::resize()
{
  updateScrollbars();
}

void
TFigureEditor::paint()
{
  if (!window) {
    cout << __PRETTY_FUNCTION__ << ": no window" << endl;
    return;
  }
  if (update_scrollbars) {
    // cout << "paint: update_scrollbars" << endl;
    updateScrollbars();
    update_scrollbars = false;
  }
  TBitmap bmp(window->getWidth(), window->getHeight(), TBITMAP_SERVER);
  TPen pen(&bmp);

  // prepare the pen to draw a grid
  if (draw_grid && gridx && gridy) {
    TBitmap bitmap(gridx,gridy, TBITMAP_SERVER);
    TPen bpen(&bitmap);
    bpen.setColor(background_color);
    bpen.fillRectangle(0,0,gridx,gridy);
    bpen.setColor(
      background_color.r > 128 ? background_color.r-128 : background_color.r+128,
      background_color.g > 128 ? background_color.g-128 : background_color.g+128,
      background_color.b > 128 ? background_color.b-128 : background_color.b+128
    );
    int ox = window->getOriginX() % gridx;
    if (ox<0)
      ox+=gridx;
    int oy = window->getOriginY() % gridy;
    if (oy<0)
      oy+=gridy;
    
    bpen.drawPoint(ox, oy);
    pen.setBitmap(&bitmap);
  } else {
    pen.setColor(background_color);
  }
  pen.fillRectangle(0,0,window->getWidth(),window->getHeight());

  pen.setOrigin(window->getOriginX(), window->getOriginY());
  pen.setColor(TColor::BLACK);
  
  TFigureModel::iterator p, e;
  
  // draw the gadgets
  p = gadgets->begin();
  e = gadgets->end();
  while(p!=e) {
    TFigure::EPaintType pt = TFigure::NORMAL;
    if (gadget==*p) {
      pt = TFigure::EDIT;
    } else if (selection.find(*p)!=selection.end()) {
      pt = TFigure::SELECT;
    }
    (*p)->paint(pen, pt);
    ++p;
  }

  // draw the selection marks over all gadgets
  pen.setColor(0,0,0);
  TGadgetSet::iterator sp,se;
  sp = selection.begin();
  se = selection.end();
  while(sp!=se) {
    (*sp)->paintSelection(pen);
    ++sp;
  }

  // draw the litle gray box between the two scrollbars (when we have
  // two of 'em)
  if (vscroll && vscroll->isMapped() &&
      hscroll && hscroll->isMapped() )
  {
    TRectangle r(window->getWidth() - TScrollBar::getFixedSize(),
                 window->getHeight()- TScrollBar::getFixedSize(),
                 TScrollBar::getFixedSize(), TScrollBar::getFixedSize());
    pen.setColor(TColor::LIGHTGRAY);
    pen.setOrigin(0,0);
    pen|=r;
    pen.fillRectangle(r);
  }

  // put the result onto the screen
  TPen scr(window);
  scr.drawBitmap(-window->getOriginX(),-window->getOriginY(), &bmp);
}

void
TFigureEditor::setLineColor(const TRGB &rgb)
{
  line_color = rgb;
  TGadgetSet::iterator p,e;
  p = selection.begin();
  e = selection.end();
  while(p!=e) {
    (*p)->line_color = line_color;
    p++;
  }
  if (gtemplate)
    gtemplate->line_color = line_color;
  window->invalidateWindow();
}

void
TFigureEditor::setFillColor(const TRGB &rgb)
{
  fill_color = rgb;
  TGadgetSet::iterator p,e;
  p = selection.begin();
  e = selection.end();
  while(p!=e) {
    (*p)->fill_color = fill_color;
    p++;
  }
  if (gtemplate)
    gtemplate->fill_color = fill_color;
  window->invalidateWindow();
}

void
TFigureEditor::setFilled(bool b)
{
  filled = b;
  TGadgetSet::iterator p,e;
  p = selection.begin();
  e = selection.end();
  while(p!=e) {
    (*p)->filled = b;
    p++;
  }
  if (gtemplate)
    gtemplate->filled = b;
  window->invalidateWindow();
}

void
TFigureEditor::add(TFigure *g)
{
  gadgets->add(g);
  update_scrollbars = true;
  invalidateFigure(g);
}

void
TFigureEditor::deleteGadget(TFigure *g)
{
  if (g==gadget)
    gadget=NULL;
  if (g==gtemplate)
    gtemplate=NULL;
  
  TFigureModel::iterator p,e;
  p = gadgets->begin();
  e = gadgets->end();
  while(p!=e) {
    if (g==*p) {
      gadgets->erase(p);
      break;
    }
    ++p;
  }

  TGadgetSet::iterator s;
  s = selection.find(g);
  if (s!=selection.end())
    selection.erase(s); 

  delete g;
}

void
TFigureEditor::clearSelection()
{
  selection.erase(selection.begin(), selection.end());
  window->invalidateWindow();
}

/**
 * Delete all selected objects with `removeable' being true.
 */
void
TFigureEditor::deleteSelection()
{
//cout << "delete selection" << endl;
  history.add(new TUndoableDelete(*gadgets, selection));
//cout << "selection size: " << selection.size() << endl;

  TFigureModel::iterator p,e,del;
  p = gadgets->begin();
  e = gadgets->end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end() &&
        (*p)->removeable )
    {
      del = p;
      ++p;
      if (gadget==*del)
        gadget=NULL;
//      cout << "removing gadget " << *del << endl;
      gadgets->erase(del);
    } else {
      p++;
    }
  }

  clearSelection();
  update_scrollbars = true;
  window->invalidateWindow();
}

void
TFigureEditor::selectAll()
{
  TFigureModel::iterator p,e;
  p = gadgets->begin();
  e = gadgets->end();
  while(p!=e) {
    selection.insert(*p);
    ++p;
  } 
}

void
TFigureEditor::deleteAll()
{
  selectAll();
  deleteSelection();
  setOrigin(0,0);
  if (vscroll)
    vscroll->setValue(0);
  if (hscroll)
    hscroll->setValue(0);
  updateScrollbars();
}

void
TFigureEditor::selection2Top()
{
  TFigureModel::iterator p,b,np;
  p = gadgets->end();
  b = gadgets->begin();

  if (p==b)
    return;

  --p; np=p;                      // p, np @ last element

  if (p==b)
    return;

  while(true) {
    if (selection.find(*p)!=selection.end()) {
      if (p!=np) {
        TFigure *akku = *p;
        TFigureModel::iterator mp = p;
        while(mp!=np) {
          TFigureModel::iterator op = mp;
          ++mp;
          *op = *mp;
        }
        *np = akku;
      }
      --np;
    }
    if (p==b)
      break;
    --p;
  }
  
  window->invalidateWindow();
}

void
TFigureEditor::selection2Bottom()
{
  TFigureModel::iterator p, e, np;
  p = np = gadgets->begin();
  e = gadgets->end();
  if (p==e)
    return;

  if (selection.find(*p)!=selection.end())
    ++np;
  ++p;

  while(p!=e) {
    // if *p is in the list
    if (selection.find(*p)!=selection.end()) {
      TFigure *akku = *p;
      TFigureModel::iterator mp = p;
      while(mp!=np) {
        TFigureModel::iterator op = mp;
        --mp;
        *op = *mp;
      }
      *np = akku;
      ++np;
    }
    ++p;
  }
  window->invalidateWindow();
}

void
TFigureEditor::selectionUp()
{
  TFigureModel::iterator p,e,b,prev;
  p = e = prev = gadgets->end();
  b = gadgets->begin();
  if (p==b)
    return;
  while(true) {
    if (selection.find(*p)!=selection.end()) {
      if (prev!=e) {
        TFigure* a = *p;
        *p = *prev;
        *prev = a;
      }
      prev = e;
    } else {
      prev = p;
    }
    if (p==b)
      break;
    --p;
  }
  window->invalidateWindow();
}

void
TFigureEditor::selectionDown()
{
  TFigureModel::iterator p,e,prev;
  p = gadgets->begin();
  e = prev = gadgets->end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end()) {
      if (prev!=e) {
        TFigure* a = *p;
        *p = *prev;
        *prev = a;
      }
      prev=e;
    } else {
      prev=p;
    }
    ++p;
  }
  window->invalidateWindow();
}

void
TFigureEditor::group()
{
  if (selection.size()<2)
    return;
  TFGroup *group = new TFGroup();
  TFigureModel::iterator p,e;

  p = gadgets->begin();
  e = gadgets->end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end()) {
      group->gadgets.add(*p);
      TFigureModel::iterator del = p;
      ++p;
      gadgets->erase(del);
    } else {
      ++p;
    }
  }

  clearSelection();
  group->calcSize();
  gadgets->insert(p, group);
  selection.insert(group);
}

void
TFigureEditor::ungroup()
{
  TFigureModel::iterator p,e;
  p = gadgets->begin();
  e = gadgets->end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end()) {
      TFGroup *group = dynamic_cast<TFGroup*>(*p);
      if (group) {
        TFigureModel::iterator vp,ve;
        vp = group->gadgets.begin();
        ve = group->gadgets.end();
        gadgets->insert(p, vp,ve);
        group->gadgets.erase(group->gadgets.begin(),group->gadgets.end());
        delete group;
        TFigureModel::iterator del = p;
        ++p;
        gadgets->erase(del);
        continue;
      }
    }
    ++p;
  }
  clearSelection();
}

/**
 * Abondon the current mode of operation and select a new mode.
 */
void
TFigureEditor::setOperation(unsigned op)
{
#if VERBOSE
  cout << "Setting Operation " << op << endl;
#endif
  stopOperation();
  if (window)
    window->setFocus();
  operation = op;
}

/**
 * Select 'create object' as the new operation mode.
 *
 * \param t
 *   A template for the object to be created.
 *
 * \todo
 *   t shall be be const
 */
void
TFigureEditor::setCreate(TFigure *t)
{
  if (gtemplate) {
    delete gtemplate;
  }
  TCloneable *clone = t->clone();
  gtemplate = dynamic_cast<TFigure*>(clone);
  if (!gtemplate) {
    cerr << "TFigure::clone() didn't delivered a TFigure" << endl;
    delete clone;
    return;
  }
  gtemplate->line_color = line_color;
  gtemplate->fill_color = fill_color;
  gtemplate->filled     = filled;
  gtemplate->removeable = true;
  clearSelection();
  setOperation(OP_CREATE);
}

/**
 * Abort the current operation mode.
 */
void
TFigureEditor::stopOperation()
{
  switch(state) {
    case STATE_CREATE:
      clearSelection();
      if (gadget) {
        selection.insert(gadget);
        history.add(new TUndoableCreate(*gadgets, selection));
//        clearSelection();
      }
      setMouseMoveMessages(TMMM_ANYBUTTON);
      break;
  }
  gadget = NULL;
  state = STATE_NONE;
}

void
TFigureEditor::keyDown(TKey key, char *s, unsigned m)
{
  if (!window)
    return;

redo:
  switch(operation) {
    case OP_SELECT: {
      if (state!=STATE_EDIT) {
        switch(key) {
          case TK_DELETE:
            deleteSelection();
            break;
        }
        break;
      }
    }
    case OP_CREATE: {
      if (state==STATE_NONE) {
        if (key==TK_DELETE)
          deleteSelection();
        break;
      }
      assert(gadget!=NULL);
      unsigned r = gadget->keyDown(this,key,s,m);
      if (r & TFigure::DELETE)
        deleteGadget(gadget);
      if (r & TFigure::STOP)
        stopOperation();
      if (r & TFigure::REPEAT)
        goto redo;
    } break;
  }
}

void
TFigureEditor::mouseLDown(int mx,int my, unsigned m)
{
  #if VERBOSE
    cout << __PRETTY_FUNCTION__ << endl;
  #endif

  if (!window)
    return;

  int x = ((mx+gridx/2)/gridx)*gridx;
  int y = ((my+gridy/2)/gridy)*gridy;

  down_x = x;
  down_y = y;

  if (window)
    window->setFocus();

  // handle special operation modes
  //--------------------------------
redo:

  switch(state) {

    case STATE_NONE: {
      #if VERBOSE
        cout << "  STATE_NONE" << endl;
      #endif
      switch(operation) {
        case OP_SELECT: {
          #if VERBOSE
            cout << "    OP_SELECT" << endl;
          #endif

          // handle the handles
          //--------------------
          if ( !selection.empty() && !(m&MK_DOUBLE) ) {
            TGadgetSet::iterator p,e;
            p = selection.begin();
            e = selection.end();
            #if VERBOSE
              cout << "      mouse @ " << mx << ", " << my << endl;
            #endif
            while(p!=e) {
              unsigned h=0;
              while(true) {
                if (!(*p)->getHandle(h,memo_pt))
                  break;
                if (memo_pt.x-2<=mx && mx<=memo_pt.x+2 && 
                    memo_pt.y-2<=my && my<=memo_pt.y+2) {
                  #if VERBOSE
                    cout << "      found handle at cursor => STATE_MOVE_HANDLE" << endl;
                  #endif
                  handle = h;
                  #if VERBOSE
                  cout << "      handle " << h << " @ " << memo_pt.x << ", " << memo_pt.y << endl;
                  #endif
                  state = STATE_MOVE_HANDLE;
                  if (selection.size()>1) {
                    TFigure *g = *p;
                    clearSelection();
                    selection.insert(g);
                    sigSelectionChanged();
                  }
                  return;
                }
                h++;
              }
              p++;
            }
          }

          // selection, start movement, start edit
          //--------------------------------------
          TFigure *g = findGadgetAt(mx, my);
          if (g) {
            #if VERBOSE
              cout << "      gadget at cursor";
            #endif
            TGadgetSet::iterator gi = selection.find(g);
            
            if (m & MK_DOUBLE) {
              #if VERBOSE
                cout << ", double click => ";
              #endif
              if (g->startInPlace()) {
                #if VERBOSE
                  cout << "STATE_EDIT" << endl;
                #endif
                clearSelection();
                sigSelectionChanged();
                gadget = g;
                state = STATE_EDIT;
                goto redo;
              }
              #if VERBOSE
                cout << "not editing" << endl;
              #endif
            } else 
            
            if (m & MK_SHIFT) {
              #if VERBOSE
                cout << ", shift => ";
              #endif
              if (! (m&MK_CONTROL)) {
                #if VERBOSE
                  cout << "  adding object to selection" << endl;
                #endif
                if (gi==selection.end())
                  selection.insert(g);
              } else {
                #if VERBOSE
                  cout << " removing object from selection" << endl;
                #endif
                if (gi!=selection.end())
                  selection.erase(gi);
              }
              sigSelectionChanged();
              window->invalidateWindow();
            } else {
              #if VERBOSE
                cout << " => ";
              #endif
              if (gi==selection.end()) {
                clearSelection();
                selection.insert(g);
                sigSelectionChanged();
                if (m&MK_CONTROL) {
                  state =  STATE_SELECT_RECT;
                } else {
                  memo_x = memo_y = 0;
                  state = STATE_MOVE;
                }
              } else {
                if (m&MK_CONTROL) {
                  clearSelection();
                  selection.insert(g);
                  sigSelectionChanged();
                  state =  STATE_SELECT_RECT;
                } else {
                  state = STATE_MOVE;
                  memo_x = memo_y = 0;
                  #if VERBOSE
                    cout << "STATE_MOVE" << endl;
                  #endif
                }
              }
            }
          } else {
            #if VERBOSE
              cout << "      nothing at cursor => STATE_SELECT_RECT" << endl;
            #endif
            if (!(m & MK_SHIFT)) {
              clearSelection();
              sigSelectionChanged();
            }
            state =  STATE_SELECT_RECT;
          }
        } break;

        case OP_CREATE: {
          #if VERBOSE
            cout << "    OP_CREATE => STATE_CREATE" << endl;
          #endif
          clearSelection();
          gadget = static_cast<TFigure*>(gtemplate->clone());
          gadgets->add(gadget);
          window->invalidateWindow();
          state = STATE_START_CREATE;
          setMouseMoveMessages(TWindow::TMMM_ALL);
          gadget->startCreate();
          unsigned r = gadget->mouseLDown(this,x,y,m);
          state = STATE_CREATE;
          if (r & TFigure::DELETE)
            deleteGadget(gadget);
          if (r & TFigure::STOP)
            stopOperation();
          if (r & TFigure::REPEAT)
            goto redo;
          return;
        } break;
      }
    } break;
    
    case STATE_CREATE: 
    case STATE_EDIT: {
      assert(gadget!=NULL);
      #if VERBOSE
      if (state==STATE_CREATE)
        cout << "  STATE_CREATE" << endl;
      else
        cout << "  STATE_EDIT" << endl;
      #endif
      unsigned r = gadget->mouseLDown(this,x,y,m);
      if (r & TFigure::DELETE) {
        #if VERBOSE
          cout << "    delete gadget" << endl;
        #endif
        deleteGadget(gadget);
      }
      if (r & TFigure::STOP) {
        #if VERBOSE
          cout << "    stop" << endl;
        #endif
        stopOperation();
      }
      if (r & TFigure::REPEAT) {
        #if VERBOSE
          cout << "    repeat event" << endl;
        #endif
        goto redo;
      }
    } break;
  }
}


void
TFigureEditor::mouseMove(int x, int y, unsigned m)
{

  #if VERBOSE
    cout << __PRETTY_FUNCTION__ << endl;
  #endif

  if (!window)
    return;

  x = ((x+gridx/2)/gridx)*gridx;
  y = ((y+gridy/2)/gridy)*gridy;

redo:

  switch(state) {
    case STATE_CREATE: 
    case STATE_EDIT: {
      #if VERBOSE
        if (state==STATE_CREATE)
          cout << "  STATE_CREATE => mouseMove to gadget" << endl;
        else
          cout << "  STATE_EDIT => mouseMove to gadget" << endl;
      #endif
      assert(gadget!=NULL);
      unsigned r = gadget->mouseMove(this,x,y,m);
      if (r & TFigure::DELETE) {
        #if VERBOSE
          cout << "    delete gadget" << endl;
        #endif
        deleteGadget(gadget);
      }
      if (r & TFigure::STOP) {
        #if VERBOSE
          cout << "    stop" << endl;
        #endif
        stopOperation();
      }
      if (r & TFigure::REPEAT) {
        #if VERBOSE
          cout << "    repeat event" << endl;
        #endif
        goto redo;
      }
      return;
    } break;

    case STATE_MOVE: {
#if VERBOSE
      cout << "  STATE_MOVE => moving selection" << endl;
#endif
      int dx = x-down_x; down_x=x;
      int dy = y-down_y; down_y=y;
      TGadgetSet::iterator p,e;
      p = selection.begin();
      e = selection.end();
      memo_x+=dx;
      memo_y+=dy;
      while(p!=e) {
        invalidateFigure(*p);
        (*p)->translate(dx, dy);
        invalidateFigure(*p);
        p++;
      }
      updateScrollbars();
    } break;

    case STATE_MOVE_HANDLE: {
      #if VERBOSE
        cout << "  STATE_MOVE_HANDLE => moving handle" << endl;
      #endif
      invalidateFigure(*selection.begin());
      (*selection.begin())->translateHandle(handle, x, y);
      invalidateFigure(*selection.begin());
    } break;

    case STATE_SELECT_RECT: {
      // don't start the rectangle select, when there's still something
      // selected and the mouse was only moved a little bit due to a
      // shacky hand
      if (selection.begin() != selection.end()) {
        int dx = down_x - x;
        int dy = down_y - y;
        if (dx<0) dx=-dx;
        if (dy<0) dy=-dy;
        if (dx<2 || dy<2)
          break;
        clearSelection();
        sigSelectionChanged();
      }
      #if VERBOSE
        cout << "  STATE_SELECT_RECT => redrawing rectangle" << endl;
      #endif
      window->invalidateWindow();
      window->paintNow();
      TPen pen(window);
      pen.setLineStyle(TPen::DOT);
      TRectangle r(TPoint(down_x,down_y), TPoint(x,y));
      pen.drawRectangle(r);
    } break;
  }
}

void
TFigureEditor::mouseLUp(int x, int y, unsigned m)
{
#if VERBOSE
  cout << __PRETTY_FUNCTION__ << endl;
#endif

  if (!window)
    return;

  x = ((x+gridx/2)/gridx)*gridx;
  y = ((y+gridy/2)/gridy)*gridy;

redo:

  switch(state) {
    case STATE_CREATE:
    case STATE_EDIT: {
      assert(gadget!=NULL);
      unsigned r = gadget->mouseLUp(this,x,y,m);
      if (r & TFigure::DELETE) {
        #if VERBOSE
          cout << "    delete gadget" << endl;
        #endif
        deleteGadget(gadget);
      }
      if (r & TFigure::STOP) {
        #if VERBOSE
          cout << "    stop" << endl;
        #endif
        stopOperation();
      }
      if (r & TFigure::REPEAT) {
        #if VERBOSE
          cout << "    repeat event" << endl;
        #endif
        goto redo;
      }
      return;
    } break;

    case STATE_MOVE: {
      #if VERBOSE
        cout << "  STATE_MOVE => STATE_NONE" << endl;
      #endif
#if 0
      int dx = x-down_x; down_x=x;
      int dy = y-down_y; down_y=y;
      TGadgetSet::iterator p,e;
      p = selection.begin();
      e = selection.end();
      memo_x += dx;
      memo_y += dy;
#endif
      TUndoableMove *undo = new TUndoableMove(memo_x, memo_y, selection);
      history.add(undo);
#if 0
      while(p!=e) {
        invalidateFigure(*p);
        (*p)->translate(dx, dy);
        invalidateFigure(*p);
        p++;
      }
      updateScrollbars();
#endif
      state = STATE_NONE;
    } break;

    case STATE_MOVE_HANDLE: {
      #if VERBOSE
        cout << "  STATE_MOVE_HANDLE => updating scrollbars, STATE_NONE" << endl;
      #endif
      invalidateFigure(*selection.begin());
      (*selection.begin())->translateHandle(handle, x, y);
      invalidateFigure(*selection.begin());
      state = STATE_NONE;
      updateScrollbars();
      
      TPoint pt(x,y);
      history.add(new TUndoableHandleMove(*selection.begin(), handle, memo_pt, pt));
    } break;

    case STATE_SELECT_RECT: {
      #if VERBOSE
        cout << "  STATE_SELECT_RECT => ";
      #endif
      bool selecting = true;
      if (selection.begin() != selection.end()) {
        int dx = down_x - x;
        int dy = down_y - y;
        if (dx<0) dx=-dx;
        if (dy<0) dy=-dy;
        if (dx<2 || dy<2)
          selecting = false;
      }
      if (selecting) {
        TFigureModel::iterator p, e;
        p = gadgets->begin();
        e = gadgets->end();
        TRectangle r1(TPoint(down_x,down_y), TPoint(x,y));
        TRectangle r2;
        while(p!=e) {
          (*p)->getShape(r2);
          if (r1.isInside( r2.x, r2.y ) &&
              r1.isInside( r2.x+r2.w, r2.y+r2.h ) )
          {
            selection.insert(*p);
          }
          p++;
        }
        #if VERBOSE
          cout << selection.size() << " objects selected, STATE_NONE" << endl;
        #endif
      }
      window->invalidateWindow(); // ??
      state = STATE_NONE;
    } break;
  }
}


void
TFigureEditor::invalidateFigure(TFigure* gadget)
{
  if (window) {
    TRectangle r;
    gadget->getShape(r);
//cout << "invalidating shape " << r.x << "," << r.y << "," << r.w << "," << r.h << endl;
    r.x-=3;
    r.y-=3;
    r.w+=6;
    r.h+=6;
    r.x+=window->getOriginX();
    r.y+=window->getOriginY();
    window->invalidateWindow(r);
  }
}

/**
 * Find the gadget at position at (mx, my).
 *
 * This method doesn't find gadgets which are currently created or edited.
 */
TFigure*
TFigureEditor::findGadgetAt(int mx, int my)
{
#if VERBOSE
  cerr << "TFigureEditor::findGadgetAt(" << mx << ", " << my << ")\n";
#endif
  double distance = TFigure::OUT_OF_RANGE;
  TFigureModel::iterator p,b,found;
  p = found = gadgets->end();
  b = gadgets->begin();
  while(p!=b) {
    --p;
    if (*p!=gadget) {
      double d = (*p)->distance(mx, my);
      if (d<distance) {
        distance = d;
        found = p;
      }
    }
  }
  if (distance > TFigure::RANGE)
    return NULL;
  return *found;
}

void
TFigureEditor::updateScrollbars()
{
  if (!window || !use_scrollbars)
    return;
DBM(cout << __PRETTY_FUNCTION__ << ": entry" << endl;)

  // determine area size
  //-----------------------------------------------------------------
  int x1, y1; // upper, left corner
  int x2, y2; // lower, right corner

  x1=-window->getOriginX();
  if (x1>0)
    x1=0;
  y1=-window->getOriginY();
  if (y1>0)
    y1=0;

  x2=x1+1;
  y2=y1+1;

//DBM(cout << "x-org     : " << window->getOriginX() << endl;)
//DBM(cout << "x-axis (1): " << x1 << " - " << x2 << endl;)
//DBM(cout << "y-axis (1): " << y1 << " - " << y2 << endl;)
  
  TFigureModel::iterator p, e;
  p = gadgets->begin();
  e = gadgets->end();
  TRectangle r;
  while(p!=e) {
    int a;
    (*p)->getShape(r);
    if (r.x<x1)
      x1=r.x;
    a = r.x+r.w;
    if (a>x2)
      x2=a;
    if (r.y<y1)
      y1=r.y;
    a = r.y+r.h;
    if (a>y2)
      y2=a;
    p++;
  }

//DBM(cout << "x-axis (2): " << x1 << " - " << x2 << endl;)
//DBM(cout << "y-axis (2): " << y1 << " - " << y2 << endl;)

DBM(cout << "area size: (" << x1 << ", " << y1 << ") - ("
         << x2 << ", " << y2 << ")\n";)


  // calculate lower, right visible corner still in occupied area in x22, y22
  int w, h;
  w = window->getWidth();
  h = window->getHeight();

  int x22 = x2, y22 = y2;

  x22=x1 + w-1;
  if (x22<x2) x22=x2;
  if (x22<w) x22=w;
    
  y22=y1 + h-1;
  if (y22<y2) y22=y2;
  if (y22<h) y22=h;

//DBM(cout << "x-axis (3): " << x1 << " - " << x22 << endl;)
//DBM(cout << "y-axis (3): " << y1 << " - " << y22 << endl;)

DBM(cout << "lower, right visible: " << x22 << ", " << y22 << endl;)
DBM(cout << "height, width       : " << w << ", " << h << endl;)
  // setup scrollbars
  //-----------------------------------------------------------------
  bool vs, hs;
  vs = hs = false;

  if (x1<0 || x22>w) {
    hs   = true;
    h   -= TScrollBar::getFixedSize();
    y22 -= TScrollBar::getFixedSize();
    if (y22<y2)
      y22=y2;
  }
  if (y1<0 || y22>h) {
    vs   = true;
    w   -= TScrollBar::getFixedSize();
    x22 -= TScrollBar::getFixedSize();
    if (x22<x2)
      x22=x2;
  }
  if (!hs && (x1<0 || x2>w)) {
    hs   = true;
    h   -= TScrollBar::getFixedSize();
    y22 -= TScrollBar::getFixedSize();
    if (y22<y2)
      y22=y2;
  }
  x2=x22;
  y2=y22;

//DBM(cout << "x-axis (4): " << x1 << " - " << x22 << endl;)
//DBM(cout << "y-axis (4): " << y1 << " - " << y22 << endl;)


  if (hs) {
    if (!hscroll) {
      hscroll = new TScrollBar(window, "hscroll");
      CONNECT_VALUE(hscroll->getModel()->sigChanged, this, actHScroll, hscroll);
    }
    hscroll->setShape(0,h,w,TScrollBar::getFixedSize());
    hscroll->getModel()->setRangeProperties(
      -window->getOriginX(),
      w,
      x1, x2);
DBM(cout << "hscroll: " << x1 << ", " << -window->getOriginX() << ", " << x2 << endl;)
    if (!hscroll->isRealized()) {
      hscroll->createWindow();
    } else {
      hscroll->setMapped(true);
    }
  } else {
    if (hscroll) {
      hscroll->setMapped(false);
    }
  }

  if (vs) {
    if (!vscroll) {
      vscroll = new TScrollBar(window, "vscroll");
      CONNECT_VALUE(vscroll->getModel()->sigChanged, this, actVScroll, vscroll);
    }
    vscroll->setShape(w,0,TScrollBar::getFixedSize(),h);
DBM(cout << "updateScrollbars: vscroll to " << -window->getOriginY() << endl;)
    vscroll->getModel()->setRangeProperties(
      -window->getOriginY(),
      h,
      y1, y2);
    if (!vscroll->isRealized()) {
      vscroll->createWindow();
    } else {
      vscroll->setMapped(true);
    }
  } else {
    if (vscroll) {
      vscroll->setMapped(false);
    }
  }
DBM(cout << __PRETTY_FUNCTION__ << ": exit" << endl << endl;)
}

void
TFigureEditor::actVScroll(int v)
{
DBM(cout << "actVScroll: v=" << v << " vscroll range=" << vscroll->getMinimum() << " - " << vscroll->getMaximum() << endl;)

  window->scrollTo(window->getOriginX(), -v);
  updateScrollbars();
}

void
TFigureEditor::actHScroll(int v)
{
DBM(cout << "actHScroll: v=" << v << " hscroll range=" << hscroll->getMinimum() << " - " << hscroll->getMaximum() << endl;)
  window->scrollTo(-v, window->getOriginY());
  updateScrollbars();
}

void
TFigureEditor::undo()
{
  clearSelection();
  if (history.getBackSize()>0) {
    history.getCurrent()->undo();
    history.goBack();
    window->invalidateWindow();
    updateScrollbars();
  }
}

void
TFigureEditor::redo()
{
  clearSelection();
  if (history.getForwardSize()>0) {
    history.goForward();
    history.getCurrent()->redo();
    window->invalidateWindow();
    updateScrollbars();
  }
}

TFigureEditor::TColorSelector::TColorSelector(TWindow *parent, 
                               const string &title,
                               TFigureEditor *gedit):
  super(parent, title)
{
  this->gedit = gedit;
  setSize(32, 32);
  filled = false;
  linecolor.set(0,0,0);
  fillcolor.set(255,255,255);
}

void
TFigureEditor::TColorSelector::paint()
{
  TPen pen(this);

  border = getWidth() / 6;
  
  pen.setColor(linecolor);
  pen.fillRectangle(0, 0, getWidth(), getHeight());
  
  if (filled) {
    pen.setColor(fillcolor);
    pen.fillRectangle(border, border, getWidth()-border*2, getHeight()-border*2);
  } else {
    pen.setColor(255,255,255);
    pen.fillRectangle(border, border, getWidth()-border*2, getHeight()-border*2);
    pen.setColor(0,0,0);
    pen.drawLine(getWidth()-border-1, border, border-1, getHeight()-border);
    pen.drawLine(border, border, getWidth()-border, getHeight()-border);
  }
}

void
TFigureEditor::TColorSelector::mouseLDown(int x, int y, unsigned modifier)
{
  if (x<border || 
      y<border || 
      x>getWidth()-border ||
      y>getWidth()-border) 
  {
    TColorDialog ce(this, "Line Color", &linecolor);
    ce.doModalLoop();
    invalidateWindow();
  } else {
    TColorDialog ce(this, "Fill Color", &fillcolor);
    TCheckBox *fill = new TCheckBox(&ce, "Filled");
    fill->setShape(x=8+256+8+16+8+12, 228, 80, 32);
    fill->getModel()->setValue(filled);
    ce.doModalLoop();
    filled = fill->getModel()->getValue();
    invalidateWindow();
  }
  gedit->setLineColor(linecolor);
  gedit->setFillColor(fillcolor);
  gedit->setFilled(filled);
}
