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

#ifndef TFileDialog
#define TFileDialog TFileDialog

#include <toad/dialog.hh>
#include <toad/textfield.hh>
#include <toad/textarea.hh>
#include <toad/tablemodels.hh>

namespace toad {

class TListBox;
class TCheckBox;
class TComboBox;

class TFileDialog:
  public TDialog
{
    typedef TDialog super;
  public:
    TFileDialog(TWindow* ,const string&);
    ~TFileDialog();
    unsigned getResult() const;
    const string& getFilename() const;
    // void* getXtra() const;
    void setFilename(const string&);
    void addFileType(const string &name,const string &ext,void *xtra);
    void setFileType(unsigned);

  protected:
    void create();
    void destroy();
    void paint();
    
    void btnOk();
    void btnAbort();
    void selectFile(TStringVectorSelectionModel*);
    void selectDir(TStringVectorSelectionModel*);
    void hidden(bool);
    // void actFileType(TComboBox*);
    
  private:
    unsigned result;
    bool bShowHiddenFiles;
    void loadDir();
    void clrDir();
    bool pFilterCompare(const char*);
  
    string currentdir;
    string filter;
    TTextModel filename;
    // TTextField *wndFileEdit;
    // TListBox *lb_file, *lb_dir;
    // TComboBox *cb;
    TStringVector file;
    TStringVector directory;

  public:   
    struct TFileType {
      string name,ext;
      void *xtra;
      TFileType(){};
      TFileType(const string &n,const string &e,void *x)
      { name=n; ext=e; xtra=x;}
      TFileType(const TFileType &x)
      {
        name  = x.name;
        ext   = x.ext;
        xtra  = x.xtra;
      }
    };
    typedef vector<TFileType> TFTList;
    TFTList filetype;
};

} // namespace toad

#endif
