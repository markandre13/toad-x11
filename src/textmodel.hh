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
    
    typedef string::size_type size_type;
    typedef string::const_reference const_reference;
    static const size_type npos = string::npos;
    
    void setValue(const string&);
    void setValue(const char *data, size_t len);
    const string& getValue() const { return _data; }
    
    TTextModel(const TTextModel &model) {
      setValue(model._data);
    }
    
    size_type size() const { return _data.size(); }
    void clear();
    bool empty() const { return _data.empty(); }
    
    const_reference operator[] (size_type p) const { return _data[p]; }
    const_reference at(size_type p) const { return _data.at(p); }
    
    TTextModel& operator+=(const TTextModel &m) { return this->append(m); }
    TTextModel& operator+=(const string &m) { return this->append(m); }
    TTextModel& operator+=(const char *m) { return this->append(m); }
    TTextModel& operator+=(char m) { return this->append(m); }
    
    TTextModel& append(TTextModel &m) { return this->insert(_data.size(), m._data); }
    TTextModel& append(const string &m) { return this->insert(_data.size(), m); }
    TTextModel& append(const char *m) { return this->insert(_data.size(), m); }
    TTextModel& append(char m) { return this->insert(_data.size(), m); }
    
    // assign
    
    // insert
    TTextModel& insert(size_type offset, int c);
    TTextModel& insert(size_type offset, const string&);

    // erase
    TTextModel& erase(size_t offset=0, size_t length=npos);
    
    // replace
    
    string& operator=(string &s) { setValue(s); return s; }
    const string& operator=(const string &s) { setValue(s); return s; }
    operator const string&() const { return _data; }
    const char * c_str() const { return _data.c_str(); }
    const char * data() const { return _data.data(); }

    size_type find(const char *s, size_type p, size_type n) const {
      return _data.find(s, p, n);
    }
    size_type find(const string &s, size_type p=0) const {
      return _data.find(s, p);
    }
    size_type find(const char *s, size_type p=0) const {
      return _data.find(s, p);
    }
    size_type find(char c, size_type p=0) const {
      return _data.find(c, p);
    }
    size_type rfind(const string &s, size_type p=npos) const {
      return _data.rfind(s, p);
    }
    size_type rfind(const char *s, size_type p, size_type n) const {
      return _data.rfind(s, p, n);
    }
    size_type rfind(const char *s, size_type p=npos) const {
      return _data.rfind(s, p);
    }
    size_type rfind(char c, size_type p=npos) const {
      return _data.rfind(c, p);
    }
    // find_first_of
    // find_last_of
    // find_first_not_of
    // find_last_not_of
    
    string substr(size_type p=0, size_type n=npos) const {
      return _data.substr(p, n);
    }
    int compare(const string &s) const {
      return _data.compare(s);
    }
    // more compare...
    
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
          model->erase(offset, length);
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
    string _data;
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

// the following is usually handled by a template in io/serializable.hh
// but this fails with g++ 4.1.0...
inline bool restore(atv::TInObjectStream &in, const char *attribute, toad::TTextModel *value) {
  if (in.attribute != attribute)
    return false;
  return restore(in, value);
}


#endif
