#ifndef _TOAD_MODELLAYOUTEDITOR_HH
#define _TOAD_MODELLAYOUTEDITOR_HH 1

#include "modellayout.hh"
#include <toad/figureeditor.hh>
#include <toad/textmodel.hh>
#include <toad/integermodel.hh>
#include <toad/labelowner.hh>

namespace toad {

class TModelLayoutEditor:
  public TLayoutEditor
{
  public:
    TModelLayoutEditor(TWindow*, const string&, TModelLayout*, TWindow *forWindow);
    ~TModelLayoutEditor();
    
    TModelLayout *layout;
    TWindow *forWindow;
    TTable *tmodel, *twidget;
    
    GVector<string> widgets;
    
    void modelSelected();
    void addWidget();
    
    class TMyMouseFilter:
      public TEventFilter
    {
      public:
        TMyMouseFilter(TModelLayoutEditor *parent) {
          this->parent = parent;
        }
        bool mouseEvent(TMouseEvent &me) {
          parent->gedit.mouseEvent(me);
          return true;
        }
        bool keyEvent(TKeyEvent &ke) {
          parent->gedit.keyEvent(ke);
          return true;
        }
        
        TModelLayoutEditor *parent;
    };
    TMyMouseFilter mymousefilter;
    TEventFilter * getFilter();
    TFigureEditor gedit;
    void paint();

    string selectionname;
    TTextModel label;
    TIntegerModel width, height;
    TLabelOwner * labelowner;   

    void enabled();
    void selectionChanged();
    void labelChanged();
    void sizeChanged(); 
    
    class TModelListAdapter:
      public GTableAdapter<TModelLayout::TModelList>
    {
      public:
        TModelListAdapter(TModelLayout::TModelList *aModel):
          GTableAdapter<TModelLayout::TModelList>(aModel) {}
        TModelLayout::TModelList* getModel() const { return model; }
        void modelChanged(bool newmodel) {};
        void tableEvent(TTableEvent &te);
    };
};        

} // namespace toad

#endif
