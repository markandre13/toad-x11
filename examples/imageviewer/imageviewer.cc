/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2002 by Mark-André Hopf <mhopf@mark13.de>
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
#include <toad/scrollbar.hh>
#include <toad/region.hh>
#include <toad/form.hh>
#include <toad/menubar.hh>
#include <toad/action.hh>
#include <toad/filedialog.hh>
#include <toad/scrolledarea.hh>
#include <toad/bitmap.hh>

using namespace toad;

class TMainWindow: public TForm
{
  public:
    typedef TForm super;
    TMainWindow(TWindow *p, const string &t):super(p,t){}
  protected:
    void create();
    void menuQuit();
};

class TImageViewer: public TScrolledArea
{
  public:
    typedef TScrolledArea super;
    TImageViewer(TWindow *p, const string &t);
    void Load();
    void paint();
    TBitmap bmp;
};

int main(int argc, char** argv, char** envv)
{
  try {
    toad::initialize(argc, argv, envv);
    
    TMainWindow *window = new TMainWindow(NULL, "TOAD View");

    toad::mainLoop();

    delete window;

    toad::terminate();
    
    return 0;

  } catch (exception &e) {
    cout << "exception: " << e.what() << endl;
  }
}

void TMainWindow::create()
{
  TMenuBar *mb = new TMenuBar(this,"menubar");
  TAction *act;
  TImageViewer *view = new TImageViewer(this, "image");
  
  act = new TAction(this, "file|quit");
  CONNECT(act->sigClicked, this, menuQuit);
  
  attach(mb, SIDE_TOP | SIDE_LEFT | SIDE_RIGHT, ATTACH_FORM);
  attach(view, SIDE_TOP, ATTACH_WINDOW, mb);
  attach(view, SIDE_LEFT | SIDE_RIGHT | SIDE_BOTTOM, ATTACH_FORM);
}

void TMainWindow::menuQuit()
{
  postQuitMessage(0);
}

TImageViewer::TImageViewer(TWindow *p, const string &t)
  :super(p,t)
{
  setBackground(191,191,191);
  TAction *act;
  act = new TAction(this, "file|open");
  CONNECT(act->sigClicked, this, Load);
}

void TImageViewer::Load()
{
  TFileDialog dlg(NULL,"Imageviewer: Open..");

  //  TBitmap::getInputFilter(dlg);

  dlg.doModalLoop();
  if (dlg.getResult()==TMessageBox::OK) {
    try {
      bmp.load(dlg.getFilename().c_str());
    } catch(exception &e) {
      TMessageBox(NULL, "Error", e.what(), 
           TMessageBox::ICON_INFORMATION|TMessageBox::OK);
      return;
    }
    setArea(bmp.width,bmp.height, 0,0, 1,1);
    invalidateWindow();
  }
}

void TImageViewer::paint()
{
  TPen pen(this);
  clipPen(pen);
  pen.drawBitmap(-area_x,-area_y,bmp);
}
