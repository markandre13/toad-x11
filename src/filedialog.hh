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

#ifndef _TOAD_FILEDIALOG_HH
#define _TOAD_FILEDIALOG_HH

#include <set>
#include <toad/dialog.hh>
#include <toad/textmodel.hh>
#include <toad/boolmodel.hh>
#include <toad/tablemodels.hh>

namespace toad {

class TTable;
class TComboBox;

/**
 *
 * A table item renderer to be called from 'GTableCellRenderer_ItemRow'.
 */
struct TTableItemRow
{
  public:
    virtual int getColWidth(int col) const = 0;
    virtual void renderItem(TPen &pen, int col, int w, int h) const = 0;
};

//----------------------------------------------

struct TDirectoryEntry:
  public TTableItemRow
{
  string name;
  mode_t mode;
  off_t size;

  bool operator<(const TDirectoryEntry &f) const;
  int getColWidth(int col) const;
  void renderItem(TPen &pen, int col, int w, int h) const;
};

typedef GSTLSet<set<TDirectoryEntry>, TDirectoryEntry> TDirectoryEntrySet;

class TFileFilter
{
  public:
    virtual bool doesMatch(const string &filename) = 0;
    virtual const char * toText() const = 0;
    
    static bool wildcard(const string &str, const string &filter);
    
    const char * toText(unsigned) const { return toText(); }
};

class TSimpleFileFilter:
  public TFileFilter
{
  public:
    TSimpleFileFilter(const string &name);
    bool doesMatch(const string &filename);
    const char * toText() const;

  protected:
    string name;
    vector<string> extension;
};

class TFileDialog:
  public TDialog
{
    typedef TFileDialog This;
  public:
    enum EMode {
      MODE_OPEN,
      MODE_SAVE
    };
  
    TFileDialog(TWindow *parent, const string &title, EMode mode = MODE_OPEN);

    void setFilename(const string &s);
    string getFilename() const;
    
    
    void addFileFilter(TFileFilter *);
    void addFileFilter(const string &name);

    unsigned getResult() const {
      return result;
    }
  protected:
    typedef GSTLRandomAccess<vector<TFileFilter*>, TFileFilter*> TFilterList;
    TFilterList filterlist;
    TFileFilter *filter;
  
    EMode mode;
    TPushButton *btn_ok;
  
    TTextModel filename;
    unsigned result;
    bool first_chdir;

    TDirectoryEntrySet entries;
    TBoolModel show_hidden;
    
    void hidden();
    void filenameEdited();
    void filterSelected();
    void adjustOkButton();
    void button(unsigned);
    void loadDirectory();
    void jumpDirectory();
    
    TTable *tfiles;
    TComboBox *cb, *cb_filter;

    void fileSelected();
    void doubleClick();
};

} // namespace toad

#endif
