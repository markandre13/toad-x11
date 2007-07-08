/*
 * robot -- a demonstration of the TOAD TGLWindow
 * Copyright (C) 1997-2003 by Mark-André Hopf <mhopf@mark13.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *   
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Requirements:
 * - Mesa 2.0 or OpenGL
 * - TOAD
 */

#include <toad/toad.hh>
#include <toad/form.hh>
#include <toad/menubar.hh>
#include <toad/action.hh>
#include <toad/scrollbar.hh>
#include "glwindow.hh"

// redefine 'exception' to avoid trouble between C++ and SGIs <math.h>
#define exception mexception
#include <math.h>
#undef exception

#include <cstdio>
#include <cstdlib>
#include <vector>

#define DEBUG

using namespace toad;

class TViewer:
  public TGLWindow
{
  public:
    GLfloat angle[4];
    TViewer(TWindow *p,const string &t):TGLWindow(p,t)
    {
      for(int i=0;i<4;i++)
        angle[i]=0.0;
    };
  protected:
    void glPaint();
};

class TMainWindow:
  public TForm
{
  public:
    TMainWindow(TWindow *p,const string &t)
    :TForm(p,t){};
  protected:
    TViewer *gl;
    void create();
    void menuQuit();
    void menuInfo();
    void menuCopyright();
    void actScrollBar(TScrollBar*,int);
};


int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMainWindow wnd(NULL, "Robot");
    toad::mainLoop();
  } toad::terminate();
  return 0;
}

// TMainWindow
//--------------------------------------------------------------------
void TMainWindow::create()
{
  setSize(640,480);
  setBackground(TColor::DIALOG);

  // create menubar
  //----------------
  TMenuBar *mb=new TMenuBar(this, "menubar");
  
  TAction *action;
  action = new TAction(this, "file|quit");
  CONNECT(action->sigClicked, this,menuQuit);

  action = new TAction(this, "help|info");
  CONNECT(action->sigClicked, this,menuInfo);

  action = new TAction(this, "help|copyright");
  CONNECT(action->sigClicked, this,menuCopyright);

  // create viewer
  //---------------
  gl = new TViewer(this,"OpenGL Window");

  // create scrollbars
  //-------------------
  const int maxbar = 4;

  TScrollBar *sb[maxbar];
  int i;
  for(i=0; i<maxbar; i++) {
    sb[i] = new TScrollBar(this,"");
    sb[i]->setRangeProperties(0, 1, -180, 180);
    // CONNECT(sb[i]->sigValueChanged, this, actScrollBar, sb[i], i);
    CONNECT(sb[i]->getModel()->sigChanged, this, actScrollBar, sb[i], i);
  }

  // set up form definition
  //------------------------
  attach(mb, SIDE_TOP | SIDE_LEFT | SIDE_RIGHT, ATTACH_FORM);

  for(i=0; i<maxbar; i++) {
    attach(sb[i], SIDE_TOP, ATTACH_WINDOW, mb);
    attach(sb[i], SIDE_BOTTOM, ATTACH_FORM);
    if (i==0)
      attach(sb[0], SIDE_RIGHT, ATTACH_FORM);
    else
      attach(sb[i], SIDE_RIGHT, ATTACH_WINDOW, sb[i-1]);
    distance(sb[i],10);
  }
  
  attach(gl, SIDE_TOP, ATTACH_WINDOW, mb);
  attach(gl, SIDE_LEFT | SIDE_BOTTOM, ATTACH_FORM);
  attach(gl, SIDE_RIGHT, ATTACH_WINDOW, sb[maxbar-1]);
  distance(gl, 10);
}

void
TMainWindow::actScrollBar(TScrollBar *sb, int id)
{
  // printf("Value of %i set to %i\n",sb->ID(),sb->Value());
  gl->angle[id] = sb->getValue();
  gl->invalidateWindow();
}

void
TMainWindow::menuQuit()
{
  if(messageBox(this,
                getTitle(),
                "Do you really want to quit the program?",
                TMessageBox::ICON_QUESTION | TMessageBox::YESNO
               ) == TMessageBox::YES)
  {
    postQuitMessage(0);
  }
}

void
TMainWindow::menuInfo()
{
  messageBox(this, 
             getTitle(),
            "This program is a demonstration for the TOAD GUI Toolkit "
            "cooperating with Mesa 2.0 and OpenGL.\n"
            "For further information write to:\n" 
            "mhopf@mark13.de",
            TMessageBox::ICON_INFORMATION | TMessageBox::OK);
}

void
TMainWindow::menuCopyright()
{
  messageBox(this, getTitle(),
    "Copyright © 1997,99 by Mark-André Hopf\n\n"
    "This program is free software; you can redistribute it and/or modify "
    "it under the terms of the GNU General Public License as published by "
    "the Free Software Foundation; either version 2 of the License, or "
    "(at your option) any later version.\n\n"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
    "GNU General Public License for more details.\n\n"
    "You should have received a copy of the GNU General Public License "
    "along with this program; if not, write to the Free Software "
    "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.",
    TMessageBox::ICON_INFORMATION | TMessageBox::OK);
}            

// TViewer
//--------------------------------------------------------------------
static void 
segment(GLfloat len)
{
  glBegin(GL_POLYGON);
  glNormal3f( 0.0, 1.0, 0.0);
  glVertex3f(-0.5, len,-0.5);
  glVertex3f(-0.5, len, 0.5);
  glVertex3f( 0.5, len, 0.5);
  glVertex3f( 0.5, len,-0.5);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3f( 0.0,-1.0, 0.0);
  glVertex3f(-0.5, 0.0,-0.5);
  glVertex3f( 0.5, 0.0,-0.5);
  glVertex3f( 0.5, 0.0, 0.5);
  glVertex3f(-0.5, 0.0, 0.5);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3f( 0.0, 0.0, 1.0);
  glVertex3f(-0.5, 0.0, 0.5);
  glVertex3f( 0.5, 0.0, 0.5);
  glVertex3f( 0.5, len, 0.5);
  glVertex3f(-0.5, len, 0.5);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3f(-1.0, 0.0,  0.0);
  glVertex3f(-0.5, 0.0, -0.5);
  glVertex3f(-0.5, 0.0,  0.5);
  glVertex3f(-0.5, len,  0.5);
  glVertex3f(-0.5, len, -0.5);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3f( 0.0, 0.0, -1.0);
  glVertex3f(-0.5, 0.0, -0.5);
  glVertex3f(-0.5, len, -0.5);
  glVertex3f( 0.5, len, -0.5);
  glVertex3f( 0.5, 0.0, -0.5);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3d(1.0, 0.0,  0.0);
  glVertex3f(0.5, 0.0, -0.5);
  glVertex3f(0.5, len, -0.5);
  glVertex3f(0.5, len,  0.5);
  glVertex3f(0.5, 0.0,  0.5);
  glEnd();
}

void
TViewer::glPaint()
{
  glClearColor( 0.0, 0.0, 0.5, 0.0 );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glFrustum( -1.0,    // left
              1.0,    // right
              -1.0,   // bottom
              1.0,    // top
              1.0,    // near
              40.0    // far
  );
  
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_LIGHT0);

  glTranslatef(0.0, -5.0, -10.0);
  glRotatef(angle[0], 0.0, 1.0, 0.0);
  glRotatef(angle[1], 1.0, 0.0, 0.0);

  glColor3f(1.0, 0.0, 0.0);
  segment(5.0);
  
  glTranslatef(0.0, 5.0, 0.0);
  glRotatef(angle[2], 1.0, 0.0, 0.0);
  glColor3f(0.0, 1.0, 0.0);
  segment(3.0);

  glTranslatef(0.0, 3.0, 0.0);
  glRotatef(angle[3], 1.0, 0.0, 0.0);
  glColor3f(0.0, 0.5, 1.0);
  segment(2.0);
}
