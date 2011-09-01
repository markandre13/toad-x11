/*
 * Fischland -- A 2D vector graphics editor
 * Copyright (C) 1999-2006 by Mark-André Hopf <mhopf@mark13.org>
 * Visit http://www.mark13.org/fischland/.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "fischland.hh"
#include "toolbox.hh"
#include "colorpalette.hh"
#include "lineal.hh"
#include "page.hh"
#include "config.h"

#ifdef HAVE_LIBCAIRO
#include "cairo.hh"
#endif

#include <toad/toad.hh>
#include <toad/springlayout.hh>
#include <toad/menubar.hh>
#include <toad/undomanager.hh>
#include <toad/action.hh>
#include <toad/fatradiobutton.hh>
#include <toad/pushbutton.hh>
#include <toad/figure.hh>
#include <toad/figureeditor.hh>
#include <toad/colorselector.hh>
#include <toad/filedialog.hh>
#include <toad/combobox.hh>
#include <toad/arrowbutton.hh>
#include <toad/exception.hh>
#include <toad/dnd/color.hh>
#include <toad/dialog.hh>
#include <toad/textfield.hh>
#include <toad/fatcheckbutton.hh>
#include <toad/htmlview.hh>
#include <toad/popupmenu.hh>
#include <fstream>
#include <vector>

#include <stdlib.h>
#include <time.h>
#include <fcntl.h>


// basename on MacOS X
#include <libgen.h>

using namespace toad;
using namespace fischland;

string programname("Fischland");

string datadir("/usr/local/share");
string resourcename("fischland");
string version("snapshot");

struct TPaperSize {
  const char *name;
  double w, h; // cm, 1cm = 0.394in, 2.54cm = 1in
};

TPaperSize papersize[] = {
  { "a0",         84.1   , 118.9   },
  { "a1",         59.4   , 84.1    },
  { "a2",         42.0   , 59.4    },
  { "a3",         29.7   , 42.0    },
  { "a4",         21.0   , 29.7    },
  { "a5",         14.8   , 21.0    },
  { "a6",         10.5   , 14.8    },
  { "a7",         7.4    , 10.5    },
  { "a8",         5.2    , 7.4     },
  { "a9",         3.7    , 5.2     },
  { "a10",        2.6    , 3.7     },
  { "b0",         100.0  , 141.4   },
  { "b1",         70.7   , 100.0   },
  { "b2",         50.0   , 70.7    },
  { "b3",         35.3   , 50.0    },
  { "b4",         25.0   , 35.3    },
  { "b5",         17.6   , 25.0    },
  { "b6",         12.5   , 17.6    },
  { "b7",         8.8    , 12.5    },
  { "b8",         6.2    , 8.8     },
  { "b9",         4.4    , 6.20    },
  { "b10",        3.1    , 4.40    },
  { "archA",      22.86,   30.48   },
  { "archB",      30.48,   45.72   },
  { "archC",      45.72,   60.96   },
  { "archD",      60.96,   91.44   },
  { "archE",      91.44,   121.92  },
  { "flsa",       21.59,   33.02   },
  { "flse",       21.59,   33.02   },
  { "halfletter", 13.97,   21.59   },
  { "note",       19.05,   25.4    },
  { "letter",     21.59,   27.94   },
  { "legal",      21.59,   35.56   },
  { "11x17",      27.94,   43.18   },
  { "ledger",     43.18,   27.94   }
};

struct TAspectRatio
{
  const char *name;
  double ratio;
};

TAspectRatio aspectRatio[] = {
  { "MGM Camera 65 (Ben Hur)", 2.76 },
  { "CinemaScope 55 (Original Cinemascope)", 2.55 },
  { "CinemaScope, Techniscope after 1970 (2.39)", 2.39 },
  { "CinemaScope, Techniscope prior 1970 (2.35)", 2.35 },
  { "North American Widescreen (Academy Flat)", 1.85 },
  { "16:9, HDTV", 1.78 },
  { "European Widescreen, Super 16", 1.66 },
  { "Academy Standard (Edison)", 1.33 }
};

struct TPaperFormat
{
  string papername;
  double pw, ph; // paper size in cm
  string framename;
  double fx, fy, fw, fh;
  bool landscape;
  bool twopages;
  string header;
  string footer;
};

/**
 * 
 *
 */
string
resource(const string &filename)
{
  char *home = getenv("HOME");
  char cwd[4096];
  getcwd(cwd, sizeof(cwd)-1);
  string path;

  for(int i=0; i<3; ++i) {
    switch(i) {
      case 0:
        if (home) {
          path = home;
          path += "/.";
          path += resourcename;
        }
        break;
      case 1:
        path = datadir;
        path += "/";
        path += resourcename;
        break;
      case 2:
        path = cwd;
        path += "/resource";
        break;
      default:
        path.clear();
    }
    path += "/";
    path += filename;
    if (!path.empty()) {
      int fd = open(path.c_str(), O_RDONLY);
      if (fd>=0) {
        close(fd);
        return path;
      }
    }
  }
  return path;
}

// i've put TCollection into it's own namespace so it can be
// included in other programs too
namespace fischland {

class TPage:
  public TSerializable
{
  SERIALIZABLE_INTERFACE(fischland::, TPage);
  
  public:
    TPage() {}
    TPage(const string &name, TFigureModel *model) {
      this->name = name;
      this->model = model;
    }
  
//  protected:
    PFigureModel model;
    TTextModel name;
    TTextModel description;
};

/**
 * This models holds the various figure models which ____ Fischlands
 * image. (Layer's aren't part of this yet, sorry.)
 */
class TCollection:
  public TModel, public TSerializable
{
  SERIALIZABLE_INTERFACE(fischland::, TCollection);

  public:
    void append(TFigureModel *model) {
      storage.push_back(new TPage("unnamed page", model));
    }

//  protected:
    typedef vector<TPage*> TStorage;
    TStorage storage;
    string author;
    string date;
    string description;
};

typedef GSmartPointer<TCollection> PCollection;

} // namespace fischland

using namespace fischland;

void
TPage::store(TOutObjectStream &out) const
{
  ::store(out, "name", name);
  ::store(out, "description", description);
  ::store(out, model);
}

bool
TPage::restore(TInObjectStream &in)
{
  if (in.what == ATV_START) {
    model = 0;
//    name.clear();
//    description.clear();
    return true;
  }
  if (in.what == ATV_GROUP && in.type == "toad::TFigureModel") {
    model = new TFigureModel();
    in.setInterpreter(model);
    return true;
  }
  if (
    ::restore(in, "name", &name) ||
    ::restore(in, "description", &description) ||
    finished(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}

void
TCollection::store(TOutObjectStream &out) const
{
  ::store(out, "author", author);
  ::store(out, "date", date);
  ::store(out, "description", description);
  for(TStorage::const_iterator p = storage.begin();
      p != storage.end();
      ++p)
  {
    ::store(out, *p);
  }
}

bool
TCollection::restore(TInObjectStream &in)
{
  if (in.what == ATV_GROUP && in.type == "fischland::TPage") {
    TPage *page = new TPage();
    storage.push_back(page);
    in.setInterpreter(page);
    return true;
  }
  if (
    ::restore(in, "author", &author) ||
    ::restore(in, "date", &date) ||
    ::restore(in, "description", &description) ||
    finished(in)
  ) return true;
  ATV_FAILED(in)
  return false;
}

class TFischEditor:
  public TFigureEditor
{
    PEditModel editmodel;
    PBitmap cache;
    bool modified2;
    
    // speed up painting:
    TFigure *first_figure, *last_figure;
#ifdef HAVE_LIBCAIRO
    cairo_surface_t *before_selection, *after_selection;
#endif
    
  public:
    typedef TFigureEditor super;
  
    TFischEditor(TWindow *parent, const string &title):
      super(parent, title) { 
      modified2 = false;
      first_figure = last_figure = 0;
#ifdef HAVE_LIBCAIRO
      before_selection = after_selection = 0;
#endif
      quick = false;
    }

    TSignal sigFischModified;
    void clearFischModified() {
      if (modified2) {
        modified2 = false;
        sigFischModified();
      }
    }
    bool isFischModified() const {
      return modified2;
    }
      
    void paint();
    void setEditModel(TEditModel *e) {
      if (editmodel) {
        disconnect(editmodel->sigChanged, this);
        if (editmodel->figuremodel)
          disconnect(editmodel->figuremodel->sigChanged, this);
      }
      editmodel = e;
      setModified(false);

      if (editmodel) {
        CONNECT(editmodel->sigChanged, this, editmodelChanged);
        setModel(editmodel->figuremodel);
        if (editmodel->figuremodel)
          CONNECT(editmodel->figuremodel->sigChanged, this, figuremodelChanged);
      }
      modified2 = false;
    }
    
    void editmodelChanged() {
//cout << __PRETTY_FUNCTION__ << " modified=" << modified << endl;
      if (editmodel) {
//        cout << "set figure model " << editmodel->figuremodel << endl;
        if (getModel())
          disconnect(getModel()->sigChanged, this);
        setModel(editmodel->figuremodel);
        if (getModel())
          CONNECT(getModel()->sigChanged, this, figuremodelChanged);
      } else {
        setModel(0);
      }
      if (!modified2) {
        modified2 = true;
        sigFischModified();
      }
    }
    
    void figuremodelChanged() {
//cout << __PRETTY_FUNCTION__ << " modified=" << modified << endl;
      if (!modified2) {
        modified2 = true;
        sigFischModified();
      }
    }
    
    void toolChanged(TFigureTool *tool);
};

void
TFischEditor::paint()
{
  if (!window) {
    cout << __PRETTY_FUNCTION__ << ": no window" << endl;
    return;
  }
  if (update_scrollbars) {
    // cout << "paint: update_scrollbars" << endl;
    updateScrollbars();
    update_scrollbars = false;
  }

  if (!editmodel || editmodel->modelpath.empty() ) {
    TPen pen(window);
    pen.setColor(TColor::DIALOG);
    pen.fillRectangle(0,0,window->getWidth(), window->getHeight());
    return;
  }

  TPen scr(window);
  scr.identity();  
  TRectangle r;    
  scr.getClipBox(&r);
  
  if (cache &&
      (cache->getWidth() !=window->getWidth() ||
       cache->getHeight()!=window->getHeight()))
  {
//cout << "*** drop cache" << endl;
    cache = 0;
  }
  if (!cache) {
//cout << "*** new cache" << endl;
    cache = new TBitmap(window->getWidth(),window->getHeight(), TBITMAP_SERVER);
    quickready = false;
  }
  
  if (quick && quickready) {
//cout << "*** quick" << endl;
    scr.drawBitmap(0,0, cache);
    scr.translate(window->getOriginX()+visible.x,
                  window->getOriginY()+visible.y);
    if (mat)
      scr.multiply(mat);
    if (getTool())
      getTool()->paintSelection(this, scr);
    else {
      scr.setLineWidth(0);
      paintSelection(scr);
    }
    scr.identity();
    paintDecoration(scr);
  } else {
//cout << "*** normal" << endl;
#ifdef HAVE_LIBCAIRO
    TCairo pen(cache);
#else
    TPen pen(cache);
#endif
    pen.identity();
    pen.setColor(window->getBackground());
    pen.fillRectangle(r.x,r.y,r.w, r.h);
    pen.translate(window->getOriginX()+visible.x,
                  window->getOriginY()+visible.y);

    // draw the paper (fixed by now)
    // assume screen resolution of 100dpi, w/2.54 = w in inch
    TPaperSize *ps = papersize + 4;
    pen.setColor(0,0,0);
    pen.setLineWidth(1);
    double w = ps->w/2.54*100.0*96.0;
    double h = ps->h/2.54*100.0*96.0;
    if (mat)
      mat->map(w, h, &w, &h);
    pen.drawRectangle(0,0, w, h);
    pen.fillRectangle(w,2, 2, h);
    pen.fillRectangle(2,h, w, 2);
    pen.drawString(w+3,h+3, ps->name);

    // pen.drawRectangle(w/9, h/9, w/9*6, h/9*6);

    if (mat)
      pen.multiply(mat);
    paintGrid(pen);
    
    // draw all figure models on all active slides and layers
    for(vector<TFigureModel*>::iterator p = editmodel->modelpath.begin();
        p != editmodel->modelpath.end();
        ++p)
    {
      print(pen, *p, !getTool() && !quick);
      // in case this figuremodel is the one were editing on and there's
      // figure being created (current), paint it now
      if (*p==editmodel->figuremodel &&
          getCurrent())
      {
        getCurrent()->paint(pen);
      }
    }

    pen.setLineWidth(0);
    if (!quick) {
      if (getTool())
        getTool()->paintSelection(this, pen);
      else
        paintSelection(pen);
    }
    scr.drawBitmap(0,0, cache);
    if (quick) {
      scr.identity();
      scr.translate(window->getOriginX()+visible.x,
                    window->getOriginY()+visible.y);
      if (getTool()) {
        if (mat)
          scr.multiply(mat);
        scr.setLineWidth(0);
        getTool()->paintSelection(this, scr);
      } else {
        scr.setLineWidth(0);
        paintSelection(scr);
      }
      scr.identity();
      quickready = true;
    }
    paintDecoration(scr);
  }
}

class TToolOptions;

static TToolOptions *toolOptionsWindow = 0;

class TToolOptions:
  public TWindow
{
    TWindow *editor;
    string defaulttitle;
  public:
    TToolOptions(TWindow *p, const string &t):TWindow(p, t)
    {
      editor = 0;
      defaulttitle = t;
      setSize(320, 5);
      bStaticFrame = true;
      bParentlessAssistant = true;
      setBackground(TColor::DIALOG);
    }
    ~TToolOptions() {
      toolOptionsWindow = 0;
    }
    void closeRequest() {
      delete this;
    }
    void toolChanged(TFigureTool *tool);
};


void
TFischEditor::toolChanged(TFigureTool *tool)
{
  cout << "new tool is " << tool << endl;
  if (!toolOptionsWindow)
    return;
  toolOptionsWindow->toolChanged(tool);
}

void
TToolOptions::toolChanged(TFigureTool *tool)
{
  cout << "new tool is " << tool << endl;
  if (editor) {
    delete editor;
    editor = 0;
  }
  
  if (!tool) {
    setTitle(defaulttitle);
    setSize(320, 5);
    return;
  }
      
  editor = tool->createEditor(this);
  if (!editor) {
    setTitle(defaulttitle);
    setSize(320, 5);
    return;
  }
  setTitle(defaulttitle + ": " + editor->getTitle());

  editor->setPosition(2,2);
  editor->createWindow();
  setSize(editor->getWidth()+6, editor->getHeight()+6);
}

class TMainWindow:
  public TWindow
{
    typedef TWindow super;
    typedef TMainWindow This;
    string filename;
    PEditModel editmodel;
    TFischEditor *editor;
    TSingleSelectionModel currentPage;
    
    bool _check();
    bool _save(const string &title);

  public:
    TMainWindow(TWindow *parent, const string &title, TEditModel *m=0);
    ~TMainWindow();
    
    void load(const string &filename);
    
    void menuNew();
    void menuNewView();
    void menuClose();
    
    void menuOpen();
    bool menuSave();
    bool menuSaveAs();
    void menuPrint();
    void menuAbout();
    void menuCopyright();
    
    void menuSlides();
    void menuLayers();
    void menuToolOptions();

    void closeRequest();
    
    void changeZoom(TFigureEditor *gw, TComboBox*);
    
    void setEditModel(TEditModel*);
    TEditModel *getEditModel() const {
      return editmodel;
    }
    TSignal sigEditModel;
    
    TPushButton *page_add, *page_del, *page_up, *page_down, *page_edit;

    void pageAdd();
    void pageDelete();
    void pageUp();
    void pageDown();
    void pageEdit();
    void updatePageButtons();
    
    void _gotoPage(int page);

    TEditModel* newEditModel();
    void _fixLineWidth();
    void editorModified();
};

/**
 * kludge for old files using a line width of 0 to 12 instead of 96-12*96
 */
static void
_fixLineWidth(TFigureModel::iterator b, TFigureModel::iterator e)
{
  for(TFigureModel::iterator p1 = b; p1 != e; ++p1) {
    TFGroup *g = dynamic_cast<TFGroup*>(*p1);
    if (g) {
      _fixLineWidth(g->gadgets.begin(), g->gadgets.end());
      continue;
    }
    TColoredFigure *cf = dynamic_cast<TColoredFigure*>(*p1);
    if (cf) {
      TFigureAttributes a;
      cf->getAttributes(&a);
      if (a.linewidth<15) {
        if (a.linewidth==0)
          a.linewidth=48;
        else
          a.linewidth*=96;
        a.reason = TFigureAttributes::LINEWIDTH;
        cf->setAttributes(&a);
      }
    }
  }
}

void
TMainWindow::_fixLineWidth()
{
#if 1
  cout << __PRETTY_FUNCTION__ << " is disabled\n";
#else
  if (!collection)
    return;
    
  for(vector<TPage*>::iterator p0 = collection->storage.begin();
      p0 != collection->storage.end();
      ++p0)
  {
    ::_fixLineWidth((*p0)->model->begin(), (*p0)->model->end());
  }    
#endif
}

void
TMainWindow::_gotoPage(int page)
{
#if 1
  cout << "TMainWindow::_gotoPage(" << page << ") is disabled\n";
#else
  TFigureModel *m = 0;
  if (!collection->storage.empty())
    m = collection->storage[/*page*/currentPage.getRow()]->model;
  editor->setModel(m);
  updatePageButtons();
#endif
}

void
TMainWindow::pageAdd()
{
#if 1
  cout << "TMainWindow::pageAdd() is disabled\n";
#else
  TPage *page = new TPage("new page", new TFigureModel());
  int n = currentPage.getRow() + 1;
  TCollection::TStorage::iterator p = collection->storage.begin() + n;
  collection->storage.insert(p, page);
  sigCollection();
  currentPage.setSelection(0, n);
  updatePageButtons();
  pageEdit();
#endif
}

void
TMainWindow::pageDelete()
{
#if 1
  cout << "TMainWindow::pageDelete() is disabled\n";
#else
  if (collection->storage.size()<=1)
    return;
  int n = currentPage.getRow();
  TCollection::TStorage::iterator p = collection->storage.begin() + n;
  collection->storage.erase(p);
  sigCollection();
  if (n>=collection->storage.size())
    n = collection->storage.size()-1;
  currentPage.setSelection(0, n);
  updatePageButtons();
#endif
}

void
TMainWindow::pageUp()
{
#if 1
  cout << "TMainWindow::pageUp() is disabled\n";
#else
  if (collection->storage.size()<=1)
    return;
  int n = currentPage.getRow();
  if (n<=0)
    return;
  TPage *memo = collection->storage[n];
  collection->storage[n] = collection->storage[n-1];
  collection->storage[n-1] = memo;
  sigCollection();
  currentPage.setSelection(0, n-1);
  updatePageButtons();
#endif
}

void
TMainWindow::pageDown()
{
#if 1
  cout << "TMainWindow::pageDown() is disabled\n";
#else
  if (collection->storage.size()<=1)
    return;
  int n = currentPage.getRow();
  if (n>=collection->storage.size()-1)
    return;
  TPage *memo = collection->storage[n];
  collection->storage[n] = collection->storage[n+1];
  collection->storage[n+1] = memo;
  sigCollection();
  currentPage.setSelection(0, n+1);
  updatePageButtons();
#endif
}

void
TMainWindow::pageEdit()
{
#if 1
  cout << "TMainWindow::pageEdit() is disabled\n";
#else
  TDialog *dlg = new TDialog(this, "Edit Page");
  new TTextField(dlg, "page name", 
                 &collection->storage[currentPage.getRow()]->name);
  new TTextArea(dlg, "page description",
                &collection->storage[currentPage.getRow()]->description);
  connect((new TPushButton(dlg, "ok"))->sigClicked, dlg, &TDialog::closeRequest);
  dlg->loadLayout(RESOURCE("editpage.atv"));
  dlg->doModalLoop();
  delete dlg;
#endif
}

void
TMainWindow::updatePageButtons()
{
#if 1
  cout << "TMainWindow::updatePageButtons() is disabled\n";
#else
  page_del->setEnabled(collection->storage.size()>1);
  page_up->setEnabled(currentPage.getRow()>0);
// cerr << "current page = " << currentPage.getRow() << endl;
  page_down->setEnabled(currentPage.getRow()<collection->storage.size()-1);
#endif
}

#if 0
class TCollectionRenderer:
  public TSimpleTableAdapter
{
    int width, height;
    TMainWindow *wnd;
  public:
    TCollectionRenderer(TMainWindow *w) {
      wnd = w;
      TFont &font(TOADBase::getDefaultFont());
      height = font.getHeight();
      width = 0;
      adjust();
      connect(wnd->sigCollection, this, &TCollectionRenderer::adjust);
    }
    
    ~TCollectionRenderer() {
      disconnect(wnd->sigCollection, this);
    }
    
    void adjust()
    {
      TFont &font(TOADBase::getDefaultFont());
      width = 0;
      if (!wnd->getCollection()) {
        sigChanged();
        return;
      }
      for(TCollection::TStorage::iterator p = wnd->getCollection()->storage.begin();
          p != wnd->getCollection()->storage.end();
          ++p)
      {
        int w = font.getTextWidth( (*p)->name );
        if (w>width)
          width = w;
      }
      sigChanged();
    }
    size_t getRows() { 
      return wnd->getCollection() ?
        wnd->getCollection()->storage.size() :
        0; 
    }
    int getRowHeight(size_t row) { return height; }
    int getColWidth(size_t col) { return width; }
    void renderItem(TPen &pen, const TTableEvent &te)
    {
      if (!wnd->getCollection()) {
        cerr << "hey? TTable/TComboBox tried to render non-existent collection in row " << te.row << endl;
        return;
      }
      if (wnd->getCollection()->storage.size()<=te.row) {
        cerr << "hey? TTable/TComboBox tried to render non-existent row " << te.row << endl;
        return;
      }
      pen.drawString(0,0,wnd->getCollection()->storage[te.row]->name);
    }
};
#endif

void
TMainWindow::menuNew()
{
  if (!_check())
    return;
    
  filename.erase();
  setTitle(programname);
  setEditModel(newEditModel());
}

void
TMainWindow::menuNewView()
{
  TMainWindow *mw = new TMainWindow(NULL, programname, editmodel);
  mw->placeWindow(mw, PLACE_PARENT_RANDOM, this);
  mw->setTitle(programname+ ": " + basename((char*)filename.c_str()));
  mw->filename = filename;
  mw->createWindow();
}

bool
TMainWindow::_check()
{
  if (editor->isModified() && editmodel->_toad_ref_cntr<=2) {
    unsigned r = messageBox(NULL,
      "Figure is modified",
      "Do you want to save the current file?",
      TMessageBox::ICON_QUESTION |
      TMessageBox::YES | TMessageBox::NO );
    if (r==TMessageBox::YES) {
      if (!menuSave())
        return false;
    } else if (r!=TMessageBox::NO) {
      return false;
    }
  }
  return true;
}

void
TMainWindow::menuClose()
{
  
}

void
TMainWindow::menuOpen()
{
  if (!_check())
    return;

  TFileDialog dlg(this, "Open..");
  dlg.addFileFilter("Fischland (*.atv, *.vec, *.fish)");
  // dlg.addFileFilter("Scaleable Vector Graphics (*.svg)");
  dlg.doModalLoop();
  if (dlg.getResult()!=TMessageBox::OK)
    return;
  load(dlg.getFilename());
}

void
TMainWindow::load(const string &filename)
{    
  ifstream fin(filename.c_str());
  TInObjectStream in(&fin);
  //  oin.setVerbose(true);
  //  oin.setDebug(true);

  TSerializable *s = in.restore();
  if (!in || !s) {
    string msg =
      programname + " failed to load the object.\n\n" +
      in.getErrorText();
      messageBox(0, 
               "Failed to load file",
               msg,
               TMessageBox::ICON_STOP | TMessageBox::OK);
    return;
  }

  TFigureModel *figuremodel;
  TCollection *collection;
  TDocument *document;
  string msg;

  figuremodel = dynamic_cast<TFigureModel *>(s);
  if (figuremodel) {
    cout << "found old TFigureModel object" << endl;
    goto done;
  }

  collection = dynamic_cast<TCollection*>(s);
  if (collection) {
    cout << "found old TCollection object" << endl;
    document = new TDocument();
    TSlide *slide = 0;
    for(TCollection::TStorage::iterator p = collection->storage.begin();
        p != collection->storage.end();
        ++p)
    {
      if (!slide) {
        slide = new TSlide;
        document->content.setRoot(slide);
      } else {
        slide->next = new TSlide;
        slide = slide->next;
      }
      TLayer *layer = new TLayer;
      slide->content.setRoot(layer);
      slide->name = (*p)->name;
      slide->comment = (*p)->description;
      for(TFigureModel::iterator q = (*p)->model->begin();
          q != (*p)->model->end();
          ++q)
      {
        layer->content.add(*q);
      }
      (*p)->model->drop();
      slide->content.update();
    }
    document->content.update();
    editmodel->setDocument(document);
    goto done;
  }
  
  document = dynamic_cast<TDocument*>(s);
  if (document) {
    cout << "found document!" << endl;
    editmodel->setDocument(document);
    editor->clearFischModified();
    goto done;
  }

  delete s;
  msg = programname + " can't load objects of type '" +
        s->getClassName() + "'.\n\n"
        "I've expected either 'fischland::TDocument', 'fischland::TCollection' or "
        "'toad::TFigureModel'.";
  messageBox(0, "Failed to load file", msg,
             TMessageBox::ICON_STOP | TMessageBox::OK);
  return;

done:
  _fixLineWidth();

  this->filename = filename;
  setTitle(programname+ ": " + basename((char*)filename.c_str()));
}

void
TMainWindow::setEditModel(TEditModel *e)
{
//cerr << "enter TMainWindow::setCollection" << endl;
//cerr << "  remove model from editor" << endl;
//  editor->setModel(0);
//cerr << "  set collection object" << endl;
  editmodel = e;
  editor->setEditModel(e);
//cerr << "  goto page 0" << endl;
//  currentPage.setSelection(0,0);
//  _gotoPage(0);
//cerr << "  trigger sigCollection" << endl;
//  sigCollection();
//  updatePageButtons();
//cerr << "leave TMainWindow::setCollection" << endl;
}

bool
TMainWindow::menuSave()
{
  if (filename.empty()) {
    return menuSaveAs();
  }
  _save("Save");
  return true;
}

bool
TMainWindow::menuSaveAs()
{
  TFileDialog dlg(this, "Save As..", TFileDialog::MODE_SAVE);
  dlg.addFileFilter("Fischland (*.atv, *.vec, *.fish)");
  dlg.setFilename(filename);
  dlg.doModalLoop();
  if (dlg.getResult()==TMessageBox::OK) {
    filename = dlg.getFilename();
    if (_save("Save As..")) {
      setTitle(programname+ ": " + basename((char*)filename.c_str()));
      return true;
    }
  }
  return false;
}

bool
TMainWindow::_save(const string &title)
{
  ofstream out(filename.c_str());
  if (!out) {
    messageBox(NULL,
               title,
               "Damn! I've failed to create the file.",
               TMessageBox::ICON_EXCLAMATION | TMessageBox::OK);
    return false;
  }
  out << "// fish -- a Fischland 2D Vector Graphics file" << endl
      << "// Please see http://www.mark13.org/fischland/ for more details." << endl;
  TOutObjectStream oout(&out);
  oout.store(editmodel->document);
  editor->clearFischModified();
  return true;
}

void
printLayer(TPenBase &pen, TLayer *layer)
{
  while(layer) {
    if (layer->print) {
      for(TFigureModel::iterator p = layer->content.begin();
          p != layer->content.end();
          ++p)
      {
        if ((*p)->mat) {
          pen.push();   
          pen.multiply( (*p)->mat );
          (*p)->paint(pen, TFigure::NORMAL);
          pen.pop();
        } else {
          (*p)->paint(pen, TFigure::NORMAL);
        }
      }
    }
    printLayer(pen, layer->down);
    layer = layer->next;
  }
}

void
printSlide(TPenBase &pen, TSlide *slide)
{
  while(slide) {
    if (slide->print) {
      printLayer(pen, slide->content.getRoot());
      pen.showPage();
    }
    printSlide(pen, slide->down);
    slide = slide->next;
  }
}

void
TMainWindow::menuPrint()
{
/*
  class W: 
    public TWindow
  {
  public:
    PEditModel editmodel;
    W(TWindow *p, const string &t, TEditModel *m): TWindow(p, t) {editmodel=m;}
    void paint() {
  // current page
*/
#ifdef HAVE_LIBCAIRO
  TCairo pen(/*this*/ "output.pdf");
  pen.scale(1.0/(96.0), 1.0/(96.0));
  pen.setLineWidth(96.0);

  // all pages
  printSlide(pen, editmodel->document->content.getRoot());
#endif
/*
    }
  };
  
  TWindow *w = new W(0, "print", editmodel);
  w->createWindow();
*/
}

void
TMainWindow::closeRequest()
{
  if (_check())
    sendMessageDeleteWindow(this);
}

static PBitmap bmp_vlogo;

void
TMainWindow::menuAbout()
{
  THTMLView *htmlview = new THTMLView(0, "Fischland -- About");
  TPopupMenu *menu = new TPopupMenu(htmlview, "popupmenu");
  menu->addFilter();
  htmlview->open(RESOURCE("index.html"));
  htmlview->createWindow();
}

void
TMainWindow::menuCopyright()
{
  messageBox(
    this, 
    "Fischland -- Copyright",
    "Fischland v0.7 -- A 2D vector graphics editor\n"
    "\n"
    "Copyright (C) 2003-2007 by Mark-André Hopf <mhopf@mark13.org>\n"
    "Visit http://mark13.org/fischland/.\n"
    "\n"
    "Pencil curve smoothing code was taken from Xara LX,\n"
    "Copyright (C) 1993-2006 Xara Group Ltd. and written by Rik Heywood.\n"
    "\n"
    "This program is free software; you can redistribute it and/or modify "
    "it under the terms of the GNU General Public License as published by "
    "the Free Software Foundation; either version 2 of the License, or "
    "(at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License "
    "along with this program; if not, write to the Free Software "
    "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA",
    TMessageBox::OK,
    bmp_vlogo);
}

// extern void createMemoryFiles();
struct TZoom
{
  const char *text;
  double factor;
  const char * toText(unsigned) const { return text; }
};

// for this table we assume a screen resolution of 100dpi and a
// maximum resolution of 9600 dpi for our image files
TZoom zoom[13] = {
  { "12.5%", 0.125/96.0 },
  { "25%",  0.25/96.0 },
  { "50%",  0.50/96.0 },
  { "75%",  0.75/96.0 },
  { "100%", 1.0/96.0 },
  { "150%", 1.5/96.0 },
  { "200%", 2.0/96.0 },
  { "300%", 3.0/96.0 },
  { "600%", 6.0/96.0 },
  { "1200%", 12.0/96.0 },
  { "2400%", 24.0/96.0 },
  { "4800%", 48.0/96.0 },
  { "9600%", 1.0 }
};

class TZoomAdapter:
  public TSimpleTableAdapter
{
  public:
    size_t getRows() { return 13; }
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = TOADBase::getDefaultFont().getTextWidth("9600%");
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = TOADBase::getDefaultFont().getHeight();
          break;
        case TTableEvent::PAINT:
          renderBackground(te);
          te.pen->drawString(0,0,zoom[te.row].text);
          // renderCursor(te);
          break;
      }
    }
};

static void
gotoPage(TMainWindow *wnd, TComboBox *cb)
{
  // wnd->_gotoPage(cb->getLastSelectionRow());
}

void
TMainWindow::editorModified()
{
  if (editor->isFischModified()) {
    setTitle(programname+ ": *" + basename((char*)filename.c_str()));
  } else {
    setTitle(programname+ ": " + basename((char*)filename.c_str()));
  }
}

TMainWindow::TMainWindow(TWindow *p, const string &t, TEditModel *e):
  super(p, t)
{
  new TUndoManager(this, "undomanager");

  TFischEditor *me = new TFischEditor(this, "figureeditor");
  CONNECT(me->sigFischModified, this, editorModified);
  editor = me;
  me->setAttributes(TToolBox::preferences);
  me->setBackground(255,255,255);
  me->setRowHeaderRenderer(new TLineal(true));
  me->setColHeaderRenderer(new TLineal(false));
  me->setFont("arial,helvetica,sans-serif:size=1152"); // 12pt * 9600 dpi / 100

  TMenuBar *mb = new TMenuBar(this, "menubar");
  mb->loadLayout(RESOURCE("menubar.atv"));

  TAction *a;
  a = new TAction(this, "file|new");
  CONNECT(a->sigClicked, this, menuNew);
  a = new TAction(this, "file|newview");
  CONNECT(a->sigClicked, this, menuNewView);
  
  a = new TAction(this, "file|open");
  CONNECT(a->sigClicked, this, menuOpen);
  a = new TAction(this, "file|save");
  CONNECT(a->sigClicked, this, menuSave);
  a = new TAction(this, "file|save_as");
  CONNECT(a->sigClicked, this, menuSaveAs);
  a = new TAction(this, "file|print");
  CONNECT(a->sigClicked, this, menuPrint);
  a = new TAction(this, "file|close");
  CONNECT(a->sigClicked, this, menuClose);
  a = new TAction(this, "file|quit");
  CONNECT(a->sigClicked, this, closeRequest);
  
  a = new TAction(this, "windows|tooloption");
  TCLOSURE(
    a->sigClicked,
    if (toolOptionsWindow) {
      if (!toolOptionsWindow->isRealized())
        toolOptionsWindow->createWindow();
      toolOptionsWindow->raiseWindow();
    } else {
      toolOptionsWindow = new TToolOptions(0, "Tool Options");
      toolOptionsWindow->createWindow();
      toolOptionsWindow->toolChanged(TToolBox::preferences->getTool());
    }
  )
  
  a = new TAction(this, "edit|insert_image");
  TCLOSURE1(
    a->sigClicked,
    gw, me,
    gw->addFigure(new TFImage());
  )

  a = new TAction(this, "windows|slides");
  CONNECT(a->sigClicked, this, menuSlides);

  a = new TAction(this, "windows|layers");
  CONNECT(a->sigClicked, this, menuLayers);

  a = new TAction(this, "help|manual");
  CONNECT(a->sigClicked, this, menuAbout);
  a = new TAction(this, "help|about");
  CONNECT(a->sigClicked, this, menuCopyright);
  
  setBackground(TColor::DIALOG);
  setSize(640,480);

  TComboBox *zoom;
  zoom = new TComboBox(this, "zoom");
  zoom->setSize(64, TSIZE_PREVIOUS);
  zoom->setAdapter(new TZoomAdapter);
  connect(zoom->sigSelection, this, &This::changeZoom, me, zoom);
  zoom->setCursor(0,4);
  zoom->clickAtCursor();

  TComboBox *page;
  page = new TComboBox(this, "page");
  page->setSelectionModel(&currentPage);
  connect(currentPage.sigChanged, gotoPage, this, page);
  page->setSize(150, TSIZE_PREVIOUS);
#if 0
  page->setAdapter(new TCollectionRenderer(this));
#endif
  me->setGrid(4*96);
  
  TCoord h = page->getHeight();

  page_add  = new TPushButton(this, "page add");
  page_add->setLabel("+");
  page_add->setToolTip("insert a new page after this one");
  page_add->setSize(h,h);
  CONNECT(page_add->sigClicked, this, pageAdd);

  page_del  = new TPushButton(this, "page delete");
  page_del->setLabel("-");
  page_del->setToolTip("delete this page");
  page_del->setSize(h,h);
  CONNECT(page_del->sigClicked, this, pageDelete);

  page_up   = new TPushButton(this, "page up");
  page_up->setLabel("u");
  page_up->setToolTip("move this page up");
  page_up->setSize(h,h);
  CONNECT(page_up->sigClicked, this, pageUp);

  page_down = new TPushButton(this, "page down");
  page_down->setLabel("d");
  page_down->setToolTip("move this page down");
  page_down->setSize(h,h);
  CONNECT(page_down->sigClicked, this, pageDown);

  page_edit = new TPushButton(this, "page edit");
  page_edit->setLabel("e");
  page_edit->setToolTip("edit page name");
  page_edit->setSize(h,h);
  CONNECT(page_edit->sigClicked, this, pageEdit);

  if (!e)
    e = newEditModel();
    
  setEditModel(e);
  
  loadLayout(RESOURCE("TMainWindow.atv"));
}

TMainWindow::~TMainWindow()
{
}

TEditModel*
TMainWindow::newEditModel()
{
  TEditModel *e = new TEditModel;
  TDocument *doc = new TDocument();
  doc->content.addBelow(0);            // add a slide
  doc->content[0].content.addBelow(0); // add a layer
  e->setDocument(doc);
  e->slide.select(0,0);
  e->layer.select(0,0);
  e->figuremodel = &doc->content[0].content[0].content;
  e->modelpath.push_back(e->figuremodel);
  return e;
}

void
TMainWindow::changeZoom(TFigureEditor *fe, TComboBox *cb)
{
  unsigned i = cb->getSelectionModel()->getRow();
  fe->identity();
  fe->scale(zoom[i].factor, zoom[i].factor);
  TCoord foobar = 0.5 / zoom[i].factor;
  fe->translate(foobar, foobar);
}

void
TMainWindow::menuSlides()
{
  if (!editmodel) {
    cout << "no edit model" << endl;
    return;
  }
  TPageDialog *dlg = new TPageDialog(this, "Slides", editmodel, true);
  dlg->createWindow();
}

void
TMainWindow::menuLayers()
{
  if (!editmodel) {
    cout << "no edit model" << endl;
    return;
  }
  TPageDialog *dlg = new TPageDialog(this, "Layers", editmodel, false);
  dlg->createWindow();
}

#if 0
class TTest:
  public TWindow
{
  public:
    TTest(TWindow *p, const string &t): TWindow(p, t) {}
    void paint();
};

void
TTest::paint()
{
{
  TCairo pen("test.pdf");
  double x = 1.0/96.0;
  pen.setColor(0,0,255);
  pen.drawRectangle(0,0,320,200);
  pen.drawString(10,10, "Hallo");
  pen.scale(x, x);
  pen.setLineWidth(96.0);
  pen.setColor(255,0,0);
  pen.drawRectangle(5,5,20,20);
  pen.setColor(0,255,0);
  pen.drawCircle(30,30,20,20);
  pen.drawString(50,50, "Würze Dein GUI!");
  pen.drawLine(0,0,320,200);
  pen.setColor(0,0,0);
  pen.drawRectangle(5,5,310,190);
  pen.fillRectangle(0,0,5*96,5*96);
  pen.fillRectangle(315,195,5,5);
  pen.showPage();
} exit(0);
}
#endif

TCursor *fischland::cursor[16];

class TResourceFile
{
    string filename;
    string comment; 
  public:
  
    TFileDialog::TResource *resource;
  
    TResourceFile(const string &filename, const string &comment);
    ~TResourceFile();
};
  
TResourceFile::TResourceFile(const string &filename, const string &comment)
{
cerr << "reading resource file '" << filename << "'\n";
  this->filename = filename;
  this->comment  = comment; 
  
  ifstream in(filename.c_str());
  if (!in) {
    cerr << "failed to open resource file for reading\n";
    resource = new TFileDialog::TResource;
    return;
  }
   
#if 0
  TObjectStore os;
  os.registerObject(new TResource);
  TInObjectStream is(&in, &os);
#else
  atv::getDefaultStore().registerObject(new TFileDialog::TResource);
  TInObjectStream is(&in);
#endif
  
  TSerializable *s = is.restore();
  if (!s || !in) {
    cerr << "failed to restore the resource:\n" << is.getErrorText() << endl;
    if (s)
      delete s;
    resource = new TFileDialog::TResource;
    return;
  }
  resource = dynamic_cast<TFileDialog::TResource*>(s);
  if (!resource) {
    cerr << "resource wasn't the right type\n";
    delete s;
    resource = new TFileDialog::TResource;
  }
}  
   
TResourceFile::~TResourceFile()
{
cerr << "writing resource file '" << filename << "'\n";
  ofstream out(filename.c_str());
  if (!out) {
    cerr << "failed to open resource file for writing\n";
    return;
  }
   
  out << "// " << comment;
  TOutObjectStream oout(&out);
  oout.store(resource);
  out << endl;
}

int 
main(int argc, char **argv, char **envv)
{
#if 0
  TCairo pen("test.pdf");
  pen.setFont("futura:size=12");
  pen.drawString(10,10, "futura:size=12");
  pen.setFont("Hiragino Kaku Gothic Pro W3:size=12");
  pen.drawString(10,30, "Hiragino Kaku Gothic Pro W3:size=12");
  pen.setFont("Comic Sans MS:size=12");
  pen.drawString(10,50, "Comic Sans MS:size=12");
  pen.setFont("Free Serif:size=12");
  pen.drawString(10,70, "Free Serif:size=12");
  pen.showPage();
  return 0;
#endif

#if 1
  toad::initialize(argc, argv, envv);
  argc = 1;
#else
  toad::initialize(1, argv, envv);
#endif
//    createMemoryFiles();
  toad::getDefaultStore().registerObject(new TPage());
  toad::getDefaultStore().registerObject(new TCollection());

  toad::getDefaultStore().registerObject(new TDocument());
  toad::getDefaultStore().registerObject(new TSlide());
  toad::getDefaultStore().registerObject(new TLayer());

  string rcfilename = getenv("HOME");
  rcfilename+="/.fischlandrc";

  TResourceFile rc(rcfilename, "Fischland Resource File");

  bmp_vlogo = new TBitmap();
  bmp_vlogo->load(RESOURCE("logo_vertical.jpg"));

  static const char bm[6][32][32+1] = {
    {
    // 0        1         2         3
    // 12345678901234567890123456789012
      "    .                           ",
      "    .                           ",
      "   ...                          ",
      "   ...                          ",
      "  .#.#.                         ",
      "  .#.#.                         ",
      " .##.##.                        ",
      " .##.##.                        ",
      ".###.###.                       ",
      ".#######.                       ",
      ".###.###.                       ",
      ".#######.                       ",
      " .#####.                        ",
      ".........                       ",
      ".........                       ",
      "#.......#                       ",
      "#.......#                       ",
      "#.......#                       ",
      "#.......#                       ",
      "#.......#                       ",
      "#.......#                       ",
      " #######                        ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                "
    },{
    // 0        1         2         3
    // 12345678901234567890123456789012
      "    .                           ",
      "    .                           ",
      "   ...                          ",
      "   ...                          ",
      "  .#.#.                         ",
      "  .#.#.                         ",
      " .##.##.                        ",
      " .##.##.                        ",
      ".###.###.                       ",
      ".#######.                       ",
      ".###.###.                       ",
      ".#######.                       ",
      " .#####.                        ",
      ".........                       ",
      ".........                       ",
      "#.......#                       ",
      "#.......#                       ",
      "#.......#   ...                 ",
      "#.......#  .###.                ",
      "#.......# .#   #.               ",
      "#.......# .#   #.               ",
      " #######  .#   #.               ",
      "           .###.                ",
      "            ...                 ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                "
    },{
    // 0        1         2         3
    // 12345678901234567890123456789012
      "    .                           ",
      "    .                           ",
      "   ...                          ",
      "   ...                          ",
      "  .#.#.                         ",
      "  .#.#.                         ",
      " .##.##.                        ",
      " .##.##.                        ",
      ".###.###.                       ",
      ".#######.                       ",
      ".###.###.                       ",
      ".#######.                       ",
      " .#####.                        ",
      ".........                       ",
      ".........                       ",
      "#.......#                       ",
      "#.......# #                     ",
      "#.......##.#                    ",
      "#.......##..#                   ",
      "#.......##.#.#                  ",
      "#.......##.##.#                 ",
      "##########.# #.#                ",
      "         #.#  #                 ",
      "         #.#                    ",
      "          #                     ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                "
    },{
    // 0        1         2         3
    // 12345678901234567890123456789012
      " #                              ",
      "#.#                             ",
      "#..#                            ",
      "#...#                           ",
      "#....#                          ",
      "#.....#                         ",
      "#......#                        ",
      "#.......#                       ",
      "#........#                      ",
      "#.........#                     ",
      "#....#######                    ",
      "#...#                           ",
      "#..#                            ",
      "#.#                             ",
      "##                              ",
      "#                               ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                "
    },{
    // 0        1         2         3
    // 12345678901234567890123456789012
      "                ####            ",
      "               #....#           ",
      "              #.####.#          ",
      "             #..#####.#         ",
      "            #....####.#         ",
      "           #.##......#          ",
      "          #.####....#           ",
      "         #.#######.#            ",
      "        #.#######.#             ",
      "       #.#######.#              ",
      "      #.#######.#               ",
      "     #.#######.#                ",
      "    #.#######.#                 ",
      "   #.#######.#                  ",
      "  #.#######.#    ...            ",
      " #.#######.#    .###.           ",
      "#..######.#    .#   #.          ",
      "#.#..###.#     .#   #.          ",
      "#.###...#      .#   #.          ",
      "#..###.#        .###.           ",
      "#.....#          ...            ",
      "######                          ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                "
    },{
    // 0        1         2         3
    // 12345678901234567890123456789012
      "                ####            ",
      "               #....#           ",
      "              #.####.#          ",
      "             #..#####.#         ",
      "            #....####.#         ",
      "           #.##......#          ",
      "          #.####....#           ",
      "         #.#######.#            ",
      "        #.#######.#             ",
      "       #.#######.#              ",
      "      #.#######.#               ",
      "     #.#######.#                ",
      "    #.#######.#                 ",
      "   #.#######.#                  ",
      "  #.#######.#                   ",
      " #.#######.#                    ",
      "#..######.#                     ",
      "#.#..###.#                      ",
      "#.###...#                       ",
      "#..###.#                        ",
      "#.....#                         ",
      "######                          ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                ",
      "                                "
    }
  };
  cursor[0] = new TCursor(bm[0], 4, 0);
  cursor[1] = new TCursor(bm[1], 4, 0);
  cursor[2] = new TCursor(bm[2], 4, 0);
  cursor[3] = new TCursor(bm[3], 0, 0);
  cursor[4] = new TCursor(bm[4], 0, 21);
  cursor[5] = new TCursor(bm[5], 0, 21);

#if 1
  new TToolBox(0, programname);
  if (argc==1) {
    new TMainWindow(0, programname);
  } else {
    for(int i=1; i<argc; ++i) {
      TMainWindow *wnd = new TMainWindow(0, programname);
      wnd->load(argv[i]);
    }
  }
#else
  new TTest(0, "test");
#endif
  toad::mainLoop();
  bmp_vlogo = 0;
  toad::terminate();
  return 0;
}

