#include <toad/toad.hh>
#include <toad/connect.hh>
#include <toad/scrollbar.hh>

using namespace toad;

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  TScrollBar *sb = new TScrollBar(NULL, "Window 1");
  sb->setRangeProperties(0, 10, 0, 20);
  BGN_CONNECT_CODE(sb->model->sigChanged, NONE, sb->model, NULL);
    cout << src->getValue() << endl;
  END_CONNECT_CODE();
  toad::mainLoop();
  toad::terminate();
  return 0;
}
