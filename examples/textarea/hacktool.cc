/*
  toe -- TOAD's Own Editor
  Copyright (C) 1999-2004 by Mark-André Hopf <mhopf@mark13.org>

  - the keypad isn't available in TTextArea
  - Some of the stuff in TMainWindow should be moved into a special class.
  - Use file locking to avoid some of the troubles I've had with joe
  - multiple files
  - font selection
  - session management
  - the filedialog should stay in the last recently used directory
    and offer a list of recently accessed directories
  - the message for `save file?' on exit states `before loading'
*/

#include <toad/toad.hh>
#include <toad/form.hh>
#include <toad/menubar.hh>
#include <toad/undomanager.hh>
#include <toad/action.hh>
#include <toad/filedialog.hh>
#include <toad/dialog.hh>
#include <toad/pushbutton.hh>
#include <toad/textfield.hh>
#include <toad/checkbox.hh>
#include <toad/radiobutton.hh>
#include <toad/io/binstream.hh>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <libgen.h>
#include <sys/stat.h>
#include <signal.h>

#include <fstream>
#include <sstream>

#include <toad/textarea.hh>

using namespace toad;

#define RESOURCE(file) "memory://" file

static const string programname("toe");
static string fontname("monospace:size=11");

class TMainWindow;

class TStatusBar:
  public TWindow
{
  public:
    TStatusBar(TWindow *, const string&, TMainWindow*, TTextArea*);
    
    void paint();
    void mouseLDown(int,int,unsigned);
    
    void update();
    void rowChanged();
    
  protected:
    bool lock;
    TIntegerModel line;    // current line
    TMainWindow *main;
    TTextArea *ta;              // textarea in TMainWindow

    TTextField  *txt_row;
    int ypos;                   // ypos for text
    int xpos_row;               // xpos for `Row:' text
};

class TMainWindow:
  public TForm
{
    typedef TForm super;
  public:
    TMainWindow(TWindow *parent, const string &title);
    TMainWindow(TWindow *parent, const string &title, const string &filename);
    TMainWindow(TWindow *parent, const string &title, TTextModel*);
    ~TMainWindow();
    
    void menuNew();
    void menuNewView();
    void menuOpen();
    bool menuSave();
    void menuSaveAs();
    void menuSearch();
    void menuPreferences();
    void menuAbout();
    void menuCopyright();

  protected:
    void init(TTextModel*);
  
    void closeRequest();
  
    TTextArea *ta;
    string filename;

    bool _Check();
    bool _Save(const string& title);
    
    // void search(TStringManipulator*);

  public:
    static void signalQuit(int);
};

TBitmap *logo;

void createMemoryFiles();

int 
main(int argc, char **argv, char **envv)
{
  static char** argv2 = new char*[10];
  int argc2=0;
  
  argv2[argc2++]=argv[0];
  
  int i;
  for(i=1; i<argc; ++i) {
    if (strcmp(argv[i], "--font")==0) {
      fontname = argv[++i];
    } else
    if (strcmp(argv[i], "--font-engine")==0) {
      argv2[argc2++]=argv[i];
      argv2[argc2++]=argv[++i];
    } else
    if (strcmp(argv[i], "--")==0) {
      break;
    } else {
      break;
    }
  }
  toad::initialize(argc2, argv2, envv);
  
  int result=0;

  createMemoryFiles();
  logo = new TBitmap;
  try {
    logo->load(RESOURCE("hacktool.png"));
    signal(SIGINT , TMainWindow::signalQuit);
    signal(SIGTERM, TMainWindow::signalQuit);
    
    if (i>=argc) {
      new TMainWindow(NULL, programname);
    } else {
      for(; i<argc; ++i) {
        TMainWindow *wnd = new TMainWindow(NULL, programname, argv[i]);
      }
    }
    toad::mainLoop();
  } catch (exception &e) {
    cout << e.what() << endl;
    if (logo) {
      delete logo;
      logo = NULL;
    }
  }
  delete logo;
  toad::terminate();
  return result;
}

TMainWindow::TMainWindow(TWindow *p, const string &t, const string &filename):
  super(p, t)
{
  ifstream in(filename.c_str());
  TTextModel *model = new TTextModel;
  if (in) {
    stringbuf buf;
    in.get(buf, 0); 
    model->setValue(buf.str());
  }
  init(model);
  this->filename = filename;
  setTitle(programname+ ": " + basename((char*)filename.c_str()));
}

TMainWindow::TMainWindow(TWindow *p, const string &t):
  super(p, t)
{
  init(0);
}

TMainWindow::TMainWindow(TWindow *p, const string &t, TTextModel *m):
  super(p, t)
{
  init(m);
}

void
TMainWindow::init(TTextModel *tm)
{
  setSize(498, 374);

  TMenuBar *mb = new TMenuBar(this, "menubar");
  mb->loadLayout(RESOURCE("order.atv"));
  
  new TUndoManager(this, "undomanager");
  
  TAction *action;
  action = new TAction(this, "file|new");
  CONNECT(action->sigClicked, this, menuNew);
  action = new TAction(this, "file|open");
  CONNECT(action->sigClicked, this, menuOpen);
  action = new TAction(this, "file|save_as");
  CONNECT(action->sigClicked, this, menuSaveAs);
  action = new TAction(this, "file|save");
  CONNECT(action->sigClicked, this, menuSave);
  action = new TAction(this, "file|quit");
  CONNECT(action->sigClicked, this, closeRequest);

  action = new TAction(this, "file|newview");
  CONNECT(action->sigClicked, this, menuNewView);

  action = new TAction(this, "edit|search");
  CONNECT(action->sigClicked, this, menuSearch);
  action = new TAction(this, "edit|preferences");
  CONNECT(action->sigClicked, this, menuPreferences);

  action = new TAction(this, "info|about");
  CONNECT(action->sigClicked, this, menuAbout);
  action = new TAction(this, "info|copyright");
  CONNECT(action->sigClicked, this, menuCopyright);

  if (!tm)
    tm = new TTextModel;
  ta = new TTextArea(this, "textarea", tm);
//cerr << "other size of TTextArea = " << sizeof(TTextArea) << endl;
  ta->getPreferences()->setFont(::fontname);
  ta->setFocus();
#if 0
  TTextArea::TConfiguration config;
  config.tabwidth = 2;
  ta->setConfiguration(&config);
#endif

  TStatusBar *sb = new TStatusBar(this, "statusbar", this, ta);
  CONNECT(ta->sigStatus, sb, update);
  
  attach(mb, TOP | LEFT | RIGHT);
  attach(ta, TOP, mb);
  attach(ta, LEFT | RIGHT);
  attach(ta, BOTTOM, sb);
  attach(sb, LEFT | RIGHT | BOTTOM);
}

TMainWindow::~TMainWindow()
{
}

/**
 * This static method is called when the applications receives a
 * SIGINT (CTRL+C) or SIGTERM (ie. during shutdown) and stores
 * the current editors.
 */
void 
TMainWindow::signalQuit(int)
{
  cout << "CTRL+C" << endl;

#if 0
  file << "\n"
       << "*** Modified files in TOE when it aborted on \n";
  switch(...) {
    case :
      file << "*** TOE was aborted because the terminal closed\n";
      break;
    case 
      file << "*** TOE was aborted by signal 1\n";
      break;
  }
  file << "\n";
  for(...) {
    file << "*** File '" << filename << "'\n"
         << data;
         << "\n";
  }
#endif

  postQuitMessage(0);
}

void 
TMainWindow::menuNew()
{
  if (!_Check())
    return;
    
  filename.erase();
  setTitle(programname);
  ta->setModel(new TTextModel);
}

void
TMainWindow::menuNewView()
{
  TMainWindow *mw = new TMainWindow(NULL, programname, ta->getModel());
  mw->placeWindow(mw, PLACE_PARENT_RANDOM, this);
  mw->setTitle(programname+ ": " + basename((char*)filename.c_str()));
  mw->filename = filename;
  mw->createWindow();
}

bool 
TMainWindow::_Check()
{
//  cout << "_Check: _toad_ref_cntr=" << ta->getModel()->_toad_ref_cntr << endl;

  if (ta->isModified() && ta->getModel()->_toad_ref_cntr==1) {
    unsigned r = messageBox(NULL,
                   "Buffer is modified",
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
TMainWindow::menuOpen()
{
  if (!_Check())
    return;

  TFileDialog dlg(this, "Open..");
  dlg.doModalLoop();
  if (dlg.getResult()==TMessageBox::OK) {
//cerr << "OK" << endl;
    ifstream in(dlg.getFilename().c_str());
    stringbuf buf;
    in.get(buf, 0);
    TTextModel *model = new TTextModel();
    model->setValue(buf.str());
    ta->setModel(model);
    filename = dlg.getFilename();
    setTitle(programname+ ": " + basename((char*)filename.c_str()));
  }
//else cerr << "NOT OK" << endl;
}

bool 
TMainWindow::menuSave()
{
  string title = "Save";
  if (filename.size()==0) {
    TFileDialog dlg(this, title, TFileDialog::MODE_SAVE);
    dlg.doModalLoop();
    if (dlg.getResult()==TMessageBox::OK) {
      filename = dlg.getFilename();
    } else {
      return false;
    }
  }
  return _Save(title);
}

void 
TMainWindow::menuSaveAs()
{
  string title = "Save As..";

  TFileDialog dlg(this, title, TFileDialog::MODE_SAVE);
  dlg.setFilename(filename);
  dlg.doModalLoop();
  if (dlg.getResult()==TMessageBox::OK) {
    filename = dlg.getFilename();
    if (_Save(title))
      setTitle(programname+ ": " + basename((char*)filename.c_str()));
  }
}

/**
 * Save file and use `title' as the title for the message boxes
 */
bool
TMainWindow::_Save(const string &title)
{
  struct stat st;

  bool makebackup = false;
    
  // check original filename
  //-------------------------
  if (stat(filename.c_str(), &st)==0) {
    if (S_ISDIR(st.st_mode)) {
      messageBox(NULL, 
                 title, 
                 "You've specified a directory but not a filename.",
                 TMessageBox::ICON_STOP |
                 TMessageBox::OK);
      return false;
    }
    if (!S_ISREG(st.st_mode)) {
      messageBox(NULL,
                 title,
                 "Sanity check. You haven't specified a regular file.",
                 TMessageBox::ICON_STOP |
                 TMessageBox::OK);
      return false;
    }
    if (st.st_mode & S_IFLNK) {
      char real[PATH_MAX];
      if (realpath(filename.c_str(), real) == NULL) {
        messageBox(NULL,
                   title,
                   "Internal error: realpath failed to resolve symlink.",
                   TMessageBox::ICON_STOP |
                   TMessageBox::OK);
        return false;
      }
      filename=real;
    }
    makebackup = true;
  }
  
  // check backup filename
  //-----------------------
  string backupfile = filename+"~";
  if (stat(backupfile.c_str(), &st)==0) {
    if (!S_ISREG(st.st_mode)) {
      if (messageBox(NULL,
                     title,
                     "I can't create the backup file.\n\n"
                     "Do you want to continue?",
                     TMessageBox::ICON_QUESTION |
                     TMessageBox::YES | TMessageBox::NO
                    ) != TMessageBox::YES)
      {
        return false;
      }
      makebackup = false;
    }
  }
  if (makebackup) {
    if (rename(filename.c_str(), backupfile.c_str())!=0) {
      if (messageBox(NULL,
                     title,
                     "Failed to create the backup file.\n\n"
                     "Do you want to continue?",
                     TMessageBox::ICON_QUESTION |
                     TMessageBox::YES | TMessageBox::NO |
                     TMessageBox::BUTTON2) != TMessageBox::YES)
      {
        return false;
      }
    }
  }

  int fd = open(filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
  if (fd==-1) {
    if (makebackup) {
      messageBox(NULL,
                 title,
                 "Damn! I've failed to create the file.",
                 TMessageBox::ICON_EXCLAMATION | TMessageBox::OK);
      return false;
    }
  }
  
  string data = ta->getValue();

  if (write(fd, data.c_str(), data.size())!=data.size()) {
    messageBox(NULL,
               title,
               "Couldn't write everything...",
               TMessageBox::ICON_EXCLAMATION | TMessageBox::OK);
    close(fd);
    return false;
  }
  close(fd);
  ta->setModified(false);
  return true;
}

void
TMainWindow::closeRequest()
{
  if (!_Check())
    return;
  super::closeRequest();
  // delete this; // delete window when closed
  sendMessageDeleteWindow(this);
}

TTextField *tf;

void
TMainWindow::menuSearch()
{
  static TDialog * dlg = NULL;
//  TTextField *tf;
  TPushButton *btn;
  
  static string phrase;

  if (!dlg) {
    dlg = new TDialog(NULL, "Search");
/*
    dlg->setSize(175,34);
    tf  = new TTextField(dlg, "phrase", &phrase);
    tf->setDirect(true);
    tf->setShape(5,5,100,22);
*/
    btn = new TPushButton(dlg, "do_search");
    btn->setLabel("Search");
    btn->setShape(110,5,60,22);
  
//    CONNECT(tf->sigClicked, this, search, tf);
//    CONNECT(btn->sigClicked, this, search, tf);
  }
  if (!dlg->isRealized()) {
    dlg->createWindow();
    dlg->setMapped(true);
  } else {
    cout << "REALIZED\n";
  }
  dlg->setMapped(true);
  dlg->raiseWindow();
}

#if 0
void
TMainWindow::search(TStringManipulator *str)
{
  cout << "search phrase:" << str->getValue() << endl;
  ta->find(str->getValue());
}
#endif

/*
class TPreferencesDialog:
  public TDialog
{
  public:
    TPreferencesDialog(TWindow *parent, const string &title);
};
*/

void
TMainWindow::menuPreferences()
{
  TIntegerModel tabwidth;
  TBoolModel autoindent;
  TBoolModel notabs;
  TBoolModel viewtabs;
  GRadioStateModel<TTextArea::TPreferences::EMode> mode;
  
  TTextArea::TPreferences *p = ta->getPreferences();
  tabwidth.setValue(p->tabwidth);
  autoindent = p->autoindent;
  notabs = p->notabs;
  viewtabs = p->viewtabs;
#warning "fixme: setting value for GRadioStateModel doesn't work here"
#warning "fixme: GRadioStateModel causes a crash"
// MALLOC_CHECK_=2 ./hacktool --toad-debug-memory &> l
#warning "fixme: TMenuBar frees memory twice: in ~TNode and TNode::noWindow"
//  mode.setValue(p->mode);

  TDialog dlg(this, "Preferences");
  new TTextArea(&dlg, "tabwidth", &tabwidth);
  new TCheckBox(&dlg, "autoindent", &autoindent);
  new TCheckBox(&dlg, "notabs", &notabs);
  new TCheckBox(&dlg, "viewtabs", &viewtabs);
#warning "radiobuttons must be removed before GRadioStateModel because model doesn't inform the buttons about it's death"
  mode.add(new TRadioButton(&dlg, "mode.normal"), 
         TTextArea::TPreferences::NORMAL);
  mode.add(new TRadioButton(&dlg, "mode.wordstar"), 
         TTextArea::TPreferences::WORDSTAR);
  mode.setValue(p->mode);
  
  dlg.loadLayout(RESOURCE("preferences.atv"));
  dlg.doModalLoop();
  
  p->tabwidth = tabwidth.getValue();
  p->autoindent = autoindent;
  p->notabs = notabs;
  p->viewtabs = viewtabs;
  p->mode = mode.getValue();
#if 0
cerr << "tabwidth  : " << tabwidth << endl
     << "autoindent: " << autoindent << endl
     << "notabs    : " << notabs << endl
     << "viewtabs  : " << viewtabs << endl
     << "mode      : " << mode.getValue() << endl;
#endif
}

void
TMainWindow::menuAbout()
{
  messageBox(this, "About",
    "toe - TOAD's Own Editor\n"
    "Copyright © 1999-2004 by Mark-André Hopf <mhopf@mark13.org>\n"
    "See http://www.mark13.org/toad/ for further information.\n"
    "\n"
    "TOE is a simple UTF-8 text editor.\n"
    "\n"
    "The name TOE is a reverence to Joe's Own Editor JOE, which "
    "was used during the first years writing the TOAD library.",
    TMessageBox::OK, logo);
}

void
TMainWindow::menuCopyright()
{
  messageBox(this, "Copyright",
    "toe - TOAD's Own Editor v0.3\n"
    "Copyright © 1999-2004 by Mark-André Hopf <mhopf@mark13.org>\n"
    "\n"
    "This program is free software; you can redistribute it and/or modify "
    "it under the terms of the GNU General Public License as published by "
    "the Free Software Foundation; either version 2 of the License, or "
    "(at your option) any later version.\n\n"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
    "GNU General Public License for more details.\n\n"
    "You should have received a copy of the GNU General Public License "
    "along with this program; if not, write to the Free Software "
    "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA."
    , TMessageBox::OK, logo);
}

// TStatusBar
//---------------------------------------------------------------------------
TStatusBar::TStatusBar(TWindow *p,
                       const string &t,
                       TMainWindow *m,
                       TTextArea *tx):
  TWindow(p, t)
{
  main = m;
  lock = false;
  ta = tx;
  // editor = new TEditorControl(ta);
  setBackground(TColor::DIALOG);
  setSize(1,24);
  ypos = (getHeight()-getDefaultFont().getHeight())>>1;
  xpos_row = getHeight()+5;
  line = 1;
  txt_row = new TTextField(this, "row", &line);
//  TUndoManager::unregisterModel(&line);
  TUndoManager::unregisterModel(txt_row->getModel());

//  txt_row->getModel()->undo = false;

  connect(line.sigChanged, this, &TStatusBar::rowChanged);
  txt_row->setShape(xpos_row+getDefaultFont().getTextWidth("Row:"),
                    2,
                    40, TSIZE_PREVIOUS);
}

void 
TStatusBar::paint()
{
  TPen pen(this);

  pen.setFillColor(TColor::DIALOG);

  pen.fillString(xpos_row, ypos, string("Row:"));
  
  stringstream str;
  str << "Col: " << ta->getCursorX()+1 << "   ";
  pen.fillString(txt_row->getXPos()+40+5,ypos, str.str());

  if (ta->isModified()) {
    pen.setColor(128,0,0);
    pen.fillCircle(4,4,getHeight()-8, getHeight()-8);
    pen.setColor(TColor::DIALOG);
    pen.fillCircle(6,6,getHeight()-12, getHeight()-12);
    pen.setColor(128,0,0);
    pen.fillCircle(8,8,getHeight()-16, getHeight()-16);
  } else {
    pen.setFillColor(TColor::DIALOG);
    pen.fillCircle(4,4,getHeight()-8, getHeight()-8);
  }
}

/**
 * TMainWindow connected this method with TTextArea's sigStatus signal.
 */
void
TStatusBar::update()
{
//cerr << "TStatusBar::update: set " << &line << endl;
  if (!lock && ta->getCursorY()+1!=line) {
    lock = true;
    line = ta->getCursorY()+1;
    lock = false;
  }
  invalidateWindow(false);
}

void
TStatusBar::rowChanged()
{
//cerr << "rowChanged to " << (line-1) << endl;
  if (!lock && ta->getCursorY()!=line-1) {
    lock = true;
    ta->gotoLine(line-1);
    ta->setFocus();
    lock = false;
  }
}

void TStatusBar::mouseLDown(int x, int y, unsigned)
{
  TRectangle rect(4,4,getHeight()-8, getHeight()-8);
  if (ta->isModified() && rect.isInside(x,y)) {
    main->menuSave();
  }
}
