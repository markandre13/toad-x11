#include <toad/toad.hh>
#include <toad/pushbutton.hh>

using namespace toad;

class TMyWindow: public TWindow
{
  public:
    TMyWindow(TWindow *parent, const string &title):TWindow(parent,title){};
  protected:
    void create();
    void actButton();
};

void TMyWindow::create()
{
  TPushButton *btn = new TPushButton(this, "Hit me !");
  btn->setShape(5,5,120,22);
  CONNECT(btn->sigClicked, this,actButton);
}

void TMyWindow::actButton()
{
#if 0
  messageBox(this, 
             getTitle() + " Dialog", 
             "Ouch...", 
             TMessageBox::OK | TMessageBox::ICON_HAND);
#else
  cout << "Ouch..." << endl;
#endif
}

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  TMyWindow wnd(NULL, "3rd program");
  toad::mainLoop();
  toad::terminate();
}
