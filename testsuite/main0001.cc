#include <toad/toad.hh>

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  toad::terminate();
}
    
