#include <toad/toad.hh>
#include <toad/form.hh>
#include <toad/menubar.hh>
#include <toad/action.hh>
#include <toad/macros.hh>

using namespace toad;

#include "MainWindow.hh"

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); { 
    TMainWindow wnd(NULL, "(untitled)");
    toad::mainLoop(); 
  } toad::terminate();
  return 0;
}

CONSTRUCTOR(TMainWindow)
{
  TMenuBar *mb = new TMenuBar(this, "menubar");
  attach(mb, LEFT | RIGHT | TOP);

  TAction *a;

  a = new TAction(this, "file|new");
  a = new TAction(this, "file|open");
  a = new TAction(this, "file|save");
  a = new TAction(this, "file|save as");
  a = new TAction(this, "file|quit");
  CONNECT(a->sigClicked, this,closeRequest);
}
