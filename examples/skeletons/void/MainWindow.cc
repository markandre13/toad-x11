#include <toad/toad.hh>
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
}
