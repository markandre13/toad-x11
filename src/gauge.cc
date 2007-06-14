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

#include <toad/gauge.hh>

using namespace toad;

TGauge::TGauge(TWindow *parent, const string &title, TIntegerModel *model):
  TControl(parent, title)
{
  setBorder(0);
  setBackground(0,0,0);
  setCursor(TCursor::VERTICAL);

  up   = new TArrowButton(this, "up",   TArrowButton::ARROW_DOWN);
  up->setCursor(TCursor::DEFAULT);
  connect(up->sigClicked, this, &TGauge::increment);
  down = new TArrowButton(this, "down", TArrowButton::ARROW_UP);
  down->setCursor(TCursor::DEFAULT);
  connect(down->sigClicked, this, &TGauge::decrement);
  
  unitIncrement = 1;
  setModel(model);
}

void
TGauge::setModel(TIntegerModel *m)
{
  if (model)
    disconnect(model->sigChanged, this);
  model = m;
  if (model) {
    connect(model->sigChanged, this, &TGauge::modelChanged);
    modelChanged();
  }
}

void
TGauge::resize()
{
  TCoord h = getHeight()/2-1;
  up->setShape(0,0,getWidth(),h);
  down->setShape(0,getHeight()-h,getWidth(),h);
}

void
TGauge::decrement()
{
  if (model)
    model->setValue(model->getValue()-unitIncrement);
}

void
TGauge::increment()
{
  if (model)
    model->setValue(model->getValue()+unitIncrement);
}

void
TGauge::modelChanged()
{
  if (!model) {
    down->setEnabled(false);
    up->setEnabled(false);
    return;
  }
  int v = model->getValue();
  down->setEnabled(v > model->getMinimum());
  up->setEnabled(v+model->getExtent()-1 < model->getMaximum());
}

void
TGauge::mouseEvent(const TMouseEvent &me)
{
  if (!model)
    return;
    
  switch(me.type) {
    case TMouseEvent::LDOWN:
      down_y = me.y;
      down_val = model->getValue();
      break;
    case TMouseEvent::MOVE:
    case TMouseEvent::LUP:
      model->setValue(down_val + down_y - me.y);
      break;
  }
}

