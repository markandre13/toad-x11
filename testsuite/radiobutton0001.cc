#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/radiobutton.hh>

using namespace toad;

class TMyWindow:
  public TDialog
{
  public:
    TMyWindow(TWindow *parent, const string &title);
    void action(int);
};

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); 
  TWindow *wnd;
  wnd = new TMyWindow(NULL, "TOAD");
  toad::mainLoop();
  toad::terminate();
  delete wnd;
  return 0;
}

TMyWindow::TMyWindow(TWindow *parent, const string &title):
  TDialog(parent, title)
{
  GRadioStateModel<int> *state = new GRadioStateModel<int>();
  
  int x,y,w,h;
  x=y=5;
  w=320; h=getDefaultFont().getHeight()+2;
  state->add(new TRadioButton(this, "One"),   1)
    ->setShape(x,y,w,h);
  y+=h;
  state->add(new TRadioButton(this, "Two"),   2)
    ->setShape(x,y,w,h);
  y+=h;
  state->add(new TRadioButton(this, "Three"), 3)
    ->setShape(x,y,w,h);
  CONNECT_VALUE(state->sigChanged, this, action, state);
}

void
TMyWindow::action(int v)
{
  cout << "radiobutton changed to value " << v << endl;
}
