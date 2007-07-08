#include <toad/toad.hh>
#include <toad/messagebox.hh>

using namespace toad;

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  toad::messageBox(NULL, "Hello", "Ouch...", 
    TMessageBox::OK | TMessageBox::ICON_EXCLAMATION);
  toad::terminate();
}
