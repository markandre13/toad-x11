/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public 
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

/**
 * \file tablemodels.hh
 *
 * This file contains some predefined model for the TTable widget
 * based on STL containers.
 *
 * \todo
 *   \li Not all STL containers are covered.
 *   \li Not all methods of the provided containers are covered.
 */

#ifndef TTableModel_CString
#define TTableModel_CString TTableModel_CString

#include <toad/table.hh>
#include <toad/stacktrace.hh>
#include <string>
#include <vector>
#include <set>
#include <functional>

namespace toad {

class TAbstractTableModel:
  public TModel
{
  public:
    virtual int getRows() = 0;
    virtual int getCols();
};

typedef GSmartPointer<TAbstractTableModel> PAbstractTableModel;

template <class T>
class GAbstractTableModel:
  public TAbstractTableModel
{
  public:
    typedef T TElement;
    virtual const TElement& getElementAt(int xindex, int yindex) = 0;
};

template <class T>
class GTableSelectionModel:
  public TTableSelectionModel
{
  public:
    typedef T TModel;
    typedef typename T::TElement TElement;
   
    TModel *model;
 
    GTableSelectionModel(TModel *m):model(m) { }
  
    class iterator:
      public TTableSelectionModel::iterator
    {
        TModel *model;
        typedef TTableSelectionModel::iterator super;
      public:
        iterator() {
          model = 0;
        }
        iterator(TRegion *r, bool b, TModel *m):
           TTableSelectionModel::iterator(r, b), model(m) {}
        const TElement& operator*() { return model->getElementAt(getX(), getY()); }
    };
    iterator begin() {
      return iterator(&region, true, model);
    }
    iterator end() {
      return iterator(&region, false, model);
    }
};



/**
 * \ingroup table
 * \class toad::TTableModel_CString
 * 
 * This class may be obsoleted by GArrayWrap
 */
class TTableModel_CString:
  public GAbstractTableModel<const char *>
{
  private:
    const char **list;
    int size;
  public:
    TTableModel_CString(const char **l, int s): list(l), size(s) {}
    TTableModel_CString(const char **l): list(l) {
      const char **p = list;
      size = 0;
      while(*p) {
        size++;
        p++;
      }
    }
    int getRows() {
      return size;
    }
    const TElement& getElementAt(int, int index) {
      assert(index<size);
      return list[index];
    }
};

typedef GSmartPointer<TTableModel_CString> PTableModel_CString;
typedef GTableSelectionModel<TTableModel_CString> TCStringSelectionModel;

/**
 * \ingroup table
 * \class TTableCellRenderer_CString
 * 
 * This class may be obsoleted by TTableModel_String
 */
class TTableCellRenderer_CString:
  public TAbstractTableCellRenderer
{
  private:
    PTableModel_CString model;
  public:
    TTableCellRenderer_CString(TTableModel_CString *m) {
      setModel(m);
    }
    TTableCellRenderer_CString(const char **l) {
      setModel(new TTableModel_CString(l));
    }
    TTableCellRenderer_CString(const char **l, int s) {
      setModel(new TTableModel_CString(l, s));
    }
    ~TTableCellRenderer_CString() {
      setModel(0);
    }
    void setModel(TTableModel_CString *m) {
      if (model)
        disconnect(model->sigChanged, this);
      model = m;
      if (model)
        connect(model->sigChanged, 
                this, &TTableCellRenderer_CString::modelChanged);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    TTableModel_CString * getModel() {
      return model;
    }
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return model->getCols();
    }
    int getRowHeight(int) {
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int) {
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(model->getElementAt(0, i));
        if (w>max)
          max = w;
      }
      return max+2;
    }
    void renderItem(TPen &pen, int x, int y, int w, int h, bool cursor, bool selected, bool focus) {
      pen.drawString(1, 1, model->getElementAt(x, y));
    }
};

/**
 * \ingroup table
 * \class toad::TStringVector
 * 
 * The class should be made obsolete by a GSTLVector template.
 */
class TStringVector:
  public vector<string>,
  public GAbstractTableModel<string>
{
    typedef vector<string> vec;
  public:
    int getRows() {
      return size();
    }
    
    const TElement&
    getElementAt(int, int index) {
      assert(index<size());
      return (*this)[index];
    }
    void push_back(const string &s) {
      vec::push_back(s);
      sigChanged();
    }
};

typedef GSmartPointer<TStringVector> PStringVector;
typedef GTableSelectionModel<TStringVector> TStringVectorSelectionModel;

/**
 * \ingroup table
 * \class toad::GArrayWrap
 * 
 * This template wraps a C array of a fixed size.
 *
 * \code
   struct TZoom {
     const char *text;
     double factor;
     const char * toText(unsigned) const { return text; }
   };
   
   TZoom zoom[13] = { ... };
 
   TComboBox *cb = new TComboBox(this, "zoom");
   cb->setRenderer(
     new GTableCellRenderer_Text<GArrayWrap<TZoom>, 1>(
       new GArrayWrap<TZoom>(zoom, 13)
     );
   );
   \endcode
 */
template <class D>
class GArrayWrap:
  public GAbstractTableModel<D>
{
    typedef GAbstractTableModel<D> TParent;
    typedef typename TParent::TElement TElement;
    TElement *array;
    unsigned size;
    
  public:
    GArrayWrap(TElement *array, unsigned size) {
      this->array = array;
      this->size  = size;
    }
    int getRows() {
      return size;
    }
    const D& getElementAt(int, int index) {
      if (index<0 || index>=size) {
#if 0
        throw TException("index is out of bounds");
#else
        std::cerr << "GArrayWrap::getElementAt: index>=size (" << index << ">=" << size << std::endl;
        printStackTrace();
        abort();
#endif
      }
      return array[index];
    }
};


typedef less<string> less_string;

/**
 * \ingroup table
 * \class toad::GSTLSet
 * 
 * This template wraps the STL's set template.
 *
 * \todo
 *   Wrap more member functions.
 */
template <class C, class D>
class GSTLSet:
  public C,
  public GAbstractTableModel<D>
{
    typedef C container;
    
    int idx;
    typename container::iterator ptr;
    
  public:
    int getRows() {
      return container::size();
    }
    typename container::const_reference 
    getElementAt(int, int index) {
      assert(index>=0 && index<(int)container::size());
      while(idx<index) {
        ++idx;
        ++ptr;
      }
      while(idx>index) {
        --idx;
        --ptr;
      }
      return *ptr;
    }
    void insert(typename container::const_reference s) {
      container::insert(s);
      idx = 0;
      ptr = container::begin();
      GAbstractTableModel<D>::sigChanged();
    }
};

/**
 * \ingroup table
 * \class toad::GSTLMap
 * 
 * This template wraps the STL's map template.
 *
 * \todo
 *   Wrap more member functions.
 */
template <class C, class K, class D>
class GSTLMap:
  public C,
  public GAbstractTableModel<D>
{
    typedef C container;
    
    int idx;
    typename container::iterator ptr;
    
  public:
    int getRows() {
      return container::size();
    }
    const D& getElementAt(int, int index) {
      if (index>=container::size()) {
        cerr << "GSTLMap::getElementAt: index>=size (" << index << ">=" << container::size() << endl;
        printStackTrace();
        abort();
      }
      while(idx<index) {
        ++idx;
        ++ptr;
      }
      while(idx>index) {
        --idx;
        --ptr;
      }
      return ptr->second;
    }
    D& operator[](const K &key) {
      container *p = this;
      D &d( (*p)[key] );
      idx = 0;
      ptr = container::begin();
      return d;
    }
    void clear() {
      container::clear();
      GAbstractTableModel<D>::sigChanged();
    }
};

typedef GSTLSet<set<string>, string> TStringSet;

/**
 * \ingroup table
 *
 * Render items by printing them as a 'string'.
 */
template <class T>
class GTableCellRenderer_String:
  public TAbstractTableCellRenderer
{
  private:
    GSmartPointer<T> model;
    typedef GTableCellRenderer_String<T> This;
    
  public:
    GTableCellRenderer_String(T *m) {
      setModel(m);
    }
    ~GTableCellRenderer_String() {
      setModel(0);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    T * getModel() {
      return model;
    }
    void setModel(T *m) {
      if (model)
        disconnect(model->sigChanged, this);
      model = m;
      if (model)
        connect(model->sigChanged, this, &This::modelChanged);
    }
    int getRows() {
      if (!model)
        return 0;
      return model->getRows();
    }
    int getCols() {
      if (!model)
        return 0;
      return model->getCols();
    }
    int getRowHeight(int) {
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int) {
      if (!model)
        return 0;
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(
          model->getElementAt(0, i)
        );
        if (w>max)
          max = w;
      }
      return max+2;
    }
    void renderItem(TPen &pen, int x, int y, int w, int h, bool cursor, bool selected, bool focus) {
      pen.drawString(1, 1, model->getElementAt(x, y));
    }
};

typedef GTableCellRenderer_String<TStringVector> TTableCellRenderer_StringVector;
typedef GTableCellRenderer_String<TStringSet> TTableCellRenderer_StringSet;

/**
 * \ingroup table
 * Render items by printing the result of their 'toText(int)' method.
 */
template <class T, unsigned COLS>
class GTableCellRenderer_Text:
  public TAbstractTableCellRenderer
{
  private:
    GSmartPointer<T> model;
    typedef GTableCellRenderer_String<T> This;
    
  public:
    GTableCellRenderer_Text(T *m) {
      setModel(m);
    }
    ~GTableCellRenderer_Text() {
      setModel(0);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    T * getModel() {
      return model;
    }
    void setModel(T *m) {
      if (model)
        disconnect(model->sigChanged, this);
      model = m;
      if (model)
        connect(model->sigChanged, this, &This::modelChanged);
    }
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return COLS;
    }
    int getRowHeight(int) {
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int) {
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(
          model->getElementAt(0, i).toText(i)
        );
        if (w>max)
          max = w;
      }
      return max+2;
    }
    void renderItem(TPen &pen, int x, int y, int w, int h, bool cursor, bool selected, bool focus) {
      pen.drawString( 1, 1, model->getElementAt(0, y).toText(x));
    }
};

/**
 * \ingroup table
 * Render items by printing the result of their 'toText(int)' method.
 *
 * \note
 *   This generic uses a pointer to access 'toText' a better solution
 *   might be to move the pointer unreferencing into the container (wrapper)
 *   class.
 */
template <class T, unsigned COLS>
class GTableCellRenderer_PText:
  public TAbstractTableCellRenderer
{
  private:
    GSmartPointer<T> model;
    typedef GTableCellRenderer_String<T> This;
    
  public:
    GTableCellRenderer_PText(T *m) {
      setModel(m);
    }
    ~GTableCellRenderer_PText() {
      setModel(0);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    T * getModel() {
      return model;
    }
    void setModel(T *m) {
      if (model)
        disconnect(model->sigChanged, this);
      model = m;
      if (model)
        connect(model->sigChanged, this, &This::modelChanged);
    }
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return COLS;
    }
    int getRowHeight(int) {
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int) {
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = TOADBase::getDefaultFont().getTextWidth(
          model->getElementAt(0, i)->toText(i)
        );
        if (w>max)
          max = w;
      }
      return max+2;
    }
    void renderItem(TPen &pen, int x, int y, int w, int h, bool cursor, bool selected, bool focus) {
      pen.drawString( 1, 1, model->getElementAt(0, y)->toText(x));
    }
};

/**
 * \ingroup table
 *
 */
template <class T, int WIDTH>
class GTableRowRenderer:
  public TAbstractTableCellRenderer
{
  private:
    GSmartPointer<T> model;
    typedef GTableRowRenderer<T, WIDTH> This;
    
  public:
    GTableRowRenderer(T *m) { 
      setModel(m);
      per_row = true;
    }
    ~GTableRowRenderer() {
      setModel(0);
    }
    // implements 'virtual TAbstractTableModel * getModel() = 0;'
    T * getModel() {
      return model;
    }
    void setModel(T *m) {
      if (model)
        disconnect(model->sigChanged, this);
      model = m;
      if (model)
        connect(model->sigChanged, this, &This::modelChanged);
    }
    int getRows() {
      return model->getRows();
    }
    int getCols() {
      return WIDTH;
    }
    int getRowHeight(int) {
      return TOADBase::getDefaultFont().getHeight()+2;
    }
    int getColWidth(int col) {
      int n = model->getRows();
      int max = 0;
      for(int i=0; i<n; i++) {
        int w = model->getElementAt(0, i).getColWidth(col);
        if (w>max)
          max = w;
      }
      return max+2;
    }
    void renderItem(TPen &pen, int col, int index, int w, int h, bool cursor, bool selected, bool focus) {
      model->getElementAt(0, index).renderItem(pen, col, w, h);
    }
};

/**
 * \ingroup table
 * A wrapper for STL random access containers like vector or deque.
 *
 * \code
typedef GSTLRandomAccess<deque<string>, string> TStrings;
TStrings strings;
strings.push_back("Hello");
strings.push_back("You");
table->setRenderer(new GTableCellRenderer_String<TStrings>(&strings));
   \endcode
 */
template <class CONTAINER, class TYPE>
class GSTLRandomAccess:
  public CONTAINER,
  public GAbstractTableModel<TYPE>
{
  public:
    typedef typename CONTAINER::iterator iterator;
  
    int getRows() {
      return CONTAINER::size();
    }
     
    const TYPE&
    getElementAt(int, int index) {
      assert(index>=0 && index<(int)CONTAINER::size());
      return (*this)[index];
    }
    void push_back(const TYPE &s) {
      CONTAINER::push_back(s);
      GAbstractTableModel<TYPE>::sigChanged();
    }
    iterator insert(iterator p, const TYPE &s) {
      iterator r = CONTAINER::insert(p, s);
      GAbstractTableModel<TYPE>::sigChanged();
      return r;
    }
};   

// list, slist, ...
// GSTLTraversalAccess


} // namespace toad

#endif
