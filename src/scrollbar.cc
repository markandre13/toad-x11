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

#define NO_FLICKER

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/region.hh>
#include <toad/pushbutton.hh>
#include <toad/simpletimer.hh>
#include <toad/boundedrangemodel.hh>
#include <toad/scrollbar.hh>
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

// Constructor
//---------------------------------------------------------------------------
TScrollBar::TScrollBar(TWindow *parent, const string &title, TBoundedRangeModel *model)
  :TControl(parent,title)
{
  _b = DEFAULT_FIXED_BORDER;
  bNoBackground = true;
  if (!model)
    model = new TBoundedRangeModel();
  this->model = model;
  CONNECT(model->sigChanged, this, modelChanged);
  
  bVertical   = true;
  nMouseDown  = -1;
  unitIncrement = 1;
  _w=_h=DEFAULT_FIXED_SIZE;

  setMouseMoveMessages(TMMM_LBUTTON);
  btn1 = new TArrowButton(this, "up/left", bVertical ? 
                                TArrowButton::ARROW_UP : 
                                TArrowButton::ARROW_LEFT);
  CONNECT(btn1->sigActivate, this, decrement);
  btn2 = new TArrowButton(this, "down/right", bVertical ? 
                                TArrowButton::ARROW_DOWN : 
                                TArrowButton::ARROW_RIGHT);
  CONNECT(btn2->sigActivate, this, increment);
}

void
TScrollBar::setModel(TBoundedRangeModel *m)
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
  int c;
  if (bVertical)
  {
    c = r.y+(r.h>>1)-2;
    pen.fillRectanglePC(r.x+5, c+1, r.w-8, 5);
  } else {
    c = r.x+(r.w>>1)-2;
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
  int v, n, m;

  if (isFocus()) {
    pen.setColor(0,0,0);
    pen.drawRectanglePC(0,0, _w, _h);
    v=1;
  } else {
    v=0;
  }
  
  if (bVertical) {
    // background
    //------------
    pen.setColor(TColor::BTNFACE);
//pen.setColor(255,0,0);  // red
    n = rectSlider.y-_w-2;
    if (n>0)
      pen.fillRectanglePC(v+1,_w+1, _w-2-(v<<1), n+1);
//pen.setColor(255,128,0); // orange
    n += rectSlider.h + _w + 2;
    m = _h-_w-n;
    if (m>0)
      pen.fillRectanglePC(v+1, n, _w-2-(v<<1), m);
  
    // shadow
    //------------
    pen.setColor(TColor::BTNSHADOW);
//pen.setColor(0,128,0); // dark green
    if (_w+1<=rectSlider.y-1) {
      p[0].set(_w-2-v, _w);
      p[1].set(     v, _w);
      p[2].set(     v, rectSlider.y-1);
      pen.drawLines(p,3);
    }
//pen.setColor(0,255,0); // bright green
    pen.drawLine(v, rectSlider.y + rectSlider.h + 1,
                 v, _h-_w-1 );

    // light
    //------------
    pen.setColor(TColor::BTNLIGHT);
//pen.setColor(0,0,255); // blue
    pen.drawLine(_w-1-v,0, _w-1-v,rectSlider.y-1);
    p[0].set(_w-1-v,rectSlider.y+rectSlider.h+1);   // slider bottom, right
    p[1].set(_w-1-v,_h-_w-1);                       // bottom,right
    p[2].set(1+v,_h-_w-1);                          // bottom,left
//pen.setColor(0,128,255); // light blue
    pen.drawLines(p,3);
  } else {
    // background
    //------------
    pen.setColor(TColor::BTNFACE);

    // left side of the slider
    n = rectSlider.x-_h;
    if (n>0)
      pen.fillRectanglePC(_h+1, v+1, n, _h-(v<<1)-2);

    // right side of the slider
    n = rectSlider.x+rectSlider.w + 1;
    m = _w - n - _h;
    pen.fillRectanglePC(n, v+1, m, _h-(v<<1)-2);
  
    // shadow
    //------------
    pen.setColor(TColor::BTNSHADOW);
    p[0].set(_h,_h-1-v);
    p[1].set(_h,v);
    p[2].set(rectSlider.x,v);
    pen.drawLines(p,3);
    pen.drawLine(rectSlider.x+rectSlider.w+1, v,
                 _w-_h-1, v);

    // light
    //------------
    pen.setColor(TColor::BTNLIGHT);
    pen.drawLine(_h+1, _h-1-v,
                 rectSlider.x-1, _h-1-v);
    p[0].set(rectSlider.x+rectSlider.w+2,_h-1-v);
    p[1].set(_w-_h-1, _h-1-v);
    p[2].set(_w-_h-1,v+1);
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
  setFocus();
  model->setValue(model->getValue()-unitIncrement);
}

void
TScrollBar::increment()
{
  setFocus();
  model->setValue(model->getValue()+unitIncrement);
}

void
TScrollBar::modelChanged()
{
  _placeSlider();
  invalidateWindow();
  valueChanged();
}

void
TScrollBar::valueChanged()
{
  if (isRealized()) {
    invalidateWindow();
    _placeSlider();
  }
}

void
TScrollBar::keyDown(TKey key,char*,unsigned)
{
  switch(key) {
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
      break;
    case TK_PAGEDOWN:
      break;
  }
}

void
TScrollBar::mouseLDown(int x,int y,unsigned)
{
  setFocus();
  int v = model->getValue();
  
  if (!rectSlider.isInside(x,y)) {
    // move by a page
    int e = model->getExtent();
    if (e<1) e=1; else e--;
    if (bVertical ? y<rectSlider.y : x<rectSlider.x) {
      model->setValue(v-e);
    } else {
      model->setValue(v+e);
    }
  } else {
    // move slider
    TPen pen(this);
    _drawSlider(pen, rectSlider);
    nMouseDown = bVertical ? y-rectSlider.y : x-rectSlider.x;
    // model->setValueIsAdjusting(true);
  }
}

void
TScrollBar::mouseMove(int x,int y,unsigned)
{
  if (nMouseDown!=-1) {
    TRectangle rectOld;
    rectOld = rectSlider;
    if (bVertical)
      _moveSliderTo(y - nMouseDown);
    else
      _moveSliderTo(x - nMouseDown);
  }
}

void
TScrollBar::mouseLUp(int,int,unsigned)
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
}

void
TScrollBar::_placeSlider()
{
  assert(model!=NULL);

  if (model->getExtent()<0)
    return;
      
  // calculate range of possible slider positions 
  //----------------------------------------------
  int nRange = model->getMaximum() - model->getMinimum() - model->getExtent();

  if (nRange<0)
    nRange = 0;

  // calculate size of the area in which the slider is
  //---------------------------------------------------
  TRectangle rect1,rect2;
  btn1->getShape(&rect1);
  btn2->getShape(&rect2);
  int nSize = bVertical ? _h - rect1.h - rect1.y - _h+ rect2.y
                        : _w  - rect1.w - rect1.x - _w + rect2.x;
    
  // calculate slider size 
  //-----------------------
  assert(model->getExtent() + nRange != 0);
  
  int nSlider = (model->getExtent() * nSize) / (model->getExtent() + nRange);

  // limit slider size
  if (bVertical) {
    if (nSlider<_w)
      nSlider=_w;
    else if (nSlider>nSize)
      nSlider=nSize;
  } else {
    if (nSlider<_h)
      nSlider=_h;
    else if (nSlider>nSize)
      nSlider=nSize;
  } 

  // calculate slider position
  //---------------------------
  int nSetRange = nSize-nSlider+1;
  int pos;
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
    rectSlider.set(isFocus()?1:0,pos, _w+(isFocus()?-2:0), nSlider);
  } else {
    if ( pos+nSlider-1 > rect2.x ) 
      pos = rect2.x - nSlider + 1;
    rectSlider.set(pos, isFocus()?1:0, nSlider, _h+(isFocus()?-2:0));
  }
}

void
TScrollBar::_moveSliderTo(int pos)
{
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
  int nSlider = bVertical ? rectSlider.h : rectSlider.w;
  // size of slider area
  int nSize = bVertical ? _h - rect1.h - rect1.y - _h+ rect2.y - 2
                        : _w  - rect1.w - rect1.x - _w + rect2.x - 2 ;
  // size of area to place the slider
  int nSetRange = nSize-nSlider;

  // avoid division by zero
  //------------------------
  if (nSetRange==0 || nSize==0) {
    model->setValue(model->getMinimum());
    return;
  }
  
  int nValue2;
  int nRange = model->getMaximum() - model->getMinimum() - model->getExtent() + 1;
  
  if (nRange<0)
    nRange=0;
  
  pos -= bVertical ? rect1.y+rect1.h-1 : rect1.x+rect1.w-1;
  nValue2 = ( pos * nRange ) / nSetRange + model->getMinimum();

  model->setValue(nValue2);
}

void
TScrollBar::_placeChildren()
{
  if (bVertical) {
    btn1->setType(TArrowButton::ARROW_UP);
    btn2->setType(TArrowButton::ARROW_DOWN);
    btn1->setShape(0,0,_w,_w);
    // btn1->setShape(0,_h-_w-_w,_w,_w);
    btn2->setShape(0,_h-_w,_w,_w);
  } else {  
    btn1->setType(TArrowButton::ARROW_LEFT);
    btn2->setType(TArrowButton::ARROW_RIGHT);
    btn1->setShape(0,0,_h,_h);
    // btn1->setShape(_w-_h-_h,0,_h,_h);
    btn2->setShape(_w-_h,0,_h,_h);
  }
}
