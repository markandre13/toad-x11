/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
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

#ifndef _TOAD_LAYOUT_HH
#define _TOAD_LAYOUT_HH 1

#include <toad/io/serializable.hh>
#include <toad/control.hh>

namespace toad {

class TPen;

class TLayoutEditor:
  public TControl
{
    typedef TControl super;
  public:
    TLayoutEditor(TWindow *parent, const string &title);
    virtual TEventFilter * getFilter();
};

class TLayout:
  public TEventFilter, public TSerializable
{
    friend class TWindow;
    typedef TSerializable super;
  public:
    TLayout();
    TLayout(const TLayout&);
    void toFile();
  
    virtual void arrange();
    
    virtual void paint();
  
    virtual TLayoutEditor * createEditor(TWindow *inWindow, TWindow *forWindow);

    bool restore(TInObjectStream&);
    
    /**
     * Set a filename for storage.
     *
     * TLayout will serialize itself when the filename isn't empty
     * and isModified() returns 'true'. 
     *
     * \param name A filename for usage with toad::urlstream.
     * \sa toad:urlstream
     */
    void setFilename(const string &name) { filename = name; }
    const string& getFilename() const { return filename; }
    
    /**
     * Mark the layout as modified or not modified.
     *
     * This method is used by the layout editor.
     *
     * \sa setFilename.
     */
    void setModified(bool b) { modified = b; }
    bool isModified() const { return modified; }
    TWindow * getWindow() const { return window; }

  protected:    
    /**
     * Window this layout is associated with (set by TWindow::setLayout)
     */
    TWindow *window;
    
    bool modified;
    string filename;
};

} // namespace toad

#endif
