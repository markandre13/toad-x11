#include <toad/toad.hh>
#include <toad/scrollbar.hh>

using namespace toad;

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  TScrollBar *sb = new TScrollBar(NULL, "Window 1");
  toad::mainLoop();
  toad::terminate();
  return 0;
}
