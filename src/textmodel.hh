/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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
#include <toad/undo.hh>
#include <toad/io/serializable.hh>

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
    size_t offset;
    /**
     * Length of modification.
     */
    size_t length;
    /**
     * The number of lines modified.
     */
    unsigned lines;
    
    /**
     * Total number of lines inside the text.
     */
    unsigned nlines;
    
    void setValue(const string&);
    void setValue(const char *data, size_t len);
    const string& getValue() const { return data; }
    
    TTextModel(const TTextModel &model) {
      setValue(model.getValue());
    }
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
    
    void insert(size_t offset, int c);
    void insert(size_t offset, const string&);
    void remove(size_t offset, size_t length=1);

    static const int CHARACTER_NONE = 0;
    static const int CHARACTER_TABULATOR = 8;
    static const int CHARACTER_RETURN = 13;
    static const int CHARACTER_CURSOR_UP = 1;
    static const int CHARACTER_CURSOR_DOWN = 2;

    /** 
     * The model may not accept all characters or use other characters.
     */
    virtual int filter(int character);
    
    virtual void focus(bool);
    
    class TUndoInsert:
      public TUndo
    {
        TTextModel *model;
        size_t offset;
        size_t length;
      public:
        TUndoInsert(TTextModel *m, size_t o, size_t l) {
          model  = m;
          offset = o;
          length = l;
        }
        bool getRedoName(string *name) const;
        bool getUndoName(string *name) const;
        void undo() {
          model->remove(offset, length);
        }
    };

    class TUndoRemove:
      public TUndo
    {
        TTextModel *model;
        size_t offset;
        string text;
      public:
        TUndoRemove(TTextModel *m, size_t o, const string &t) {
          model  = m;
          offset = o;
          text   = t;
        }
        bool getRedoName(string *name) const;
        bool getUndoName(string *name) const;
        void undo() {
          model->insert(offset, text);
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

void store(atv::TOutObjectStream &out, const toad::TTextModel &value);
bool restore(atv::TInObjectStream &p, toad::TTextModel *value);

#endif
