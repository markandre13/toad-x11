/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-Andr� Hopf <mhopf@mark13.de>
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
#include <toad/action.hh>
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

inline void 
utf8inc(const string &text, unsigned int *cx)
{
  ++*cx;
  while( ((unsigned char)text[*cx] & 0xC0) == 0x80)
    ++*cx;
}

inline void 
utf8dec(const string &text, unsigned int *cx)
{
  --*cx;
  while( ((unsigned char)text[*cx] & 0xC0) == 0x80)
    --*cx;
}

// return the number of characters in text from start to start+bytelen
inline int
utf8charcount(const string &text, int start, int bytelen)
{
  return XCountUtf8Char((const unsigned char*)text.c_str()+start, bytelen);
}

inline int
utf8bytecount(const string &text, int start, int charlen)
{
  int result = 0;
  unsigned char *ptr = (unsigned char*)text.c_str() + start;
  while(charlen>0) {
    ++ptr;
    --charlen;
    ++result;
    while( (*ptr & 0xC0) == 0x80 ) {
      ++result;
      ++ptr;
    }
  }
  return result;
}

inline int
utf8charsize(const string &text, int pos)
{
  return utf8bytecount(text, pos, 1);
}

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

// timer for blinking caret
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
  
static TTextArea::TBlink blink;

void
TTextArea::TBlink::tick()
{
return;
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
//  fontname = "fixed,monospace:size=12";
  fontname = "sans-serif:size=12";
}

TTextArea::TPreferences::~TPreferences()
{
}

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
  
  preferences = new TPreferences();
  
  TAction *action;

  action = new TAction(this, "edit|cut");
  CONNECT(action->sigActivate, this, _selection_cut);
  action = new TAction(this, "edit|copy");
  CONNECT(action->sigActivate, this, _selection_copy);
  action = new TAction(this, "edit|paste");
  CONNECT(action->sigActivate, this, _selection_paste);
  action = new TAction(this, "edit|delete");
  CONNECT(action->sigActivate, this, _selection_erase);

  action = new TAction(this, "edit|undo");
  CONNECT(action->sigActivate, this, _undo);
  action = new TAction(this, "edit|redo");
  CONNECT(action->sigActivate, this, _redo);
}

TTextArea::~TTextArea()
{
  if (blink.current==this) {
    blink.stopTimer();  
    blink.current=NULL;
  }
  if (model)
    disconnect(model->sigTextArea, this);
  if (preferences)
    delete preferences;
}

void
TTextArea::keyDown(TKey key, char* str, unsigned modifier)
{
  if (!model)
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
        _undo();
        return;
      case 'r':
      case 'R':
        if (preferences->mode==TPreferences::NORMAL)
          _selection_clear();
        _redo();
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
            unsigned oldpos = _pos;
            _selection_paste();
            unsigned l = _eos - _bos; // _eos and _bos were sorted by 'paste()'
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
      if (!(modifier&MK_SHIFT) && preferences->mode==TPreferences::NORMAL)
        _selection_clear();
      _cursor_up();
      break;
    case TK_DOWN:
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
        unsigned m = (preferences->tabwidth -
                      utf8charcount(model->getValue(), _bol, _pos-_bol+1)-1) % preferences->tabwidth;
        string s;
        s.replace(0,1, m+1, ' ');
        _insert(s);
      } else {
        _insert("\t");
      }
      break;
    default:
      if ((unsigned char)str[0]>=32 || str[1]!=0) {
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
  if (!model)
    return;
  _goto_pixel(x, y);
  _bos = _eos = _pos;
  blink.visible=true;
  setFocus();
}

void
TTextArea::mouseMove(int x, int y, unsigned)
{
  if (!model)
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

void
TTextArea::_goto_pixel(int x, int y)
{
x-=2-_tx;
y-=2;
  TFont *font = TPen::lookupFont(preferences->getFont());
  int h = font->getHeight();
  y /= h;

  setCursor(0, y + _ty);

//  string line = model->getValue().substr(_bol, _eol==string::npos ? _eol : _eol-_bol);
  string line;
  int sx;
  unsigned d2, d3;
  _get_line(&line, _bol, _eol, &sx, &d2, &d3);
//  cerr << "found line '" << line << "'\n";

  int w1 = 0, w2 = 0;
  unsigned p, cx;
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
  if (!model)
    return;
  model->focus(b);
//cout << getTitle() << ".focus "<<isFocus()<<endl;
  _invalidate_line(_cy);
  if (isFocus()) {
    blink.current=this;
    blink.visible=true;
    blink.blink=true;
    blink.startTimer(0,500000);
  } else {
    blink.stopTimer(); 
    blink.current=NULL;
  }
}

void
TTextArea::_set_model(TTextModel *m)
{
  if (model)
    disconnect(model->sigTextArea, this);
  model = m;
  _cx = 0;
  _cy = 0;
  _ty = 0;
  _bol = 0;
  _pos = 0;
  if (model) {
    connect(model->sigTextArea, this, &TTextArea::modelChanged);
    _eol_from_bol();
  }
  _bos = _eos = 0;
  if (vscroll)
    vscroll->setValue(_ty);
  adjustScrollbars();
  invalidateWindow(true);
}

//#undef DBM
//#define DBM(CMD) CMD

#ifdef TOAD_TEXTAREA_CHECK
void
checkCursor2(TTextArea *ta)
{
  unsigned pos = ta->getPos();
  unsigned x, y;
  x = y = 0;
  const string &str = ta->getModel()->getValue();
  for(unsigned i=0; i<pos; ++i) {
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
checkCursor3(TTextArea *ta, unsigned offset, unsigned size)
{
  unsigned pos = ta->getPos();
  unsigned x, y;
  x = y = 0;
  string str = ta->getModel()->getValue();
  str.erase(offset, size);
  for(unsigned i=0; i<pos; ++i) {
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
        unsigned m1 = model->offset;
        unsigned m2 = model->offset+model->length;
        
        DBM(
          cout << "  REMOVE: offset=" << model->offset << endl
               << "          length=" << model->length << endl;
        )
        
        // update _cy, _ty
        
        unsigned n = 0;
        if (m2 <= _bol) {
          n = model->lines;
        } else
        if (m1 < _bol && _bol < m2) {
          n = 0;
          for(unsigned i=m1; i<=_bol; ++i) {
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
TTextArea::adjustScrollbars()
{
  visible.set(2,2,getWidth()-4,getHeight()-4);
//  cerr << "'" << getTitle() << "', adjustScrollbars: visible = " << visible << endl;

  if (!preferences || !model)
    return;

  if (preferences->singleline)
    return;

  bool need_vscroll = false;

  // vertical visible lines
  TFont *font = TPen::lookupFont(preferences->getFont());
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

void
TTextArea::_get_line(string *line, 
          unsigned bol, unsigned eol,
          int *sx,
          unsigned *bos, unsigned *eos)
{
  assert(line!=0);
  assert(sx!=0);
  *line = model->getValue().substr(bol, eol==string::npos ? eol : eol-bol);
      
  // set *bos < *eos
  //^^^^^^^^^^^^^^^^^^
  if (bos) {
    // _bos < _eos
    unsigned _bos = this->_bos;
    unsigned _eos = this->_eos;
    if (_bos > _eos) {
      unsigned a = _bos;
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

  for(unsigned i=0, j=0; 
    j<line->size();
    ++i, utf8inc(*line, &j))
  {
    if ((*line)[j]=='\t') {
      unsigned m = preferences->tabwidth - (i % preferences->tabwidth);
      if (!preferences->viewtabs) {
        line->replace(j, 1, m+1, ' ');
      } else {
//        line->replace(j, 1, m+1, '·');
//        line->replace(j, 1, 1  , '»');
        line->replace(j, 1, m+1, '.');
        line->replace(j, 1, 1  , '|');
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

/**
 * Paint the screen.
 * \todo
 *   Improve speed
 */
void
TTextArea::paint()
{
  TPen pen(this);
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
  unsigned bol = 0;
  unsigned eol;
  int y=-_ty * pen.getHeight();
  int sy, sx;
  sy = -_ty;
  
  while(true) {
    eol = data.find('\n', bol);
    unsigned n = eol==string::npos ? eol : eol-bol; // n=characters in line
    if (y+pen.getHeight()>=clipbox.y) { // loop has reached the visible area
//cerr << "line " << bol << "-" << eol << endl;

      string line;
      unsigned bos, eos;
      _get_line(&line, bol, eol, &sx, &bos, &eos);

//cerr << "draw line: '" << line << "'\n";
/*
for(unsigned i=0; i<line.size(); ++i) {
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
*/
//cerr << "  line     : " << bol << " - " << eol << endl;
//cerr << "  selection: " << _bos << " - " << _eos << endl;
//cerr << "  selection: " << bos << " - " << eos << endl;
      // draw text
      bool part=false;
      if (bos <= bol && eol <= eos) {
//cerr << "    line is inside selection\n";
        // inside selection
        pen.setLineColor(255,255,255);
        pen.setFillColor(0,0,0);
      } else if (eol < bos || bol > eos) {
//cerr << "    line is outside selection\n";
        // outside selection
        pen.setLineColor(0,0,0);
        pen.setFillColor(255,255,255);
      } else {
//cerr << "    line and selection true intersection\n";
        pen.setLineColor(0,0,0);
        pen.setFillColor(255,255,255);
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
        unsigned pos = 0;
        unsigned len = eol-bol;
        if (bol < bos) { // start is inside
//cerr << "start is inside" << endl;
          pos = bos-bol;
        }
        if (eos < eol) { // end is inside
//cerr << "end is inside" << endl;
          len = eos - bol;
        }
        if (bol < bos) { // start is inside
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
          pen.setLineColor(255,255,255);
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
  if (preferences->mode==TPreferences::NORMAL) {
    if (_bos != _eos)
      _selection_erase();
  }
  if (preferences->singleline) {
    unsigned p = s.find('\n');
    if (p!=string::npos) {
      string s2(s.substr(0,p));
      model->insert(_pos, s2);
      return;
    }
  }
  model->insert(_pos, s);
}

// dummy clipboard
static string clipboard;

void
TTextArea::_selection_erase()
{
  MARK
  if (_bos > _eos) {
    unsigned a = _bos;
    _bos = _eos;
    _eos = a;
  }
  model->remove(_bos, _eos-_bos);
}

void
TTextArea::_selection_cut()
{
  MARK
  _selection_copy();
  model->remove(_bos, _eos-_bos);
}

void
TTextArea::_selection_copy()
{
  MARK
  if (_bos > _eos) {
    unsigned a = _bos;
    _bos = _eos;
    _eos = a;
  }
  clipboard = model->getValue().substr(_bos, _eos-_bos);
//  cout << "'" << clipboard << "'" << endl;
}

void
TTextArea::_selection_paste()
{
  MARK
  _insert(clipboard);
}

void
TTextArea::_selection_clear()
{
  _bos = _eos = 0;
}
void
TTextArea::_delete_current_line()
{
  MARK
  model->remove(_bol, _eol-_bol+1);
}

void
TTextArea::_undo()
{
  MARK
  model->doUndo();
}

void
TTextArea::_redo()
{
  MARK
  model->doRedo();
}

void
TTextArea::mouseMDown(int,int,unsigned)
{
  if (!model)
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
#warning "_cursor_down doesn't handle utf-8"
  MARK
  if (_cxpx == -1) {
//    cout << "_cursor_down: _cxpx==-1\n";
    TFont *font = TPen::lookupFont(preferences->getFont());
    _cxpx = font->getTextWidth(model->getValue().substr(_bol, _pos-_bol));
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

/**
 * calculate _cx, _pos from _bol, _eol and _cxpx
 */
void
TTextArea::_pos_from_cxpx()
{
#warning "_pos_from_cxpx doesn't handle tabulators"
  assert(_cxpx != -1);
  TFont *font = TPen::lookupFont(preferences->getFont());
  string line = model->getValue().substr(_bol, _eol==string::npos ? _eol : _eol-_bol);

  int w1 = 0, w2 = 0;
  unsigned p, cx;
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
#warning "_cursor_up doesn't handle utf-8"
  MARK

  if (_cxpx == -1) {
//    cout << "_cursor_down: _cxpx==-1\n";
    TFont *font = TPen::lookupFont(preferences->getFont());
    _cxpx = font->getTextWidth(model->getValue().substr(_bol, _pos-_bol));
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
    _invalidate_line(_cy);
    _cxpx = -1;
    _catch_cursor();
    blink.visible=true;
  }
}

void
TTextArea::_cursor_end()
{
  MARK
  unsigned n = _eol - _pos;
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
    unsigned i;
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
    model->remove(_pos, utf8charsize(model->getValue(), _pos));
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
  MARK
  TFont *font = TPen::lookupFont(preferences->getFont());
  int h = font->getHeight();
  int w = font->getTextWidth("X");
  int th = (visible.h-h+1) / h;
  int tw = (visible.w-w+1) / w;

  DBM(cout << "_catch_cursor: th="<<th<<", tw="<<tw<<", _cx="<<_cx<<", _cy="<<_cy<<endl;)
DBM(cout << __FILE__ << ':' << __LINE__ << ": _eol=" << _eol << endl;)
  if (_cy > th) {
    _scroll_down(_cy-th);
  }
  if (_cy < 0) {
    _scroll_up(-_cy);
  }

  int tx;
  if (_cxpx==-1) {
    const string line(model->getValue().substr(_bol, _pos-_bol));
    tx = font->getTextWidth(line);
  } else {
    tx = _cxpx;
  }
  
  int width = getWidth() - 5;
  if (vscroll)
    width -= vscroll->getWidth() + vscroll->getBorder()*2;
  
  if (tx > _tx+width) {
    _tx = tx-width;
    invalidateWindow(); // call scrollRectangle instead
  } else if (tx < _tx) {
    _tx = tx;
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
   
void
TTextArea::setValue(const string &txt)
{
  assert(model!=NULL);
  return model->setValue(txt);
}

void
TTextArea::setValue(const char *data, unsigned len)
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

void
TTextArea::setCursor(unsigned x, unsigned y)
{
#warning "setCursor doesn't handle utf-8 and tabulators"
  if (!model)
    return;
    
  if (_cx == x && _cy==y)
    return;
  
  MARK
  DBM(cout << "x, y = " << x << ", " << y << endl;)
    
  _invalidate_line(_cy);
  
  TFont *font = TPen::lookupFont(preferences->getFont());
  int wx = visible.w / font->getTextWidth("x");
  int wy = visible.h / font->getHeight();

#if 0
cout << "screen position is at " << x << ", " << y << endl;
cout << "screen is " << _tx << " - " << _ty << ", "
     << (_tx + wx) << " - " << (_ty + wy) << endl;
#endif
  if (_ty <= y && y <= _ty + wy )
  {
    DBM(cout << "_bol, _pos, _eol = " << _bol << ", " << _pos << ", " << _eol << endl;)

    unsigned i=0;
    string::const_iterator tpos = model->getValue().begin();
    while(i<_ty) {
      if (*tpos=='\n')
        ++i;
      ++tpos;
    }
    tpos+=0;
    
//    cerr << "old line: " << model->getValue().substr(_bol, _eol-_bol) << endl;
    y-=_ty;
    string::const_iterator p = tpos;
    string::const_iterator bol = p;
    _cx = _cy = 0;
    while(p!=model->getValue().end()) {
      DBM(cout << "_cx, _cy = " << _cx << ", " << _cy << endl;)
      if (_cy==y && _cx==x) {
        DBM(cout << "break 2" << endl;)
        break;
      }
      if (*p=='\n') {
        if (_cy==y) {
          DBM(cout << "break 1" << endl;)
          break;
        }
        _cx = 0;
        ++_cy;
        bol = p+1;
      } else {
        ++_cx;
      }
      ++p;
    }
    _bol = bol - model->getValue().begin();
    // _pos = p - model->getValue().begin();
    _pos = _bol + _cx;
    _eol_from_bol();
    DBM(cout << "_bol, _pos, _eol = " << _bol << ", " << _pos << ", " << _eol << endl;)
//    cerr << "new line: " << model->getValue().substr(_bol, _eol-_bol) << endl;
  } else
  {
    // stupid implementation
    _cx = _cy = 0;
    _ty = 0;
    _pos = 0;
    _bol = 0;
    _eol = model->getValue().find('\n', _bol);
    _cursor_down(x);
    _cursor_right(y);
    if (vscroll)
      vscroll->setValue(_ty);
  }
  DBM(cout << "_cx, _cy = " << _cx << ", " << _cy << endl;)
  blink.visible=true;
  _invalidate_line(_cy);
}
    
unsigned 
TTextArea::getCursorX() const
{
  #warning "getCursorX doesn't handle tabulators"
  return _cx;
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
//---------------------------------------------------------------------

TTextModel::TTextModel()
{
  nlines = 0;
  undo = true;
  _modified = false;
  history = new THistory();
}

void
TTextModel::setValue(const string &d)
{
//cerr << "TTextModel[" << this << "]::setValue(string)\n";
  if (data==d) {
//    cerr << "-> not changed\n";
    return;
  }
//  DBM(cout << __PRETTY_FUNCTION__ << endl;)

  offset = 0;
  data = d;
  length = data.size();
  lines = (unsigned)-1;   // all lines have changed

  nlines = 0;
  unsigned pos = 0;
  while(true) {
    pos = d.find('\n', pos);
    if (pos==string::npos)
      break;
    pos++;
    nlines++;
  }
  
  _modified = false;
  type = CHANGE;
  sigTextArea();
  sigChanged();
}

void
TTextModel::setValue(const char *d, unsigned len)
{
//cerr << "TTextModel[" << this << "]::setValue(char*)\n";
#ifndef OLDLIBSTD
  if (data.compare(0, string::npos, d, len)==0) {
//    cerr << "-> not changed\n";
    return;
  }
#else
  if (data.compare(d, len)==0) {
//    cerr << "-> not changed\n";
    return;
  }
#endif

  offset = 0;
  length = len;
  data.assign(d, len);
  lines = (unsigned)-1;   // all lines have changed

  nlines = 0;
  unsigned pos = 0;
  while(true) {
    pos = data.find('\n', pos);
    if (pos==string::npos)
      break;
    pos++;
    nlines++;
  }
  
  _modified = false;
  type = CHANGE;
  sigTextArea();
  sigChanged();
}

/**
 * insert a single char
 */
void
TTextModel::insert(unsigned p, int c, bool undo)
{
  c = filter(c);
  if (!c)
    return;

  if (undo && history) {
    string s;
    s+=(char)c;
    history->add(new TUndoableInsert(this, p, s));
  }
  data.insert(p, 1, c);
  
  type = INSERT;
  offset = p;
  length = 1;
  lines  = c=='\n' ? 1 : 0;
  _modified = true;
  
  nlines += lines;
  
  sigTextArea();
  sigChanged();
}

/**
 * insert multiple chars
 */
void
TTextModel::insert(unsigned p, const string &aString, bool undo)
{
  string s(aString);

  string::iterator sp;
  sp = s.begin();
  while(sp!=s.end()) {
    if (filter(*sp)==0)
      sp = s.erase(sp);
    else  
      ++sp;
  }

  if (s.empty())
    return;
                              
  if (undo && history)
    history->add(new TUndoableInsert(this, p, s));

  data.insert(p, s);
  
  type = INSERT;
  offset = p;
  length = s.size();
  lines = 0;
  for(unsigned i=0; i<length; i++) {
    if (s[i]=='\n')
      lines++;
  }

  nlines += lines;
  _modified = true;
  
  sigTextArea();
  sigChanged();
}

/**
 * remove multiple chars
 *
 * \param p    offset
 * \param l    length
 * \param undo store in undo history if true
 */
void
TTextModel::remove(unsigned p, unsigned l, bool undo)
{
  DBM(cout << "remove at " << p << endl;)
  if (undo && history)
    history->add(new TUndoableRemove(this, p, data.substr(p,l)));
  lines = 0;
  for(unsigned i=p; i<p+l; ++i) {
    if (data[i]=='\n')
      ++lines;
  }
  nlines -= lines;
  type   = REMOVE;
  offset = p;
  length = l;
  _modified = true;
  sigTextArea();
  data.erase(p, l);
  sigChanged();
}

int
TTextModel::filter(int c)
{
  return c;
}

void
TTextModel::focus(bool)
{
}

void
TTextModel::doUndo()
{
  if (history && history->getBackSize()>0) {
     history->getCurrent()->undo();
     history->goBack();
  }
}

void
TTextModel::doRedo()
{
  if (history && history->getForwardSize()>0) {
     history->goForward();
     history->getCurrent()->redo();
  }
}
