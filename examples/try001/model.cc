/*
 * Need to simplify the table API (again) or write documentation...
 * dialog editor should also configure tooltips
 */

#include "modellayout.hh"

#include <toad/toad.hh>
#include <toad/dialog.hh>
#include <toad/boolmodel.hh>
#include <toad/integermodel.hh>
#include <toad/floatmodel.hh>
#include <toad/textmodel.hh>

using namespace toad;

class TMyDialog:
  public TDialog
{
    TIntegerModel im;
    TFloatModel fm;
    TTextModel tm;
    TBoolModel bm;
    
  public:
    TMyDialog(TWindow *p, const string &t);
};

TMyDialog::TMyDialog(TWindow *p, const string &t):
  TDialog(p, t)
{
  TModelLayout *layout = new TModelLayout();
  layout->addModel(&im, "age");
  layout->addModel(&fm, "weight");
  layout->addModel(&tm, "name");
  layout->addModel(&bm, "married");
  setLayout(layout);
}

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);
  TMyDialog *dlg = new TMyDialog(NULL, "Dialog II");
  toad::mainLoop();
  toad::terminate();
  return 0;
}
