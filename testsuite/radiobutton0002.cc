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
#if 0
  int x,y,w,h;
  x=y=5;
  w=320; 
  h=getDefaultFont().getHeight()+2;

  TRadioStateModel *state = new TRadioStateModel();
  TRadioButton *btn;
  
  btn = new TRadioButton(this, "One", state);
  btn->setShape(x,y,w,h);
  y+=h;
  CONNECT(btn->sigChanged, this, action, 1);
  btn = new TRadioButton(this, "Two", state);
  btn->setShape(x,y,w,h);
  y+=h;
  CONNECT(btn->sigChanged, this, action, 2);
#endif
}

void
TMyWindow::action(int v)
{
  cout << "radiobutton changed to value " << v << endl;
}
