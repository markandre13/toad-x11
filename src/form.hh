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

#ifndef TForm
#define TForm TForm

// for backward compability TOAD <= 0.0.32
#define SIDE_TOP        TFormBase::TOP
#define SIDE_BOTTOM       TFormBase::BOTTOM
#define SIDE_LEFT         TFormBase::LEFT
#define SIDE_RIGHT        TFormBase::RIGHT

#define ATTACH_NONE       TFormBase::NONE
#define ATTACH_FORM       TFormBase::FORM
#define ATTACH_WINDOW       TFormBase::WINDOW
#define ATTACH_OPPOSITE_WINDOW  TFormBase::OPPOSITE_WINDOW

#ifdef FORM
#error "FORM defined"
#endif

namespace toad {

class TFormBase
{
  public:
    // attachment side
    static const unsigned TOP=1, BOTTOM=2, LEFT=4, RIGHT=8, ALL=15;
    // attachment method
    static const unsigned NONE=0;
    static const unsigned FORM=1;
    static const unsigned WINDOW=2;
    static const unsigned OPPOSITE_WINDOW=3;

    TFormBase();
    ~TFormBase();
    void addForm(TWindow*);
    void removeForm(TWindow*);
    void attach(TWindow *wnd, unsigned where, unsigned how, TWindow *which=NULL);

    void attach(TWindow *wnd, unsigned where, TWindow *which=NULL);
    void distance(TWindow*, int distance, unsigned where=ALL);

    void attachLast(unsigned where, unsigned how, TWindow *which=NULL);
    int nBorderOverlap;
    bool bKeepOwnBorder;
    void arrange(int x,int y,int w,int h);

bool running;
    
  private:
    class TFormNode;
    TFormNode* _find(TWindow*);
    TFormNode *flist, *lastadd;
};

inline void 
TFormBase::attach(TWindow *wnd, unsigned where, TWindow *which)
{
#if 0
  attach(wnd, where, which ? WINDOW : FORM, which);
#else
  attach(wnd, where, which ? 2 : 1, which);
#endif
}


class TForm: 
  public TWindow, public TFormBase
{
    typedef TWindow super;
  public:
    TForm(TWindow *p, const string &t="TForm"):TWindow(p,t),TFormBase(){};

  private:
    void adjust(){arrange(0,0,getWidth(),getHeight());}
    void resize(){arrange(0,0,getWidth(),getHeight());}
    void childNotify(TWindow *c,EChildNotify t);
};

} // namespace toad

#endif
