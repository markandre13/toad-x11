/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2006 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

//
// MacOS X behaviour:
// <name>.<ext>/Contents/PkgInfo (non-empty)
//   will be displayed as a sheet of paper
// <name>.app
//   will be displayed as a sheet of paper with pen, rulees and brush


#include <toad/filedialog.hh>

#include <toad/toad.hh>
#include <toad/textfield.hh>
#include <toad/combobox.hh>
#include <toad/checkbox.hh>
#include <toad/table.hh>
//#include <toad/tablemodels.hh>
#include <toad/pushbutton.hh>
#include <toad/stl/deque.hh>

// #include <toad/stacktrace.hh>

#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>



#define FINAL_FILEDIALOG
//#define RESOURCE(file) "file://resource/" file
#define RESOURCE(file) "memory://toad/" file

using namespace toad;

//typedef GTableRowRenderer<TDirectoryEntrySet, 3> TTableAdapter_DirectoryEntrySet;
//typedef GSTLRandomAccess<deque<string>, string> TPreviousDirs;

typedef GDeque<string> TPreviousDirs;

namespace {

TBitmap bmp;
string cwd;
TPreviousDirs previous_cwds;

void  
removeDuplicates()
{
  TPreviousDirs::iterator p, e;
  p = previous_cwds.begin();
  e = previous_cwds.end();
  ++p;
  while(p!=e) {
    if (*p == cwd) {
      p = previous_cwds.erase(p);
      e = previous_cwds.end();
    } else {
      ++p;
    }
  }
  
  while(previous_cwds.size()>10)
    previous_cwds.pop_back();
}

}; // namespace


namespace toad {
class TDirectoryAdapter:
  public TTableAdapter, public GModelOwner<TDirectory>
{
    int w, h;
  public:
    TDirectoryAdapter(TDirectory *directory) {
      setModel(directory);
    }
    ~TDirectoryAdapter() { setModel(0); }
    size_t getCols() { return 3; }
    size_t getRows() { return model ? model->entries.size() : 0; }
    TDirectory* getModel() const { return GModelOwner<TDirectory>::getModel(); }

    void modelChanged(bool newmodel) {
      if (model) {
        TFont &font(TOADBase::getDefaultFont());
        h = font.getHeight();
        w = 0;
        for(vector<TDirectory::TDirectoryEntry>::iterator p = model->entries.begin();
            p != model->entries.end();
            ++p)
        {
          w = max(w, font.getTextWidth(p->name));
        }
      }
      TTableAdapter::modelChanged(newmodel);
    }
    
    void tableEvent(TTableEvent &te);
};

}

void
TDirectoryAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      switch(te.col) {
        case 0: te.w = 16; break;
        case 1: te.w = w+2; 
        break;
        case 2: te.w = 40; break;
       }
       break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = h+2;
      break;
    case TTableEvent::PAINT: {
      renderBackground(te);
      TDirectory::TDirectoryEntry &e = model->entries[te.row];
      switch(te.col) {
        case 0:
          if (S_ISDIR(e.mode)) {
            te.pen->drawBitmap(0, (h - bmp.getHeight())/2, bmp);
          }
          break;
        case 1:
          te.pen->drawString(1, 1, e.name);
          break;
        case 2: {
          char buffer[256];
          int s = e.size;
          if (s < 1024) {
            snprintf(buffer, sizeof(buffer), "%iB", s);
          } else {
            s/=1024;
            if (s < 1024) {
              snprintf(buffer, sizeof(buffer), "%iKB", s);
            } else {
              s/=1024;
              if (s < 1024) {
                snprintf(buffer, sizeof(buffer), "%iMB", s);
              } else {
                s/=1024;
                snprintf(buffer, sizeof(buffer), "%iGB", s);
              }
            }
          }
          int x = te.w - te.pen->getTextWidth(buffer) - 1;
          if (x<0)
            x = 1;
          te.pen->drawString( x, 1, buffer);
        } break;
      }
      renderCursor(te);
    } break;
  }
}

class TFilterListAdapter:
  public TTableAdapter, GModelOwner<TFilterList>
{
    int w, h;
  public:
    TFilterListAdapter(TFilterList *m) { setModel(m); }
    ~TFilterListAdapter() { setModel(0); }
    TFilterList* getModel() const { return GModelOwner<TFilterList>::getModel(); }

    void modelChanged(bool newmodel) {
      if (model) {
        TFont &font(TOADBase::getDefaultFont());
        h = font.getHeight();
        w = 0;
        for(TFilterList::const_iterator p = model->begin();
            p != model->end();
            ++p)
        {
          w = max(w, font.getTextWidth((*p)->toText()));
        }
      }
      TTableAdapter::modelChanged(newmodel);
    }
    size_t getCols() { return 1; }
//    size_t getRows() { return model ? model->size() : 0; }
    void tableEvent(TTableEvent &te);
};

void
TFilterListAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      te.w = w+2;
       break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = h+2;
      break;
    case TTableEvent::PAINT:
      renderBackground(te);
      te.pen->drawString(1,1, (*model)[te.row]->toText());
      renderCursor(te);
      break;
  }
}

class TDequeStringAdapter:
  public TTableAdapter, GModelOwner< GDeque<string> >
{
    int w, h;
  public:
    TDequeStringAdapter(GDeque<string> *m) { setModel(m); }
    ~TDequeStringAdapter() { setModel(0); }
    GDeque<string>* getModel() const { return  GModelOwner< GDeque<string> >::getModel(); }

    void modelChanged(bool newmodel) {
      if (model) {
        TFont &font(TOADBase::getDefaultFont());
        h = font.getHeight();
        w = 0;
        for(GDeque<string>::const_iterator p = model->begin();
            p != model->end();
            ++p)
        {
          w = max(w, font.getTextWidth(*p));
        }
      }
      TTableAdapter::modelChanged(newmodel);
    }
    size_t getCols() { return 1; }
//    size_t getRows() { return model ? model->size() : 0; }
    void tableEvent(TTableEvent &te);
};

void
TDequeStringAdapter::tableEvent(TTableEvent &te)
{
  switch(te.type) {
    case TTableEvent::GET_COL_SIZE:
      te.w = w+2;
       break;
    case TTableEvent::GET_ROW_SIZE:
      te.h = h+2;
      break;
    case TTableEvent::PAINT:
      renderBackground(te);
      te.pen->drawString(1,1, (*model)[te.row]);
      renderCursor(te);
      break;
  }
}


#if 0
TTableAdapter*
GDeque<string>::getDefaultAdapter() {
  return new TDequeStringAdapter(this);
}
#endif

void
TFileDialog::TResource::store(TOutObjectStream &out) const
{
  TPreviousDirs::iterator p, e;
  p = previous_cwds.begin();
  e = previous_cwds.end();
  while(p!=e) {
    out.indent();
    ::store(out, *p);
    ++p;
  }
}

bool
TFileDialog::TResource::restore(TInObjectStream &in)
{
  string filename;
  if (in.what==ATV_START)
    previous_cwds.erase(previous_cwds.begin(), previous_cwds.end());
  if (::restore(in, &filename)) {
//    cerr << "restored '" << filename << "'\n";
    previous_cwds.push_back(filename);
    return true;
  }
  if (super::restore(in)) 
    return true;
  ATV_FAILED(in);
  return false;
}

#if 0
bool
TFileDialog::TDirectoryEntry::operator<(const TDirectoryEntry &f) const {
  return (strcasecmp(name.c_str(), f.name.c_str()) < 0);
}
#endif

TFileDialog::TFileDialog(TWindow *parent, const string &title, EMode mode):
  TDialog(parent, title)
{
  this->mode = mode;

  // create data structures
  if (cwd.empty()) {
    char buffer[4096];
    getcwd(buffer, 4095);
    cwd = buffer;
  }
  
  static bool loadedbitmap = false;
  if (!loadedbitmap) {
    bmp.load(RESOURCE("folder_red_open.png"));
    loadedbitmap = true;
  }

  if (previous_cwds.empty()) {
    previous_cwds.push_front(cwd);
  }
  first_chdir = true;

  // announce data structures to the GUI (after TOAD 1.0)

  // create widgets (going to be obsolete after TOAD 1.0)
  tfiles = new TTable(this, "fileview");
  tfiles->noCursor = true;
  tfiles->selectionFollowsMouse = true;
  loadDirectory();
  tfiles->setAdapter(new TDirectoryAdapter(&entries));
  entrychoice.setRowColMode(TRectangleSelectionModel::WHOLE_ROW);
  tfiles->setSelectionModel(&entrychoice);
  connect(tfiles->sigSelection, this, &This::fileSelected);
  connect(tfiles->sigDoubleClicked, this, &This::doubleClick);

  new TTextField(this, "filename", &filename);
  connect(filename.sigChanged, this, &This::filenameEdited);

  addFileFilter("All Files (*)");
  TComboBox *cb_filter = new TComboBox(this, "filetype");
  cb_filter->setAdapter(new TFilterListAdapter(&filterlist));
  cb_filter->setSelectionModel(&filter);
  cb_filter->clickAtCursor();

  new TCheckBox(this, "show hidden", &show_hidden);
  connect(show_hidden.sigChanged, this, &This::hidden);

  result = TMessageBox::ABORT;
  
  btn_ok = new TPushButton(this, "ok");
  
  connect(btn_ok->sigClicked, 
          this, &This::button, TMessageBox::OK);
  connect((new TPushButton(this, "cancel"))->sigClicked, 
          this, &This::button, TMessageBox::ABORT);

  cb_prev = new TComboBox(this, "previous");
//  cb_prev->setAdapter(new GTableCellRenderer_String<TPreviousDirs>(&previous_cwds));
  cb_prev->setAdapter(new TDequeStringAdapter(&previous_cwds));
  cb_prev->clickAtCursor();

  // don't connect earlier to avoid loadDirectory being called unneccessary
  filter.select(0,1);
  connect(filter.sigChanged, this, &This::filterSelected);
  connect(cb_prev->sigSelection, this, &This::jumpDirectory);
  
  loadLayout(RESOURCE("TFileDialog.atv"));
  
  adjustOkButton();
}

void
TFileDialog::create()
{
  // between the constructor and window creation new file filters may
  // have been added, so we invoke loadDirectory (by selecting the
  // first filter) here:
  // cb_filter->clickAtCursor();
  filter.select(0,0);
}

/**
 * \param s The new filename. In case the filename contains an '/',
 *          the part before the last '/' will be used to specify a
 *          new working directory for the filedialog.
 */
void
TFileDialog::setFilename(const string &s)
{
  size_t n = s.rfind('/');
  if (n==string::npos) { 
    filename = s;
  } else {
    cwd = s.substr(0,n);  
    previous_cwds.push_front(cwd);
    removeDuplicates();
    filename = s.substr(n+1);
  }
}

/**
 * Returns the full pathname of the selected file.
 */
string
TFileDialog::getFilename() const
{
  return cwd + "/" + filename;
}

void
TFileDialog::addFileFilter(TFileFilter *ff)
{
  if (filterlist.begin()!=filterlist.end())
    filterlist.insert(filterlist.end()-1, ff);
  else
    filterlist.push_back(ff);
}

/**
 * This method provides a simple way to add a file filter.
 *
 * \param name  A filtername, ie. "JPEG Image (*.jpg, *.jpeg)" or
 *              "All Files (*)"
 */
void
TFileDialog::addFileFilter(const string &name)
{
  addFileFilter(new TSimpleFileFilter(name));
}

TSimpleFileFilter::TSimpleFileFilter(const string &name)
{
  this->name = name;
  
  unsigned state = 0;
  unsigned b;
  for(size_t i=0; i<name.size(); ++i) {
    char c = name[i];
    switch(state) {
      case 0:
        if (c=='(')
          state = 1;
        break;
      case 1:
        if (c==')') {
          state = 3;
        } else
        if (!isblank(c)) {
          b = i;
          state = 2;
        }
        break;
      case 2:
        if (isblank(c) || c==',' || c==')') {
          extension.push_back(name.substr(b, i-b));
          if (c!=')')
            state = 1;
          else
            state = 3;
        }
        break;
    }
  }
}

bool
TFileFilter::wildcard(const string &str, const string &filter)
{
  const char *flt = filter.c_str();
  bool wild = false;
  int fp=0,sp=0, f,s;

  moonchild:
  while(flt[fp]=='*') {
    fp++;
    wild = true;
  }
  if (flt[fp]==0)
    return true;

  while(true) {
    f=fp; s=sp;
    while(tolower(flt[f])==tolower(str[s]) || flt[f]=='?') {
      if (flt[f]==0 || str[s]==0)
        return (tolower(flt[f])==tolower(str[s]) /*flt[f]==str[s]*/);
      s++;
      f++;
    }
    if (flt[f]=='*') {
      fp=f;
      sp=s;
      goto moonchild;
    }
    if (!wild)
      return false;
    sp++;
    if (str[sp]==0)
      return false;
  }
  return false;
}

bool
TSimpleFileFilter::doesMatch(const string &filename) const
{
  vector<string>::const_iterator p, e;
  p = extension.begin();
  e = extension.end();
  while(p!=e) {
//    cout << "compare " << filename << " with " << *p << endl;
    if (wildcard(filename, *p)) {
//      cout << "match" << endl;
      return true;
    }
    ++p;
  }
  return false;
}

const char * 
TSimpleFileFilter::toText() const
{
  return name.c_str();
}

/**
 * Reconfigure the view in case the 'hidden' checkbox was modified.
 */
void
TFileDialog::hidden()
{
//  tfiles->getSelectionModel()->clearSelection();
  loadDirectory();
}

/**
 * The filename was edited.
 */
void
TFileDialog::filenameEdited()
{
  adjustOkButton();
}

/**
 * One of the pushbuttons was pressed.
 */
void
TFileDialog::button(unsigned result)
{
  this->result = result;
  previous_cwds.push_front(cwd);
  removeDuplicates();
  destroyWindow();
}

/**
 * A file was selected with a single click
 */
void
TFileDialog::fileSelected()
{
#if 1
  if (entrychoice.empty())
    return;
  filename = entries[entrychoice.getRow()].name;
  adjustOkButton();
#else
  static bool lock = false;
  if (lock) return;

  const TDirectoryEntry &file(entries.getElementAt(0, entrychoice.getRow()));
//  cerr << "selected " << file.name << endl;
  filename = file.name;
  adjustOkButton();
#endif
}

enum EFileType { TYPE_NEW, TYPE_DIRECTORY, TYPE_FILE };

void
TFileDialog::adjustOkButton()
{  
  EFileType type;
  struct stat st;
  if (stat(getFilename().c_str(), &st)!=0) {
    type = TYPE_NEW;
  } else {
    if (S_ISDIR(st.st_mode))
      type = TYPE_DIRECTORY;
    else
      type = TYPE_FILE;
  }
  
  switch(mode) {
    case MODE_OPEN:
      switch(type) {
        case TYPE_FILE:
          btn_ok->setLabel("Open");
          break;
        case TYPE_DIRECTORY:
          btn_ok->setLabel("Enter");
          break;
        case TYPE_NEW:
          btn_ok->setLabel("Open New");
          break;
      }
      break;
    case MODE_SAVE:
      switch(type) {
        case TYPE_FILE:
          btn_ok->setLabel("Save");
          break;
        case TYPE_DIRECTORY:
          btn_ok->setLabel("Enter");
          break;
        case TYPE_NEW:
          btn_ok->setLabel("Save New");
          break;
      }
      break;
  }
}

/**
 * A file was selected with a double click
 */
void
TFileDialog::doubleClick()
{
  static bool lock = false;
  if (lock) return;
//toad::printStackTrace();
//  if (tfiles->getSelectionModel()->isEmpty())
//    return;

  const TDirectory::TDirectoryEntry &file = entries[entrychoice.getRow()];
//  const TDirectoryEntry &file(entries.getElementAt(0, entrychoice.getRow()));
//  cerr << "selected " << file.name << endl;
  if (S_ISDIR(file.mode)) {
    if (file.name=="..") {
      size_t p = cwd.rfind('/');
      if (p>0) {
        cwd.erase(p);
      } else {
        cwd="/";
      }
    } else {
      if (cwd.size()>1)
        cwd+="/";
      cwd+=file.name;
    }
//    cerr << "  is a directory '" << cwd << "'\n";
    lock=true;

    if (first_chdir) {
//cerr << "push current cwd" << endl;
      previous_cwds.push_front(cwd);
      first_chdir = false;
    } else {
//cerr << "set previous_cwds[0] to current directory" << endl;
//      previous_cwds[0]=cwd;
      previous_cwds.set(0, cwd);
    }
//    previous_cwds.sigChanged();
    cb_prev->setCursor(0,0);
    cb_prev->clickAtCursor();

    loadDirectory();
    lock=false;
  } else {
//    cerr << "  is a file" << endl;
    filename = file.name;
    button(TMessageBox::OK);
  }
}

/**
 * The directory was changed.
 */
void
TFileDialog::jumpDirectory()
{
  cwd = previous_cwds[cb_prev->getCursorRow()];
  loadDirectory();
}

void
TFileDialog::filterSelected()
{
//  cout << "TFileDialog::filterSelected" << endl;
//  filter = filterlist[cb_filter->getCursorRow()];
  loadDirectory();
}

const TFileFilter*
TFileDialog::getFileFilter() const
{
//  cout << "TFileDialog::getFileFilter: row=" << filter.getRow() << endl;
  if (filter.empty())
    return 0;
  return filterlist[filter.getRow()];
}



/**
 * Load the currenty directory.
 */
void
TFileDialog::loadDirectory()
{
//  entries.load(cwd, 0, show_hidden);
  entries.load(cwd, getFileFilter(), show_hidden);
}

void
TDirectory::load(const string &cwd, const TFileFilter *filter, bool hidden)
{
  dirent *de;
  DIR *dd;

  dd = opendir(cwd.c_str());
  if (!dd) {
    perror("opendir");
    return;
  }

//  entries.sigChanged.lock();
  entries.clear();
  
  while( (de=readdir(dd))!=NULL ) {

    if (de->d_name[0]=='.' && de->d_name[1]==0)
      continue;

    // check if hidden file
    bool show = true;
    if (!hidden && de->d_name[0]=='.') {
      show = false;
      if (de->d_name[1]=='.' && de->d_name[2]==0)
        show = true;
    }
    if (!show)
      continue;
      
    struct stat st;
    string fullpath=cwd+"/"+de->d_name;
    stat(fullpath.c_str(), &st);

    if (filter && 
        !S_ISDIR(st.st_mode) &&
        !filter->doesMatch(de->d_name))
      continue;
    TDirectoryEntry e;
    e.name = de->d_name;
    e.mode = st.st_mode;
    e.size = st.st_size;
    entries.push_back(e);
  }
#if 0
#if __GLIBCXX__ != 20050421
  sort(entries.begin(), entries.end());
#else
  std::__insertion_sort(entries.begin(), entries.end());
#endif
#endif  
//  entries.unlock();
  sigChanged();  
  closedir(dd);
}
