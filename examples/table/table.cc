#include <toad/toad.hh>
#include <toad/table.hh>
#include <toad/checkbox.hh>
#include <toad/radiobutton.hh>
#include <toad/dialog.hh>
#include <toad/stl/vector.hh>
#include <toad/utf8.hh>

using namespace toad;

class TTableModel:
  public TModel
{
};

struct TPerson {
  TPerson(const string &name, const string &surname) {
    this->name = name;
    this->surname = surname;
  }
  string name;
  string surname;
};

typedef GVector<TPerson> TPeople;
TPeople people;

class TMainWindow:
  public TDialog
{
  public:
    TMainWindow(TWindow *parent, const string &title);
};

class TMyRenderer:
  public TSimpleTableAdapter
{
    TPeople *container;
    
  public:
    TMyRenderer(TPeople *container) {
      this->container = container;
    }
    virtual size_t getRows() { return container->size(); }
    virtual size_t getCols() { return 2; }
    
    void tableEvent(TTableEvent &te);
};

void
TMyRenderer::tableEvent(TTableEvent &te)
{
  switch(te.col) {
    case 0:
      handleString(te, &(*container)[te.row].name);
      break;
    case 1:
      handleString(te, &(*container)[te.row].surname);
      break;
  }
}

TMainWindow::TMainWindow(TWindow *parent, const string &title):
  TDialog(parent, title)
{
  TTable *table1 = new TTable(this, "table1");
  TTable *table2 = new TTable(this, "table2");

  TCheckBox *cb;
  cb = new TCheckBox(this, "rowheader");
  TCLOSURE1(
    cb->getModel()->sigChanged,
    t, table1,
    if (t->getRowHeaderRenderer()) {
      t->setRowHeaderRenderer(0);
    } else {
      t->setRowHeaderRenderer(new TDefaultTableHeaderRenderer);
    }
  )
  cb = new TCheckBox(this, "colheader");
  TCLOSURE1(
    cb->getModel()->sigChanged,
    t, table1,
    if (t->getColHeaderRenderer()) {
      t->setColHeaderRenderer(0);
    } else {
      t->setColHeaderRenderer(new TDefaultTableHeaderRenderer(false));
    }
  )
  cb = new TCheckBox(this, "selcur");
  TCLOSURE3(
    cb->getModel()->sigChanged,
    cb, cb->getModel(),
    t1, table1,
    t2, table2,
    t1->selectionFollowsMouse = cb->getValue();
    t2->selectionFollowsMouse = cb->getValue();
  )

  GRadioStateModel<PAbstractSelectionModel> *rsm = 
    new GRadioStateModel<PAbstractSelectionModel>();
  
  TAbstractSelectionModel *tsm;
  
  rsm->add(new TRadioButton(this, "none"), 0);

  tsm = new TSingleSelectionModel;
  rsm->add(new TRadioButton(this, "single"), tsm);

  tsm = new TSingleSelectionModel;
  tsm->setRowColMode(TRectangleSelectionModel::WHOLE_ROW);
  rsm->add(new TRadioButton(this, "single_row"), tsm);

  tsm = new TSingleSelectionModel;
  tsm->setRowColMode(TRectangleSelectionModel::WHOLE_COL);
  rsm->add(new TRadioButton(this, "single_col"), tsm);

  tsm = new TRectangleSelectionModel;
  rsm->add(new TRadioButton(this, "rectangle"), tsm);

  tsm = new TRectangleSelectionModel;
  tsm->setRowColMode(TRectangleSelectionModel::WHOLE_ROW);
  rsm->add(new TRadioButton(this, "rectangle_row"), tsm);

  tsm = new TRectangleSelectionModel;
  tsm->setRowColMode(TRectangleSelectionModel::WHOLE_COL);
  rsm->add(new TRadioButton(this, "rectangle_col"), tsm);

  tsm = new TSelectionModel;
  rsm->add(new TRadioButton(this, "any"), tsm);

  tsm = new TSelectionModel;
  tsm->setRowColMode(TRectangleSelectionModel::WHOLE_ROW);
  rsm->add(new TRadioButton(this, "any_row"), tsm);

  tsm = new TSelectionModel;
  tsm->setRowColMode(TRectangleSelectionModel::WHOLE_COL);
  rsm->add(new TRadioButton(this, "any_col"), tsm);

  TCLOSURE3(
    rsm->sigChanged,
    model, rsm,
    t1, table1,
    t2, table2,
    t1->setSelectionModel(model->getValue());
    t2->setSelectionModel(model->getValue());
  )

  table1->setAdapter(new TMyRenderer(&people));
  table2->setAdapter(new TMyRenderer(&people));

  loadLayout("layout.atv");
}

int
main(int argc, char **argv, char **envv)
{
  people.push_back(TPerson("Einstein", "Albert"));
  people.push_back(TPerson("Feynmann", "Richard"));
  people.push_back(TPerson("Ritchie", "Dennis M."));
  people.push_back(TPerson("Kernighan", "Brian W."));
  people.push_back(TPerson("Wood", "Ed"));
  people.push_back(TPerson("Chaplin", "Charles"));
  people.push_back(TPerson("Ghandi", "(Mahatma)"));
  people.push_back(TPerson("Kubrick", "Stanley"));
  people.push_back(TPerson("Carpenter", "John"));
  people.push_back(TPerson("Huston", "John"));
  people.push_back(TPerson("Hitchcock", "Alfred"));
  people.push_back(TPerson("Burton", "Tim"));
  
  toad::initialize(argc, argv, envv);
  {
    TMainWindow wnd(0, "TOAD TTable");
    toad::mainLoop();
  }
  toad::terminate();
  return 0;
}
