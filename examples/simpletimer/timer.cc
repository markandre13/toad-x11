/**
 * \file timer.cc
 *
 * This is a small example on how the 'select' based timer can be used.
 */

#include <toad/toad.hh>
#include <toad/simpletimer.hh>
#include <iostream>

using namespace toad;

class TMyWindow:
  public TWindow
{
  public:
    TMyWindow(TWindow* p, const string& t);
};

class TMyTimer:
  public TSimpleTimer
{
    string _name;
  public:
    TMyTimer(const string name, int ms) {
      _name = name;
      startTimer(0, ms);
    }
    void tick() {
      cout << "tick: " << _name << endl;
    }
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMyWindow wnd(NULL, "Timer Demo");
    toad::mainLoop();
  } toad::terminate();
  return 0;
}

TMyWindow::TMyWindow(TWindow* p, const string& t)
  :TWindow(p, t)
{
  new TMyTimer("timer 1s", 1000000);
  new TMyTimer("timer 0,5s", 500000);
}
