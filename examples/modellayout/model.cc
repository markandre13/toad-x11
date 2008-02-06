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
#include <toad/radiobutton.hh>
#include <toad/action.hh>

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
  TModelLayout *layout = new TModelLayout("TModelLayout.atv");
  setLayout(layout);
  layout->addModel(&im, "age");
  layout->addModel(&fm, "weight");
  layout->addModel(&tm, "name");
  layout->addModel(&bm, "married");
}

int 
main(int argc, char **argv, char **envv)
{
  toad::initialize(argc, argv, envv);

  TObjectStore& serialize(toad::getDefaultStore());
  serialize.registerObject(new TModelLayout());

  TModelLayout::registerWidget("toad::TCheckBox",       typeid(TBoolModel));
  TModelLayout::registerWidget("toad::TTextArea",       typeid(TTextModel));
  TModelLayout::registerWidget("toad::TTextField",      typeid(TTextModel));
  TModelLayout::registerWidget("toad::TTextField",      typeid(TIntegerModel));
  TModelLayout::registerWidget("toad::TTextField",      typeid(TFloatModel));
  TModelLayout::registerWidget("toad::TScrollBar",      typeid(TIntegerModel));
  TModelLayout::registerWidget("toad::TScrollBar",      typeid(TFloatModel));
  TModelLayout::registerWidget("toad::TGauge",          typeid(TIntegerModel));
  TModelLayout::registerWidget("toad::TGauge",          typeid(TFloatModel));
  TModelLayout::registerWidget("toad::TRadioButton",    typeid(TRadioStateModel));
  TModelLayout::registerWidget("toad::TFatRadioButton", typeid(TRadioStateModel));
  TModelLayout::registerWidget("toad::TPushButton",     typeid(TAction));
   
  TMyDialog *dlg = new TMyDialog(NULL, "Dialog II");
  toad::mainLoop();
  toad::terminate();
  return 0;
}
