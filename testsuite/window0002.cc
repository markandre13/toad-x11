#include <toad/toad.hh>

using namespace toad;

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); 
  TWindow *w1;
  w1 = new TWindow(NULL, "Window 1");
  toad::mainLoop();
  toad::terminate();
  return 0;
}
