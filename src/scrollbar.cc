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

#define NO_FLICKER

// #include <toad/core.hh>
#include <toad/scrollbar.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/arrowbutton.hh>

#include <unistd.h>
#include <limits.h>

using namespace toad;

/**
 * \class TScrollBar
 *
 * \todo
 *   \li
 *     implement block and unit increments/decrements
 */

#define DEFAULT_FIXED_SIZE 15
#define DEFAULT_FIXED_BORDER 1

class TFloat2IntegerModel:
  public TIntegerModel
{
    TFloatModel *model;
    bool lock;
  public:
    TFloat2IntegerModel(TFloatModel*);
    ~TFloat2IntegerModel();
  private:
    void masterChanged();
    void slaveChanged();
};

TFloat2IntegerModel::TFloat2IntegerModel(TFloatModel *m)
{
  model = m;
  lock = false;
  
  if (model) {
    setRangeProperties(model->getValue(),
                       model->getExtent(),
                       model->getMinimum(),
                       model->getMaximum(),
                       false);
    connect(model->sigChanged, this, &TFloat2IntegerModel::slaveChanged);
    slaveChanged();
  }
  connect(this->sigChanged, this, &TFloat2IntegerModel::masterChanged);
}

TFloat2IntegerModel::~TFloat2IntegerModel()
{
  if (model)
    disconnect(model->sigChanged, this);
}

void
TFloat2IntegerModel::masterChanged()
{
  if (lock) return;
  lock = true;
  model->setValue(getValue());
  lock = false;
}

void
TFloat2IntegerModel::slaveChanged()
{
  if (lock) return;
  lock=true;
  setValue(model->getValue());
  lock=false;
}


TScrollBar::TScrollBar(TWindow *parent, const string &title, TFloatModel *model):
  TControl(parent, title)
{
  _b = DEFAULT_FIXED_BORDER;
//  flagNoBackground = true;
  bNoBackground = true;
  this->model = new TFloat2IntegerModel(model);
  CONNECT(model->sigChanged, this, modelChanged);
  
  bVertical   = true;
  nMouseDown  = -1;
  unitIncrement = 1;
  w=h=DEFAULT_FIXED_SIZE;

//  setMouseMoveMessages(TMMM_LBUTTON);
  btn1 = new TArrowButton(this, "up/left", bVertical ? 
                                TArrowButton::ARROW_UP : 
                                TArrowButton::ARROW_LEFT);
  CONNECT(btn1->sigClicked, this, decrement);
  btn2 = new TArrowButton(this, "down/right", bVertical ? 
                                TArrowButton::ARROW_DOWN : 
                                TArrowButton::ARROW_RIGHT);
  CONNECT(btn2->sigClicked, this, increment);
}

// Constructor
//---------------------------------------------------------------------------
TScrollBar::TScrollBar(TWindow *parent, const string &title, TIntegerModel *model)
  :TControl(parent,title)
{
  _b = DEFAULT_FIXED_BORDER;
//  flagNoBackground = true;
  bNoBackground = true;
  if (!model)
    model = new TIntegerModel();
  this->model = model;
  CONNECT(model->sigChanged, this, modelChanged);
  
  bVertical   = true;
  nMouseDown  = -1;
  unitIncrement = 1;
  w=h=DEFAULT_FIXED_SIZE;

//  setMouseMoveMessages(TMMM_LBUTTON);
  btn1 = new TArrowButton(this, "up/left", bVertical ? 
                                TArrowButton::ARROW_UP : 
                                TArrowButton::ARROW_LEFT);
  CONNECT(btn1->sigClicked, this, decrement);
  btn2 = new TArrowButton(this, "down/right", bVertical ? 
                                TArrowButton::ARROW_DOWN : 
                                TArrowButton::ARROW_RIGHT);
  CONNECT(btn2->sigClicked, this, increment);
}

void
TScrollBar::setModel(TIntegerModel *m)
{
  disconnect(model->sigChanged, this);
  CONNECT(model->sigChanged, this, modelChanged);
}

int
TScrollBar::getFixedSize()
{
  return DEFAULT_FIXED_SIZE+DEFAULT_FIXED_BORDER*2;
}

void
TScrollBar::resize()
{
  bVertical = getWidth() < getHeight();
  _placeChildren();
  _placeSlider();
}

void
TScrollBar::_drawSlider(TPen &pen, TRectangle &r)
{
  TPoint p[6];

  // draw slider face
  //------------------
  pen.setColor(TColor::SLIDER_FACE);
  pen.fillRectanglePC(r.x+3, r.y+3, r.w-6, r.h-6);

  // draw border
  //-------------
  pen.setColor(0,0,0);
  pen.drawRectanglePC(r.x, r.y, r.w, r.h);
  
  // draw shadow(s)
  //----------------
  pen.setColor(TColor::SLIDER_SHADOW);
  TCoord c;
  if (bVertical)
  {
    c = r.y+(r.h/2)-2;
    pen.fillRectanglePC(r.x+5, c+1, r.w-8, 5);
  } else {
    c = r.x+(r.w/2)-2;
    pen.fillRectanglePC(c+1, r.y+5, 5, r.h-8);
  }

  p[0].set(r.x+1    , r.y+r.h-2);
  p[1].set(r.x+r.w-2, r.y+r.h-2);
  p[2].set(r.x+r.w-2, r.y+1    );
  p[3].set(r.x+r.w-3, r.y+2    );
  p[4].set(r.x+r.w-3, r.y+r.h-3);
  p[5].set(r.x+2    , r.y+r.h-3);
  pen.drawLines(p,6);
  
  // draw light
  //------------
  pen.setColor(TColor::SLIDER_LIGHT);
  if (bVertical) {
    pen.drawLine(r.x+4,c  , r.x+5+r.w-10,c  );
    pen.drawLine(r.x+4,c+2, r.x+5+r.w-10,c+2);
    pen.drawLine(r.x+4,c+4, r.x+5+r.w-10,c+4);
    pen.drawLine(r.x+4,c  , r.x+4       ,c+5);
  } else {
    pen.drawLine(c  ,r.y+4, c  ,r.y+5+r.h-10);
    pen.drawLine(c+2,r.y+4, c+2,r.y+5+r.h-10);
    pen.drawLine(c+4,r.y+4, c+4,r.y+5+r.h-10);
    pen.drawLine(c  ,r.y+4, c+5,r.y+4);
  }
  
  p[0].set(r.x+1    , r.y+r.h-2);
  p[1].set(r.x+1    , r.y+1    );
  p[2].set(r.x+r.w-2, r.y+1    );
  p[3].set(r.x+r.w-3, r.y+2    );
  p[4].set(r.x+2    , r.y+2    );
  p[5].set(r.x+2    , r.y+r.h-3);
  pen.drawLines(p,6);
  
  // draw sliders shadow
  //---------------------
  pen.setColor(TColor::BTNSHADOW);
  if (bVertical) {
    pen.drawLine(r.x, r.y+r.h,     r.x+r.w-1, r.y+r.h);
    pen.drawLine(r.x+2, r.y+r.h+1, r.x+r.w-1, r.y+r.h+1);
  } else {
    pen.drawLine(r.x+r.w, r.y,     r.x+r.w, r.y+r.h-1);
    pen.drawLine(r.x+r.w+1, r.y+2, r.x+r.w+1, r.y+r.h-1);
  }
}

/** 
 * this method draws the area where the slider is moving; to reduce
 * flicker and double buffering, it excludes the region of the slider
 */
void
TScrollBar::_drawArea(TPen &pen)
{
  TPoint p[3];
  TCoord v, n, m;

  if (isFocus()) {
    pen.setColor(0,0,0);
    pen.drawRectanglePC(0,0, w, h);
    v=1;
  } else {
    v=0;
  }
  
  if (bVertical) {
    // background
    //------------
    pen.setColor(TColor::BTNFACE);
//pen.setColor(255,0,0);  // red
    n = rectSlider.y-w-2;
    if (n>0)
      pen.fillRectanglePC(v+1,w+1, w-2-(v*2), n+1);
//pen.setColor(255,128,0); // orange
    n += rectSlider.h + w + 2;
    m = h-w-n;
    if (m>0)
      pen.fillRectanglePC(v+1, n, w-2-(v*2), m);
  
    // shadow
    //------------
    pen.setColor(TColor::BTNSHADOW);
//pen.setColor(0,128,0); // dark green
    if (w+1<=rectSlider.y-1) {
      p[0].set(w-2-v, w);
      p[1].set(     v, w);
      p[2].set(     v, rectSlider.y-1);
      pen.drawLines(p,3);
    }
//pen.setColor(0,255,0); // bright green
    pen.drawLine(v, rectSlider.y + rectSlider.h + 1,
                 v, h-w-1 );

    // light
    //------------
    pen.setColor(TColor::BTNLIGHT);
//pen.setColor(0,0,255); // blue
    pen.drawLine(w-1-v,0, w-1-v,rectSlider.y-1);
    p[0].set(w-1-v,rectSlider.y+rectSlider.h+1);   // slider bottom, right
    p[1].set(w-1-v,h-w-1);                       // bottom,right
    p[2].set(1+v,h-w-1);                          // bottom,left
//pen.setColor(0,128,255); // light blue
    pen.drawLines(p,3);
  } else {
    // background
    //------------
    pen.setColor(TColor::BTNFACE);

    // left side of the slider
    n = rectSlider.x-h;
    if (n>0)
      pen.fillRectanglePC(h+1, v+1, n, h-(v*2)-2);

    // right side of the slider
    n = rectSlider.x+rectSlider.w + 1;
    m = w - n - h;
    pen.fillRectanglePC(n, v+1, m, h-(v*2)-2);
  
    // shadow
    //------------
    pen.setColor(TColor::BTNSHADOW);
    p[0].set(h,h-1-v);
    p[1].set(h,v);
    p[2].set(rectSlider.x,v);
    pen.drawLines(p,3);
    pen.drawLine(rectSlider.x+rectSlider.w+1, v,
                 w-h-1, v);

    // light
    //------------
    pen.setColor(TColor::BTNLIGHT);
    pen.drawLine(h+1, h-1-v,
                 rectSlider.x-1, h-1-v);
    p[0].set(rectSlider.x+rectSlider.w+2,h-1-v);
    p[1].set(w-h-1, h-1-v);
    p[2].set(w-h-1,v+1);
    pen.drawLines(p,3);
  }
}

void
TScrollBar::focus(bool)
{
//  _placeChildren();
  _placeSlider();
  invalidateWindow();
}

void
TScrollBar::decrement()
{
  model->setValue(model->getValue()-unitIncrement);
}

void
TScrollBar::increment()
{
  model->setValue(model->getValue()+unitIncrement);
}

void
TScrollBar::pageUp()
{
  model->setValue(model->getValue()-model->getExtent());
}

void
TScrollBar::pageDown()
{
  model->setValue(model->getValue()+model->getExtent());
}

void
TScrollBar::modelChanged()
{
  if (isRealized()) {
    _placeSlider();
    invalidateWindow();
  }
}

void
TScrollBar::keyDown(const TKeyEvent &ke)
{
//  setFocus();
  switch(ke.key()) {
    case TK_UP:
      if (bVertical)
        decrement();
      break;
    case TK_DOWN:
      if (bVertical)
        increment();
      break;
    case TK_LEFT:
      if (!bVertical)
        decrement();
      break;
    case TK_RIGHT:
      if (!bVertical)
        increment();
      break;
    case TK_PAGEUP:
      pageUp();
      break;
    case TK_PAGEDOWN:
      pageDown();
      break;
  }
}

void
TScrollBar::mouseEvent(const TMouseEvent &me)
{
  switch(me.type) {
    case TMouseEvent::ROLL_UP:
      decrement();
      break;
    case TMouseEvent::ROLL_DOWN:
      increment();
      break;
    default:
      TWindow::mouseEvent(me);
  }
}

void
TScrollBar::mouseLDown(const TMouseEvent &me)
{
  setFocus();
  int v = model->getValue();
  
  if (!rectSlider.isInside(me.x,me.y)) {
    // move by a page
    int e = model->getExtent();
    if (e<1) e=1; else e--;
    if (bVertical ? me.y<rectSlider.y : me.x<rectSlider.x) {
      model->setValue(v-e);
    } else {
      model->setValue(v+e);
    }
  } else {
/*
    // move slider
    TPen pen(this);
    _drawSlider(pen, rectSlider);
*/
    invalidateWindow();
    nMouseDown = bVertical ? me.y-rectSlider.y : me.x-rectSlider.x;
    // model->setValueIsAdjusting(true);
  }
}

void
TScrollBar::mouseMove(const TMouseEvent &me)
{
//printf("scrollbar mouseMove: x=%i,y=%i,nMouseDown=%i\n",x,y,nMouseDown);
  if (nMouseDown!=-1) {
    TRectangle rectOld;
    rectOld = rectSlider;
    if (bVertical)
      _moveSliderTo(me.y - nMouseDown);
    else
      _moveSliderTo(me.x - nMouseDown);
  }
}

void
TScrollBar::mouseLUp(const TMouseEvent&)
{
  if (nMouseDown!=-1) {
    nMouseDown = -1;
    // model->setValueIsAdjusting(false);
  }
}

void
TScrollBar::paint()
{
  TPen pen(this);
  _drawArea(pen);
  _drawSlider(pen, rectSlider);
//printf("paint: rectSlider: %i,%i,%i,%i\n", rectSlider.x,rectSlider.y,rectSlider.w,rectSlider.h);
}

void
TScrollBar::_placeSlider()
{
  assert(model!=NULL);
//printf("_placeSlider\n");
  if (model->getExtent()<0)
    return;
//printf("_placeSlider 2\n");
      
  // calculate range of possible slider positions 
  //----------------------------------------------
  int nRange = model->getMaximum() - model->getMinimum() - model->getExtent() + 1;

//printf("range = %i\n", model->getMaximum() - model->getMinimum());

//printf("nRange=%i, min=%i, max=%i, extend=%i\n", nRange, model->getMinimum(), model->getMaximum() , model->getExtent());
  if (nRange<0)
    nRange = 0;

  // calculate size of the area in which the slider is
  //---------------------------------------------------
  TRectangle rect1,rect2;
  btn1->getShape(&rect1);
  btn2->getShape(&rect2);
  TCoord nSize = bVertical ? h - rect1.h - rect1.y - h+ rect2.y
                        : w - rect1.w - rect1.x - w + rect2.x;
    
  // calculate slider size 
  //-----------------------
  assert(model->getExtent() + nRange != 0);
  
  TCoord nSlider = (model->getExtent() * nSize) / (model->getExtent() + nRange);

  // limit slider size
  if (bVertical) {
    if (nSlider<w)
      nSlider=w;
    else if (nSlider>nSize)
      nSlider=nSize;
  } else {
    if (nSlider<h)
      nSlider=h;
    else if (nSlider>nSize)
      nSlider=nSize;
  } 

  // calculate slider position
  //---------------------------
  TCoord nSetRange = nSize-nSlider+1;
  TCoord pos;
  if (nRange==0)
    pos = 0;
  else
    pos = (model->getValue()-model->getMinimum()) * nSetRange / nRange;

  pos += bVertical ? rect1.h+rect1.y : rect1.w+rect1.x;
  pos -= 1;

  // correct slider position 
  //-------------------------
  if (bVertical) {
    if ( pos+nSlider-1 > rect2.y ) 
      pos = rect2.y - nSlider + 1;
    rectSlider.set(isFocus()?1:0,pos, w+(isFocus()?-2:0), nSlider);
  } else {
    if ( pos+nSlider-1 > rect2.x ) 
      pos = rect2.x - nSlider + 1;
    rectSlider.set(pos, isFocus()?1:0, nSlider, h+(isFocus()?-2:0));
  }
}

void
TScrollBar::_moveSliderTo(TCoord pos)
{
//printf("moveSliderTo %i\n", pos);
  TRectangle rect1,rect2;
  btn1->getShape(&rect1);
  btn2->getShape(&rect2);

  // validate new position and place slider 
  //----------------------------------------
  if (bVertical) {
    if (pos < rect1.y+rect1.h-1 )
      pos = rect1.y+rect1.h-1;
    else if ( pos+rectSlider.h-1 > rect2.y )
      pos = rect2.y - rectSlider.h + 1;
    rectSlider.x = isFocus()?1:0;
    rectSlider.y = pos;
  } else {
    if (pos < rect1.x+rect1.w-1 )
      pos = rect1.x+rect1.w-1;
    else if ( pos+rectSlider.w-1 > rect2.x )
      pos = rect2.x - rectSlider.w + 1;
    rectSlider.x = pos;
    rectSlider.y = isFocus()?1:0;
  }

  // change _data 
  //---------------
  // slider size
  TCoord nSlider = bVertical ? rectSlider.h : rectSlider.w;
  // size of slider area
  TCoord nSize = bVertical ? h - rect1.h - rect1.y - h + rect2.y - 2
                        : w - rect1.w - rect1.x - w + rect2.x - 2 ;
  // size of area to place the slider
  TCoord nSetRange = nSize-nSlider;

  // avoid division by zero
  //------------------------
  if (nSetRange==0 || nSize==0) {
    model->setValue(model->getMinimum());
    return;
  }
  
  TCoord nValue2;
  TCoord nRange = model->getMaximum() - model->getMinimum() - model->getExtent() + 1;
  
  if (nRange<0)
    nRange=0;
  
  pos -= bVertical ? rect1.y+rect1.h-1 : rect1.x+rect1.w-1;
  nValue2 = ( pos * nRange ) / nSetRange + model->getMinimum();
//printf("new value = %i\n", nValue2);
  model->setValue(nValue2);
}

void
TScrollBar::_placeChildren()
{
  if (bVertical) {
    btn1->setType(TArrowButton::ARROW_UP);
    btn2->setType(TArrowButton::ARROW_DOWN);
    btn1->setShape(0,0,w,w);
    // btn1->setShape(0,_h-_w-_w,_w,_w);
    btn2->setShape(0,h-w,w,w);
  } else {  
    btn1->setType(TArrowButton::ARROW_LEFT);
    btn2->setType(TArrowButton::ARROW_RIGHT);
    btn1->setShape(0,0,h,h);
    // btn1->setShape(_w-_h-_h,0,_h,_h);
    btn2->setShape(w-h,0,h,h);
  }
}
