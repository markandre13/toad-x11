/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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

#include <toad/filedialog.hh>

#include <toad/toad.hh>
#include <toad/textfield.hh>
#include <toad/combobox.hh>
#include <toad/checkbox.hh>
#include <toad/table.hh>
#include <toad/tablemodels.hh>
#include <toad/pushbutton.hh>

// #include <toad/stacktrace.hh>

#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <deque>

// #include "filedialog.hh"

#define FINAL_FILEDIALOG
//#define RESOURCE(file) "file://resource/" file
#define RESOURCE(file) "memory://toad/" file

using namespace toad;

typedef GTableRowRenderer<TDirectoryEntrySet, 3> TTableRowRenderer_DirectoryEntrySet;
typedef GSTLRandomAccess<deque<string>, string> TPreviousDirs;
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
      previous_cwds.erase(p);
      p = previous_cwds.begin();
      e = previous_cwds.end();
    }
    ++p;
  }
  
  while(previous_cwds.size()>10)
    previous_cwds.pop_back();

}

}; // namespace

bool
TDirectoryEntry::operator<(const TDirectoryEntry &f) const {
  return (strcasecmp(name.c_str(), f.name.c_str()) < 0);
}

int
TDirectoryEntry::getColWidth(int col) const {
  switch(col) {
    case 0:
      return 16;
    case 1:
      return 320;
    case 2:
      return 40;
  }
  return 0;
}

void 
TDirectoryEntry::renderItem(TPen &pen, int col, int w, int h) const {
  switch(col) {
    case 0:
      if (S_ISDIR(mode)) {
        pen.drawBitmap(0, (h - bmp.getHeight())/2, bmp);
      }
      break;
    case 1:
      pen.drawString(0,0,name);
      break;
    case 2: {
      char buffer[256];
      int s = size;
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
      pen.drawString(0,0,buffer);
      } break;
  }
}

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
  
  if (previous_cwds.empty()) {
    bmp.load(RESOURCE("folder_red_open.png"));
    previous_cwds.push_front(cwd);
  }
  first_chdir = true;

  // announce data structures to the GUI (after TOAD 1.0)

  // create widgets (going to be obsolete after TOAD 1.0)
  tfiles = new TTable(this, "fileview");
  tfiles->noCursor = true;
  tfiles->selectionFollowsMouse = true;
  tfiles->setRenderer(new TTableRowRenderer_DirectoryEntrySet(&entries));
  connect(tfiles->sigSelection, this, &This::fileSelected);
  connect(tfiles->sigDoubleClicked, this, &This::doubleClick);

  new TTextField(this, "filename", &filename);
  connect(filename.sigChanged, this, &This::filenameEdited);

  filter = 0;
  addFileFilter("All Files (*)");
  cb_filter = new TComboBox(this, "filetype");
  cb_filter->setRenderer(
    new GTableCellRenderer_PText<TFilterList, 1>(&filterlist)
  );
  connect(cb_filter->sigSelection, this, &This::filterSelected);
  cb_filter->getSelectionModel()->setSelection(0,0);

  new TCheckBox(this, "show hidden", &show_hidden);
  connect(show_hidden.sigChanged, this, &This::hidden);

  result = TMessageBox::ABORT;
  
  btn_ok = new TPushButton(this, "ok");
  
  connect(btn_ok->sigActivate, 
          this, &This::button, TMessageBox::OK);
  connect((new TPushButton(this, "cancel"))->sigActivate, 
          this, &This::button, TMessageBox::ABORT);

  cb = new TComboBox(this, "previous");
  cb->setRenderer(new GTableCellRenderer_String<TPreviousDirs>(&previous_cwds));
  cb->getSelectionModel()->setSelection(0,0);
  connect(cb->sigSelection, this, &This::jumpDirectory);
  
//  loadDirectory();
  
  loadLayout(RESOURCE("TFileDialog.atv"));
  
  adjustOkButton();
}

/**
 * \param s The new filename. In case the filename contains an '/',
 *          the part before the last '/' will be used to specify a
 *          new working directory for the filedialog.
 */
void
TFileDialog::setFilename(const string &s)
{
  unsigned n = s.rfind('/');
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
  filterlist.push_back(ff);
//  cb_filter->getSelectionModel()->setSelection(0,0);
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
  for(unsigned i=0; i<name.size(); ++i) {
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
    while(flt[f]==str[s] || flt[f]=='?') {
      if (flt[f]==0 || str[s]==0)
        return (flt[f]==str[s]);
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
TSimpleFileFilter::doesMatch(const string &filename)
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
  tfiles->getSelectionModel()->clearSelection();
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
  static bool lock = false;
  if (lock) return;
//toad::printStackTrace();
  if (tfiles->getSelectionModel()->isEmpty())
    return;

  const TDirectoryEntry &file(
    entries.getElementAt(0, tfiles->getSelectionModel()->begin().getY())
  );
//  cerr << "selected " << file.name << endl;
  filename = file.name;
  adjustOkButton();
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
  if (tfiles->getSelectionModel()->isEmpty())
    return;

  const TDirectoryEntry &file(
    entries.getElementAt(0, tfiles->getSelectionModel()->begin().getY())
  );
//  cerr << "selected " << file.name << endl;
  if (S_ISDIR(file.mode)) {
    if (file.name=="..") {
      unsigned p = cwd.rfind('/');
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
      previous_cwds[0]=cwd;
    }
    previous_cwds.sigChanged();
    cb->getSelectionModel()->setSelection(0,0);

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
//  cerr << "selected directory " << cb->getSelectionModel()->begin().getY() << endl;
  cwd = previous_cwds[cb->getSelectionModel()->begin().getY()];
  loadDirectory();
}

void
TFileDialog::filterSelected()
{
  filter = filterlist[cb_filter->getSelectionModel()->begin().getY()];
  loadDirectory();
}


/**
 * Load the currenty directory.
 */
void
TFileDialog::loadDirectory()
{
  dirent *de;
  DIR *dd;

//cerr << "load directory " << cwd << endl;
  
  dd = opendir(cwd.c_str());
  if (!dd) {
    perror("opendir");
    return;
  }

  entries.sigChanged.lock();
  entries.clear();
  
  while( (de=readdir(dd))!=NULL ) {

    if (de->d_name[0]=='.' && de->d_name[1]==0)
      continue;

    // check if hidden file
    bool show = true;
    if (!show_hidden && de->d_name[0]=='.') {
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
    entries.insert(e);
  }
  
  entries.unlock();
  
  closedir(dd);
}
