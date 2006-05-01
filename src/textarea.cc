/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/textarea.hh>
#include <toad/simpletimer.hh>
#include <toad/undomanager.hh>
#include <toad/action.hh>
#include <toad/utf8.hh>
#include <cstdio>
#include <assert.h>

#if 0
#define DBM(M) M
#define MARK { cout << __PRETTY_FUNCTION__ << endl; }
#else
#define DBM(M)
#define MARK
#endif

using namespace toad;

TTextModel *
toad::createTextModel(TTextModel *m)
{
  return m;
}

/**
 * \ingroup control
 * \class toad::TTextArea
 *
 * A widget to edit text in multiple lines.
 *
 * Multiple TTextModel's can share one TTextModel.
 *
 * @sa TTextModel
 *
 * \todo
 *   \li update scrollbar after setModel
 *   \li when cursor is at the end of a long line, moves up into a
 *       shorter line, we're in a broken state
 *   \li when possible, we should scroll text up from the buttom when
 *       deleting multiple lines
 *   \li clear old selection on insert and make inserted text the new
 *       selection
 *   \li horizontal scrolling starts too late when line contains tabs
 *   \li infinite loop in a situation when trying to select from bol to
 *       eol and eol is outside the visible area (fixed?)
 *   \li 'caught exception in toad::TOADBase::runApp:
 *       basic_string::substr' when selecting and scrolling (fixed?)
 *   \li when the whole line is selected, invert the whole line from the
 *       left to the right side of the window
 *   \li scrollbars (optional ones)
 *   \li non-monospaced characters
 *   \li later: control characters for multiple colors, fonts, etc.
 *   \li improve page up & down
 *   \li improve scrolling
 *   \li improve screen updates
 */

/**
 * Timer for blinking caret which is shared by all textareas.
 */
class TTextArea::TBlink:
  public TSimpleTimer   
{
  public:
  TBlink() {
    current = NULL;
  }
  TTextArea *current;
  bool visible;           // true: draw text cursor
  bool blink;             // blink state (it would be better to sync the timer
                          // every keyDown & mouseLDown event!)
  void tick();
};
  
TTextArea::TBlink blink;

void
TTextArea::TBlink::tick()
{
//return;
  if (!current) {      // sanity check, not needed
    cout << "  no current => " << endl;
    return;
  }
  blink = !blink;
  if (visible!=blink) {
    visible = blink;   
    current->_invalidate_line(current->_cy, false);
    current->paintNow();
  }
}  

TTextArea::TPreferences::TPreferences()
{
  mode = NORMAL;
  autoindent = true;
  notabs = true;
  viewtabs = true;
  tabwidth = 8;
  singleline = false;
  password = false;
  fontname = "arial,helvetica,sans-serif:size=12";
}

TTextArea::TPreferences::~TPreferences()
{
}

/**
 * Helper method for TTextArea's constructors.
 */
void
TTextArea::init()
{
  _bol = 0;
  if (model)
    _eol_from_bol();
  else
    _eol = 0;
  _bos = _eos = 0;
  _cx = _cy = 0;
  _cxpx = -1;
  _ty = 0;
  _tx = 0;
  _pos = 0;
  
  bDoubleBuffer = true;
  bTabKey = true;
  setBorder(0);
  vscroll = hscroll = 0;
  
  setPreferences(new TPreferences());
  
  TAction *action;

  action = new TAction(this, "edit|cut");
  CONNECT(action->sigClicked, this, _selection_cut);
  action = new TAction(this, "edit|copy");
  CONNECT(action->sigClicked, this, _selection_copy);
  action = new TAction(this, "edit|paste");
  CONNECT(action->sigClicked, this, _selection_paste);
  action = new TAction(this, "edit|delete");
  CONNECT(action->sigClicked, this, _selection_erase);
}

TTextArea::~TTextArea()
{
  if (blink.current==this) {
    blink.stopTimer();  
    blink.current=NULL;
  }
  if (model) {
    disconnect(model->sigTextArea, this);
    disconnect(model->sigMeta, this);
  }
  setPreferences(0);
}

void
TTextArea::keyDown(TKey key, char* str, unsigned modifier)
{
  if (!isEnabled())
    return;

DBM(cout << "ENTER keyDown '" << str << "'" << endl;
    cout << "  _cx, _cy        : " << _cx << ", " << _cy << endl;
    cout << "     , _ty        : " << ", " << _ty << endl;
    cout << "  _bol, _pos, _eol: " << _bol << ", " << _pos << ", " << _eol << endl;)

  // MacOS alike keybindings
  if (modifier & MK_CONTROL) {
    switch(key) {
      case 'x':
      case 'X':
      case TK_DELETE:
        _selection_cut();
        return;
      case 'c':
      case 'C':
      case TK_INSERT:
        _selection_copy();
        return;
      case 'v':
      case 'V':
        _selection_paste();
        return;
      case 'y':case 'Y': // remove current line (wordstar)
        if (preferences->mode==TPreferences::NORMAL)
          _selection_clear();
        _delete_current_line();
        return;
      case '_':
      case 'z':
      case 'Z':
        if (preferences->mode==TPreferences::NORMAL)
          _selection_clear();
//        TUndoManager::undo();
        return;
      case 'r':
      case 'R':
        if (preferences->mode==TPreferences::NORMAL)
          _selection_clear();
//        TUndoManager::redo();
        return;
      case 'a':
      case 'A':
        if (preferences->mode==TPreferences::NORMAL)
          _selection_clear();
        _cursor_home();
        break;
      case 'e':
      case 'E':
        if (preferences->mode==TPreferences::NORMAL)
          _selection_clear();
        _cursor_end();
        break;
    }
  }
  
  if (modifier & MK_SHIFT) {
    switch(key) {
      case TK_INSERT:
        switch(preferences->mode) {
          case TPreferences::NORMAL:
            _selection_clear();
            _selection_paste();
            break;
          case TPreferences::WORDSTAR: {
            size_t oldpos = _pos;
            _selection_paste();
            size_t l = _eos - _bos; // _eos and _bos were sorted by 'paste()'
            _bos = oldpos;
            _eos = _bos + l;
            } break;
        } return;
      case TK_LEFT:
      case TK_RIGHT:
      case TK_UP:
      case TK_DOWN:
      case TK_HOME:
      case TK_END:
      case TK_PAGEDOWN:
      case TK_PAGEUP:
        if (_eos!=_pos) {
          invalidateWindow();
          _bos = _eos = _pos;
//cerr << "start selection at _pos = " << _pos << endl;
        }
        break;
    }
  }

  switch(key) {
    case TK_LEFT:
      if (!(modifier&MK_SHIFT) && preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _cursor_left();
      break;
    case TK_RIGHT:
      if (!(modifier&MK_SHIFT) && preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _cursor_right();
      break;
    case TK_UP:
      model->filter(TTextModel::CHARACTER_CURSOR_UP);
      if (!(modifier&MK_SHIFT) && preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _cursor_up();
      break;
    case TK_DOWN:
      model->filter(TTextModel::CHARACTER_CURSOR_DOWN);
      if (!(modifier&MK_SHIFT) && preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _cursor_down();
      break;
    case TK_HOME:
      if (!(modifier&MK_SHIFT) && preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _cursor_home();
      break;
    case TK_END:
      if (!(modifier&MK_SHIFT) && preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _cursor_end();
      break;
    case TK_RETURN:
    case TK_KP_RETURN:
      if (!preferences->singleline) {
        if (preferences->mode==TPreferences::NORMAL)
          _selection_clear();
        _return();
      } else {
        // some models, ie TBoundedRangeTextModel or TColorTextModel
        // want perform some action when RETURN is pressed and need to
        // be informed
        model->filter('\n'); 
      }
      break;
    case TK_DELETE:
      switch(preferences->mode) {
        case TPreferences::NORMAL:
          if (_bos != _eos)
            _selection_erase();
          else
            _delete();
          break;
        case TPreferences::WORDSTAR:
          _delete();
          break;
      }
      break;
    case TK_BACKSPACE:
      switch(preferences->mode) {
        case TPreferences::NORMAL:
          if (_bos != _eos)
            _selection_erase();
          else
            _backspace();
          break;
        case TPreferences::WORDSTAR:
          _backspace();
          break;
      }
      break;
    case TK_PAGEDOWN:
      if (preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _page_down();
      break;
    case TK_PAGEUP:
      if (preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _page_up();
      break;
    case TK_TAB:
      if (preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      if (preferences->notabs) {
        unsigned m = preferences->tabwidth -
                      (utf8charcount(model->getValue(), _bol, _pos-_bol)
                      % preferences->tabwidth);
        string s;
        s.replace(0,0, m, ' ');
        _insert(s);
      } else {
        _insert("\t");
      }
      break;
    default:
      if ( (unsigned char)str[0]>=32 || 
           (str[0]!=0 && str[1]!=0) ) 
      {
//cout << "insert " << strlen(str) << endl;
//cout << (unsigned char)str[0] << endl;
        _insert(str);
      }
//      else
//        printf("unhandled character '%s'\n", str);
  }

  switch(key) {
    case TK_LEFT:
    case TK_RIGHT:
    case TK_UP:
    case TK_DOWN:
    case TK_HOME:
    case TK_END:
    case TK_RETURN:
    case TK_PAGEDOWN:
    case TK_PAGEUP:
      if (modifier & MK_SHIFT)
        _eos = _pos;
      break;
  }

DBM(cout << "LEAVE keyDown" << endl;
    cout << "  _cx, _cy        : " << _cx << ", " << _cy << endl;
    cout << "  _ty             : " << _ty << endl;
    cout << "  _bol, _pos, _eol: " << _bol << ", " << _pos << ", " << _eol << endl;)
}

void
TTextArea::mouseLDown(int x, int y, unsigned)
{
  if (!isEnabled())
    return;
  _goto_pixel(x, y);
  _bos = _eos = _pos;
  blink.visible=true;
  setFocus();
}

void
TTextArea::mouseMove(int x, int y, unsigned)
{
  if (!isEnabled())
    return;
  _goto_pixel(x, y);
  if (_eos != _pos) {
    _eos = _pos;
    invalidateWindow(true);
    // _invalidate_line(_cy);
  }
}

void
TTextArea::mouseLUp(int x, int y, unsigned)
{
}

/**
 * Set cursor based on pixel coordinates.
 */
void
TTextArea::_goto_pixel(int x, int y)
{
x-=2-_tx;
y-=2;
  TFont *font = TPen::lookupFont(preferences->getFont());
  int h = font->getHeight();
  y /= h;

  setCursor(getCursorX(), y + _ty); // outch! overhead!

//  string line = model->getValue().substr(_bol, _eol==string::npos ? _eol : _eol-_bol);
  string line;
  int sx;
  size_t d2, d3;
  _get_line(&line, _bol, _eol, &sx, &d2, &d3);
//  cerr << "found line '" << line << "'\n";

  int w1 = 0, w2 = 0;
  size_t p;
  unsigned cx;
  for(p=0, cx=0; p<line.size(); utf8inc(line, &p), cx++) {
    w2 = font->getTextWidth(line.substr(0, p));
    if (w2>x)
      break;
    w1 = w2;
  }
  if (p==line.size()) {
    w2 = font->getTextWidth(line);
    if (w2<=x)
      w1 = w2;
  }
//cerr << "x-w1=" << (x-w1) << ", w2-x=" << (w2-x) << endl;

  if ( x-w1 < w2-x ) {
    utf8dec(line, &p);
    _cxpx = w1;
    cx--;
  } else {
    _cxpx = w2;
  }
  
//  cerr << "w1 = " << w1 << ", w2 = " << w2 << endl;
  
//cerr << "position " << p << ", " << y << endl;

  setCursor(cx, y + _ty);
}

void
TTextArea::focus(bool b)
{
  b = b && isEnabled();
  model->focus(b);
//cout << getTitle() << ".focus "<<isFocus()<<endl;
  _invalidate_line(_cy);
  if (b) {
    blink.current=this;
    blink.visible=true;
    blink.blink=true;
    blink.startTimer(0,500000);
  } else {
    blink.stopTimer(); 
    blink.current=NULL;
  }
}

/**
 * Helper method to set a new model.
 */
void
TTextArea::_set_model(TTextModel *m)
{
  if (model) {
    disconnect(model->sigTextArea, this);
    TUndoManager::unregisterModel(this, model);
  }
  model = m;
  _cx = 0;
  _cxpx = -1;
  _cy = 0;
  _ty = 0;
  _bol = 0;
  _pos = 0;
  if (model) {
    connect(model->sigTextArea, this, &TTextArea::modelChanged);
    connect(model->sigMeta    , this, &TTextArea::modelMeta);
    _eol_from_bol();
    TUndoManager::registerModel(this, model);
  }
  _bos = _eos = 0;
  if (vscroll)
    vscroll->setValue(_ty);
  adjustScrollbars();
  invalidateWindow(true);
}

void
TTextArea::modelMeta()
{
  if (!model) {
    cout << "toad: error text area '" << getTitle() << "' received "
            "sigMeta but has no model" << endl;
    return;
  }
  if (model->meta == TModel::META_ENABLED) {
#if 1
    if (blink.current==this) {
      blink.stopTimer();
      blink.current = NULL;
    }
    invalidateWindow();
#else
    setFocus(false);
#endif
  }
}

//#undef DBM
//#define DBM(CMD) CMD

#ifdef TOAD_TEXTAREA_CHECK
void
checkCursor2(TTextArea *ta)
{
  size_t pos = ta->getPos();
  unsigned x, y;
  x = y = 0;
  const string &str = ta->getModel()->getValue();
  for(size_t i=0; i<pos; ++i) {
    ++x;
    if (str[i]=='\n') {
      x=0;
      ++y;
    }
  }
  if (x!=ta->getCursorX() || y!=ta->getCursorY()) {
    cout << "str   = '" << str << "'\n";
    cout << "wrong cursor position: textarea is at (" 
         << ta->getCursorX() << ", " << ta->getCursorY()
         << ") but pos " << pos << " is at (" 
         << x << ", " << y << ")\n";
    exit(0);
  }
}

void
checkCursor3(TTextArea *ta, size_t offset, size_t size)
{
  size_t pos = ta->getPos();
  unsigned x, y;
  x = y = 0;
  string str = ta->getModel()->getValue();
  str.erase(offset, size);
  for(size_t i=0; i<pos; ++i) {
    ++x;
    if (str[i]=='\n') {
      x=0;
      ++y;
    }
  }
  if (x!=ta->getCursorX() || y!=ta->getCursorY()) {
    cout << "str   = '" << str << "'\n";
    cout << "wrong cursor position: textarea is at (" 
         << ta->getCursorX() << ", " << ta->getCursorY()
         << ") but pos " << pos << " is at (" 
         << x << ", " << y << ")\n";
    exit(0);
  }
}
#endif

/**
 * Update view.
 *
 * This method is called when the model has changed so that the
 * text area can update it's view.
 */
void
TTextArea::modelChanged()
{
  if (!isRealized())
    return;
/*
  if (model->length==0) {
    adjustScrollbars();
    invalidateWindow();
    sigStatus();
    return;
  }
*/
  int oldcx=_cx;

  // check 'blink.current' before modifing blink.visible
  // we might not own the cursor
  if (blink.current==this)
    blink.visible=true;
DBM(static unsigned opcount=0;
    opcount++;
    cout << "enter modelChanged (" << getTitle() << ")" << endl
         << "  model->offset=" << model->offset << endl
         << "  model->length=" << model->length << endl
         << "  model->lines =" << model->lines << endl
         << "  _cx, _cy (" << _cx << ", " << _cy << ")" << endl
         << "  _ty (" << _ty << ")" << endl
         << "  _bol, _eol   (" << _bol << ", " << _eol << ")" << endl
         << "  _pos " << _pos << endl
         << " value='" << model->getValue() << "'\n"
         << "  opcount=" << opcount << endl;
if (opcount==591) {
//  cout << "UPSI DAISY\n";
})
  switch(model->type) {
    case TTextModel::CHANGE:
      _cx = 0;
      _cy = 0;
      _cxpx = -1;
      _ty = 0;
      _bol = 0;
      _pos = 0;
      _eol_from_bol();
      _bos = _eos = 0;
      if (vscroll)
        vscroll->setValue(_ty);
      break;
    case TTextModel::INSERT:
      {
        bool inside_current_line = false;
        if (_bol <= model->offset && model->offset <=_eol)
          inside_current_line=true;

        bool before_current_line = false;
        if (model->offset+model->length < _bol)
          before_current_line = true;
          
        bool before_cursor = false;
        if (model->offset <= _pos)
          before_cursor = true;

        // update _pos, _bol and _eol
        if (model->offset <= _pos) _pos+=model->length;
        if (model->offset <  _bol) _bol+=model->length;
        if (model->offset <= _eol) _eol+=model->length;
        if (_bos != _eos) {
          if (model->offset <  _bos) _bos+=model->length;
          if (model->offset <= _eos) _eos+=model->length;
        }

        if (before_current_line) {
          _ty += model->lines;
          if (vscroll)
            vscroll->setValue(_ty);
        } else {
          if (before_cursor)
            _cy += model->lines;
        }
        
        if (inside_current_line) {
          const string &s = model->getValue();

          if (_pos > 0) {
          _bol = s.rfind('\n', _pos-1);
          if (_bol==string::npos)
            _bol=0;
          else
            _bol++;
          } else _bol = 0;
            
          _eol = s.find('\n', _pos);
          if (_eol==string::npos)
            _eol=s.size();
            
          _cx = utf8charcount(s, _bol, _pos - _bol);
         }
        _cxpx = -1;
        _catch_cursor();
      }
#ifdef TOAD_TEXTAREA_CHECK
      checkCursor2(this);
#endif
      break;

    // REMOVE is called before the section to be removed is actually
    // removed from the model
    case TTextModel::REMOVE:
      {
        const string s(model->getValue());
        _bos = _eos = 0;
        size_t m1 = model->offset;
        size_t m2 = model->offset+model->length;
        
        DBM(
          cout << "  REMOVE: offset=" << model->offset << endl
               << "          length=" << model->length << endl;
        )
        
        // update _cy, _ty
        
        size_t n = 0;
        if (m2 <= _bol) {
          n = model->lines;
        } else
        if (m1 < _bol && _bol < m2) {
          n = 0;
          for(size_t i=m1; i<=_bol; ++i) {
            if (s[i] == '\n')
              n++;
          }
        }
        if (n>0) {
          if (n<=_ty) {
            _ty -= n;
          } else {
            _cy -= n - _ty;
            _ty = 0;
          }
          if (vscroll)
            vscroll->setValue(_ty);
        }
        
        // update _pos, _bol and _eol
        
        // _bol
        if (m2 < _bol) {               // (A) _bol before sel.
          _bol -= model->length;
        } else
        if (m1 < _bol && _bol <= m2) { // (B) _bol inside sel.
          if (m1==0) {
            _bol = 0;
          } else {
            _bol = s.rfind('\n', m1-1);
            if (_bol==string::npos) {
              _bol=0;
            } else {
              _bol++;
            }
          }
        }
        
        // _pos
        if (m2 < _pos) {               // (C) _pos before sel.
          _pos -= model->length;
        } else
        if (m1 < _pos && _pos <= m2) { // (D) _pos inside sel.
          _pos = model->offset;
        }
        
        DBM(cout << "m1, m2 = " << m1 << ", " << m2 << endl;)
        DBM(cout << "_eol   = " << _eol << endl;)
        // _eol
        if (m2 < _eol) {               // (E) _eol before sel.
          DBM(cout << __FILE__ << ':' << __LINE__ << endl;)
          _eol -= model->length;
        } else
        if (m1 <= _eol && _eol <= m2) { // (F) _eol inside sel.
          _eol = s.find('\n', m2);
          if (_eol==string::npos) {
            _eol = s.size();
          }
          DBM(cout << "_eol   = " << _eol << endl;)
          _eol -= model->length;
          DBM(cout << "_eol   = " << _eol << endl;)
        }
        DBM(cout << "_eol   = " << _eol << endl;)

        _cx = utf8charcount(s, _bol, _pos - _bol);
        _cxpx = -1;
        _catch_cursor();
      }
#ifdef TOAD_TEXTAREA_CHECK
      checkCursor3(this, model->offset, model->length);
#endif
      break;
  }
DBM(cout << "leave modelChanged (" << getTitle() << ")" << endl;
    cout << "  _cx, _cy (" << _cx << ", " << _cy << ")" << endl;
    cout << "  _ty (" << _ty << ")" << endl;
    cout << "  _bol, _eol (" << _bol << ", " << _eol << ")" << endl;
    cout << "  _pos " << _pos << endl;
    cout << "----------------------------------------------------" << endl;)

  if (oldcx!=_cx)
    _cxpx = -1;


  adjustScrollbars();
  invalidateWindow();
  sigStatus();
}

//#undef DBM
//#define DBM(CMD)

void
TTextArea::preferencesChanged()
{
  invalidateWindow();
}

void
TTextArea::adjustScrollbars()
{
  visible.set(2,2,getWidth()-4,getHeight()-4);
//  cerr << "'" << getTitle() << "', adjustScrollbars: visible = " << visible << endl;

  if (!preferences || preferences->singleline || !model) {
    if (vscroll) {
      vscroll->setMapped(false);
      vscroll->setValue(0);
    }
    return;
  }

  if (preferences->singleline)
    return;

  bool need_vscroll = false;

  // vertical visible lines
  TFont *font = TPen::lookupFont(preferences->getFont());
  if (font->getHeight()==0) {
    cout << "error: TTextArea::adjustScrollbars: font of height 0" << endl;
    return;
  }
  int vvl = visible.h/font->getHeight();
  if (vvl<model->nlines)
    need_vscroll = true;

  if (need_vscroll)
    visible.w -= TScrollBar::getFixedSize();
    
  if (need_vscroll) {
    if (!vscroll) {
      vscroll = new TScrollBar(this, "vertical");
      connect(vscroll->getModel()->sigChanged, this, &TTextArea::scrolled);
      vscroll->createWindow();
    }
//    cerr << "  '"  << getTitle() << "', visible = " << visible << endl;
//    printStackTrace();
    vscroll->bNoFocus=true;
    vscroll->setShape(
      getWidth()-TScrollBar::getFixedSize(), 0,
      TScrollBar::getFixedSize(), getHeight());
/*
    vscroll->setShape(
      visible.x+visible.w,
      visible.y,
      TScrollBar::getFixedSize(), 
      visible.h);
*/
    vscroll->setExtent(vvl);
    vscroll->setMinimum(0);
    vscroll->setMaximum(model->nlines);
    vscroll->setUnitIncrement(1);
    vscroll->setMapped(true);
  } else {
    if (vscroll) {
      vscroll->setMapped(false);
      vscroll->setValue(0);
    }
  }
//  cerr << "'" << getTitle() << "', leave" << endl;
}

/**
 * Method to handle scrollbar changes.
 */
void
TTextArea::scrolled()
{
  if (vscroll) {
    int dy = vscroll->getValue() - _ty;
    _ty += dy;
    _cy -= dy;
    invalidateWindow();
  }
}

/**
 * Helper method to retrieve a line from the model.
 *
 * This method also inserts whitespace for tabs.
 */
void
TTextArea::_get_line(string *line, 
          size_t bol, size_t eol,
          int *sx,
          size_t *bos, size_t *eos) const
{
  assert(line!=0);
  assert(sx!=0);
  *line = model->getValue().substr(bol, eol==string::npos ? eol : eol-bol);
      
  // set *bos < *eos
  //^^^^^^^^^^^^^^^^^^
  if (bos) {
    // _bos < _eos
    size_t _bos = this->_bos;
    size_t _eos = this->_eos;
    if (_bos > _eos) {
      size_t a = _bos;
      _bos = _eos;
      _eos = a;
    }
    *bos = _bos;
    *eos = _eos;
  }

  // substitute tabs with spaces
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  // line = line with tabs converted to spaces
  // sx   = cx in line with tabs converted to spaces
  *sx = _cx;
//cerr << "\n1 sx=" << *sx << endl;

  for(size_t i=0, j=0; 
    j<line->size();
    ++i, utf8inc(*line, &j))
  {
    if ((*line)[j]=='\t') {
      unsigned m = preferences->tabwidth - (i % preferences->tabwidth);
      if (!preferences->viewtabs) {
        line->replace(j, 1, m, ' ');
      } else {
//        line->replace(j, 1, m+1, '·');
//        line->replace(j, 1, 1  , '»');
        line->replace(j, 1, m, '.');
        line->replace(j, 1, 1, '|');
      }
#if 0
cout << "found tab:" << endl
     << "  _cx = " << _cx << endl
     << "  *sx = " << *sx << endl
     << "  i = " << i << endl
     << "  m = " << m << endl
     << "  bol = " << bol << endl
     << "  *bos = " << *bos << endl
     << "  *eos = " << *eos << endl
     ;
#endif
      --m;
      if (i<*sx)
        *sx+=m;
      if (bos) {
        if (*bos > bol && *bos-bol > j)
          *bos+=m; // utf8bytes
        if (*eos > bol && *eos-bol > j)
          *eos+=m; // utf8bytes
      }
      i+=m;
      j+=m;
    }
  }
//cerr << "2 sx=" << *sx << endl;

}

#if 0
unsigned
TTextArea::_screenx_to_cx(const string &line, unsigned sx)
{
  unsigned i, j;     
  for(i=0, j=0;
      j<line.size();
      ++j)     
  {
    unsigned m = 1;
    if (line[j]=='\t') {
      m = tabwidth - (i % tabwidth);         
    }
    if (i<=sx && sx<i+m)
      break;
    i+=m;   
  }
  return j;
}
#endif

/**
 * Paint the screen.
 * \todo
 *   Improve speed
 */
void
TTextArea::paint()
{
  TPen pen(this);
  TColor fillcolor;
  if (isEnabled())
    fillcolor.set(255,255,255);
  else
    fillcolor.set(TColor::DIALOG);
  pen.setFillColor(fillcolor);
  pen.fillRectangle(0,0,getWidth(),getHeight());
  
  pen.draw3DRectangle(
    visible.x-2, visible.y-2,
    visible.w+4-1, visible.h+4-1);
    
  if (!model) {
    pen.setColor(TColor::DIALOG);
    pen.fillRectanglePC(visible);
    return;
  }

  TRectangle clipbox;
  pen.getClipBox(&clipbox);

  pen &= visible;

  pen.translate(2,2);
  pen.setFont(preferences->getFont());

  const string &data = model->getValue();
  
  // paint the lines
  //^^^^^^^^^^^^^^^^^
  size_t bol = 0;
  size_t eol;
  int y=-_ty * pen.getHeight();
  int sy, sx;
  sy = -_ty;
  
  while(true) {
    eol = data.find('\n', bol);
    size_t n = eol==string::npos ? eol : eol-bol; // n=characters in line
    if (y+pen.getHeight()>=clipbox.y) { // loop has reached the visible area
//cerr << "line " << bol << "-" << eol << endl;

      string line;
      size_t bos, eos;
      _get_line(&line, bol, eol, &sx, &bos, &eos);
/*
// cerr << "draw line: '" << line << "'\n";
for(size_t i=0; i<line.size(); ++i) {
  int c=line[i];
  switch(c) {
    case '\t':
      cout << "\\t";
      break;
    default:
      cout << (char)c;
  }
}
cout << endl;
cerr << "  line     : " << bol << " - " << eol << endl;
cerr << "  selection: " << _bos << " - " << _eos << endl;
cerr << "  selection: " << bos << " - " << eos << endl;
*/
      // draw text
      bool part=false;
      
      size_t bos2, eos2;
      if (_bos < _eos) {
        bos2 = _bos;
        eos2 = _eos;
      } else {
        bos2 = _eos;
        eos2 = _bos;
      }
      
      if (bos2 <= bol && eol <= eos2) {
//cout << "    line is inside selection\n";
        // inside selection
        pen.setLineColor(fillcolor);
        pen.setFillColor(0,0,0);
      } else if (eol < bos2 || bol > eos2) {
//cout << "    line is outside selection\n";
        // outside selection
        pen.setLineColor(0,0,0);
        pen.setFillColor(fillcolor);
      } else {
//cout << "    line and selection true intersection\n";
        pen.setLineColor(0,0,0);
        pen.setFillColor(fillcolor);
        part = true;
      }
      pen.fillString(-_tx,y,line);
      
      if (part) {
#if 0
        cerr<<"bol  = "<<bol<<endl
            <<"_bos = "<<_bos<<endl
            <<"bos  = "<<bos<<endl
            <<"eol  = "<<eol<<endl
            <<"_eos = "<<_eos<<endl
            <<"eos  = "<<eos<<endl
            <<endl;
#endif            
        int x=0;
        size_t pos = 0;
        size_t len = eol-bol;
        if (bol < bos2) { // start is inside
//cerr << "start is inside" << endl;
          pos = bos-bol;
        }
        if (eos2 < eol) { // end is inside
//cerr << "end is inside" << endl;
          len = eos - bol;
        }
        if (bol < bos2) { // start is inside
//cerr << "start is inside" << endl;
          pos = bos-bol;
          len-=pos;
        }
        // cut pos & len to _tx
//cerr << "pos = " << pos << endl << "_tx = " << _tx << endl;
        x = pen.getTextWidth(line.substr(0,pos));
//cerr << "  \"" << line.substr(_tx,pos-_tx) << "\"" << endl;
//cerr << "x   = " << x << endl << "len = " << len << endl;
        if (len>0 && pos<line.size()) {
          pen.setLineColor(fillcolor);
          pen.setFillColor(0,0,0);
          pen.fillString(x-_tx, y, line.c_str()+pos, len);
        }
      }
      
      // draw cursor
      if (blink.visible && blink.current==this && sy==_cy) {
        pen.setColor(0,0,0);
//cerr << "2 sx=" << sx << endl;
//string l2 = line.substr(0, utf8bytecount(line, 0, sx));
//cerr << "'" << l2 << "'\n";
        if (_cxpx<0) {
          _cxpx = pen.getTextWidth(line.substr(0, utf8bytecount(line, 0, sx)));
        }
        pen.setMode(TPen::INVERT);
        pen.drawLine(_cxpx-_tx,y,_cxpx-_tx,y+pen.getHeight()-1);
        pen.setMode(TPen::NORMAL);
      }
    }
    if (eol==string::npos)
      break;
    y+=pen.getHeight();
    if (y>clipbox.y+clipbox.h)
      break;
    sy++;
    bol=eol+1;
  }
}

void
TTextArea::_invalidate_line(unsigned y, bool statusChanged)
{
  int h = TPen::lookupFont(preferences->getFont())->getHeight();
  invalidateWindow(visible.x, visible.y+y * h, visible.w, h);
  if (statusChanged)
    sigStatus();
}

void
TTextArea::_insert(const string &s)
{
  MARK
  if (s.empty())
    return;
  if (preferences->mode==TPreferences::NORMAL) {
    if (_bos != _eos)
      _selection_erase();
  }
  if (preferences->singleline) {
    size_t p = s.find('\n');
    if (p!=string::npos) {
      string s2(s.substr(0,p));
      model->insert(_pos, s2);
      return;
    }
  }
  model->insert(_pos, s);
}

void
TTextArea::_selection_erase()
{
  MARK
  if (_bos > _eos) {
    size_t a = _bos;
    _bos = _eos;
    _eos = a;
  }
  model->erase(_bos, _eos-_bos);
}

void
TTextArea::_selection_cut()
{
  MARK
  _selection_copy();
  model->erase(_bos, _eos-_bos);
}

void
TTextArea::_selection_copy()
{
  MARK
  if (_bos > _eos) {
    size_t a = _bos;
    _bos = _eos;
    _eos = a;
  }
  setSelection(model->getValue().substr(_bos, _eos-_bos));
//  cout << "'" << clipboard << "'" << endl;
}

void
TTextArea::_selection_paste()
{
  MARK
  _insert(getSelection());
}

void
TTextArea::_selection_clear()
{
  if (_bos==0 && _eos==0)
    return;
  _bos = _eos = 0;
  invalidateWindow();
}
void
TTextArea::_delete_current_line()
{
  MARK
  model->erase(_bol, _eol-_bol+1);
}

void
TTextArea::mouseMDown(int,int,unsigned)
{
  if (!isEnabled())
    return;
  _insert(getSelection());
}

void
TTextArea::_cursor_left(unsigned n)
{
  MARK
  for(unsigned i=0; i<n; ++i) {
    if (_pos>_bol) {
      utf8dec(model->getValue(), &_pos);
      if (_cx>0) {
        --_cx;
        _invalidate_line(_cy);
        _cxpx = -1;
        _catch_cursor();
      }
    } else 
    if (_cy+_ty>0) {
      _cursor_up();
      _cursor_end();
    }
  }
  blink.visible=true;
}

void
TTextArea::_cursor_right(unsigned n)
{
  MARK
//cerr << "cursor_right" << endl;
//cerr << "   _cx = " << _cx << endl;
//cerr << "  _pos = " << _pos << endl;
  for(unsigned i=0; i<n; ++i) {
    if (_pos<_eol) {
      ++_cx;
      utf8inc(model->getValue(), &_pos);
      _invalidate_line(_cy);
      _cxpx = -1;
      _catch_cursor();
    } else 
    if (_eol+1<model->getValue().size()) {
      _cursor_down();
      _cursor_home();
    }
  }
//cerr << "   _cx = " << _cx << endl;
//cerr << "  _pos = " << _pos << endl;
  blink.visible=true;
}

void
TTextArea::_cursor_down(unsigned n)
{
  MARK
  if (_cxpx == -1) {
    _cxpx_from_cx();
  }
  
  for(unsigned i=0; i<n; ++i) {
    if(_eol+1<model->getValue().size()) {
      _bol=_eol+1;
      _eol_from_bol();
      _invalidate_line(_cy);
      _cy++;
      _invalidate_line(_cy);
      _pos_from_cxpx();
    }
  }
  _catch_cursor();    
  blink.visible=true;
}

void
TTextArea::_cxpx_from_cx()
{
  string line;
  int sx;
  size_t d2, d3;
  _get_line(&line, _bol, _eol, &sx, &d2, &d3);
  TFont *font = TPen::lookupFont(preferences->getFont());
  _cxpx = font->getTextWidth(line.substr(0,utf8bytecount(line, 0, sx)));
}

/**
 * calculate _cx, _pos from _bol, _eol and _cxpx
 */
void
TTextArea::_pos_from_cxpx()
{
  assert(_cxpx != -1);
  TFont *font = TPen::lookupFont(preferences->getFont());
  string line = model->getValue().substr(_bol, _eol==string::npos ? _eol : _eol-_bol);

  int w1 = 0, w2 = 0;
  size_t p;
  unsigned cx;
  for(p=0, cx=0; p<=line.size(); utf8inc(line, &p), ++cx) {
    w2 = font->getTextWidth(line.substr(0, p));
    if (w2>_cxpx)
      break;
    w1 = w2;
  }
  if (p>=line.size()) {
    utf8dec(line, &p);
    --cx;
  } else
  if ( _cxpx-w1 < w2-_cxpx ) {
    utf8dec(line, &p);
    --cx;
  }

  _pos = _bol + p;
  _cx  = cx;
}

void
TTextArea::_cursor_up(unsigned n)
{
  MARK

  if (_cxpx == -1) {
    _cxpx_from_cx();
  }

  for(unsigned i=0; i<n; ++i) {
    if (_bol>0) {
      if (_bol>1) {
        _bol = model->getValue().rfind('\n', _bol-2);
        if (_bol==string::npos)
          _bol=0;
        else
          _bol++;
      } else {
        _bol=0;
      }
      _eol_from_bol();
      _invalidate_line(_cy);
      _cy--;
      _invalidate_line(_cy);
      _pos_from_cxpx();
    }
  }
  _catch_cursor();
  blink.visible=true;
}

void
TTextArea::_cursor_home()
{
  MARK
  if (_cx!=0) {
    bool flag = false;
    _pos=_bol;
    _cx=0;
  }
  _invalidate_line(_cy);
  _cxpx = -1;
  _catch_cursor();
  blink.visible=true;
}

void
TTextArea::_cursor_end()
{
  MARK
  size_t n = _eol - _pos;
  if (n!=0) {
    _cx+=utf8charcount(model->getValue(), _pos, n);
    _pos=_eol;
  }
  _cxpx = -1;
  _catch_cursor();
  _invalidate_line(_cy);
  blink.visible=true;
}

void
TTextArea::_page_down()
{
  MARK
  int h = visible.h / TPen::lookupFont(preferences->getFont())->getHeight();
  _cursor_down(h);
}

void
TTextArea::_page_up()
{
  MARK
  int h = visible.h / TPen::lookupFont(preferences->getFont())->getHeight();
  _cursor_up(h);
}

void
TTextArea::_return()
{
  MARK
  string indent;
  if (preferences->autoindent) {
    const string &s = model->getValue();
    size_t i;
    for(i=_bol; i<_eol; i++) {
      if (s[i]!=' ' && s[i]!='\t')
        break;
      indent+=s[i];
    }
    if (_bol >= _eol) // ???
#ifndef OLDLIBSTD
      indent.clear();
#else
      indent.erase();
#endif
    if (_cx < i-_bol)
      indent.erase(_cx);
  }
  indent.insert(0, "\n");
  _insert(indent);
}

void
TTextArea::_delete()
{
  MARK
  DBM(cout << "_delete: _bol=" << _bol << ", _pos=" << _pos << ", _eol=" << _eol << endl;)
  if (_pos<model->getValue().size()) {
    model->erase(_pos, utf8charsize(model->getValue(), _pos));
  }
}

void
TTextArea::_backspace()
{
  MARK
  if (_pos>0) {
    _cursor_left();
    _delete();
  }
}

void
TTextArea::resize()
{
  _catch_cursor();
  adjustScrollbars();
}

/**
 * Adjust view (_tx, _ty) so that the cursor is still visible.
 */
void
TTextArea::_catch_cursor()
{
  if (!model)
    return;

  MARK

  TFont *font = TPen::lookupFont(preferences->getFont());
  int h = font->getHeight();
  if (h==0) {
    cout << "error: TTextArea::_catch_cursor font of height 0" << endl;
    return;
  }
//  int w = font->getTextWidth("X");
  int th = (visible.h-h+1) / h;
//  int tw = (visible.w-w+1) / w;

  DBM(cout << "_catch_cursor: th="<<th<<", tw="<<tw<<", _cx="<<_cx<<", _cy="<<_cy<<endl;)
DBM(cout << __FILE__ << ':' << __LINE__ << ": _eol=" << _eol << endl;)
  if (_cy > th) {
    _scroll_down(_cy-th);
  }
  if (_cy < 0) {
    _scroll_up(-_cy);
  }

  if (_cxpx==-1) {
    _cxpx_from_cx();
  }
  
  int width = getWidth() - 5;
  if (vscroll)
    width -= vscroll->getWidth() + vscroll->getBorder()*2;

//cerr << "_tx = " << _tx << ", _tx+width = " << (_tx+width) << ", tx = " << tx << endl;
  
  if (_cxpx > _tx+width) {
//cerr << "  scroll to the right" << endl;
    _tx = _cxpx-width;
    invalidateWindow(); // call scrollRectangle instead
  } else if (_cxpx < _tx) {
//cerr << "  scroll to the left" << endl;
    _tx = _cxpx;
    invalidateWindow(); // call scrollRectangle instead
  }
DBM(cout << __FILE__ << ':' << __LINE__ << ": _eol=" << _eol << endl;)
}

void
TTextArea::_scroll_down(unsigned n)
{
  MARK
  if (_cy==0)
    return;
  _cy-=n;
  _ty+=n;
  if (vscroll)
    vscroll->setValue(_ty);
  scrollRectangle(visible, 0, -TPen::lookupFont(preferences->getFont())->getHeight()*n);
}

void
TTextArea::_scroll_up(unsigned n)
{
  MARK
  if (_ty==0)
    return;
  if (n>_ty)
    n=_ty;
  _cy+=n;
  _ty-=n;
  if (vscroll)
    vscroll->setValue(_ty);
  scrollRectangle(visible, 0, TPen::lookupFont(preferences->getFont())->getHeight()*n);
}

void
TTextArea::_scroll_right(unsigned n)
{
  MARK
  if (_cx==0)
    return;
//  _cx-=n;
//  _tx+=n;
  _cxpx = -1;
  invalidateWindow();  
}

void
TTextArea::_scroll_left(unsigned n)
{
  MARK
//  if (_tx==0)
//    return;
//  if (n>_tx)
//    n = _tx;
//  _cx+=n;
//  _tx-n;
  _cxpx = -1;
  invalidateWindow();
}

void 
TTextArea::setModified(bool m)
{
  assert(model!=NULL);
  model->setModified(m);
}

bool 
TTextArea::isModified() const
{
  if (!model)
    return false;
  return model->isModified();
}

bool
TTextArea::isEnabled() const {
  return model && model->isEnabled() && super::isEnabled();
}


void
TTextArea::setValue(const string &txt)
{
  assert(model!=NULL);
  return model->setValue(txt);
}

void
TTextArea::setValue(const char *data, size_t len)
{
  assert(model!=NULL);
  return model->setValue(data, len);
}

const string& 
TTextArea::getValue() const
{
  assert(model!=NULL);
  return model->getValue();
}

/**
 * Set cursor at text position (x, y).
 */
void
TTextArea::setCursor(unsigned cx, unsigned cy)
{
  if (!model)
    return;
  
  // we need to calculate:
  // _ty, _cx, _cy, _bol, _eol, _pos
  
  const string &data = model->getValue();
  size_t i;
  unsigned j, y;
  
  // calculate _bol and _eol
  i = 0;
  y = 0;
  _bol = 0;
  while(cy!=y && i<data.size()) {
    if (data[i]=='\n') {
      ++y;
      utf8inc(data, &i);
      _bol = i;
    } else {
      utf8inc(data, &i);
    }
  }
  _eol_from_bol();
  
  string line = data.substr(_bol, _eol-_bol);
//  cerr << "_bol = " << _bol << ", _eol = " << _eol << endl;
//  cerr << "found line '" << line << "'\n";

  // calculate _pos and _cx
  unsigned tabwidth = preferences->tabwidth;
  _pos = _bol;
  i = 0;
  j = 0;
  while(_pos<_eol) {
    unsigned m = 1;
    if (data[_pos]=='\t') {
      m = tabwidth - (i % tabwidth);
    }
//cerr << " cx = " << cx << ", i = " << i << ", i+m = " << (i+m) << endl;
    if (i<=cx && cx<i+m)
      break;
    i+=m;
    ++j;
    utf8inc(data, &_pos);
  }
  _cx = j;
  
  _cxpx = -1;
  _invalidate_line(_cy);
  _cy = y - _ty;
  _invalidate_line(_cy);
  _catch_cursor();

//  cerr << "_pos = " << _pos << endl;
}
    
unsigned 
TTextArea::getCursorX() const
{
  int cx;
  size_t d2, d3;
  string line;
  _get_line(&line, _bol, _eol, &cx, &d2, &d3);
  return cx;
}

unsigned 
TTextArea::getCursorY() const
{
  return _cy+_ty;
}
    
unsigned 
TTextArea::gotoLine(unsigned l)
{
//  cout << __PRETTY_FUNCTION__ << endl;
  if (!model)
    return 0;
  if (l>model->nlines)
    l=model->nlines;
  if (_cy+_ty==l)
    return l;
  if (_cy+_ty<l) {
    _cursor_down(l - _cy-_ty);
  } else {
    _cursor_up  (_cy+_ty - l);
  }
  return 0;
}

void 
TTextArea::find(const string&)
{
  cout << __PRETTY_FUNCTION__ << endl;
}

unsigned 
TTextArea::getLines() const
{
  if (!model)
    return 0;
  return model->nlines;
}
