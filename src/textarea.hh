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

#ifndef _TOAD_TEXTAREA_HH
#define _TOAD_TEXTAREA_HH

#include <toad/toad.hh>
#include <toad/control.hh>
#include <toad/textmodel.hh>
#include <toad/scrollbar.hh>

namespace toad {

class TTextArea:
  public TControl
{
    typedef TControl super;
    void init();
  public:
    class TBlink;
    friend class TBlink;

  public:
    class TPreferences {
      public:
        TPreferences();
        ~TPreferences();
        //! auto indention after carriage return
        bool autoindent:1;
        //! use spaces instead of tabulator characters
        bool notabs:1;
        //! view tabulator characters on output
        bool viewtabs:1;
        //! don't allow newline characters (for TTextField)
        bool singleline:1;
        //! print dummy characters instead of real input to hide passwords
        bool password:1;
        //! tabulator width to use to display tabulator characters
        unsigned tabwidth;
        
        enum EMode {
          NORMAL,
          WORDSTAR
        } mode;
        
        void setFont(const string &aFontname) { fontname = aFontname; }
        const string& getFont() const { return fontname; }
      protected:
        string fontname;
    };
    
    TPreferences *preferences;
    
    void setPreferences(TPreferences *aPreference) { preferences = aPreference; }
    TPreferences * getPreferences() const { return preferences; }
  
    static const bool debug_modelchanged = true;


    TTextArea(TWindow *p, const string &t):
      super(p, t)
    {
      vscroll = 0;
#ifndef OLDLIBSTD
      setModel(new TTextModel());
#endif
      init();
    }
    TTextArea(TWindow *p, const string &t, TTextModel *m):
      super(p, t)
    {
      vscroll = 0;
#ifndef OLDLIBSTD
      setModel(m);
#endif
      init();
    }
    template <class T> 
    TTextArea(TWindow *p, const string &t, T * m):
      super(p, t)
    {
      vscroll = 0;
#ifdef OLDLIBSTD
      setModel(m);
#endif
      init();
    }
    ~TTextArea();

    void keyDown(TKey, char*, unsigned);
    void mouseLDown(int, int, unsigned);
    void mouseMDown(int, int, unsigned);
    void focus(bool);

    /**
     * This signal is triggered when the TTextArea or its TTextModel
     * changes.
     */
    TSignal sigStatus;
    
  protected:
    //! Pointer to the text we're editing.
    PTextModel model;
    
    //! Called by the model when it was changed.
    void modelChanged();
    
    void adjustScrollbars();
    TRectangle visible;
    TScrollBar *vscroll, *hscroll;
    void scrolled();
    
    void resize();
    void paint();
    
    void _invalidate_line(unsigned line, bool statusChanged=true);

    //! position of the window's upper left char inside data
    unsigned _tx, _ty;
    
    //! cursor position (relative to screen)
    int _cx, _cy;

    //! last and first char of current line inside data
    unsigned _bol, _eol;
    
    //! position of cursor inside data (_bol <= _pos <= _eol)
    unsigned _pos;
    
    //! begin and end of selection
    unsigned _bos, _eos;
    
    // methods to handle keyboard input
    void _cursor_left(unsigned n=1);
    void _cursor_right(unsigned n=1);
    void _cursor_up(unsigned n=1);
    void _cursor_down(unsigned n=1);
    void _cursor_home();
    void _cursor_end();
    void _page_down();
    void _page_up();
    void _return();
    void _delete();
    void _backspace();

    void _selection_erase();
    void _selection_cut();
    void _selection_copy();
    void _selection_paste();
    void _selection_clear();
    void _delete_current_line();
    
    void _undo();
    void _redo();
    
    // methods to handle screen
    void _catch_cursor();
    void _scroll_down(unsigned n=1);
    void _scroll_up(unsigned n=1);
    void _scroll_left(unsigned n=1);
    void _scroll_right(unsigned n=1);
    
    //! insert a single character at cursor position (_cx, _cy)
    void _insert(int c);
    
    //! insert string at cursor position (_cy, _cy)
    void _insert(const string&);
    
    // doesn't work's during REMOVE
    void _eol_from_bol()
    {
      _eol = model->getValue().find('\n', _bol);
      if (_eol==string::npos)
        _eol = model->getValue().size();
    }
    
    void _set_model(TTextModel*);

  public:
    void setModel(int) {
      _set_model(0);
    }
  
    template <class T>
    void setModel(T * m) {
      TTextModel *mo = 0;
      if (m)
        mo = createTextModel(m);
      _set_model(mo);
    }

    TTextModel *getModel() const { return model; }
    
    void setModified(bool);
    bool isModified() const;
    
    void setValue(const string&);
    void setValue(const char *data, unsigned len);
    const string& getValue() const;
    
    void setCursor(unsigned x, unsigned y);
    unsigned getCursorX() const;
    unsigned getCursorY() const;
    
    unsigned gotoLine(unsigned);
    void find(const string&);
    unsigned getLines() const;
};

} // namespace toad

#endif
