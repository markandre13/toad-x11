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

void click1()
{
  cout << __PRETTY_FUNCTION__ << endl;
}

void click2(const char *txt)
{
  cout << __PRETTY_FUNCTION__ << ": " << txt << endl;
}

void click3(int n)
{
  cout << __PRETTY_FUNCTION__ << ": " << n << endl;
}

struct TType1
{
  void click1() {
    cout << __PRETTY_FUNCTION__ << endl;
  }
  
  virtual void click2() {
    cout << __PRETTY_FUNCTION__ << endl;
  }
};

struct TType2:
  public TType1
{
  virtual void click2() {
    cout << __PRETTY_FUNCTION__ << endl;
  }

  void click3() {
    cout << __PRETTY_FUNCTION__ << endl;
  }
  
  virtual void click4() {
    cout << __PRETTY_FUNCTION__ << endl;
  }
};

class Test
{
  public:
    void print0() {
      cout << "print0" << endl;
    }
    void print1(int a) {
      cout << "print1 " << a << endl;
    }
    void print2(int a, int b) {
      cout << "print2 " << a << ", " << b << endl;
    }
    
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);

  TPushButton *btn = new TPushButton(NULL, "Signal Demo");

  TType1 *t1 = new TType1();
  TType2 *t2 = new TType2();
  TType1 *td = t2;
  connect(btn->sigClicked, &click1);
  connect(btn->sigClicked, &click2, "Hello");
  connect(btn->sigClicked, &click3, 17);

  // connect(btn->sigClicked, t1, &t1->click1);     // deprecated by ANSI C++
  // connect(btn->sigClicked, t1, &t2->click1);     // deprecated by ANSI C++
  connect(btn->sigClicked, t1, &TType1::click1);
  connect(btn->sigClicked, t1, &TType2::click1);    // not virtual => sane
  CONNECT(btn->sigClicked, t1, click1);

  // connect(btn->sigClicked, t1, &t1->click2);     // deprecated by ANSI C++
  // connect(btn->sigClicked, t1, &t2->click2);     // deprecated by ANSI C++
  connect(btn->sigClicked, t1, &TType1::click2);
  // connect(btn->sigClicked, t1, &TType2::click2); // OK => compiler error
  CONNECT(btn->sigClicked, t1, click2);

  // connect(btn->sigClicked, t1, &t2->click3);     // deprecated by ANSI C++
  // btn->sigClicked(t1, &TType2::click3);          // OK => compiler error
  // CONNECT(btn->sigClicked, t1, click3);          // OK => compiler error

  // connect(btn->sigClicked, t1, &t2->click4);     // deprecated by ANSI C++
  // btn->sigClicked(t1, &TType2::click4);          // OK => compiler error
  // CONNECT(btn->sigClicked, t1, click4);          // OK => compiler error

  // connect(btn->sigClicked, t2, &t1->click1);     // deprecated by ANSI C++
  // connect(btn->sigClicked, t2, &t2->click1);     // deprecated by ANSI C++
  connect(btn->sigClicked, t2, &TType1::click1);
  connect(btn->sigClicked, t2, &TType2::click1);
  CONNECT(btn->sigClicked, t2, click1);

  // connect(btn->sigClicked., t2, &t1->click2);     // deprecated by ANSI C++
  // connect(btn->sigClicked, t2, &t2->click2);      // deprecated by ANSI C++
  connect(btn->sigClicked, t2, &TType1::click2);     // OK! TType2::click2
  connect(btn->sigClicked, t2, &TType2::click2);     // not virtual => sane
  CONNECT(btn->sigClicked, t2, click2);
  
  connect(btn->sigClicked, td, &TType1::click2);     // OK! TType2::click2
  // connect(btn->sigClicked, td, &TType2::click2);  // OK => compiler error
  CONNECT(btn->sigClicked, td, click2);

  disconnect(btn->sigClicked, td);

  btn->bStaticFrame = true;   // stupid name => bNoResize
  btn->setSize(180,24);
  btn->setLabel("Hit here!");

  toad::mainLoop();
  
  delete btn;
  
  toad::terminate();

  return 0;
}
