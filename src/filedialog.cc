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

typedef GTableRowRenderer<TDirectoryEntrySet, 3> TTableCellRenderer_DirectoryEntrySet;
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
        pen.drawBitmap(0,0,bmp);
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

TFileDialog::TFileDialog(TWindow *parent, const string &title):
  TDialog(parent, title)
{
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
  tfiles->setRenderer(new TTableCellRenderer_DirectoryEntrySet(&entries));
  connect(tfiles->sigSelection, this, &This::fileSelected);

  new TCheckBox(this, "show hidden", &show_hidden);
  connect(show_hidden.sigChanged, this, &This::hidden);
  new TTextField(this, "filename", &filename);

  // TComboBox *cb;
  cb = new TComboBox(this, "previous");
  cb->setRenderer(new GTableCellRenderer_String<TPreviousDirs>(&previous_cwds));
  cb->getSelectionModel()->setSelection(0,0);
  connect(cb->sigSelection, this, &This::jumpDirectory);

  new TComboBox(this, "filetype");

  result = TMessageBox::ABORT;
  connect((new TPushButton(this, "ok"))->sigActivate, 
          this, &This::button, TMessageBox::OK);
  connect((new TPushButton(this, "cancel"))->sigActivate, 
          this, &This::button, TMessageBox::ABORT);
  
  loadDirectory();
  
  loadLayout(RESOURCE("TFileDialog.atv"));
}

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

string
TFileDialog::getFilename() const
{
  return cwd + "/" + filename;
}

void
TFileDialog::hidden()
{
  tfiles->getSelectionModel()->clearSelection();
  loadDirectory();
}

void
TFileDialog::button(unsigned result)
{
  this->result = result;
  previous_cwds.push_front(cwd);
  removeDuplicates();
  destroyWindow();
}

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
    
    TDirectoryEntry e;
    e.name = de->d_name;
    e.mode = st.st_mode;
    e.size = st.st_size;
    entries.insert(e);
  }
  
  entries.unlock();
  
  closedir(dd);
}

void
TFileDialog::jumpDirectory()
{
//  cerr << "selected directory " << cb->getSelectionModel()->begin().getY() << endl;
  cwd = previous_cwds[cb->getSelectionModel()->begin().getY()];
  loadDirectory();
}

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
  }
}
