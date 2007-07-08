// `toad-config --cxx --cxxflags --libs` -g html.cc
// or
// g++ `pkg-config --cflags --libs toad` -g html.cc
//
// g++ `pkg-config --cflags --libs toad` -g -pg -fprofile-arcs html.cc
// rm gmon.out
// ./a.out http://localhost/~mark/other/wimp_geschichte/
// gperf a.out gmon.out

// lsqou, rsquo

#include <toad/toad.hh>
#include <toad/menubar.hh>
#include <toad/form.hh>
#include <toad/action.hh>
#include <toad/htmlview.hh>

using namespace toad;

class THTMLBrowser:
  public TForm
{
  public:
    THTMLBrowser(TWindow *parent, const string &title);
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  {
    THTMLBrowser wnd(0, "TOAD HTML Browser");
    TAction *a = new TAction(&wnd, "file|exit");
    connect(a->sigClicked, &wnd, &THTMLBrowser::closeRequest);
    toad::mainLoop();
  }
  toad::terminate();
  return 0;
}

THTMLBrowser::THTMLBrowser(TWindow *parent, const string &title):
  TForm(parent, title)
{
  TMenuBar *mb = new TMenuBar(this, "menubar");
  attach(mb, TOP|LEFT|RIGHT);
  
  THTMLView *view = new THTMLView(this, "htmlview");
  attach(view, TOP, mb);
  attach(view, LEFT|RIGHT|BOTTOM);

  // home, back, forward, url
  
  // statusbar
}
