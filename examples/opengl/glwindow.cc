/*
 * TOAD -- A Simple and Powerfull C++ GUI Toolkit for X-Windows
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// OpenGL support for TOAD

// UNDER CONSTRUCTION

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define _TOAD_PRIVATE
#include <toad/toad.hh>
#include "glwindow.hh"

using namespace toad;

TGLWindow::TGLWindow(TWindow *p,const string &t)
:TWindow(p,t)
{
  bX11GC=true;
  bNoBackground = true;
}

void
TGLWindow::createX11Window(TX11CreateWindow *px11)
{
  TX11CreateWindow &x11 = *px11;

  Display *dpy = toad::x11display;
  
  // this shouldn't be hardcoded:
  static int attributeList[] =
  { 
    GLX_RGBA,
    GLX_DOUBLEBUFFER,
    GLX_DEPTH_SIZE, 16,
    None 
  };
  XVisualInfo *vi = glXChooseVisual(dpy, DefaultScreen(dpy), attributeList);

  GLXContext cx = glXCreateContext(dpy, vi, 0, GL_TRUE);
  glContext = cx;

  static XSetWindowAttributes attr;
  unsigned long mask = 0;

  // With MesaLib we use the standard colormap to avoid changing colors
  // on 8bpp systems; with SGI OpenGL we MUST create an own colormap
  //--------------------------------------------------------------------
  #ifndef MESA
  Colormap cmap = XCreateColormap(
    dpy, RootWindow(dpy, vi->screen), vi->visual, AllocNone);
  attr.colormap = cmap;
  mask |= CWColormap;
  #endif

  attr.border_pixel = 0;
  mask |= CWBorderPixel;

  attr.event_mask = x11.attributes->event_mask;
  mask |= CWEventMask;

  x11.display   = dpy;
  x11.depth     = vi->depth;
  x11.wclass    = InputOutput;
  x11.visual    = vi->visual;
  x11.valuemask = mask;
  x11.attributes= &attr;
}

void
TGLWindow::destroy()
{
  glXDestroyContext(x11display, glContext);
}

void
TGLWindow::paint()
{
  // connect OpenGl's drawing context with this window
  ::glXMakeCurrent(x11display,x11window,glContext);
  ::glViewport(0, 0, getWidth(), getHeight());
  glPaint();
  ::glFinish();
  ::glXSwapBuffers(x11display, x11window);
}

void TGLWindow::glPaint()
{
}
