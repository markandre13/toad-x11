#include <toad/toad.hh>

using namespace toad;

void
bar()
{
  printStackTrace();
}

void
foo()
{
  bar();
}

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  foo();
  toad::terminate();
}
