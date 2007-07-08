#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define _TOAD_PRIVATE

#include <toad/toad.hh>

using namespace toad;

class TMyWindow:
  public TWindow
{
  public:
    TMyWindow(TWindow *p, const string &t);
    void paint();
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TMyWindow wnd(NULL, "DirectX");
    toad::mainLoop();
  } toad::terminate();
}

TMyWindow::TMyWindow(TWindow *p, const string &t):
  TWindow(p, t)
{
}

void
TMyWindow::paint()
{
  GC gc = XCreateGC(x11display, x11window, 0,0);
  
  XDrawLine(x11display, x11window, gc, 50,50,100,100);
  
  XFreeGC(x11display, gc);
}
