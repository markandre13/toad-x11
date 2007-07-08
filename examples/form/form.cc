#include <toad/toad.hh>
#include <toad/formlayout.hh>
#include <toad/pushbutton.hh>
#include <toad/scrollbar.hh>

using namespace toad;

int
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv); {
    TWindow *wnd = new TWindow(NULL,"form example");
    TPushButton *btn;
    btn = new TPushButton(wnd, "menubar");
    btn = new TPushButton(wnd, "toolbar");
    btn = new TPushButton(wnd, "clientarea");
    btn = new TPushButton(wnd, "statusbar");
    TScrollBar *sb;
    sb = new TScrollBar(wnd, "horizontal");
    sb = new TScrollBar(wnd, "vertical");

    TFormLayout *form = new TFormLayout;
    form->attach("menubar", TFormLayout::TOP|TFormLayout::LEFT|TFormLayout::RIGHT);
    
    form->attach("toolbar", TFormLayout::LEFT|TFormLayout::RIGHT);
    form->attach("toolbar", TFormLayout::TOP, "menubar");
    
    form->attach("vertical", TFormLayout::LEFT);
    form->attach("vertical", TFormLayout::TOP, "menubar");
    form->attach("vertical", TFormLayout::BOTTOM, "horizontal");

    form->attach("horizontal", TFormLayout::LEFT, "vertical");
    form->attach("horizontal", TFormLayout::RIGHT);
    form->attach("horizontal", TFormLayout::BOTTOM, "statusbar");
    
    form->attach("clientarea", TFormLayout::LEFT, "vertical");
    form->attach("clientarea", TFormLayout::RIGHT);
    form->attach("clientarea", TFormLayout::TOP, "menubar");
    form->attach("clientarea", TFormLayout::BOTTOM, "horizontal");
    
    form->attach("statusbar", TFormLayout::LEFT|TFormLayout::RIGHT|TFormLayout::BOTTOM);

    wnd->setLayout(form);

    toad::mainLoop();
  } toad::terminate();
  return 0;
}
