/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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
#include <toad/undomanager.hh>

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
 * \ingroup figure
 * \class toad::TFigureEditor
 * TFigureEditor is a graphical editor for TFigureModels.
 * 
 * TFigureEditor can be used as a window of its own or just as an object 
 * to delegate events from other windows to. The later was needed for
 * some of TOAD's dialog editors.
 *
 * Selection Mode
 *
 * The following keys control the selection mode and are choosen to match
 * the behaviour of selecting objects in TTable:
 *
 * \li click           : select and move object and its handles
 * \li SHIFT+click     : select area
 * \li CTRL+click      : select/deselect single object
 * \li SHIFT+CTRL+click: select additional area
 *
 * \todo
 *   \li
 *      undo, redo & model-view for selection2Top, selection2Bottom, 
 *      selectionUp, selectionDown and rotate
 *   \li
 *      group followed by undo causes a segfault or inifinite recursion
 *      or something like that
 *   \li
 *      scrollbars aren't setup properly during scaling
 *   \li
 *      resize groups
 *   \li
 *      adjust getShape to check transformations
 *   \li
 *      adjust finding figures & handles for transformations
 *   \li
 *      scrollbars aren't update after and during object creation
 *   \li
 *      polgons contain bogus points (can be seen during/after rotation)
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
 *      color changes aren't part of undo/redo
 */

/*
 * The 'gadget' attribute is used to
 * o identify the figure currently rotated
 * o ...
 *
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
  init(NULL);
  bExplicitCreate = true; // don't create, see TWindow::createParentless()
  window = NULL;
  row_header_renderer = col_header_renderer = 0;
}

TFigureEditor::TFigureEditor(TWindow *p, const string &t, TFigureModel *m):
  super(p, t)
{
  init(m);
  bNoBackground = true;
  window = this;
  row_header_renderer = col_header_renderer = 0;
}

TFigureEditor::~TFigureEditor()
{
  setModel(0);
  setAttributes(0);
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
  invalidateWindow();
  window = w;
  invalidateWindow();
}

static void
foobar(TFigureAttributes *p) {
  p->sigChanged();
}

TFigureAttributes::TFigureAttributes()
{
  linecolor.set(0,0,0);
  fillcolor.set(255,255,255);
  filled = false;
  fontname = "arial,helvetica,sans-serif:size=12";

  drawgrid = true;
  connect(drawgrid.sigChanged, foobar, this);
  gridsize = 4;
  connect(gridsize.sigChanged, foobar, this);
  
  linewidth = 0;
  linestyle = TPen::SOLID;
  arrowmode = TFLine::NONE;
  arrowtype = TFLine::EMPTY;
  
  current = 0;
}

TFigureAttributes::~TFigureAttributes()
{
}

void
TFigureAttributes::setOperation(unsigned op)
{
  if (current) current->setOperation(op);
}

#if 0
void
TFigureAttributes::setCreate(TFigure *figure)
{
  if (current) current->setCreate(figure);
}
#endif

void
TFigureAttributes::setTool(TFigureTool *tool)
{
  if (current) current->setTool(tool);
}

void
TFigureAttributes::group()
{
  if (current) current->group();
}

void
TFigureAttributes::ungroup()
{
  if (current) current->ungroup();
}

void
TFigureAttributes::selectionDown()
{
  if (current) current->selectionDown();
}

void
TFigureAttributes::selection2Bottom()
{
  if (current) current->selection2Bottom();
}

void
TFigureAttributes::selectionUp()
{
  if (current) current->selectionUp();
}

void
TFigureAttributes::selection2Top()
{
  if (current) current->selection2Top();
}

void
TFigureAttributes::applyAll()
{
  if (current) current->applyAll();
}

void
TFigureEditor::init(TFigureModel *m)
{
  preferences = 0;
  tool = 0;
  setAttributes(new TFigureAttributes);
  fuzziness = 2;

  handle = -1;
  gadget = NULL;
  operation = OP_SELECT;
  state = STATE_NONE;
  use_scrollbars = true;
  mat = 0;
//  vscroll = NULL;
//  hscroll = NULL;
  model = 0;
  if (!m)
    m=new TFigureModel();
  setModel(m);
  x1=y1=x2=y2=0;
  update_scrollbars = false;

  TAction *action;

  action = new TAction(this, "edit|cut");
  CONNECT(action->sigClicked, this, selectionCut);
  action = new TAction(this, "edit|copy");
  CONNECT(action->sigClicked, this, selectionCopy);
  action = new TAction(this, "edit|paste");
  CONNECT(action->sigClicked, this, selectionPaste);

  action = new TAction(this, "edit|delete");
  CONNECT(action->sigClicked, this, deleteSelection);
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
  if (b==preferences->drawgrid)
    return;
  preferences->drawgrid = b;
  invalidateWindow(visible);
}

/**
 * Set the size of the grid.
 */
void
TFigureEditor::setGrid(int gridsize) {
  if (gridsize<0)
    gridsize=0;
  preferences->gridsize = gridsize;
  invalidateWindow(visible);
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

  TPen scr(window);
  scr.identity();
  TRectangle r;
  scr.getClipBox(&r);
  TBitmap bmp(r.w, r.h, TBITMAP_SERVER);
  TPen pen(&bmp);

  pen.setColor(window->getBackground());
  pen.identity();
  pen.fillRectanglePC(0,0,r.w,r.h);
  pen.translate(window->getOriginX()+visible.x-r.x, 
                window->getOriginY()+visible.y-r.y);
  if (mat)
    pen.multiply(mat);
    
  paintGrid(pen);
  print(pen, true);
  paintSelection(pen);

  // put the result onto the screen
  scr.drawBitmap(r.x,r.y, &bmp);
  paintDecoration(scr);
}

/**
 * Called from 'paint' to draw the grid.
 */
void
TFigureEditor::paintGrid(TPenBase &pen)
{
  if (preferences->drawgrid && preferences->gridsize) {
    const TColor &background_color = window->getBackground();
    pen.setColor(
      background_color.r > 128 ? background_color.r-64 : background_color.r+64,
      background_color.g > 128 ? background_color.g-64 : background_color.g+64,
      background_color.b > 128 ? background_color.b-64 : background_color.b+64
    );
    int x1, x2, y1, y2;
    int g = preferences->gridsize;
  
    TRectangle r;
    pen.getClipBox(&r);
    x1=r.x;
    y1=r.y;
    x2=r.x+r.w+1;
    y2=r.y+r.h+1;

    TMatrix2D *mat = pen.getMatrix();
    if (mat) {
      int gx0, gx, gy0, gy;
      TMatrix2D m(*mat);   
      m.map(0, 0, &gx0, &gy0);
      m.map(preferences->gridsize, preferences->gridsize, &gx, &gy);
      gx-=gx0;
      gy-=gy0;
//cout << "gx,gy=" << gx << "," << gy << endl;
      m.invert();
      if (gx<=2 || gy<=2) {
        // don't draw grid, it's too small
        return;
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
}  

/**
 * Called from 'paint' to draw the selection marks.
 */
void
TFigureEditor::paintSelection(TPenBase &pen)
{
  // draw the selection marks over all figures
  for(TFigureSet::iterator sp = selection.begin();
      sp != selection.end();
      ++sp)
  {
    if ((*sp)->mat) {
      pen.push();
      pen.multiply( (*sp)->mat );
    }
    pen.setLineWidth(1);
    if (*sp!=gadget) {
      (*sp)->paintSelection(pen, -1);
    } else {
      (*sp)->paintSelection(pen, handle);
    }
    if ((*sp)->mat)
      pen.pop();
  }

  if (state==STATE_SELECT_RECT) {
    if (pen.mat) {
      TMatrix2D *mat = pen.mat;
      pen.push();
      pen.identity();
      pen.setColor(0,0,0);
      pen.setLineStyle(TPen::DOT);
      pen.setLineWidth(1.0);
      int x0, y0, x1, y1;
      mat->map(down_x, down_y, &x0, &y0);
      mat->map(select_x, select_y, &x1, &y1);
      pen.drawRectanglePC(x0, y0, x1-x0, y1-y0);
      pen.pop();
    } else {
      pen.setColor(0,0,0);
      pen.setLineStyle(TPen::DOT);
      pen.setLineWidth(0);
      pen.drawRectanglePC(down_x, down_y, select_x-down_x, select_y-down_y);
    }
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
    pen.setLineWidth(1);
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

  if (tool) {
    tool->paintSelection(pen);
  }
}

/**
 * Called from 'paint' to draw the corners and the row and column headers
 */
void
TFigureEditor::paintDecoration(TPenBase &scr)
{
  if (window==this)
    paintCorner(scr);
  // else ... not implemented yet
  
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

/**
 * Draw all figures.
 *
 * This method is called from 'paint' to draw the whole drawing area.
 * It's provided as an separate method so users can also draw on other
 * devices than the screen.
 *
 * \param pen
 *   The pen to be used, ie. TPen for the screen or TPrinter for the printer.
 * \param withSelection
 *   When set to 'true', the method will call the paint method of all
 *   selected figures with TFigure::SELECT as 2nd parameter. The figure
 *   TFText uses this to draw the text caret when in edit mode.
 *
 *   Handles to move, resize and rotate figures are drawn by the paint
 *   method itself.
 */  
void
TFigureEditor::print(TPenBase &pen, bool withSelection)
{
  if (!model)
    return;
    
  TRectangle cb, r;
  pen.getClipBox(&cb);
  for(TFigureModel::iterator p = model->begin();
      p != model->end();
      ++p)
  {
    TRectangle r;
    getFigureShape(*p, &r, pen.getMatrix());
    if (!r.intersects(cb)) {
      continue;
    }

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
    
    if (withSelection && gadget!=*p && selection.find(*p)!=selection.end()) {
      pt = TFigure::SELECT;
    }
    (*p)->paint(pen, pt);
    while(pushs) {
      pen.pop();
      pushs--;
    }
  }
}

void 
TFigureEditor::setAttributes(TFigureAttributes *p) {
  if (preferences) {
    disconnect(preferences->sigChanged, this);
    if (preferences->getCurrent() == this)
      preferences->setCurrent(0);
  }
  preferences = p;
  if (preferences) {
    preferences->setCurrent(this);
    connect(preferences->sigChanged, this, &TThis::preferencesChanged);
  }
}

void
TFigureEditor::preferencesChanged()
{
  if (!preferences)
    return;

  if (tool)
    tool->setAttributes(preferences);
    
  model->setAttributes(selection, preferences);
//  invalidateWindow(visible); 
}

void
TFigureEditor::setLineColor(const TRGB &rgb)
{
  preferences->linecolor = rgb;
  preferencesChanged();
}

void
TFigureEditor::setFillColor(const TRGB &rgb)
{
  preferences->setFillColor(rgb);
}

void
TFigureEditor::unsetFillColor()
{
  preferences->filled = false;
  preferencesChanged();
}

void
TFigureEditor::setFont(const string &fontname)
{
  preferences->fontname = fontname;
  preferencesChanged();
}

void
TFigureEditor::modelChanged()
{
  switch(model->type) {
    case TFigureModel::MODIFY:
    case TFigureModel::MODIFIED:
    case TFigureModel::ADD:
      update_scrollbars = true;
      for(TFigureSet::iterator p=model->figures.begin();
          p!=model->figures.end();
          ++p)
      {
        invalidateFigure(*p);
      }
      break;
    case TFigureModel::REMOVE:
      update_scrollbars = true;
      for(TFigureSet::iterator p=model->figures.begin();
          p!=model->figures.end();
          ++p)
      {
        invalidateFigure(*p);
        TFigureSet::iterator q = selection.find(*p);
        if (q!=selection.end()) {
          selection.erase(q);
        }
      }
      break;
    case TFigureModel::GROUP:
      #warning "not removing figure from selection"
      selection.clear();
      selection.insert(model->figure);
      invalidateFigure(model->figure);
      break;
    case TFigureModel::_UNDO_GROUP:
      invalidateFigure(model->figure);
      selection.clear();
      selection.insert(model->figures.begin(), model->figures.end());
      break;
    case TFigureModel::UNGROUP:
      #warning "not removing figure (group) from selection"
      invalidateWindow(visible); // OPTIMIZE ME
      break;
  }
}

/**
 * Add a figure to the editors model.
 */
void
TFigureEditor::addFigure(TFigure *figure)
{
  model->add(figure);
}

/**
 * Removes a figure from the editors model.
 */
void
TFigureEditor::deleteFigure(TFigure *g)
{
  if (g==gadget)
    gadget=NULL;

  TFigureSet::iterator s;
  s = selection.find(g);
  if (s!=selection.end())
    selection.erase(s); 

  model->erase(g);
}

bool
TFigureEditor::clearSelection()
{
  if (selection.empty())
    return false;
  for(TFigureSet::iterator p = selection.begin();
      p != selection.end();
      ++p)
  {
    invalidateFigure(*p);
  }
  selection.erase(selection.begin(), selection.end());
  return true;
}

/**
 * Delete all selected objects with `removeable' being true.
 */
void
TFigureEditor::deleteSelection()
{
//cout << "delete selection" << endl;

  if (gadget && selection.find(gadget)!=selection.end()) {
    gadget = 0;
  }
  model->erase(selection);
}

void
TFigureEditor::selectAll()
{
  for(TFigureModel::iterator p = model->begin();
      p != model->end();
      ++p)
  {
    selection.insert(*p);
  }
  invalidateWindow(visible);
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
  if (model)
    model->group(selection);
}

void
TFigureEditor::ungroup()
{
  if (model)
    model->ungroup(selection, &selection);
}

void
TFigureEditor::setModel(TFigureModel *m)
{
  if (model==m)
    return;
  if (model) {
    stopOperation();
    clearSelection();
    disconnect(model->sigChanged, this);
    TUndoManager::unregisterModel(this, model);
  }
  model = m;
  if (model) {
    connect(model->sigChanged, this, &TFigureEditor::modelChanged);
    TUndoManager::registerModel(this, model);
  }
  if (isRealized()) {
    invalidateWindow();
    updateScrollbars();
  }
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
  tool = 0;
}

void
TFigureEditor::setTool(TFigureTool *tool)
{
  if (tool)
    tool->stop(this);
  this->tool = tool;
}

void
TFigureEditor::applyAll()
{
  preferences->reason = TFigureAttributes::ALLCHANGED;
  model->setAttributes(selection, preferences);
}

/**
 * Abort the current operation mode.
 */
void
TFigureEditor::stopOperation()
{
  if (tool) {
    tool->stop(this);
  } else {
    switch(state) {
      case STATE_CREATE:
        clearSelection();
        if (gadget) {
          // selection.insert(gadget);
          model->figures.clear();
          model->figures.insert(gadget);
          model->type = TFigureModel::MODIFIED;
          model->sigChanged();
        }
        window->setMouseMoveMessages(TMMM_ANYBUTTON);
        break;
    }
  }
  gadget = NULL;
  state = STATE_NONE;
}

namespace {
  TFigureVector clipboard;
}

void
TFigureEditor::selectionCut()
{
  selectionCopy();
  deleteSelection();
}

void
TFigureEditor::selectionCopy()
{
  for(TFigureVector::iterator p = clipboard.begin();
      p != clipboard.end();
      ++p)
  {
    delete *p;
  }
  clipboard.clear();

  for(TFigureModel::iterator p = model->begin();
      p != model->end();  
      ++p)
  {
    if (selection.find(*p)!=selection.end()) {
      clipboard.push_back( static_cast<TFigure*>( (*p)->clone() ) );
    }
  }
}

void
TFigureEditor::selectionPaste()
{
  clearSelection();

  TFigureVector copy;
  for(TFigureVector::iterator p = clipboard.begin();
      p != clipboard.end();
      ++p)
  {
    TFigure *f = static_cast<TFigure*>( (*p)->clone() );
    selection.insert(f);
    copy.push_back(f);
  }
  model->add(copy);
}

void
TFigureEditor::keyEvent(TKeyEvent &ke)
{
  if (tool) {
    tool->keyEvent(this, ke);
  } else {
    TWindow::keyEvent(ke);
  }
}

void
TFigureEditor::keyDown(TKey key, char *s, unsigned m)
{
  if (!window || !model || tool)
    return;

  if (key == TK_ESCAPE) {
#if 0
    if (operation==OP_CREATE) {
      stopOperation();
      deleteSelection();
    } else {
#endif
      stopOperation();
      clearSelection();
#if 0
    }
#endif
    return;
  }

redo:
  switch(operation) {
    case OP_SELECT: {
      if (state!=STATE_EDIT) {
        if (m & MK_CONTROL) {
          switch(key) {
            case 'x':
            case 'X':
              selectionCut();
              break;
            case 'c':
            case 'C':
              selectionCopy();
              break;
            case 'v':
            case 'V':
              selectionPaste();
              break;
          }
        } else {
          switch(key) {
            case TK_DELETE:
            case TK_BACKSPACE:
              deleteSelection();
              break;
          }
        }
        break;
      }

      if (state==STATE_NONE) {
        if (key==TK_DELETE || key==TK_BACKSPACE)
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
      } else {
        if (!tool) {
          super::mouseEvent(me);
          return;
        }
      }
      break;
    default:
      if (!tool) {
        super::mouseEvent(me);
        return;
      }
  }
  
  if (!tool)
    return;
  if (!window)
    return;
    
  if (me.type==TMouseEvent::LDOWN ||
      me.type==TMouseEvent::MDOWN ||
      me.type==TMouseEvent::RDOWN)
  {
    setFocus();
    if (preferences)
      preferences->setCurrent(this);
  }  
    
  tool->mouseEvent(this, me);
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

namespace {
  bool mouseMoved;
}

void
TFigureEditor::mouseLDown(int mx, int my, unsigned m)
{
  #if VERBOSE
    cout << __PRETTY_FUNCTION__ << endl;
  #endif

  if (!window || !model)
    return;
  setFocus();

  if (preferences)
    preferences->setCurrent(this);

  mouse2sheet(mx, my, &mx, &my);
  int x, y;
  sheet2grid(mx, my, &x, &y);

//cerr << "mouse down at " << mx << ", " << my << endl;
//cerr << " with grid at " << x << ", " << y << endl;

  down_x = x;
  down_y = y;

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
            for(TFigureSet::iterator p=selection.begin();
                p!=selection.end();
                ++p)
            {
              // map desktop (mx,my) to figure (x,y) (copied from findFigureAt)
              int x, y;
              if ((*p)->mat) {
                TMatrix2D m(*(*p)->mat);
                m.invert();
                m.map(mx, my, &x, &y);
              } else {
                x = mx;
                y = my;
              }

              // loop over all handles
              unsigned h = 0;
              while(true) {
                if (!(*p)->getHandle(h,&memo_pt))
                  break;
                if (memo_pt.x-fuzziness<=x && x<=memo_pt.x+fuzziness && 
                    memo_pt.y-fuzziness<=y && y<=memo_pt.y+fuzziness) {
                  #if VERBOSE
                    cout << "      found handle at cursor => STATE_MOVE_HANDLE" << endl;
                  #endif
                  handle = h;
                  gadget = *p;
                  #if VERBOSE
                  cout << "      handle " << h << " @ " << memo_pt.x << ", " << memo_pt.y << endl;
                  #endif
                  state = STATE_MOVE_HANDLE;
                  mouseMoved = false;
                  if (selection.size()>1) {
                    clearSelection();
                    selection.insert(gadget);
                    sigSelectionChanged();
                  }
                  invalidateFigure(gadget);
                  return;
                }
                h++;
              }
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
                invalidateFigure(gadget);
                state = STATE_EDIT;
                goto redo;
              }
              #if VERBOSE
                cout << "not editing" << endl;
              #endif
            } else 
            
            if (m & MK_CONTROL) {
              #if VERBOSE
                cout << ", control => ";
              #endif
              if (! (m&MK_SHIFT)) {
                #if VERBOSE
                  cout << "  adding object to selection" << endl;
                #endif
                if (gi==selection.end()) {
                  selection.insert(g);
                } else {
                  selection.erase(gi);
                }
                invalidateFigure(g);
                sigSelectionChanged();
              } else {
                state =  STATE_SELECT_RECT;
                select_x = x;
                select_y = y;
              }
            } else {
              #if VERBOSE
                cout << " => ";
              #endif
              if (gi==selection.end()) {
                clearSelection();
                selection.insert(g);
                invalidateFigure(g);
                sigSelectionChanged();
                if (m&MK_SHIFT) {
                  state =  STATE_SELECT_RECT;
                  select_x = x;
                  select_y = y;
                } else {
                  memo_x = memo_y = 0;
                  state = STATE_MOVE;
                  TUndoManager::beginUndoGrouping();
                }
              } else {
                if (m&MK_SHIFT) {
                  clearSelection();
                  selection.insert(g);
                  invalidateFigure(g);
                  sigSelectionChanged();
                  state =  STATE_SELECT_RECT;
                  select_x = x;
                  select_y = y;
                } else {
                  state = STATE_MOVE;
                  memo_x = memo_y = 0;
                  TUndoManager::beginUndoGrouping();
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
            if (!(m & MK_CONTROL)) {
              if (clearSelection())
                sigSelectionChanged();
            }
            state =  STATE_SELECT_RECT;
            select_x = x;
            select_y = y;
          }
        } break; // end of OP_SELECT

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
            invalidateWindow(visible);
//            cerr << "state = STATE_ROTATE" << endl;
          }
          return; 
        } break; // end of OP_ROTATE
      }
    } break; // end of STATE_NONE
    
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
        if (m & MK_DOUBLE) {
          cerr << "TFigureEditor: kludge: avoiding endless loop bug\n";
          break;
        }
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
//cout << "mouseMove for window " << window->getTitle() << endl;
  #if VERBOSE
    cout << __PRETTY_FUNCTION__ << endl;
  #endif

  if (!window || !model)
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
      memo_x+=dx;
      memo_y+=dy;
      model->translate(selection, dx, dy);
    } break;

    case STATE_MOVE_HANDLE: {
      #if VERBOSE
        cout << "  STATE_MOVE_HANDLE => moving handle" << endl;
      #endif
      
      if (!mouseMoved) {
        if (down_x!=x || down_y != y) {
          mouseMoved = true;
          TUndoManager::beginUndoGrouping();
        }
      }

      if (mouseMoved) {
        /* copied from findFigureAt */
        int x2, y2;
        if (gadget->mat) {
          TMatrix2D m(*gadget->mat);
          m.invert();
          m.map(x, y, &x2, &y2);
        } else {
          x2 = x;
          y2 = y;
        }

        model->translateHandle(gadget, handle, x2, y2, m);
      }
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
        if (!(m&MK_CONTROL)) {
          clearSelection();
          sigSelectionChanged();
        }
      }
      #if VERBOSE
        cout << "  STATE_SELECT_RECT => redrawing rectangle" << endl;
      #endif
      if (select_x != x || select_y != y) {
        TRectangle r;
        
        r.set(down_x, down_y, x-down_x, y-down_y);
        if (mat) {
          mat->map(r.x, r.y, &r.x, &r.y);
          mat->map(r.w, r.h, &r.w, &r.h);
        }
        r.x+=window->getOriginX()+visible.x;
        r.y+=window->getOriginY()+visible.y;
        r.w++;
        r.h++;
        invalidateWindow(r);
        
        r.set(down_x, down_y, select_x-down_x, select_y-down_y);
        if (mat) {
          mat->map(r.x, r.y, &r.x, &r.y);
          mat->map(r.w, r.h, &r.w, &r.h);
        }
        r.x+=window->getOriginX()+visible.x;
        r.y+=window->getOriginY()+visible.y;
        r.w++;
        r.h++;
        invalidateWindow(r);
        
        select_x = x;
        select_y = y;
      }
    } break;
    
    case STATE_ROTATE: {
      rotd=atan2(static_cast<double>(my - roty), 
                 static_cast<double>(mx - rotx)) * 360.0 / (2.0 * M_PI);
//      cerr << "rotd="<<rotd<<", rotd0="<<rotd0<<" -> " << (rotd-rotd0) << "\n";
      rotd-=rotd0;
      invalidateWindow(visible);
    } break;
    
    case STATE_MOVE_ROTATE: {
      rotx = mx;
      roty = my;
      invalidateWindow(visible);
    } break;
  }
}

void
TFigureEditor::mouseLUp(int mx, int my, unsigned m)
{
#if VERBOSE
  cout << __PRETTY_FUNCTION__ << endl;
#endif

  if (!window || !model)
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
      TUndoManager::endUndoGrouping();
      state = STATE_NONE;
    } break;

    case STATE_MOVE_HANDLE: {
      #if VERBOSE
        cout << "  STATE_MOVE_HANDLE => updating scrollbars, STATE_NONE" << endl;
      #endif

      if (mouseMoved) {      
        /* copied from findFigureAt */            
        int x2, y2;
        if (gadget->mat) {
          TMatrix2D m(*gadget->mat);
          m.invert();
          m.map(x, y, &x2, &y2);
        } else {
          x2 = x;
          y2 = y;
        }

        model->translateHandle(gadget, handle, x2, y2,m);
        TUndoManager::endUndoGrouping();
      }
      invalidateFigure(gadget);

      state = STATE_NONE;
      gadget = 0;
      handle = -1;
      updateScrollbars();
    } break;

    case STATE_SELECT_RECT: {
      #if VERBOSE
        cout << "  STATE_SELECT_RECT => ";
      #endif
      TRectangle r;
      
      r.set(down_x, down_y, x-down_x, y-down_y);
      if (mat) {
        mat->map(r.x, r.y, &r.x, &r.y);
        mat->map(r.w, r.h, &r.w, &r.h);
      }
      r.x+=window->getOriginX()+visible.x;
      r.y+=window->getOriginY()+visible.y;
      r.w++;
      r.h++;
      invalidateWindow(r);
        
      r.set(down_x, down_y, select_x-down_x, select_y-down_y);
      if (mat) {
        mat->map(r.x, r.y, &r.x, &r.y);
        mat->map(r.w, r.h, &r.w, &r.h);
      }
      r.x+=window->getOriginX()+visible.x;
      r.y+=window->getOriginY()+visible.y;
      r.w++;
      r.h++;
      invalidateWindow(r);

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
  if (!window || !model)
    return;

//  stopOperation();
//  clearSelection();
  setFocus();
  mouse2sheet(mx, my, &mx, &my);
  TFigure *f = findFigureAt(mx, my);
  if (f) {
    if (state==STATE_NONE) {
      clearSelection();
      selection.insert(f);
      invalidateFigure(f);
    }
    unsigned op = f->mouseRDown(this, mx, my, modifier);
    if (op & TFigure::STOP) {
      stopOperation();
      clearSelection();
    }
    if (op & TFigure::DELETE) {
      deleteSelection();
    }
  }
}

void
TFigureEditor::invalidateFigure(TFigure* figure)
{
  if (!window)
    return;
  TRectangle r;
//figure->getShape(&r);
//cout << figure->getClassName() << ": invalidate shape " <<r.x<<"-"<<(r.x+r.w)<<","<<r.y<<"-"<<(r.y+r.h)<<endl;
  getFigureShape(figure, &r, mat);
  r.x+=window->getOriginX() + visible.x;
  r.y+=window->getOriginY() + visible.y;
//cout << figure->getClassName() << ": invalidate window " <<r.x<<"-"<<(r.x+r.w)<<","<<r.y<<"-"<<(r.y+r.h)<<endl;
  invalidateWindow(r);
}

void
TFigureEditor::getFigureShape(TFigure* figure, TRectangle *r, TMatrix2D *mat)
{
  figure->getShape(r);

  if (mat || figure->mat) {
    TMatrix2D m;
    if (mat) {
      m=*mat;
    }
    if (figure->mat)
      m.multiply(figure->mat);
      
    int x1, x2, y1, y2;
    int x, y;
    m.map(r->x, r->y, &x, &y);
    x1 = x2 = x;
    y1 = y2 = y;
    for(int i=1; i<4; ++i) {
      switch(i) {
        case 1:
          m.map(r->x+r->w, r->y, &x, &y);
          break;
        case 2:
          m.map(r->x+r->w, r->y+r->h, &x, &y);
          break;
        case 3:
          m.map(r->x, r->y+r->h, &x, &y);
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
    r->set(TPoint(x1,y1), TPoint(x2, y2));
  }
//cout << "invalidating shape " << r.x << "," << r.y << "," << r.w << "," << r.h << endl;

  // add some extra for the figure handles
  // (this would be be delegated to the figure itself, as the figure contains
  // the code to draw the handles!)
  r->x-=3;
  r->y-=3;
  r->w+=6;
  r->h+=6;
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
  TMatrix2D stack;

  double inside = 0.4 * fuzziness * TFigure::RANGE;

  bool stop = false;
  while(p!=b && !stop) {
    --p;
    if (*p!=gadget) {
      int x, y;
      if ((*p)->mat) {
        stack.multiply((*p)->mat);
        stack.invert();
        stack.map(mx, my, &x, &y);
      } else {
        x = mx;
        y = my;
      }
//cerr << "  after rotation ("<<x<<", "<<y<<")\n";
      double d = (*p)->_distance(this, x, y);
      if (d==TFigure::INSIDE) {
        d = inside;
        stop = true;
      }
//cerr << "  distance = " << d << endl;
      stack.identity(); // why is this instruction here?
      if (d<distance) {
        distance = d;
        found = p;
      }
    }
  }

  if (found == model->end())
    return NULL;

//  if (distance > TFigure::RANGE)
  if (distance > 0.5*fuzziness*TFigure::RANGE) {
    return NULL;
  }
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

  if (model) {

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

TFigureTool::~TFigureTool()
{
}

void
TFigureTool::stop(TFigureEditor*)
{
}

void
TFigureTool::mouseEvent(TFigureEditor *fe, TMouseEvent &me)
{
}

void
TFigureTool::keyEvent(TFigureEditor *fe, TKeyEvent &ke)
{
}

void
TFigureTool::setAttributes(TFigureAttributes *p)
{
}

void
TFigureTool::paintSelection(TPenBase &)
{
}

void
TFCreateTool::stop(TFigureEditor *fe)
{
  if (figure) {
    unsigned r = figure->stop(fe);
    if ( r & TFigure::DELETE )
      fe->deleteFigure(figure);
    figure = 0;
    fe->setCurrent(0);
    fe->getWindow()->ungrabMouse();
  }
  fe->state = TFigureEditor::STATE_NONE;
}

void
TFCreateTool::mouseEvent(TFigureEditor *fe, TMouseEvent &me)
{
//cout << "TFCreateTool::mouseEvent" << endl;
  int x0, y0, x1, y1;
  unsigned r;

redo:

  switch(fe->state) {
    case TFigureEditor::STATE_NONE:
      switch(me.type) {
        case TMouseEvent::LDOWN:
//          cout << "TFCreateTool: LDOWN" << endl;
//          cout << "TFCreateTool: start create" << endl;
          fe->mouse2sheet(me.x, me.y, &x0, &y0);
          fe->sheet2grid(x0, y0, &x1, &y1);
          fe->clearSelection();
          figure = static_cast<TFigure*>(tmpl->clone());
          fe->setCurrent(figure);
//cout << "  new figure " << figure << endl;
          figure->removeable = true;
          fe->getAttributes()->reason = TFigureAttributes::ALLCHANGED;
          figure->setAttributes(fe->getAttributes());
          fe->addFigure(figure);
          figure->startCreate();
          fe->state = TFigureEditor::STATE_START_CREATE;
          r = figure->mouseLDown(fe, x1, y1, me.modifier);
          fe->state = TFigureEditor::STATE_CREATE;
          if (r & TFigure::DELETE) {
//            cout << "  delete" << endl;
            fe->deleteFigure(figure);
            figure = 0;
          }
          if (r & TFigure::STOP) {
//cout << "  stop" << endl;
            fe->state = TFigureEditor::STATE_NONE;
            fe->getWindow()->ungrabMouse();
            fe->setCurrent(0);
            if (figure) {
              TFigureModel *model = fe->getModel();
              model->figures.clear();
              model->figures.insert(figure);
              model->type = TFigureModel::MODIFIED;
              model->sigChanged();
            }
          }
          if (fe->state != TFigureEditor::STATE_NONE &&
              !(r & TFigure::NOGRAB) )
          {
            fe->getWindow()->grabMouse(TWindow::TMMM_ALL);
          }
          if (r & TFigure::REPEAT) {
//            cout << "  repeat" << endl;
            goto redo;
          }
          break;
        default:
//          cout << "TFCreateTool: unhandled mouse event in state 0" << endl;
          break;
      }
      break;
    
    case TFigureEditor::STATE_CREATE:
      fe->mouse2sheet(me.x, me.y, &x0, &y0);
      fe->sheet2grid(x0, y0, &x1, &y1);

      switch(me.type) {
        case TMouseEvent::LDOWN:
//          cout << "TFCreateTool: mouseLDown during create" << endl;
          r = figure->mouseLDown(fe, x1, y1, me.modifier);
          break;
        case TMouseEvent::MOVE:
//          cout << "TFCreateTool: mouseMove during create" << endl;
          r = figure->mouseMove(fe, x1, y1, me.modifier);
          break;
        case TMouseEvent::LUP:
//          cout << "TFCreateTool: mouseLUp during create" << endl;
          r = figure->mouseLUp(fe, x1, y1, me.modifier);
          break;
        case TMouseEvent::RDOWN:
//          cout << "TFCreateTool: mouseRDown during create" << endl;
          r = figure->mouseRDown(fe, x1, y1, me.modifier);
          break;
        default:
//          cout << "TFCreateTool: unhandled mouse event in state 1" << endl;
          return;
      }

      if (r & TFigure::DELETE) {
//        cout << "  delete figure" << endl;
        fe->deleteFigure(figure);
        figure = 0;
      }
      if (r & TFigure::STOP) {
//        cout << "  stop" << endl;
        // fe->stopOperation();
        fe->getWindow()->ungrabMouse();
        fe->state = TFigureEditor::STATE_NONE;
        fe->setCurrent(0);
        if (figure) {
          TFigureModel *model = fe->getModel();
          model->figures.clear();
          model->figures.insert(figure);
          model->type = TFigureModel::MODIFIED;
          model->sigChanged();
          figure = 0;
        }
      }
      if (r & TFigure::REPEAT) {
//        cout << "  repeat" << endl;
        if (me.modifier & MK_DOUBLE) {
//          cerr << "TFigureEditor: kludge: avoiding endless loop bug\n";
          break;
        }
        goto redo;
      }
      break;
  }
  if (figure)
    fe->invalidateFigure(figure);
}

void
TFCreateTool::keyEvent(TFigureEditor *fe, TKeyEvent &ke)
{
  if (!figure || ke.type != TKeyEvent::DOWN)
    return;
  if (ke.getKey() == TK_ESCAPE) {
    fe->deleteFigure(figure);
    fe->state = TFigureEditor::STATE_NONE;
    fe->getWindow()->ungrabMouse();
    figure = 0;
    return;
  }

  unsigned r = figure->keyDown(fe, ke.getKey(), const_cast<char*>(ke.getString()), ke.getModifier());
  if (r & TFigure::DELETE) {
//        cout << "  delete figure" << endl;
    fe->deleteFigure(figure);
    figure = 0;
  }
  if (r & TFigure::STOP) {
    fe->getWindow()->ungrabMouse();
    fe->state = TFigureEditor::STATE_NONE;
    if (figure) {
      TFigureModel *model = fe->getModel();
      model->figures.clear();
      model->figures.insert(figure);
      model->type = TFigureModel::MODIFIED;
      model->sigChanged();
      figure = 0;
    }
  }
  if (figure)
    fe->invalidateFigure(figure);
}

void
TFCreateTool::setAttributes(TFigureAttributes *a)
{
  if (figure)
    figure->setAttributes(a);
}
