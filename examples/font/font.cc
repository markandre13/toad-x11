/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <toad/toad.hh>

using namespace toad;

class TMyWindow:
  public TWindow
{
  public:
    TMyWindow(TWindow *p, const string &t):TWindow(p,t)
    {
      setSize(480,390);
    }
    void paint();
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMyWindow wnd(NULL, "Font Test");
    toad::mainLoop();
  } toad::terminate();
}

void
TMyWindow::paint()
{
  TPen pen(this);
  PFont fn;
  int y = 0;

  static const char *text = "I'm not okay, you're not okay. But hey, that's okay.";
  
//  pen.rotate(15);

  pen.setFont("arial,helvetica,sans-serif:size=4");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:size=6");
  pen.drawString(5,y, text);
  y+=pen.getHeight();
#if 0
  pen.setFont("arial,helvetica,sans-serif:size=8");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:size=10");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:size=12");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:size=18");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:size=24");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:italic");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:bold");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("arial,helvetica,sans-serif:bold:italic");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("serif");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("serif:italic");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("serif:bold");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("serif:bold:italic");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("monospace");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("monospace:italic");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("monospace:bold");
  pen.drawString(5,y, text);
  y+=pen.getHeight();

  pen.setFont("monospace:bold:italic");
  pen.drawString(5,y, text);
#endif
}
