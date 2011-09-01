/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for X-Windows
 * Copyright (C) 1996-2005 by Mark-André Hopf <mhopf@mark13.org>
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
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define _TOAD_PRIVATE

#include "fischland.hh"
#include "fontdialog.hh"

#include <toad/toadbase.hh>
#include <toad/dialog.hh>
#include <toad/pushbutton.hh>
#include <toad/checkbox.hh>
#include <toad/radiobutton.hh>
#include <toad/textfield.hh>
#include <toad/table.hh>
#include <toad/config.h>
#include <toad/font.hh>
#include <toad/utf8.hh>
#include <set>

//#ifdef HAVE_LIBXFT

//#ifdef _XFT_NO_COMPAT_
//#undef _XFT_NO_COMPAT_
//#endif

#include <X11/Xft/Xft.h>

//#endif

using namespace toad;

typedef set<string> TFontFamilies;
TFontFamilies font_families;

class TFontFamilyAdapter:
  public TSimpleTableAdapter
{
    int width, height;
    TFontFamilies *families;
    
  public:
    TFontFamilyAdapter(TFontFamilies *ff) {
      families = ff;
      adjust();
    }
    void adjust() {
      TFont &font(TOADBase::getDefaultFont());
      height = font.getHeight();
      width = 0;
      for(TFontFamilies::iterator p = families->begin();
          p != families->end();
          ++p)
      {
        int w = font.getTextWidth(*p);
        if (w>width)
          width = w;
      }
    }
    size_t getRows() {
      return families->size();
    }
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = width;
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = height;
          break;
        case TTableEvent::PAINT:
          renderBackground(te);
          int row = te.row;
          TFontFamilies::iterator p = families->begin();
          while(row>0) {
            --row;
            ++p;
          }
          te.pen->drawString(0,0,*p);
          renderCursor(te);
          break;
      }
    }
};

struct fontstyle_t {
  fontstyle_t(string name, int weight, int slant, int width) {
    this->name   = name;
    this->weight = weight;
    this->slant  = slant;
    this->width  = width;
  }
  string name;
  int weight, slant, width;
};

class TFontDialog::TFontStyles:
  public vector<fontstyle_t>
{};

class TFontDialog::TFontStyleAdapter:
  public TSimpleTableAdapter
{
    int width, height;
    TFontDialog::TFontStyles *styles;
    
  public:
    TFontStyleAdapter(TFontDialog::TFontStyles *fs) {
      styles = fs;
      adjust();
    }
    void adjust() {
      TFont &font(TOADBase::getDefaultFont());
      height = font.getHeight();
      width = 0;
      for(TFontStyles::iterator p = styles->begin();
          p != styles->end();
          ++p)
      {
        int w = font.getTextWidth(p->name);
        if (w>width)
          width = w;
      }
    }
    size_t getRows() {
      return styles->size();
    }
    void tableEvent(TTableEvent &te) {
      switch(te.type) {
        case TTableEvent::GET_COL_SIZE:
          te.w = width; 
          break;
        case TTableEvent::GET_ROW_SIZE:
          te.h = height;
          break;
        case TTableEvent::PAINT:
          renderBackground(te);
          te.pen->drawString(0,0,styles->at(te.row).name);
          renderCursor(te);
          break;
      }
    }
};

// XftFont: FcCharSet, FcPattern

TFontDialog::TFontDialog(TWindow *parent, const string &title):
  TDialog(parent, title)
{
  if (font_families.empty()) {
    FcFontSet *fl;
    TFontManager *fm = TFontManager::getDefault();
    if (fm->getName() == "x11") {
      fl = fm->getFcFontSet();
    } else
    if (TFontManager::getDefault()->getName() == "freetype") {
        fl = XftListFonts(x11display, x11screen,
                          0,
                          XFT_FAMILY, 0);
    }    
    for(int i=0; i<fl->nfont; ++i) {
      FcChar8 *s;
      FcPatternGetString(fl->fonts[i], FC_FAMILY, 0, &s);
      font_families.insert((char*)s);
    }
    if (fm->getName() != "x11")
      FcFontSetDestroy(fl);
  }
  
  result = TMessageBox::ABORT;

  family = new TTable(this, "family");
  family->setAdapter(new TFontFamilyAdapter(&font_families));
  connect(family->sigCursor, this, &This::familySelected);

  style = new TTable(this, "style");
  font_styles = new TFontStyles();
  style_rndr = new TFontStyleAdapter(font_styles);
  style->setAdapter(style_rndr);
  connect(style->sigCursor, this, &This::styleSelected);

  font_size.setRangeProperties(12, 0, 1, 9600);
  new TTextField(this, "size", &font_size);
  connect(font_size.sigChanged, this, &This::updateFont);

  example = new TTextField(this, "example");
  example->setValue("ABCDEFG abcdefgh 012345 どらごばる");

  TPushButton *pb;
  pb = new TPushButton(this, "ok");
  connect(pb->sigClicked, this, &This::button, TMessageBox::OK);
  pb = new TPushButton(this, "abort");
  connect(pb->sigClicked, this, &This::button, TMessageBox::ABORT);
  pb = new TPushButton(this, "fonttable");
  connect(pb->sigClicked, this, &This::showFontTable);

  loadLayout(RESOURCE("fontdialog.atv"));

  familySelected();
}

TFontDialog::~TFontDialog()
{
  delete font_styles;
}

void
TFontDialog::button(unsigned result)
{
  this->result = result;
  destroyWindow();
}

void
TFontDialog::setFont(const string &fontname)
{
  int i;
  font_name = fontname;
  FcPattern *pattern = FcNameParse((FcChar8*)fontname.c_str());
  FcPattern *found;
  FcResult result;

  TFontManager *fm = TFontManager::getDefault();
  if (fm->getName() == "x11") {
      //FcConfigSubstitute(fm->getFcConfig(), pattern, FcMatchPattern);
      //FcDefaultSubstitute(pattern);
      FcFontSet *fs = fm->getFcFontSet();
      found = FcFontSetMatch(0,
                             &fs, 1,
                             pattern, &result);
  } else
  if (TFontManager::getDefault()->getName() == "freetype") {
      XftDefaultSubstitute(x11display, x11screen, pattern);
      found = XftFontMatch(x11display, x11screen, pattern, &result);
  }
  if (!found) {
    cerr << "no font found for '" << fontname << "'\n";
    FcPatternDestroy(pattern);
    return;
  }
  
  FcChar8* s;
  int slant, weight, width;
  double size;
  FcPatternGetString(found, FC_FAMILY, 0, &s);
  FcPatternGetInteger(found, FC_WEIGHT, 0, &weight);
  FcPatternGetInteger(found, FC_SLANT,  0, &slant);
  FcPatternGetInteger(found, FC_WIDTH,  0, &width);
  FcPatternGetDouble(pattern, FC_SIZE, 0, &size);
  FcPatternDestroy(pattern);
  
  family_name = (char*)s;
  i = 0;
  for(TFontFamilies::iterator p = font_families.begin();
      p!=font_families.end();
      ++p, ++i)
  {
    if (strcmp((char*)s, p->c_str())==0) {
      family->setCursor(0, i);
      familySelected();
      break;
    }
  }
  
  i = 0;
  for(TFontStyles::iterator p = font_styles->begin();
      p!=font_styles->end();
      ++p, ++i)
  {
    if (p->weight==weight && p->slant==slant && p->width==width) {
      style->setCursor(0, i);
      style->selectAtCursor();
      break;
    }
  }
  
  font_size = static_cast<int>(size);
  example->getPreferences()->setFont(font_name);
}

struct width_t {  
  const char *x11;
  int fc;
};

static width_t widths[] = {
  { "condensed",        FC_WIDTH_CONDENSED },
  { "extended",         FC_WIDTH_EXPANDED }, 
  { "expanded",         FC_WIDTH_EXPANDED }, 
  { "narrow",           FC_WIDTH_CONDENSED },
  { "normal",           FC_WIDTH_NORMAL },
  { "semicondensed",    FC_WIDTH_SEMICONDENSED }, 
  { "semi condensed",   FC_WIDTH_SEMICONDENSED }, 
  { "extracondensed",   FC_WIDTH_EXTRACONDENSED },
  { "extra condensed",  FC_WIDTH_EXTRACONDENSED },
  { "ultracondensed",   FC_WIDTH_ULTRACONDENSED },
  { "ultra condensed",  FC_WIDTH_ULTRACONDENSED },
  { "semiexpanded",     FC_WIDTH_SEMIEXPANDED },  
  { "semi expanded",    FC_WIDTH_SEMIEXPANDED },  
  { "extraexpanded",    FC_WIDTH_EXTRAEXPANDED },
  { "extra expanded",   FC_WIDTH_EXTRAEXPANDED },
  { "ultraexpanded",    FC_WIDTH_ULTRAEXPANDED },
  { "ultra expanded",   FC_WIDTH_ULTRAEXPANDED }
};

struct weight_t {
  const char *x11;
  int fc;
};

#ifndef FC_WEIGHT_BOOK       
#define FC_WEIGHT_BOOK 75
#endif
#ifndef FC_DUAL
#define FC_DUAL 90
#endif

static weight_t weights[] = {
  { "thin",         FC_WEIGHT_THIN },
  { "extralight",   FC_WEIGHT_EXTRALIGHT },
  { "extra light",  FC_WEIGHT_EXTRALIGHT },
  { "ultralight",   FC_WEIGHT_ULTRALIGHT },
  { "ultra light",  FC_WEIGHT_ULTRALIGHT },
  { "light",        FC_WEIGHT_LIGHT },
  { "book",         FC_WEIGHT_BOOK },
  { "regular",      FC_WEIGHT_REGULAR },
  { "normal",       FC_WEIGHT_NORMAL },
  { "medium",       FC_WEIGHT_MEDIUM },
  { "demi",         FC_WEIGHT_DEMIBOLD },
  { "demibold",     FC_WEIGHT_DEMIBOLD },
  { "demi bold",    FC_WEIGHT_DEMIBOLD },
  { "semibold",     FC_WEIGHT_SEMIBOLD },
  { "semi bold",    FC_WEIGHT_SEMIBOLD },
  { "bold",         FC_WEIGHT_BOLD },
  { "extrabold",    FC_WEIGHT_EXTRABOLD },
  { "extra bold",   FC_WEIGHT_EXTRABOLD },
  { "ultrabold",    FC_WEIGHT_ULTRABOLD },
  { "ultra bold",   FC_WEIGHT_ULTRABOLD },
  { "ultrablack",   FC_WEIGHT_ULTRABOLD },
  { "ultra black",  FC_WEIGHT_ULTRABOLD },
  { "black",        FC_WEIGHT_BLACK  },
  { "heavy",        FC_WEIGHT_HEAVY }
};

struct slant_t {  
  const char *x11;
  int fc;
  const char *text;
};

static slant_t slants[] = {
  { "", FC_SLANT_ROMAN, "roman" },    
  { "r", FC_SLANT_ROMAN, "roman" },   
  { "i", FC_SLANT_ITALIC, "italic" }, 
  { "o", FC_SLANT_OBLIQUE, "oblique" }
};

void
TFontDialog::familySelected()
{
  family->selectAtCursor();
  int row = family->getSelectionModel()->getRow();
  TFontFamilies::iterator p = font_families.begin();
  while(row>0 && p != font_families.end()) {
    --row;
    ++p;
  }
  if (p == font_families.end()) {
    cerr << "can not select family in row " << row << ", no such row" << endl;
    return;
  }
  //cerr << "family: " << *p << endl;
  family_name = *p;

  FcFontSet *fl;
  TFontManager *fm = TFontManager::getDefault();
  if (fm->getName() == "x11") {
      FcPattern *pattern = FcPatternCreate();
      FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)p->c_str());
      FcObjectSet *os = FcObjectSetCreate();
      FcObjectSetAdd(os, FC_SLANT);
      FcObjectSetAdd(os, FC_WEIGHT);
      FcObjectSetAdd(os, FC_WIDTH);
      fl = FcFontList(fm->getFcConfig(), pattern, os);
      FcObjectSetDestroy(os);
      FcPatternDestroy(pattern);
  } else
  if (TFontManager::getDefault()->getName() == "freetype") {
      fl = XftListFonts(x11display, x11screen,
           FC_FAMILY, XftTypeString, p->c_str(), 0,
           FC_SLANT, FC_WEIGHT, FC_WIDTH, 0);
  }


  
  int slant, weight, width;
  
  font_styles->clear();
  
  for(int i=0; i<fl->nfont; ++i) {
    FcChar8 *s;
    FcPatternGetInteger(fl->fonts[i], FC_WEIGHT, 0, &weight);
    FcPatternGetInteger(fl->fonts[i], FC_SLANT,  0, &slant);
    FcPatternGetInteger(fl->fonts[i], FC_WIDTH,  0, &width);
    
    string style;

    if (weight != FC_WEIGHT_NORMAL) {
      for(int j=0; j<sizeof(weights)/sizeof(weight_t); ++j) {
        if(weights[j].fc == weight) {
          // cerr << "weight: " << weights[j].x11 << endl;
          if (!style.empty())
            style += ' ';
          style += weights[j].x11;
          break;
        }
      }
    }
    if (slant != FC_SLANT_ROMAN) {
      for(int j=0; j<sizeof(slants)/sizeof(slant_t); ++j) {
        if (slants[j].fc == slant) {
          // cerr << "  slant: " << slants[j].text << endl;
          if (!style.empty()) {
            style += ' ';
          }
          style += slants[j].text;
          break;
        }
      }
    }
    if (width != FC_WIDTH_NORMAL) {   
      for(int j=0; j<sizeof(widths)/sizeof(width_t); ++j) {
        if(widths[j].fc == width) {
          // cerr << "width: " << widths[j].x11 << endl;
          if (!style.empty()) {
            style += ' ';
          }
          style += widths[j].x11;
          break;
        }
      }
    }
    if (style.empty())
      style = "normal";
    //cerr << "  style: " << style << endl;
    font_styles->push_back(fontstyle_t(style, weight, slant, width));
  }
  FcFontSetDestroy(fl);

  style_rndr->adjust();
  style_rndr->sigChanged();
  styleSelected();
}

void
TFontDialog::styleSelected()
{
  style->selectAtCursor();
  updateFont();
}

void
TFontDialog::updateFont()
{
  FcPattern *pattern = FcPatternCreate();
  FcPatternAddString(pattern, FC_FAMILY, (FcChar8*)family_name.c_str());
  
  int row = style->getSelectionModel()->getRow();
  if (row>=font_styles->size()) {
    cerr << "font style row " << row << " larger than " << font_styles->size() << endl;
    return;
  }
  FcPatternAddInteger(pattern, FC_WEIGHT, (*font_styles)[row].weight);
  FcPatternAddInteger(pattern, FC_SLANT , (*font_styles)[row].slant);
  FcPatternAddInteger(pattern, FC_WIDTH , (*font_styles)[row].width);
  FcPatternAddInteger(pattern, FC_SIZE, font_size);
  
  //cerr << (char*)FcNameUnparse(pattern) << endl;
  font_name = (char*)FcNameUnparse(pattern);
  invalidateWindow();
  
  FcPatternDestroy(pattern);
  
  example->getPreferences()->setFont(font_name);
}

class TFontTable:
  public TDialog
{
  public:
    TFontTable(TWindow *parent, const string &title, const string &font);
    void paint();
    
    void table(int dx);
    int ct;
    string font;
};

void
TFontDialog::showFontTable()
{
  TFontTable dlg(this, "Font Table", font_name);
  dlg.doModalLoop();
}

TFontTable::TFontTable(TWindow *parent, const string &title, const string &aFont):
  TDialog(parent, title), font(aFont)
{
  setLayout(0);
  setSize(16*24+10, 16*24+30+10);
  
  TPushButton *pb;
  
  pb = new TPushButton(this, "<");
  connect(pb->sigClicked, this, &TFontTable::table, -1);
  pb->setShape(5,5,80,25);

  pb = new TPushButton(this, ">");
  connect(pb->sigClicked, this, &TFontTable::table, 1);
  pb->setShape(5+80+5,5,80,25);
  
  ct = 0;
  table(0);
}

void
TFontTable::table(int dx)
{
  ct+=dx;
  if (ct<0)
    ct=0;
  if (ct>255)
    ct=255;
  invalidateWindow();
}

void
TFontTable::paint()
{
  TPen pen(this);
  pen.setFont(font);
  
  unsigned i, j, x, y, c;
  int s = 24;
#if 0
cerr << "table: " << ct << endl;
if (font.x11fs) {
  cerr << "  first char: " << font.x11fs->min_char_or_byte2 << endl;
  cerr << "  last  char: " << font.x11fs->max_char_or_byte2 << endl;
  cerr << "  first row : " << font.x11fs->min_byte1 << endl;
  cerr << "  last row  : " << font.x11fs->max_byte1 << endl;
  cerr << "  direction : " << font.x11fs->direction << endl;
  cerr << "  all_chars_exist: " << font.x11fs->all_chars_exist << endl;
  cerr << "  default char   : " << font.x11fs->default_char << endl;
}
#endif
  pen.translate(5,35);
  
  x = y = 0;
  c = 0;
  
  pen.setColor(226, 145, 145);
  for(i=0; i<16; ++i) {
    for(j=0; j<16; ++j) {
      unsigned word = (ct<<8) + c;
      if (word<=0x007f) {
        pen.fillRectangle(x, y, s, s);
      }
      x+=s;
      c++;
    }
    x=0;
    y+=s;
  }
  pen.setColor(0, 0, 0);
  
  x = y = 0;
  c = 0;
  for(i=0; i<16; ++i) {
    pen.drawLine(i*s,0,i*s,s*16);
    pen.drawLine(0,i*s,s*16,i*s);
    for(j=0; j<16; ++j) {
#if 1
      pen.drawString(x+2, y+2, utf8fromwchar((ct<<8)+c));
#endif
#if 0
      XChar2b buffer[2];
      buffer[0].byte1 = ct;
      buffer[0].byte2 = c;
      drawString16(pen, font, xftdraw, x+2, y+2, buffer, 1);
#endif
      x+=s;
      c++;
    }
    x=0;
    y+=s;
  }
  pen.drawLine(i*s,0,i*s,s*16);
  pen.drawLine(0,i*s,s*16,i*s);
}
