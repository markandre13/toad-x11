/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-Andre Hopf <mhopf@mark13.org>
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

#include <toad/htmlview.hh>
#include <toad/scrollbar.hh>
#include <toad/io/urlstream.hh>
#include <toad/action.hh>
#include <toad/filedialog.hh>
#include <toad/undomanager.hh>
#include <toad/utf8.hh>

#include <ctype.h>
#include <string>
#include <map>
#include <stack>

// test only:
//#include <fstream>

using namespace toad;

/**
 * \class THTMLView
 * A minimal HTML viewer to view on-line help.
 *
 * This widget supports a subset of HTML with the following tags:
 * html, body, b, i, em, br, p, hr, a, table, tr, th
 *
 * The following tags are planned: table, tr, td, th, img.
 *
 * \todo
 *   \li 
 *     tables within tables don't work
 *   \li
 *     rendering the whole page per scroll is too slow for large
 *     pages, there is currently a workaround for the head of large
 *     pages, which is enabled via definition of SPEEDUP_KLUDGE
 *   \li
 *     grayscale png are broken
 *   \li
 *     TScrollPane misses to invalidate some screen regions
 */

#define SPEEDUP_KLUDGE

namespace {

struct TState;
struct TElement;
struct TETable;
struct TEAnchor;

} // namespace

class THTMLView::TElementStorage:
  public vector<TElement*>
{
  public:
    void clear();
};

class THTMLView::TAnchorStorage:
  public vector<TEAnchor*>
{
};

namespace {

typedef THTMLView::TElementStorage TElementStorage;
typedef THTMLView::TAnchorStorage TAnchorStorage;

typedef map<string,string> TParameters;

/**
 * TParser converts HTML into a vector of TElement's.
 */
class TParser
{
  public:
    TParser();
    ~TParser();
  
    void parse(TElementStorage *out, 
               TAnchorStorage *aout, 
               istream &in,
               const string &url);
    
    bool 
    getParameter(int *value, const string &name, int d) const {
      TParameters::const_iterator p = parammap.find(name);
      if (p == parammap.end()) {
        *value = d;
        return false;
      }
      *value = atoi(p->second.c_str());
      return true;
    }

    bool 
    getParameter(string *value, const string &name, const string &d) const {
      TParameters::const_iterator p = parammap.find(name);
      if (p == parammap.end()) {
        *value = d;
        return false;
      }
      *value = p->second.c_str();
      return true;
    }
    
    string relative(const string&) const;

  protected:
    string base_host, base_path;
    void setBase(const string &url);
  
    // parser section:
    string text;
    string tagname;
    TParameters parammap;
    static const unsigned START = 1;
    static const unsigned END = 2;
    unsigned tagtype;
    
    bool inside_pre;
  
    void handleTag();
    void handleText(bool blank);
    
    // semantic section:
    bool inside_body;
    
    // semantic: support for tables:
    struct TPStackNode {
      TPStackNode(TElementStorage *pout, TETable *table) {
        lastpout = pout;
        lasttable = table;
      }
      TElementStorage *lastpout;
      TETable *lasttable;
    };
    
    stack<TPStackNode> parsedstack;
    void pushParsed() {
      parsedstack.push(TPStackNode(pout, table));
    }
    void popParsed() {
      if (parsedstack.empty())
        return;
      pout = parsedstack.top().lastpout;
      table = parsedstack.top().lasttable;
      parsedstack.pop();
    }
    TETable *table;

    TElementStorage* pout;
    TAnchorStorage* aout;
};

/**
 * TElement represents HTML elements like a word, <b>, <table>, etc.
 */
struct TElement
{
  virtual void render(TPen &pen, TState &state) = 0;
  
  // these two flags are used to justify a line during stage 1
  bool bol;
  int indent;
};

/**
 * TState manages all information when rendering a vector of TElements,
 * ie. the left and right margins, the current font, etc.
 *
 * It is used with output=false to calculate the layout informations and
 * with output=true to render the TElements.
 */
struct TState
{
  public:
    enum  EAlignment {
      ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT, ALIGN_BLOCK
    };

    TState(int width) {
      left  = 5;
      right = width - 5;
      y     = 5;
      init();
    }
    
    TState(int ax, int awidth, int ay) {
      left  = ax;
      right = awidth;
      y     = ay;
      init();
    }

    void init() {
      x     = left;
      top   = bottom = y;

      itemnumber = 0;
      itemdepth  = 0;
      
      s_left= left;
      s_top = top;
      
      face   = "arial,helvetica,sans-serif";
      size   = 12;
      bold   = 0;
      italic = 0;
      color.set(0,0,0);
      current_color = color;
      
      align = ALIGN_LEFT;
pen = 0;
      bol = 0;
      lineindent = 0;
      
      anchor = 0;
      
      lmin = lmax = lmax_per_line = 0;
      blank = false;
    }
  
    bool blank;
    void addBlank() {
      blank = true;
    }
    
    void spaceForWidth(TPen &pen, int width) {
      int w = 0;
      
      if (blank)
        w = pen.getTextWidth(" ");
      if (x+w+width > right) {
        newLine();
      } else
      if (blank) {
        blank = false;
        x+=w;
        lmax_per_line+=w;
        if (lmax_per_line > lmax)
          lmax = lmax_per_line;
      }
    }
    
TPen *pen;

    void addElement(int width, int height) {
/*
if (pen) {
  pen->setColor(0,0,255);
  pen->drawRectangle(x, y, width, height);
  pen->setColor(0,0,0);
}
*/
      x += width;
      lmax_per_line += width;
      if (lmax_per_line > lmax)
        lmax = lmax_per_line;
      if (width>lmin)
        lmin = width;
      if (y+height > bottom)
        bottom = y+height;
    }

    void handle(TPen &pen, TElement *e) {
      current = e;
      if (!output) {
        e->bol = false;
        e->indent = 0;
        if (bol==0) {
          bol = e;
          e->bol = true;
        }
      } else {
        if (e->bol) {
//cerr << "handle, e->indent=" << e->indent << endl;
          lineindent = e->indent;
        }
      }
      e->render(pen, *this);
      if (x4<x)
        x4 = x;
    }

    // these 3 attributes will be used to justify a line
    int lineindent;
    TElement *current; 
    TElement *bol;   

    void newLine() {
      blank = false;
      if (!output) {
        if (align==ALIGN_CENTER) {
          bol->indent = (right-x)/2;
        } else
        if (align==ALIGN_RIGHT) {
          bol->indent = right-x;
        }
        bol = current;
        bol->bol = true;
      }
/*
if (pen) {
  pen->setColor(255,0,0);
  pen->drawRectangle(left, y, x-left, bottom-y);
  pen->setColor(0,0,0);
}
*/
      newline = true;
//cerr << "lmax_per_line = " << lmax_per_line << ", lmax = " << lmax << endl;
      if (lmax_per_line > lmax)
        lmax = lmax_per_line;
      lmax_per_line = 0;
      lastlineright = x;
      x = left;
      y = bottom;
      if (x1>x)
        x1 = x;
    }

    void done() {
      if (!output) {
        if (align==ALIGN_CENTER) {
          bol->indent = (right-x)/2;
        }
      }
    }
    
    void indent(int width) {
      left += width;
      if (x<left)
        x = left;
    }
    
    
    int x1, x2, x3, x4, y1, y2, y3, y4;
    TEAnchor *anchor;
    TAnchorStorage *aout;
    
    void beginAnchor(TEAnchor*);
    void endAnchor();
    
    int X() const { return x+lineindent; }
    int Y() const { return y; }
    int Left() const { return left; }
    int Right() const { return right; }
    int getBottom() const { return bottom; }
    void getSize(int *w, int *h) const {
      *w = right - s_left;
      *h = bottom - s_top;
    }
    void getPos(int *x, int *y, int *bottom) const {
      *x = this->x;
      *y = this->y;
      *bottom = this->bottom;
    }
    void setPos(int x, int y, int bottom) {
      this->x = x;
      this->y = y;
      this->bottom = bottom;
    }

    string fontname;

    void applyAttributes(TPen &pen) {
      string fontname = face + ":size=";
      char buffer[20];
      snprintf(buffer, sizeof(buffer), "%i", size);
      fontname+=buffer;

      if (bold) {
        fontname += ":bold";
      }
      if (italic) {
        fontname += ":italic";
      }
      if (this->fontname != fontname) {
        pen.setFont(fontname);
        this->fontname = fontname;
      }
      if (current_color != color) {
        current_color = color;
        pen.setColor(color);
      }
    }

    struct state_t {
      int left;
      unsigned size;
      string face;
      TRGB color;
      EAlignment align;
    };
    stack<state_t*> stack;
    
    void pushState() {
      state_t *s = new state_t;
      stack.push(s);
      s->left  = left;
      s->size  = size;
      s->face  = face;
      s->color = color;
      s->align = align;
    }
    
    void popState() {
      if (stack.empty())
        return;
      state_t *s = stack.top();
      stack.pop();
      left  = s->left;
      size  = s->size;
      face  = s->face;
      color = s->color;
      align = s->align;
      delete s;
    }
    
  protected:
    int left, right;
    int top, bottom;
    int x, y;
    int s_left, s_top;
    
  public:
    // data used to align tables:
    // lmin: minimal size of an element
    // lmax: size of all elements
    int lmin, lmax, lmax_per_line;

    // 'false' during stage 1 -> don't draw
    // 'true' during stage 2 -> draw
    bool output;

    int lastlineright;
    bool newline;
    int linewidth;
  
    EAlignment align;
  
    // current ul and ol attributes
    unsigned itemnumber;
    unsigned itemdepth;

    // current font attributes
    string face;
    unsigned size;
    unsigned bold;
    unsigned italic;
    TRGB color;
    TRGB current_color;
};

struct TEWord:
  public TElement
{
  TEWord(const string &word, bool blank) { 
    this->word = word; 
    this->blank = blank;
  }
  
  string word;
  bool blank; // word is followed by a blank
  
  void render(TPen &pen, TState &s) {
    s.applyAttributes(pen);
    int width = pen.getTextWidth(word);
    s.spaceForWidth(pen, width);
    if (s.output) {
// cerr << s.X() << "," << s.Y() << " " << word << endl;
      pen.drawString(s.X(), s.Y(), word);
    }
    s.addElement(width, pen.getHeight());
    if (blank)
      s.addBlank();
  }
};

struct TEBreak:
  public TElement
{
  void render(TPen &pen, TState &s) {
    s.addElement(0, pen.getHeight());
    s.newLine();
  }
};

struct TEParagraph:
  public TElement
{
    bool begin;
    TState::EAlignment align;
  public:
    TEParagraph(bool begin, TParser &parser) {
      this->begin = begin;

      if (!begin)
        return;
        
      string tmp;
      parser.getParameter(&tmp, "align", "left");
      align = TState::ALIGN_LEFT;

      if (strcasecmp(tmp.c_str(), "center")==0)
        align = TState::ALIGN_CENTER;
      else if (strcasecmp(tmp.c_str(), "right")==0)
        align = TState::ALIGN_RIGHT;
    }
    void render(TPen &pen, TState &s) {
      if (begin) {
        s.addElement(0, pen.getHeight()/2);
        s.pushState();
        s.newLine();
        s.align = align;
      } else {
        s.newLine();
        s.popState();
        s.addElement(0, pen.getHeight()/2);
      }
  }
};

struct TEHeading:
  public TElement
{
    bool begin;
    unsigned level;
    TState::EAlignment align;
  public:
    TEHeading(bool begin, TParser &parser, unsigned level) {
      this->begin = begin;

      if (!begin)
        return;

      this->level = level;        
      string tmp;
      parser.getParameter(&tmp, "align", "left");
      align = TState::ALIGN_LEFT;

      if (strcasecmp(tmp.c_str(), "center")==0)
        align = TState::ALIGN_CENTER;
      else if (strcasecmp(tmp.c_str(), "right")==0)
        align = TState::ALIGN_RIGHT;
    }
    void render(TPen &pen, TState &s) {
      if (begin) {
        s.addElement(0, pen.getHeight()/2);
        s.pushState();
        s.newLine();
        s.align = align;
        switch(level) {
          case 1:
            s.size += 4;
            break;
          case 2:
            s.size += 2;
            break;
        }
        s.bold++;
      } else {
        s.newLine();
        s.bold--;
        s.popState();
        s.addElement(0, pen.getHeight()/2);
      }
  }
};

struct TEHRule:
  public TElement
{
  void render(TPen &pen, TState &s) {
    s.applyAttributes(pen);
    s.newLine();
    int l = s.Left();
    int r = s.Right();
    int y = s.Y();
    if (s.output)
      pen.fillRectangle(l, y, r-l, 2);
    s.addElement(r-l, 2);
    s.newLine();
  }
};

struct TEUnorderList:
  public TElement
{
  bool begin;
  
  TEUnorderList(bool begin) {
    this->begin = begin;
  }
  void render(TPen &pen, TState &s) {
    if (begin) {
      s.pushState();
      s.itemdepth++;
      s.newLine();
      s.indent(pen.getHeight());
    } else {
      s.itemdepth--;
      s.popState();
      s.newLine();
    }
  }
};

struct TEListItem:
  public TElement
{
  void render(TPen &pen, TState &s) {
    s.newLine();
    s.itemnumber++;
    switch(s.itemdepth) {
      case 0:
      case 1:
        if (s.output) {
          s.applyAttributes(pen);
          pen.fillCircle(s.Left()-8, s.Y()+pen.getHeight()/2-3, 6, 6);
        }
        break;        
      default:
        if (s.output) {
          s.applyAttributes(pen);
          pen.fillCircle(s.Left()-6, s.Y()+pen.getHeight()/2-2, 4, 4);
        }
    }
    s.addElement(0,10);
  }
};

struct TEBold:
  public TElement
{
  bool begin;
  
  TEBold(bool begin) {
    this->begin = begin;
  }
  void render(TPen &pen, TState &s) {
    if (begin) {
      s.bold++;
    } else {
      s.bold--;
    }
  }
};

struct TEItalic:
  public TElement
{
  bool begin;
  
  TEItalic(bool begin) {
    this->begin = begin;
  }
  void render(TPen &pen, TState &s) {
    if (begin) {
      s.italic++;
    } else {
      s.italic--;
    }
  }
};

struct TEAlignment:
  public TElement
{
  bool begin;
  TState::EAlignment align;
  
  TEAlignment(bool begin, TState::EAlignment a) {
    this->begin = begin;
    align = a;
  }
  void render(TPen &pen, TState &s) {
    if (begin) {
      s.pushState();
      s.align = align;
    } else {
      s.popState();
    }
  }
};

struct TEAnchor:
  public TElement
{
  TPolygon polygon;
  string href;
  string alt;
  
  TEAnchor(TParser &parser) {
    parser.getParameter(&href, "href", "");
    href = parser.relative(href);
    parser.getParameter(&alt, "alt", "");
  }
  void render(TPen &pen, TState &s) {
    s.pushState();
    if (!s.output) {
      s.beginAnchor(this);
    } else {
      if (!href.empty())
        s.color.set(0, 0, 232);
    }
  }
};

struct TEAnchorEnd:
  public TElement
{
  void render(TPen &pen, TState &s) {
    if (!s.output)
      s.endAnchor();
    s.popState();
  }
};

struct TEImage:
  public TElement
{
  TEImage(TParser &parser);
  ~TEImage();
  void render(TPen &pen, TState &s);
  
  string src;
  string alt;
  int width, height;
  int border;
  TBitmap bitmap;
};

void
TParser::setBase(const string &url)
{
  base_host.clear();
  base_path.clear();
//cerr << "set base" << endl;
  size_t p1 = url.find("://");
//cerr << "  " << p1 << endl;
  if (p1!=string::npos) {
    p1 = url.find("/", p1+3);
    base_host = url.substr(0, p1);
//cerr << "  host '" << base_host << "'\n";
  } else {
    p1 = 0;
  }

  size_t p2 = url.rfind("/");
  if (p2!=string::npos) {
    base_path = url.substr(p1, p2-p1+1);
//cerr << "  path '" << base_path << "'\n";
  }
}

string
TParser::relative(const string &url) const
{
  if (url.find("://") != string::npos)
    return url;
  if (url.size()>1 && url[0]=='/') {
    return base_host + url;
  }
  return base_host + base_path + url;
}

TEImage::TEImage(TParser &parser)
{
  parser.getParameter(&border, "border", 1);
  parser.getParameter(&src, "src", "");
  parser.getParameter(&alt, "alt", src);

//cerr << "got image path '" << src << "'\n";
  src = parser.relative(src);
//cerr << "using '" << src << "'\n";
  if (src.empty())
    return;

  try {
    bitmap.load(src);
  } catch (...) {
    bitmap.load("memory://toad/broken.png");
  }
  width = bitmap.getWidth();
  height = bitmap.getHeight();
}

TEImage::~TEImage()
{
}

void
TEImage::render(TPen &pen, TState &s)
{
  if (src.empty())
    return;
    
  s.spaceForWidth(pen, width+border*2);
  if (s.output) {
    pen.drawBitmap(s.X()+border, s.Y()+border, bitmap);
    for(int i=0; i<border; ++i)
      pen.drawRectanglePC(s.X()+i, s.Y()+i, 
                          width+border*2-i*2, 
                          height+border*2-i*2);
  }
  s.addElement(width+border*2, height+border*2);
/*
  if (blank)
    s.addBlank();
*/
}

struct TETable:
  public TElement
{
  TETable(TParser &parser);
  ~TETable();
  
  void render(TPen &pen, TState &s);

  bool initialized;
  int weight;

  struct TColumn {
    TColumn() {
      width = 0;
      lmin = 0;
      weight = 0;
    }
    int width, lmin, weight;
  };
  vector<TColumn> columns;
  
  struct TField {
    TField() {
      colspan = rowspan = 1;
    }
    TRGB background;
    int colspan;
    int rowspan;
    int lmin, lmax;
    TElementStorage parsed;
  };
  
  struct TRow {
    TRGB background;
    vector<TField> fields;
    int height;
    int lmin, lmax;
  };
  
  vector<TRow> rows;
  
  void newRow();
  TElementStorage *newField(bool isHead);
  
  int border, cellpadding, cellspacing;
  
  enum { NONE, COLOR, IMAGE } background;
  
  int lmin, lmax;

  int width, height;
};

} // namespace

TETable::TETable(TParser &parser)
{
  parser.getParameter(&border, "border", 0);
  if (border<0)
    border = 0;
  parser.getParameter(&cellpadding, "cellpadding", 1);
  if (cellpadding<0)
    cellpadding = 0;
  parser.getParameter(&cellspacing, "cellspacing", 2);
  if (cellspacing < 0)
    cellspacing = 0;

  lmin = lmax = 0;
  
  background = NONE;
  
  initialized = false;
}

TETable::~TETable()
{
  vector<TRow>::iterator pr(rows.begin()), er(rows.end());
  while(pr!=er) {
    vector<TField>::iterator pf(pr->fields.begin()),
                             ef(pr->fields.end());
    while(pf!=ef) {
      pf->parsed.clear();
      ++pf;
    }
    ++pr;
  }
}

void
TETable::render(TPen &pen, TState &s) {
  s.newLine();
  if (!s.output) {
    if (!initialized) {
//cerr << "trying to estimate the size of the table" << endl;
    
      int cols_per_table = 0;
    
      vector<TRow>::iterator pr(rows.begin()), er(rows.end());
      while(pr!=er) {
        pr->lmin = 0;
        pr->lmax = 0;
        vector<TField>::iterator pf(pr->fields.begin()),
                                 ef(pr->fields.end());
        int cols_per_row = 0;
        while(pf!=ef) {
          cols_per_row += pf->colspan;
          TState state(INT_MAX);
          state.output = false;
          TElementStorage::iterator p(pf->parsed.begin()),
                                    e(pf->parsed.end());
          while(p!=e) {
            state.handle(pen, *p);
            ++p;
          }
//          cerr << "weight: " << state.lmin << ", " << state.lmax << endl;
          pr->lmin += state.lmin;
          pr->lmax += state.lmax;
          pf->lmin = state.lmin;
          pf->lmax = state.lmax;
          ++pf;
        }
        if (cols_per_row > cols_per_table)
          cols_per_table = cols_per_row;
        if (pr->lmin > lmin)
          lmin = pr->lmin;
        if (pr->lmax > lmax)
          lmax = pr->lmax;
        ++pr;
      }
//      cerr << "table: " << lmin << ", " << lmax << endl;

      columns.erase(columns.begin(), columns.end());
      for(int i=0; i<cols_per_table; ++i)
        columns.push_back(TColumn());
      
      pr = rows.begin();
      weight = 0;
      while(pr!=er) {
        vector<TField>::iterator pf(pr->fields.begin()), ef(pr->fields.end());
        vector<TColumn>::iterator pc = columns.begin();
        while(pf!=ef) {
          pc->weight += pf->lmax - pf->lmin;
          if (pc->lmin < pf->lmin)
            pc->lmin = pf->lmin;
          weight += pf->lmax - pf->lmin;
          pc += pf->colspan;
          ++pf;
        }
        ++pr;
      }
      initialized = true;
    }
    
    // calculate the width of the table:
    width = s.Right() - s.Left();

//cerr << endl;
//cerr << "maximal width for the table: " << width << endl;
    
    int extra = (columns.size() + 1) * cellspacing +
                columns.size() * cellpadding * 2;
    if (border)
      extra +=  border * 2 + columns.size() * 2;

//cerr << "lmax = " << lmax << endl;
//cerr << "lmin = " << lmin << endl;
      
    if (width>lmax+extra)
      width = lmax+extra;
    else if (width<lmin+extra)
      width = lmin+extra;

//cerr << "using width: " << width << endl;
      
    vector<TColumn>::iterator pc(columns.begin()), ec(columns.end());
    int rwidth = width - extra;
    while(pc!=ec) {
      rwidth -= pc->lmin;
      ++pc;
    }
    pc = columns.begin();
    while(pc!=ec) {
      pc->width = pc->lmin;
      if (weight)
        pc->width += rwidth * pc->weight / weight;
      else
        pc->width += rwidth / columns.size();
      ++pc;
    }

    int x, y;
    
    y=s.Y() + border;
    y+=cellspacing;
    int bottom = 0;
    vector<TRow>::iterator pr(rows.begin()), er(rows.end());
    while(pr!=er) {
      if (border)
        y++;
      y+=cellpadding;
      x = s.X();
      x+=border;
      x+=cellspacing;
      vector<TField>::iterator pf(pr->fields.begin()), ef(pr->fields.end());
      vector<TColumn>::iterator pc = columns.begin();
      while(pf!=ef) {
        x+=cellpadding;
        if (border)
          x++;
        TState state(x, x+pc->width, y);
        state.output = false;
        TElementStorage::iterator p(pf->parsed.begin()),
                                  e(pf->parsed.end());
        while(p!=e) {
          state.handle(pen, *p);
          ++p;
        }
        if (state.getBottom() > bottom)
          bottom = state.getBottom();
        x += pc->width;
        x += cellpadding;
        if (border)
          x++;
        x += cellspacing;
        pc += pf->colspan;
        ++pf;
      }
//cerr << "row height: " << (bottom - y) << endl;
      pr->height = bottom - y;
      y = bottom;
      y += cellpadding;
      if (border)
        y++;
      y += cellspacing;
      ++pr;
//      pen.drawLine(s.X(), y, s.X()+width, y);
    }
    height = y+border-s.Y();
  } else {
    // s.output
    
    int x, y;
    
    vector<TRow>::iterator pr(rows.begin()), er(rows.end());
    y = s.Y() + border + cellspacing;
    while(pr!=er) {
      vector<TColumn>::iterator pc(columns.begin()), ec(columns.end());
      vector<TField>::iterator pf(pr->fields.begin()), ef(pr->fields.end());
      x = s.X() + border + cellspacing;
      while(pf!=ef) {
        if (border) {
          pen.drawRectangle(x, y, 
                            pc->width + cellpadding * 2 + 1, 
                            pr->height + cellpadding * 2 + 1);
        }
        switch(background) {
          case NONE:
            break;
          case COLOR:
            pen.setColor(255,128,0);
            pen.fillRectanglePC(x+(border?1:0),
                                y+(border?1:0),
                                pc->width  + cellpadding * 2,
                                pr->height + cellpadding * 2);
            pen.setColor(192,192,255);
            pen.fillRectanglePC(x+(border?1:0)+cellpadding,
                                y+(border?1:0)+cellpadding,
                                pc->width,
                                pr->height);
            pen.setColor(0,0,0);
            break;
          case IMAGE:
            break;
        }
        x += cellpadding;
        if (border)
          ++x;
        TState state(x, x+pc->width, y+cellpadding+(border?1:0));
        TElementStorage::iterator p(pf->parsed.begin()),
                                  e(pf->parsed.end());
        while(p!=e) {
          state.handle(pen, *p);
          ++p;
        }
        if (border)
          ++x;
        x += pc->width + cellpadding + cellspacing;
        pc += pf->colspan;
        ++pf;
      }
      if (border)
        y+=2;
      y+=pr->height + cellpadding * 2 + cellspacing;
      ++pr;
    }
  }
  if (border) {
    for(int i=0; i<border; ++i)
      pen.drawRectanglePC(s.X()+i, s.Y()+i, width-i*2, height-i*2);
  }
  s.addElement(width,height);
  s.newLine();
}

void
TETable::newRow()
{
  rows.push_back(TRow());
}

TElementStorage*
TETable::newField(bool isHead)
{
  if (rows.empty())
    newRow();
  TRow &lastrow(rows[rows.size()-1]);
  lastrow.fields.push_back(TField());
  TField &lastfield(lastrow.fields[lastrow.fields.size()-1]);
  if (isHead) {
    lastfield.parsed.push_back(new TEBold(true));
    lastfield.parsed.push_back(new TEAlignment(true, TState::ALIGN_CENTER));
  }
  return &lastfield.parsed;
}

/*
  The meaning of x1..x4, y1..y4:

  y1            +---------------------------------+
                |                                 |
  y2      +-----+                                 |
          |                                       |
  y3      |                      +----------------+
          |                      |
  y4      +----------------------+
          x1    x2               x3               x4
*/

void
TState::beginAnchor(TEAnchor *anchor)
{
  x1 = x2 = x3 = x4 = x;
  y1 = y;
  y2 = y3 = y4 = bottom;
  this->anchor = anchor;
}

void
TState::endAnchor()
{
  x3 = x;
  y3 = y;
  y4 = bottom;

  if (!anchor || output)
    return;

  TPolygon &p(anchor->polygon);
  p.erase(p.begin(), p.end());
  p.addPoint(x2,y1);
  p.addPoint(x4,y1);
  p.addPoint(x4,y3);
  p.addPoint(x3,y3);
  p.addPoint(x3,y4);
  p.addPoint(x1,y4);
  p.addPoint(x1,y2);
  p.addPoint(x2,y2);
/*
if (pen) {
  pen->setColor(0,255,0);
  pen->drawPolygon(p);
}
*/
  anchor = 0;
}


TParser::TParser()
{
  pout = 0;
  table = 0;
}

TParser::~TParser()
{
}

// created automatically by:lexit.cc on Oct  1 1998

static const unsigned char entity_table[33][58] = 
{
  {26,0,199,0,27,0,0,0,28,0,0,0,0,209,29,0,0,0,0,222,31,0,0,0,221,0,0,0,0,0,0,0,1,166,5,16,32,21,159,0,4,0,0,2,15,3,11,7,156,14,9,30,10,0,0,0,8,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,225,0,19,0,230,0,224,0,0,0,0,0,157,0,0,0,0,229,0,227,228,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,171,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,158,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,160,0,0,0,0,0,0,0,0,0,0,0,0,172,0,0,0,0,241,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,237,0,238,0,161,0,236,0,0,0,0,0,0,0,0,0,191,0,0,0,239,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,231,0,6,0,0,0,0,0,0,0,0,0,169,0,0,0,0,0,164,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,184,0,0,0,0,0,0,0,0,0,162,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,182,0,0,0,0,0,0,0,0,0,0,177,0,0,163,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,253,0,0,0,165,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,167,0,0,173,0,0,0,0,0,0,0,0,0,0,0,0,17,0,0,0,0,223},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,250,0,251,0,0,0,249,0,0,0,0,0,168,0,0,0,0,0,0,0,252,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,243,0,244,0,0,0,242,0,0,0,0,0,0,0,0,0,0,12,248,245,246,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,0,0,0,0,0,0,186,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,187,0,0,0,174,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,175,0,0,0,0,0,0,0,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,176,0,0,0,247,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,18,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,226,0,0,0,0,0,0,0,0,0,0,0,180,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,181,183,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,23,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,198,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,193,0,194,0,0,0,192,0,0,0,0,0,0,0,0,0,0,197,0,195,196,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,0,0,0,0,0,0,0,0,0,0,0,0,201,0,202,0,0,0,200,0,0,0,0,0,0,0,0,0,0,0,0,0,203,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,205,0,206,0,0,0,204,0,0,0,0,0,0,0,0,0,0,0,0,0,207,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,211,0,212,0,0,0,210,0,0,0,0,0,0,0,0,0,0,0,216,213,214,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,215,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,218,0,219,0,0,0,217,0,0,0,0,0,0,0,0,0,0,0,0,0,220,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,233,0,234,0,0,0,232,0,0,0,0,0,0,0,0,0,0,0,0,240,235,0,0,0,0,0}
};

static const char rest_pointer[100] = {
  0,1,2,3,4,5,3,6,7,8,9,10,11,12,2,13,3,14,15,16,15,17,2,2,18,19,20,
  21,22,2,2,13,2,2,23,24,25,26,27,28,29,30,31,32,25,26,27,29,25,26,27,29,33,
  34,25,26,27,28,29,35,36,25,26,27,29,37,38,31,25,26,39,28,29,30,31,40,25,26,27,
  29,25,26,27,29,41,28,25,26,27,28,29,42,36,25,26,27,29,26,43,29
};

static const char *rest_strings[44] = {
  "uot","p","","t","sp","xcl","und","rren","n","rvbar","ct","l","py","quo","y",
  "g","cr","usmn","te","ro","ra","dot","il","4","uest","rave","cute","irc",
  "ilde","ml","ing","lig","cedil","H","tilde","mes","lash","acute","HORN","rc",
  "edil","h","vide","orn"
};

void
TParser::parse(TElementStorage *out,
               TAnchorStorage *aout,
               istream &in,
               const string &url)
{
  inside_body = false;
  pout = out;
  this->aout = aout;
  setBase(url);

  string name, value;
  unsigned state = 0;
  unsigned current_table;
  const char *entity_tail;
  string entity;
  int c;
  bool blank = false;
  inside_pre = false;
  while(true) {
    if (!in)
      break;
    c = in.get();
    if (c==-1)
      break;
// cerr << endl << "[" << state << "|" << tagname << "]" << (char)c << ": ";
    switch(state) {
      case 0:
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            blank = true;
            if (!inside_pre) {
              if (!text.empty()) {
                handleText(blank);
                blank = false;
                text.clear();
              }
              state = 1;
            } else {
              text+=c;
            }
            break;
          case '<':
            if (!text.empty()) {
              handleText(blank);
              text.clear();
            }
            tagname.clear();
            tagtype = 0;
            parammap.clear();
            state = 2;
            break;
          case '&':
//cerr << "found entity at state 0" << endl;
            current_table = 0;
            state = 15;
            break;
          default:
            text+=c;
            break;
        }
        break;
      
      // consume whitespace
      case 1:
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            blank = true;
            break;
          case '<':
            tagname.clear();
            tagtype = 0;
            parammap.clear();
            state = 2;
            break;
          case '&':
//cerr << "found entity at state 1" << endl;
            current_table = 0;
            state = 15;
            break;
          default:
            text+=c;
            state = 0;
            break;
        }
        break;
      
      // tag or comment =>
      case 2: // <? ;
        switch(c) {
          case '!':
            state = 3;
            break;
          case '/':
            tagtype |= END;
            state = 8;
            break;
          default:
            tagtype |= START;
            tagname += tolower(c);
            state = 8;
            break;
        } break;
        
      // comment
      case 3: // <!?
        switch(c) {
          case '-':
            state = 4;
            break;
          default:
            text += "<!";
            text += c;
            state = 0;
        }
        break;
      case 4: // <!-?
        switch(c) {
          case '-':
            state = 5;
            break;
          default:
            text += "<!-";
            text += c;
            state = 0;
        }
        break;
      case 5: // <!-- ... ?
        if (c=='-')
          state = 6;
        break;
      case 6: // <!-- ... -?
        switch(c) {
          case '-':
            state = 7;
            break;
          default:
            state = 5;
        }
        break;
      case 7: // <!-- ... --?
        switch(c) {
          case '-':
            break;
          case '>':
            state = 0;
            break;
          default:
            state = 5;
        }
        break;
      
      // tag
      case 8: // <...?
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            state = 9;
            break;
          case '/':
            tagtype |= END;
            break;
          case '>':
            handleTag();
            state = 0;
            break;
          default:
            tagname += tolower(c);
        }
        break;
      case 9: // <... ?
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            break;
          case '/':
            tagtype |= END;
            break;
          case '>':
            handleTag();
            state = 0;
            break;
          default:
            name = tolower(c);
            value.clear();
            state = 10;
            break;
        }
        break;
      case 10: // <... ...?
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            state = 11;
            break;
          case '=':
            state = 12;
            break;
          case '/':
            tagtype |= END;
            break;
          case '>':
            parammap[name]=value;
            handleTag();
            state = 0;
          default:
            name += tolower(c);
        }
        break;
      case 11: // <... ... ?
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            break;
          case '=':
            state = 12;
            break;
          case '/':
            tagtype |= END;
            break;
          case '>':
            parammap[name] = value;
            handleTag();
            state = 0;
            break;
          default:
            parammap[name] = value;
            name = tolower(c);
            value.clear();
            state = 10;
            break;
        }
        break;
      case 12: // <... ...=?
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            break;
          case '\"':
            state = 14;
            break;
          case '/':
            tagtype |= END;
            break;
          case '>':
            parammap[name]=value;
            handleTag();
            state = 0;
            break;
          default:
            value+=c;
            state = 13;
        }
        break;
      case 13: // <... ...=...?
        switch(c) {
          case ' ':
          case '\n':
          case '\t':
            parammap[name]=value;
            state = 9;
            break;
          case '/':
            tagtype |= END;
            state = 9;
            break;
          case '>':
            parammap[name]=value;
            handleTag();
            state = 0;
            break;
          default:
            value+=c;
        }
      case 14: // <... ...="...?
        switch(c) {
          case '"':
            parammap[name]=value;
            state = 9;
            break;
          default:
            value+=c;
        }
        break;

      case 15: // &?
//cerr << "next char after & is " << (char)c << endl;
        entity = '&';
        entity += (char)c;
        switch(c) {
          case '#':
            text += "&#";
            state = 0;
            break;
          case ';':
            text += "&;";
            state = 0;
            break;
          default:
            if (c<'A' || c>'A'+58) {
//cerr << "char in entity is out of range" << endl;
              text += entity;
              state = 0;
              continue;
            }
            current_table = entity_table[current_table][c-'A'];
//cerr << "  new current table = " << current_table << endl;
            if (!current_table) {
              text += entity;
              state = 0;
            } else {
//cerr << "  switching to state 16\n";
              if (current_table>=156) {
                entity_tail = rest_strings[rest_pointer[current_table-156]];
                state = 17;
//cerr << "  current_table-156 = " << (current_table-156) << endl;
//cerr << "  rest_pointer[current_table-156] = " << rest_pointer[current_table-156] << endl;
//cerr << "  assume rest string to be: '"  << entity_tail << "'\n";
//cerr << "  current table '" << (char)current_table << "'\n";
//cerr << "  switching to state 17" << endl;
              } else {
                state = 16;
              }
            }
        }
        break;
      case 16:
//cerr << "next char after &... is " << (char)c << endl;
        entity += c;
        if (c<'A' || c>'A'+58 || c==';') {
//cerr << "  char in entity is out of range" << endl;
          text += '&';
          text += entity;
          state = 0;
          continue;
        }
        current_table = entity_table[current_table][c-'A'];
//cerr << "  new current table = " << current_table << endl;
        if (!current_table) {
//cerr << "  no table follows\n";
          text += entity;
          state = 0;
        }
        if (current_table>=156) {
          entity_tail = rest_strings[rest_pointer[current_table-156]];
          state = 17;
//cerr << "  current_table-156 = " << (current_table-156) << endl;
//cerr << "  rest_pointer[current_table-156] = " << rest_pointer[current_table-156] << endl;
//cerr << "  assume rest string to be: '"  << entity_tail << "'\n";
//cerr << "  current table '" << (char)current_table << "'\n";
//cerr << "  switching to state 17" << endl;
        }
        break;
      case 17:
//cerr << "next tail char is " << (char)c << endl;
        entity += c;
        if (c==';') {
//cerr << "  end of tail\n";
          if (*entity_tail==0) {
//cerr << "  matched\n";
             static const char out_of_order[]="\"&<>";
             if (current_table<160)
                text += utf8fromwchar(out_of_order[current_table-156]);
             else
                text += utf8fromwchar(current_table);
          } else {
//cerr << "  didn't match\n";
            text += entity;
          }
          state = 0;
        } else
        if (c==*entity_tail ) {
//cerr << "  still matching\n";
          ++entity_tail;
        } else {
//cerr << "  mismatch, abort\n";
          text += entity;
          state = 0;
        }
        break;
    }
  }
  text.clear();
  tagname.clear();
  parammap.clear();
}

/**
 * \param blank
 *    'true' in case the word was followed by a blank.
 */
void
TParser::handleText(bool blank)
{
  if (inside_body) {
    pout->push_back(new TEWord(text, blank));
  }
}

void
TParser::handleTag()
{
  if (tagname == "body") {
    inside_body = true;
  } else
  if (inside_body) {
    if (tagname == "br" && (tagtype & START)) {
      pout->push_back(new TEBreak());
    } else
    if (tagname == "hr" && (tagtype & START)) {
      pout->push_back(new TEHRule());
    } else
    if (tagname == "ul" && (tagtype & START)) {
      pout->push_back(new TEUnorderList(true));
    } else
    if (tagname == "ul" && (tagtype & END)) {
      pout->push_back(new TEUnorderList(false));
    } else
    if (tagname == "li" && (tagtype & START)) {
      pout->push_back(new TEListItem());
    } else
    if (tagname == "b" && (tagtype & START)) {
      pout->push_back(new TEBold(true));
    } else
    if (tagname == "b" && (tagtype & END)) {
      pout->push_back(new TEBold(false));
    } else
    if (tagname == "i" && (tagtype & START)) {
      pout->push_back(new TEItalic(true));
    } else
    if (tagname == "i" && (tagtype & END)) {
      pout->push_back(new TEItalic(false));
    } else
    if (tagname == "em" && (tagtype & START)) {
      pout->push_back(new TEItalic(true));
    } else
    if (tagname == "em" && (tagtype & END)) {
      pout->push_back(new TEItalic(false));
    } else
    if (tagname == "a" && (tagtype & START)) {
      TEAnchor *a = new TEAnchor(*this);
      aout->push_back(a);
      pout->push_back(a);
    } else
    if (tagname == "a" && (tagtype & END)) {
      pout->push_back(new TEAnchorEnd());
    } else
    if (tagname == "p" && (tagtype & START)) {
      pout->push_back(new TEParagraph(true, *this));
    } else
    if (tagname == "p" && (tagtype & END)) {
      pout->push_back(new TEParagraph(false, *this));
    } else
    if (tagname == "h1" && (tagtype & START)) {
      pout->push_back(new TEHeading(true, *this, 1));
    } else
    if (tagname == "h1" && (tagtype & END)) {
      pout->push_back(new TEHeading(false, *this, 1));
    } else
    if (tagname == "h2" && (tagtype & START)) {
      pout->push_back(new TEHeading(true, *this, 2));
    } else
    if (tagname == "h2" && (tagtype & END)) {
      pout->push_back(new TEHeading(false, *this, 2));
    } else
    if (tagname == "h3" && (tagtype & START)) {
      pout->push_back(new TEHeading(true, *this, 3));
    } else
    if (tagname == "h3" && (tagtype & END)) {
      pout->push_back(new TEHeading(false, *this, 3));
    } else
    if (tagname == "img" && (tagtype & START)) {
      pout->push_back(new TEImage(*this));
    } else
    if (tagname == "table" && (tagtype & START)) {
      TETable *t = new TETable(*this);
      pout->push_back(t);
      pushParsed();
      table = t;
    } else
    if (tagname == "table" && (tagtype & END)) {
      popParsed();
    } else
    if (tagname == "tr" && (tagtype & START)) {
      if (table)
        table->newRow();
    } else
    if (tagname == "td" && (tagtype & START)) {
      if (table)
        pout = table->newField(false);
    } else
    if (tagname == "th" && (tagtype & START)) {
      if (table)
        pout = table->newField(true);
    }
  }
//  cerr << '<' << tagname << '>';
}

class TBrowserMovement:
  public TUndo
{
    THTMLView *view;
    string oldurl;
  public:
    TBrowserMovement(THTMLView*, const string &oldurl);
    
    void undo();
};

TBrowserMovement::TBrowserMovement(THTMLView *view, const string &oldurl)
{
//cerr << "new browser movement event" << endl;
  this->view = view;
  this->oldurl = oldurl;
}

void
TBrowserMovement::undo()
{
//  cerr << "undo" << endl;
  view->open(oldurl);
}

static void
open_file(THTMLView *view)
{
  TFileDialog dlg(view, "Open File");
  dlg.addFileFilter("HTML (*.html, *.htm)");
  dlg.doModalLoop();
  if (dlg.getResult()==TMessageBox::OK) {
    cerr << __FILE__ << ":" << __LINE__ << ": not adding undo object" << endl;
/*
    TUndoManager::registerUndo((view,
                         new TBrowserMovement(view,
                                              view->getURL()));
*/
    view->open(dlg.getFilename());
  }
}

THTMLView::THTMLView(TWindow *parent, const string &title):
  TScrollPane(parent, title)
{
  parsed = 0;
  anchors = 0;
  stage1 = false;
  setSize(540,680);
  setMouseMoveMessages(TMMM_ALL);
  pane.set(0,0,getWidth(),getHeight());
  
  TAction *action = new TAction(this, "file|open");
  connect(action->sigActivate, open_file, this);
  
  new TUndoManager(this, "undomanager", "go|back", "go|forward");
}

THTMLView::~THTMLView()
{
  if (parsed) {
    parsed->clear();
    delete parsed;
  }
  if (anchors)
    delete anchors;
}

/**
 * Open a HTML page.
 */
bool
THTMLView::open(const string &url)
{
  iurlstream in;
  try {
    in.open(url);
  } catch(...) {
    string s =
      "<body>"
      "<h1>Error</h1>"
      "Failed to open the URL '<em>";
    s+=url;
    s+=
      "'</em>.</body>";
    istringstream in(s);
    parse(in);
    return false;
  }
  
  if (!in) {
    return false;
  }

  this->url = url;
  parse(in);
}

void
THTMLView::parse(istream &in)
{
  if (!parsed)  
    parsed = new TElementStorage();
  else
    parsed->clear();
    
  stage1 = false;

  if (!anchors)
    anchors = new TAnchorStorage();
  else
    anchors->erase(anchors->begin(), anchors->end());

  TParser parser;
  parser.parse(parsed, anchors, in, url);
  width = 0;
  if (isRealized()) {
    setPanePos(0, 0);
    doLayout();
    invalidateWindow(true);
  }
}

void
THTMLView::adjustPane()
{
  if (!parsed)
    return;

  invalidateWindow();

  stage1 = true;

  if (width!=getWidth()) {
    TElementStorage::iterator p, e;
    TPen pen(this);
    int bottom, w;
    w = width = getWidth();
    {
      TState state(width);
      state.output = false;
      // state.pen = &pen;

      p = parsed->begin();
      e = parsed->end();
      while(p!=e) {
        state.handle(pen, *p);
        ++p;
      }
      state.done();
      bottom = state.getBottom();
    }
    
    if (bottom > getHeight()) {
      w -= TScrollBar::getFixedSize();
      TState state(w);
      state.output = false;
      // state.pen = &pen;

      p = parsed->begin();
      e = parsed->end();
      while(p!=e) {
        state.handle(pen, *p);
        ++p;
      }
      state.done();
      bottom = state.getBottom();
    }

    pane.set(0,0, w, bottom);
  }
}

void
THTMLView::paint()
{
  if (!parsed) {
//    cerr << "  parsed == NULL\n";
    return;
  }
  if (!stage1) {
//    cerr << "  stage1 == false\n";
    return;
  }

  TPen pen(this);

  TElementStorage::iterator p, e;

#ifdef SPEEDUP_KLUDGE
int x, y, h;
getPanePos(&x, &y);
h = getHeight() + y;
int flag=0;
#endif

  TState state(pane.w);
  state.output = true;
  p = parsed->begin();
  e = parsed->end();
  while(p!=e) {

#ifdef SPEEDUP_KLUDGE
state.newline = false;
#endif

    state.handle(pen, *p);

#ifdef SPEEDUP_KLUDGE
if (state.newline && state.getBottom() > h) {
  flag++;
  if (flag>1)
    break;
}
#endif

    ++p;
  }
  
  paintCorner(pen);
}

void
THTMLView::mouseLDown(int x,int y, unsigned modifier)
{
  if (!anchors) {
    return;
  }
  TAnchorStorage::iterator p, e;
  p = anchors->begin();
  e = anchors->end();
  while(p!=e) {
    if ((*p)->polygon.isInside(x, y)) {
      setCursor(TCursor::DEFAULT);
//      cerr << "goto: '" << (*p)->href << "'\n";
      if ( !(*p)->href.empty()) {
cerr << __FILE__ << ":" << __LINE__ << ": not adding undo object" << endl;
#if 0
        TUndoManager::registerUndo(this,
                             new TBrowserMovement(this, url);
#endif
        open((*p)->href);
      }
      return;
    }
    ++p;
  }
}

void
THTMLView::mouseMove(int x,int y, unsigned modifier)
{
  if (!anchors) {
    return;
  }
  TAnchorStorage::iterator p, e;
  p = anchors->begin();
  e = anchors->end();
  while(p!=e) {
    if ((*p)->polygon.isInside(x, y)) {
      setCursor(TCursor::HAND);
      return;
    }
    ++p;
  }
  setCursor(TCursor::DEFAULT);
}

void
THTMLView::TElementStorage::clear()
{
  iterator p(begin()), e(end());
  while(p!=e) {
    delete *p;
    ++p;
  }
  erase(begin(), end());
}
