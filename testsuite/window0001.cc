#include <toad/toad.hh>

using namespace toad;

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  TWindow *w1, *w2;
  w1 = new TWindow(NULL, "Window 1");
  cout << w1 << endl;
  w2 = new TWindow(w1, "Window 2");
  cout << w2 << endl;
  delete w1;
  toad::terminate();
  return 0;
}
