/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2007 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_DRAGNDROP_HH
#define _TOAD_DRAGNDROP_HH 1

#include <string>
#include <vector>
#include <toad/toad.hh>
#include <toad/pointer.hh>

namespace toad {

// these values might be or'ed only for TDnDObject::SetType(...)
// (or when an action is selected during TDropSite::dropRequest?)
/**
 * @ingroup dnd
 *@{
 */
static const unsigned ACTION_NONE     = 0;
//! Copy
static const unsigned ACTION_COPY     = 1;
static const unsigned ACTION_MOVE     = 2;
static const unsigned ACTION_LINK     = 4;
static const unsigned ACTION_ASK      = 8;
static const unsigned ACTION_PRIVATE  = 16;
/*@}*/

/**
 * @ingroup dnd
 *
 * Xdnd3 uses MIME types as defined in RFC 1341. The class is a helper
 * to handle them.
 */
class TDnDType:
  public TSmartObject
{
  public:
    TDnDType(const string &mime, unsigned actions=0);
    string mime;    // the mime type as a whole
    
    string major;   // content type
    string minor;   // sub-type
    // no MIME parameters are used for DnD
    
    bool wanted;
    unsigned actions;
};
typedef GSmartPointer<TDnDType> PDnDType;
typedef vector<PDnDType> TDnDTypeList;

/**
 * @ingroup dnd
 * Base class to encapsulate data when initiating Drag'n Drop.
 */
struct TDnDObject:
  public TSmartObject
{
  TDnDObject();
  virtual ~TDnDObject();
  
  string flatdata;

  //! Store your data in <VAR>flatdata</VAR> using type <VAR>type</VAR>.
  virtual void flatten();
  TDnDTypeList typelist;
  
  PDnDType type;
  int x,y;

  void setType(const string &mime, unsigned);

  static bool select(TDnDObject &drop, const string &major, const string &minor);

  unsigned action;    // set by TDropSite::dropRequest
  bool local;         // drop is within this application
};

typedef GSmartPointer<TDnDObject> PDnDObject;

/**
 * @ingroup dnd
 * Base class to receive data during Drag'n Drop.
 */
class TDropSite
{
  public:
    TDropSite(TWindow*, TRectangle const&);
    TDropSite(TWindow*);
    virtual ~TDropSite();
    
    const TRectangle& getShape();
    void setShape(int x, int y, int w, int h);
    void setShape(const TRectangle &);
    TWindow* getParent() const { return parent; }
    
    virtual void dropRequest(TDnDObject&) = 0;
    virtual void drop(TDnDObject&) = 0;
    virtual void leave();
    virtual void paint();
  protected:
    void init();
    TWindow *parent;
    
    bool use_parent;
    TRectangle rect;
};

} // namespace toad

#endif
