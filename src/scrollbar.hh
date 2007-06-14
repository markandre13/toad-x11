/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_SCROLLBAR_HH
#define _TOAD_SCROLLBAR_HH 1

#include <toad/control.hh>
#include <toad/integermodel.hh>
#include <toad/floatmodel.hh>

namespace toad {

// class TSlider;
class TArrowButton;

class TScrollBar: 
  public TControl
{
    typedef TControl super;
    typedef TScrollBar thisClass;

  protected:
    bool bVertical;
    int unitIncrement;

  public:
    TScrollBar(TWindow*, const string&, TIntegerModel *model=0);
    TScrollBar(TWindow*, const string&, TFloatModel *model);
    
    PIntegerModel model;
    
    static int getFixedSize();

    void setModel(TIntegerModel*);
    TIntegerModel* getModel() const { return model; }
    
    void setValue(int v) { model->setValue(v); }
    void setMinimum(int m) { model->setMinimum(m); }
    void setMaximum(int m) { model->setMaximum(m); }
    void setExtent(int e) { model->setExtent(e); }
    void setRangeProperties(int v, int e, int mi, int ma, bool a=false) {
      assert(model!=NULL);
      model->setRangeProperties(v, e, mi, ma, a);
    }
    
    int getValue() const { return model->getValue(); }
    int getMinimum() const { return model->getMinimum(); }
    int getMaximum() const { return model->getMaximum(); }
    int getExtent() const { return model->getExtent(); }
    void setUnitIncrement(int i) { unitIncrement=i; }
    int getUnitIncrement() const { return unitIncrement; }

    void increment();
    void decrement();
    void pageUp();
    void pageDown();

    void mouseEvent(const TMouseEvent&);

  protected:
    void resize();
    void paint();
    void focus(bool);
    void valueChanged();

    void mouseLDown(const TMouseEvent&);
    void mouseLUp(const TMouseEvent&);
    void mouseMove(const TMouseEvent&);
    void keyDown(const TKeyEvent&);

    void _drawSlider(TPen &pen, TRectangle &r);
    void _drawArea(TPen&);
    void _placeChildren();
    void _placeSlider();
    void _moveSliderTo(TCoord y);

    TArrowButton *btn1;
    TArrowButton *btn2;
    TRectangle rectSlider;
    int nMouseDown;
    
    void modelChanged();
};

} // namespace toad

#endif
