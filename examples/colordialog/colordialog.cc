#include <toad/colordialog.hh>

using namespace toad;

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TColorDialog wnd(0, "TColorDialog");
    toad::mainLoop();
  } toad::terminate();
  return 0; 
}
