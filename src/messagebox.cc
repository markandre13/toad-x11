/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/font.hh>
#include <toad/pushbutton.hh>
#include <toad/messagebox.hh>

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
  int icon_width, icon_height;
  if (bitmap) {
    icon_width = bitmap->width;
    icon_height = bitmap->height;
  } else {
    icon_width = 32;
    icon_height = 32;
  }

  // button dimensions
  const unsigned btn_width  = 100;    // width for all buttons
  const unsigned btn_height = getDefaultFont().getHeight() + 8;
  const unsigned btn_hspace = 8;      // space between buttons
  const unsigned vspace = VSPACE;     // space between buttons and window bottom

  unsigned y;

  int msg_height;
  int msg_width=380-32 + icon_width;    // width of the messagebox

  // space for icon & position for text;
  if (type & 0xF000 || bitmap)
    tx=icon_width+32;         // text x position
  else
    tx=8;

  // text size
  int txt_width = msg_width - tx - 8;
  int txt_height = getDefaultFont().getHeightOfTextFromWidth(text, txt_width)
    - getDefaultFont().getHeight();
  tw = txt_width; // store value for 'paint()' method

  if (txt_height<icon_height) {
    ty = vspace+icon_height/2-txt_height/2;
    iy = vspace;
    txt_height=icon_height;
  } else {
    ty = vspace;
    iy = vspace+txt_height/2-icon_height/2;
  }

  y = (vspace<<1) + txt_height;
  
  msg_height = vspace + txt_height + (vspace<<1) + btn_height;
  
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
  unsigned w = n * btn_width + (n-1) * btn_hspace;
  unsigned x = (_w - w)>>1;
  static const char* label[8] =
  {
    "Accept","Abort","Ok","Retry","Yes","No","Cancel","Ignore"
  };
  btn = type & 0xFF;
  unsigned count = 0;
  for(unsigned i=0; i<8; i++) {
    if (btn&1) {
      // create button
      TPushButton *pb = new TPushButton(this, label[i], 1<<i);
        CONNECT(pb->sigActivate, this, button, pb);
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

unsigned TMessageBox::getResult() const
{
  return result;
}

void TMessageBox::button(TPushButton* pb)
{
  result = pb->getID();
  destroyWindow();
//  endDialog(this);
}

void TMessageBox::adjust()
{
  TOADBase::placeWindow(this, _placement, getParent());
}

void TMessageBox::paint()
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
  switch(type & 0xF000)
  {
    case ICON_INFORMATION: {
      pen.setColor(0,0,128);
      pen.fillCircle(x,y,32,32);
      pen.setColor(0,0,0);
      pen.drawCircle(x,y,32,32);
      
      pen.setColor(255,255,255);
      #define TPOINT(a,b,c) p[a].x=b+x;p[a].y=c+y;
      TPoint p[10];
      TPOINT(0,11,11);
      TPOINT(1,19,11);
      TPOINT(2,19,25);
      TPOINT(3,22,25);
      TPOINT(4,22,27);
      TPOINT(5,11,27);
      TPOINT(6,11,25);
      TPOINT(7,14,25);
      TPOINT(8,14,12);
      TPOINT(9,11,12);
      #undef TPOINT
      pen.fillPolygon(p,10);
      pen.fillRectanglePC(x+14,y+2,4,7);
      pen.fillRectanglePC(x+13,y+3,6,5);
      pen.fillRectanglePC(x+12,y+4,8,3);
      } break;

    case ICON_EXCLAMATION:
      pen.setColor(255,255,0);
      pen.fillCircle(x,y,32,32);
      pen.setColor(0,0,0);
      pen.drawCircle(x,y,32,32);
      pen.fillRectanglePC(x+12,y+3,8,18);
      pen.fillRectanglePC(x+15,y+22,2,8);
      pen.fillRectanglePC(x+13,y+23,6,6);
      pen.fillRectanglePC(x+12,y+25,8,2);
      break;

    case ICON_STOP:
      pen.setColor(255,0,0);
      pen.fillCircle(x,y,32,32);
      pen.setColor(0,0,0);
      pen.drawCircle(x,y,32,32);
      pen.setColor(255,255,255);
      pen.fillRectanglePC(x+3,y+13,26,6);
      break;

    case ICON_QUESTION:
      pen.setColor(0,128,0);
      pen.fillCircle(x,y,32,32);
      pen.setColor(0,0,0);
      pen.drawCircle(x,y,32,32);
      pen.setColor(255,255,255);
      static struct {int x,y,w,h;} r[17]={
      {7,7,6,5},
      {19,7,6,6},
      {13,2,6,6},
      {11,3,10,5},
      {18,4,5,5},
      {9,4,5,5},
      {8,5,16,2},
      {12,19,8,3},
      {13,17,7,2},
      {14,16,7,2},
      {15,15,7,1},
      {16,14,7,1},
      {17,13,7,1},
      {18,12,2,2},
      {12,25,8,3},
      {13,24,6,5},
      {14,23,4,7}};
      for(int i=0; i<17; i++)
        pen.fillRectanglePC(r[i].x+x,r[i].y+y,r[i].w,r[i].h);
      break;
  }
}

/**
 * This function creates and displays a modal dialog that contains a text,
 * an icon and pushbuttons.<P>
 *  The following values define <VAR>type</VAR> and can be joined by the
 *  '|' operator:<P>
 *  <TABLE BORDER=1 WIDTH=100%>
 *  <TR><TH>
 *  ICON_EXCLAMATION
 *  </TH><TD>
 *  <IMG SRC="img/icon_exclamation.gif">
 *  </TD></TR><TR><TH>
 *  ICON_HAND
 *  </TH><TD>
 *  <IMG SRC="img/icon_hand.gif">
 *  </TD></TR><TR><TH>
 *  ICON_STOP
 *  </TH><TD>
 *  <IMG SRC="img/icon_stop.gif">
 *  </TD></TR><TR><TH>
 *  ICON_INFORMATION
 *  </TH><TD>
 *  <IMG SRC="img/icon_information.gif">
 *  </TD></TR><TR><TH>
 *  ICON_QUESTION
 *  </TH><TD>
 *  <IMG SRC="img/icon_question.gif">
 *  </TD></TR><TR><TH>
 *  ABORTRETRYIGNORE
 *  </TH><TD>
 *  </TD></TR><TR><TH>
 *  OK
 *  </TH><TD>
 *  </TD></TR><TR><TH>
 *  OKCANCEL
 *  </TH><TD>
 *  </TD></TR><TR><TH>
 *  RETRYCANCEL
 *  </TH><TD>
 *  </TD></TR><TR><TH>
 *  YESNO
 *  </TH><TD>
 *  </TD></TR><TR><TH>
 *  YESNOCANCEL
 *  </TH><TD>
 *  </TD></TR><TR><TH>
 *  DEFBUTTON1
 *  </TH><TD>
 *  The 1st button is the default button, which is the default
 *  setting.
 *  </TD></TR><TR><TH>
 *  DEFBUTTON2
 *  </TH><TD>
 *  The 2nd button is the default button.
 *  </TD></TR><TR><TH>
 *  DEFBUTTON3
 *  </TH><TD>
 *  The 3rd button is the default button.
 *  </TD></TR>
 *  </TABLE>
 *  <P>
 *  Possible return values are: <B>IDACCEPT, IDABORT, IDOK, IDRETRY, IDYES,
 *  IDNO, IDCANCEL</B> and <B>IDIGNORE</B>.
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

