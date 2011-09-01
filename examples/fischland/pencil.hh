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

#ifndef _FISCHLAND_PENCIL_HH
#define _FISCHLAND_PENCIL_HH 1

#include <toad/toad.hh>
#include <toad/color.hh>

namespace toad {

class TPencil
{
  public:
    virtual ~TPencil();
  
    virtual void LoadIdentity() = 0;
    virtual void Translate(double dx, double dy) = 0;
    virtual void Scale(double x, double y) = 0;
    virtual void Rotate(double d) = 0;
    
    virtual void SetColor(int,int,int) = 0;
    virtual void SetLineColor(TRGB&) = 0;
    virtual void SetFillColor(TRGB&) = 0;
    
    virtual void DrawLine(double x1, double y1, double x2, double y2) = 0;
    virtual void DrawRectangle(double x, double y, double w, double h) = 0;
    virtual void FillRectangle(double x, double y, double w, double h) = 0;
    virtual void DrawCircle(double x, double y, double w, double h) = 0;
    virtual void FillCircle(double x, double y, double w, double h) = 0;
    
    virtual double Height() = 0;
    virtual double TextWidth(const string&) = 0;
    virtual void DrawString(double x, double y, const string &text) = 0;
};

class TScreenPencil:
  public TPencil
{
  public:
    TScreenPencil(TWindow*);
    TScreenPencil(TBitmap*);
    
    void LoadIdentity();
    void Translate(double x, double y);
    void Scale(double x, double y);
    void Rotate(double d);
    
    void SetColor(int,int,int);
    void SetLineColor(TRGB&);
    void SetFillColor(TRGB&);
    
    void DrawLine(double x1, double y1, double x2, double y2);
    void DrawRectangle(double x, double y, double w, double h);
    void FillRectangle(double x, double y, double w, double h);
    void DrawCircle(double x, double y, double w, double h);
    void FillCircle(double x, double y, double w, double h);
    
    double Height();
    double TextWidth(const string&);
    void DrawString(double x, double y, const string &text);
    
    double m11, m12, m13;
    double m21, m22, m23;

  protected:
    TPen pen;
};

} // namespace toad
