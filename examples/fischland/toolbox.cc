/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2007 by Mark-André Hopf <mhopf@mark13.org>
 * Visit http://www.mark13.org/fischland/.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "toolbox.hh"
#include "selectiontool.hh"
#include "filltool.hh"
#include "pentool.hh"
#include "penciltool.hh"
#include "directselectiontool.hh"
#include "colorpalette.hh"
#include "fischland.hh"
#include "cairo.hh"

#include <toad/combobox.hh>
#include <toad/pushbutton.hh>
#include <toad/fatradiobutton.hh>
#include <toad/fatcheckbutton.hh>
#include <toad/textfield.hh>
#include <toad/colorselector.hh>
#include <toad/colordialog.hh>
#include <toad/gauge.hh>
#include <toad/figure.hh>

#include "fontdialog.hh"
#include "fpath.hh"

using namespace fischland;

static const TRGB white(255,255,255);
static const TRGB black(0,0,0);

class TArrowTypeAdapter:
  public TSimpleTableAdapter
{

  public:
    size_t getRows() { return 7; }
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = 35;
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = 18;
          break;
        case TTableEvent::PAINT: {
          renderBackground(te);
          TPoint p1(30,9);
          TPoint p2(5,9);
          te.pen->drawLine(p1, p2);
          TFLine::drawArrow(*te.pen,
                            p1, p2,
                            black, white,
                            8, 16,
                            static_cast<TFLine::EArrowType>(te.row));
          renderCursor(te);
        } break;
      }
    }
};

class TArrowModeAdapter:
  public TSimpleTableAdapter
{

  public:
    size_t getRows() { return 4; }
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = 35;
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = 18;
          break;
        case TTableEvent::PAINT: {
          renderBackground(te);
          TPoint p1(30,9);
          TPoint p2(5,9);
          te.pen->drawLine(p1, p2);
          if (te.row==1 || te.row==3)
            TFLine::drawArrow(*te.pen, p1, p2, black, white, 5, 10, TFLine::FILLED);
          if (te.row==2 || te.row==3)
            TFLine::drawArrow(*te.pen, p2, p1, black, white, 5, 10, TFLine::FILLED);
          renderCursor(te);
        } break;
      }
    }
};

class TLineStyleAdapter:
  public TSimpleTableAdapter
{
  public:
    size_t getRows() { return 5; }
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = 35;
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = 18;
          break;
        case TTableEvent::PAINT: {
          renderBackground(te);
          TPoint p1(30,9);
          TPoint p2(5,9);
          te.pen->setLineWidth(2);
          te.pen->setLineStyle(static_cast<TPenBase::ELineStyle>(te.row+1));
          te.pen->drawLine(p1, p2);
          renderCursor(te);
        } break;
      }
    }

};

class TLineWidthAdapter:
  public TSimpleTableAdapter
{
  public:
    size_t getRows() { return 12; }
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = 35;
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = te.row+6;
          break;
        case TTableEvent::PAINT: {
          renderBackground(te);
          TPoint p1(30,te.h/2);
          TPoint p2(5,te.h/2);
          te.pen->setLineColor(0,0,0);
          te.pen->setLineWidth(te.row);
          te.pen->drawLine(p1, p2);
          te.pen->setLineWidth(0);
          renderCursor(te);
        } break;
      }
    }

};

class TFontButton:
  public TPushButton
{
    TFigureAttributes *fa;
  public:
    TFontButton(TWindow *parent, const string &title, TFigureAttributes *fa):
      TPushButton(parent, title)
    {
      this->fa = fa;
      CONNECT(fa->sigChanged, this, fontChanged);
    }
    
    ~TFontButton() {
      disconnect(fa->sigChanged, this);
    }
    
    void fontChanged() {
      if (fa->reason == TFigureAttributes::ALLCHANGED || 
          fa->reason == TFigureAttributes::FONTNAME)
      {
        invalidateWindow();
      }
    }
    
    void paint() {
      TPen pen(this);
      pen.scale(1.0/96.0, 1.0/96.0);
      pen.setFont(fa->getFont());
      const char *text = "F";
      TCoord n =(bDown && bInside)?1:0;
      TCoord x = (getWidth()*96-pen.getTextWidth(text)) / 2;
      TCoord y = (getHeight()*96-pen.getHeight()) / 2;
      pen.setColor(TColor::BTNTEXT);
      pen.drawString(x+n, y+n, text);
      if (isFocus()) {
        pen.setLineStyle(TPen::DOT);
        pen.drawRectanglePC(x+n-3,y+n-1,pen.getTextWidth(text)+6,pen.getHeight()+1);
        pen.setLineStyle(TPen::SOLID);
      }
      pen.identity();
      drawShadow(pen, bDown && bInside);
    }
};

TToolBox* TToolBox::toolbox = 0;
PFigureAttributes TToolBox::preferences;

void
setLineWidth(TSingleSelectionModel *model)
{
  int n = model->getRow();
  if (n==0)
    n=48;
  else
    n*=96;
  if (TToolBox::preferences->linewidth != n) {
    TToolBox::preferences->reason = TFigureAttributes::LINEWIDTH;
    TToolBox::preferences->linewidth = n;
    TToolBox::preferences->sigChanged();
  }
}

void
TToolBox::preferencesChanged()
{
  int n;

  // lw_sm, line width selection model
  n = TToolBox::preferences->linewidth;
  if (n==48)
    n=0;
  else
    n/=96;
  if (n != lw_sm->getRow()) {
 //   cout << "line width changed from " << lw_sm->getRow() << " to " << n << endl;
    lw_sm->select(0, n);
  }

  n = TToolBox::preferences->linestyle - 1;
  if (n != ls_sm->getRow()) {
 //   cout << "line style changed from " << ls_sm->getRow() << " to " << n << endl;
    ls_sm->select(0, n);
  } 

  n = TToolBox::preferences->arrowmode;
  if (n != am_sm->getRow()) {
//    cout << "arrow mode changed from " << am_sm->getRow() << " to " << n << endl;
    am_sm->select(0, n);
  } 

  n = TToolBox::preferences->arrowtype;
  if (n != at_sm->getRow()) {
//    cout << "arrow style changed from " << at_sm->getRow() << " to " << n << endl;
    at_sm->select(0, n);
  } 
}

void
setLineStyle(TSingleSelectionModel *model)
{
  TPenBase::ELineStyle n = static_cast<TPenBase::ELineStyle>(model->getRow()+1);
  if (TToolBox::preferences->linestyle != n) {
    TToolBox::preferences->reason = TFigureAttributes::LINESTYLE;
    TToolBox::preferences->linestyle = n;
    TToolBox::preferences->sigChanged();
  }
}

void
setArrowMode(TSingleSelectionModel *model)
{
  TFLine::EArrowMode n = static_cast<TFLine::EArrowMode>(model->getRow());
  if (TToolBox::preferences->arrowmode != n) {
    TToolBox::preferences->reason = TFigureAttributes::ARROWMODE;
    TToolBox::preferences->arrowmode = n;
    TToolBox::preferences->sigChanged();
  }
}

void
setArrowType(TSingleSelectionModel *model)
{
  TFLine::EArrowType n = static_cast<TFLine::EArrowType>(model->getRow());
  if (TToolBox::preferences->arrowtype != n) {
    TToolBox::preferences->reason = TFigureAttributes::ARROWSTYLE;
    TToolBox::preferences->arrowtype = n;;
    TToolBox::preferences->sigChanged();
  }
}

static void
selectFont()
{
  TFontDialog dlg(0, "Select Font");
  dlg.setFont(TToolBox::preferences->getFont());
  dlg.setFontSize(dlg.getFontSize()/96);
  dlg.doModalLoop();
  if (dlg.getResult() == TMessageBox::OK) {
    dlg.setFontSize(dlg.getFontSize()*96);
    TToolBox::preferences->setFont(dlg.getFont());
  }
}

static void
openPalette(TWindow *parent)
{
  TColorPalette *cp = new TColorPalette(parent, "Palette", TToolBox::preferences);
  cp->createWindow();
}





class TColorPickTool:
  public TFigureTool
{
  public:
    static TColorPickTool* getTool();
    void mouseEvent(TFigureEditor *fe, const TMouseEvent &me);
};

TColorPickTool*
TColorPickTool::getTool()
{
  static TColorPickTool* tool = 0;
  if (!tool)
    tool = new TColorPickTool();
  return tool;
}

void 
TColorPickTool::mouseEvent(TFigureEditor *fe, const TMouseEvent &me)
{
  if (me.type!=TMouseEvent::LDOWN)
    return;
    
  TCoord x, y;
  fe->mouse2sheet(me.x, me.y, &x, &y);
  TFigure *f = fe->findFigureAt(x, y);
  cout << "found figure " << f << endl;
  if (f) {
    TToolBox::preferences->reason = TFigureAttributes::ALLCHANGED;
    f->getAttributes(TToolBox::preferences);
    TToolBox::preferences->sigChanged();
  }
}


TToolBox::TToolBox(TWindow *p, const string &t):
  super(p, t)
{
TObjectStore& serialize(toad::getDefaultStore());
serialize.registerObject(new TFPath());

  bParentlessAssistant = true;

  static TFCreateTool frect(new TFRectangle);
  static TFCreateTool fcirc(new TFCircle);
  static TFCreateTool ftext(new TFText);
  static TFCreateTool fpoly(new TFPolygon);
  static TFCreateTool fline(new TFLine);
  static TFCreateTool fbezierline(new TFBezierline);
  static TFCreateTool fbezier(new TFBezier);

  assert(toolbox==0);
  toolbox = this;
  preferences = new TFigureAttributes;
  preferences->linewidth = 48;

  bNoMenu = true;
  setLayout(0);
  setSize(2+2+28+28+2+2, 480+50+28);
  
  bmp.load(RESOURCE("fischland-small.png"));
  
  TRadioStateModel *state = new TRadioStateModel();
  TWindow *wnd;
  TButtonBase *rb;
  bool odd = true;
  
  int x, y;
  x = 4;
  y = 4+2+28+2+1;

  TFigureAttributes *me = preferences;
  
  for(unsigned i=0; i<20; i++) {
    wnd = 0;
    switch(i) {
      case 0:
        wnd = rb = new TFatRadioButton(this, "pencil", state);
        wnd->setToolTip("Selection");
        rb->loadBitmap(RESOURCE("tool_select.png"));
        CONNECT(rb->sigClicked, me, setTool, TSelectionTool::getTool());
        me->setTool(TSelectionTool::getTool());
        rb->setDown();
        break;
      case 1:
        wnd = rb = new TFatRadioButton(this, "select", state);
        wnd->setToolTip("Direct Selection");
        rb->loadBitmap(RESOURCE("tool_directselect.png"));
        CONNECT(rb->sigClicked, me, setTool, TDirectSelectionTool::getTool());
        break;
      case 2:
        wnd = rb = new TFatRadioButton(this, "pen", state);
        wnd->setToolTip("Pen: draw beziers");
        rb->loadBitmap(RESOURCE("tool_pen.png"));
        CONNECT(rb->sigClicked, me, setTool, TPenTool::getTool());
        break;
      case 3:
        wnd = rb = new TFatRadioButton(this, "pencil", state);
        wnd->setToolTip("Pencil: freehand curves");
        rb->loadBitmap(RESOURCE("tool_pencil.png"));
        CONNECT(rb->sigClicked, me, setTool, TPencilTool::getTool());
        break;
/*
      case 2:
*/
      case 5:
        wnd = rb = new TFatRadioButton(this, "floodfill", state);
        wnd->setToolTip("Flood Fill");
        rb->loadBitmap(RESOURCE("tool_floodfill.png"));
        CONNECT(rb->sigClicked, me, setTool, TFillTool::getTool());
        break;
      case 6:
        wnd = rb = new TFatRadioButton(this, "text", state);
        wnd->setToolTip("create text");
        rb->loadBitmap(RESOURCE("tool_text.png"));
        CONNECT(rb->sigClicked, me, setTool, &ftext);
        break;
      case 7:
        wnd = rb = new TFatRadioButton(this, "circle", state);
        wnd->setToolTip("create ellipse");
        rb->loadBitmap(RESOURCE("tool_circ.png"));
        CONNECT(rb->sigClicked, me, setTool, &fcirc);
        break;
      case 8:
        wnd = rb = new TFatRadioButton(this, "rectangle", state);
        wnd->setToolTip("create rectangle");
        rb->loadBitmap(RESOURCE("tool_rect.png"));
        CONNECT(rb->sigClicked, me, setTool, &frect);
        break;
        
      case 9:
        y+=5;
        wnd = rb = new TFatRadioButton(this, "rotate", state);
        CONNECT(rb->sigClicked, me, setOperation, TFigureEditor::OP_ROTATE);
        wnd->setToolTip("rotate");
        rb->loadBitmap(RESOURCE("tool_rotate.png"));
        break;
      case 10:
        wnd = rb = new TFatRadioButton(this, "free transform", state);
        wnd->setToolTip("free transform");
        break;
        
      case 13:
        y+=5;
        wnd = rb = new TPushButton(this, "group");
        wnd->setToolTip("group selection");
        rb->loadBitmap(RESOURCE("tool_group.png"));
        CONNECT(rb->sigClicked, me, group);
        break;
      case 14:
        wnd = rb = new TPushButton(this, "ungroup");
        wnd->setToolTip("ungroup selection");
        rb->loadBitmap(RESOURCE("tool_ungroup.png"));
        CONNECT(rb->sigClicked, me, ungroup);
        break;

      case 15:
        y+=5;
        wnd = rb = new TPushButton(this, "down");
        wnd->setToolTip("move selection down");
        rb->loadBitmap(RESOURCE("tool_down.png"));
        CONNECT(rb->sigClicked, me, selectionDown);
        break;
      case 16:
        wnd = rb = new TPushButton(this, "bottom");
        wnd->setToolTip("move selection to bottom");
        rb->loadBitmap(RESOURCE("tool_bottom.png"));
        CONNECT(rb->sigClicked, me, selection2Bottom);
        break;

      case 17:
        wnd = rb = new TPushButton(this, "up");
        wnd->setToolTip("move selection up");
        rb->loadBitmap(RESOURCE("tool_up.png"));
        CONNECT(rb->sigClicked, me, selectionUp);
        break;
      case 18:
        wnd = rb = new TPushButton(this, "top");
        wnd->setToolTip("move selection to top");
        rb->loadBitmap(RESOURCE("tool_top.png"));
        CONNECT(rb->sigClicked, me, selection2Top);
        break;
      case 19:
        wnd = new TColorSelector(this, "colorselector", preferences);
        y+=5;
        {
          // some hack, as TColorSelector doesn't create a palette and
          // the whole palette code is still inside this source file
          TInteractor *in = wnd->getFirstChild();
          do {
            if (in->getTitle()=="palette") {
              TPushButton *btn = dynamic_cast<TPushButton*>(in);
              if (btn)
                connect(btn->sigClicked, &openPalette, (TWindow*)0);
              break;
            }
            in = in->getNextSibling();
          } while(in);
        }
        break;
    }
    if (wnd) {
      if (i!=19)
        wnd->setSize(28,28);
      else
        wnd->setSize(28+28,28+28-16);
      if (odd) {
        x = 4;
      } else {
        x = 4 + 28;
      }
      wnd->setPosition(x, y);
      if (!odd)
        y+=28;
      odd = !odd;
    }
  }
  
  TComboBox *cb;
  y+=28+28-16+5;
  
  TTextField *tf = new TTextField(this, "alpha", &me->alpha);
  tf->setToolTip("alpha");
  tf->setPosition(4,y);
  tf->setSize(28+28-16, 20);
  
  TGauge *gg = new TGauge(this, "alphag", &me->alpha);
  gg->setToolTip("alpha");
  gg->setPosition(4+28+28-16,y);
  gg->setSize(16,20);
  
//  connect(cb->sigSelection, setLineWidth, cb);
  y+=20+5;

  cb = new TComboBox(this, "linewidth");
  cb->setToolTip("line width");
  cb->setAdapter(new TLineWidthAdapter);
  cb->setPosition(4,y);
  cb->setSize(28+28, 20);
  lw_sm = new TSingleSelectionModel();
  cb->setSelectionModel(lw_sm);
  connect(lw_sm->sigChanged, setLineWidth, lw_sm);
  y+=20;

  cb = new TComboBox(this, "linestyle");
  cb->setToolTip("line style");
  cb->setAdapter(new TLineStyleAdapter);
  cb->setPosition(4,y);
  cb->setSize(28+28, 20);
  ls_sm = new TSingleSelectionModel();
  cb->setSelectionModel(ls_sm);
  connect(cb->sigSelection, setLineStyle, ls_sm);
  y+=20+5;

  cb = new TComboBox(this, "arrowmode");
  cb->setToolTip("arrow mode");
  cb->setAdapter(new TArrowModeAdapter);
  cb->setPosition(4,y);
  cb->setSize(28+28, 20);
  am_sm = new TSingleSelectionModel();
  cb->setSelectionModel(am_sm);
  connect(cb->sigSelection, setArrowMode, am_sm);
  y+=20;

  cb = new TComboBox(this, "arrowtype");
  cb->setToolTip("arrow type");
  cb->setAdapter(new TArrowTypeAdapter);
  cb->setPosition(4,y);
  cb->setSize(28+28, 20);
  at_sm = new TSingleSelectionModel();
  cb->setSelectionModel(at_sm);
  connect(cb->sigSelection, setArrowType, at_sm);
  y+=20+5;

  rb = new TFontButton(this, "font", me);
  rb->setPosition(4,y);
  rb->setSize(28+28,28+28);
  rb->setToolTip("select font");
  connect(rb->sigClicked, selectFont);
  y+=28+28+5;

  tf = new TTextField(this, "gridsize");
  tf->setToolTip("grid size");
  tf->setPosition(4,y);
  tf->setSize(28+8, 20);
  tf->setModel(&preferences->gridsize);

  TFatCheckButton *pb = new TFatCheckButton(this, "enablegrid");
  pb->setToolTip("enabled/disable grid");
  pb->loadBitmap(RESOURCE("enablegrid.png"));
  pb->setPosition(4+28+8,y);
  pb->setSize(20, 20);
  pb->setModel(&preferences->drawgrid);
  y+=20+5;

  rb = new TPushButton(this, "applyall");
  rb->setPosition(4,y);
  rb->setSize(28,28);
  rb->setToolTip("apply all attributes to selection");
  rb->loadBitmap(RESOURCE("tool_fill.png"));
  CONNECT(rb->sigClicked, me, applyAll);
//  y+=20+5;

  rb = new TFatRadioButton(this, "fetchall", state);
//  rb = new TPushButton(this, "fetchall");
  rb->setPosition(4+28,y);
  rb->setSize(28,28);
  rb->setToolTip("pick attributes from figure");
  rb->loadBitmap(RESOURCE("tool_pick.png"));
  TCLOSURE1(
    rb->sigClicked,
    attr, me,
    attr->setTool(TColorPickTool::getTool());
  )
  
  CONNECT(preferences->sigChanged, this, preferencesChanged);
}

void
TToolBox::paint()
{
  TPen pen(this);

  // image
  pen.drawBitmap(4,4,bmp);
  pen.draw3DRectanglePC(3,3,1+28+28,1+28, false);

  int y, h, w;
  w = 4+28+28;

  // image frame
  y = 2; h = 4+28;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // draw tools
  y += h+1; h = 4+4*28;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // rotate & ?
  y += h+1; h = 4+28;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // group & ungroup
  y += h+1; h = 4+28;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // depth
  y += h+1; h = 4+2*28;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // color palette
  y += h+1; h = 4+28+28-16;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // alpha
  y += h+1; h = 4+20;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // line style
  y += h+1; h = 4+2*20;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // arrow style
  y += h+1; h = 4+2*20;
  pen.draw3DRectanglePC(2,y,w,h, true);

  // font button
  y += 25+25+5+5;

  // grid
  y += h+1; h = 4+20;
  pen.draw3DRectanglePC(2,y,w,h, true);
}
