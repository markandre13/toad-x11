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

#include <cmath>
#include <algorithm>

// missing in mingw
#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi */
#endif

using namespace toad;

#define DBM(CMD)
// #define VERBOSE 1

/**
 * \class toad::TFigureEditor
 * TFigureEditor is a common graphical editor for TFigure objects.
 * 
 * It's still experimental so expect major changes in the future before
 * using it. One major goal is to make it possible to edit scaleable and
 * rotateable 2D figures and to create 3D objects.
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
 *      group followed by undo causes a segfault or inifinite recursion
 *      or something like that
 *   \li
 *      scrollbars aren't setup properly during scaling
 *   \li
 *      ungroup must take care of the groups transformation matrix
 *   \li
 *      resize groups
 *   \li
 *      adjust getShape to check transformations
 *   \li
 *      adjust finding figures & handles for transformations
 *   \li
 *      undo history isn't erased when a new model is set
 *   \li
 *      scrollbars aren't update after and during object creation
 *   \li
 *      group/ungroup isn't part of undo/redo
 *   \li
 *      undo/redo entries are active also when they are a null operation
 *   \li
 *      text is misplaced during rotation (beside the fact that the text
 *      itself isn't rotated
 *   \li
 *      polgons contain bogus points (can be seen during/after rotation)
 *   \li
 *      help points of bezier must not be part of the bezier shape
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

TFigureEditor::TFigureEditor(TWindow *p, const string &t):
  super(p, t)
{
  init();
  setMouseMoveMessages(TMMM_LBUTTON);
  bNoBackground = true;
  window = this;
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
  // [store figures to `window']
  if (window)
    window->invalidateWindow();
  window = w;
  // [get figures from `window']
  if (window)
    window->invalidateWindow();
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
  mat = 0;
//  vscroll = NULL;
//  hscroll = NULL;
  model = new TFigureModel();
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
  TSerializable *s;

  // ::restorePtr(in, &s);
  s = in.restore();

  TFigureModel *m = dynamic_cast<TFigureModel *>(s);
  if (!m) {
    cerr << "wasn't a TFigureModel" << endl;
    return false;
  }
  setModel(m);
  return true;
}

void
TFigureEditor::store(TOutObjectStream &out) const
{
  if (model)
    ::store(out, model);
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

int rotx=100;
int roty=100;
double rotd=0.0;
double rotd0;

int select_x;
int select_y;

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
    bpen.fillRectanglePC(0,0,gridx,gridy);
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
  pen.identity();
  pen.fillRectanglePC(0,0,window->getWidth(),window->getHeight());
  pen.translate(window->getOriginX(), window->getOriginY());

  if (mat)
    pen.multiply(mat);
  
  pen.setColor(TColor::BLACK);

  TFigureModel::iterator p, e;
  
  // draw the figures
  p = model->begin();
  e = model->end();
  while(p!=e) {
    TFigure::EPaintType pt = TFigure::NORMAL;
    unsigned pushs = 0;
    if ((*p)->mat) {
      pen.push();
      pushs++;
      pen.multiply( (*p)->mat );
    }
    if (gadget==*p) {
      if (state==STATE_ROTATE) {
        pen.push();
        pushs++;
        pen.translate(rotx, roty);
        pen.rotate(rotd);
        pen.translate(-rotx, -roty);
      } else {
        pt = TFigure::EDIT;
      }
    } else if (selection.find(*p)!=selection.end()) {
      pt = TFigure::SELECT;
    }
    (*p)->paint(pen, pt);
    while(pushs) {
      pen.pop();
      pushs--;
    }
    ++p;
  }

  // draw the selection marks over all gadgets
  pen.setColor(0,0,0);
  TFigureSet::iterator sp,se;
  sp = selection.begin();
  se = selection.end();
  while(sp!=se) {
    unsigned pushs = 0;
    if ((*sp)->mat) {
      pen.push();
      pushs++;
      pen.multiply( (*sp)->mat );
    }
    if (gadget==*sp && state==STATE_ROTATE) {
        pen.push();
        pushs++;
        pen.translate(rotx, roty);
        pen.rotate(rotd);
        pen.translate(-rotx, -roty);
    }
    (*sp)->paintSelection(pen);
    while(pushs) {
      pen.pop();
      pushs--;
    }
    ++sp;
  }

  if (state==STATE_SELECT_RECT) {
    pen.setColor(0,0,0);
    pen.setLineStyle(TPen::DOT);
    pen.drawRectanglePC(down_x, down_y, select_x-down_x, select_y-down_y);
  }

  // draw rotation center  
  if (operation==OP_ROTATE) {
    int r1=10;
    pen.setColor(0,0,0);
    pen.fillCirclePC(rotx-r1, roty-r1, r1*2, r1*2);
    
    r1-=2;
    pen.setColor(255,255,255);
    pen.fillCirclePC(rotx-r1, roty-r1, r1*2, r1*2);
    
    r1-=2;
    pen.setColor(0,0,0);
    pen.fillCirclePC(rotx-r1, roty-r1, r1*2, r1*2);
  }

  // put the result onto the screen
  TPen scr(window);
  scr.identity();
  scr.drawBitmap(0,0, &bmp);
  
  paintCorner(scr);
}

void
TFigureEditor::setLineColor(const TRGB &rgb)
{
  line_color = rgb;
  TFigureSet::iterator p,e;
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
  TFigureSet::iterator p,e;
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
  TFigureSet::iterator p,e;
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

/**
 * Add a figure to the editors model.
 */
void
TFigureEditor::addFigure(TFigure *g)
{
  model->add(g);
  update_scrollbars = true;
  invalidateFigure(g);
}

/**
 * Removes a figure from the editors model.
 */
void
TFigureEditor::deleteFigure(TFigure *g)
{
  if (g==gadget)
    gadget=NULL;
  if (g==gtemplate)
    gtemplate=NULL;
  
  TFigureModel::iterator p,e;
  p = model->begin();
  e = model->end();
  while(p!=e) {
    if (g==*p) {
      model->erase(p);
      break;
    }
    ++p;
  }

  TFigureSet::iterator s;
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
  history.add(new TUndoableDelete(*model, selection));
//cout << "selection size: " << selection.size() << endl;

  TFigureModel::iterator p,e,del;
  p = model->begin();
  e = model->end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end() &&
        (*p)->removeable )
    {
      del = p;
      ++p;
      if (gadget==*del)
        gadget=NULL;
//      cout << "removing gadget " << *del << endl;
      model->erase(del);
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
  p = model->begin();
  e = model->end();
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
//  setOrigin(0,0); needed?
  setPanePos(0,0);
  updateScrollbars();
}

void
TFigureEditor::selection2Top()
{
  TFigureModel::iterator p,b,np;
  p = model->end();
  b = model->begin();

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
  p = np = model->begin();
  e = model->end();
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
  p = e = prev = model->end();
  b = model->begin();
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
  p = model->begin();
  e = prev = model->end();
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

  p = model->begin();
  e = model->end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end()) {
      group->gadgets.add(*p);
      TFigureModel::iterator del = p;
      ++p;
      model->erase(del);
    } else {
      ++p;
    }
  }

  clearSelection();
  group->calcSize();
  model->insert(p, group);
  selection.insert(group);
}

void
TFigureEditor::ungroup()
{
  TFigureModel::iterator p,e;
  p = model->begin();
  e = model->end();
  while(p!=e) {
    if (selection.find(*p)!=selection.end()) {
      TFGroup *group = dynamic_cast<TFGroup*>(*p);
      if (group) {
        TFigureModel::iterator vp,ve;
        vp = group->gadgets.begin();
        ve = group->gadgets.end();
        model->insert(p, vp,ve);
        group->gadgets.erase(group->gadgets.begin(),group->gadgets.end());
        delete group;
        TFigureModel::iterator del = p;
        ++p;
        model->erase(del);
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
        history.add(new TUndoableCreate(*model, selection));
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
        deleteFigure(gadget);
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
  if (mat) {
    TMatrix2D m(*mat);
    m.invert();
    m.map(mx, my, &mx, &my);
  }

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
            TFigureSet::iterator p,e;
            p = selection.begin();
            e = selection.end();
            #if VERBOSE
              cout << "      mouse @ " << mx << ", " << my << endl;
            #endif

/* copied from findGadgetAt */            
      short x, y;
      if ((*p)->mat) {
        TMatrix2D m(*(*p)->mat);
        m.invert();
        m.map(mx, my, &x, &y);
      } else {
        x = mx;
        y = my;
      }
            
            
            while(p!=e) {
              unsigned h=0;
              while(true) {
                if (!(*p)->getHandle(h,memo_pt))
                  break;
                if (memo_pt.x-2<=x && x<=memo_pt.x+2 && 
                    memo_pt.y-2<=y && y<=memo_pt.y+2) {
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
            TFigureSet::iterator gi = selection.find(g);
            
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
                  select_x = x;
                  select_y = y;
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
                  select_x = x;
                  select_y = y;
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
            select_x = x;
            select_y = y;
          }
        } break;

        case OP_CREATE: {
          #if VERBOSE
            cout << "    OP_CREATE => STATE_CREATE" << endl;
          #endif
          clearSelection();
          gadget = static_cast<TFigure*>(gtemplate->clone());
          model->add(gadget);
          window->invalidateWindow();
          state = STATE_START_CREATE;
          setMouseMoveMessages(TWindow::TMMM_ALL);
          gadget->startCreate();
          unsigned r = gadget->mouseLDown(this,x,y,m);
          state = STATE_CREATE;
          if (r & TFigure::DELETE)
            deleteFigure(gadget);
          if (r & TFigure::STOP)
            stopOperation();
          if (r & TFigure::REPEAT)
            goto redo;
          return;
        } break;
        
        case OP_ROTATE: {
          TFigure *g = findGadgetAt(mx, my);
          if (g) {
            clearSelection();
            state = STATE_ROTATE;
            gadget = g;
            TRectangle r;
            g->getShape(r);
            rotx = r.x + r.w/2;
            roty = r.y + r.h/2;
            rotd0=atan2(static_cast<double>(y - roty), 
                  static_cast<double>(x - rotx)) * 360.0 / (2.0 * M_PI);
            rotd = rotd0;
            cerr << "state = STATE_ROTATE" << endl;
          } else {
            cerr << "no gadget found for rotation" << endl;
          }
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
        deleteFigure(gadget);
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
  if (mat) {
    TMatrix2D m(*mat);
    m.invert();
    m.map(x, y, &x, &y);
  }
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
        deleteFigure(gadget);
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
      TFigureSet::iterator p,e;
      p = selection.begin();
      e = selection.end();
      memo_x+=dx;
      memo_y+=dy;
      while(p!=e) {
        invalidateFigure(*p);
        if ( !(*p)->mat) {
          (*p)->translate(dx, dy);
        } else {
          TMatrix2D m;
          m.translate(dx, dy);
          m.multiply((*p)->mat);
          *(*p)->mat = m;
        }
        invalidateFigure(*p);
        p++;
      }
      updateScrollbars();
    } break;

    case STATE_MOVE_HANDLE: {
      TFigure *f = *selection.begin();
      #if VERBOSE
        cout << "  STATE_MOVE_HANDLE => moving handle" << endl;
      #endif

/* copied from findGadgetAt */
      short x2, y2;
      if (f->mat) {
        TMatrix2D m(*f->mat);
        m.invert();
        m.map(x, y, &x2, &y2);
      } else {
        x2 = x;
        y2 = y;
      }

      invalidateFigure(f);
      f->translateHandle(handle, x2, y2);
      invalidateFigure(f);
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
      select_x = x;
      select_y = y;
/*
      window->paintNow();
      TPen pen(window);
      pen.setLineStyle(TPen::DOT);
      pen.drawRectanglePC(down_x, down_y, x-down_x, y-down_y);
*/
    } break;
    
    case STATE_ROTATE: {
      rotd=atan2(static_cast<double>(y - roty), 
                 static_cast<double>(x - rotx)) * 360.0 / (2.0 * M_PI);
//      cerr << "rotd="<<rotd<<", rotd0="<<rotd0<<" -> " << (rotd-rotd0) << "\n";
      rotd-=rotd0;
      invalidateWindow();
    }
  }
}

void
TFigureEditor::mouseLUp(int x, int y, unsigned m)
{
  if (mat) {
    TMatrix2D m(*mat);
    m.invert();
    m.map(x, y, &x, &y);
  }
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
        deleteFigure(gadget);
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
      TFigureSet::iterator p,e;
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
      TFigure *f = *selection.begin();

/* copied from findGadgetAt */            
      short x2, y2;
      if (f->mat) {
        TMatrix2D m(*f->mat);
        m.invert();
        m.map(x, y, &x2, &y2);
      } else {
        x2 = x;
        y2 = y;
      }

      invalidateFigure(f);
      f->translateHandle(handle, x2, y2);
      invalidateFigure(f);
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
        p = model->begin();
        e = model->end();
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
    
    case STATE_ROTATE: {
      if (!gadget->mat)
        gadget->mat = new TMatrix2D();
      gadget->mat->translate(rotx, roty);
      gadget->mat->rotate(rotd);
      gadget->mat->translate(-rotx, -roty);
      state = STATE_NONE;
    } break;
  }
}

void
TFigureEditor::invalidateFigure(TFigure* figure)
{
  if (!window)
    return;

  TRectangle r;
  figure->getShape(r);
  if (mat || figure->mat) {
    TMatrix2D m;
    if (mat) {
      m=*mat;
    }
    if (figure->mat)
      m.multiply(figure->mat);
      
    int x1, x2, y1, y2;
    short x, y;
    m.map(r.x, r.y, &x, &y);
    x1 = x2 = x;
    y1 = y2 = y;
    for(int i=1; i<4; ++i) {
      switch(i) {
        case 1:
          m.map(r.x+r.w, r.y, &x, &y);
          break;
        case 2:
          m.map(r.x+r.w, r.y+r.h, &x, &y);
          break;
        case 3:
          m.map(r.x, r.y+r.h, &x, &y);
          break;
      }
      if (x1>x)
        x1=x;
      if (x2<x)
        x2=x;
      if (y1>y)
        y1=y;
      if (y2<y)
        y2=y;
    }
    r.set(TPoint(x1,y1), TPoint(x2, y2));
  }
//cout << "invalidating shape " << r.x << "," << r.y << "," << r.w << "," << r.h << endl;

  r.x-=3;
  r.y-=3;
  r.w+=6;
  r.h+=6;

  r.x+=window->getOriginX();
  r.y+=window->getOriginY();

  window->invalidateWindow(r);
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
  p = found = model->end();
  b = model->begin();
  TMatrix2D *stack = new TMatrix2D();
  while(p!=b) {
    --p;
    if (*p!=gadget) {
      short x, y;
      if ((*p)->mat) {
        stack->multiply((*p)->mat);
        stack->invert();
        stack->map(mx, my, &x, &y);
      } else {
        x = mx;
        y = my;
      }
//cerr << "  after rotation ("<<x<<", "<<y<<")\n";
      double d = (*p)->distance(x, y);
//cerr << "  distance = " << d << endl;
      stack->identity();
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
TFigureEditor::adjustPane()
{
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
/*
  x1=-window->getOriginX();
  if (x1>0)
    x1=0;
  y1=-window->getOriginY();
  if (y1>0)
    y1=0;

  x2=x1+1;
  y2=y1+1;
*/
//DBM(cout << "x-org     : " << window->getOriginX() << endl;)
//DBM(cout << "x-axis (1): " << x1 << " - " << x2 << endl;)
//DBM(cout << "y-axis (1): " << y1 << " - " << y2 << endl;)
  
  TFigureModel::iterator p, e;
  p = model->begin();
  e = model->end();
  TRectangle r;
  
  if (p==e) {
    x1 = x2 = y1 = y2 = 0;
  } else {
    (*p)->getShape(r);
    x1=r.x;
    y1=r.y;
    x2=r.x+r.w-1;
    y2=r.y+r.h-1;
    ++p;
  }
  
  while(p!=e) {
    int a;
    (*p)->getShape(r);
    if (r.x<x1)
      x1=r.x;
    a = r.x+r.w-1;
    if (a>x2)
      x2=a;
    if (r.y<y1)
      y1=r.y;
    a = r.y+r.h-1;
    if (a>y2)
      y2=a;
    p++;
  }
  
  if (x1>0) x1=0;
  if (y1>0) y1=0;

//DBM(cout << "x-axis (2): " << x1 << " - " << x2 << endl;)
//DBM(cout << "y-axis (2): " << y1 << " - " << y2 << endl;)

DBM(cout << "area size: (" << x1 << ", " << y1 << ") - ("
         << x2 << ", " << y2 << ")\n";)


  pane.x = x1;
  pane.y = y1;
  pane.w = x2-x1+1;
  pane.h = y2-y1+1;
  doLayout();
DBM(cout << __PRETTY_FUNCTION__ << ": exit" << endl << endl;)
}

void
TFigureEditor::scrolled(int dx, int dy)
{
  int x, y;
  getPanePos(&x, &y);
  // window->scrollTo(-x, -y);
  window->setOrigin(-x, -y);
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
  pen.fillRectanglePC(0, 0, getWidth(), getHeight());
  
  if (filled) {
    pen.setColor(fillcolor);
    pen.fillRectanglePC(border, border, getWidth()-border*2, getHeight()-border*2);
  } else {
    pen.setColor(255,255,255);
    pen.fillRectanglePC(border, border, getWidth()-border*2, getHeight()-border*2);
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
    fill->getModel()->setValue(true);
    ce.doModalLoop();
    if (ce.apply)
      filled = fill->getModel()->getValue();
    invalidateWindow();
  }
  gedit->setLineColor(linecolor);
  gedit->setFillColor(fillcolor);
  gedit->setFilled(filled);
}
