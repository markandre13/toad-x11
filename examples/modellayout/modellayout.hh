#ifndef _TOAD_MODELLAYOUT_HH
#define _TOAD_MODELLAYOUT_HH 1

#include <toad/layout.hh>
#include <toad/figuremodel.hh>
#include <toad/stl/vector.hh>

namespace toad {

class TModelLayoutEditor;

class TModelLayout:
  public TLayout
{
    typedef TLayout super;
    void init();
  public:
    TModelLayout();
    TModelLayout(const string &filename);

    static void registerWidget(const string &widget, const type_info &model);
    TWindow* createWidget(const string &widget, const string &modelname, TModel *model=0);

    PFigureModel gadgets;
    unsigned height;
    unsigned width;
    bool drawfocus;
    TModelLayoutEditor *editor;

    void arrange();
    void paint();

    struct TModelName {
      TModelName(const string &aName, TModel *aModel):
        name(aName), model(aModel) {}
      TModelName(TModel *aModel, const string &aName):
        name(aName), model(aModel) {}
      string name;
      TModel *model;
    };
    typedef GVector<TModelName> TModelList;
    TModelList modellist;
    
    void addModel(TModel *model, const string &name);
    TLayoutEditor* createEditor(TWindow *inWindow, TWindow *forWindow);

    SERIALIZABLE_INTERFACE(toad::, TModelLayout)
};

} // namespace toad

#endif
