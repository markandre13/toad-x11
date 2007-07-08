#include <toad/toad.hh>
#include <toad/pushbutton.hh>

using namespace toad;

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TWindow mainwin(NULL, "2nd program");
    TPushButton *btn = new TPushButton(&mainwin, "Motörhead");
    btn->setShape(5,5,120,22);
    toad::mainLoop();
  } toad::terminate();
  return 0;
}
