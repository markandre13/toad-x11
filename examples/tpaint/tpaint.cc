/*
 * TPaint -- a simple bitmap editor
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <typeinfo>
#include <string>

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/pushbutton.hh>
#include <toad/fatradiobutton.hh>
#include <toad/form.hh>
#include <toad/scrollbar.hh>
#include <toad/menubar.hh>
#include <toad/action.hh>
#include <toad/textfield.hh>
#include <toad/radiobutton.hh>
#include <toad/checkbox.hh>
#include <toad/mdiwindow.hh>
#include <toad/combobox.hh>
#include <toad/filedialog.hh>
#include <toad/dialogeditor.hh>

using namespace toad;

#define RESOURCE(file) "memory://" file

class TBitmapEditor;
class TTool;

TBitmapEditor *bme;       // current bitmap
TColor color[1];          // current color
TTool *tool = NULL;       // current tool

void RegisterBitmapEditor(TBitmapEditor *ed)
{
  bme = ed;
}

void RemoveBitmapEditor(TBitmapEditor*)
{
  bme = NULL;
}

#include "MainWindow.hh"
#include "NewBitmapDialog.hh"
#include "ZoomDlg.hh"
#include "BitmapEditor.hh"
#include "Tool.hh"
#include "ColorEditor.hh"
#include "Palette.hh"
#include "ToolBar.hh"

/***************************************************************************
                                  ToadMain
 ***************************************************************************/
void createMemoryFiles();

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  try{
    createMemoryFiles();
    tool = new TDrawTool();
    TMainWindow wnd(NULL,"TPaint");
    toad::mainLoop();
    delete tool;
  }
  catch(exception &e) {
    cout << e.what() << endl;
  }
  toad::terminate();
  return 0;
}

/***************************************************************************
                                 TMainWindow
 ***************************************************************************/
TMainWindow::TMainWindow(TWindow *p,const string &t)
  :TForm(p,t)
{
}

static TBitmap *bmpTPaint;


void TMainWindow::create()
{
  bKeepOwnBorder=true;
  setSize(640,480);

  // set icon for main window
  //--------------------------
  bmpTPaint = new TBitmap;
  bmpTPaint->load(RESOURCE("tpaint.png"));
  setIcon(bmpTPaint);

  // create MDI window with 4 children 
  //-----------------------------------
  ca=new TMDIWindow(this, "Test");

    (new TToolBar(ca,"TToolBar"))
      ->setPosition(390,5);
    (new TPalette(ca,"TPalette"))
      ->setPosition(400,70);
    (new TColorEditor(ca,"TColorEditor"))
      ->setPosition(380,140);

  // configure TMenuBar
  //--------------------
  TMenuBar *mb=new TMenuBar(this, "menubar");

  TAction *action;
  
  action = new TAction(this, "file|new");
  CONNECT(action->sigClicked, this, menuNew);
  action = new TAction(this, "file|open");
  CONNECT(action->sigClicked, this, menuOpen);
  action = new TAction(this, "file|save_as");
  CONNECT(action->sigClicked, this, menuSaveAs);
  action = new TAction(this, "file|quit");
  CONNECT(action->sigClicked, this, menuQuit);
  action = new TAction(this, "option|zoom");
  CONNECT(action->sigClicked, this, menuZoom);

#if 0
  mb->BgnPulldown("Windows");
    mc = mb->AddCheck("Toolbar",0);
    CONNECT(mc->sigClicked, this,toolbar);
    mb->AddCheck("Palette", 0);
    mb->AddCheck("ColorEditor", 0);
    mb->BgnPulldown("More");
      mb->AddItem("Red");
      mb->AddItem("Green");
      mb->AddItem("Blue");
    mb->EndPulldown();
  mb->EndPulldown();
#endif

  action = new TAction(this, "help|info");
  CONNECT(action->sigClicked, this, menuInfo);
  action = new TAction(this, "help|copyright");
  CONNECT(action->sigClicked, this, menuCopyright);
  action = new TAction(this, "help|help");
  CONNECT(action->sigClicked, this, menuHelp);

  // configure TForm
  attach(mb, SIDE_TOP | SIDE_LEFT | SIDE_RIGHT, ATTACH_FORM);

  attach(ca, SIDE_TOP, ATTACH_WINDOW, mb);
  attach(ca, SIDE_LEFT | SIDE_RIGHT | SIDE_BOTTOM, ATTACH_FORM);
}

void TMainWindow::menuNew()
{
  TNewBitmapDialog(this,"TPaint: New", ca).doModalLoop();
}

void TMainWindow::menuOpen()
{
  TFileDialog dlg(this,"TPaint: Open..");
//  TAlterBitmap::getInputFilter(dlg);
  dlg.doModalLoop();
  if (dlg.getResult()==TMessageBox::OK) {
    TBitmapEditor *bme = new TBitmapEditor(ca, dlg.getFilename());
    if (bme->load(dlg.getFilename().c_str() )) {
      bme->createWindow();
    } else {
      delete bme;
    }
  }
}

void TMainWindow::menuSaveAs()
{
  if (bme) {
    TFileDialog dlg(this,"TPaint: Save As..");
//    TAlterBitmap::getOutputFilter(dlg);
    dlg.setFilename(bme->getTitle());
    dlg.doModalLoop();
    if (dlg.getResult()==TMessageBox::OK) {
      try {
        bme->save(dlg.getFilename().c_str(), 0 /* dlg.getXtra() */);
        bme->setTitle(dlg.getFilename());
      }
      catch(exception &e) {
        messageBox(this, "Failed to save", e.what(), 
                   TMessageBox::OK | TMessageBox::ICON_STOP);
      }
    }
  } else {
    messageBox(this,"TPaint: Save As..",
      "No active bitmap editor window.",
      TMessageBox::OK | TMessageBox::ICON_EXCLAMATION);
  }
}

void TMainWindow::menuZoom()
{
  if (bme)
    TZoomDlg(this,"TPaint: change zoom", bme).doModalLoop();
  else
    messageBox(this, "TPaint: change zoom", "No active bitmap editor.",
    TMessageBox::OK | TMessageBox::ICON_INFORMATION );
}

void TMainWindow::menuInfo()
{
  messageBox(this, "TPaint: Info",
    "TPaint v0.2 Alpha\n"
    "Copyright © 1996-2002 by Mark-André Hopf\n\n"
    "This program is part of the TOAD GUI Toolkit, see:\n"
    "www: http://www.mark13.de/toad/\n"
    "for further information or write to:\n"
    "eMail: mhopf@mark13.de"
    , TMessageBox::OK, bmpTPaint);
}

void TMainWindow::menuCopyright()
{
  messageBox(this, "TPaint: Copyright",
    "TPaint v0.2 Alpha\n"
    "Copyright © 1996-2002 by Mark-André Hopf\n\n"
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
    "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA."
    , TMessageBox::OK, bmpTPaint);
}

void TMainWindow::menuHelp()
{
  messageBox(this, "TPaint: Help",
    "Use the middle mouse button for draging and droping "
    "the colours around.\n\n"
    "More help will be provided in the future when the "
    "THyperView class is done."
    , TMessageBox::ICON_INFORMATION | TMessageBox::OK);
}

void TMainWindow::menuQuit()
{
  postQuitMessage(0);
}

void TMainWindow::toolbar()
{
  printf("void MainWindow::toolbar\n");
}

/***************************************************************************
                                  TToolBar
 ***************************************************************************/
TToolBar::TToolBar(TWindow *p,const string &t)
  :super(p,t)
{
  setBackground(TColor::DIALOG);
  bFocusManager = true;
  setSize(120,40);
  TFatRadioButton *btn;
  
  TRadioStateModel *state = new TRadioStateModel();
  
  btn=new TFatRadioButton(this, "Plot", state);
    CONNECT(btn->sigClicked, this,button, ID_PLOT);
    btn->setShape(0,0,40,40);
    btn->loadBitmap(RESOURCE("tpaint_plot.png"));
    btn->setToolTip("draw points");

  btn=new TFatRadioButton(this, "Draw", state);
    CONNECT(btn->sigClicked, this,button, ID_DRAW);
    btn->setShape(40,0,40,40);
    btn->loadBitmap(RESOURCE("tpaint_draw.png"));
    btn->setToolTip("draw lines");

  btn=new TFatRadioButton(this, "Fill", state);
    CONNECT(btn->sigClicked, this,button, ID_FILL);
    btn->setShape(80,0,40,40);
    btn->loadBitmap(RESOURCE("tpaint_fill.png"));
    btn->setToolTip("fill a region");
}

void
TToolBar::button(EState state)
{
  if (::tool) delete ::tool;      // remove old tool
  
  switch(state)
  {
    case ID_PLOT:
      ::tool = new TPlotTool();
      break;
    case ID_DRAW:
      ::tool = new TDrawTool();
      break;
    case ID_FILL:
      ::tool = new TFillTool();
      break;
  }
}
