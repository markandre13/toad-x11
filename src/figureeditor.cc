/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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
 * TFigureEditor is a graphical editor for TFigure objects.
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
  row_header_renderer = col_header_renderer = 0;
}

TFigureEditor::TFigureEditor(TWindow *p, const string &t):
  super(p, t)
{
  init();
  setMouseMoveMessages(TMMM_LBUTTON);
  bNoBackground = true;
  window = this;
  row_header_renderer = col_header_renderer = 0;
}

TFigureEditor::~TFigureEditor()
{
  setPreferences(0);

//  SetMode(MODE_SELECT);
//  cout << "gadgets total: " << gadgets.size() << endl;
  if (mat)
    delete mat;
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

class THistoryAction:
  public TAction
{
    TFigureEditor::THistory *history;
    bool undo;
  public:
    THistoryAction(TWindow *w, const string &t, TFigureEditor::THistory *h, bool u):
      TAction(w, t), history(h), undo(u)
    {
    }
    
    bool getState(string *text, bool *active) const {
      if (undo) {
        if (history->getBackSize()>0)
          history->getCurrent()->getUndoName(text);
        else
          *text = "(Undo)";
      } else {
        if (history->getForwardSize()>0) {
          history->goForward();
          history->getCurrent()->getRedoName(text);
          history->goBack();
        } else {
          *text = "(Redo)";
        }
      }
      return true;
    }
};

void foobar(TFigurePreferences *p) {
  p->sigChanged();
}

TFigurePreferences::TFigurePreferences()
{
  linecolor.set(0,0,0);
  fillcolor.set(255,255,255);
  filled = false;
  fontname = "arial,helvetica,sans-serif:size=12";

  drawgrid = true;
  connect(drawgrid.sigChanged, foobar, this);
  gridsize = 4;
  
  linewidth = 0;
  linestyle = TPen::SOLID;
  arrowmode = TFLine::NONE;
  arrowtype = TFLine::EMPTY;
  
  current = 0;
}

TFigurePreferences::~TFigurePreferences()
{
}

void
TFigurePreferences::setOperation(unsigned op)
{
  if (current) current->setOperation(op);
}

void
TFigurePreferences::setCreate(TFigure *figure)
{
  if (current) current->setCreate(figure);
}

void
TFigurePreferences::group()
{
  if (current) current->group();
}

void
TFigurePreferences::ungroup()
{
  if (current) current->ungroup();
}

void
TFigurePreferences::selectionDown()
{
  if (current) current->selectionDown();
}

void
TFigurePreferences::selection2Bottom()
{
  if (current) current->selection2Bottom();
}

void
TFigurePreferences::selectionUp()
{
  if (current) current->selectionUp();
}

void
TFigurePreferences::selection2Top()
{
  if (current) current->selection2Top();
}



void
TFigureEditor::init()
{
  preferences = 0;
  setPreferences(new TFigurePreferences);
  background_color.set(192,192,192);
  fuzziness = 2;

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

  action = new THistoryAction(this, "edit|undo", &history, true);
  CONNECT(action->sigActivate, this, undo);
  action = new THistoryAction(this, "edit|redo", &history, false);
  CONNECT(action->sigActivate, this, redo);
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

void 
TFigureEditor::identity() 
{ 
  if (mat) {
    mat->identity();
    updateScrollbars();
  }
}

/**
 * This method is doing nothing yet.
 */
void TFigureEditor::rotate(double)
{
}

/**
 * This method is doing nothing yet.
 */
void TFigureEditor::rotateAt(double x, double y, double degree)
{
}

/**
 * This method is doing nothing yet.
 */
void TFigureEditor::translate(double, double)
{
}

/**
 * Scale the edit pane.
 */
void TFigureEditor::scale(double sx, double sy)
{
  if (!mat)
    mat = new TMatrix2D();
  mat->scale(sx, sy);

// better: create 2 points, transform 'em and calculate the
// distance
  fuzziness = static_cast<int>(2.0 / sx);
  
  updateScrollbars();
  invalidateWindow();
}

/**
 * This method is doing nothing yet.
 */
void TFigureEditor::shear(double, double) 
{
  updateScrollbars();
}

/**
 * This method is doing nothing yet.
 */
void TFigureEditor::multiply(const TMatrix2D*)
{
  updateScrollbars();
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
  preferences->drawgrid = b;
}

/**
 * Set the size of the grid.
 */
void
TFigureEditor::setGrid(int gridsize) {
  if (gridsize<0)
    gridsize=0;
  preferences->gridsize = gridsize;
  if (window)
    window->invalidateWindow(visible);
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
  TBitmap bmp(visible.w, visible.h, TBITMAP_SERVER);
  TPen pen(&bmp);

  pen.setColor(background_color);
  pen.identity();
  pen.fillRectanglePC(0,0,visible.w,visible.h);
  pen.translate(window->getOriginX(), window->getOriginY());

  if (mat)
    pen.multiply(mat);
    

  if (preferences->drawgrid && preferences->gridsize) {
    pen.setColor(
      background_color.r > 128 ? background_color.r-64 : background_color.r+64,
      background_color.g > 128 ? background_color.g-64 : background_color.g+64,
      background_color.b > 128 ? background_color.b-64 : background_color.b+64
    );
    int x1, x2, y1, y2;
    
    getPanePos(&x1, &y1, true);
    x2 = x1 + visible.w;
    y2 = y1 + visible.h;
    
    if (mat) {
      int gx, gy;
      mat->map(preferences->gridsize, preferences->gridsize, &gx, &gy);
      if (gx<2 || gy<2) {
//        cerr << "don't draw grid, it's too small" << endl;
        x1=y1=1;
        x2=y2=0;
      } else {
        TMatrix2D m(*mat);
        m.invert();
//        cerr << "draw grid of size " << gx << ", " << gy << endl;
        m.map(x1, y1, &x1, &y1);
        m.map(x2, y2, &x2, &y2);
        if (x1>x2) {
          int a = x1; x1 = x2; x2 = a;
        }
        if (y1>y2) {
          int a = y1; y1 = y2; y2 = a;
        }
      }
    }

    // justify to grid
    int g = preferences->gridsize;
    x1 -= x1 % g;
    y1 -= y1 % g;

//    cerr << "draw grid from (" << x1 << ", " << y1 << ") to ("
//         << x2 << ", " << y2 << ")" << endl;

    for(int y=y1; y<=y2; y+=g) {
      for(int x=x1; x<=x2; x+=g) {
        pen.drawPoint(x, y);
      }
    }
  }
  
  print(pen);

  // draw the selection marks over all figures
  for(TFigureSet::iterator sp = selection.begin();
      sp != selection.end();
      ++sp)
  {
    if ((*sp)->mat) {
      pen.push();
      pen.multiply( (*sp)->mat );
    }
    (*sp)->paintSelection(pen);
    if ((*sp)->mat)
      pen.pop();
  }

  if (state==STATE_SELECT_RECT) {
    pen.setColor(0,0,0);
    pen.setLineStyle(TPen::DOT);
    pen.drawRectanglePC(down_x, down_y, select_x-down_x, select_y-down_y);
  }

  // draw rotation center  
  if (gadget && operation==OP_ROTATE) {

    // draw center of rotation
    int x, y;
    if (pen.mat) {
      pen.mat->map(rotx, roty, &x, &y);
      pen.push();
      pen.identity();
    } else {
      x = rotx;
      y = roty;
    }
    
    pen.setLineColor(TColor::FIGURE_SELECTION);
    pen.setFillColor(TColor::WHITE);
    pen.drawCirclePC(x-3,y-3,7,7);
    pen.drawLine(x,y+3,x,y+6);
    pen.drawLine(x+3,y,x+6,y);
    pen.drawLine(x,y-3,x,y-6);
    pen.drawLine(x-3,y,x-6,y);
    if (pen.mat)
      pen.pop();
    
    // draw handles for rotated figure
    pen.push();
    if (state==STATE_ROTATE) {
      pen.translate(rotx, roty);
      pen.rotate(rotd);
      pen.translate(-rotx, -roty);
    }
    if (gadget->mat)
      pen.multiply(gadget->mat);

    TRectangle r;
    gadget->getShape(&r);
    for(int i=0; i<=4; ++i) {
      switch(i) {
        case 0: x = r.x;       y = r.y;       break;
        case 1: x = r.x+r.w-1; y = r.y;       break;
        case 2: x = r.x+r.w-1; y = r.y+r.h-1; break;
        case 3: x = r.x;       y = r.y+r.h-1; break;
        case 4: x = r.x+r.w/2; y = r.y+r.h/2; break;
      }
      if (pen.mat) {
        pen.mat->map(x, y, &x, &y);
        pen.push();
        pen.identity();
      }
      if (i!=4) {
        pen.fillRectanglePC(x-2,y-2,5,5);
      } else {
        pen.drawLine(x-2, y-2, x+2, y+2);
        pen.drawLine(x+2, y-2, x-2, y+2);
      }
      if (pen.mat)
        pen.pop();
    }
    pen.pop();
  }

  // put the result onto the screen
  TPen scr(window);
/*
TRectangle foo;
scr.getClipBox(&foo);
cout << "clip box " << foo << endl;
*/
  scr.identity();
  scr.drawBitmap(visible.x,visible.y, &bmp);
/*
static bool bar=true;
if (bar)
  scr.setColor(255,0,0);
else
  scr.setColor(0,255,0);
bar=!bar;
  scr.drawLine(0,64, 64,0);
*/  
  paintCorner(scr);
  
  if (row_header_renderer) {
    TRectangle clip(0, visible.y, visible.x, visible.h);
    TRectangle dummy(0,0,getWidth(), getHeight());
    scr.identity();
    scr|=dummy;
    scr&=clip;
    scr.translate(0, visible.y+window->getOriginY());
    row_header_renderer->render(scr, -window->getOriginY(), visible.h, mat);
  }

  if (col_header_renderer) {
    TRectangle clip(visible.x, 0, visible.w, visible.y);
    TRectangle dummy(0,0,getWidth(), getHeight());
    scr.identity();
    scr|=dummy;
    scr&=clip;
    scr.translate(visible.x+window->getOriginX(), 0);
    col_header_renderer->render(scr, -window->getOriginX(), visible.w, mat);
  }
}
  
void
TFigureEditor::print(TPenBase &pen)
{
  TFigureModel::iterator p, e;
  
  // draw the figures
  p = model->begin();
  e = model->end();
  while(p!=e) {
    TFigure::EPaintType pt = TFigure::NORMAL;
    unsigned pushs = 0;
    if (gadget==*p) {
      if (state==STATE_ROTATE) {
//cerr << "paint figure for rotation edited figure at rotd=" << rotd << endl;
        pen.push();
        pushs++;
        pen.translate(rotx, roty);
        pen.rotate(rotd);
        pen.translate(-rotx, -roty);
      } else {
        pt = TFigure::EDIT;
      }
    }
    if ((*p)->mat) {
      pen.push();
      pushs++;
      pen.multiply( (*p)->mat );
    }
    
    if (gadget!=*p && selection.find(*p)!=selection.end()) {
      pt = TFigure::SELECT;
    }
    (*p)->paint(pen, pt);
    while(pushs) {
      pen.pop();
      pushs--;
    }
    ++p;
  }
}

void
TFigureEditor::setLineColor(const TRGB &rgb)
{
  preferences->linecolor = rgb;
  TFigureSet::iterator p,e;
  p = selection.begin();
  e = selection.end();
  while(p!=e) {
    (*p)->setLineColor(preferences->linecolor);
    p++;
  }
  if (gtemplate)
    gtemplate->setLineColor(preferences->linecolor);
  window->invalidateWindow();
}

void
TFigureEditor::setFillColor(const TRGB &rgb)
{
  preferences->setFillColor(rgb);
}

void 
TFigureEditor::setPreferences(TFigurePreferences *p) {
  if (preferences) {
    disconnect(preferences->sigChanged, this);
    if (preferences->getCurrent() == this)
      preferences->setCurrent(0);
  }
  preferences = p;
  if (preferences) {
    preferences->setCurrent(this);
    connect(preferences->sigChanged, this, &TThis::modelChanged);
  }
}

void
TFigureEditor::modelChanged()
{
  if (!preferences)
    return;
  switch(preferences->reason) {
    case TFigurePreferences::LINECOLOR:
      for(TFigureSet::iterator p = selection.begin();
          p != selection.end();
          ++p)
      {
        (*p)->setLineColor(preferences->linecolor);
      }
      if (gtemplate)
        gtemplate->setLineColor(preferences->linecolor);
      window->invalidateWindow();
      break;
    case TFigurePreferences::FILLCOLOR:
      for(TFigureSet::iterator p = selection.begin();
          p != selection.end();
          ++p)
      {
        (*p)->setFillColor(preferences->fillcolor);
      }
      if (gtemplate)
        gtemplate->setFillColor(preferences->fillcolor);
      window->invalidateWindow();
      break;
    case TFigurePreferences::UNSETFILLCOLOR:
      for(TFigureSet::iterator p = selection.begin();
          p != selection.end();
          ++p)
      {
        (*p)->unsetFillColor();
      }
      if (gtemplate)
        gtemplate->unsetFillColor();
      window->invalidateWindow();
      break;
    case TFigurePreferences::LINEWIDTH:
    case TFigurePreferences::LINESTYLE:
    case TFigurePreferences::ARROWMODE:
    case TFigurePreferences::ARROWSTYLE:
      for(TFigureSet::iterator p = selection.begin();
          p != selection.end();
          ++p)
      {
        (*p)->setFromPreferences(preferences);
      }
      if (gtemplate)
        gtemplate->setFromPreferences(preferences);
      window->invalidateWindow();
      break;
    default:
      window->invalidateWindow();
  }
}

void
TFigureEditor::unsetFillColor()
{
  preferences->filled = false;
  TFigureSet::iterator p,e;
  p = selection.begin();
  e = selection.end();
  while(p!=e) {
    (*p)->unsetFillColor();
    p++;
  }
  if (gtemplate)
    gtemplate->unsetFillColor();
  window->invalidateWindow();
}

void
TFigureEditor::setFont(const string &fontname)
{
  preferences->fontname = fontname;
  TFigureSet::iterator p,e;
  p = selection.begin();
  e = selection.end();
  while(p!=e) {
    (*p)->setFont(fontname);
    p++;
  }
  if (gtemplate)
    gtemplate->setFont(fontname);
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
        if (group->mat) {
          TFigureModel::iterator vp,ve;
          vp = group->gadgets.begin();
          ve = group->gadgets.end();
          while(vp!=ve) {
            TMatrix2D *m = new TMatrix2D(*group->mat);
            if ((*vp)->mat) {
              m->multiply((*vp)->mat);
              delete((*vp)->mat);
            }
            (*vp)->mat = m;
            ++vp;
          }
        }
        model->insert(p, group->gadgets.begin(), group->gadgets.end());
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

void
TFigureEditor::setModel(TFigureModel *m)
{
  stopOperation();
  clearSelection();
  model = m;
  invalidateWindow();
  updateScrollbars();
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
  clearSelection();
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
  gtemplate->setLineColor(preferences->linecolor);
  if (preferences->filled)
    gtemplate->setFillColor(preferences->fillcolor);
  else
    gtemplate->unsetFillColor();
  gtemplate->setFont(preferences->fontname);
  gtemplate->removeable = true;
preferences->reason = TFigurePreferences::ALLCHANGED;
  gtemplate->setFromPreferences(preferences);
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
    
  if (key == TK_ESCAPE) {
    stopOperation();
    deleteSelection();
    return;
  }

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
TFigureEditor::mouse2sheet(int mx, int my, int *sx, int *sy)
{
  mx-=visible.x;
  my-=visible.y;
  if (mat) {
    TMatrix2D m(*mat);
    m.invert();
    m.map(mx, my, &mx, &my);
  }
  *sx = mx;
  *sy = my;
}

void
TFigureEditor::mouseEvent(TMouseEvent &me)
{
  int x = me.x + getOriginX();
  int y = me.y + getOriginY();
  
  switch(me.type) {
    case TMouseEvent::LDOWN:
    case TMouseEvent::MDOWN:
    case TMouseEvent::RDOWN:
/*
    // the would require some kind of grab emulation...
    case TMouseEvent::LUP:
    case TMouseEvent::MUP:
    case TMouseEvent::RUP:
    ...
*/
      if (row_header_renderer &&
          x < visible.x &&
          y >= visible.y ) 
      {
        me.x = x;
        row_header_renderer->mouseEvent(me);
      } else
      if (col_header_renderer &&
          x >= visible.x &&
          y < visible.y ) 
      {
        me.y = y;
        row_header_renderer->mouseEvent(me);
      } else
        super::mouseEvent(me);
      break;
    default:
      super::mouseEvent(me);
  }
}

void
TFigureEditorHeaderRenderer::mouseEvent(TMouseEvent &me)
{
}

void
TFigureEditor::sheet2grid(int sx, int sy, int *gx, int *gy)
{
  if (!preferences->drawgrid) {
    *gx = sx;
    *gy = sy;
    return;
  }
  if (state!=STATE_ROTATE && state!=STATE_MOVE_ROTATE) {
    int g = preferences->gridsize;
    *gx = ((sx+g/2)/g)*g;
    *gy = ((sy+g/2)/g)*g;
  } else {
    *gx = sx;
    *gy = sy;
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

  if (preferences)
    preferences->setCurrent(this);

  mouse2sheet(mx, my, &mx, &my);
  int x, y;
  sheet2grid(mx, my, &x, &y);

//cerr << "mouse down at " << mx << ", " << my << endl;
//cerr << " with grid at " << x << ", " << y << endl;

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

/* copied from findFigureAt */            
      int x, y;
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
                if (memo_pt.x-fuzziness<=x && x<=memo_pt.x+fuzziness && 
                    memo_pt.y-fuzziness<=y && y<=memo_pt.y+fuzziness) {
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
          } // end of handling the handles

          // selection, start movement, start edit
          //--------------------------------------
          TFigure *g = findFigureAt(mx, my);
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
            // no gaget at cursor
          
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

        case OP_ROTATE: {
          if (gadget) {
            if (rotx-fuzziness*2 <= mx && mx<=rotx+fuzziness*2 &&
                roty-fuzziness*2 <= my && my<=roty+fuzziness*2)
            {
//              cerr << "going to move rotation center" << endl;
              state = STATE_MOVE_ROTATE;
              return;
            }
          }
        
          TFigure *g2 = gadget;
          gadget = 0;
          TFigure *g = findFigureAt(mx, my);
          gadget = g2;
          if (g) {
            state = STATE_ROTATE;
            TRectangle r;
            g->getShape(&r);
            if (gadget!=g) {
              rotx = r.x + r.w/2;
              roty = r.y + r.h/2;
              gadget = g;
            }
            rotd0=atan2(static_cast<double>(my - roty), 
                        static_cast<double>(mx - rotx)) * 360.0 / (2.0 * M_PI);
            rotd = 0.0;
            invalidateWindow();
//            cerr << "state = STATE_ROTATE" << endl;
          }
          return; 
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
TFigureEditor::mouseMove(int mx, int my, unsigned m)
{

  #if VERBOSE
    cout << __PRETTY_FUNCTION__ << endl;
  #endif

  if (!window)
    return;

  mouse2sheet(mx, my, &mx, &my);
  int x, y;
  sheet2grid(mx, my, &x, &y);

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

/* copied from findFigureAt */
      int x2, y2;
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
      rotd=atan2(static_cast<double>(my - roty), 
                 static_cast<double>(mx - rotx)) * 360.0 / (2.0 * M_PI);
//      cerr << "rotd="<<rotd<<", rotd0="<<rotd0<<" -> " << (rotd-rotd0) << "\n";
      rotd-=rotd0;
      invalidateWindow();
    } break;
    
    case STATE_MOVE_ROTATE: {
      rotx = mx;
      roty = my;
      invalidateWindow();
    } break;
  }
}

void
TFigureEditor::mouseLUp(int mx, int my, unsigned m)
{
#if VERBOSE
  cout << __PRETTY_FUNCTION__ << endl;
#endif

  if (!window)
    return;

  mouse2sheet(mx, my, &mx, &my);
  int x, y;
  sheet2grid(mx, my, &x, &y);

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
        updateScrollbars();
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

/* copied from findFigureAt */            
      int x2, y2;
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
          (*p)->getShape(&r2);
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
#if 1
      TMatrix2D *m = new TMatrix2D();
      m->translate(rotx, roty);
      m->rotate(rotd);
      m->translate(-rotx, -roty);
      if (gadget->mat) {
        m->multiply(gadget->mat);
        delete gadget->mat;
      }
      gadget->mat = m;
#else
      if (!gadget->mat)
        gadget->mat = new TMatrix2D();
      gadget->mat->translate(rotx, roty);
      gadget->mat->rotate(rotd);
      gadget->mat->translate(-rotx, -roty);
#endif
      state = STATE_NONE;
      invalidateWindow();
    } break;
    
    case STATE_MOVE_ROTATE: {
/*      if (!gadget->mat)
        gadget->mat = new TMatrix2D();
      gadget->mat->translate(rotx, roty);
      gadget->mat->rotate(rotd);
      gadget->mat->translate(-rotx, -roty);
*/
      state = STATE_NONE;
    } break;
  }
}

void
TFigureEditor::mouseRDown(int mx, int my, unsigned modifier)
{
  if (!window)
    return;

  stopOperation();
//  clearSelection();
  setFocus();
  mouse2sheet(mx, my, &mx, &my);
  TFigure *f = findFigureAt(mx, my);
  if (f)
    f->mouseRDown(this, mx, my, modifier);
}

void
TFigureEditor::invalidateFigure(TFigure* figure)
{
  if (!window)
    return;

  TRectangle r;
  figure->getShape(&r);
  if (mat || figure->mat) {
    TMatrix2D m;
    if (mat) {
      m=*mat;
    }
    if (figure->mat)
      m.multiply(figure->mat);
      
    int x1, x2, y1, y2;
    int x, y;
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

  r.x+=window->getOriginX() + visible.x;
  r.y+=window->getOriginY() + visible.y;

  window->invalidateWindow(r);
}

/**
 * Find the gadget at position at (mx, my).
 *
 * This method doesn't find gadgets which are currently created or edited.
 */
TFigure*
TFigureEditor::findFigureAt(int mx, int my)
{
#if VERBOSE
  cerr << "TFigureEditor::findFigureAt(" << mx << ", " << my << ")\n";
#endif
  double distance = INFINITY;
  TFigureModel::iterator p,b,found;
  p = found = model->end();
  b = model->begin();
  TMatrix2D *stack = new TMatrix2D();
  while(p!=b) {
    --p;
    if (*p!=gadget) {
      int x, y;
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
  if (found == model->end())
    return NULL;
//  if (distance > TFigure::RANGE)
  if (distance > 0.5*fuzziness*TFigure::RANGE)
    return NULL;
  return *found;
}

void
TFigureEditor::adjustPane()
{
  if (!window)
    return;
  if (!use_scrollbars) {
    visible = pane;
    return;
  }
  visible.set(0,0,window->getWidth(), window->getHeight());
  if (row_header_renderer) {
    visible.x = row_header_renderer->getSize();
    visible.w -= visible.x;
  }
  if (col_header_renderer) {
    visible.y = col_header_renderer->getSize();
    visible.h -= visible.y;
  }
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

  x1 = y1 = INT_MAX;
  x2 = y2 = INT_MIN;

  TRectangle r;
  for(TFigureModel::iterator p = model->begin();
      p != model->end();
      ++p)
  {
    int ax1, ay1, ax2, ay2;
    (*p)->getShape(&r);
    ax1=r.x;
    ay1=r.y;
    ax2=r.x+r.w-1;
    ay2=r.y+r.h-1;
    
    if ( (*p)->mat) {
      (*p)->mat->map(ax1, ay2, &ax1, &ay1);
      (*p)->mat->map(ax2, r.y, &ax2, &ay2);

//printf("lower left  (%i, %i)\n"
//       "upper right (%i, %i)\n", ax1, ay1, ax2, ay2);
      
      if (ax1>ax2) {
        int a = ax1; ax1 = ax2; ax2 = a;
      }
      if (ay1>ay2) {
        int a = ay1; ay1 = ay2; ay2 = a;
      }

      
      if (ax1<x1)
        x1=ax1;
      if (ax2>x2)
        x2=ax2;
      if (ay1<y1)
        y1=ay1;
      if (ay2>y2)
        y2=ay2;

      ax1=r.x;
      ay1=r.y;
      ax2=r.x+r.w-1;
      ay2=r.y+r.h-1;

      (*p)->mat->map(ax1, ay1, &ax1, &ay1);
      (*p)->mat->map(ax2, ay2, &ax2, &ay2);
//printf("upper left  (%i, %i)\n"
//       "lower right (%i, %i)\n\n", ax1, ay1, ax2, ay2);
      if (ax1>ax2) {
        int a = ax1; ax1 = ax2; ax2 = a;
      }
      if (ay1>ay2) {
        int a = ay1; ay1 = ay2; ay2 = a;
      }
    }

    if (ax1<x1)
      x1=ax1;
    if (ax2>x2)
      x2=ax2;
    if (ay1<y1)
      y1=ay1;
    if (ay2>y2)
      y2=ay2;
//cout << "area size: (" << x1 << ", " << y1 << ") - (" << x2 << ", " << y2 << ")\n";
  }
  
  if (x1>0) x1=0;
  if (y1>0) y1=0;
  if (x2<0) x2=0;
  if (y2<0) y2=0;

//cout << "area size: (" << x1 << ", " << y1 << ") - (" << x2 << ", " << y2 << ")\n";
//cout << "final" << endl << endl;
  if (!mat) {
    pane.x = x1;
    pane.y = y1;
    pane.w = x2-x1+1;
    pane.h = y2-y1+1;
  } else {
    double dx1, dy1, dx2, dy2;
    mat->map(x1, y1, &dx1, &dy1);
    mat->map(x2+1, y2+1, &dx2, &dy2);
    pane.x = static_cast<int>(dx1);
    pane.y = static_cast<int>(dy1);
    pane.w = static_cast<int>(dx2-dx1);
    pane.h = static_cast<int>(dy2-dy1);
  }
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
