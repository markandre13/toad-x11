/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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

/*
  Example for TOAD first data interface implementation:
*/

#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/textarea.hh>
#include <toad/checkbox.hh>
#include <toad/pushbutton.hh>
#include <toad/radiobutton.hh>
#include <toad/scrollbar.hh>

using namespace toad;

#define RESOURCE(file) "file://resource/" file

class TMainWindow:
  public TDialog
{
  public:
    TMainWindow(TWindow *, const string &);
    TTextArea *tf1;

    // these are the attributes the dialog is going to edit:
    //------------------------------------------------------
    TTextModel data1a, data1b;
    TIntegerModel data2;
    double data3;
    TBoolModel data4;
    TTextModel data5;
    TTextModel data6a, data6b;
    TIntegerModel data7a, data7b;
    GRadioStateModel<int>* rstate;
    
    bool flag;    
    void print();
    void interface();
    
    void printInt(int);
};

void TMainWindow::printInt(int n)
{
  cout << n << endl;
}

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
  // open the resource file with the dialogs layout information
  //-----------------------------------------------------------
  TMainWindow wnd(NULL, "data manipulation test");
  toad::mainLoop();
  } toad::terminate();
  return 0;
}

TMainWindow::TMainWindow(TWindow *p, const string &t)
  :TDialog(p,t)
{
  TTextArea *tf;
  TPushButton *btn;
  TScrollBar *sb;

  // init the attributes with predefined values
  //-------------------------------------------
  data1a = "Hello";
  data1b = "You";
  data2.setValue(4711);
  data3 = 3.1415;
  data4 = true;
  data5 = "Battle Angle Alita";
  
  data7a.setRangeProperties(5,2,0,10);
  data7b.setRangeProperties(5,1,0,10);
  
  flag = false;

  // create a text field control
  // this   : the controls parent window
  // "data1": the controls identification name in the resource file
  // &data1 : a pointer to the attribute the control will modify
  //---------------------------------------------------------------
  tf1 = new TTextArea(this, "data1", &data1a);
  
  // create a push button and let the button call `this->interface()'
  // when pushed
  //-----------------------------------------------------------------
  btn = new TPushButton(this, "Interface");
  connect(btn->sigClicked, this, &TMainWindow::interface);

  // more controls (see above for an explanation of the parameters)
  //---------------------------------------------------------------
  TTextArea *ta;
  ta = new TTextArea(this, "data2");
  ta->setModel(&data2);
//  new TTextArea(this, "data3", &data3);
  new TCheckBox(this, "data4", &data4);
  
  // another text field but this one will modify the attribute at
  // `&data5' at once (you have to call `Apply()' before the other
  // controls copy their value to the attribute)
  //--------------------------------------------------------------
  tf = new TTextArea(this, "data5", &data5);

  // radio buttons (old style)
  //---------------------------
  rstate = new GRadioStateModel<int>();
  rstate->add(new TRadioButton(this, "option1"),   1);
  rstate->add(new TRadioButton(this, "option2"),   2);
  rstate->add(new TRadioButton(this, "option3"),   3);
  
  new TTextArea(this, "data6", &data6a);
  
  sb = new TScrollBar(this, "data7a", &data7a);
  tf = new TTextArea (this, "tf.data7a", &data7a);
  sb = new TScrollBar(this, "data7b", &data7b);
  tf = new TTextArea (this, "tf.data7b", &data7b);
  sb = new TScrollBar(this, "sb2.data7b", &data7b);
//    CONNECT(sb->sigValueChanged, this, printInt, sb);

  btn = new TPushButton(this, "Apply");
//    CONNECT(btn->sigClicked, this, Apply);

  btn = new TPushButton(this, "Reset");
//    CONNECT(btn->sigClicked, this, Reset);

  btn = new TPushButton(this, "Print");
  connect(btn->sigClicked, this, &TMainWindow::print);
    
  // do the layout for all child windows created above; and use
  // the string "TMainWindow" to identify resource in the resource
  // file
  //--------------------------------------------------------------
  loadLayout(RESOURCE("dialog.atv"));
}

void TMainWindow::print()
{
  cout<< "data1a: " << data1a << endl
      << "data1b: " << data1b << endl
      << "data2 : " << data2  << endl
      << "data3 : " << data3  << endl
      << "data4 : " << data4  << endl
      << "data5 : " << data5  << endl
      << "data6a: " << data6a << endl
      << "data7a: " << data7a << endl
      << "data7b: " << data7b << endl
      << "rstate: " << rstate->getValue() << endl
      ;
}

void TMainWindow::interface()
{
  // change the pointer in the interface of the text field
  //------------------------------------------------------
  if (flag)
    tf1->setModel(&data1a);
  else
    tf1->setModel(&data1b);
  flag=!flag;
}
