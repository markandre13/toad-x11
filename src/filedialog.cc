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

#include <typeinfo>
#include <string>
#include <algorithm>

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/dialog.hh>
#include <toad/pushbutton.hh>
#include <toad/table.hh>
//#include <toad/listbox.hh>
//#include <toad/combobox.hh>
#include <toad/textfield.hh>
#include <toad/textarea.hh>
#include <toad/checkbox.hh>
#include <toad/filedialog.hh>
//#include <toad/lba/StaticCString.hh>
//#include <toad/lba/STLVectorCString.hh>

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

/**
 * \class toad::TFileDialog
 * A dialog for file selections.
 *
 * \todo
 *   \li only change directories on double click!
 */

using namespace toad;

TFileDialog::TFileDialog(TWindow *p,const string &t)
  :TDialog(p,t)
{
  result = TMessageBox::ABORT;
  filter = "";

  setSize(442,315);

#if 0
  TTextField *tf;
  tf = new TTextField(this, "filename");
    tf->setShape(8,24,329,21);
    CONNECT(tf->sigActivate, this, btnOk);
#else
  TTextArea *tf;
  tf = new TTextField(this, "filename", &filename);
    tf->setShape(8,24,329,21);
#endif

  TTable *table;
  
  TStringVectorSelectionModel *sel;

  table = new TTable(this, "dirbox");
    sel = new GTableSelectionModel<TStringVector>(&directory);
    table->setSelectionModel(sel);
    table->setRenderer(new TTableCellRenderer_String(&directory));
    table->setShape(8,68,161,197);
    connect(sel->sigChanged,
      this, &TFileDialog::selectDir,
      sel);
    
  table = new TTable(this, "filebox");
    sel = new GTableSelectionModel<TStringVector>(&file);
    table->setSelectionModel(sel);
    table->setRenderer(new TTableCellRenderer_String(&file));
    table->setShape(176,68,161,197);
    connect(sel->sigChanged,
      this, &TFileDialog::selectFile,
      sel);

#if 0
  lb_dir=new TListBox(this,"dirbox", &directory);
    CONNECT(lb_dir->sigSelect, this, dirSelect, lb_dir);
    lb_dir->setShape(8,68,161,197);

  lb_file=new TListBox(this,"filebox", &file);
    CONNECT(lb_file->sigSelect, this, fileSelect, lb_file);
    CONNECT(lb_file->sigDoubleClick, this, btnOk);
    lb_file->setShape(176,68,161,197);
  
  cb=new TComboBox(this, "filetype", newListBoxAdapter(filetype));
    cb->setSelection(0);
    cb->setShape(8,288,329,21);
    CONNECT(cb->sigSelect, this, actFileType, cb);
#endif

  TPushButton *btn;
  btn = new TPushButton(this, "OK", TMessageBox::OK);
    CONNECT(btn->sigActivate, this, btnOk);
    btn->setShape(348,8,85,21);
  btn = new TPushButton(this, "Abort", TMessageBox::ABORT);
    CONNECT(btn->sigActivate, this, btnAbort);
    btn->setShape(348,32,85,21);
  btn = new TPushButton(this, "Reload", 102);
    btn->setShape(348,68,85,21);
    btn->setEnabled(false);

  bShowHiddenFiles=false;
  TCheckBox *chkb;
  chkb = new TCheckBox(this, "show hidden files");
  connect_value(chkb->getModel()->sigChanged,
                this, &TFileDialog::hidden,
                chkb->getModel());
//    CONNECT(chkb->sigValueChanged, this, hidden, chkb);
    chkb->setShape(348,104,85,29);
//    chkb->setValue(bShowHiddenFiles);

  if (filetype.size()>0)
    filter = filetype[0].ext;
  
  char buffer[4097];
  getcwd(buffer, 4096);
  
  string b(buffer);
  b+="/"+filter;
  filename=b;
  
  loadDir();
}

void TFileDialog::create()
{
  super::create();
  if (filetype.empty()) {
//    cb->bExplicitCreate = true;
//    setSize(getWidth(), getHeight()-28-16);
#if 0
    cb->bFocusTraversal = false;
    cb->bNoFocus = true;
#endif
  }
}

TFileDialog::~TFileDialog()
{
  clrDir();
}

void TFileDialog::paint()
{
  TPen pen(this);
  pen.drawString(8+8,24-16, "Selection");
  pen.drawString(8+8,68-16,"Directories");
  pen.drawString(176+8,68-16,"Files");
  if (!filetype.empty())
    pen.drawString(8+8,288-16,"Filetype");
}

unsigned
TFileDialog::getResult() const
{
  return result;
}

const string& 
TFileDialog::getFilename() const
{
  return filename.getValue();
}

/**
 * file was selected
 */
void 
TFileDialog::selectFile(TStringVectorSelectionModel *m)
{
  TStringVectorSelectionModel::iterator
    p = m->begin(),
    e = m->end();

  if (p==e)
    return;
    
  string filename = this->filename;
  unsigned i = filename.rfind("/");
  filename = i!=string::npos ? filename.substr(0,i) : "";
  filename+='/';
  filename+=*p;
  
  this->filename = filename;
}

/**
 * directory was selected in listbox
 */
void TFileDialog::selectDir(TStringVectorSelectionModel *m)
{
  TStringVectorSelectionModel::iterator
    p = m->begin(),
    e = m->end();

  if (p==e)
    return;

  string filename = this->filename;

  string where = *p;
  if (where==".")
    return;
  
  unsigned i = filename.rfind("/");
  if (i!=string::npos) {
    filename=filename.substr(0,i);
  } else {
    filename="";
  }
  if (where=="..") {
    i = filename.rfind("/");
    filename = i!=string::npos ? filename.substr(0,i) : "";
  } else {
    filename+="/"+where;
  }

  filename+="/"+filter;
  this->filename = filename;

  loadDir();
}

#if 0
// file type selected in combobox
//---------------------------------------------------------------------------
void
TFileDialog::actFileType(TComboBox *cb)
{
  // copy new file extension to 'filter'
  //-------------------------------------
  filter = filetype[cb->getFirstSelectedItem()].ext;
  
  // update textfield
  //------------------
  unsigned wild_pos = min( filename.find("*"), filename.find("?") );

  unsigned slash_pos = filename.rfind("/", wild_pos);
  if (slash_pos==string::npos) {
    filename="/"+filename;
    slash_pos = 0;
  }
  
  string path = filename.substr(0,slash_pos+1);
  filename = filename.substr(slash_pos+1);  
  
  unsigned n = filename.getValue().rfind(".");
  if (n!=string::npos) {
    if (filter.size()>=1)
      filename = filename.getValue().substr(0,n) + filter.substr(1);
  } else {
    filename = filter;
  }

  filename = path + filename;
  
  TDataManipulator::assimilate(&filename);  // update 
  
  // update file and directory window
  //----------------------------------
  loadDir();

  lb_file->setTopItemPosition(0);
  lb_file->adapterChanged();
  lb_dir->setTopItemPosition(0);
  lb_dir->adapterChanged();
}

void *TFileDialog::getXtra() const
{
  unsigned p=cb->getFirstSelectedItem();
  if (p==(unsigned)-1) {
    #ifdef DEBUG
    printf("void *TFileDialog::getXtra(): no selection, returning NULL\n");
    #endif
    return NULL;
  }
  return filetype[p].xtra;
}
#endif

void TFileDialog::hidden(bool flag)
{
  bShowHiddenFiles = flag;
  loadDir();
/*
  lb_file->setTopItemPosition(0);
  lb_file->adapterChanged();

  lb_dir->setTopItemPosition(0);
  lb_dir->adapterChanged();
*/
} 
 
void TFileDialog::destroy()
{
//  lb_file = lb_dir = NULL;
  ENTRYEXIT("TFileDialog::destroy()");
  clrDir();
  chdir(currentdir.c_str());
}

void TFileDialog::btnOk()
{
  filter = filename;
  if (filter.find("*")!=(unsigned)-1 || filter.find("?")!=(unsigned)-1) {
    loadDir();
#if 0
    lb_file->setTopItemPosition(0);
    lb_file->adapterChanged();
    lb_dir->setTopItemPosition(0);
    lb_dir->adapterChanged();
#endif
  } else {
    if (filter.size()==0)
      result = TMessageBox::ABORT;
    else
      result = TMessageBox::OK;
//    endDialog(this);
    destroyWindow();
  }
}

void TFileDialog::btnAbort()
{
  result = TMessageBox::ABORT;
//  endDialog(this);
  destroyWindow();
}
 
struct TComp
{
  bool operator()(const char* a, const char* b) const{
    return strcasecmp(a,b)<0;
  }
  bool operator()(const string& a, const string& b) const{
    return strcasecmp(a.c_str(), b.c_str())<0;
  }
};
static TComp comp;

void
TFileDialog::loadDir()
{
  // clear file and directory buffer
  //---------------------------------
  clrDir();
  
  // read current directory
  //------------------------
  unsigned p = filename.getValue().rfind("/");
  string cwd = p!=string::npos ? filename.getValue().substr(0,p) : "";
  cwd+="/";

  dirent *de;
  
  DIR *dd = opendir(cwd.c_str());
  if (dd==NULL) {
    perror("opendir");
    directory.push_back("..");
    return;
  }

  while( (de=readdir(dd))!=NULL ) {
    string filename = cwd + de->d_name;
    struct stat st;
    stat(filename.c_str(), &st);
    bool bShow = true;

    // check if hidden file
    //----------------------
    if (!bShowHiddenFiles && de->d_name[0]=='.') {
      bShow = false;
      if (de->d_name[1]==0) {
        bShow=true;
      } else {
        if (de->d_name[1]=='.' && de->d_name[2]==0)
          bShow=true;
      }
    }
    if (bShow) {
      if (S_ISDIR(st.st_mode)) {
        directory.push_back(de->d_name);
      } else {
        if (pFilterCompare(de->d_name)) {
          file.push_back(de->d_name);
        }
      }
    }
  }
  closedir(dd);
  
  // sort file and directory entrys
  //--------------------------------
  sort(file.begin(),file.end(), comp);
  sort(directory.begin(),directory.end(),comp);

  // directory without read permission
  //----------------------------------
  if (directory.size()==0) {
    directory.push_back("..");
  }

  file.sigChanged();
  directory.sigChanged();
} 

// compare filename 'str' with 'filter'
//--------------------------------------
bool TFileDialog::pFilterCompare(const char *str)
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

void TFileDialog::clrDir()
{
  file.erase(file.begin(), file.end());
  directory.erase(directory.begin(), directory.end());
}

void TFileDialog::setFileType(unsigned n) {
//  cb->setSelection(n);
}

void TFileDialog::setFilename(const string& s)
{
  filename = s;
#if 0
  TDataManipulator::assimilate(&filename);
  TFTList::iterator p=filetype.begin(), e=filetype.end();
  unsigned i = 0;
  while(p!=e) {
    int n = s.size()-(*p).ext.size()+1;
    if ( n>=0 && (*p).ext.size()>0 && s.substr(n) == (*p).ext.substr(1) ) {
      setFileType(i);
      return;
    }
    p++;
    i++;
  }
  setFileType(0);
#endif
}

void TFileDialog::addFileType(const string &name,const string &ext,void* xtra)
{
#if 0
  filetype.push_back(TFileType(name,ext,xtra));
  const string &s = filename;
  unsigned i = 0;
  TFTList::iterator p=filetype.begin(), e=filetype.end();
  while(p!=e) {
    int n = s.size()-(*p).ext.size()+1;
    if ( n>=0 && (*p).ext.size()>0 && s.substr(n) == (*p).ext.substr(1) ) 
    {
      setFileType(i);
      return;
    }
    p++;
    i++;
  }
  setFileType(0);
#endif
}

#if 0
// ListBoxAdapter for TFileType vector
//---------------------------------------------------------------------------

void TLBA_FileType::printItem(int x,int y,unsigned item,TPen &pen)
{
  if (item>=vec.size()) return;
  string s = vec[item].name;
  if (vec[item].ext.size()>0) {
    s+=" (";
    s+=vec[item].ext;
    s+=")";
  }
  pen.drawString(x,y,s);
}

unsigned TLBA_FileType::getItemHeight() const
{
  return TOADBase::getDefaultFont().getHeight();
}

unsigned TLBA_FileType::getItemCount() const
{
  return vec.size();
}
#endif
