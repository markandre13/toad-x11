/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef TButtonBase
#define TButtonBase TButtonBase

#include <toad/labelowner.hh>

namespace toad {

class TButtonBase: 
  public TLabelOwner
{
    typedef TLabelOwner super;
  public:
    TButtonBase(TWindow *p, const string &t);
    ~TButtonBase();
    
    virtual void setDown(bool down=true);
    bool isDown() const { return bDown; }
    
    bool loadBitmap(const string&);
    
    TSignal sigActivate;
    TSignal sigArm, sigDisarm;
    
    void mouseLDown(int,int,unsigned);
    void mouseLUp(int,int,unsigned);
    void mouseEnter(int,int,unsigned);
    void mouseLeave(int,int,unsigned);
    void keyDown(TKey,char*,unsigned);

  protected:
    TBitmap *bitmap;
    bool bDown,bInside;
    void drawShadow(TPen &pen, bool down, bool onwhite=false);
    void drawLabel(TPen&,const string&,bool,bool bEnabled=true);
};

} // namespace toad

#endif
