/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/messagebox.hh>
#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/font.hh>
#include <toad/pushbutton.hh>
#include <toad/bitmap.hh>

using namespace toad;

/**
 * \class toad::TMessageBox
 *
 * For the MessageBox function in TOADBase.
 */
TMessageBox::TMessageBox(TWindow* p, 
                         const string &t1, 
                         const string &t2,
                         ulong t,
                         TBitmap *bitmap,
                         EWindowPlacement placement)
  :TWindow(p,t1)
{
  setBackground(TColor::DIALOG);
  bShell = bStaticFrame = true;
  bFocusManager = true;
  _placement = placement;

  text=t2;
  type=t;
  result=0;
  this->bitmap = bitmap;

  #define VSPACE 16

  // icon dimensions
  TCoord icon_width, icon_height;
  if (bitmap) {
    icon_width = bitmap->width;
    icon_height = bitmap->height;
  } else {
    icon_width = 68;
    icon_height = 44;
  }

  // button dimensions
  const TCoord btn_width  = 100;    // width for all buttons
  const TCoord btn_height = getDefaultFont().getHeight() + 8;
  const TCoord btn_hspace = 8;      // space between buttons
  const TCoord vspace = VSPACE;     // space between buttons and window bottom

  TCoord y;

  TCoord msg_height;
  TCoord msg_width=380-32 + icon_width;    // width of the messagebox

  // space for icon & position for text;
  if (type & 0xF000 || bitmap)
    tx=icon_width+32;         // text x position
  else
    tx=8;

  // text size
  TCoord txt_width = msg_width - tx - 8;
  TCoord txt_height = TPen::getHeightOfTextFromWidth(&getDefaultFont(), text, txt_width);

  tw = txt_width; // store value for 'paint()' method

  if (txt_height<icon_height) {
    ty = vspace+icon_height/2-txt_height/2;
    iy = vspace;
    txt_height=icon_height;
  } else {
    ty = vspace;
    iy = vspace+txt_height/2-icon_height/2;
  }

  y = vspace*2 + txt_height;
  
  msg_height = vspace + txt_height + vspace*2 + btn_height;
  
  setSize(msg_width,msg_height);

  // create buttons
  //----------------
      
  // count buttons
  unsigned btn = type & 0xFF;
  unsigned n = 0;
  for(int i=0; i<8; i++) {
    if(btn&1)
      n++;
    btn>>=1;
  }
  if (n==0) {
    type|=OK;
    n++;
  }
  
  // create buttons
  TCoord w = n * btn_width + (n-1) * btn_hspace;
  TCoord x = (this->w - w)/2;
  static const char* label[8] =
  {
    "Accept","Abort","Ok","Retry","Yes","No","Cancel","Ignore"
  };
  btn = type & 0xFF;
  unsigned count = 0;
  for(unsigned i=0; i<8; i++) {
    if (btn&1) {
      // create button
      TPushButton *pb = new TPushButton(this, label[i]);
        CONNECT(pb->sigClicked, this, button, 1<<i);
        pb->setShape(x,y,btn_width,btn_height);
      x=x+btn_width+btn_hspace;
      // MB_DEFBUTTON?
      count+=0x0100;
      if (count == (type & 0x0F00) && getLastChild())
        getLastChild()->setFocus();
    }
    btn>>=1;
  }
}

unsigned
TMessageBox::getResult() const
{
  return result;
}

void
TMessageBox::button(int id)
{
  result = id;
  destroyWindow();
}

void
TMessageBox::adjust()
{
  TOADBase::placeWindow(this, _placement, getParent());
}

void
TMessageBox::paint()
{
  TPen pen(this);
  pen.drawTextWidth(tx,ty,text,tw);

  int x=16;
  int y=iy;

  if (bitmap) {
    pen.drawBitmap(x,y,bitmap);
    return;
  }

  // draw icon

  unsigned idx = ((type & 0xF000) >> 12)-1;
  if (idx>4)
    return;
  static TBitmap *bmp[5] = { 0, 0, 0, 0, 0 };
  static const char *filename[5] = {
    "memory://toad/exclamation.png",
    "memory://toad/hand.png",
    "memory://toad/stop.png",
    "memory://toad/information.png",
    "memory://toad/question.png"
  };
  if (bmp[idx]==0) {
    bmp[idx] = new TBitmap();
    bmp[idx]->load(filename[idx]);
  }
  pen.drawBitmap(x, y, bmp[idx]);
}

/**
 * This function creates and displays a modal dialog that contains a text,
 * an icon and pushbuttons.
 *
 *  The following values define <VAR>type</VAR> and can be joined by the
 *  '|' operator:
 *
 *  \li
 *    <img src="img/information.png"> ICON_INFORMATION:
 *    Just informational, you may ignore it.
 *  \li
 *    <img src="img/question.png"> ICON_QUESTION:
 
 *  \li
 *    <img src="img/exclamation.png"> ICON_EXCLAMATION:
 *  \li
 *    <img src="img/hand.png"> ICON_HAND:
 *    Wait a moment and think about what you're going to do.
 *  \li
 *    <img src="img/stop.png"> ICON_STOP
 *    Don't do it!
 *
 *  The following button combinations are supported:
 *
 *  \li ABORTRETRYIGNORE
 *  \li OKCANCEL
 *  \li RETRYCANCEL
 *  \li YESNO
 *  \li YESNOCANCEL
 *
 *  When the message box opens, the first button is selected and
 *  you should keep it like this to avoid to confuse the user. Anyway
 *  you can also select another button with the following values:
 *
 *  \li DEFBUTTON1
 *  \li DEFBUTTON2
 *  \li DEFBUTTON3
 *
 *  The function returns which button was pressed with one of the values
 *  below or '0' (Null, Zero), in case the dialog was closed without a button
 *  being pressed, which you should treat as 'CANCEL':
 *
 *  \li ACCEPT
 *  \li ABORT
 *  \li RETRY
 *  \li YES
 *  \li NO
 *  \li CANCEL
 *  \li IGNORE
 */
unsigned 
toad::messageBox(TWindow* parent, 
           const string &title,
           const string &text,
           unsigned long type,
           TBitmap *bmp,
           EWindowPlacement placement)
{
  if (TOADBase::bAppIsRunning) {
    TMessageBox* msg = new TMessageBox(parent,
                                       title,
                                       text, 
                                       type,
                                       bmp,
                                       placement);
    TOADBase::doModalLoop(msg);
    int result = msg->getResult();
    delete msg;
    return result;
  } else {
    cerr << "messageBox " << title.c_str() << ": " << text.c_str() << endl;
  }
  return 0;
}

