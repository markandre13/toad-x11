/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <toad/toad.hh>
#include <toad/pushbutton.hh>

using namespace toad;

class TMyWindow:
  public TWindow
{
  public:
    TMyWindow(TWindow *p, const string &t);
    void click(const char*);
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMyWindow wnd(NULL, "Tooltips");
    toad::mainLoop();
  } toad::terminate();
}

TMyWindow::TMyWindow(TWindow *p, const string &t):
  TWindow(p,t)
{
  bStaticFrame = true;
  setBackground(TColor::DIALOG);
  setToolTip("nothing to select here...");

  TPushButton *btn;
  
  btn = new TPushButton(this, "Agree");
    btn->setShape(10,10,100,25);
    CONNECT(btn->sigClicked, this, click, "I agree!!!");
    btn->setToolTip("You agree with the license");

  btn = new TPushButton(this, "Cancel");
    btn->setShape(10,40,100,25);
    CONNECT(btn->sigClicked, this, click, "I'll use it anyway ;)");
    btn->setToolTip("You dislike the license");
    
  setSize(120,75);
}

void
TMyWindow::click(const char* text)
{
  cout << "button \"" << text << "\"" << endl;
}
