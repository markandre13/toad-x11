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

using namespace toad;

/**
 * The applications main window.
 */
class TMyWindow:
  public TWindow
{
    string name;
  public:
    TMyWindow(TWindow *p, const string &t);
    void setName(const string &aName);
    void paint();
};

/**
 * Window with a special cursor which tells TMyWindow it's name,
 * when the mouse enters.
 */
class TCWindow:
  public TWindow
{
    TMyWindow * mywindow;
  public:
    TCWindow(TMyWindow *p, const string &t);
    void mouseEnter(int, int, unsigned);
    void mouseLeave(int, int, unsigned);
};



int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMyWindow wnd(NULL, "Cursor Demo");
    toad::mainLoop();
  } toad::terminate();
  return 0;
}



TMyWindow::TMyWindow(TWindow *p, const string &t):
  TWindow(p,t)
{
  TWindow *wnd;
  setBackground(TColor::LIGHTGRAY);
  
  wnd = new TCWindow(this,"DEFAULT");
    wnd->setCursor(TCursor::DEFAULT);
    wnd->setShape(5,5,15,15);

  wnd = new TCWindow(this,"CROSSHAIR");
    wnd->setCursor(TCursor::CROSSHAIR);
    wnd->setShape(25,5,15,15);

  wnd = new TCWindow(this,"HAND");
    wnd->setCursor(TCursor::HAND);
    wnd->setShape(45,5,15,15);

  wnd = new TCWindow(this,"TEXT");
    wnd->setCursor(TCursor::TEXT);
    wnd->setShape(65,5,15,15);

  wnd = new TCWindow(this,"WAIT");
    wnd->setCursor(TCursor::WAIT);
    wnd->setShape(85,5,15,15);

  wnd = new TCWindow(this,"MOVE");
    wnd->setCursor(TCursor::MOVE);
    wnd->setShape(105,5,15,15);


  wnd = new TCWindow(this,"N_RESIZE");
    wnd->setCursor(TCursor::N_RESIZE);
    wnd->setShape(5,25,15,15);

  wnd = new TCWindow(this,"NE_RESIZE");
    wnd->setCursor(TCursor::NE_RESIZE);
    wnd->setShape(25,25,15,15);

  wnd = new TCWindow(this,"NW_RESIZE");
    wnd->setCursor(TCursor::NW_RESIZE);
    wnd->setShape(45,25,15,15);

  wnd = new TCWindow(this,"S_RESIZE");
    wnd->setCursor(TCursor::S_RESIZE);
    wnd->setShape(65,25,15,15);

  wnd = new TCWindow(this,"SE_RESIZE");
    wnd->setCursor(TCursor::SE_RESIZE);
    wnd->setShape(85,25,15,15);

  wnd = new TCWindow(this,"SW_RESIZE");
    wnd->setCursor(TCursor::SW_RESIZE);
    wnd->setShape(105,25,15,15);


  wnd = new TCWindow(this,"W_RESIZE");
    wnd->setCursor(TCursor::W_RESIZE);
    wnd->setShape(5,45,15,15);

  wnd = new TCWindow(this,"E_RESIZE");
    wnd->setCursor(TCursor::E_RESIZE);
    wnd->setShape(25,45,15,15);

  wnd = new TCWindow(this,"QUESTION");
    wnd->setCursor(TCursor::QUESTION);
    wnd->setShape(45,45,15,15);

  wnd = new TCWindow(this,"EXCHANGE");
    wnd->setCursor(TCursor::EXCHANGE);
    wnd->setShape(65,45,15,15);

  wnd = new TCWindow(this,"RESIZE");
    wnd->setCursor(TCursor::RESIZE);
    wnd->setShape(85,45,15,15);

  wnd = new TCWindow(this,"PIRATE");
    wnd->setCursor(TCursor::PIRATE);
    wnd->setShape(105,45,15,15);


  wnd = new TCWindow(this,"MOUSE");
    wnd->setCursor(TCursor::MOUSE);
    wnd->setShape(5,65,15,15);

  wnd = new TCWindow(this,"PENCIL");
    wnd->setCursor(TCursor::PENCIL);
    wnd->setShape(25,65,15,15);

  wnd = new TCWindow(this,"SPRAY");
    wnd->setCursor(TCursor::SPRAY);
    wnd->setShape(45,65,15,15);

  wnd = new TCWindow(this,"HORIZONTAL");
    wnd->setCursor(TCursor::HORIZONTAL);
    wnd->setShape(65,65,15,15);

  wnd = new TCWindow(this,"VERTICAL");
    wnd->setCursor(TCursor::VERTICAL);
    wnd->setShape(85,65,15,15);

  wnd = new TCWindow(this,"TARGET");
    wnd->setCursor(TCursor::TARGET);
    wnd->setShape(105,65,15,15);


  wnd = new TCWindow(this,"DOT");
    wnd->setCursor(TCursor::DOT);
    wnd->setShape(5,85,15,15);

  wnd = new TCWindow(this,"CIRCLE");
    wnd->setCursor(TCursor::CIRCLE);
    wnd->setShape(25,85,15,15);

}

void
TMyWindow::setName(const string &aName)
{
  if (name == aName)
    return;
  name = aName;
  invalidateWindow();
}

void
TMyWindow::paint()
{
  TPen pen(this);
  pen.drawString(5, 120, name);
}



TCWindow::TCWindow(TMyWindow *p, const string &t):
  TWindow(p, t)
{
  mywindow = p;
}

void
TCWindow::mouseEnter(int, int, unsigned)
{
  mywindow->setName(getTitle());
}

void
TCWindow::mouseLeave(int, int, unsigned)
{
  mywindow->setName("");
}
