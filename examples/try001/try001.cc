#include <toad/toad.hh>

using namespace toad;

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TWindow wnd(NULL, "Hello Dave...");
    toad::mainLoop();
  } toad::terminate();
  return 0;
}
