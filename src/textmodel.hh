/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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

#ifndef _TOAD_TEXTMODEL_HH
#define _TOAD_TEXTMODEL_HH

#include <iostream>
#include <toad/model.hh>
#include <toad/undoable.hh>
#include <toad/util/history.hh>

namespace toad {

/**
 * \class TTextModel
 * Data storage for TTextArea.
 *
 * \sa TTextArea
 */
class TTextModel:
  public TModel
{
  public:
    TTextModel();
  
    /** 
     * Almost like sigChanged but in case of text removal it's triggered
     * _before_ the models value is modified, which is needed by
     * TTextArea to perform its screen updates.
     */
    TSignal sigTextArea;
  
    /**
     * Kind of modification that took place.
     */
    enum { CHANGE, INSERT, REMOVE } type;
    /**
     * Start of modification.
     */
    unsigned offset;
    /**
     * Length of modification.
     */
    unsigned length;
    /**
     * The number of lines modified.
     */
    unsigned lines;
    
    /**
     * Total number of lines inside the text.
     */
    unsigned nlines;
    
    void setValue(const string&);
    void setValue(const char *data, unsigned len);
    const string& getValue() const { return data; }
    
    string& operator=(string &s) { setValue(s); return s; }
    const string& operator=(const string &s) { setValue(s); return s; }
    operator const string&() const { return getValue(); }
    const char * c_str() const { return getValue().c_str(); }
    
    //! 'true' when model was modified an needs to be saved
    bool _modified;
    
    bool isModified() const { return _modified; }
    void setModified(bool m) { 
      if (m != _modified) {
        type = INSERT;
        offset = 0;
        length = 0;
        lines = 0;
        _modified = m;
        sigTextArea();
        sigChanged(); 
      }
    }
    
    void doUndo();
    void doRedo();
    
    void insert(unsigned offset, int c, bool undo=true);
    void insert(unsigned offset, const string&, bool undo=true);
    void remove(unsigned offset, unsigned length=1, bool undo=true);

    /** the model may not accept all characters or use other characters */
    virtual int filter(int);
    
    virtual void focus(bool);
    
    typedef GHistory<PUndoable> THistory;
    THistory *history;
    //! 'true' track undo/redo history
    bool undo;
    
    class TUndoableInsert:
      public TUndoable
    {
        TTextModel *model;
        unsigned offset;
        string text;
      public:
        TUndoableInsert(TTextModel *m, unsigned o, const string &t) {
          model  = m;
          offset = o;
          text   = t;
        }
        bool getRedoName(string *name) const { *name = "Redo: Insert"; return true; }
        bool getUndoName(string *name) const { *name = "Undo: Insert"; return true; }
        void undo() {
          model->remove(offset, text.size(), false);
        }
        void redo() {
          model->insert(offset, text, false);
        }
    };

    class TUndoableRemove:
      public TUndoable
    {
        TTextModel *model;
        unsigned offset;
        string text;
      public:
        TUndoableRemove(TTextModel *m, unsigned o, const string &t) {        
          model  = m;
          offset = o;
          text   = t;
        }
        bool getRedoName(string *name) const { *name = "Redo: Delete"; return true; }
        bool getUndoName(string *name) const { *name = "Undo: Delete"; return true; }
        void undo() {
          model->insert(offset, text, false);
        }
        void redo() {
          model->remove(offset, text.size(), false);
        }
    };
    
  protected:
    string data;
};

inline ostream& operator<<(ostream &s, const TTextModel& m) {
  return s<<m.getValue();
}

template <class T>
inline string operator+(const T &l, const TTextModel &r) {
  return l + r.getValue();
}

template <class T>
inline string operator+(const TTextModel &l, const T &r) {
  return l.getValue() + r;
}

typedef GSmartPointer<TTextModel> PTextModel;

TTextModel * createTextModel(TTextModel *m);

} // namespace toad

#endif
